import numpy as np
import cv2
import calibration
import utils

# calculate the transform from L to R
def calculate_W_L_to_R(K_l, Rt_l, K_r, Rt_r):
    W = K_r * Rt_r * Rt_l.I * K_l.I
    return np.matrix(W)

calibrator = calibration.ThermalVisualCalibrator("thermal-img", "normal-img", (3, 9), 25, 50)
calibrator.StartCalibration()
utils.undistort_image("normal-img", calibrator.visual_images_list, calibrator.visual_K, calibrator.visual_dist)
utils.undistort_image("thermal-img", calibrator.thermal_images_list, calibrator.thermal_K, calibrator.thermal_dist)

print("thermal K (homogeneous):")
print(calibrator.thermal_K_homo)
print("visual K (homogeneous):")
print(calibrator.visual_K_homo)

print("thermal Rt[0]:")
print(calibrator.thermal_Rt[0])
print("visual Rt[0]:")
print(calibrator.visual_Rt[0])

W = calculate_W_L_to_R(calibrator.visual_K_homo, calibrator.visual_Rt[0], calibrator.thermal_K_homo, calibrator.thermal_Rt[0]);
print("W martix:")
print(W)

z = np.array(calibrator.visual_Rt[0])[2][3]
thermal = cv2.imread("1.jpg")
visual = cv2.imread("2.jpg")
(height, width, c) = visual.shape


#  z_R * [u_R, v_R, 1, 1/z_R]^T = W * z_L * [u_L, v_L, 1, 1/z_L]^T
visual_image_pos = np.zeros((4, width * height), np.float32)
visual_image_pos[0,:] = np.tile(np.arange(width),height)
visual_image_pos[1,:] = np.repeat(np.arange(height),width)
visual_image_pos[2,:] = 1
visual_image_pos[3,:] = 1/z
visual_pos_in_thermal = W * visual_image_pos

visual_pos_in_thermal = visual_pos_in_thermal / visual_pos_in_thermal[2]

x_map = visual_pos_in_thermal[0].reshape(height, width).astype(np.float32)
y_map = visual_pos_in_thermal[1].reshape(height, width).astype(np.float32)

thermal_mapped = cv2.remap(thermal, x_map, y_map , cv2.INTER_LINEAR)
merged = cv2.addWeighted(visual, 0.4, thermal_mapped, 0.8, 0.2)
cv2.imwrite("merged.jpg", merged)

#  Try using depth map
depth_img = cv2.imread("smvs-B2.jpg", cv2.IMREAD_GRAYSCALE)
dp_height, dp_width = depth_img.shape

depth_img = cv2.resize(depth_img, (width, height))
depth_img = depth_img * 20000
depth_img[depth_img == 0] = 1

#  z_R * [u_R, v_R, 1, 1/z_R]^T = W * z_L * [u_L, v_L, 1, 1/z_L]^T
visual_image_pos = np.zeros((4, width * height), np.float32)
visual_image_pos[0,:] = np.tile(np.arange(width),height)
visual_image_pos[1,:] = np.repeat(np.arange(height),width)
visual_image_pos[2,:] = 1
visual_image_pos[3,:] = 1 / depth_img.reshape(width * height)
visual_pos_in_thermal = W * visual_image_pos

visual_pos_in_thermal = visual_pos_in_thermal / visual_pos_in_thermal[2]

x_map = visual_pos_in_thermal[0].reshape(height, width).astype(np.float32)
y_map = visual_pos_in_thermal[1].reshape(height,width).astype(np.float32)

thermal_mapped = cv2.remap(thermal, x_map, y_map , cv2.INTER_LINEAR)
merged = cv2.addWeighted(visual, 0.4, thermal_mapped, 0.8, 0.2)
cv2.imwrite("merged-with-dm.jpg", merged)
