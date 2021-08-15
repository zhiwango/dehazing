# rewrite source code to python version
# -*- coding: utf-8 -*-
import numpy as np
import cv2


def calc_y_channel(src):
    tmp = src.copy()
    cv2.cvtColor(tmp, cv2.COLOR_RGB2GRAY, tmp)
    y_channel, u_channel, v_channel = cv2.split(tmp)
    # cv2.imshow("y_channel", y_channel)
    return y_channel


def calc_dark_channel(src, block_size):
    b, g, r = cv2.split(src)
    rgb_min = cv2.min(cv2.min(b, g), r)
    kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (block_size, block_size))
    dark_channel = cv2.erode(rgb_min, kernel)
    # cv2.imshow("rgb_min", rgb_min)
    # cv2.imshow("dark_channel", dark_channel)
    return dark_channel


def calc_morphology_threshold_image(dark_channel, morphology_size):
    # filtered the 90% pixel in dark channel
    pixel_list = []
    for i, j in np.ndindex(dark_channel.shape):
        pixel_list.append(dark_channel[i, j])
    # set the rest 10% pixel to air area
    sorted_pixel_list = sorted(pixel_list, reverse=True)
    threshold_sort_pixel = sorted_pixel_list[int(dark_channel.shape[0] * dark_channel.shape[1] * 0.1)]
    # create a threshold image (white = air light in concern area, black = not in concern area)
    threshold_img = dark_channel.copy()
    for i, j in np.ndindex(dark_channel.shape):
        if dark_channel[i, j] < threshold_sort_pixel:
            threshold_img[i, j] = 0
        else:
            threshold_img[i, j] = 255
    # cv2.imshow("threshold_img", threshold_img)
    # morphological transformation
    element = cv2.getStructuringElement(cv2.MORPH_RECT, (morphology_size, morphology_size))
    morphology_threshold_img = cv2.morphologyEx(threshold_img, cv2.MORPH_OPEN, element)
    return morphology_threshold_img


def calc_air_light(src, y_channel, dark_channel, threshold_img, is_show_circled_image):
    circle_img = src.copy()
    # find the brightest pixel in dark channel under the air area
    air_light_val = 0
    air_light_idx = []
    for i, j in np.ndindex(dark_channel.shape):
        if threshold_img[i, j] == 255:
            _, air_light_val, _, air_light_idx = cv2.minMaxLoc(dark_channel)
    cv2.circle(circle_img, air_light_idx, 5, (0, 255, 0), thickness=2)
    # find the brightest pixel in Y channel
    if is_show_circled_image is True:
        _, _, _, max_pt = cv2.minMaxLoc(y_channel)
        cv2.circle(circle_img, max_pt, 5, (0, 0, 255), thickness=2)
        cv2.imshow("circle_point", circle_img)
    return air_light_val


def calc_average_pixel(src):
    min_val, max_val, _, _ = cv2.minMaxLoc(src)
    average = (min_val + max_val) / 2
    return average


def calc_gamma_correction(src, gamma):
    # inv_gamma = 1.0 / gamma
    gamma_table = [np.power(i / 255.0, gamma) * 255.0 for i in range(256)]
    gamma_table = np.round(np.array(gamma_table)).astype(np.uint8)
    return cv2.LUT(src, gamma_table)


def calc_transmission(min_med_img, air_light, th_omega):
    m = calc_average_pixel(min_med_img) / 255.0
    p = 1.3
    q = 1 + (m - 0.5)
    omega = min(m * p * q, th_omega)
    transmission_map = np.uint8(255 * (1 - omega * min_med_img / air_light))
    calc_gamma_correction(transmission_map, p - m)
    # cv2.imshow("transmission_map", transmission_map)
    return transmission_map


def clamp(minimum, x, maximum):
    return max(minimum, min(x, maximum))


def dehaze(src, transmission, air_light):
    restored_image = np.zeros(src.shape[:3], np.uint8)
    for i in range(src.shape[0]):
        for j in range(src.shape[1]):
            t_max = max(transmission[i, j] / 255, 0.1)
            for ch in range(src.shape[2]):
                restored_image.itemset((i, j, ch), clamp(0, (src.item(i, j, ch) - air_light) / t_max + air_light, 255))
    return restored_image


def main():
    block_size = 5
    morphology_transform_kernel_size = 15
    median_filter_kernel_size = 5
    show_circled_image = True
    image = cv2.imread("./img/canyon2.bmp")
    y_channel = calc_y_channel(image)
    dark_channel = calc_dark_channel(image, block_size)
    threshold_img = calc_morphology_threshold_image(dark_channel, morphology_transform_kernel_size)
    air_light = calc_air_light(image, y_channel, dark_channel, threshold_img, show_circled_image)
    min_med_img = cv2.medianBlur(y_channel, median_filter_kernel_size)
    transmission_map = calc_transmission(min_med_img, air_light, 0.95)
    dehaze_image = dehaze(image, transmission_map, air_light)
    print("atmospheric light: ", air_light)
    cv2.imshow("src", image)
    cv2.imshow("dst", dehaze_image)
    cv2.waitKey(0)
    cv2.destroyAllWindows()


if __name__ == '__main__':
    main()
