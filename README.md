# 视觉感知流水线 — RTAB-Map 抽帧工具

> **作者：** Ray Duan  
> **分支：** `feature/db-frame-extraction`  
> **范围：** 本 README 仅覆盖视觉感知侧的数据处理流水线。有关机器人硬件、SLAM、MQTT/Corelink 集成，请参阅对应仓库。

---

## 概述

本流水线用于处理 RTAB-Map 在机器人建图过程中生成的离线 `.db` 文件，自动提取具有代表性的图像帧，用于后续 **YOLOv8 微调**。它同时兼顾了直线走廊的均匀采样与急转弯处的动态采样，最终将所有帧合并为一条统一时间轴，并生成 `manifest.json` 索引文件。

**输入：** `rtabmap.db`（RTAB-Map 输出的数据库文件）  
**输出：** `timeline_frames/`（精选 JPG 图片）+ `manifest.json`（含来源标记的索引文件）

---

## 流水线架构

```text
rtabmap.db
    │
    ▼
┌──────────────────────┐
│  general_db_info.py  │  ← 步骤 1：体检数据库（表结构、帧率、时长）
└──────────┬───────────┘
           │
           ▼
┌─────────────────────────────┐
│ general_extract_by_time_db  │  ← 步骤 2：均匀抽帧（默认每 10 秒一帧）
└──────────┬──────────────────┘
           │
           ▼
┌──────────────────────────────┐
│  general_extract_by_pose.py  │  ← 步骤 3：基于姿态的转弯检测与抽帧
└──────────┬───────────────────┘
           │
           ▼
┌──────────────────────────────┐
│  general_merge_timeline.py   │  ← 步骤 4：合并、重命名、生成清单
└──────────┬───────────────────┘
           │
           ▼
    timeline_frames/
      ├── frame_00001_node0042.jpg
      ├── frame_00002_node0089.jpg
      └── manifest.json

所有步骤由 main.py 统一调度。

文件说明
文件	用途
main.py	入口脚本，按顺序执行完整的 4 步流水线。
general_db_info.py	读取 .db 表结构，统计帧数、计算平均帧率与总时长。
general_extract_by_time_db.py	每 SECONDS_INTERVAL 秒抽取一帧（默认 10.0 秒）。
general_extract_by_pose.py	从 Node.pose 数据中检测真实转弯，并在转弯处提取关键帧簇。
general_merge_timeline.py	合并基础帧与转弯帧，统一重命名，写入 manifest.json。
pyproject.toml / uv.lock	Python 项目元数据（由 uv 管理）。
环境依赖
Python ≥ 3.11

opencv-python

numpy

uv（推荐）或 pip

安装依赖：

bash
uv pip install opencv-python numpy
或使用 pip：

bash
pip install opencv-python numpy
