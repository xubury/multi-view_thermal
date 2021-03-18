import numpy as np
import cv2
from scipy import ndimage


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


EPS = np.finfo(float).eps


def mutualInformation2D(x, y, sigma=1, normalized=False):
    """
    Computes (normalized) mutual information between two 1D variate from a
    joint histogram.
    Parameters
    ----------
    x : 1D array
        first variable
    y : 1D array
        second variable
    sigma: float
        sigma for Gaussian smoothing of the joint histogram
    Returns
    -------
    nmi: float
        the computed similariy measure
    """
    bins = (256, 256)

    jh = np.histogram2d(x, y, bins=bins)[0]

    # smooth the jh with a gaussian filter of given sigma
    ndimage.gaussian_filter(jh, sigma=sigma, mode='constant',
                            output=jh)

    # compute marginal histograms
    jh = jh + EPS
    sh = np.sum(jh)
    jh = jh / sh
    s1 = np.sum(jh, axis=0).reshape((-1, jh.shape[0]))
    s2 = np.sum(jh, axis=1).reshape((jh.shape[1], -1))

    # Normalised Mutual Information of:
    # Studholme,  jhill & jhawkes (1998).
    # "A normalized entropy measure of 3-D medical image alignment".
    # in Proc. Medical Imaging 1998, vol. 3338, San Diego, CA, pp. 132-143.
    if normalized:
        mi = ((np.sum(s1 * np.log(s1)) + np.sum(s2 * np.log(s2)))
              / np.sum(jh * np.log(jh))) - 1
    else:
        mi = (np.sum(jh * np.log(jh)) - np.sum(s1 * np.log(s1))
              - np.sum(s2 * np.log(s2)))

    return mi
