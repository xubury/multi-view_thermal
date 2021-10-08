import img_glob
mve_dir = "E:\\recon-image\\2021-06-17\\1\\visible\\scene\\views"
thermal_dir = "E:\\recon-image\\2021-06-17\\1\\thermal-2"
# mve_dir = "E:/recon-image/1-13/2/visual/scene/views"
# thermal_dir = "E:/recon-image/1-13/2/thermal"

mve_entris = img_glob.getMVEEntries(mve_dir)
img_glob.copyThermalToDir(thermal_dir, mve_dir)
