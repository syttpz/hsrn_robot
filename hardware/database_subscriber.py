"""Optional PostgreSQL receiver. Run schema.sql first."""
import json
import os
import logging
import math
from datetime import datetime
import paho.mqtt.client as mqtt
import psycopg2
import config

logging.basicConfig(level=logging.INFO)

def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code.is_failure:
        logging.error("MQTT connection rejected: %s", reason_code)
        return
    client.subscribe("vip/+/sensors/environment", qos=1)

def on_message(client, userdata, message):
    try:
        data = json.loads(message.payload.decode())
        device = data["device_id"]
        if not isinstance(device, str) or message.topic != f"vip/{device}/sensors/environment":
            raise ValueError("Device/topic mismatch")
        stamp = datetime.fromisoformat(data["timestamp"].replace("Z", "+00:00"))
        if stamp.tzinfo is None:
            raise ValueError("Timestamp must include timezone")
        values = [float(data[key]) for key in ("temperature", "humidity", "pressure")]
        if not all(math.isfinite(v) for v in values):
            raise ValueError("Nonfinite sensor value")
        simulated = data.get("simulated", False)
        if not isinstance(simulated, bool):
            raise ValueError("simulated must be boolean")
        # A connection per message keeps a failed transaction from poisoning later inserts.
        with psycopg2.connect(os.environ["DATABASE_URL"]) as db:
            with db.cursor() as cursor:
                cursor.execute("""INSERT INTO sensor_data
                    (device_id, timestamp, temperature, humidity, pressure, simulated)
                    VALUES (%s, %s, %s, %s, %s, %s)
                    ON CONFLICT (device_id, timestamp) DO NOTHING""",
                    (device, stamp, *values, simulated))
        logging.info("Saved reading from %s", device)
    except Exception:
        logging.exception("Reading rejected or database insert failed")

def main():
    if not os.getenv("DATABASE_URL"):
        raise RuntimeError("Set DATABASE_URL before running the subscriber")
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = on_connect
    client.on_message = on_message
    if config.MQTT_USERNAME:
        client.username_pw_set(config.MQTT_USERNAME, config.MQTT_PASSWORD)
    if config.MQTT_TLS:
        client.tls_set()
    client.connect(config.MQTT_BROKER, config.MQTT_PORT, 60)
    try:
        client.loop_forever()
    finally:
        client.disconnect()

if __name__ == "__main__":
    main()
