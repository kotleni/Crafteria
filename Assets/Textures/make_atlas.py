import os
import math
from PIL import Image

def merge_images_to_atlas(folder, output_path, max_columns=None):
    images = [Image.open(os.path.join(folder, f)) for f in os.listdir(folder) if f.endswith(('png')) and not f.endswith("_atlas.png")]
    images_names = [os.path.join(folder, f) for f in os.listdir(folder) if f.endswith(('png')) and not f.endswith("_atlas.png")]

    if not images:
        print("No images found in the folder.")
        return

    max_width = max(img.width for img in images)
    max_height = max(img.height for img in images)

    num_images = len(images)
    cols = max_columns or math.ceil(math.sqrt(num_images))
    rows = math.ceil(num_images / cols)

    atlas_width = cols * max_width
    atlas_height = rows * max_height

    atlas = Image.new('RGBA', (atlas_width, atlas_height), (0, 0, 0, 0))

    x_offset = 0
    y_offset = 0

    for idx, img in enumerate(images):
        name = images_names[idx]
        atlas.paste(img, (x_offset, y_offset))

        print(f"{name}: ({x_offset}, {y_offset})")

        x_offset += max_width
        if (idx + 1) % cols == 0:
            x_offset = 0
            y_offset += max_height

    atlas.save(output_path)
    print(f"Atlas saved as {output_path}")

folder_path = "./"
output_file = "blocks_atlas.png"
merge_images_to_atlas(folder_path, output_file)
