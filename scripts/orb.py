from __future__ import print_function
import numpy as np
import argparse
import imutils
import glob
import cv2
import os

MAX_FEATURES = 500
GOOD_MATCH_PERCENT = 0.15


def alignImages(src, dst, filename, _type):

    # Convert images to grayscale
    srcGray = cv2.cvtColor(src, cv2.COLOR_BGR2GRAY)
    dstGray = cv2.cvtColor(dst, cv2.COLOR_BGR2GRAY)

    # Detect ORB features and compute descriptors.
    orb = cv2.ORB_create(MAX_FEATURES)
    srcKeypoints, srcDescriptors = orb.detectAndCompute(srcGray, None)
    dstKeypoints, dstDescriptors = orb.detectAndCompute(dstGray, None)

    # Match features.
    matcher = cv2.DescriptorMatcher_create(
        cv2.DESCRIPTOR_MATCHER_BRUTEFORCE_HAMMING)
    matches = matcher.match(srcDescriptors, dstDescriptors, None)

    # Sort matches by score
    matches.sort(key=lambda x: x.distance, reverse=False)

    # Remove not so good matches
    numGoodMatches = int(len(matches) * GOOD_MATCH_PERCENT)
    matches = matches[:numGoodMatches]

    # Draw top matches
    imMatches = cv2.drawMatches(
        src, srcKeypoints, dst, dstKeypoints, matches, None)
    cv2.imwrite(filename, imMatches)
    # Extract location of good matches
    srcPts = np.zeros((len(matches), 2), dtype=np.float32)
    dstPts = np.zeros((len(matches), 2), dtype=np.float32)

    for i, match in enumerate(matches):
        srcPts[i, :] = srcKeypoints[match.queryIdx].pt
        dstPts[i, :] = dstKeypoints[match.trainIdx].pt

    # Find homography
    h, _ = cv2.findHomography(srcPts, dstPts, cv2.RANSAC)

    # Use homography
    im1Reg = cv2.warpPerspective(src, h, (dst.shape[1], dst.shape[0]))

    return im1Reg, h


