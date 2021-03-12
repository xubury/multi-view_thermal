import numpy as np
import cv2
import calibration
import img_glob
import os
import utils
import imutils


class Matching():
    calibrator = 0
    scale_factor = 0
    W = np.eye(4)

    def __init__(self, scale_factor, h_step, v_step):
        self.calibrator = calibration.ThermalVisualCalibrator(
            "../res/thermal-img", "../res/normal-img", (3, 9), h_step, v_step)
        self.scale_factor = scale_factor

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
        depth_img, dp_width, dp_height = img_glob.readMVEI(dm_name)

        depth_img = np.array(depth_img)
        depth_img = depth_img.reshape(dp_height, dp_width)
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
        merged = cv2.addWeighted(visual, 0.5, thermal_mapped, 0.5, 0)
        return merged

    def getScaleScore(self, visualName, thermalName, depthMapName, scale):
        thermal = cv2.imread(thermalName)
        visual = cv2.imread(visualName)
        depthMap, dp_width, dp_height = img_glob.readMVEI(depthMapName)
        depthMap = np.array(depthMap)
        depthMap = depthMap.reshape(dp_height, dp_width)
        minX = thermal.shape[1]
        minY = thermal.shape[0]
        maxX = 0
        maxY = 0
        (rows, cols) = visual.shape[:2]
        for y in range(rows):
            for x in range(cols):
                if depthMap[y, x] == 0:
                    continue
                testPos = [x, y, 1, 1 / (depthMap[y, x] * scale)]
                testPos = np.reshape(testPos, (4, 1))
                res = self.W * testPos
                res = res / res[2]

                refX = int(res[0])
                refY = int(res[1])
                if refX >= 0 and refX < thermal.shape[1] and refY >= 0 and refY < thermal.shape[0]:
                    minX = min(minX, x)
                    minY = min(minY, y)
                    maxX = max(maxX, x)
                    maxY = max(maxY, y)

        if minX >= 0 and minX < maxX and minY >= 0 and minY < maxY:
            cropImg = visual[minY:maxY + 1, minX:maxX + 1]
            compare = cv2.cvtColor(cropImg, cv2.COLOR_BGR2GRAY)
            compare = cv2.Canny(cropImg, 50, 200)

            template = cv2.resize(
                thermal, (cropImg.shape[1], cropImg.shape[0]))
            template = cv2.cvtColor(template, cv2.COLOR_BGR2GRAY)
            template = cv2.Canny(template, 50, 200)

            result = cv2.matchTemplate(
                compare, template, cv2.TM_CCOEFF)
            (_, maxVal, _, _) = cv2.minMaxLoc(result)
            return maxVal
        else:
            return 0

    def guessScale(self, visualName, thermalName, depthMapName, scales):
        scores = []

        maxScore = 0
        bestScale = 10
        for scale in scales:
            score = self.getScaleScore(
                visualName, thermalName, depthMapName, scale)
            scores.append(score)
            if score > maxScore:
                maxScore = score
                bestScale = scale
                print("scale:", scale, "socre:", score,
                      "bestScale", bestScale, "maxScore", maxScore)

        return bestScale, maxScore, scores
