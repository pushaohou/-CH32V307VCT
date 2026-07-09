"""
guide_cane_monitor.py — 智能导盲杖 蓝牙看护端
=============================================
通过 PC 蓝牙串口（HC-05 SPP）接收导盲杖数据，实时显示并记录日志。

使用方法：
  1. PC 蓝牙配对 HC-05（系统蓝牙设置 → 添加设备 → 搜索 → HC-05 → 配对）
  2. 配对后在设备管理器里查到 COM 端口号（如 COM3）
  3. python guide_cane_monitor.py COM3
  4. 若要同时记录日志：python guide_cane_monitor.py COM3 --log

依赖：pip install pyserial
"""

import sys
import time
import serial
import os
from datetime import datetime

# ---- 配置 ----
BAUDRATE = 9600
TIMEOUT  = 2.0
LINE_W   = 55

ALERT_COLORS = {
    "OK":     "\033[92m",  # 绿
    "WARN":   "\033[93m",  # 黄
    "DANGER": "\033[91m",  # 红
    "FALL":   "\033[41m\033[97m",  # 红底白字
    "RESET":  "\033[0m",
}

ALERT_SYMBOLS = {
    "OK":     "●",
    "WARN":   "▲",
    "DANGER": "◆",
    "FALL":   "!!!",
}


def parse_line(line: str) -> dict | None:
    """解析 "F:050 L:120 R:035 P:+01.5 R:-02.3 OK" 格式"""
    try:
        parts = line.strip().split()
        if len(parts) < 6:
            return None

        data = {}
        for p in parts[:5]:
            if ":" not in p:
                return None
            k, v = p.split(":", 1)
            data[k] = float(v)

        data["alert"] = parts[5]
        return data
    except (ValueError, IndexError):
        return None


def draw_display(data: dict):
    """终端界面绘制"""
    alert = data["alert"]
    c  = ALERT_COLORS.get(alert, "")
    cr = ALERT_COLORS["RESET"]
    s  = ALERT_SYMBOLS.get(alert, "?")

    os.system("cls" if os.name == "nt" else "clear")

    print("=" * LINE_W)
    print("      智能导盲杖 · 看护端监控")
    print("=" * LINE_W)
    print()

    # 测距栏
    print("  测距 (cm)")
    f_val = int(data["F"])
    l_val = int(data["L"])
    r_val = int(data["R"])

    bar_max = 22
    for label, val in [("前 F", f_val), ("左 L", l_val), ("右 R", r_val)]:
        bar_len = min(int(val / 200 * bar_max), bar_max)
        bar = "█" * bar_len + "░" * (bar_max - bar_len)
        print(f"  {label} │{bar}│ {val:>3} cm")

    print()

    # 姿态
    print(f"  Pitch: {data['P']:+06.1f}°     Roll: {data['R']:+06.1f}°")
    print()

    # 状态
    label = f"  {s} {alert}"
    padding = LINE_W - len(label) - 1
    print(f"{c}{label}{' ' * padding}{cr}")
    print()

    # 告警说明
    if alert == "FALL":
        print(f"{c}  ╔══════════════════════════╗{cr}")
        print(f"{c}  ║  !! 跌倒警报 !! 请立即响应  ║{cr}")
        print(f"{c}  ╚══════════════════════════╝{cr}")
    elif alert == "DANGER":
        print(f"{c}    障碍物过近！有碰撞风险{cr}")
    elif alert == "WARN":
        print(f"{c}    前方有障碍物，请注意{cr}")

    print()
    print("=" * LINE_W)
    print("  Ctrl+C 退出 | 数据自动记录到日志文件")


def main():
    if len(sys.argv) < 2:
        print("用法: python guide_cane_monitor.py <COM端口> [--log]")
        print("示例: python guide_cane_monitor.py COM3")
        print("      python guide_cane_monitor.py COM3 --log")
        sys.exit(1)

    port = sys.argv[1]
    do_log = "--log" in sys.argv

    log_file = None
    if do_log:
        log_name = f"guide_cane_{datetime.now().strftime('%Y%m%d_%H%M%S')}.log"
        log_file = open(log_name, "w", encoding="utf-8")
        print(f"[日志] 记录到 {log_name}")

    print(f"[连接] 正在连接 {port} @ {BAUDRATE} bps...")

    try:
        ser = serial.Serial(port, BAUDRATE, timeout=TIMEOUT)
    except serial.SerialException as e:
        print(f"[错误] 无法打开串口 {port}: {e}")
        print("请确认:")
        print("  1. HC-05 已配对并在设备管理器中有 COM 端口")
        print("  2. 端口号正确（如 COM3, COM4, ...）")
        print("  3. 没有其他程序占用该端口")
        sys.exit(1)

    print(f"[就绪] 已连接 {port}，等待数据...")
    print("        按 Ctrl+C 退出")
    time.sleep(1)

    try:
        while True:
            raw = ser.readline()
            if not raw:
                continue

            line_str = raw.decode("utf-8", errors="ignore").strip()
            if not line_str:
                continue

            data = parse_line(line_str)
            if data is None:
                # 非数据行也打印（如 "Guide Cane Online"）
                if line_str:
                    print(f"[信息] {line_str}")
                continue

            draw_display(data)

            # 记录日志
            if do_log and log_file:
                ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                log_file.write(f"{ts} | {line_str}\n")
                log_file.flush()

    except KeyboardInterrupt:
        print("\n[退出] 监控已停止")
    except serial.SerialException as e:
        print(f"\n[错误] 串口连接断开: {e}")
    finally:
        ser.close()
        if log_file:
            log_file.close()
            print(f"[日志] 已保存到 {log_name}")


if __name__ == "__main__":
    main()
