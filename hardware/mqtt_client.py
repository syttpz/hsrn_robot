import json
import threading
import config

class MQTTClient:
    def __init__(self):
        import paho.mqtt.client as mqtt
        self.mqtt = mqtt
        self.ready = threading.Event()
        self.error = None
        self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect
        self.client.reconnect_delay_set(min_delay=1, max_delay=30)
        if config.MQTT_USERNAME:
            self.client.username_pw_set(config.MQTT_USERNAME, config.MQTT_PASSWORD)
        if config.MQTT_TLS:
            self.client.tls_set()

    def _on_connect(self, client, userdata, flags, reason_code, properties):
        if reason_code.is_failure:
            self.error = str(reason_code)
        else:
            self.ready.set()

    def _on_disconnect(self, client, userdata, flags, reason_code, properties):
        self.ready.clear()

    def connect(self):
        self.client.connect(config.MQTT_BROKER, config.MQTT_PORT, 60)
        self.client.loop_start()
        if not self.ready.wait(10):
            self.disconnect()
            raise ConnectionError(self.error or "MQTT connection timed out")

    def publish_json(self, topic, data):
        if not self.ready.wait(10):
            raise ConnectionError("MQTT is disconnected")
        result = self.client.publish(topic, json.dumps(data), qos=1)
        if result.rc != self.mqtt.MQTT_ERR_SUCCESS:
            raise ConnectionError(f"MQTT publish failed: {result.rc}")
        result.wait_for_publish(timeout=10)
        if not result.is_published():
            raise TimeoutError("MQTT publish acknowledgement timed out")

    def disconnect(self):
        self.client.disconnect()
        self.client.loop_stop()
