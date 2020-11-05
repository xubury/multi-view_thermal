import cv2
import numpy as np
import glob

def get_points(size):
    # prepare object points, like (0,0,0), (1,0,0), (2,0,0) ....,(6,5,0)
    objp = np.zeros((size[0] * size[1], 3), np.float32)
    for r in range(0, size[1]):
        for c in range(0, size[0]):
            if r % 2 == 0:
                objp[c + r * size[0]] = [c * 2 + 1, r, 0]
            else:
                objp[c + r * size[0]] = [c * 2, r, 0]


    # Arrays to store object points and image points from all the images.
    objpoints = [] # 3d points in real world space
    imgpoints = [] # 2d points in image plane.

    # Make a list of calibration images
    images = glob.glob('*.jpg')
    # Step through the list and search for chessboard corners
    for idx, fname in enumerate(images):
        print("read in:", fname)
        img = cv2.imread(fname)
        gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

        params = cv2.SimpleBlobDetector_Params()
        params.maxArea = 10e4
        params.minArea = 10
        params.minDistBetweenBlobs = 5
        blobDetector = cv2.SimpleBlobDetector_create(params)
        # Find the circle grid centers
        ret, corners = cv2.findCirclesGrid(gray, size, cv2.CALIB_CB_ASYMMETRIC_GRID, blobDetector, None)

        # If found, add object points, image points
        if ret == True:
            objpoints.append(objp)
            imgpoints.append(corners)

            # Draw and display the corners
            cv2.drawChessboardCorners(img, size, corners, ret)
            #write_name = 'corners_found'+str(idx)+'.jpg'
            #cv2.imwrite(write_name, img)
            cv2.imshow('img' + str(idx), img)
            cv2.waitKey(0)
        else:
            print("no circle found.")


    return objpoints, imgpoints


obj, img = get_points((3, 9))
print(img)
