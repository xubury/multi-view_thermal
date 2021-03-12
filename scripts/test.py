import imutils
import os
import img_glob
import matching
import numpy as np
import cv2
import orb

scale_factor = 2
# the grid I used has 50mm distance between neighboring points horizontally and 25mm vertically
matcher = matching.Matching(2**scale_factor, 50, 25)
base_dir = "E:\\recon-image\\1-13\\2\\visual\\scene\\views\\view_0000.mve"
visual_name = os.path.join(base_dir, "undist-L2.png")
thermal_name = os.path.join(base_dir, "thermal.jpg")
dm_name = os.path.join(base_dir, "smvs-B2.mvei")
output_name = "merged-test.jpg"

pos = matcher.match_thermal_to_visual(
    visual_name, thermal_name, dm_name, output_name, 90)

thermal = cv2.imread(thermal_name)
visual = cv2.imread(visual_name)


template = cv2.cvtColor(thermal, cv2.COLOR_BGR2GRAY)
template = cv2.Canny(template, 50, 200)
(tH, tW) = template.shape[:2]

# loop over the images to find the template in

# load the image, convert it to grayscale, and initialize the
# bookkeeping variable to keep track of the matched region
gray = cv2.cvtColor(visual, cv2.COLOR_BGR2GRAY)
found = []

# loop over the scales of the image
for scale in np.linspace(0.2, 1.5, 100)[::-1]:
    # resize the image according to the scale, and keep track
    # of the ratio of the resizing
    resized = imutils.resize(
        gray, width=int(gray.shape[1] * scale))
    r = gray.shape[1] / float(resized.shape[1])

    # if the resized image is smaller than the template, then break
    # from the loop
    if resized.shape[0] < tH or resized.shape[1] < tW:
        break

    # detect edges in the resized, grayscale image and apply template
    # matching to find the template in the image
    edged = cv2.Canny(resized, 50, 200)
    result = cv2.matchTemplate(
        edged, template, cv2.TM_CCOEFF)
    (_, maxVal, _, maxLoc) = cv2.minMaxLoc(result)

    # if we have found a new maximum correlation value, then update
    # the bookkeeping variable
    if len(found) == 0 or maxVal > found[0]:
        found = (maxVal, maxLoc, r)

# unpack the bookkeeping variable and compute the (x, y) coordinates
# of the bounding box based on the resized ratio
(_, maxLoc, r) = found
(startX, startY) = (int(maxLoc[0] * r), int(maxLoc[1] * r))
(endX, endY) = (
    int((maxLoc[0] + tW) * r), int((maxLoc[1] + tH) * r))

crop_img = visual[startY:endY, startX:endX]
cv2.imwrite("crop.jpg", crop_img)
cv2.imwrite("thermal.jpg", thermal)

crop_img = cv2.resize(
    crop_img, (thermal.shape[1], thermal.shape[0]))
final = np.concatenate((crop_img, thermal), axis=1)
cv2.imwrite("align.jpg", final)

# draw a bounding box around the detected result and display the image
cv2.rectangle(visual, (startX, startY),
              (endX, endY), (0, 0, 255), 2)

cv2.imwrite("detected.jpg", visual)

minX = template.shape[1]
minY = template.shape[0]
maxX = 0
maxY = 0
for y in range(startY, endY):
    for x in range(startX, endX):
        testPos = pos[:, x + y * visual.shape[1]]
        testPos = np.reshape(testPos, (4, 1))
        if testPos[3] == float('inf'):
            continue
        res = matcher.W * testPos
        res = res / res[2]

        refX = int(res[0])
        refY = int(res[1])
        # oX = int(x / r)
        # oY = int(y / r)
        if refX >= 0 and refX < template.shape[1] and refY >= 0 and refY < template.shape[0]:
            minX = min(minX, x)
            minY = min(minY, y)
            maxX = max(maxX, x)
            maxY = max(maxY, y)
            visual[y, x] = thermal[refY, refX]
scale = ((maxX - minX) / r * (maxY - minY) / r) / \
    ((endX - startX) * (endY - startY))
print(scale ** 0.5)
cv2.rectangle(visual, (minX, minY),
              (maxX, maxY), (0, 255, 0), 2)
cv2.imshow("test", visual)
cv2.waitKey(0)
