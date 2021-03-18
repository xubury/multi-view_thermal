import img_glob
import matching
import os
import re
import numpy as np
import time
import cv2
from sklearn import linear_model, datasets
import matplotlib.pyplot as plt

mve_dir = "E:/recon-image/1-13/2/visual/scene/views"
thermal_dir = "E:/recon-image/1-13/2/thermal"
scale_factor = 2

mve_entris = img_glob.getMVEEntries(mve_dir)

img_glob.copyThermalToDir(thermal_dir, mve_dir)

# measure wall time
t0 = time.time()
# the grid I used has 50mm distance between neighboring points horizontally and 25mm vertically
matcher = matching.Matching(2**scale_factor, 50, 25)

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
                visual_name, thermal_name, dm_name, 90)
            cv2.imwrite(output_name, res)
            break

X = []
Y = []
for mve_view_dir in mve_entris:
    for filename in os.listdir(mve_view_dir):
        if re.search("smvs-visual-B" + str(scale_factor) + ".mvei", filename):
            dm_name = os.path.join(mve_view_dir, filename)
            visual_name = os.path.join(
                mve_view_dir, "undist-L" + str(scale_factor) + ".png")
            thermal_name = os.path.join(mve_view_dir, "thermal.jpg")
            output_name = os.path.join(mve_view_dir, "merged-smvs.jpg")
            sgm_name = os.path.join(mve_view_dir, "smvs-visual-sgm.mvei")
            thermal_dm_name = os.path.join(
                mve_view_dir, "smvs-thermal-sgm.mvei")
            scales = range(10, 300, 5)
            bestScale, bestScore, scores = matcher.guessScale(
                visual_name, thermal_name, dm_name, sgm_name, thermal_dm_name, scales)
            X.append(bestScale)
            Y.append(bestScore)
            # 104 converts the depth value unit to mm(millimeter)
            res = matcher.mapThermalToVisual(
                visual_name, thermal_name, dm_name, 90)
            cv2.imwrite(output_name, res)
            break

print("Matching took: ", time.time() - t0, "seconds.")
X = np.array(X)
Y = np.array(Y)
X = X.reshape(-1, 1)
Y = Y.reshape(-1, 1)
total = np.sum(Y)
best = 0
for i in range(len(X)):
    best += Y[i] / total * X[i]
print("best", best)
plt.scatter(X, Y, alpha=0.4, label='类别A')
plt.show()
