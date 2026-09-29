import sqlite3
import cv2
import numpy as np
import os
from datetime import datetime

# ================= 配置区 =================
DB_PATH = "rtabmap.db"  # 数据库路径
OUTPUT_DIR = "extracted_frames_10s"  # 输出图片文件夹
LOG_FILE = "extract_log.txt"  # 日志文件
SECONDS_INTERVAL = 10.0  # 抽帧间隔（秒）


# ==========================================

def parse_time(time_str):
    """时间解析模块：将字符串转为 datetime 对象"""
    if not time_str:
        return None
    try:
        # 兼容标准格式
        return datetime.strptime(time_str[:19], "%Y-%m-%d %H:%M:%S")
    except ValueError:
        try:
            # 兼容带毫秒或时区的情况
            return datetime.fromisoformat(time_str)
        except Exception:
            return None


def extract_frames_by_time():
    # 创建输出文件夹
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    # 连接数据库
    if not os.path.exists(DB_PATH):
        print(f"❌ 找不到数据库文件: {DB_PATH}")
        return
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()

    print(f"===== 开始按时间抽帧 (每 {SECONDS_INTERVAL} 秒) =====")

    # 获取所有节点，按时间排序
    cursor.execute("SELECT id, time_enter FROM Node ORDER BY time_enter ASC")
    nodes = cursor.fetchall()

    if not nodes:
        print("❌ Node 表为空，无法抽帧。")
        conn.close()
        return

    last_extracted_time = None
    saved_count = 0
    log_entries = []

    for node_id, time_str in nodes:
        current_time = parse_time(time_str)
        if not current_time:
            continue

        # 判断是否达到时间间隔
        should_extract = False
        if last_extracted_time is None:
            should_extract = True  # 第一帧必抽
        else:
            time_diff = (current_time - last_extracted_time).total_seconds()
            if time_diff >= SECONDS_INTERVAL:
                should_extract = True

        if should_extract:
            # 从 Data 表中查图片
            cursor.execute("SELECT image FROM Data WHERE id=?", (node_id,))
            row = cursor.fetchone()

            if row and row[0]:
                img_blob = row[0]
                # 解码图片
                img_array = np.frombuffer(img_blob, np.uint8)
                img = cv2.imdecode(img_array, cv2.IMREAD_COLOR)

                if img is not None:
                    # 保存图片
                    save_name = f"frame_{node_id:04d}_{time_str[11:19].replace(':', '')}.jpg"
                    save_path = os.path.join(OUTPUT_DIR, save_name)
                    cv2.imwrite(save_path, img)

                    saved_count += 1
                    log_entries.append(f"{node_id}\t{time_str}\t{save_name}")
                    print(f"  [已保存] {save_name}")

            last_extracted_time = current_time

    conn.close()

    # 写入日志文件（清单日志模块）
    with open(LOG_FILE, 'w', encoding='utf-8') as f:
        f.write("Node_ID\tTime_Stamp\tSaved_File\n")
        f.write("\n".join(log_entries))

    print(f"\n✅ 抽帧完成！共保存 {saved_count} 张图片到 '{OUTPUT_DIR}'。")
    print(f"📄 抽帧清单已保存到 '{LOG_FILE}'。")


if __name__ == '__main__':
    extract_frames_by_time()