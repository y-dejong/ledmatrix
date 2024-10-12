import sys

def as_c_string(arr):
    return "{" + ", ".join(arr) + "}"

def hex_to_rgb(hex_color):
    hex_color = hex_color.lstrip('#')
    return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))

def rgb_to_hex(rgb_color):
    return '0x{:02x}{:02x}{:02x}'.format(rgb_color[0], rgb_color[1], rgb_color[2])

def interpolate(start, end, step, total_steps):
    return int(start + (end - start) * (step / total_steps))

def gen_gradient(hex_color1, hex_color2, num_steps=64):
    color1 = hex_to_rgb(hex_color1)
    color2 = hex_to_rgb(hex_color2)
    gradient = []

    for step in range(num_steps):
        r = interpolate(color1[0], color2[0], step, num_steps - 1)
        g = interpolate(color1[1], color2[1], step, num_steps - 1)
        b = interpolate(color1[2], color2[2], step, num_steps - 1)
        gradient.append(rgb_to_hex((r, g, b)))

    return gradient

def main():
    if sys.argv[1] == "gengradient":
        if len(sys.argv) >= 5:
            print(as_c_string(gen_gradient(sys.argv[2], sys.argv[3], int(sys.argv[4]))))
        else:
            print(as_c_string(gen_gradient(sys.argv[2], sys.argv[3])))

if __name__ == "__main__":
    main()
