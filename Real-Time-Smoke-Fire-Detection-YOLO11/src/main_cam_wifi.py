"""
SFFS Integrated AI Module — Main Orchestrator (WiFi Robust Byte-Stream Version)
==============================================
Coordinates fire/smoke detection, occupancy tracking, ESP32 sensor fusion,
MQTT command publishing, and Telegram/GSM-SMS alerting in one real-time loop.
"""

import http
import cv2
import time
import sys
import logging
import threading
import winsound
import numpy as np
import urllib.request
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


def draw_system_hud(frame, state, inside_count, esp32_online, mqtt_connected, fps):
    """HUD overlay: state badge, connectivity, FPS, occupancy."""
    h, w = frame.shape[:2]

    badge_color = {
        SystemState.SAFE:         (0, 180, 0),
        SystemState.SENSOR_ALERT: (0, 165, 255),
        SystemState.FIRE:         (0, 0, 255),
    }.get(state, (255, 255, 255))

    overlay = frame.copy()
    cv2.rectangle(overlay, (w - 260, 5), (w - 5, 120), (0, 0, 0), -1)
    cv2.addWeighted(overlay, 0.6, frame, 0.4, 0, frame)

    cv2.rectangle(frame, (w - 255, 8), (w - 10, 38), badge_color, -1)
    cv2.putText(frame, f"STATE: {state.value}", (w - 248, 30),
                cv2.FONT_HERSHEY_SIMPLEX, 0.65, (255, 255, 255), 2)

    mqtt_text  = "MQTT: CONNECTED" if mqtt_connected else "MQTT: OFFLINE"
    esp_text   = "ESP32: ONLINE" if esp32_online else "ESP32: OFFLINE"
    mqtt_color = (0, 255, 0) if mqtt_connected else (0, 0, 255)
    esp_color  = (0, 255, 0) if esp32_online else (0, 0, 255)

    cv2.putText(frame, mqtt_text, (w - 248, 60),
                cv2.FONT_HERSHEY_SIMPLEX, 0.45, mqtt_color, 1)
    cv2.putText(frame, esp_text, (w - 248, 80),
                cv2.FONT_HERSHEY_SIMPLEX, 0.45, esp_color, 1)
    cv2.putText(frame, f"FPS: {fps:.1f}", (w - 248, 100),
                cv2.FONT_HERSHEY_SIMPLEX, 0.45, (255, 255, 255), 1)

    people_color = (0, 255, 0) if inside_count == 0 else (0, 200, 255)
    cv2.putText(frame, f"People Inside: {inside_count}", (10, h - 20),
                cv2.FONT_HERSHEY_SIMPLEX, 0.7, people_color, 2)


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

    if is_gpu:
        import torch
        torch.cuda.empty_cache()  # Flush VRAM before allocating models

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
    
    # Enforce CPU tracking for reliability and preventing VRAM resource allocation crashes
    occupancy = OccupancyTracker(
        model_path=str(person_model_path),
        line_x=Config.OCCUPANCY_LINE_X,
        confidence=0.5,
        device="cpu",
    )

    decision = DecisionEngine()
    notification = NotificationService(Config)
    alert_gate = AlertGate(Config.ALERT_COOLDOWN)
    logger.info("[Init] All modules initialized.")

    # --- Capture via WiFi HTTP IP Stream (Robust Byte-Stream Parser) ---
    esp32_wifi_url = "http://10.251.189.171:81/stream"
    
    try:
        stream = urllib.request.urlopen(esp32_wifi_url, timeout=10)
        logger.info(f"[Init] Camera stream opened successfully via WiFi: {esp32_wifi_url}")
    except Exception as e:
        logger.critical(f"[Init] FATAL: Cannot connect to WiFi stream: {e}")
        sys.exit(1)

    bytes_buffer = b''

    # --- Loop state ---
    frame_count = 0
    fire_result = EMPTY_RESULT
    last_status_publish = 0.0
    last_people_publish = 0.0
    last_siren_time = 0.0
    last_published_detection = None
    manual_override_prev = False
    fps_timer = time.time()
    fps = 0.0

    logger.info("🚀 Entering main detection loop (press 'Q' to quit)...\n")

    try:
        while True:
            try:
                # Read raw binary data blocks from network socket
                chunk = stream.read(4096)
                if not chunk:
                    logger.warning("[Loop] Empty stream chunk received. Retrying...")
                    time.sleep(0.1)
                    continue
                bytes_buffer += chunk

                # Parse JPEG SOI (Start of Image) and EOI (End of Image) boundaries
                a = bytes_buffer.find(b'\xff\xd8')
                b = bytes_buffer.find(b'\xff\xd9')

                if a != -1 and b != -1:
                    if b < a:
                        # Drop misaligned historical bytes to synch next frame buffer
                        bytes_buffer = bytes_buffer[a:]
                        continue
                    
                    jpg_data = bytes_buffer[a:b+2]
                    bytes_buffer = bytes_buffer[b+2:]
                    
                    # Safe matrix decoding from processed memory buffer
                    frame = cv2.imdecode(np.frombuffer(jpg_data, dtype=np.uint8), cv2.IMREAD_COLOR)
                    if frame is None:
                        continue
                else:
                    continue

            except Exception as stream_err:
                logger.warning(f"[Loop] Network stream read frame failed: {stream_err}")
                time.sleep(0.1)
                continue

            frame_count += 1
            now = time.time()

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
            annotated_frame, inside_count = occupancy.update(frame)

            # --- C. Manual override: physical button intercepts the AI loop ---
            manual_override = bool(sensor_data.get("manual_trigger")) \
                if (sensor_data := mqtt_client.get_sensor_data()) else False
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

            # --- G. Single final visualization pass: fire boxes + HUD ---
            fire_detector.draw(annotated_frame, fire_result)
            draw_system_hud(
                annotated_frame, state=state, inside_count=inside_count,
                esp32_online=mqtt_client.is_esp32_online(),
                mqtt_connected=mqtt_client.is_connected(), fps=fps,
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
        cv2.destroyAllWindows()
        mqtt_client.disconnect()
        notification.cleanup()
        logger.info("🛑 SFFS AI Module shutdown complete.\n")


if __name__ == "__main__":
    main()