if __name__ == '__main__':
    print("Enter the required options : ")
    print("1) Automatic Registration on all images")
    print("2) Enter name of visible and thermal image yourself")
    option = int(input())
    if(option == 1):
        directory = 'thermal'
        thermal_images_files = []
        for filename in os.listdir(directory):
            thermal_images_files.append(filename)
        directory = 'visible'
        visible_images_files = []
        for filename in os.listdir(directory):
            visible_images_files.append(filename)
        for i in range(len(thermal_images_files)):
            try:
                filename = './results_orb/'+str(i)+'.jpg'
                if os.path.exists(filename):
                    print("image already exists")
                    continue
                else:
                    print("performing registration")
                    # load the image image, convert it to grayscale, and detect edges
                    template = cv2.imread('thermal/'+thermal_images_files[i])
                    template = cv2.cvtColor(template, cv2.COLOR_BGR2GRAY)
                    template = cv2.Canny(template, 50, 200)
                    (tH, tW) = template.shape[:2]

                    # loop over the images to find the template in

                    # load the image, convert it to grayscale, and initialize the
                    # bookkeeping variable to keep track of the matched region
                    image = cv2.imread('visible/'+visible_images_files[i])
                    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
                    found = []

                    # loop over the scales of the image
                    for scale in np.linspace(0.2, 1.0, 20)[::-1]:
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
                        if found is None or maxVal > found[0]:
                            found = (maxVal, maxLoc, r)

                    # unpack the bookkeeping variable and compute the (x, y) coordinates
                    # of the bounding box based on the resized ratio
                    (_, maxLoc, r) = found
                    (startX, startY) = (int(maxLoc[0] * r), int(maxLoc[1] * r))
                    (endX, endY) = (
                        int((maxLoc[0] + tW) * r), int((maxLoc[1] + tH) * r))

                    # draw a bounding box around the detected result and display the image
                    cv2.rectangle(image, (startX, startY),
                                  (endX, endY), (0, 0, 255), 2)
                    crop_img = image[startY:endY, startX:endX]

                    name = "thermal/"+thermal_images_files[i]
                    thermal_image = cv2.imread(name, cv2.IMREAD_COLOR)

                    crop_img = cv2.resize(
                        crop_img, (thermal_image.shape[1], thermal_image.shape[0]))

                    cv2.imwrite(os.path.join(
                        './output_orb/', str(i)+'.jpg'), crop_img)

                    final = np.concatenate((crop_img, thermal_image), axis=1)
                    cv2.imwrite(os.path.join(
                        './results_orb/', str(i)+'.jpg'), final)

                    cv2.waitKey(0)

                    # Read reference image
                    refFilename = "thermal/"+thermal_images_files[i]
                    print("Reading reference image : ", refFilename)
                    imReference = cv2.imread(refFilename, cv2.IMREAD_COLOR)

                    # Read image to be aligned
                    imFilename = "output_orb/"+str(i)+'.jpg'
                    print("Reading image to align : ", imFilename)
                    im = cv2.imread(imFilename, cv2.IMREAD_COLOR)
                    file_name = thermal_images_files[i]
                    imReg, h = alignImages(
                        im, imReference, file_name, "cropped")
                    print("Estimated homography : \n",  h)
                    imReg, h = alignImages(
                        image, imReference, file_name, "uncropped")

            except:
                pass

    else:
        print("Enter the name of thermal image in thermal folder")
        thermal = input()
        print("Enter the name of visible image in visible folder")
        visible = input()
        image = cv2.imread('visible/'+visible+'.jpg')
        template = cv2.imread('thermal/'+thermal+'.jpg')

        print("performing registration")
        # load the image image, convert it to grayscale, and detect edges
        template = cv2.cvtColor(template, cv2.COLOR_BGR2GRAY)
        template = cv2.Canny(template, 50, 200)
        (tH, tW) = template.shape[:2]

        # loop over the images to find the template in

        # load the image, convert it to grayscale, and initialize the
        # bookkeeping variable to keep track of the matched region
        image = cv2.imread('visible/'+visible+'.jpg')
        gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
        found = None

        # loop over the scales of the image
        for scale in np.linspace(0.2, 1.0, 20)[::-1]:
            # resize the image according to the scale, and keep track
            # of the ratio of the resizing
            resized = imutils.resize(gray, width=int(gray.shape[1] * scale))
            r = gray.shape[1] / float(resized.shape[1])

            # if the resized image is smaller than the template, then break
            # from the loop
            if resized.shape[0] < tH or resized.shape[1] < tW:
                break

            # detect edges in the resized, grayscale image and apply template
            # matching to find the template in the image
            edged = cv2.Canny(resized, 50, 200)
            result = cv2.matchTemplate(edged, template, cv2.TM_CCOEFF)
            (_, maxVal, _, maxLoc) = cv2.minMaxLoc(result)

            # if we have found a new maximum correlation value, then update
            # the bookkeeping variable
            if found is None or maxVal > found[0]:
                found = (maxVal, maxLoc, r)

        # unpack the bookkeeping variable and compute the (x, y) coordinates
        # of the bounding box based on the resized ratio
        (_, maxLoc, r) = found
        (startX, startY) = (int(maxLoc[0] * r), int(maxLoc[1] * r))
        (endX, endY) = (int((maxLoc[0] + tW) * r), int((maxLoc[1] + tH) * r))

        # draw a bounding box around the detected result and display the image
        cv2.rectangle(image, (startX, startY), (endX, endY), (0, 0, 255), 2)
        crop_img = image[startY:endY, startX:endX]

        name = "thermal/"+thermal+'.jpg'
        thermal_image = cv2.imread(name, cv2.IMREAD_COLOR)

        crop_img = cv2.resize(
            crop_img, (thermal_image.shape[1], thermal_image.shape[0]))

        cv2.imwrite(os.path.join('./output_orb/', thermal+'.jpg'), crop_img)

        final = np.concatenate((crop_img, thermal_image), axis=1)
        cv2.imwrite(os.path.join('./results_orb/', thermal+'.jpg'), final)

        cv2.waitKey(0)

        # Read reference image
        refFilename = "thermal/"+thermal+'.jpg'
        print("Reading reference image : ", refFilename)
        imReference = cv2.imread(refFilename, cv2.IMREAD_COLOR)

        # Read image to be aligned
        imFilename = "output_orb/"+thermal+'.jpg'
        print("Reading image to align : ", imFilename)
        im = cv2.imread(imFilename, cv2.IMREAD_COLOR)
        file_name = thermal+'.jpg'
        imReg, h = alignImages(im, imReference, file_name, "cropped")
        print("Estimated homography : \n",  h)
        imReg, h = alignImages(image, imReference, file_name, "uncropped")
