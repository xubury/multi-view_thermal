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

def K_to_homogenous_matrix(K):
    K = np.vstack((K, [0, 0, 0]));
    K = np.hstack((K, [[0], [0], [0], [1]]));
    K = np.matrix(K)
    return K

def Rt_to_homogeneous_matrix(R, t, Rodrigues = True):
    if Rodrigues:
        rvec, _= cv2.Rodrigues(R, None)
    else:
        rvec = R
    transform = np.hstack((rvec, t))
    transform = np.vstack((transform, [0, 0, 0, 1]));
    transform = np.matrix(transform)
    return transform


def calculate_W_L_to_R(K_l, Rt_l, K_r, Rt_r):
    W = K_r * Rt_r * Rt_l.I * K_l.I
    return W

# Thermal images
print("Running thermal images calibration...")
t_images, t_obj_pos, t_img_pos, shape = get_points("thermal-img", (3, 9), False)
ret, thermal_K, t_dist, thermal_rvecs, thermal_tvecs = cv2.calibrateCamera(t_obj_pos, t_img_pos, shape, None, None)
undistort_image("thermal-img", t_images, thermal_K, t_dist)
print("Done!")

thermal_transform = Rt_to_homogeneous_matrix(thermal_rvecs[0], thermal_tvecs[0])
print(thermal_transform)

thermal_K = K_to_homogenous_matrix(thermal_K)
print(thermal_K)

# Normal images
print("Running normal images calibration...")
images, obj_pos, img_pos, shape = get_points("normal-img", (3, 9), True)
ret, normal_K, dist, normal_rvecs, normal_tvecs = cv2.calibrateCamera(obj_pos, img_pos, shape, None, None)
undistort_image("normal-img", images, normal_K, dist)
print("Done!")

normal_transform = Rt_to_homogeneous_matrix(normal_rvecs[0], normal_tvecs[0])
print(normal_transform)

normal_K = K_to_homogenous_matrix(normal_K)
print(normal_K)

W = calculate_W_L_to_R(normal_K, normal_transform, thermal_K, thermal_transform);
print(W)


z = np.array(normal_transform)[2][3]
normal = cv2.imread(images[0])
thermal = cv2.imread(t_images[0])
(height, width, c) = normal.shape

normal_image_pos = np.zeros((4, width * height), np.float32)
normal_image_pos[0,:] = np.tile(np.arange(width),height)
normal_image_pos[1,:] = np.repeat(np.arange(height),width)
normal_image_pos[2,:] = 1
normal_image_pos[3,:] = 1/z
thermal_test_img_pos = W * normal_image_pos

thermal_test_img_pos = thermal_test_img_pos / thermal_test_img_pos[2]

x_map = thermal_test_img_pos[0].reshape(height, width).astype(np.float32)
y_map = thermal_test_img_pos[1].reshape(height,width).astype(np.float32)

thermal_mapped = cv2.remap(thermal, x_map, y_map , cv2.INTER_LINEAR)
normal = cv2.addWeighted(normal, 0.4, thermal_mapped, 0.8, 0.2)

u = img_pos[0][0][0]
v = img_pos[0][0][1]
normal_test_img_pos = np.array([u, v, 1, 1 / z]).reshape(4, 1)
thermal_test_img_pos = W * normal_test_img_pos
print("Input:", u, v)
estimate_pos = thermal_test_img_pos / thermal_test_img_pos[2]
x = estimate_pos[0]
y = estimate_pos[1]
print("Estimate:", x, y)
print("Real:", t_img_pos[0][0])
cv2.imwrite("test.jpg", normal)
