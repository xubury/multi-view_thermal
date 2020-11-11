import glob
import os


def get_mve_views_entries(search_path):
    dir_entries = glob.glob(os.path.join(search_path, "view_[0-9]*.mve"), recursive=True)
    return dir_entries

def search_files_by_time(path, regex, reverse = False):
    regex = os.path.join(path, regex)
    files = sorted(glob.glob(regex), key=lambda x: os.path.getmtime(x), reverse = reverse)
    return files
