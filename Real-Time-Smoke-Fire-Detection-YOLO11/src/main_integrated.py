"""
SFFS Integrated AI Module — Main Orchestrator
==============================================
Coordinates fire/smoke detection, occupancy tracking, ESP32 sensor fusion,
MQTT command publishing, and Telegram/GSM-SMS alerting in one real-time loop.

Usage:
    cd src/
    python main_integrated.py

Frame pipeline (B3-safe):
    raw frame ─┬─► fire_detector.detect()      (clean frame, no draw)
               └─► occupancy.update()          (clean frame, tracks + draws persons)
                        │
                        ▼
               fire_detector.draw() + HUD      (single final visualization pass)

Threading:
    Main  → capture, inference, fusion, display
    MQTT  → paho background loop
    Notif → ThreadPoolExecutor dispatch
"""

import http

import cv2
import time
import sys
import logging
import threading
import winsound
from pathlib import Path

from config import Config, setup_logging
from fire_detector import Detector, EMPTY_RESULT
from occupancy_tracker import OccupancyTracker
from decision_engine import DecisionEngine, SystemState
from mqtt_client import SffsMqttClient
from notification_service import NotificationService


# Severity ordering for B4 escalation logic (higher = more dangerous).
STATE_PRIORITY = {
    SystemState.SAFE:         0,
    SystemState.SENSOR_ALERT: 1,
    SystemState.FIRE:         2,
}

STATUS_REPUBLISH_INTERVAL = 5.0   # B1: refresh retained command every 5s
PEOPLE_PUBLISH_INTERVAL   = 5.0

# Audible siren (winsound — Windows native, no extra dependencies).
SIREN_FREQUENCY_HZ = 2000
SIREN_DURATION_MS  = 800
SIREN_REPEAT_INTERVAL = 1.0   # seconds between beeps while FIRE persists


def _play_siren():
    """Blocking Beep() run on a daemon thread so it never stalls the loop."""
    try:
        winsound.Beep(SIREN_FREQUENCY_HZ, SIREN_DURATION_MS)
    except RuntimeError:
        pass


# ==========================================
# Initialization
# ==========================================
def select_device():
    """Interactive GPU/CPU selection. Returns 'cuda' or 'cpu' (CUDA-verified)."""
    print("\n╔══════════════════════════════════════════════╗")
    print("║  🔥 SFFS — AI Vision Module Setup            ║")
    print("╠══════════════════════════════════════════════╣")
    print("║  1. Run on GPU (CUDA FP16 - Recommended)     ║")
    print("║  2. Run on CPU (ONNX / Standard)             ║")
    print("╚══════════════════════════════════════════════╝")
    choice = input("  Select device (1 or 2): ").strip()

    if choice == "1":
        try:
            import torch
            if torch.cuda.is_available():
                print("  → System starting on GPU (FP16)...\n")
                return "cuda"
            print("  ⚠ CUDA not available — falling back to CPU.\n")
        except Exception as e:
            print(f"  ⚠ CUDA check failed ({e}) — falling back to CPU.\n")
        return "cpu"

    print("  → System starting on CPU...\n")
    return "cpu"


def init_mqtt():
    """Initialize MQTT with graceful failure (camera-only mode if broker down)."""
    logger = logging.getLogger(__name__)
    mqtt_client = SffsMqttClient(
        broker_host=Config.MQTT_BROKER_HOST,
        broker_port=Config.MQTT_BROKER_PORT,
    )
    if mqtt_client.connect():
        logger.info("[Init] MQTT connected. ESP32 data bridge ACTIVE.")
    else:
        logger.warning(
            "[Init] MQTT unavailable. CAMERA-ONLY mode "
            "(sensor fusion & actuator control DISABLED)."
        )
    return mqtt_client


# ==========================================
# Display helpers
# ==========================================
def build_alert_caption(state, inside_count, sensor_data, confidence):
    """Rich alert caption for Telegram alerts."""
    header = {
        SystemState.FIRE:         "🔥 FIRE",
        SystemState.SENSOR_ALERT: "⚠️ SENSOR ALERT",
    }.get(state, "⚠️ ALERT")

    lines = [
        f"🚨 {header} (confidence: {confidence:.0%})",
        f"👥 Occupants inside: {inside_count}",
    ]
    if sensor_data:
        gas = sensor_data.get("gas", {})
        env = sensor_data.get("env", {})
        lines.append(
            f"📊 Temp: {env.get('temp', 'N/A')}°C | "
            f"MQ2: {gas.get('mq2', 'N/A')} | "
            f"MQ5: {gas.get('mq5', 'N/A')} | "
            f"MQ7: {gas.get('mq7', 'N/A')}"
        )
    else:
        lines.append("📊 ESP32 sensors: Offline")
    return "\n".join(lines)


