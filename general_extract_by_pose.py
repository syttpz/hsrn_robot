import sqlite3
import struct
import math
import os
import cv2
import numpy as np
from datetime import datetime

# ================= 配置区 =================
DB_PATH = "rtabmap.db"
OUTPUT_BASE_DIR = "check_turns"
# 每次运行自动生成带时间戳的文件夹，防止旧数据被覆盖
RUN_TIMESTAMP = datetime.now().strftime("%Y%m%d_%H%M%S")
OUTPUT_DIR = os.path.join(OUTPUT_BASE_DIR, f"run_{RUN_TIMESTAMP}")

# --- 核心阈值调节区（根据实际效果微调） ---
# 1. 角度条件
YAW_RATE_THRESHOLD = 35.0  # 瞬时角速度阈值（度/秒）：调大过滤瞬间抖动
YAW_NET_SUM_THRESHOLD = 25.0  # 5秒内净累计偏航角阈值（度）：调大过滤缓慢漂移
WINDOW_SEC = 10.0  # 滑动窗口时间（秒）

# 2. 位移条件（核心！过滤原地打转）
MIN_DISTANCE_MOVED = 0.05  # 当前帧最小位移（米），至少移动5厘米
MIN_WINDOW_DISTANCE = 0.15  # X秒窗口内最小净位移（米），至少移动15厘米

# 3. 抽图范围
FRAMES_BEFORE_AND_AFTER = 4  # 触发点前后各抽多少帧


# ==========================================

def parse_time(time_str):
    """解析时间字符串"""
    if not time_str: return None
    try:
        return datetime.strptime(time_str[:19], "%Y-%m-%d %H:%M:%S")
    except ValueError:
        try:
            return datetime.fromisoformat(time_str)
        except Exception:
            return None


def extract_pose_info(pose_blob):
    """同时提取 Yaw 和平移距离"""
    if not pose_blob or len(pose_blob) < 28: return None, 0.0, 0.0
    dx, dy = 0.0, 0.0
    try:
        # 兼容 12 个 float32 (3x4 矩阵)
        if len(pose_blob) >= 48:
            floats = struct.unpack('<12f', pose_blob[:48])
            yaw = math.degrees(math.atan2(floats[4], floats[0]))
            dx, dy = floats[3], floats[7]
            return yaw, dx, dy
        # 兼容 7 个 float32 (四元数 x,y,z,w + 位置 x,y,z)
        elif len(pose_blob) >= 28:
            floats = struct.unpack('<7f', pose_blob[:28])
            x, y, z, w = floats[0], floats[1], floats[2], floats[3]
            siny_cosp = 2 * (w * z + x * y)
            cosy_cosp = 1 - 2 * (y * y + z * z)
            yaw = math.degrees(math.atan2(siny_cosp, cosy_cosp))
            dx, dy = floats[4], floats[5]
            return yaw, dx, dy
    except Exception:
        return None, 0.0, 0.0
    return None, 0.0, 0.0


