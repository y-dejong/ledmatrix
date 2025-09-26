import re
import sys
import textwrap
import socket

# === CONFIG ===
WIDTH = 64   # set your grid width
HEIGHT = 64  # set your grid height

def parse_rle(rle_str):
    """Parse an RLE string into a 2D list of 0/1 values (dead/alive)."""
    lines_raw = [l for l in rle_str.splitlines() if not l.startswith('#')]

    matches = re.search(r"x\s*=\s*(\d+),\s*y\s*=\s*(\d+)", next((l for l in lines_raw if l.startswith("x"))))
    pattern_width = int(matches.group(1))
    pattern_height = int(matches.group(2))

    x_offset = (WIDTH - pattern_width) // 2
    y_offset = (HEIGHT - pattern_height) // 2
    
    lines = [l for l in lines_raw if not l.startswith('x')]
    
    pattern = ''.join(lines).replace('\n', '').strip('!')

    grid = [[0] * WIDTH for _ in range(HEIGHT)]

    x = x_offset
    y = y_offset
    tokens = re.findall(r'(\d*)([ob$])', pattern)
    
    for count_str, symbol in tokens:
        count = int(count_str) if count_str else 1
        if symbol == 'b':  # dead cells
            x += count
        elif symbol == 'o':  # live cells
            for _ in range(count):
                if 0 <= x < WIDTH and 0 <= y < HEIGHT:
                    grid[y][x] = 1
                x += 1
        elif symbol == '$':  # end of line(s)
            y += count
            x = x_offset
    return grid

def grid_to_bytes(grid):
    """Convert grid of 0/1 into little-endian bit-packed byte array."""
    bits = []
    for row in grid:
        bits.extend(row)
    # Pad to nearest byte
    while len(bits) % 8 != 0:
        bits.append(0)
    # Pack bits (little-endian per byte)
    data = []
    for i in range(0, len(bits), 8):
        byte = 0
        for j in range(8):
            if bits[i + j]:
                byte |= (1 << j)
        data.append(byte)
    return data

def format_c_array(data, var_name="board"):
    """Format as C uint8_t array."""
    hex_bytes = [f"0x{b:02x}" for b in data]
    wrapped = textwrap.fill(", ".join(hex_bytes), width=80)
    return f"const uint8_t {var_name}[] = {{\n{wrapped}\n}};"

def set_conway_state(data):
    """Send to running conway app"""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((sys.argv[1], 2314)) # Hardcoded port number

        sock.sendall(b"msg conway0 setstate\r")
        try:
            while True:
                response = sock.recv(4096)
                if not data:
                    return
                if "Ready" in response.decode(errors="replace"):
                    break
                else:
                    print("received non Ready data")
        except Exception as e:
            print(f"Failed to receive response: {e}")
            return

        sock.sendall(bytes(data))
        print("Sent " + str(len(data)) + " bytes")

        

if __name__ == "__main__":
    # Example usage: pass RLE string via stdin or file
    rle_input = sys.stdin.read()
    grid = parse_rle(rle_input)
    data = grid_to_bytes(grid)

    if len(sys.argv) >= 2:
        set_conway_state(data)
    else:
        print(format_c_array(data))
