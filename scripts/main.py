import img_glob
import matching
import os
import re
import numpy as np
import time
import cv2
import matplotlib.pyplot as plt
import ransac

#  mve_dir = "/mnt/e/recon-image/11-7/3/visual/scene/views"
#  thermal_dir = "/mnt/e/recon-image/11-7/3/thermal"
#  real = 120

#  mve_dir = "/mnt/e/recon-image/11-7/4/visual/scene/views"
#  thermal_dir = "/mnt/e/recon-image/11-7/4/thermal"
#  real = 130

mve_dir = "/mnt/e/recon-image/1-13/2/visual/scene/views"
thermal_dir = "/mnt/e/recon-image/1-13/2/thermal"
real = 90

scale_factor = 2

mve_entris = img_glob.getMVEEntries(mve_dir)
# measure wall time
t0 = time.time()
# the grid I used has 50mm distance between neighboring points horizontally and 25mm vertically
matcher = matching.Matching(2**scale_factor, 50, 25)

# estimate best scale
scales = []
scores = []
for mve_view_dir in mve_entris:
    for filename in os.listdir(mve_view_dir):
        if re.search("smvs-visual-B" + str(scale_factor) + ".mvei", filename):
            dm_name = os.path.join(mve_view_dir, filename)
            visual_name = os.path.join(
                mve_view_dir, "undist-L" + str(scale_factor) + ".png")
            thermal_name = os.path.join(mve_view_dir, "thermal.jpg")
            output_name = os.path.join(mve_view_dir, "merged-smvs.jpg")
            thermal_dm_name = os.path.join(
                mve_view_dir, "smvs-thermal-SGM.mvei")
            ran = range(10, 300, 5)
            bestScale, bestScore, res = matcher.guessScale(
                visual_name, thermal_name, dm_name, thermal_dm_name, ran)
            scales.append(bestScale)
            scores.append(bestScore)
            break

scales = [item for sublist in scales for item in sublist]
scores = [item for sublist in scores for item in sublist]
# calculate average
scales = np.array(scales)
scores = np.array(scores)
scales = scales.reshape(-1, 1)
scores = scores.reshape(-1, 1)
total = np.sum(scores)
avgScale = 0
for i in range(len(scales)):
    avgScale += scores[i] / total * scales[i]
print("Scale estimation took:", time.time() - t0, "seconds.")
print("average scale:", avgScale)

ransac = ransac.Ransac(100, 20, 0.8, 10, True)
_, ransacScale = ransac.fit(scales, scores)
t0 = time.time()
# map the image using the scale
for mve_view_dir in mve_entris:
    for filename in os.listdir(mve_view_dir):
        if re.search("depth-visual-L" + str(scale_factor) + ".mvei", filename):
            dm_name = os.path.join(mve_view_dir, filename)
            visual_name = os.path.join(
                mve_view_dir, "undist-L" + str(scale_factor) + ".png")
            thermal_name = os.path.join(mve_view_dir, "thermal.jpg")
            output_name = os.path.join(mve_view_dir, "merged-mvs.jpg")
            # 104 converts the depth value unit to mm(millimeter)
            res = matcher.mapThermalToVisual(
                visual_name, thermal_name, dm_name, ransacScale)
            cv2.imwrite(output_name, res)
            break

for mve_view_dir in mve_entris:
    for filename in os.listdir(mve_view_dir):
        if re.search("smvs-visual-B" + str(scale_factor) + ".mvei", filename):
            dm_name = os.path.join(mve_view_dir, filename)
            visual_name = os.path.join(
                mve_view_dir, "undist-L" + str(scale_factor) + ".png")
            thermal_name = os.path.join(mve_view_dir, "thermal.jpg")
            output_name = os.path.join(mve_view_dir, "merged-smvs.jpg")
            # 104 converts the depth value unit to mm(millimeter)
            res = matcher.mapThermalToVisual(
                visual_name, thermal_name, dm_name, ransacScale)
            cv2.imwrite(output_name, res)
            break

print("Matching took:", time.time() - t0, "seconds.")
plt.figure()
plt.axvline(x=real, color='g', linestyle='-', label="Ground Truth")
plt.axvline(x=avgScale, color='r', linestyle='-', label="Average")
plt.axvline(x=ransacScale, color='b', linestyle='-', label="RANSAC")
plt.scatter(scales, scores, alpha=0.4)
plt.xlabel("Scale")
plt.ylabel("Score")
plt.savefig("output/final.jpg", dpi=300)
