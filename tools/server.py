#!/usr/bin/env python
import socket
import time

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.bind(('0.0.0.0', 3234))
sock.listen()
s, a = sock.accept()
print('client connected')
for i in range(10):
    data = f'i={i}'.encode()
    s.send(data)
    print('send', data)
    data = s.recv(100)
    print('recv', data)
    time.sleep(1)
