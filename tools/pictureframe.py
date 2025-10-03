from PIL import Image
import socket
import time
import sys
import threading

send_ready = False

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

def image_to_rgb555_array(img, size=(64,64)):
    try:
        img = img.convert('RGB')
        img = crop_and_resize(img, size)

        pixels = list(img.getdata())

        # Create the RGB555 array
        return [((pixel[2] >> 3) << 10) | ((pixel[1] >> 3) << 5) | (pixel[0] >> 3) | (1 << 15) for pixel in pixels]

    except Exception as e:
        print(f"Error processing the image: {e}")
        raise e

def socket_receiver(sock):
    """Background thread that prints anything received from sock."""
    try:
        while True:
            data = sock.recv(4096)
            if not data:
                break
            if data.decode(errors="replace").startswith("Ready"):
                global send_ready
                send_ready = True
            print("RECEIVED:", data.decode(errors="replace"))
    except Exception as e:
        print(f"Receiver stopped: {e}")

def send_image(img_path, coords, size, addr, port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((addr, port))

        # Start async receiver thread
        threading.Thread(target=socket_receiver, args=(sock,), daemon=True).start()

        time.sleep(0.5)

        sock.sendall(bytes(f"msg pictureframe0 add {coords[0]} {coords[1]} {size[0]} {size[1]}\r", "utf8"))
        # sock.sendall(bytes(f"msg pictureframe0 set 0\r", "utf8"))
        img_arr = image_to_rgb555_array(Image.open(img_path), size)

        global send_ready
        while not send_ready:
            time.sleep(0.01)
        all_data = bytes()

        for item in img_arr:
            # gamma = 2.2
            # gamma_corrected_pixel = (int(255 * (((item >> 16) & 0xFF) / 255) ** gamma) << 16) | \
            #     (int(255 * (((item >> 8) & 0xFF) / 255) ** gamma) << 8)  | \
            #     int(255 * ((item & 0xFF) / 255) ** gamma)
            all_data += item.to_bytes(2, 'little')

        # print(all_data)
        # print(len(all_data))
        sock.sendall(all_data)

        print("sent", len(all_data))

        # Keep alive briefly so background thread can receive
        time.sleep(2)

def main():
    if len(sys.argv) < 2:
        print("Usage: python pictureframe.py <file> <x> <y> <width> <height>")
        return

    send_image(sys.argv[1], (int(sys.argv[2]), int(sys.argv[3])), (int(sys.argv[4]), int(sys.argv[5])), "192.168.1.30", 2314)

if __name__ == "__main__":
    main()
