#!/usr/bin/env python3
import socket

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(('0.0.0.0', 8080))
    s.listen(1)
    print("Server listening on port 8080...")
    
    conn, addr = s.accept()
    print(f"Connected by {addr}")
    
    data = conn.recv(1024)
    print(f"Received: {data.decode()}")
    conn.sendall(data)
