"""
guide_cane_server.py — 智能导盲杖 串口→网页 实时数据服务
=========================================================
读取 WCH-Link 调试串口(COMx, 115200bps)，解析传感器数据，
通过 HTTP 提供给网页前端实时显示。
跌倒时自动发送邮件通知预设联系人。

用法: python guide_cane_server.py [COM端口] [端口号]
示例: python guide_cane_server.py COMx 8080
"""

import sys
import json
import time
import smtplib
import serial
import threading
from email.mime.text import MIMEText
from http.server import HTTPServer, BaseHTTPRequestHandler

# ---- 配置 ----
SERIAL_PORT = sys.argv[1] if len(sys.argv) > 1 else "COMx"
SERIAL_BAUD = 115200
HTTP_PORT   = int(sys.argv[2]) if len(sys.argv) > 2 else 8080

# ---- 邮件通知配置 (QQ邮箱SMTP) ----
# 凭据通过环境变量传入，不要在代码中写明文密码
# Windows: set GCM_EMAIL_SENDER=xxx@qq.com
#          set GCM_EMAIL_PASSWORD=授权码
#          set GCM_EMAIL_TO=xxx@qq.com
# QQ邮箱 → 设置 → 账户 → POP3/SMTP服务 → 开启 → 获取授权码
import os as _os
EMAIL_ENABLED   = bool(_os.environ.get("GCM_EMAIL_SENDER"))
EMAIL_SMTP      = "smtp.qq.com"
EMAIL_PORT      = 587
EMAIL_SENDER    = _os.environ.get("GCM_EMAIL_SENDER", "")
EMAIL_PASSWORD  = _os.environ.get("GCM_EMAIL_PASSWORD", "")
EMAIL_RECIPIENT = _os.environ.get("GCM_EMAIL_TO", "")
# 也可用手机号邮箱: 移动=号码@139.com, 联通=号码@wo.cn, 电信=号码@189.cn

# ---- 共享数据（线程安全） ----
data_lock = threading.Lock()
latest = {
    "front_cm": 0,
    "left_cm":  0,
    "right_cm": 0,
    "angle":    0,
    "fall":     0,
    "alert":    "OK",
    "raw":      "",
    "updated":  False,
    "notified": False,    # 本次跌倒是否已发送通知
}
_notification_sent = False    # 去重：同一次跌倒只发一次


def parse_safe(line: str):
    """解析 $SAFE,fall_flag,angle#"""
    try:
        parts = line.replace("$SAFE,", "").replace("#", "").split(",")
        if len(parts) >= 2:
            with data_lock:
                latest["fall"] = int(parts[0])
                latest["angle"] = int(parts[1])
                if latest["fall"]:
                    latest["alert"] = "FALL"
                latest["updated"] = True
    except ValueError:
        pass


def parse_hc(line: str):
    """解析 HC: ch0 dur=xxxxx us dist=xxx cm"""
    try:
        # "HC: ch0 dur=32990 us dist=568 cm"
        parts = line.replace("HC:", "").strip().split()
        ch   = int(parts[0][2:])        # "ch0" → 0
        dist = int(parts[3].split("=")[1])  # "dist=568" → 568

        key = {0: "front_cm", 1: "left_cm", 2: "right_cm"}.get(ch)
        if key:
            with data_lock:
                # 超时值(>500cm)视为无目标
                latest[key] = dist if dist < 500 else 0
                latest["updated"] = True
    except (ValueError, IndexError):
        pass


def send_fall_email():
    """发送跌倒通知邮件"""
    if not EMAIL_ENABLED:
        print("[通知] 邮件功能未启用，跳过发送")
        return False
    try:
        subject = "【紧急】智能导盲杖 — 跌倒警报！"
        body = f"""跌倒警报通知

智能导盲杖检测到使用者跌倒，请立即确认安全。

时间: {time.strftime('%Y-%m-%d %H:%M:%S')}
倾角: {latest['angle']}°
前方距离: {latest['front_cm']} cm
左侧距离: {latest['left_cm']} cm
右侧距离: {latest['right_cm']} cm

此邮件由智能导盲杖看护系统自动发送。
"""
        msg = MIMEText(body, "plain", "utf-8")
        msg["Subject"] = subject
        msg["From"] = EMAIL_SENDER
        msg["To"] = EMAIL_RECIPIENT

        server = smtplib.SMTP(EMAIL_SMTP, EMAIL_PORT, timeout=10)
        server.starttls()
        server.login(EMAIL_SENDER, EMAIL_PASSWORD)
        server.sendmail(EMAIL_SENDER, [EMAIL_RECIPIENT], msg.as_string())
        server.quit()
        print(f"[通知] 跌倒邮件已发送至 {EMAIL_RECIPIENT}")
        return True
    except Exception as e:
        print(f"[通知] 邮件发送失败: {e}")
        return False


