import utils
import cv2
import numpy as np
import sys
import matplotlib.pyplot as plt
import img_glob
import os
import re

def normalizeVector(v):
    length = np.sqrt(v[0]**2 + v[1]**2 + v[2]**2)
    v = v / length
    return v

def depthToNormal(dm):
    height, width = dm.shape[:2]
    normal = np.zeros((height, width, 3), dtype=np.float32)

    for y in range(1, height-1):
        for x in range(1, width-1):

            dzdx = (float(dm[y, x + 1]) - float(dm[y, x - 1])) / 2.0
            dzdy = (float(dm[y + 1, x]) - float(dm[y - 1, x])) / 2.0

            d = (-dzdx, -dzdy, 1.0)

            n = normalizeVector(d)

            normal[y, x] = n * 0.5 + 0.5
    return normal

if __name__ == "__main__":
    mve_dir = "/mnt/e/recon-image/11-7/3/visual/scene/views"
    mve_entris = img_glob.getMVEEntries(mve_dir)
    for mve_view_dir in mve_entris:
        for filename in os.listdir(mve_view_dir):
            if re.search("smvs-visual-B" + str(2) + ".mvei", filename):
                dm_name = os.path.join(mve_view_dir, filename)
                thermal_dm = utils.readMVEI(os.path.join(mve_view_dir, "smvs-thermal-SGM.mvei"))
                visual_dm = utils.readMVEI(os.path.join(mve_view_dir, "smvs-visual-SGM.mvei"))

                visual_dm = cv2.resize(visual_dm, thermal_dm.shape[:2])
                print(cal_psnr(visual_dm, thermal_dm))
                break
    #  normal = depthToNormal(thermal_dm)
    #  normal *= 255
    #  normal = normal.astype('uint8')
    #  plt.imshow(normal)
    #  plt.show()

