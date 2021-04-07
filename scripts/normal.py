import utils
import cv2
import numpy as np
import sys
import matplotlib.pyplot as plt

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
    dm = utils.readMVEI("/mnt/e/recon-image/11-7/3/visual/scene/views/view_0032.mve/smvs-visual-SGM.mvei")
    normal = depthToNormal(dm)
    #  normal *= 255
    #  normal = normal.astype('uint8')
    #  plt.imshow(normal)
    #  plt.show()

