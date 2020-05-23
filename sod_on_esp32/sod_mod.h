/*
* NOTE: PORTED FOR EE3080 DIP PROJECT; TO RUN ON ESP32 BOARD.
*		Only necessary functions are kept, everything else is discarded.
*
* ======================================================================================
*
* SOD - An Embedded Computer Vision & Machine Learning Library.
* Copyright (C) 2018 - 2020 PixLab| Symisc Systems. https://sod.pixlab.io
* Version 1.1.8
*
* Symisc Systems employs a dual licensing model that offers customers
* a choice of either our open source license (GPLv3) or a commercial 
* license.
*
* For information on licensing, redistribution of the SOD library, and for a DISCLAIMER OF ALL WARRANTIES
* please visit:
*     https://pixlab.io/sod
* or contact:
*     licensing@symisc.net
*     support@pixlab.io
*/
/*
 * This file is part of Symisc SOD - Open Source Release (GPLv3)
 *
 * SOD is free software : you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * SOD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SOD. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SOD_H_
#define _SOD_H_

#include <Arduino.h>

/* Ensure compatibility with CPP program (Arduino sketch). */
#ifdef __cplusplus
extern "C" {
#endif

#define SOD_IMG_COLOR     0 /* Load full color channels. */
#define SOD_IMG_GRAYSCALE 1 /* Load an image in the grayscale colorpsace only (single channel). */

/* Possible return value from each exported SOD interface defined below. */
#define SOD_OK           0 /* Everything went well */
#define SOD_UNSUPPORTED -1 /* Unsupported Pixel format */
#define SOD_OUTOFMEM    -4 /* Out-of-Memory */
#define SOD_ABORT	    -5 /* User callback request an operation abort */
#define SOD_IOERR       -6 /* IO error */
#define SOD_LIMIT       -7 /* Limit reached */


/* bounding box structure documented at https://sod.pixlab.io/api.html#sod_box. */
typedef struct sod_box sod_box;
/*
 * Image/Frame container documented at https://sod.pixlab.io/api.html#sod_img. */
typedef struct sod_img sod_img;
/* 
 * Point instance documented at https://sod.pixlab.io/api.html#sod_pts. */
typedef struct sod_pts sod_pts;

/*
 * A bounding box or bbox for short is represented by an instance of the `sod_box` structure.
 * A sod_box instance always store the coordinates of a rectangle obtained from a prior successful
 * call to one of the object detection routines of a sod_cnn or sod_realnet handle such as
 * `sod_cnn_predict()` or from the connected component labeling interface `sod_image_find_blobs()`.
 *
 * Besides the rectangle coordinates. The `zName` and `score` fields member of this structure hold
 * useful information about the object it surround.
 *
 * This structure and related interfaces are documented at https://sod.pixlab.io/api.html#sod_box.
 */
struct sod_box {
	int x;  /* The x-coordinate of the upper-left corner of the rectangle */
	int y;  /* The y-coordinate of the upper-left corner of the rectangle */
	int w;  /* Rectangle width */
	int h;  /* Rectangle height */
	float score;       /* Confidence threshold. */
	const char *zName; /* Detected object name. I.e. person, face, dog, car, plane, cat, bicycle, etc. */
	void *pUserData;   /* External pointer used by some modules such as the face landmarks, NSFW classifier, pose estimator, etc. */
};
/*
 * Internally, each in-memory representation of an input image or video frame
 * is kept in an instance of the `sod_img` structure. Basically, a `sod_img` is just a record
 * of the width, height and number of color channels in an image, and also the pixel values
 * for every pixel. Images pixels are arranged in CHW format. This means in a 3 channel 
 * image with width 400 and height 300, the first 400 values are the 1st row of the 1st channel
 * of the image. The second 400 pixels are the 2nd row. after 120,000 values we get to pixels
 * in the 2nd channel, and so forth.
 *
 * This structure and related interfaces are documented at https://sod.pixlab.io/api.html#sod_img.
 */
struct sod_img {
	int h;   /* Image/frame height */
	int w;   /* Image/frame width */
	int c;   /* Image depth/Total number of color channels e.g. 1 for grayscale images, 3 RGB, etc. */
	uint8_t *data; /* Blob */
};
/*
 * An instance of the `sod_pts` structure describe a 2D point in space with integer coordinates
 * (usually zero-based). This structure is rarely manipulated by SOD and is used mostly by 
 * the Hough line detection interface `sod_hough_lines_detect()` and line drawing routine `sod_image_draw_line()`.
 *
 * This structure and related interfaces are documented at https://sod.pixlab.io/api.html#sod_pts.
 */
struct sod_pts {
	int x; /* The x-coordinate, in logical units of the point offset. */
	int y; /* The y-coordinate, in logical units of the point offset. */
};


/* Function prototypes */

/* Freeing functions */
void sod_free_image(sod_img m);
void sod_hough_lines_release(sod_pts * pLines);

/* Image creation functions */
sod_img sod_make_empty_image(int w, int h, int c);
sod_img sod_make_image(int w, int h, int c);
sod_img sod_copy_image(sod_img m);

/* Drawing functions */
static inline void set_pixel(sod_img m, int x, int y, int c, uint8_t val);
void sod_image_draw_line(sod_img im, sod_pts start, sod_pts end, uint8_t r, uint8_t g, uint8_t b);

/* Gaussian noise reduce */
/* INPUT IMAGE MUST BE GRAYSCALE */
void sod_gaussian_noise_reduce(sod_img grayscale, sod_img out);

/* Sobel edge detection */
void sod_sobel_image(sod_img im, sod_img out);

/* Canny edge detection */
sod_img sod_canny_edge_image(sod_img im, int reduce_noise);

/* Hough lines detect */
sod_pts * sod_hough_lines_detect(sod_img im, int threshold, int *nPts);

/* Hilditch thinning */
sod_img sod_hilditch_thin_image(sod_img im);

#ifdef __cplusplus
}
#endif

#endif  // _SOD_H_
