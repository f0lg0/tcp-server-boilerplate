import socket
from time import sleep

HOST = "0.0.0.0"
PORT = 1234

def send_msg(s, msg):
	final = f"{len(msg):04d}{msg}"
	s.sendall(bytes(final, encoding="UTF-8"))


def main():
	clients = []

	for i in range(0, 10):
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		clients.append(s)

		s.connect((HOST, PORT))
		send_msg(s, f"hello #{i}")

	for c in clients:
		c.close()

if __name__ == "__main__":
	main()