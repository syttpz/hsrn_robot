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
```

所有步骤由 `main.py` 统一调度。

---

## 文件说明

| 文件 | 用途 |
|------|------|
| `main.py` | 入口脚本，按顺序执行完整的 4 步流水线。 |
| `general_db_info.py` | 读取 `.db` 表结构，统计帧数、计算平均帧率与总时长。 |
| `general_extract_by_time_db.py` | 每 `SECONDS_INTERVAL` 秒抽取一帧（默认 10.0 秒）。 |
| `general_extract_by_pose.py` | 从 `Node.pose` 数据中检测真实转弯，并在转弯处提取关键帧簇。 |
| `general_merge_timeline.py` | 合并基础帧与转弯帧，统一重命名，写入 `manifest.json`。 |
| `pyproject.toml` / `uv.lock` | Python 项目元数据（由 `uv` 管理）。 |

---

## 环境依赖

- Python ≥ 3.11
- `opencv-python`
- `numpy`
- `uv`（推荐）或 `pip`

安装依赖：

```bash
uv pip install opencv-python numpy
```

或使用 pip：

```bash
pip install opencv-python numpy
```

---

## 使用方法

1. 将你的 `.db` 文件放在项目根目录，并重命名为 `rtabmap.db`（或在 `main.py` 中修改 `DB_PATH`）。
2. 运行完整流水线：

```bash
python main.py
```

也可单独运行每一步：

```bash
python general_db_info.py
python general_extract_by_time_db.py
python general_extract_by_pose.py
python general_merge_timeline.py
```

---

## 配置参数说明

### `general_extract_by_time_db.py`

| 参数 | 默认值 | 含义 |
|------|--------|------|
| `SECONDS_INTERVAL` | `10.0` | 每隔 N 秒采样一帧。 |

### `general_extract_by_pose.py`

| 参数 | 默认值 | 含义 |
|------|--------|------|
| `YAW_RATE_THRESHOLD` | `35.0` | 瞬时角速度阈值（度/秒），用于过滤抖动。 |
| `YAW_NET_SUM_THRESHOLD` | `25.0` | 窗口内净累计偏航角阈值（度），用于过滤缓慢漂移。 |
| `WINDOW_SEC` | `10.0` | 滑动窗口长度（秒）。 |
| `MIN_DISTANCE_MOVED` | `0.05` | 单帧最小位移（米），过滤原地打转。 |
| `MIN_WINDOW_DISTANCE` | `0.15` | 窗口内最小净位移（米），过滤视觉漂移。 |
| `FRAMES_BEFORE_AND_AFTER` | `4` | 每个检测到的转弯前后各抽取的帧数。 |

> **调参说明：** 这些数值是在 NYU HSRN 机房数据集上经过 20 多组实验后确定的，在召回率（不错过真实转弯）与精确率（拒绝原地旋转和 SLAM 姿态漂移）之间取得了平衡。

---

## 输出格式

```text
timeline_frames/
├── frame_00001_node0042.jpg
├── frame_00002_node0089.jpg
├── ...
└── manifest.json
```

`manifest.json` 中每条记录如下：

```json
{
  "sequence": 1,
  "node_id": 42,
  "source": "base",         // "base" = 定时采样, "turn" = 姿态检测触发
  "filename": "frame_00001_node0042.jpg"
}
```

`source` 字段可让下游标注工具优先处理转弯帧（这些帧通常包含更丰富的场景变化）。

---

## 姿态解析说明

`general_extract_by_pose.py` 支持 RTAB-Map 存储的两种 `pose` 二进制格式：

- **12 × float32** — 3×4 变换矩阵（偏航角通过 `atan2(r21, r11)` 提取）
- **7 × float32** — 四元数 (x, y, z, w) + 平移 (x, y, z)

脚本会根据二进制长度自动识别格式。

---

## 已知限制

- 转弯检测依赖 RTAB-Map 的姿态输出；SLAM 在纹理稀疏的走廊中定位失败时，可能产生虚假的偏航角尖峰。
- 本流水线不进行语义标注——提取的帧未标注，需人工使用标注工具处理（参见 `TODO.txt`）。
- 不处理相机内参标定；假设 `.db` 是由已标定的 RealSense D435 采集的。

---

## 下一步计划

完整路线图见 `TODO.txt`。近期下一阶段：

1. 使用 LabelImg / Roboflow 标注 30–50 帧（类别：`cabinet`、`cable`、`box`、`chair`、`warning_sign`、`human`）
2. 在本地使用 `freeze=10` 微调 YOLOv8n
3. 集成 RealSense D435 深度信息，估计障碍物距离
4. 封装为 ROS 2 节点（`vision_perception`）并部署到机器人

---

## 许可证

内部项目 — NYU HSRN VIP ROBOTIC。不对外分发。
