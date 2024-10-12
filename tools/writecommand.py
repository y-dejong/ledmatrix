import socket
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
    sock.connect(("192.168.0.146", 2314))
    sock.sendall(b'clock\r')
