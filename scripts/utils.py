import numpy as np
import cv2


def transformLeftToRight(K_l, Rt_l, K_r, Rt_r):
    W = K_r * Rt_r * Rt_l.I * K_l.I
    return np.matrix(W)


def toHomogenousMatrix(K):
    K = np.vstack((K, [0, 0, 0]))
    K = np.hstack((K, [[0], [0], [0], [1]]))
    return np.matrix(K)


def rtToMatrix(R, t, Rodrigues=True):
    if Rodrigues:
        rvec, _ = cv2.Rodrigues(R, None)
    else:
        rvec = R
    transform = np.hstack((rvec, t))
    transform = np.vstack((transform, [0, 0, 0, 1]))
    return np.matrix(transform)


def calc_MI(X, Y):
    bins = 255
    c_XY = np.histogram2d(X, Y, bins)[0]
    c_X = np.histogram(X, bins)[0]
    c_Y = np.histogram(Y, bins)[0]

    H_X = shan_entropy(c_X)
    H_Y = shan_entropy(c_Y)
    H_XY = shan_entropy(c_XY)

    MI = H_X + H_Y - H_XY
    return MI


def shan_entropy(c):
    c_normalized = c / float(np.sum(c))
    c_normalized = c_normalized[np.nonzero(c_normalized)]
    H = -sum(c_normalized * np.log2(c_normalized))
    return H
