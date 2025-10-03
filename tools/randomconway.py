import socket
from random import randbytes

WIDTH = 64
HEIGHT = 64

def main():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect(("192.168.1.30", 2314))

        sock.sendall(b"msg conway0 setstate\r")
        try:
            while True:
                response = sock.recv(4096)
                if not response:
                    return
                if "Ready" in response.decode(errors="replace"):
                    break
                else:
                    print("received non Ready data")
        except Exception as e:
            print(f"Failed to receive response: {e}")
            return

        sock.sendall(randbytes(WIDTH * HEIGHT // 8))

if __name__ == "__main__":
    main()
