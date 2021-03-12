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
