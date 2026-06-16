"""
SFFS Decision Engine — Multi-Source Data Fusion & State Machine
===============================================================
Fuses real-time camera detection (fire/smoke) with ESP32 IoT sensor
telemetry (gas/flame/temperature) and the physical manual alarm button to
determine the overall system threat state. Outputs actuator commands for the ESP32.

Three-tier control matrix:
  Manual button, OR gas/smoke/flame sensors, OR camera fire/smoke
                                        →  FIRE          (CONFIRMED EMERGENCY: red, full suppression)
  High ambient temperature only         →  SENSOR_ALERT  (EARLY WARNING: orange, intermittent buzzer, safe actuators)
  Everything clear                      →  SAFE          (green)

#3: gas/smoke/flame do NOT require AI camera validation — the camera cannot see
    a gas leak or ambient smoke, so those sensors are authoritative and escalate
    straight to FIRE, isolating the gas valve and tripping the pumps immediately.
"""

import time
import logging
from enum import Enum


class SystemState(Enum):
    """Three-tier system threat state."""
    SAFE         = "SAFE"          # baseline
    SENSOR_ALERT = "SENSOR_ALERT"  # hardware sensors tripped, no AI/manual confirmation
    FIRE         = "FIRE"          # confirmed emergency (AI fire/smoke OR manual button)