# ==========================================
# 4-quadrant zonal room mapping (camera-frame)
# ==========================================
# NOTE: this room ID is a CAMERA-FRAME quadrant label used only for the live
# HUD. It is a vision-side spatial cue and is independent of the firmware's
# physical room->sensor mapping in config.h.
def draw_quadrants(frame):
    """Draw a faint gray crosshair that splits the frame into 4 equal quadrants.
    Returns the (Cx, Cy) midpoint where the lines intersect."""
    h, w = frame.shape[:2]
    cx, cy = w // 2, h // 2
    gray = (160, 160, 160)  # faint gray (BGR)
    cv2.line(frame, (cx, 0), (cx, h), gray, 1, cv2.LINE_AA)   # vertical divider
    cv2.line(frame, (0, cy), (w, cy), gray, 1, cv2.LINE_AA)   # horizontal divider
    return cx, cy


def room_from_box(box, cx, cy):
    """Map a detection bounding-box centroid to a quadrant room ID (1-4).
    Image coords: y grows downward, so 'bottom' means y >= cy."""
    x1, y1, x2, y2 = box
    bx = (int(x1) + int(x2)) // 2
    by = (int(y1) + int(y2)) // 2
    if bx < cx and by >= cy:
        return 1   # bottom-left
    if bx >= cx and by >= cy:
        return 2   # bottom-right
    if bx < cx and by < cy:
        return 3   # top-left
    return 4       # top-right


def draw_system_hud(frame, state, inside_count, esp32_online, mqtt_connected, fps,
                    active_rooms=None):
    """HUD overlay: state badge, connectivity, FPS, occupancy.
    When active_rooms is non-empty the top-right badge reads 'FIRE - ROOM X[, Y]';
    otherwise it shows the fused system state (e.g. 'SAFE')."""
    h, w = frame.shape[:2]

    # Top-right status string: explicit quadrant room IDs on visual detection.
    if active_rooms:
        badge_text = "FIRE - ROOM " + ", ".join(str(r) for r in sorted(active_rooms))
        badge_color = (0, 0, 255)
    else:
        badge_text = state.value
        badge_color = {
            SystemState.SAFE:         (0, 180, 0),
            SystemState.SENSOR_ALERT: (0, 165, 255),
            SystemState.FIRE:         (0, 0, 255),
        }.get(state, (255, 255, 255))

    # Dynamic-width badge so long "FIRE - ROOM 1, 2, ..." strings always fit.
    font, fscale, fthick = cv2.FONT_HERSHEY_SIMPLEX, 0.6, 2
    (tw, _th), _ = cv2.getTextSize(badge_text, font, fscale, fthick)
    badge_w = max(250, tw + 24)
    bx0 = w - badge_w - 10

    overlay = frame.copy()
    cv2.rectangle(overlay, (bx0 - 5, 5), (w - 5, 120), (0, 0, 0), -1)
    cv2.addWeighted(overlay, 0.6, frame, 0.4, 0, frame)

    cv2.rectangle(frame, (bx0, 8), (w - 10, 38), badge_color, -1)
    cv2.putText(frame, badge_text, (bx0 + 8, 30), font, fscale, (255, 255, 255), fthick)

    mqtt_text  = "MQTT: CONNECTED" if mqtt_connected else "MQTT: OFFLINE"
    esp_text   = "ESP32: ONLINE" if esp32_online else "ESP32: OFFLINE"
    mqtt_color = (0, 255, 0) if mqtt_connected else (0, 0, 255)
    esp_color  = (0, 255, 0) if esp32_online else (0, 0, 255)

    cv2.putText(frame, mqtt_text, (bx0 + 7, 60), font, 0.45, mqtt_color, 1)
    cv2.putText(frame, esp_text, (bx0 + 7, 80), font, 0.45, esp_color, 1)
    cv2.putText(frame, f"FPS: {fps:.1f}", (bx0 + 7, 100), font, 0.45, (255, 255, 255), 1)

    # Bottom-left "People Inside" overlay removed to keep the lower margin clean;
    # the live occupancy count remains shown top-left by the occupancy tracker.


