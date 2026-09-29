'''Author : Ray Duan & AIs'''

import os
from general_db_info import inspect_db
from general_extract_by_time_db import extract_frames_by_time
from general_extract_by_pose import verify_pose
from general_merge_timeline import merge_to_timeline, get_latest_turn_dir


if __name__ == '__main__':
    # 统一配置数据库路径（相对路径，因为脚本就在这个文件夹里）
    DB_PATH = "rtabmap.db"

    # 检查数据库文件是否存在
    if not os.path.exists(DB_PATH):
        print(f"❌ 找不到数据库文件: {DB_PATH}，请确保它在项目根目录下。")
    else:
        print("========== 开始执行流水线 ==========\n")

        # 第一步：体检
        print("【步骤 1/2】正在执行体检...")
        inspect_db(DB_PATH)
        print("\n------------------------------------\n")

        # 第二步：抽帧
        print("【步骤 2/2】正在执行按时间抽帧...")
        # 注意：如果你的抽帧脚本里 DB_PATH 写死了，这里可以直接调用
        extract_frames_by_time()

        # 步骤 3/4：转弯抽帧
        print("【步骤 3/4】转弯抽帧...")
        verify_pose()
        print("\n------------------------------------\n")

        # 步骤 4/4：合并到统一时间轴
        print("【步骤 4/4】合并到统一时间轴...")
        latest_turn_dir = get_latest_turn_dir("check_turns")
        merge_to_timeline(
            base_dir="extracted_frames_10s",
            turn_dir=latest_turn_dir,
            output_dir="timeline_frames"
        )

        print("\n========== 流水线执行完毕 ==========")