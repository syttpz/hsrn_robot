import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from cv_bridge import CvBridge
from ultralytics import YOLO
import cv2
import time
import sqlite3
import os


class YoloRosNode(Node):
    def __init__(self):
        super().__init__('yolo_detector_node')

        # ================= 配置区 =================
        # 动态获取当前代码所在目录 (即 .../yolo_detector/yolo_detector/)
        current_dir = os.path.dirname(os.path.abspath(__file__))
        
        # 模型路径：往上一级目录找 best.pt (即 .../yolo_detector/best.pt)
        self.model_path = os.path.join(current_dir, '..', 'best.pt')
        
        # 数据库路径：放在当前代码同级目录下（方便测试时查看）
        self.db_path = os.path.join(current_dir, 'yolo_detections.db')
        
        # 订阅的话题名（保留原样）
        self.image_topic = "/camera/color/image_raw" 
        # =========================================

        self.get_logger().info(f"正在加载模型: {self.model_path}")
        self.model = YOLO(self.model_path)
        self.bridge = CvBridge()

        # 初始化数据库
        self.init_db()

        # 订阅图像话题
        self.subscription = self.create_subscription(
            Image,
            self.image_topic,
            self.image_callback,
            10)

        # 发布带框图像的话题，方便你明天用 Foxglove 看
        self.publisher_ = self.create_publisher(Image, '/yolo/image_annotated', 10)

        self.frame_id = 0
        self.get_logger().info(f"✅ 节点已启动！等待接收话题: {self.image_topic} ...")

    def init_db(self):
        self.conn = sqlite3.connect(self.db_path)
        self.cursor = self.conn.cursor()
        self.cursor.execute('''
            CREATE TABLE IF NOT EXISTS yolo_logs (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                timestamp REAL,
                frame_id INTEGER,
                class_id INTEGER,
                class_name TEXT,
                confidence REAL,
                x_center REAL, y_center REAL, width REAL, height REAL
            )
        ''')
        self.conn.commit()

    def image_callback(self, msg):
        # 1. ROS 图像转 OpenCV
        try:
            cv_img = self.bridge.imgmsg_to_cv2(msg, desired_encoding='bgr8')
        except Exception as e:
            self.get_logger().error(f"图像转换失败: {e}")
            return

        timestamp = msg.header.stamp.sec + msg.header.stamp.nanosec * 1e-9

        # 2. YOLO 推理
        results = self.model(cv_img, conf=0.4, verbose=False)
        result = results[0]

        # 3. 写入 SQLite
        for box in result.boxes:
            cls_id = int(box.cls[0].cpu().numpy())
            cls_name = self.model.names[cls_id]
            conf = float(box.conf[0].cpu().numpy())
            x, y, w, h = box.xywhn[0].cpu().numpy()

            self.cursor.execute('''
                INSERT INTO yolo_logs (timestamp, frame_id, class_id, class_name, confidence, x_center, y_center, width, height)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
            ''', (timestamp, self.frame_id, cls_id, cls_name, conf, x, y, w, h))

        # 每10帧提交一次，避免频繁读写伤硬盘
        if self.frame_id % 10 == 0:
            self.conn.commit()

        # 4. 画框并发布
        annotated_frame = result.plot()

        # 在画面左上角打上当前帧数
        cv2.putText(annotated_frame, f"Frame: {self.frame_id}", (10, 30),
                    cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)

        # 转回 ROS 消息并发布
        out_msg = self.bridge.cv2_to_imgmsg(annotated_frame, encoding='bgr8')
        out_msg.header = msg.header
        self.publisher_.publish(out_msg)

        self.frame_id += 1
        if self.frame_id % 30 == 0:
            self.get_logger().info(f"已处理 {self.frame_id} 帧")


def main(args=None):
    rclpy.init(args=args)
    node = YoloRosNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.get_logger().info("收到 Ctrl+C，准备退出...")
    finally:
        node.conn.commit()
        node.conn.close()
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()