def verify_pose():
    if not os.path.exists(DB_PATH):
        print(f"❌ 找不到数据库文件: {DB_PATH}")
        return
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()

    print(f"===== 正在分析运动姿态: {DB_PATH} =====")
    cursor.execute("SELECT id, time_enter, pose FROM Node ORDER BY time_enter ASC")
    nodes = cursor.fetchall()
    if not nodes:
        print("❌ 数据库 Node 表为空。")
        return

    # 哈希表缓存区：node_id -> 运动特征字典
    data_cache = {}
    node_ids = [row[0] for row in nodes]
    node_times = [row[1] for row in nodes]

    last_time, last_yaw, last_dx, last_dy = None, None, None, None
    window = []  # 存储 (time, yaw, dx, dy)

    # ================= 阶段一：全量计算，写入哈希表 =================
    print("正在计算所有节点的运动特征（阶段一）...")
    for i, (node_id, time_str, pose_blob) in enumerate(nodes):
        current_time = parse_time(time_str)
        current_yaw, curr_dx, curr_dy = extract_pose_info(pose_blob)
        if current_time is None or current_yaw is None: continue

        window.append((current_time, current_yaw, curr_dx, curr_dy))
        while window and (current_time - window[0][0]).total_seconds() > WINDOW_SEC:
            window.pop(0)

        yaw_rate = 0.0
        net_yaw_sum = 0.0
        distance_moved = 0.0
        window_net_distance = 0.0

        if last_time is not None and last_yaw is not None:
            dt = (current_time - last_time).total_seconds()
            if dt >= 0.1:
                # 1. 瞬时角速度
                dyaw = current_yaw - last_yaw
                if dyaw > 180: dyaw -= 360
                if dyaw < -180: dyaw += 360
                yaw_rate = abs(dyaw) / dt

                # 2. 当前帧位移
                if last_dx is not None:
                    distance_moved = math.sqrt((curr_dx - last_dx) ** 2 + (curr_dy - last_dy) ** 2)

                # 3. 滑动窗口净累计角度（带方向抵消）
                net_yaw_sum = 0.0
                if len(window) >= 2:
                    for j in range(1, len(window)):
                        dy = window[j][1] - window[j - 1][1]
                        if dy > 180: dy -= 360
                        if dy < -180: dy += 360
                        net_yaw_sum += dy
                    net_yaw_sum = abs(net_yaw_sum)

                # 4. 滑动窗口内净位移距离
                if len(window) >= 2 and window[0][2] is not None:
                    window_net_distance = math.sqrt(
                        (window[-1][2] - window[0][2]) ** 2 + (window[-1][3] - window[0][3]) ** 2)

        # 存入哈希表
        data_cache[node_id] = {
            'yaw_rate': yaw_rate,
            'net_yaw_sum': net_yaw_sum,
            'distance_moved': distance_moved,
            'window_net_distance': window_net_distance,
            'time_str': time_str
        }

        last_time, last_yaw, last_dx, last_dy = current_time, current_yaw, curr_dx, curr_dy

    # ================= 阶段二：后处理筛选（双重锁） =================
    print("正在进行后处理筛选（阶段二）...")
    turn_indices = []
    for i, node_id in enumerate(node_ids):
        params = data_cache.get(node_id)
        if not params: continue

        yaw_rate = params['yaw_rate']
        net_yaw_sum = params['net_yaw_sum']
        dist_moved = params['distance_moved']
        win_dist = params['window_net_distance']

        # 核心双重锁逻辑：
        # 锁1：角度条件（瞬时角速度极大 或 连续5秒累计转角够大）
        has_turned = (yaw_rate >= YAW_RATE_THRESHOLD) or (net_yaw_sum >= YAW_NET_SUM_THRESHOLD)

        # 锁2：位移条件（确实移动了，且5秒窗口内有净位移，排除原地打转和视觉漂移）
        has_moved = (dist_moved >= MIN_DISTANCE_MOVED) and (win_dist >= MIN_WINDOW_DISTANCE)

        if has_turned and has_moved:
            turn_indices.append(i)
            # 可选：打印详细触发日志，方便调参
            # print(f"✅ 真实转弯: 节点 {node_id} | 瞬态: {yaw_rate:.1f} | 净转角: {net_yaw_sum:.1f} | 位移: {dist_moved:.3f}m | 窗位移: {win_dist:.3f}m")

    if not turn_indices:
        print("✅ 没有检测到任何满足双重锁条件的转弯事件。")
        conn.close()
        return

    # 将相邻的转弯节点合并成组
    groups = []
    current_group = [turn_indices[0]]
    for idx in turn_indices[1:]:
        if idx - current_group[-1] <= FRAMES_BEFORE_AND_AFTER * 2:
            current_group.append(idx)
        else:
            groups.append(current_group)
            current_group = [idx]
    groups.append(current_group)

    print(f"共检测到 {len(turn_indices)} 个有效转弯点，合并为 {len(groups)} 组。开始抽图...")

    # ================= 阶段三：抽帧与标注 =================
    for group_idx, group in enumerate(groups, start=1):
        center_idx = group[len(group) // 2]
        center_node_id = node_ids[center_idx]

        center_params = data_cache.get(center_node_id, {})
        print(f"\n📊 第 {group_idx} 组 触发参数: 节点 {center_node_id} | "
              f"瞬时角速度: {center_params.get('yaw_rate', 0):.2f} | "
              f"净累计转角: {center_params.get('net_yaw_sum', 0):.2f} | "
              f"位移: {center_params.get('distance_moved', 0):.3f}m | "
              f"窗位移: {center_params.get('window_net_distance', 0):.3f}m")

        start_idx = max(0, center_idx - FRAMES_BEFORE_AND_AFTER)
        end_idx = min(len(nodes) - 1, center_idx + FRAMES_BEFORE_AND_AFTER)

        saved_in_group = 0
        for idx in range(start_idx, end_idx + 1):
            node_id = node_ids[idx]
            time_str = node_times[idx]

            cursor.execute("SELECT image FROM Data WHERE id=?", (node_id,))
            row = cursor.fetchone()
            if row and row[0]:
                img_array = np.frombuffer(row[0], np.uint8)
                img = cv2.imdecode(img_array, cv2.IMREAD_COLOR)
                if img is not None:
                    params = data_cache.get(node_id, {})

                    # 在图片上写字（OpenCV不支持中文，我们写英文缩写）
                    text1 = f"Node {node_id}"
                    text2 = f"YawRate: {params.get('yaw_rate', 0):.1f}"
                    text3 = f"NetYaw: {params.get('net_yaw_sum', 0):.1f}"
                    text4 = f"Dist: {params.get('distance_moved', 0):.3f}m"

                    cv2.putText(img, text1, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
                    cv2.putText(img, text2, (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
                    cv2.putText(img, text3, (10, 90), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
                    cv2.putText(img, text4, (10, 120), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)

                    saved_in_group += 1
                    save_name = f"g{group_idx}_{saved_in_group}_node{node_id}.jpg"
                    save_path = os.path.join(OUTPUT_DIR, save_name)
                    cv2.imwrite(save_path, img)

    conn.close()
    print(f"\n✅ 验证提取完成！共保存 {len(groups)} 组图片，位于：{OUTPUT_DIR}")


if __name__ == '__main__':
    verify_pose()