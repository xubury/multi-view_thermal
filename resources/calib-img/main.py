import img_glob
import matching
import os
import shutil
import re

test = img_glob.search_files_by_name("thermal-img", "*.jpg")
for name in test:
    print(name)

mve_dir = "E:/normal-img/scene/views"
thermal_dir = "E:/recon_image/11-7/thermal/4"
sclae_factor = 2

mve_entris = img_glob.get_mve_views_entries(mve_dir)

thermal_images_list = img_glob.search_files_by_time(thermal_dir, "*.jpg")

for idx, name in enumerate(thermal_images_list):
    for filename in os.listdir(mve_entris[idx]):
        if re.search("smvs-B[0-9].mvei", filename):
            dst_file_name = os.path.join(mve_entris[idx], "thermal.jpg")
            shutil.copyfile(name, dst_file_name)
            print("Copying " + name + " to " + dst_file_name)
            break


# the grid I used has 50mm distance between neighboring points horizontally and 25mm vertically
matcher = matching.Matching(2**sclae_factor, 50, 25)

for mve_view_dir in mve_entris:
    for filename in os.listdir(mve_view_dir):
        if re.search("smvs-B[0-9].mvei", filename):
            dm_name = os.path.join(mve_view_dir, filename)
            visual_name = os.path.join(mve_view_dir, "undist-L2.png")
            thermal_name = os.path.join(mve_view_dir, "thermal.jpg")
            output_name = os.path.join(mve_view_dir, "merged.jpg")
            # 104 converts the depth value unit to mm(millimeter)
            matcher.match_thermal_to_visual(visual_name, thermal_name, dm_name, output_name, 104)
            break


