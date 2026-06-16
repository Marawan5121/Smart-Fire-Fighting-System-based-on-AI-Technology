"""
SFFS MQTT Client — Bidirectional ESP32 Communication Bridge
============================================================
Subscribes to ESP32 sensor telemetry, publishes AI actuator commands, and
signals AI liveness via a DEDICATED status topic.

B2 fix: the Last Will & Testament lives on `sffs/ai/status` ("offline") and is
NEVER published on the actuator-command topic — so an AI crash can no longer be
mis-parsed by the ESP32 as a "open the gas valve" command.

Thread-safe by design: paho-mqtt runs its own background network thread.
"""

import paho.mqtt.client as mqtt
import json
import time
import threading
import logging


class SffsMqttClient:
    """Asynchronous MQTT bridge between the AI vision module and the ESP32 node."""

    # ==========================================
    # MQTT Topic Architecture
    # ==========================================
    TOPIC_SENSOR_DATA   = "sffs/sensors/data"        # ESP32 → AI  (sensor telemetry)
    TOPIC_SYSTEM_STATUS = "sffs/system/status"       # AI → ESP32  (actuator commands)
    TOPIC_HEARTBEAT     = "sffs/system/heartbeat"    # ESP32 → AI  (alive signal)
    TOPIC_AI_STATUS     = "sffs/ai/status"           # AI → all    (AI liveness / LWT)
    TOPIC_AI_DETECTION  = "sffs/ai/detection"        # AI → broker (raw detection events)
    TOPIC_PEOPLE_COUNT  = "sffs/ai/people_count"     # AI → broker (occupancy data)

    def __init__(self, broker_host="localhost", broker_port=1883,
                 client_id="sffs_ai_module"):
        self.logger = logging.getLogger(__name__)
        self.broker_host = broker_host
        self.broker_port = broker_port

        # Thread-safe shared state
        self._lock = threading.Lock()
        self._latest_sensor_data = None
        self._last_heartbeat_time = 0
        self._connected = False

        self._client = mqtt.Client(
            client_id=client_id,
            protocol=mqtt.MQTTv311,
            clean_session=True,
        )
        self._client.on_connect    = self._on_connect
        self._client.on_disconnect = self._on_disconnect
        self._client.on_message    = self._on_message

        # B2: LWT on a DEDICATED status topic — not an actuator command.
        self._client.will_set(
            self.TOPIC_AI_STATUS,
            json.dumps({"ts": int(time.time()), "status": "offline"}),
            qos=1, retain=True,
        )

    # ==========================================
    # Connection Management
    # ==========================================
    def connect(self):
        """Begin connecting. Returns True if the attempt was initiated."""
        try:
            self._client.connect(self.broker_host, self.broker_port, keepalive=60)
            self._client.loop_start()
            self.logger.info(
                f"[MQTT] Connecting to {self.broker_host}:{self.broker_port}..."
            )
            time.sleep(1.0)  # brief grace for the async handshake
            return True
        except ConnectionRefusedError:
            self.logger.warning(
                f"[MQTT] Broker refused connection at "
                f"{self.broker_host}:{self.broker_port}"
            )
            return False
        except Exception as e:
            self.logger.warning(f"[MQTT] Connection failed: {e}")
            return False

    def disconnect(self):
        """Mark the AI offline (on the status topic) and stop the network thread."""
        try:
            if self._connected:
                self._publish_ai_status("offline")
            self._client.loop_stop()
            self._client.disconnect()
            self.logger.info("[MQTT] Disconnected cleanly.")
        except Exception as e:
            self.logger.error(f"[MQTT] Disconnect error: {e}")

    def is_connected(self):
        return self._connected

    # ==========================================
    # Callbacks (paho background thread)
    # ==========================================
    def _on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            self._connected = True
            self.logger.info("[MQTT] Connected to broker successfully!")
            client.subscribe(self.TOPIC_SENSOR_DATA, qos=0)
            client.subscribe(self.TOPIC_HEARTBEAT, qos=0)
            self._publish_ai_status("online")
            self.logger.info(
                f"[MQTT] Subscribed to {self.TOPIC_SENSOR_DATA}, {self.TOPIC_HEARTBEAT}"
            )
        else:
            reasons = {
                1: "Incorrect protocol version", 2: "Invalid client ID",
                3: "Server unavailable", 4: "Bad username/password",
                5: "Not authorized",
            }
            self.logger.error(
                f"[MQTT] Connection refused: {reasons.get(rc, f'code {rc}')}"
            )

    def _on_disconnect(self, client, userdata, rc):
        self._connected = False
        if rc != 0:
            self.logger.warning(
                f"[MQTT] Unexpected disconnect (rc={rc}). Auto-reconnect pending."
            )
        else:
            self.logger.info("[MQTT] Disconnected normally.")

    def _on_message(self, client, userdata, msg):
        try:
            payload = json.loads(msg.payload.decode("utf-8"))
            if msg.topic == self.TOPIC_SENSOR_DATA:
                with self._lock:
                    self._latest_sensor_data = payload
            elif msg.topic == self.TOPIC_HEARTBEAT:
                with self._lock:
                    self._last_heartbeat_time = time.time()
        except json.JSONDecodeError:
            self.logger.warning(f"[MQTT] Invalid JSON on topic: {msg.topic}")
        except Exception as e:
            self.logger.error(f"[MQTT] Message processing error: {e}")

    # ==========================================
    # Data Retrieval (main thread)
    # ==========================================
    def get_sensor_data(self):
        with self._lock:
            return self._latest_sensor_data

    def is_esp32_online(self, timeout_seconds=15):
        with self._lock:
            if self._last_heartbeat_time == 0:
                return False
            return (time.time() - self._last_heartbeat_time) < timeout_seconds

    # ==========================================
    # Publishing (main thread)
    # ==========================================
    def _publish_ai_status(self, status: str):
        """Retained AI-liveness marker on the dedicated status topic."""
        self._client.publish(
            self.TOPIC_AI_STATUS,
            json.dumps({"ts": int(time.time()), "status": status}),
            qos=1, retain=True,
        )

    def publish_system_status(self, state, confidence, source, actions, retain=False):
        """
        Publish the AI-determined actuator command set to the ESP32 (QoS 1).

        B1 fix: callers re-publish this periodically with retain=True so a
        rebooting ESP32 immediately receives the last authoritative command
        instead of falling back to unsafe boot defaults.
        """
        payload = json.dumps({
            "ts": int(time.time()),
            "state": state,
            "confidence": round(confidence, 2),
            "source": source,
            "actions": actions,
        })
        result = self._client.publish(
            self.TOPIC_SYSTEM_STATUS, payload, qos=1, retain=retain
        )
        if result.rc != mqtt.MQTT_ERR_SUCCESS:
            self.logger.warning(
                f"[MQTT] Failed to publish system status: rc={result.rc}"
            )

    def publish_people_count(self, count):
        self._client.publish(
            self.TOPIC_PEOPLE_COUNT,
            json.dumps({"ts": int(time.time()), "inside_count": count}),
            qos=0,
        )

    def publish_detection(self, detection_type, confidence):
        self._client.publish(
            self.TOPIC_AI_DETECTION,
            json.dumps({
                "ts": int(time.time()),
                "type": detection_type,
                "confidence": round(confidence, 2),
            }),
            qos=0,
        )
