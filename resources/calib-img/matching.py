import numpy as np
import cv2
import calibration
import img_glob

# calculate the transform from L to R
def calculate_W_L_to_R(K_l, Rt_l, K_r, Rt_r):
    W = K_r * Rt_r * Rt_l.I * K_l.I
    return np.matrix(W)


class Matching():
    calibrator = 0
    W =  np.eye(4)

    def __init__(self, scale_factor, h_step, v_step):
        self.calibrator = calibration.ThermalVisualCalibrator("thermal-img", "normal-img", (3, 9), h_step, v_step)
        
        self.calibrator.StartCalibration()
        calibration.undistort_image("normal-img", self.calibrator.visual_images_list, self.calibrator.visual_K, self.calibrator.visual_dist)
        calibration.undistort_image("thermal-img", self.calibrator.thermal_images_list, self.calibrator.thermal_K, self.calibrator.thermal_dist)


        #  Note: In the 3d reconstruction, images are usually scaled
        #  Therefore, K matrix needs to be scaled too if scaled images are used 
        print("thermal K (homogeneous):")
        print(self.calibrator.thermal_K_homo)
        print("visual K (homogeneous):")
        K_homo = self.calibrator.visual_K_homo.copy()
        K_homo[0] /= scale_factor
        K_homo[1] /= scale_factor
        print(K_homo)

        pose_id = 0
        print("thermal Rt[0]:")
        print(self.calibrator.thermal_Rt[0])
        print("visual Rt[0]:")
        print(self.calibrator.visual_Rt[0])

        self.W = calculate_W_L_to_R(K_homo, self.calibrator.visual_Rt[pose_id], self.calibrator.thermal_K_homo, self.calibrator.thermal_Rt[pose_id]);
        print("W martix:")
        print(self.W)

    def match_thermal_to_visual(self, visual_name, thermal_name, dm_name, output_name, depth_scale):

        visual = cv2.imread(visual_name)
        thermal = cv2.imread(thermal_name)
        (height, width, c) = visual.shape

        #  z_R * [u_R, v_R, 1, 1/z_R]^T = W * z_L * [u_L, v_L, 1, 1/z_L]^T
        #  Try using depth map
        depth_img, dp_width, dp_height = img_glob.read_mvei_file(dm_name)

        depth_img = np.array(depth_img)
        depth_img = depth_img.reshape(dp_height, dp_width)
        depth_img *= depth_scale
        depth_img[depth_img == 0] = 1

        visual_image_pos = np.zeros((4, width * height), np.float32)
        visual_image_pos[0,:] = np.tile(np.arange(width),height)
        visual_image_pos[1,:] = np.repeat(np.arange(height),width)
        visual_image_pos[2,:] = 1
        visual_image_pos[3,:] = 1 / depth_img.reshape(width * height)
        visual_pos_in_thermal = self.W * visual_image_pos

        visual_pos_in_thermal = visual_pos_in_thermal / visual_pos_in_thermal[2]

        x_map = visual_pos_in_thermal[0].reshape(height, width).astype(np.float32)
        y_map = visual_pos_in_thermal[1].reshape(height, width).astype(np.float32)

        thermal_mapped = cv2.remap(thermal, x_map, y_map, cv2.INTER_LINEAR)
        merged = cv2.addWeighted(visual, 0, thermal_mapped, 1.0, 0)
        cv2.imwrite(output_name, merged)
