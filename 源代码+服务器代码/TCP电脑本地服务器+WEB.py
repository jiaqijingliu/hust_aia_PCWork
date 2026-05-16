from flask import Flask, request
import threading
import logging
import socket
from datetime import datetime
import json

app = Flask(__name__)

current_temp = "无数据"
last_update = "无"
temp_history = []  # 温度历史 [{"time":"...", "temp":25.5}, ...]
lock = threading.Lock()
MAX_POINTS = 120  # 最多保存数据点（

logging.basicConfig(level=logging.INFO)


# ----------------------
# 更新温度 + 保存历史
# ----------------------
def update_temp_from_text(text):
    global current_temp, last_update, temp_history

    text = text.strip()
    if "TEMP:" in text:
        try:
            temp_str = text.split("TEMP:")[-1].strip()
            temp_val = float(temp_str)
            now_time = datetime.now().strftime("%H:%M:%S")

            with lock:
                current_temp = temp_str
                last_update = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                # 保存历史
                temp_history.append({"time": now_time, "temp": temp_val})
                # 限制数量
                if len(temp_history) > MAX_POINTS:
                    temp_history.pop(0)
            return True
        except:
            pass
    return False


# ----------------------
# 首页：高级图表版
# ----------------------
@app.route("/")
def index():
    return """
    <!DOCTYPE html>
    <html>
    <head>
        <meta charset="utf-8">
        <title>温度实时监控系统</title>
        <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
        <style>
            *{box-sizing:border-box;}
            body{font-family:Arial;margin:0;padding:20px;background:#f5f5f5;}
            .panel{background:white;padding:20px;border-radius:12px;box-shadow:0 2px 10px rgba(0,0,0,0.1);margin-bottom:20px;}
            .grid{display:grid;grid-template-columns:repeat(4,1fr);gap:15px;margin-bottom:20px;}
            .card{background:white;padding:18px;border-radius:10px;text-align:center;box-shadow:0 2px 6px rgba(0,0,0,0.08);}
            .card .label{font-size:14px;color:#666;margin-bottom:6px;}
            .card .value{font-size:28px;font-weight:bold;}
            .current{color:#e53935;}
            .max{color:#d81b60;}
            .min{color:#1e88e5;}
            .avg{color:#43a047;}
            canvas{width:100% !important;height:420px !important;}
        </style>
    </head>

    <body>
        <div class="panel">
            <h1>🌡️ 温度实时监控系统</h1>
        </div>

        <div class="grid">
            <div class="card">
                <div class="label">当前温度</div>
                <div class="value current" id="temp">-</div>
            </div>
            <div class="card">
                <div class="label">最高温度</div>
                <div class="value max" id="max">-</div>
            </div>
            <div class="card">
                <div class="label">最低温度</div>
                <div class="value min" id="min">-</div>
            </div>
            <div class="card">
                <div class="label">平均温度</div>
                <div class="value avg" id="avg">-</div>
            </div>
        </div>

        <div class="panel">
            <h3>温度变化曲线</h3>
            <canvas id="tempChart"></canvas>
        </div>

        <script>
            const ctx = document.getElementById('tempChart').getContext('2d');
            const chart = new Chart(ctx, {
                type: 'line',
                data: {
                    labels: [],
                    datasets: [{
                        label: '温度 (℃)',
                        data: [],
                        borderColor: '#e53935',
                        backgroundColor: 'rgba(255,229,229,0.3)',
                        borderWidth: 3,
                        tension: 0.3,
                        fill: true,
                        pointRadius: 3,
                    }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    plugins: {
                        legend: {labels: {fontSize:14}},
                        tooltip: {mode: 'index', intersect: false}
                    },
                    scales: {
                        y: {beginAtZero: false, grid:{color:'#eee'}},
                        x: {grid:{display:false}}
                    }
                }
            });

            // 每 1 秒刷新一次数据
            setInterval(async () => {
                const res = await fetch("/get_data");
                const d = await res.json();

                document.getElementById("temp").innerText = d.current + " ℃";
                document.getElementById("max").innerText = d.max + " ℃";
                document.getElementById("min").innerText = d.min + " ℃";
                document.getElementById("avg").innerText = d.avg + " ℃";

                chart.data.labels = d.times;
                chart.data.datasets[0].data = d.values;
                chart.update();
            }, 400);
        </script>
    </body>
    </html>
    """


# ----------------------
# 图表数据接口（返回所有历史）
# ----------------------
@app.route("/get_data")
def get_data():
    with lock:
        times = [t["time"] for t in temp_history]
        values = [t["temp"] for t in temp_history]
        current = current_temp

        if not values:
            return {"current": current, "max": "-", "min": "-", "avg": "-", "times": times, "values": values}

        max_temp = round(max(values), 1)
        min_temp = round(min(values), 1)
        avg_temp = round(sum(values) / len(values), 1)

    return {
        "current": current,
        "max": max_temp,
        "min": min_temp,
        "avg": avg_temp,
        "times": times,
        "values": values
    }


# ----------------------
# HTTP 上传
# ----------------------
@app.route("/upload", methods=["POST", "GET"])
def upload():
    if request.method == "GET":
        return "请用POST发送: TEMP:26.3"
    data = request.get_data(as_text=True)
    ok = update_temp_from_text(data)
    return "OK" if ok else "ERROR"


# ----------------------
# TCP 服务器
# ----------------------
def tcp_server():
    host = "0.0.0.0"
    port = 3000
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((host, port))
    s.listen(5)
    print(f"✅ TCP服务器启动：{port} 端口，等待4G模块...")

    while True:
        conn = None
        try:
            conn, addr = s.accept()
            print(f"📶 4G模块已连接：{addr}")
            while True:
                data = conn.recv(1024)
                if not data:
                    break
                msg = data.decode("utf-8", errors="ignore")
                update_temp_from_text(msg)
                conn.sendall(b"OK\r\n")
        except:
            pass
        finally:
            if conn:
                conn.close()


# ----------------------
# 启动
# ----------------------
if __name__ == "__main__":
    threading.Thread(target=tcp_server, daemon=True).start()
    print("✅ Web服务器启动：http://0.0.0.0:5000")
    app.run(host="0.0.0.0", port=5000, debug=False, use_reloader=False)