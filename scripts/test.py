import imutils
import os
import img_glob
import matching
import numpy as np
import cv2
import orb
import matplotlib.pyplot as plt


scale_factor = 2
# the grid I used has 50mm distance between neighboring points horizontally and 25mm vertically
matcher = matching.Matching(2**scale_factor, 50, 25)
base_dir = "E:\\recon-image\\11-7\\visual\\4\\scene\\views\\view_0001.mve"
visual_name = os.path.join(base_dir, "undist-L2.png")
thermal_name = os.path.join(base_dir, "thermal.jpg")
dm_name = os.path.join(base_dir, "smvs-B2.mvei")
output_name = "merged-test.jpg"

maxScore = 0
bestScale = 10
scores = []
scales = []

for scale in range(10, 150, 5):
    score = matcher.getScaleScore(visual_name, thermal_name, dm_name, scale)
    if score > maxScore:
        maxScore = score
        bestScale = scale
    scores.append(score)
    scales.append(scale)
    print("scale:", scale, "socre:", score,
          "bestScale", bestScale, "maxScore", maxScore)

merged = matcher.mapThermalToVisual(
    visual_name, thermal_name, dm_name, bestScale)
cv2.imwrite("merged.jpg", merged)

plt.plot(scales, scores, marker='o', mec='r', mfc='w', label=u'scores')
plt.legend()
plt.show()
cv2.waitKey(0)
