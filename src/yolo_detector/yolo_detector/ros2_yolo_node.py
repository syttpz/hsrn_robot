import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from cv_bridge import CvBridge
from ultralytics import YOLO
import cv2
import time
import sqlite3
import os
import numpy as np  # 新增，用于处理深度图矩阵


class YoloRosNode(Node):
    def __init__(self):
        super().__init__('yolo_detector_node')

        # ================= 配置区 =================
        current_dir = os.path.dirname(os.path.abspath(__file__))
        
        # 模型路径
        self.model_path = os.path.join(current_dir, '..', 'best.pt')
        
        # 数据库路径
        self.db_path = os.path.join(current_dir, 'yolo_detections.db')

        # 难例保存路径
        self.hard_dir = os.path.join(current_dir, '..', 'hard_examples')
        os.makedirs(self.hard_dir, exist_ok=True)
        self.get_logger().info(f"📸 难例图片将保存至: {self.hard_dir}")
        
        # 订阅的话题名
        self.image_topic = "/camera/camera/color/image_raw" 
        self.depth_topic = "/camera/camera/depth/image_rect_raw" # 新增深度图话题
        # =========================================

        self.get_logger().info(f"正在加载模型: {self.model_path}")
        self.model = YOLO(self.model_path)
        self.bridge = CvBridge()

        # 初始化数据库
        self.init_db()

        # 订阅彩色图像话题
        self.subscription = self.create_subscription(
            Image,
            self.image_topic,
            self.image_callback,
            10)

        # 订阅深度图像话题
        self.depth_sub = self.create_subscription(
            Image,
            self.depth_topic,
            self.depth_callback,
            10)
        
        self.depth_image = None # 缓存深度图数据

        # 发布带框图像的话题
        self.publisher_ = self.create_publisher(Image, '/yolo/image_annotated', 10)

        self.frame_id = 0
        self.get_logger().info(f"✅ 节点已启动！等待接收话题...\n彩色: {self.image_topic}\n深度: {self.depth_topic}")

    def init_db(self):
        self.conn = sqlite3.connect(self.db_path)
        self.cursor = self.conn.cursor()
        
        # 升级表结构，增加 distance 字段
        self.cursor.execute('''
            CREATE TABLE IF NOT EXISTS yolo_logs (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                timestamp REAL,
                frame_id INTEGER,
                class_id INTEGER,
                class_name TEXT,
                confidence REAL,
                x_center REAL, y_center REAL, width REAL, height REAL,
                distance REAL
            )
        ''')
        self.conn.commit()

    def depth_callback(self, msg):
        try:
            # RealSense 深度图通常是 16UC1 格式，单位毫米
            self.depth_image = self.bridge.imgmsg_to_cv2(msg, desired_encoding='16UC1')
        except Exception as e:
            self.get_logger().error(f"深度图转换失败: {e}")

    def image_callback(self, msg):
        try:
            cv_img = self.bridge.imgmsg_to_cv2(msg, desired_encoding='bgr8')
        except Exception as e:
            self.get_logger().error(f"图像转换失败: {e}")
            return

        timestamp = msg.header.stamp.sec + msg.header.stamp.nanosec * 1e-9

        # 1. YOLO 推理（注意：为了抓取低置信度难例，这里把阈值降到0.25）
        results = self.model(cv_img, conf=0.25, verbose=False)
        result = results[0]

        # 2. 遍历预测框
        for box in result.boxes:
            cls_id = int(box.cls[0].cpu().numpy())
            cls_name = self.model.names[cls_id]
            conf = float(box.conf[0].cpu().numpy())
            x, y, w, h = box.xywhn[0].cpu().numpy()
            
            # 获取框的像素坐标 (左上角 x1,y1 和 右下角 x2,y2)
            x1, y1, x2, y2 = box.xyxy[0].cpu().numpy()

            # ================= 选项 A：难例挖掘 =================
            # 只有在 0.30 到 0.55 之间，模型才是在“犹豫”
            if 0.30 <= conf <= 0.55:
                img_name = f"hard_{int(timestamp)}_{self.frame_id}_{cls_name}_{conf:.2f}.jpg"
                save_path = os.path.join(self.hard_dir, img_name)
                cv2.imwrite(save_path, cv_img)
                self.get_logger().info(f"📸 抓到一只难例！已保存: {img_name}")
            # ====================================================

            # ================= 选项 B：深度测距 =================
            distance = 0.0
            if self.depth_image is not None:
                # 计算框的中心点像素坐标
                cx, cy = int((x1 + x2) / 2), int((y1 + y2) / 2)
                
                # 防止坐标越界
                dh, dw = self.depth_image.shape
                if 0 <= cx < dw and 0 <= cy < dh:
                    depth_val = self.depth_image[cy, cx]
                    # 过滤无效深度（0 通常代表无法测距）
                    if depth_val > 0:
                        distance = depth_val / 1000.0  # 毫米转为米
            # ====================================================

            # 存入 SQLite 数据库 (只有 conf > 0.4 的框才存，过滤掉太差的杂音)
            if conf >= 0.4:
                self.cursor.execute('''
                    INSERT INTO yolo_logs (timestamp, frame_id, class_id, class_name, confidence, x_center, y_center, width, height, distance)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                ''', (timestamp, self.frame_id, cls_id, cls_name, conf, x, y, w, h, distance))

        # 每10帧提交一次，避免频繁读写伤硬盘
        if self.frame_id % 10 == 0:
            self.conn.commit()

        # 3. 画图
        annotated_frame = result.plot()
        
        # 在画面上叠加距离信息（这里重新遍历一遍画文本）
        for box in result.boxes:
            conf = float(box.conf[0].cpu().numpy())
            if conf >= 0.4 and self.depth_image is not None:
                x1, y1, x2, y2 = box.xyxy[0].cpu().numpy()
                cx, cy = int((x1 + x2) / 2), int((y1 + y2) / 2)
                dh, dw = self.depth_image.shape
                if 0 <= cx < dw and 0 <= cy < dh:
                    depth_val = self.depth_image[cy, cx]
                    if depth_val > 0:
                        dist_text = f"{depth_val/1000.0:.1f}m"
                        # 把距离文字标在框的右下角附近
                        cv2.putText(annotated_frame, dist_text, (int(x2)-40, int(y2)+15),
                                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 255), 2)

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

    def __del__(self):
        self.conn.close()


def main(args=None):
    rclpy.init(args=args)
    node = YoloRosNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.get_logger().info("收到 Ctrl+C，准备退出...")
    finally:
        node.conn.commit()
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()