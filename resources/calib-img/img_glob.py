import glob
import os
import struct

def get_mve_views_entries(search_path):
    dir_entries = glob.glob(os.path.join(search_path, "view_[0-9]*.mve"), recursive=True)
    return dir_entries

def search_files_by_time(path, regex, reverse = False):
    regex = os.path.join(path, regex)
    files = sorted(glob.glob(regex), key=lambda x: os.path.getmtime(x), reverse = reverse)
    return files

def remove_suffix(s):
    return s[:s.rfind(".")]

def search_files_by_name(path, regex, reverse = False):
    regex = os.path.join(path, regex)
    files = sorted(glob.glob(regex), key=lambda x: int(remove_suffix(os.path.basename(x))), reverse = reverse)
    return files


def read_mvei_file(path):
    with open(path, "rb") as file:
        file.read(11) #mvei signature
        header = struct.unpack('i' * 4, file.read(4 * 4))
        n =  header[0] * header[1] * header[2]
        if header[3] == 9:
            buf = struct.unpack('f' * n, file.read(n * 4))
            return buf, header[0], header[1]
        else:
            print("Format unsupported yet!")
