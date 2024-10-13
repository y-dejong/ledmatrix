import sys
import socket
from PIL import Image
import os
import urllib.request

def image_or_url_to_path(path):
    if os.path.isfile(path):
        return path
    elif path.startswith('http://') or path.startswith('https://'):
        try:
            temp_file, _ = urllib.request.urlretrieve(path)
            return temp_file
        except Exception as e:
            print(f"Error downloading image from URL: {e}")
            return None
    else:
        print("Invalid")
        return None

def crop_and_resize(image, target_size):
    original_aspect = image.size[0] / image.size[1]
    target_aspect = target_size[0] / target_size[1]

    # Determine the size of the crop
    if original_aspect > target_aspect:
        # Image is wider than the target aspect ratio, crop the width
        new_width = int(image.size[1] * target_aspect)
        new_height = image.size[1]
        left = (image.size[0] - new_width) // 2
        top = 0
        right = left + new_width
        bottom = image.size[1]
    else:
        # Image is taller than the target aspect ratio, crop the height
        new_width = image.size[0]
        new_height = int(image.size[0] / target_aspect)
        left = 0
        top = (image.size[1] - new_height) // 2
        right = image.size[0]
        bottom = top + new_height

    return image.crop((left, top, right, bottom)).resize(target_size, Image.LANCZOS)

def image_to_rgb888_array(img, size=(64,64)):
    try:
        img = img.convert('RGB')
        img = crop_and_resize(img, size)

        pixels = list(img.getdata())

        # Create the RGB888 array
        return [(pixel[2] << 16) | (pixel[1] << 8) | pixel[0] for pixel in pixels]

    except Exception as e:
        print(f"Error processing the image: {e}")
        sys.exit(1)

def image_to_rgb565_array(img, size=(64, 64)):
    img = img.convert('RGB')
    img = img.crop(size)

    pixels = list(img.getdata())

    return [((pixel[2] >> 3) << 11) | ((pixel[1] >> 2) << 5) | (pixel[0] >> 3) for pixel in pixels]

def animation_to_rgb565_arrays(img, size=(64, 64)):
    frames = []

    try:
        img.seek(1)
        while True:
            frames.append(image_to_rgb565_array(img, size))
            img.seek(img.tell() + 1)
    except EOFError:
        pass
    return frames

def export_rgb888(rgb888_array, name, size):

    with open(f"img_{name}.h", 'w') as f:
        # Write the array size and declaration in C syntax
        f.write(f"// Image dimensions: {size[0]}x{size[1]}\n")
        f.write(f"uint32_t img_{name}[{size[0] * size[1]}] = {{\n")

        # Write each pixel value
        for i, value in enumerate(rgb888_array):
            # Add commas and format for easier readability
            if i % size[0] == 0:
                f.write("    ")  # Indent for new rows
            f.write(f"0x{value:06X}, ")  # Hexadecimal with leading 0x

            # Line break at the end of each row
            if (i + 1) % size[0] == 0:
                f.write("\n")

        f.write("};\n")
        print(f"C array successfully written to img_{name}.h")


def export_animation(frames, name, size):
    with open(f"img_{name}.h", 'w') as f:
        # Write the array size and declaration in C syntax
        f.write(f"// Image dimensions: {size[0]}x{size[1]}\n")
        f.write(f"const uint16_t img_{name}[{len(frames)}][{size[0] * size[1]}] = {{\n")

        for frame in frames:
            f.write("    {\n")

            # Write each pixel value
            for i, value in enumerate(frame):
                # Add commas and format for easier readability
                if i % size[0] == 0:
                    f.write("        ")  # Indent for new rows

                f.write(f"0x{value:04X}, ")  # Hexadecimal with leading 0x

                # Line break at the end of each row
                if (i + 1) % size[0] == 0:
                    f.write("\n")

            f.write("    },\n")

        f.write("};\n")
        print(f"C array successfully written to img_{name}.h")

def write_rgb888_to_network(rgb888_array, width, height, output_ip, output_port, gamma_correct=True):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((output_ip, output_port))

        sock.sendall(b'fullimg\r')

        total = 0
        for item in rgb888_array:
            if (gamma_correct):
                gamma = 2.2
                gamma_corrected_pixel = (int(255 * (((item >> 16) & 0xFF) / 255) ** gamma) << 16) | \
                        (int(255 * (((item >> 8) & 0xFF) / 255) ** gamma) << 8)  | \
                        int(255 * ((item & 0xFF) / 255) ** gamma)

            sock.sendall(gamma_corrected_pixel.to_bytes(4, 'little'))

        print("Sent full image")

        sock.settimeout(1)
        while True:
            try:
                incoming_data = sock.recv(1024)
                if not incoming_data or ("Finished." in incoming_data.decode('utf-8', errors='ignore')):
                    break
                print(f"Received: {incoming_data.decode('utf-8', errors='ignore')}")
            except TimeoutError:
                pass

def send_net():
    if len(sys.argv) != 2:
        print("Usage: python script.py <image_file>")
        sys.exit(1)

    image_path = sys.argv[1]

    # Convert image to RGB888 array
    rgb888_array, width, height = image_or_url_to_rgb888_array(image_path)

    # Write the C syntax array to the output file
    #write_rgb888_to_header_file(rgb888_array, width, height, output_file)

    # Write to network
    write_rgb888_to_network(rgb888_array, width, height, "192.168.0.146", 2314)

def gamma_correct(pixel, gamma):
    gamma_corrected_pixel = (int(255 * (((pixel >> 16) & 0xFF) / 255) ** gamma) << 16) | \
        (int(255 * (((pixel >> 8) & 0xFF) / 255) ** gamma) << 8)  | \
        int(255 * ((pixel & 0xFF) / 255) ** gamma)
    return gamma_corrected_pixel

def main():
    if len(sys.argv) < 2:
        print("Usage: python imgtools.py <command> <file>")
        return

    if sys.argv[1] == "export":
        img_path = image_or_url_to_path(sys.argv[2])

        img = Image.open(img_path)

        if img.is_animated:
            animation = animation_to_rgb565_arrays(img)
            export_animation(animation, os.path.splitext(os.path.basename(img.filename))[0], (64, 64))
        else:
            export_rgb888(image_to_rgb888_array(img), os.path.splitext(os.path.basename(img.filename))[0], (64, 64))
    elif sys.argv[1] == "draw":
        img_path = image_or_url_to_path(sys.argv[2])

        write_rgb888_to_network(image_to_rgb888_array(Image.open(img_path)), 64, 64, "192.168.0.146", 2314)

main()
