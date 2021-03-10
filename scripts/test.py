import os
import img_glob
import matching
import numpy as np

scale_factor = 2
# the grid I used has 50mm distance between neighboring points horizontally and 25mm vertically
matcher = matching.Matching(2**scale_factor, 50, 25)

if os.path.exists("calibration.txt"):
    print("calibration file loaded")
    matcher.W = np.loadtxt("calibration.txt")
else:
    matcher.calibrate()
    np.savetxt("calibration.txt", matcher.W)
