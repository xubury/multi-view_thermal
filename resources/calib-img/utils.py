import glob
import os
import cv2

def search_all_files_return_by_time_reversed(regex):
    return sorted(glob.glob(regex), key=lambda x: os.path.getmtime(x))

        
def undistort_image(output_path, images, matrix, dist):
    output_path = os.path.join(output_path, "undistorted")
    if not os.path.exists(output_path):
        os.mkdir(output_path)

    for idx, fname in enumerate(images):
        img = cv2.imread(fname)
        img = cv2.undistort(img, matrix, dist, None, None)
        write_name = str(idx)+'.jpg'
        cv2.imwrite(os.path.join(output_path, write_name), img)
