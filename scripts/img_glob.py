import glob
import os
import struct
import shutil
import re
import numpy as np


def getMVEEntries(search_path):
    dir_entries = glob.glob(os.path.join(
        search_path, "view_[0-9]*.mve"), recursive=True)
    return dir_entries


def globFilesByTime(path, regex, reverse=False):
    regex = os.path.join(path, regex)
    files = sorted(glob.glob(regex),
                   key=lambda x: os.path.getmtime(x), reverse=reverse)
    return files


def removeSuffix(s):
    return s[:s.rfind(".")]


def globFilesByName(path, regex, reverse=False):
    regex = os.path.join(path, regex)
    files = sorted(glob.glob(regex), key=lambda x: int(
        removeSuffix(os.path.basename(x))), reverse=reverse)
    return files


def readMVEI(path):
    with open(path, "rb") as file:
        file.read(11)  # mvei signature
        header = struct.unpack('i' * 4, file.read(4 * 4))
        n = header[0] * header[1] * header[2]
        if header[3] == 9:
            buf = struct.unpack('f' * n, file.read(n * 4))
            buf = np.array(buf)
            # header[0] == width and header[1] == height
            buf = buf.reshape(header[1], header[0])
            return buf
        else:
            print("Format unsupported yet!")


def copyThermalToDir(thermal_dir, mve_dir):
    mve_entris = getMVEEntries(mve_dir)
    thermal_images_list = globFilesByTime(thermal_dir, "*.jpg")

    for idx, name in enumerate(thermal_images_list):
        for filename in os.listdir(mve_entris[idx]):
            if re.search("original.jpg", filename):
                dst_file_name = os.path.join(mve_entris[idx], "thermal.jpg")
                shutil.copyfile(name, dst_file_name)
                print("Copying " + name + " to " + dst_file_name)
                break