# ==========================================
# B4: per-severity alert dispatcher
# ==========================================
class AlertGate:
    """
    Decouples alerting from one-shot state edges.
      - Each dangerous state has its own cooldown.
      - Escalation to a higher severity bypasses any active cooldown.
      - Returning to SAFE resets the high-water mark so the next danger fires
        immediately.
    """

    def __init__(self, cooldown):
        self.cooldown = cooldown
        self._last_alert_time = {}     # SystemState -> ts
        self._peak_priority = 0

    def should_alert(self, state, now):
        if state == SystemState.SAFE:
            self._peak_priority = 0
            return False

        priority = STATE_PRIORITY.get(state, 0)
        escalated = priority > self._peak_priority
        due = (now - self._last_alert_time.get(state, 0.0)) >= self.cooldown

        if escalated or due:
            self._last_alert_time[state] = now
            self._peak_priority = max(self._peak_priority, priority)
            return True
        return False


# ==========================================
# Main
# ==========================================
def main():
    setup_logging()
    logger = logging.getLogger(__name__)
    logger.info("🔥 ===== SFFS AI Module — Integrated Mode =====")

    device = select_device()
    is_gpu = (device == "cuda")

    # GPU runs fire detection every frame; CPU skips to keep FPS. Person
    # tracking ALWAYS runs every frame to preserve persistent IDs.
    fire_skip_interval = 1 if is_gpu else 3
    if not is_gpu:
        logger.info(
            f"[CPU Mode] Fire detection every {fire_skip_interval} frames; "
            f"tracking every frame."
        )

    # --- Modules ---
    mqtt_client = init_mqtt()

    fire_detector = Detector(
        Config.MODEL_PATH, device=device, imgsz=640, iou_threshold=0.20
    )

    person_model_path = Path(__file__).parent / "yolo11n.pt"
    occupancy = OccupancyTracker(
        model_path=str(person_model_path),
        line_x=Config.OCCUPANCY_LINE_X,
        confidence=0.5,
        device=device,
    )

    decision = DecisionEngine()
    notification = NotificationService(Config)
    alert_gate = AlertGate(Config.ALERT_COOLDOWN)
    logger.info("[Init] All modules initialized.")

    # --- Capture ---

    cap = cv2.VideoCapture(Config.VIDEO_SOURCE_ID)
    if not cap.isOpened():
        logger.critical(f"[Init] FATAL: cannot open camera {Config.VIDEO_SOURCE_ID}")
        sys.exit(1)
    logger.info(f"[Init] Camera opened: source={Config.VIDEO_SOURCE_ID}")

    # --- Loop state ---
    frame_count = 0
    fire_result = EMPTY_RESULT
    last_status_publish = 0.0
    last_people_publish = 0.0
    last_siren_time = 0.0
    last_published_detection = None
    manual_override_prev = False
    reported_unknown_classes = set()   # zoning diagnostic: log each unknown class once
    fps_timer = time.time()
    fps = 0.0

    logger.info("🚀 Entering main detection loop (press 'Q' to quit)...\n")

    try:
        while True:
            ret, frame = cap.read()
            if not ret:
                logger.warning("[Loop] Camera read failed. Retrying...")
                time.sleep(0.1)
                continue

            frame_count += 1
            now = time.time()

            # Per-frame zonal state: reset the active fire/smoke room set at the
            # very start of every iteration so it never carries stale rooms over.
            fire_rooms = set()

            if frame_count % 30 == 0:
                elapsed = now - fps_timer
                fps = 30.0 / elapsed if elapsed > 0 else 0.0
                fps_timer = now

            # --- A. Fire/smoke detection on the CLEAN raw frame ---
            if is_gpu or (frame_count % fire_skip_interval == 0):
                fire_result = fire_detector.detect(frame)
            detection = fire_result.label

            # --- A2. Publish raw detection events (dashboard/audit) ---
            if detection != last_published_detection and mqtt_client.is_connected():
                if detection is not None:
                    mqtt_client.publish_detection(detection, fire_result.confidence)
                    logger.info(
                        f"[Detection] {detection} (conf={fire_result.confidence:.2f})"
                    )
                elif last_published_detection is not None:
                    mqtt_client.publish_detection("Clear", 0.0)
                last_published_detection = detection

            # --- B. People tracking on the CLEAN raw frame (every frame) ---
            #     occupancy.update draws persons + line and returns the canvas.
            annotated_frame, inside_count = occupancy.update(frame)

            # --- C. Manual override: physical button intercepts the AI loop ---
            # The ESP32 publishes a top-level "manual_trigger" (1/0) inside the
            # sffs/sensors/data payload. get_sensor_data() is lock-guarded and
            # returns the latest cached packet without blocking this loop.
            # Accept int / bool / string forms defensively.
            sensor_data = mqtt_client.get_sensor_data()
            raw_manual = sensor_data.get("manual_trigger") if sensor_data else None
            manual_override = str(raw_manual).strip().lower() in ("1", "true", "yes")
            if manual_override and not manual_override_prev:
                logger.warning(
                    "[Manual Override] Manual Button pressed! "
                    "Bypassing AI loop, forcing Emergency State."
                )
            manual_override_prev = manual_override

            # --- C2. Sensor fusion (manual override forces CONFIRMED FIRE) ---
            state, state_changed, actions, confidence, source = decision.evaluate(
                detection, sensor_data, manual_override=manual_override
            )

            # --- D. B1: publish retained command on change OR every 5s ---
            if mqtt_client.is_connected() and (
                state_changed or (now - last_status_publish >= STATUS_REPUBLISH_INTERVAL)
            ):
                mqtt_client.publish_system_status(
                    state=state.value, confidence=confidence,
                    source=source, actions=actions, retain=True,
                )
                last_status_publish = now
                if state_changed:
                    logger.info(f"[MQTT] state={state.value} actions={actions}")

            # --- E. People count every 5s ---
            if mqtt_client.is_connected() and (now - last_people_publish >= PEOPLE_PUBLISH_INTERVAL):
                mqtt_client.publish_people_count(inside_count)
                last_people_publish = now

            # --- F. B4: alert via per-severity gate (escalation bypasses cooldown) ---
            if alert_gate.should_alert(state, now):
                caption = build_alert_caption(state, inside_count, sensor_data, confidence)
                notification.send_alert(annotated_frame, caption)
                logger.warning(f"[Alert] Dispatched:\n{caption}")

            # --- F2: audible siren while FIRE is active (non-blocking) ---
            if state == SystemState.FIRE and (now - last_siren_time >= SIREN_REPEAT_INTERVAL):
                threading.Thread(target=_play_siren, daemon=True).start()
                logger.info("[Alert] Audible siren activated via winsound.")
                last_siren_time = now

            # --- G. Single final visualization pass: zoning + fire boxes + HUD ---
            # Draw the 4-quadrant crosshair, then map every accepted fire/smoke
            # detection centroid to a quadrant room ID. This population MUST run
            # before draw_system_hud so the HUD receives the up-to-date room set.
            cx, cy = draw_quadrants(annotated_frame)
            for item in fire_result.drawables:
                # Defensive unpack: tolerate extra trailing fields (e.g. a track
                # ID) so a drawable-structure desync can never break zoning.
                try:
                    box, class_name, conf = item[0], item[1], float(item[2])
                except (TypeError, IndexError, ValueError):
                    print(f"[ZONING][WARN] Unrecognized drawable structure -> {item!r}")
                    continue

                name = str(class_name).lower()
                is_fire = "fire" in name
                is_smoke = "smoke" in name

                if (is_fire and conf >= fire_detector.min_confidence) or \
                   (is_smoke and conf >= fire_detector.smoke_confidence):
                    fire_rooms.add(room_from_box(box, cx, cy))
                elif not (is_fire or is_smoke):
                    # The box class is neither fire nor smoke: dump it ONCE per run
                    # so the exact data format can be diagnosed without flooding.
                    if name not in reported_unknown_classes:
                        reported_unknown_classes.add(name)
                        print(f"[ZONING][WARN] Unmapped detection class={class_name!r} "
                              f"conf={conf:.2f} box={list(map(int, box))}")

            fire_detector.draw(annotated_frame, fire_result)
            # Pass the populated active-room set explicitly into the HUD.
            draw_system_hud(
                annotated_frame, state=state, inside_count=inside_count,
                esp32_online=mqtt_client.is_esp32_online(),
                mqtt_connected=mqtt_client.is_connected(), fps=fps,
                active_rooms=fire_rooms,
            )

            cv2.imshow("SFFS - Smart Fire Fighting System", annotated_frame)
            if cv2.waitKey(1) & 0xFF == ord("q"):
                logger.info("[Loop] User shutdown (Q).")
                break

    except KeyboardInterrupt:
        logger.info("[Loop] Shutdown via Ctrl+C.")
    except Exception as e:
        logger.critical(f"[Loop] Unhandled error: {e}", exc_info=True)
    finally:
        logger.info("Shutting down all subsystems...")
        cap.release()
        cv2.destroyAllWindows()
        mqtt_client.disconnect()
        notification.cleanup()
        logger.info("🛑 SFFS AI Module shutdown complete.\n")


if __name__ == "__main__":
    main()
