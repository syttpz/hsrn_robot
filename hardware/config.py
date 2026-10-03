import os
DEVICE_ID = os.getenv("DEVICE_ID", "robot_01")
MQTT_BROKER = os.getenv("MQTT_BROKER", "localhost")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
MQTT_USERNAME = os.getenv("MQTT_USERNAME")
MQTT_PASSWORD = os.getenv("MQTT_PASSWORD")
MQTT_TLS = os.getenv("MQTT_TLS", "false").lower() == "true"
MQTT_SENSOR_TOPIC = f"vip/{DEVICE_ID}/sensors/environment"
MQTT_CAMERA_TOPIC = f"vip/{DEVICE_ID}/camera"
BME280_I2C_ADDRESS = int(os.getenv("BME280_I2C_ADDRESS", "0x76"), 0)
SENSOR_INTERVAL = float(os.getenv("SENSOR_INTERVAL", "5"))
CAMERA_INTERVAL = float(os.getenv("CAMERA_INTERVAL", "30"))
