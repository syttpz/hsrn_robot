import sqlite3
import os
from datetime import datetime


def inspect_db(db_path):
    if not os.path.exists(db_path):
        print(f"Error: File not found {db_path}")
        return

    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    print(f"===== Report for .db: {os.path.basename(db_path)} =====")

    # 1. 获取基础表结构
    cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
    tables = [row[0] for row in cursor.fetchall()]
    print(f"\nFind {len(tables)} charts ：")

    for table in tables:
        cursor.execute(f"SELECT COUNT(*) FROM '{table}'")
        count = cursor.fetchone()[0]
        cursor.execute(f"PRAGMA table_info('{table}')")
        columns = [col[1] for col in cursor.fetchall()]
        print(f"  - Name of the form: {table}\n    # lines: {count}\n    included title: {', '.join(columns)}")

        if table == 'Data':
            cursor.execute("SELECT COUNT(*) FROM Data WHERE image IS NOT NULL")
            img_count = cursor.fetchone()[0]
            print(f"    -> # lines included pictures: {img_count}")

    # 2. 深度分析 Node 表（时间、帧率、时长）
    print("\n===== Report for the video flow =====")
    try:
        # 提取 Node 表中的最小、最大时间戳以及总节点数
        cursor.execute("SELECT MIN(time_enter), MAX(time_enter), COUNT(*) FROM Node")
        min_time_str, max_time_str, total_nodes = cursor.fetchone()

        if min_time_str and max_time_str and total_nodes > 0:
            # 解析时间格式
            fmt = "%Y-%m-%d %H:%M:%S"
            # 注意：这里可能需要处理毫秒或时区，视数据库实际保存格式而定
            try:
                t_min = datetime.strptime(min_time_str.split('.')[0], fmt)
                t_max = datetime.strptime(max_time_str.split('.')[0], fmt)
            except ValueError:
                # 兼容带有毫秒但秒数后不标准的情况
                t_min = datetime.strptime(min_time_str[:19], fmt)
                t_max = datetime.strptime(max_time_str[:19], fmt)

            total_seconds = (t_max - t_min).total_seconds()

            # 计算平均帧率
            avg_fps = total_nodes / total_seconds if total_seconds > 0 else 0

            print(f"  - Start time: {min_time_str}")
            print(f"  - End time: {max_time_str}")
            print(f"  - Total frame (#Node): {total_nodes} frame")
            print(f"  - Total video length: {total_seconds:.2f} seconds (Around {total_seconds / 60:.2f} minutes)")
            print(f"  - Average frame/second: {avg_fps:.2f} FPS")
            print(f"\n💡 Advice for selected frame：Since the total # frame is {total_nodes}, set the frame between 1~{total_nodes}  ")
    except Exception as e:
        print(f"  - Report of Node error test: {e}")

    conn.close()
    print("\n===== Report end =====")


if __name__ == '__main__':
    inspect_db("rtabmap.db")