import random
from config import BME280_I2C_ADDRESS

class BME280Sensor:
    def __init__(self):
        import board
        import adafruit_bme280.basic as adafruit_bme280
        self.i2c = board.I2C()
        self.sensor = adafruit_bme280.Adafruit_BME280_I2C(self.i2c, address=BME280_I2C_ADDRESS)

    def read(self):
        return {"temperature": round(self.sensor.temperature, 2),
                "humidity": round(self.sensor.relative_humidity, 2),
                "pressure": round(self.sensor.pressure, 2)}

    def close(self):
        self.i2c.deinit()

class FakeSensor:
    def read(self):
        return {"temperature": round(random.uniform(20, 30), 2),
                "humidity": round(random.uniform(35, 70), 2),
                "pressure": round(random.uniform(1005, 1020), 2)}

    def close(self):
        pass
