# VIP Robot Hardware Gateway

BME280 temperature (°C), humidity (%), and pressure (hPa) → Raspberry Pi → MQTT → optional PostgreSQL subscriber.

## 无硬件先运行（Windows / Linux，Python 3.10+）

```bash
python main.py --fake --dry-run --count 3
```

无需第三方库、树莓派或 broker；输出模拟数据。停止持续运行的程序按 Ctrl+C。

## 测试 MQTT

```bash
python -m venv .venv
```

Windows PowerShell 激活：`.\.venv\Scripts\Activate.ps1`；Linux：`source .venv/bin/activate`。

```bash
python -m pip install -r requirements.txt
python fake_sensor.py
```

默认 broker 为 localhost，必须有可连接的 MQTT broker。PowerShell 设置真实地址：

```powershell
$env:MQTT_BROKER = "YOUR_BROKER_HOST"
$env:MQTT_PORT = "1883"
```

Linux：`export MQTT_BROKER=YOUR_BROKER_HOST`。账号密码分别用 MQTT_USERNAME、MQTT_PASSWORD 设置；TLS 用 MQTT_TLS=true，并使用管理员提供的端口。
`.env.example` 仅为配置参考，程序不会自动加载它。不要上传真实密码。

## Raspberry Pi + BME280

已安装 Raspberry Pi OS，开启 I²C。断电后连接：

| BME280 | Pi physical pin |
|---|---|
| VCC | 1 (3.3V) |
| GND | 6 (GND) |
| SDA | 3 (GPIO2) |
| SCL | 5 (GPIO3) |

```bash
sudo apt update
sudo apt install -y python3-venv i2c-tools
 i2cdetect -y 1
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements-pi.txt
python main.py --dry-run --count 3
```

地址为 77 时先执行 `export BME280_I2C_ADDRESS=0x77`。
I²C 地址只能证明设备响应，不能证明它一定是 BME280；BMP280 不支持湿度。
设置 broker 后执行 `python main.py`，每 5 秒发一条数据。

## 可选 CSI 摄像头

此代码仅支持 Picamera2 的 CSI 摄像头，USB 摄像头需要其他实现。

```bash
sudo apt install -y python3-picamera2
python3 -m venv --system-site-packages .venv-camera
source .venv-camera/bin/activate
python -m pip install -r requirements-pi.txt
python main.py --camera
```

每 30 秒拍照至 images/。MQTT 发的是 Pi 本机路径，远程服务器无法仅凭此路径下载图片。
本项目没有图片上传服务；正式接入需上传到服务器 / MinIO / S3 后再发可访问 URL 或对象键。
照片持续保留，请按需求增加清理策略。

## 可选 PostgreSQL 接收端

在服务器上运行，先创建数据库并执行 schema.sql。

```bash
python -m pip install -r requirements-server.txt
```

设置 MQTT_BROKER 及 DATABASE_URL（见 .env.example），然后：

```bash
python database_subscriber.py
```

订阅 `vip/+/sensors/environment`，接收模拟或真实读数；simulated 字段区分它们。
唯一键 (device_id, timestamp) 防止 QoS 1 重复消息插入。
接收端打印失败日志；没有磁盘队列或数据库故障自动重放，因此这是团队集成起点，不保证故障时不丢数据。

## 上传 GitHub

解压 ZIP，将 vip_robot 文件夹里的文件上传至仓库所需目录（不要只上传 ZIP）。
已有仓库可放在 hardware/vip_robot/。不要上传 .venv、真实 .env 或 images。

## 验证范围

已检查 Python 语法并运行无硬件模拟模式。真实 BME280、CSI camera、MQTT broker、PostgreSQL 需拿到设备及连接参数后集成测试。
