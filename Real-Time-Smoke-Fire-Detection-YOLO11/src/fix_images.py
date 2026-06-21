import os
from PIL import Image

# Get the absolute path of the 'src' directory where this script runs
script_dir = os.path.dirname(os.path.abspath(__file__))

# Point directly to the 'known_faces' subdirectory
faces_dir = os.path.join(script_dir, "known_faces")

image_files = ['Ahmed.jpeg', 'Marawan.jpeg', 'Nesrin.jpeg', 'Rawna.jpeg']

print(f"Target Directory: {faces_dir}\n")

for filename in image_files:
    filepath = os.path.join(faces_dir, filename)
    if os.path.exists(filepath):
        try:
            with Image.open(filepath) as img:
                # Force convert image format to standard 8-bit RGB and drop alpha channels
                rgb_img = img.convert('RGB')
                # Overwrite with clean compliant JPEG matrix configuration
                rgb_img.save(filepath, 'JPEG', quality=95)
                print(f"✓ Successfully fixed image properties for: {filename}")
        except Exception as e:
            print(f"✗ Failed to fix {filename}: {e}")
    else:
        print(f"⚠ File not found: {filepath}")