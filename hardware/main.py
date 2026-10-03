import argparse
import json
import time
from datetime import datetime, timezone
from contextlib import ExitStack
import config
from sensors import BME280Sensor, FakeSensor

def timestamp():
    return datetime.now(timezone.utc).isoformat()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--fake", action="store_true", help="Use simulated sensor readings")
    parser.add_argument("--dry-run", action="store_true", help="Print JSON without MQTT")
    parser.add_argument("--camera", action="store_true", help="Enable a real CSI camera")
    parser.add_argument("--count", type=int, default=0, help="Stop after N readings; 0 runs forever")
    args = parser.parse_args()
    if config.SENSOR_INTERVAL <= 0 or config.CAMERA_INTERVAL <= 0 or args.count < 0:
        parser.error("Intervals must be positive and count must be nonnegative")
    with ExitStack() as stack:
        sensor = FakeSensor() if args.fake else BME280Sensor()
        stack.callback(sensor.close)
        mqtt = None
        if not args.dry_run:
            from mqtt_client import MQTTClient
            mqtt = MQTTClient()
            stack.callback(mqtt.disconnect)
            mqtt.connect()
        camera = None
        if args.camera:
            from camera import CameraManager
            camera = CameraManager()
            stack.callback(camera.close)
        def send(topic, data):
            if mqtt:
                mqtt.publish_json(topic, data)
            print(topic, json.dumps(data), flush=True)
        next_camera = 0
        readings = 0
        try:
            while True:
                send(config.MQTT_SENSOR_TOPIC, {"device_id": config.DEVICE_ID,
                     "timestamp": timestamp(), "simulated": args.fake, **sensor.read()})
                if camera and time.monotonic() >= next_camera:
                    send(config.MQTT_CAMERA_TOPIC, {"device_id": config.DEVICE_ID,
                         "timestamp": timestamp(), "image_path": camera.capture(),
                         "storage": "device_local"})
                    next_camera = time.monotonic() + config.CAMERA_INTERVAL
                readings += 1
                if args.count and readings >= args.count:
                    break
                time.sleep(config.SENSOR_INTERVAL)
        except KeyboardInterrupt:
            print("Stopped")

if __name__ == "__main__":
    main()