class DecisionEngine:
    """
    Fuses camera detections and IoT sensor readings to determine
    the current system state and required actuator actions.
    Implements debounced state transitions and immediate escalation.
    """

    # Sensor thresholds (must match ESP32 config.h values)
    GAS_THRESHOLD  = 2000     # ADC value (0-4095 range)
    TEMP_THRESHOLD = 50.0     # Celsius

    # Debounce: minimum seconds before de-escalation is allowed.
    # Escalations (SAFE→FIRE) are always immediate.
    DEESCALATION_COOLDOWN = 5.0   # seconds

    def __init__(self):
        self.logger = logging.getLogger(__name__)

        self.current_state = SystemState.SAFE
        self._last_state_change_time = 0

        # Threat priority order (higher = more dangerous)
        self._priority = {
            SystemState.SAFE:         0,
            SystemState.SENSOR_ALERT: 1,
            SystemState.FIRE:         2,
        }

        # Mechanical actuator mappings per state (valve/doors/pumps).
        # The buzzer pattern and LED color are derived from `state` ON THE ESP32
        # (applyState), so they are intentionally NOT part of this command set.
        #   gas_valve: OPEN = normal flow, CLOSE = shut off
        #   doors:     OPEN = evacuation/ventilation, CLOSE = normal
        # NOTE: per the 3-tier spec, SENSOR_ALERT keeps actuators in the SAFE
        # configuration (warning only). See the gas-valve safety caveat below.†
        self._state_actions = {
            SystemState.SAFE: {
                "gas_valve": "OPEN",  "doors": "CLOSE",
                "pump1": "OFF", "pump2": "OFF"
            },
            SystemState.SENSOR_ALERT: {
                "gas_valve": "OPEN",  "doors": "CLOSE",
                "pump1": "OFF", "pump2": "OFF"
            },
            SystemState.FIRE: {
                "gas_valve": "CLOSE", "doors": "OPEN",
                "pump1": "ON", "pump2": "ON"
            },
        }
        # † SENSOR_ALERT is now TEMP-only and keeps actuators in SAFE config.
        #   Gas/smoke/flame no longer pass through here — they escalate to FIRE
        #   (valve CLOSE, pumps ON) per the #3 absolute-escalation requirement.

        self.logger.info("[Decision] Engine initialized. State: SAFE")

    # ==========================================
    # Core Evaluation Method
    # ==========================================

    def evaluate(self, camera_detection, sensor_data, manual_override=False):
        """
        Evaluate the current threat level by fusing all data sources.

        Args:
            camera_detection: "Fire", "Smoke", or None (from YOLO detector)
            sensor_data:      Dict from ESP32 MQTT payload, or None if offline.
                              Expected schema:
                              {
                                "gas": {"mq2": int, "mq5": int, "mq6": int, "mq7": int},
                                "env": {"temp": float, "hum": float},
                                "meta": {"flame": bool, ...},
                                ...
                              }
            manual_override:  True if the physical manual alarm button is pressed
                              (forces CONFIRMED FIRE, bypassing camera inference).

        Returns:
            tuple: (state, state_changed, actions, confidence, source)
                - state:          SystemState enum value
                - state_changed:  True if state transitioned this cycle
                - actions:        Dict of actuator commands
                - confidence:     Float 0.0-1.0
                - source:         String describing the decision source
        """
        # Extract sensor flags from the ESP32 payload
        gas_danger, temp_danger, flame_danger = self._parse_sensor_flags(sensor_data)

        # Run the decision matrix
        new_state, confidence, source = self._decide(
            camera_detection, gas_danger, temp_danger, flame_danger, manual_override
        )

        # Apply debounced state transition logic
        state_changed = self._apply_transition(new_state)

        # Get the actuator actions for the current (possibly updated) state
        actions = self._state_actions[self.current_state]

        return self.current_state, state_changed, actions, confidence, source

    # ==========================================
    # Internal Decision Logic
    # ==========================================

    def _parse_sensor_flags(self, sensor_data):
        """
        Extract boolean gas/temperature/flame danger flags from the ESP32 payload.
        All False if sensor_data is None.
        Returns (gas_danger, temp_danger, flame_danger).
        """
        gas_danger   = False
        temp_danger  = False
        flame_danger = False

        if sensor_data:
            gas = sensor_data.get("gas", {})
            gas_values = [
                gas.get("mq2", 0), gas.get("mq5", 0),
                gas.get("mq6", 0), gas.get("mq7", 0)
            ]
            gas_danger = any(v > self.GAS_THRESHOLD for v in gas_values)

            env = sensor_data.get("env", {})
            temp_danger = env.get("temp", 0) > self.TEMP_THRESHOLD

            # Flame flag lives in meta (ESP32 IR flame sensor, active-detected).
            flame_danger = bool(sensor_data.get("meta", {}).get("flame", False))

        return gas_danger, temp_danger, flame_danger

    def _decide(self, camera_detection, gas_danger, temp_danger,
                flame_danger, manual_override):
        """
        Core 3-tier decision matrix. Returns (state, confidence, source).

        Priority order (highest first):
          1. Manual alarm button                       →  FIRE  (bypasses camera)
          2. Gas/smoke/flame sensors OR camera fire/smoke →  FIRE  (CONFIRMED EMERGENCY)
          3. High ambient temperature only             →  SENSOR_ALERT (soft warning)
          4. Default                                    →  SAFE

        #3: gas/smoke/flame escalate to FIRE WITHOUT AI validation — the camera
        cannot see them, so they are authoritative and immediately isolate the
        gas valve + trip the pumps.
        """
        # Priority 1: physical manual override — absolute, certain.
        if manual_override:
            return SystemState.FIRE, 1.0, "MANUAL_OVERRIDE"

        # Priority 2: CONFIRMED FIRE. Hardware gas/smoke/flame escalate on their
        # own (no AI required); the camera fire/smoke also confirms. Any combination
        # lands here — enrich the source label + confidence accordingly.
        cam_fire = camera_detection in ("Fire", "Smoke")
        if gas_danger or flame_danger or cam_fire:
            sources = []
            if cam_fire:     sources.append("AI_VISUAL")
            if gas_danger:   sources.append("GAS/SMOKE")
            if flame_danger: sources.append("FLAME")
            confidence = 0.98 if len(sources) > 1 else 0.92
            return SystemState.FIRE, confidence, "+".join(sources)

        # Priority 3: high ambient temperature ALONE — ambiguous, so warn only.
        # (To make TEMP also force FIRE, move temp_danger into the block above.)
        if temp_danger:
            return SystemState.SENSOR_ALERT, 0.65, "SENSOR_TEMP"

        # Default: everything is clear
        return SystemState.SAFE, 1.0, "ALL_CLEAR"

    def _apply_transition(self, new_state):
        """
        Apply state transition with debounce logic.

        Rules:
          - Escalation (e.g., SAFE → FIRE): Always immediate. No delay.
          - De-escalation (e.g., FIRE → SAFE): Only allowed after cooldown.
            This prevents flickering when fire is intermittently visible.

        Returns True if the state actually changed.
        """
        if new_state == self.current_state:
            return False

        now = time.time()
        is_escalation = (
            self._priority[new_state] > self._priority[self.current_state]
        )

        if is_escalation:
            # Escalation: transition immediately — safety first
            self.logger.warning(
                f"[Decision] ESCALATION: {self.current_state.value} → "
                f"{new_state.value}"
            )
            self.current_state = new_state
            self._last_state_change_time = now
            return True

        else:
            # De-escalation: only after cooldown period
            elapsed = now - self._last_state_change_time
            if elapsed >= self.DEESCALATION_COOLDOWN:
                self.logger.info(
                    f"[Decision] De-escalation: {self.current_state.value} → "
                    f"{new_state.value} (after {elapsed:.1f}s)"
                )
                self.current_state = new_state
                self._last_state_change_time = now
                return True
            else:
                # Cooldown not yet elapsed — hold current state
                return False

    # ==========================================
    # Public Helpers
    # ==========================================

    def get_state(self):
        """Return the current system state."""
        return self.current_state

    def get_actions(self):
        """Return the actuator actions for the current state."""
        return self._state_actions[self.current_state]

    def force_state(self, state):
        """
        Force a state transition (bypass debounce).
        Use only for testing or manual override from the dashboard.
        """
        self.logger.warning(f"[Decision] FORCED state → {state.value}")
        self.current_state = state
        self._last_state_change_time = time.time()
