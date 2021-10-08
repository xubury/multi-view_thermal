import numpy as np
import sys
import math
import matplotlib.pyplot as plt


def ransacPlot(n, x, y, m, c, final=False, xIn=(), yIn=(), points=()):
    outputName = "output/figure-" + str(n) + ".pdf"
    if final:
        outputName = "output/final.pdf"
        title = 'Final'
    else:
        title = 'Iteration' + str(n)
    lineWidth = 1
    lineColor = '#0080ff'

    plt.figure(figsize=(8, 8))
    plt.xlabel("Scale")
    plt.ylabel("Score")

    grid = [-max(x) / 2.0, max(x), min(y) - 0.2 * min(y), max(y) + 0.2 * min(y)]
    plt.axis(grid)

    plt.scatter(x, y,  marker='o',
                label='Scale Estimations', color="#00cc00")
    if m == float('inf'):
        #  plt.plot(c, c, 'r', label='line model', color=lineColor)
        plt.axvline(x=c, label="RANSAC result", color=lineColor)
    else:
        plt.plot(x, m * x + c, 'r', label='RANSAC result', color=lineColor)
    if not final:
        plt.plot(xIn, yIn, marker='o', label='inliers',
                 color=lineColor, linestyle='None', alpha=1.0)
        plt.plot(points[:, 0], points[:, 1], marker='o',
                 label='Picked points', color='#ff0000', linestyle='None', alpha=1.0)

    plt.legend()
    plt.savefig(outputName)
    plt.close()


def findInterceptPoints(m, c, x0, y0):
    if m == float('inf'):
        return c, y0
    if m == 0:
        return x0, c
    else:
        x = (x0 + m * y0 - m * c) / (1 + m ** 2)
        y = (m * x0 + (m ** 2) * y0 - (m ** 2) * c) / (1 + m ** 2) + c
        return x, y


def findLineModel(points):
    m = (points[1, 1] - points[0, 1]) / \
        (points[1, 0] - points[0, 0] + sys.float_info.epsilon)
    c = points[1, 1] - m * points[1, 0]
    return m, c

def findBandModel(points):
    sum = np.sum(points[:,1])
    c = 0
    for pt in points:
        c += pt[1] / sum * pt[0]
    return c

class Ransac:
    iteration = 0
    threshold = 0
    ratio = 0
    nPicked = 1

    def __init__(self, iteration, threshold, ratio, picked, debug = False):
        self.iteration = iteration
        self.threshold = threshold
        self.ratio = ratio
        self.nPicked = picked
        self.debug = debug

    def fit(self, x, y):
        data = np.hstack((x, y))
        maxRatio = 0.
        minDist = 0.
        modelM = 0.
        modelC = 0.

        for it in range(self.iteration):
            n = self.nPicked
            nSample = x.shape[0]
            allIndices = np.arange(x.shape[0])
            np.random.shuffle(allIndices)
            indices1 = allIndices[:n]
            indices2 = allIndices[n:]
            maybePts = data[indices1, :]
            testPts = data[indices2, :]

            c = findBandModel(maybePts)
            m = float('inf')
            #  c = maybePts[0, 0]

            xList = []
            yList = []
            num = 0
            totalDist = 0

            for i in range(testPts.shape[0]):
                x0 = testPts[i, 0]
                y0 = testPts[i, 1]

                x1, y1 = findInterceptPoints(m, c, x0, y0)

                dist = math.sqrt((x1 - x0) ** 2 + (y1 - y0) ** 2)

                if dist < self.threshold:
                    xList.append(x0)
                    yList.append(y0)
                    num += 1
                    totalDist += dist

            xInliers = np.array(xList)
            yInliers = np.array(yList)
            ratio = num / float(nSample)
            if ratio > maxRatio or (ratio == maxRatio and totalDist < minDist):
                minDist = totalDist
                maxRatio = num / float(nSample)
                modelM = m
                modelC = c
            print("inlier ratio", num / float(nSample))
            print('model m', m)
            print('model c', c)

            if self.debug:
                ransacPlot(it, testPts[:,0], testPts[:,1], m, c, False,
                           xInliers, yInliers, maybePts)
            #  if maxRatio > self.ratio:
            #      print("Model is found")
            #      break

        # plot the final model
        if self.debug:
            ransacPlot(0, x, y, modelM, modelC, True)

        print('\nFinal model:\n')
        print('  ratio = ', maxRatio)
        print('  model m = ', modelM)
        print('  model c = ', modelC)
        return modelM, modelC


if __name__ == "__main__":
    nSample = 30
    outlierRatio = 0.3

    nInputs = 1
    nOutputs = 1

    x = 30 * np.random.normal(size=(nSample, nInputs))
    perfectFit = 10 * np.random.normal(size=(nInputs, nOutputs))

    y = np.dot(x, perfectFit)

    xNoise = np.random.normal(size=x.shape)
    yNoise = y + np.random.normal(size=y.shape)

    nOutliers = int(outlierRatio * nSample)
    indices = np.arange(xNoise.shape[0])
    np.random.shuffle(indices)
    outlierIndices = indices[:nOutliers]

    xNoise[outlierIndices] = 30 * np.random.random(size=(nOutliers, nInputs))
    #  yNoise[outlierIndices] = 30 * np.random.random(size=(nOutliers, nOutputs))

    yNoise[outlierIndices] = 30 *\
        (np.random.normal(size=(nOutliers, nInputs))**2)

    ransac = Ransac(30, 1, 0.6, 2, True)
    ransac.fit(xNoise, yNoise)
