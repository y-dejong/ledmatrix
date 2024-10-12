import socket
import sys

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
    sock.connect(("192.168.0.146", 2314))
    sock.sendall(bytes(f'{sys.argv[1]}\r', 'ascii'))
    print(sock.recv(1024))