def determine_alert():
    """根据距离和跌倒状态更新告警等级，跌倒时触发邮件通知"""
    global _notification_sent
    with data_lock:
        if latest["fall"]:
            latest["alert"] = "FALL"
            # 同一次跌倒只发一次通知
            if not _notification_sent:
                if send_fall_email():
                    latest["notified"] = True
                    _notification_sent = True
            return
        # 跌倒解除，重置通知状态
        if _notification_sent:
            _notification_sent = False
            latest["notified"] = False

        f = latest["front_cm"]
        l = latest["left_cm"]
        r = latest["right_cm"]
        near = min(f or 999, l or 999, r or 999)
        if near < 20:
            latest["alert"] = "DANGER"
        elif near < 50:
            latest["alert"] = "WARN"
        else:
            latest["alert"] = "OK"


def serial_reader():
    """后台线程：持续读取串口数据"""
    print(f"[串口] 连接 {SERIAL_PORT} @ {SERIAL_BAUD}...")
    try:
        ser = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=0.5)
    except serial.SerialException as e:
        print(f"[错误] 无法打开串口: {e}")
        return

    print(f"[串口] 已连接，等待数据...")
    while True:
        try:
            raw = ser.readline()
            if not raw:
                continue
            line = raw.decode("utf-8", errors="ignore").strip()
            if not line:
                continue

            if line.startswith("$SAFE"):
                parse_safe(line)
            elif line.startswith("HC:"):
                parse_hc(line)
            elif line:
                with data_lock:
                    latest["raw"] = line

            determine_alert()

        except serial.SerialException as e:
            print(f"[串口] 断开: {e}")
            time.sleep(2)
            try:
                ser.close()
                ser = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=0.5)
                print("[串口] 重连成功")
            except Exception:
                print("[串口] 重连失败，2秒后重试...")
                time.sleep(2)
        except Exception as e:
            print(f"[错误] {e}")
            time.sleep(0.1)


# ---- HTTP 服务 ----
HTML_FILE = __file__.replace("_server.py", "_monitor.html")


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/data":
            # API: 返回 JSON 数据
            with data_lock:
                payload = json.dumps({
                    "front_cm": latest["front_cm"],
                    "left_cm":  latest["left_cm"],
                    "right_cm": latest["right_cm"],
                    "angle":    latest["angle"],
                    "fall":     latest["fall"],
                    "alert":    latest["alert"],
                    "notified": latest["notified"],
                }, ensure_ascii=False)
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.send_header("Cache-Control", "no-cache")
            self.end_headers()
            self.wfile.write(payload.encode("utf-8"))

        elif self.path == "/" or self.path == "/index.html" or self.path.startswith("/?"):
            # 网页 (支持 ?v=xxx 缓存刷新)
            try:
                with open(HTML_FILE, "r", encoding="utf-8") as f:
                    html = f.read()
                # 注入时间戳强制浏览器刷新
                html = html.replace("<head>", "<head>\n<meta http-equiv=\"Cache-Control\" content=\"no-cache, no-store, must-revalidate\">")
                self.send_response(200)
                self.send_header("Content-Type", "text/html; charset=utf-8")
                self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
                self.send_header("Pragma", "no-cache")
                self.send_header("Expires", "0")
                self.end_headers()
                self.wfile.write(html.encode("utf-8"))
            except FileNotFoundError:
                self.send_error(404, "HTML file not found")

        else:
            self.send_error(404)

    def log_message(self, format, *args):
        # 静默日志
        pass


def main():
    print("=" * 50)
    print("  智能导盲杖 · 实时数据服务")
    print(f"  串口: {SERIAL_PORT} | HTTP: http://localhost:{HTTP_PORT}")
    print("=" * 50)

    # 启动串口读取线程
    t = threading.Thread(target=serial_reader, daemon=True)
    t.start()

    # 启动 HTTP 服务
    import socket
    server = HTTPServer(("127.0.0.1", HTTP_PORT), Handler)
    server.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    print(f"\n  打开浏览器访问: http://127.0.0.1:{HTTP_PORT}\n")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[退出] 服务已停止")
        server.shutdown()


if __name__ == "__main__":
    main()
