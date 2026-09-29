import os
import re
import json
import cv2


def get_latest_turn_dir(base="check_turns"):
    """自动找到 check_turns 下最新的 run_ 目录"""
    if not os.path.exists(base):
        return None
    dirs = [os.path.join(base, d) for d in os.listdir(base) if d.startswith("run_")]
    return max(dirs, key=os.path.getmtime) if dirs else None


def merge_to_timeline(base_dir, turn_dir, output_dir="timeline_frames"):
    """
    把基础定时抽帧和转弯抽帧合并到一条时间轴上
    - base_dir: 基础抽帧文件夹 (如 extracted_frames_10s)
    - turn_dir: 转弯抽帧文件夹 (如 check_turns/run_xxx)
    - output_dir: 合并后的统一输出文件夹
    """
    if turn_dir is None or not os.path.exists(turn_dir):
        print(f"❌ 转弯目录无效: {turn_dir}")
        return

    os.makedirs(output_dir, exist_ok=True)

    # node_id -> {路径, 来源}
    node_map = {}

    # 1. 扫描基础帧 (文件名: frame_0001_194153.jpg)
    for f in os.listdir(base_dir):
        if not f.lower().endswith(".jpg"):
            continue
        m = re.search(r"frame_(\d+)_", f)
        if m:
            nid = int(m.group(1))
            node_map[nid] = {"path": os.path.join(base_dir, f), "source": "base"}

    # 2. 扫描转弯帧 (文件名: g1_1_node387.jpg)，转弯帧优先覆盖基础帧
    for f in os.listdir(turn_dir):
        if not f.lower().endswith(".jpg"):
            continue
        m = re.search(r"node(\d+)\.jpg", f)
        if m:
            nid = int(m.group(1))
            node_map[nid] = {"path": os.path.join(turn_dir, f), "source": "turn"}

    # 3. 按 node_id 升序排列，统一重命名并保存
    sorted_nodes = sorted(node_map.keys())
    manifest = []
    for seq, nid in enumerate(sorted_nodes, start=1):
        info = node_map[nid]
        img = cv2.imread(info["path"])
        if img is None:
            continue
        new_name = f"frame_{seq:05d}_node{nid:04d}.jpg"
        cv2.imwrite(os.path.join(output_dir, new_name), img)
        manifest.append({
            "sequence": seq,
            "node_id": nid,
            "source": info["source"],   # "base" 或 "turn"
            "filename": new_name
        })

    # 4. 写 manifest.json
    manifest_path = os.path.join(output_dir, "manifest.json")
    with open(manifest_path, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2, ensure_ascii=False)

    base_count = sum(1 for m in manifest if m["source"] == "base")
    turn_count = sum(1 for m in manifest if m["source"] == "turn")
    print(f"✅ 合并完成！共 {len(manifest)} 张图 (基础帧 {base_count} + 转弯帧 {turn_count})")
    print(f"📄 时间轴清单: {manifest_path}")
    return manifest