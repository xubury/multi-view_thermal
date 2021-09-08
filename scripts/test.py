import os
import matching
import cv2
import matplotlib.pyplot as plt
import utils
import img_glob
import ransac

scale_factor = 2
# the width and height of the visible image that used to reconstruct
width = 613
height = 513

# the grid I used has 50mm distance between neighboring points horizontally and 25mm vertically
matcher = matching.Matching(width, height, 50, 25)

base_dir = "E:/recon-image/2021-06-17/1/visible/scene/views/view_0000.mve"
# base_dir = "E:/recon-image/1-13/2/visual/scene/views/view_0000.mve"  # real value: 90
real = 95

#  base_dir = "/mnt/e/recon-image/11-7/3/visual/scene/views/view_0002.mve"  # real value: 120
#  real = 120

#  base_dir = "E:\\recon-image\\11-7\\4\\visual\\scene\\views\\view_0000.mve"  # real value: 130
#  real = 130

visual_name = os.path.join(base_dir, "undist-L2.png")
thermal_name = os.path.join(base_dir, "thermal.jpg")
dm_name = os.path.join(base_dir, "smvs-visual-B2.mvei")
thermal_dm_name = os.path.join(base_dir, "smvs-thermal-SGM.mvei")
output_name = "merged-test.tif"

depthMap = utils.readMVEI(thermal_dm_name)
cv2.normalize(depthMap, dst=depthMap, alpha=20, beta=255, norm_type=cv2.NORM_MINMAX)
cv2.imwrite("thermal-depth-map.tif", depthMap)
depthMap = utils.readMVEI(dm_name)
cv2.normalize(depthMap, dst=depthMap, alpha=0, beta=255, norm_type=cv2.NORM_MINMAX)
cv2.imwrite("visual-depth-map.tif", depthMap)

bestScale, bestScore = matcher.guessScale(
    visual_name, thermal_name, dm_name, thermal_dm_name)
print("best Scale:", bestScale)
merged = matcher.mapThermalToVisual(
    visual_name, thermal_name, dm_name, bestScale)
cv2.imwrite(output_name, merged)

plt.xlabel("scale")
plt.ylabel("score")
plt.axvline(x=real, color='g', linestyle='-', label="ground truth")
plt.axvline(x=bestScale, color='r', linestyle='-', label='estimation')
plt.legend()
plt.grid()
plt.savefig("scale-estimation.pdf", dpi=300, format='pdf')
#  plt.show()
