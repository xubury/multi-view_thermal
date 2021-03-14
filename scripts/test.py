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
base_dir = "E:\\recon-image\\1-13\\2\\visual\\scene\\views\\view_0001.mve"
visual_name = os.path.join(base_dir, "undist-L2.png")
thermal_name = os.path.join(base_dir, "thermal.jpg")
dm_name = os.path.join(base_dir, "smvs-cut.mvei")
output_name = "merged-test.jpg"


scales = range(10, 250, 1)
bestScale, maxScore, scores = matcher.guessScale(
    visual_name, thermal_name, dm_name, scales)

merged = matcher.mapThermalToVisual(
    visual_name, thermal_name, dm_name, bestScale)
cv2.imwrite("merged.jpg", merged)

plt.plot(scales, scores, marker='o', mec='r', mfc='w', label=u'scores')
plt.legend()
plt.show()
cv2.waitKey(0)
