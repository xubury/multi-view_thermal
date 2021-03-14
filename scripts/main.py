import img_glob
import matching
import os
import shutil
import re
import numpy as np
import time
import cv2

mve_dir = "E:/recon-image/11-7/visual/4/scene/views"
thermal_dir = "E:/recon-image/11-7/thermal/4"
scale_factor = 2

mve_entris = img_glob.getMVEEntries(mve_dir)

thermal_images_list = img_glob.globFilesByTime(thermal_dir, "*.jpg")

for idx, name in enumerate(thermal_images_list):
    for filename in os.listdir(mve_entris[idx]):
        if re.search("undist-L" + str(scale_factor) + ".png", filename):
            dst_file_name = os.path.join(mve_entris[idx], "thermal.jpg")
            shutil.copyfile(name, dst_file_name)
            print("Copying " + name + " to " + dst_file_name)
            break


# measure wall time
t0 = time.time()
# the grid I used has 50mm distance between neighboring points horizontally and 25mm vertically
matcher = matching.Matching(2**scale_factor, 50, 25)

for mve_view_dir in mve_entris:
    for filename in os.listdir(mve_view_dir):
        if re.search("depth-L" + str(scale_factor) + ".mvei", filename):
            dm_name = os.path.join(mve_view_dir, filename)
            visual_name = os.path.join(
                mve_view_dir, "undist-L" + str(scale_factor) + ".png")
            thermal_name = os.path.join(mve_view_dir, "thermal.jpg")
            output_name = os.path.join(mve_view_dir, "merged-mvs.jpg")
            # 104 converts the depth value unit to mm(millimeter)
            res = matcher.mapThermalToVisual(
                visual_name, thermal_name, dm_name, 1274)
            cv2.imwrite(output_name, res)
            break

for mve_view_dir in mve_entris:
    for filename in os.listdir(mve_view_dir):
        if re.search("smvs-B" + str(scale_factor) + ".mvei", filename):
            dm_name = os.path.join(mve_view_dir, filename)
            visual_name = os.path.join(
                mve_view_dir, "undist-L" + str(scale_factor) + ".png")
            thermal_name = os.path.join(mve_view_dir, "thermal.jpg")
            output_name = os.path.join(mve_view_dir, "merged-smvs.jpg")
            # 104 converts the depth value unit to mm(millimeter)
            res = matcher.mapThermalToVisual(
                visual_name, thermal_name, dm_name, 1000)
            cv2.imwrite(output_name, res)
            break

print("Matching took: ", time.time() - t0, "seconds.")
