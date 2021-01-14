import numpy as np
import cv2
import os
import img_glob

def undistort_image(output_path, images, matrix, dist):
    output_path = os.path.join(output_path, "undistorted")
    if not os.path.exists(output_path):
        os.mkdir(output_path)
    img = cv2.imread(images[0])
    (height, width, c) = img.shape
    (map1, map2) = cv2.initUndistortRectifyMap(matrix, dist, None, None, (width, height), cv2.CV_32FC1)
    for idx, fname in enumerate(images):
        img = cv2.imread(fname)
        img = cv2.remap(img, map1, map2, cv2.INTER_LINEAR)
        write_name = str(idx)+'.jpg'
        cv2.imwrite(os.path.join(output_path, write_name), img)

class ThermalVisualCalibrator():
    thermal_path = ""
    visual_path = ""

    thermal_dist = np.array((1, 5), dtype=np.float32)
    visual_dist = np.array((1, 5), dtype=np.float32)

    thermal_K = np.eye(3)
    visual_K = np.eye(3)
    thermal_K_homo = np.eye(4)
    visual_K_homo = np.eye(4)

    thermal_Rt = []
    visual_Rt = []

    thermal_images_list = []
    visual_images_list= []

    thermal_img_pos = []
    thermal_world_pos = []

    visual_img_pos = []
    visual_world_pos = []

    calibration_pt_size = (3,3)

    real_hstep = 1
    real_vstep = 1
    def __init__(self, thermal_path, visual_path, pt_size, real_hstep, real_vstep):
        self.thermal_path = thermal_path
        self.visual_path = visual_path
        self.calibration_pt_size = pt_size
        self.real_hstep = real_hstep
        self.real_vstep = real_vstep

    def get_points(self, path, size, black_dot, verbose = False):
        objp = np.zeros((size[0] * size[1], 3), np.float32)
        for r in range(0, size[1]):
            y_index = size[1] - r;
            for c in range(0, size[0]):
                x_index = size[0] - c;
                if r % 2 == 0:
                    objp[c + r * size[0]] = [x_index * self.real_hstep + self.real_hstep / 2, y_index * self.real_vstep, 0]
                else:
                    objp[c + r * size[0]] = [x_index * self.real_hstep, y_index * self.real_vstep, 0]


        # Arrays to store object points and image points from all the images.
        objpoints = [] # 3d points in real world space
        imgpoints = [] # 2d points in image plane.

        # Make a list of calibration images
        images = img_glob.search_files_by_name(path, "*.jpg")
        # Step through the list and search for chessboard corners
        output_path = os.path.join(path, "corners")
        if not os.path.exists(output_path):
            os.mkdir(output_path)
        for idx, fname in enumerate(images):
            if verbose:
                print("read in:", fname)
            
            img = cv2.imread(fname)
            os.rename(fname, os.path.join(path, str(idx)+".jpg"))
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

    def StartCalibration(self):
        print("Running thermal images calibration...")
        self.thermal_images_list, self.thermal_world_pos, self.thermal_img_pos, shape = self.get_points(self.thermal_path, self.calibration_pt_size, False)

        ret, self.thermal_K, self.thermal_dist, thermal_rvecs, thermal_tvecs = cv2.calibrateCamera(self.thermal_world_pos, self.thermal_img_pos, shape, None, None)


        self.thermal_K_homo = K_to_homogenous_matrix(self.thermal_K)
        for idx in range(0, len(thermal_rvecs)):
            transform = Rt_to_homogeneous_matrix(thermal_rvecs[idx], thermal_tvecs[idx])
            self.thermal_Rt.append(transform)

        print("Done!")

        print("Running visual images calibration...")
        self.visual_images_list, self.visual_world_pos, self.visual_img_pos, shape = self.get_points(self.visual_path, self.calibration_pt_size, True)

        ret, self.visual_K, self.visual_dist, visual_rvecs, visual_tvecs = cv2.calibrateCamera(self.visual_world_pos, self.visual_img_pos, shape, None, None)


        self.visual_K_homo = K_to_homogenous_matrix(self.visual_K)
        for idx in range(0, len(visual_rvecs)):
            transform = Rt_to_homogeneous_matrix(visual_rvecs[idx], visual_tvecs[idx])
            self.visual_Rt.append(transform)
        print("Done!")


def K_to_homogenous_matrix(K):
    K = np.vstack((K, [0, 0, 0]));
    K = np.hstack((K, [[0], [0], [0], [1]]));
    return np.matrix(K)

def Rt_to_homogeneous_matrix(R, t, Rodrigues = True):
    if Rodrigues:
        rvec, _= cv2.Rodrigues(R, None)
    else:
        rvec = R
    transform = np.hstack((rvec, t))
    transform = np.vstack((transform, [0, 0, 0, 1]));
    return np.matrix(transform)

