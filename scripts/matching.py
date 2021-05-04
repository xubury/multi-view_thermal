import numpy as np
import cv2
import calibration
import img_glob
import os
import utils
from scipy.ndimage import gaussian_filter1d

class Matching():
    calibrator = 0
    scale_factor = 0
    W = np.eye(4)
    debug = False

    def __init__(self, scale_factor, h_step, v_step, debug = False):
        self.calibrator = calibration.ThermalVisualCalibrator(
            "../res/thermal-img", "../res/normal-img", (3, 9), h_step, v_step)
        self.scale_factor = scale_factor
        self.debug = debug

        if os.path.exists("calibration.txt"):
            print("calibration file loaded")
            self.W = np.loadtxt("calibration.txt")
            self.W = np.matrix(self.W)
            print(self.W)
        else:
            self.calibrate()
            np.savetxt("calibration.txt", self.W)

    def calibrate(self):
        self.calibrator.startCalibration()
        calibration.undistorImage("../res/normal-img", self.calibrator.visual_images_list,
                                  self.calibrator.visual_K, self.calibrator.visual_dist)
        calibration.undistorImage("../res/thermal-img", self.calibrator.thermal_images_list,
                                  self.calibrator.thermal_K, self.calibrator.thermal_dist)

        #  Note: In the 3d reconstruction, images are usually scaled
        #  Therefore, K matrix needs to be scaled too if scaled images are used
        print("thermal K (homogeneous):")
        print(self.calibrator.thermal_K_homo)
        print("visual K (homogeneous):")
        K_homo = self.calibrator.visual_K_homo.copy()
        K_homo[0] /= self.scale_factor
        K_homo[1] /= self.scale_factor
        print(K_homo)

        pose_id = 0
        print(self.calibrator.thermal_Rt[0])
        print(self.calibrator.visual_Rt[0])

        self.W = utils.transformLeftToRight(
            K_homo, self.calibrator.visual_Rt[pose_id], self.calibrator.thermal_K_homo, self.calibrator.thermal_Rt[pose_id])
        print("W martix:")
        print(self.W)

    def mapThermalToVisual(self, visual_name, thermal_name, dm_name, depth_scale):

        visual = cv2.imread(visual_name)
        thermal = cv2.imread(thermal_name)
        (height, width, _) = visual.shape

        #  z_R * [u_R, v_R, 1, 1/z_R]^T = W * z_L * [u_L, v_L, 1, 1/z_L]^T
        #  Try using depth map
        depth_img = utils.readMVEI(dm_name)
        depth_img = cv2.resize(depth_img, (visual.shape[1], visual.shape[0]))
        depth_img *= depth_scale

        visual_image_pos = np.zeros((4, width * height), np.float32)
        visual_image_pos[0, :] = np.tile(np.arange(width), height)
        visual_image_pos[1, :] = np.repeat(np.arange(height), width)
        visual_image_pos[2, :] = 1
        visual_image_pos[3, :] = 1 / depth_img.reshape(width * height)
        visual_pos_in_thermal = self.W * visual_image_pos

        visual_pos_in_thermal /= visual_pos_in_thermal[2]

        x_map = visual_pos_in_thermal[0].reshape(
            height, width).astype(np.float32)
        y_map = visual_pos_in_thermal[1].reshape(
            height, width).astype(np.float32)

        thermal_mapped = cv2.remap(thermal, x_map, y_map, cv2.INTER_LINEAR)
        merged = cv2.addWeighted(visual, 0.3, thermal_mapped, 0.7, 0)
        return merged

    def getScaleScore(self, visualName, thermalName, depthMapName, thermalDMName, scale):
        thermal = cv2.imread(thermalName)
        visual = cv2.imread(visualName)
        depthMap = utils.readMVEI(depthMapName)
        depthMap = cv2.resize(depthMap, (visual.shape[1], visual.shape[0]))
        depthMap *= scale
        (rows, cols) = visual.shape[:2]

        visual_image_pos = np.zeros((4, cols * rows), np.float32)
        visual_image_pos[0, :] = np.tile(np.arange(cols), rows)
        visual_image_pos[1, :] = np.repeat(np.arange(rows), cols)
        visual_image_pos[2, :] = 1
        visual_image_pos[3, :] = 1 / depthMap.ravel()
        visual_pos_in_thermal = self.W * visual_image_pos
        depthMap /= scale

        visual_pos_in_thermal /= visual_pos_in_thermal[2]

        x_map = visual_pos_in_thermal[0].reshape(
            rows, cols).astype(np.float32)
        y_map = visual_pos_in_thermal[1].reshape(
            rows, cols).astype(np.float32)
        thermalDM = utils.readMVEI(thermalDMName)
        thermalDM = cv2.resize(thermalDM, (thermal.shape[1], thermal.shape[0]))
        mapped = cv2.remap(
            thermalDM.astype(np.uint8), x_map, y_map, cv2.INTER_LINEAR)
        x, y, w, h = cv2.boundingRect(mapped)
        patch = w * h
        if patch > 0:
            thermalDM = mapped[y:y+h, x:x+w]
            cropDM = depthMap[y:y+h, x:x+w]

            cv2.normalize(cropDM, dst=cropDM,
                          alpha=255, beta=0, norm_type=cv2.NORM_MINMAX)
            cv2.normalize(thermalDM, dst=thermalDM,
                          alpha=255, beta=0, norm_type=cv2.NORM_MINMAX)
            Mi = utils.mutualInformation2D(cropDM.ravel(), thermalDM.ravel())
            if self.debug:
                cv2.imwrite("output/crop" + str(scale) +
                            ".jpg", cropDM)
                cv2.imwrite("output/thermal" + str(scale) + ".jpg",
                            thermalDM)
                cv2.rectangle(visual, (x, y), (x + w, y + h), (0, 255, 0), 2)
                cv2.putText(visual, "scale="+str(scale), (x, y), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0 , 255), 2)
                cv2.imwrite("output/patch" + str(scale) +
                            ".jpg", visual)

            n = patch / (rows * cols)
            return n * Mi
        else:
            return 0

    def guessScale(self, visualName, thermalName, depthMapName, thermalDMName, scales, sigma = 1):
        scores = []

        for scale in scales:
            score = self.getScaleScore(
                visualName, thermalName, depthMapName, thermalDMName, scale)
            scores.append(score)

        scores = gaussian_filter1d(scores, sigma)
        bestid = 0
        for id in range(len(scores)):
            if scores[id] > scores[bestid]:
                bestid = id;
        size = 1
        start = max(bestid - int(size / 2), 0)
        end = start + size
        return  scales[start:end], scores[start:end], scores
