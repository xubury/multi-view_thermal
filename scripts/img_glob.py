import glob
import os
import struct


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
            return buf, header[0], header[1]
        else:
            print("Format unsupported yet!")
