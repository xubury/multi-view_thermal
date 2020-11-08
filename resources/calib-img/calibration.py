import numpy as np
import glob
import os
import cv2
import time

def search_all_files_return_by_time_reversed(regex):
    return sorted(glob.glob(regex), key=lambda x: os.path.getmtime(x))

def get_points(path, size, black_dot, verbose = False):
    objp = np.zeros((size[0] * size[1], 3), np.float32)
    for r in range(0, size[1]):
        y = size[1] - r;
        for c in range(0, size[0]):
            x = size[0] - c;
            if r % 2 == 0:
                objp[c + r * size[0]] = [x * 2 + 1, y, 0]
            else:
                objp[c + r * size[0]] = [x * 2, y, 0]


    # Arrays to store object points and image points from all the images.
    objpoints = [] # 3d points in real world space
    imgpoints = [] # 2d points in image plane.

    # Make a list of calibration images
    images = search_all_files_return_by_time_reversed(os.path.join(path, "*.jpg"))
    # Step through the list and search for chessboard corners
    output_path = os.path.join(path, "corners")
    if not os.path.exists(output_path):
        os.mkdir(output_path)
    for idx, fname in enumerate(images):
        if verbose:
            print("read in:", fname)
        img = cv2.imread(fname)
        gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
        shape = gray.shape

        #  findCirclesGrid only detect Dark Color, so revert color here.
        if not black_dot:
            full_white = np.ones(gray.shape, dtype=np.uint8) * 255;
            gray = np.array(full_white - gray);

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
            write_name = 'corners_found'+str(idx)+'.jpg'
            cv2.imwrite(os.path.join(output_path, write_name), img)

        else: 
            print("no circle found.")

    imgpoints = np.squeeze(imgpoints)
    return images, objpoints, imgpoints, shape

def undistort_image(path, images, matrix, dist):
    output_path = os.path.join(path, "undistorted")
    if not os.path.exists(output_path):
        os.mkdir(output_path)

    for idx, fname in enumerate(images):
        img = cv2.imread(fname)
        img = cv2.undistort(img, matrix, dist, None, None)
        write_name = str(idx)+'.jpg'
        cv2.imwrite(os.path.join(output_path, write_name), img)

# Thermal images
t_images, t_obj_pos, t_img_pos, shape = get_points("thermal-img", (3, 9), False)
ret, t_matrix, t_dist, _, _ = cv2.calibrateCamera(t_obj_pos, t_img_pos, shape, None, None)
undistort_image("thermal-img", t_images, t_matrix, t_dist)
ret, t_rvec, t_tvec, _ = cv2.solvePnPRansac(np.float64(t_obj_pos[0]), np.float32(t_img_pos[0]), t_matrix, t_dist, None, None)
t_rvec = cv2.Rodrigues(t_rvec, None)

print(t_rvec[0])
print(t_tvec)

# Normal images
images, obj_pos, img_pos, shape = get_points("normal-img", (3, 9), True)
ret, matrix, dist, _, _ = cv2.calibrateCamera(obj_pos, img_pos, shape, None, None)
undistort_image("normal-img", images, matrix, dist)
ret, rvec, tvec, _ = cv2.solvePnPRansac(np.float64(obj_pos[0]), np.float32(img_pos[0]), matrix, dist, None, None)
rvec = cv2.Rodrigues(rvec, None)

print(rvec[0])
print(tvec)
