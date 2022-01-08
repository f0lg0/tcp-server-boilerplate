import socket
from time import sleep

HOST = "0.0.0.0"
PORT = 1234

clients = []

for i in range(0, 10):
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	clients.append(s)

	s.connect((HOST, PORT))
	s.sendall(b"0005hello")
	sleep(0.5)

for c in clients:
	c.close()
	sleep(0.5)