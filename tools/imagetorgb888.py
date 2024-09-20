import sys
from PIL import Image

def image_to_rgb888_array(image_path, size=(64,64)):
    try:
        # Open the image
        img = Image.open(image_path)

        img = img.convert('RGB')
        img = img.resize(size)

        # Get image dimensions
        width, height = img.size
        pixels = list(img.getdata())

        # Create the RGB888 array
        rgb888_array = []
        for pixel in pixels:
            r, g, b = pixel
            rgb888 = (b << 16) | (g << 8) | r  # Pack RGB into 24-bit value
            rgb888_array.append(rgb888)

        return rgb888_array, width, height

    except Exception as e:
        print(f"Error processing the image: {e}")
        sys.exit(1)

def write_c_array(rgb888_array, width, height, output_file):
    try:
        with open(output_file, 'w') as f:
            # Write the array size and declaration in C syntax
            f.write(f"// Image dimensions: {width}x{height}\n")
            f.write(f"uint32_t image_rgb888[{width * height}] = {{\n")
            
            # Write each pixel value
            for i, value in enumerate(rgb888_array):
                # Add commas and format for easier readability
                if i % width == 0:
                    f.write("    ")  # Indent for new rows
                f.write(f"0x{value:06X}, ")  # Hexadecimal with leading 0x
                
                # Line break at the end of each row
                if (i + 1) % width == 0:
                    f.write("\n")
            
            f.write("};\n")
            print(f"C array successfully written to {output_file}")

    except Exception as e:
        print(f"Error writing to the file: {e}")
        sys.exit(1)

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python script.py <image_file> <output_file>")
        sys.exit(1)
    
    image_path = sys.argv[1]
    output_file = sys.argv[2]

    # Convert image to RGB888 array
    rgb888_array, width, height = image_to_rgb888_array(image_path)

    # Write the C syntax array to the output file
    write_c_array(rgb888_array, width, height, output_file)

