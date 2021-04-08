import glob
import os
import shutil
import re
import numpy as np
import utils

def getMVEEntries(search_path):
    dir_entries = glob.glob(os.path.join(
        search_path, "view_[0-9]*.mve"), recursive=True)
    return dir_entries


def globFilesByTime(path, regex, reverse=False):
    regex = os.path.join(path, regex)
    files = sorted(glob.glob(regex),
                   key=lambda x: os.path.getmtime(x), reverse=reverse)
    return files

def globFilesByName(path, regex, reverse=False):
    regex = os.path.join(path, regex)
    files = sorted(glob.glob(regex), key=lambda x: int(
        utils.removeSuffix(os.path.basename(x))), reverse=reverse)
    return files


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
