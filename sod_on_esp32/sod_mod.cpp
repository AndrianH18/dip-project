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
/* $SymiscID: sod.c v1.1.8 Win10 2019-11-16 03:23 stable <devel@symisc.net> $ */

#include "sod_mod.h"

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif /* M_PI */

/* Generic dynamic set */

typedef struct SySet SySet;
struct SySet
{
	void *pBase;               /* Base pointer */
	size_t nUsed;              /* Total number of used slots  */
	size_t nSize;              /* Total number of available slots */
	size_t eSize;              /* Size of a single slot */
	void *pUserData;           /* User private data associated with this container */
};

#define SySetBasePtr(S)           ((S)->pBase)
#define SySetBasePtrJump(S, OFFT)  (&((char *)(S)->pBase)[OFFT*(S)->eSize])
#define SySetUsed(S)              ((S)->nUsed)
#define SySetSize(S)              ((S)->nSize)
#define SySetElemSize(S)          ((S)->eSize)
#define SySetSetUserData(S, DATA)  ((S)->pUserData = DATA)
#define SySetGetUserData(S)       ((S)->pUserData)

/* Dynamic Set Implementation */

static int SySetInit(SySet *pSet, size_t ElemSize)
{
	pSet->nSize = 0;
	pSet->nUsed = 0;
	pSet->eSize = ElemSize;
	pSet->pBase = 0;
	pSet->pUserData = 0;
	return 0;
}
static int SySetPut(SySet *pSet, const void *pItem)
{
	unsigned char *zbase;
	if (pSet->nUsed >= pSet->nSize) {
		void *pNew;
		if (pSet->nSize < 1) {
			pSet->nSize = 8;
		}
		pNew = ps_realloc(pSet->pBase, pSet->eSize * pSet->nSize * 2);
		if (pNew == 0) {
			return SOD_OUTOFMEM;
		}
		pSet->pBase = pNew;
		pSet->nSize <<= 1;
	}
	if (pItem) {
		zbase = (unsigned char *)pSet->pBase;
		memcpy((void *)&zbase[pSet->nUsed * pSet->eSize], pItem, pSet->eSize);
		pSet->nUsed++;
	}
	return SOD_OK;
}

/* Freeing functions */

void sod_free_image(sod_img m)
{
	if (m.data) {
		free(m.data);
	}
}

void sod_hough_lines_release(sod_pts * pLines)
{
	free(pLines);
}

/* Image creation functions */

sod_img sod_make_empty_image(int w, int h, int c)
{
	sod_img out;
	out.data = 0;
	out.h = h;
	out.w = w;
	out.c = c;
	return out;
}

sod_img sod_make_image(int w, int h, int c)
{
	sod_img out = sod_make_empty_image(w, h, c);
	out.data = (uint8_t*) ps_calloc(h*w*c, sizeof(uint8_t));
	return out;
}

sod_img sod_copy_image(sod_img m)
{
	sod_img copy = m;
	copy.data = (uint8_t*) ps_calloc(m.h*m.w*m.c, sizeof(uint8_t));
	if (copy.data && m.data) {
		memcpy(copy.data, m.data, m.h*m.w*m.c * sizeof(uint8_t));
	}
	return copy;
}

/* Drawing functions */

static inline void set_pixel(sod_img m, int x, int y, int c, uint8_t val)
{
	/* x, y, c are already validated by upper layers */
	if (m.data)
		m.data[c*m.h*m.w + y * m.w + x] = val;
}

void sod_image_draw_line(sod_img im, sod_pts start, sod_pts end, uint8_t r, uint8_t g, uint8_t b)
{
	int x1, x2, y1, y2, dx, dy, err, sx, sy, e2;
	if (im.c == 1) {
		/* Draw on grayscale image */
		r = (uint8_t) (b * 0.114 + g * 0.587 + r * 0.299);
	}
	x1 = start.x;
	x2 = end.x;
	y1 = start.y;
	y2 = end.y;
	if (x1 < 0) x1 = 0;
	if (x1 >= im.w) x1 = im.w - 1;
	if (x2 < 0) x2 = 0;
	if (x2 >= im.w) x2 = im.w - 1;

	if (y1 < 0) y1 = 0;
	if (y1 >= im.h) y1 = im.h - 1;
	if (y2 < 0) y2 = 0;
	if (y2 >= im.h) y2 = im.h - 1;

	dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
	dy = abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
	err = (dx > dy ? dx : -dy) / 2;

	for (;;) {
		set_pixel(im, x1, y1, 0, r);
		if (im.c == 3) {
			set_pixel(im, x1, y1, 1, g);
			set_pixel(im, x1, y1, 2, b);
		}
		if (x1 == x2 && y1 == y2) break;
		e2 = err;
		if (e2 > -dx) { err -= dy; x1 += sx; }
		if (e2 < dy) { err += dx; y1 += sy; }
	}
}

/* Threshold image to binary */
void sod_threshold_image(sod_img im, uint8_t thresh)
{
  if (im.data) {
    int i;
    for (i = 0; i < im.w*im.h*im.c; ++i) {
      im.data[i] = im.data[i] > thresh ? 255 : 0;
    }
  }
  return;
}

void sod_sobel_threshold_image(sod_img im, uint8_t thresh)
{
  if (im.data){
    int i = 0;
    int sum = 0;
    uint8_t average;
    for(i = 0; i < im.w*im.h*im.c; i++){
      sum += im.data[i];
    }
    average = sum /(im.w * im.h * im.c);
    for(i = 0; i < im.w*im.h*im.c; i++){
      im.data[i] = abs(im.data[i] - average) > thresh ? 255 : 0;
    } 
  }
  return;
}

/* OTSU Thresholding */

void sod_otsu_binarize_image(sod_img im)
{
#define OTSU_GRAYLEVEL 256
  if (im.data) {
    /* binarization by Otsu's method based on maximization of inter-class variance */
    int hist[OTSU_GRAYLEVEL];
    float prob[OTSU_GRAYLEVEL], omega[OTSU_GRAYLEVEL]; /* prob of graylevels */
    float myu[OTSU_GRAYLEVEL];   /* mean value for separation */
    double max_sigma, sigma[OTSU_GRAYLEVEL]; /* inter-class variance */
    uint8_t threshold; /* threshold for binarization */
    int i; /* Loop variable */

         /* Histogram generation */
    for (i = 0; i < OTSU_GRAYLEVEL; i++) hist[i] = 0;
    for (i = 0; i < im.w*im.h*im.c; ++i) {
      hist[(unsigned char)(im.data[i])]++;
    }
    /* calculation of probability density */
    for (i = 0; i < OTSU_GRAYLEVEL; i++) {
      prob[i] = (float)hist[i] / (im.w * im.h);
    }
    omega[0] = prob[0];
    myu[0] = 0.0;       /* 0.0 times prob[0] equals zero */
    for (i = 1; i < OTSU_GRAYLEVEL; i++) {
      omega[i] = omega[i - 1] + prob[i];
      myu[i] = myu[i - 1] + i * prob[i];
    }

    /* sigma maximization
    sigma stands for inter-class variance
    and determines optimal threshold value */
    threshold = 0;
    max_sigma = 0.0;
    for (i = 0; i < OTSU_GRAYLEVEL - 1; i++) {
      if (omega[i] != 0.0 && omega[i] != 1.0)
        sigma[i] = pow(myu[OTSU_GRAYLEVEL - 1] * omega[i] - myu[i], 2) /
        (omega[i] * (1.0 - omega[i]));
      else
        sigma[i] = 0.0;
      if (sigma[i] > max_sigma) {
        max_sigma = sigma[i];
        threshold = (uint8_t)i;
      }
    }
    /* binarization output */
    for (i = 0; i < im.w*im.h*im.c; ++i) {
      im.data[i] = im.data[i] > threshold ? 255 : 0;
    }
  }
  return;
}




/* Sobel Image Processing */
sod_img sod_sobel_image(sod_img im)
{
  sod_img out;
  int weight[3][3] = { { -1,  0,  1 },
  { -2,  0,  2 },
  { -1,  0,  1 } };
  int pixel_value;
  int min, max;
  int x, y, i, j;  /* Loop variable */

  if (!im.data || im.c != SOD_IMG_GRAYSCALE) {
    /* Only grayscale images */
    return sod_make_empty_image(im.w, im.h, im.c);
  }
  out = sod_make_image(im.w, im.h, im.c);
  if (!out.data) {
    return sod_make_empty_image(im.w, im.h, im.c);
  }
  /* Maximum values calculation after filtering*/
  min = 255 * 4;
  max = -255 * 4;
  for (y = 1; y < im.h - 1; y++) {
    for (x = 1; x < im.w - 1; x++) {
      pixel_value = 0;
      for (j = -1; j <= 1; j++) {
        for (i = -1; i <= 1; i++) {
          pixel_value += weight[j + 1][i + 1] * im.data[(im.w * (y + j)) + x + i];
        }
      }
      if (pixel_value < min) min = pixel_value;
      if (pixel_value > max) max = pixel_value;
    }
  }
  if ((max - min) == 0) {
    return out;
  }
  /* Generation of image2 after linear transformation */
  for (y = 1; y < out.h - 1; y++) {
    for (x = 1; x < out.w - 1; x++) {
      pixel_value = 0;
      for (j = -1; j <= 1; j++) {
        for (i = -1; i <= 1; i++) {
          pixel_value += weight[j + 1][i + 1] * im.data[(im.w * (y + j)) + x + i];
        }
      }
      out.data[out.w * y + x] = (int)((float)(pixel_value - min) / (max - min) * 255);
    }
  }
  return out;
}






/* Gaussian noise reduce */
/* INPUT IMAGE MUST BE GRAYSCALE */

sod_img sod_gaussian_noise_reduce(sod_img grayscale)
{
	int w, h, x, y, max_x, max_y;
	sod_img img_out;
	if (!grayscale.data || grayscale.c != SOD_IMG_GRAYSCALE) {
		return sod_make_empty_image(0, 0, 0);
	}
	w = grayscale.w;
	h = grayscale.h;
	img_out = sod_make_image(w, h, 1);
	if (img_out.data) {
		max_x = w - 2;
		max_y = w * (h - 2);
		for (y = w * 2; y < max_y; y += w) {
			for (x = 2; x < max_x; x++) {
				img_out.data[x + y] = (2 * grayscale.data[x + y - 2 - w - w] +
					4 * grayscale.data[x + y - 1 - w - w] +
					5 * grayscale.data[x + y - w - w] +
					4 * grayscale.data[x + y + 1 - w - w] +
					2 * grayscale.data[x + y + 2 - w - w] +
					4 * grayscale.data[x + y - 2 - w] +
					9 * grayscale.data[x + y - 1 - w] +
					12 * grayscale.data[x + y - w] +
					9 * grayscale.data[x + y + 1 - w] +
					4 * grayscale.data[x + y + 2 - w] +
					5 * grayscale.data[x + y - 2] +
					12 * grayscale.data[x + y - 1] +
					15 * grayscale.data[x + y] +
					12 * grayscale.data[x + y + 1] +
					5 * grayscale.data[x + y + 2] +
					4 * grayscale.data[x + y - 2 + w] +
					9 * grayscale.data[x + y - 1 + w] +
					12 * grayscale.data[x + y + w] +
					9 * grayscale.data[x + y + 1 + w] +
					4 * grayscale.data[x + y + 2 + w] +
					2 * grayscale.data[x + y - 2 + w + w] +
					4 * grayscale.data[x + y - 1 + w + w] +
					5 * grayscale.data[x + y + w + w] +
					4 * grayscale.data[x + y + 1 + w + w] +
					2 * grayscale.data[x + y + 2 + w + w]) / 159;
			}
		}
	}
	return img_out;
}

/* Sobel operator, needed for Canny edge detection */

static void canny_calc_gradient_sobel(sod_img * img_in, int *g, int *dir) {
	int w, h, x, y, max_x, max_y, g_x, g_y;
	float g_div;
	w = img_in->w;
	h = img_in->h;
	max_x = w - 3;
	max_y = w * (h - 3);
	for (y = w * 3; y < max_y; y += w) {
		for (x = 3; x < max_x; x++) {
			g_x = (int)((2 * img_in->data[x + y + 1]
				+ img_in->data[x + y - w + 1]
				+ img_in->data[x + y + w + 1]
				- 2 * img_in->data[x + y - 1]
				- img_in->data[x + y - w - 1]
				- img_in->data[x + y + w - 1]));
			g_y = (int)((2 * img_in->data[x + y - w]
				+ img_in->data[x + y - w + 1]
				+ img_in->data[x + y - w - 1]
				- 2 * img_in->data[x + y + w]
				- img_in->data[x + y + w + 1]
				- img_in->data[x + y + w - 1]));

			g[x + y] = sqrt(g_x * g_x + g_y * g_y);

			if (g_x == 0) {
				dir[x + y] = 2;
			}
			else {
				g_div = g_y / (float)g_x;
				if (g_div < 0) {
					if (g_div < -2.41421356237) {
						dir[x + y] = 0;
					}
					else {
						if (g_div < -0.414213562373) {
							dir[x + y] = 1;
						}
						else {
							dir[x + y] = 2;
						}
					}
				}
				else {
					if (g_div > 2.41421356237) {
						dir[x + y] = 0;
					}
					else {
						if (g_div > 0.414213562373) {
							dir[x + y] = 3;
						}
						else {
							dir[x + y] = 2;
						}
					}
				}
			}
		}
	}
}

/* Non-max suppression */

static void canny_non_max_suppression(sod_img * img, int *g, int *dir) {

	int w, h, x, y, max_x, max_y;
	w = img->w;
	h = img->h;
	max_x = w;
	max_y = w * h;
	for (y = 0; y < max_y; y += w) {
		for (x = 0; x < max_x; x++) {
			switch (dir[x + y]) {
			case 0:
				if (g[x + y] > g[x + y - w] && g[x + y] > g[x + y + w]) {
					if (g[x + y] > 255) {
						img->data[x + y] = 255;
					}
					else {
						img->data[x + y] = g[x + y]; //no cast
					}
				}
				else {
					img->data[x + y] = 0;
				}
				break;
			case 1:
				if (g[x + y] > g[x + y - w - 1] && g[x + y] > g[x + y + w + 1]) {
					if (g[x + y] > 255) {
						img->data[x + y] = 255;
					}
					else {
						img->data[x + y] = g[x + y]; //no cast
					}
				}
				else {
					img->data[x + y] = 0;
				}
				break;
			case 2:
				if (g[x + y] > g[x + y - 1] && g[x + y] > g[x + y + 1]) {
					if (g[x + y] > 255) {
						img->data[x + y] = 255;
					}
					else {
						img->data[x + y] = g[x + y]; // no cast
					}
				}
				else {
					img->data[x + y] = 0;
				}
				break;
			case 3:
				if (g[x + y] > g[x + y - w + 1] && g[x + y] > g[x + y + w - 1]) {
					if (g[x + y] > 255) {
						img->data[x + y] = 255;
					}
					else {
						img->data[x + y] = g[x + y]; //no cast
					}
				}
				else {
					img->data[x + y] = 0;
				}
				break;
			default:
				break;
			}
		}
	}
}

/* Canny estimate threshold, for Canny edge detection */

#define LOW_THRESHOLD_PERCENTAGE 0.8 /* percentage of the high threshold value that the low threshold shall be set at */
#define HIGH_THRESHOLD_PERCENTAGE 0.10 /* percentage of pixels that meet the high threshold - for example 0.15 will ensure that at least 15% of edge pixels are considered to meet the high threshold */

static void canny_estimate_threshold(sod_img * img, int * high, int * low) {

	int i, max, pixels, high_cutoff;
	int histogram[256];
	max = img->w * img->h;
	for (i = 0; i < 256; i++) {
		histogram[i] = 0;
	}
	for (i = 0; i < max; i++) {
		histogram[(int)img->data[i]]++;
	}
	pixels = (max - histogram[0]) * HIGH_THRESHOLD_PERCENTAGE;
	high_cutoff = 0;
	i = 255;
	while (high_cutoff < pixels) {
		high_cutoff += histogram[i];
		i--;
	}
	*high = i;
	i = 1;
	while (histogram[i] == 0) {
		i++;
	}
	*low = (*high + i) * LOW_THRESHOLD_PERCENTAGE;
}

/* Canny trace and canny range, needed for Canny hysteresis */

static int canny_range(sod_img * img, int x, int y)
{
	if ((x < 0) || (x >= img->w)) {
		return(0);
	}
	if ((y < 0) || (y >= img->h)) {
		return(0);
	}
	return(1);
}

static int canny_trace(int x, int y, int low, sod_img * img_in, sod_img * img_out)
{
	int y_off, x_off;
	if (img_out->data[y * img_out->w + x] == 0)
	{
		img_out->data[y * img_out->w + x] = 255;
		for (y_off = -1; y_off <= 1; y_off++)
		{ 
		  for (x_off = -1; x_off <= 1; x_off++)
			{ 
				if (!(y == 0 && x_off == 0) && canny_range(img_in, x + x_off, y + y_off) && (int)(img_in->data[(y + y_off) * img_out->w + x + x_off]) >= low) {
					if (canny_trace(x + x_off, y + y_off, low, img_in, img_out))
					{
						return(1);
					}
				}
			}
		}
		return(1);
	}
	return(0);
}

/* Canny hysteresis, for Canny edge detection */

static void canny_hysteresis(int high, int low, sod_img * img_in, sod_img * img_out)
{
	int x, y, n, max;
	max = img_in->w * img_in->h;
	for (n = 0; n < max; n++) {
		img_out->data[n] = 0;
	}
	for (y = 0; y < img_out->h; y++) {
		for (x = 0; x < img_out->w; x++) {
			if ((int)(img_in->data[y * img_out->w + x]) >= high) {
				canny_trace(x, y, low, img_in, img_out);
			}
		}
	}
}

/* Canny edge detection */

sod_img sod_canny_edge_image(sod_img im, int reduce_noise)
{
	  if (im.data && im.c == SOD_IMG_GRAYSCALE) {
		sod_img sobel, clean;
		int high, low, *g, *dir;
		if (reduce_noise) {
			clean = sod_gaussian_noise_reduce(im);
			if (!clean.data)return sod_make_empty_image(0, 0, 0);
		}
		else {
			clean = im;
		}
		sobel = sod_make_image(im.w, im.h, 1);
//		out = sod_make_image(im.w, im.h, 1);
		g = (int*) ps_malloc(im.w *(im.h + 16) * sizeof(int));
		dir = (int*) ps_malloc(im.w *(im.h + 16) * sizeof(int));
		if (g && dir && sobel.data) {
			canny_calc_gradient_sobel(&clean, &g[im.w], &dir[im.w]);
			canny_non_max_suppression(&sobel, &g[im.w], &dir[im.w]);
//			canny_estimate_threshold(&sobel, &high, &low);
//      Serial.println("Passed Threshold");
//			canny_hysteresis(high, low, &sobel, &out);
//      Serial.println("Passed Hysteresis test");
		}
		if (g)free(g);
		if (dir)free(dir);
		if (reduce_noise)sod_free_image(clean);
//		sod_free_image(sobel);
		return sobel;
	}
	/* Make a grayscale version of your image using sod_grayscale_image() or sod_img_load_grayscale() first */
	return sod_make_empty_image(0, 0, 0);
}

/* Hough lines detect */

sod_pts * sod_hough_lines_detect(sod_img im, int threshold, int *nPts)
{
#define DEG2RAD 0.017453293f
	double center_x, center_y;
	unsigned int *accu;
	int accu_w, accu_h;
	int img_w, img_h;
	double hough_h;
	SySet aLines;
	sod_pts pts;
	int x, y;
	int r, t;

	if (!im.data || im.c != SOD_IMG_GRAYSCALE) {
		/* Require a binary image using sod_canny_edge_image() */
		*nPts = 0;
		return 0;
	}
	SySetInit(&aLines, sizeof(sod_pts));
	img_w = im.w;
	img_h = im.h;

	hough_h = ((sqrt(2.0) * (double)(im.h>im.w ? im.h : im.w)) / 2.0);
	accu_h = hough_h * 2.0; /* -r -> +r */
	accu_w = 180;

	accu = (unsigned int*)ps_calloc(accu_h * accu_w, sizeof(unsigned int));
	if (accu == 0) {
		*nPts = 0;
		return 0;
	}
	center_x = im.w / 2;
	center_y = im.h / 2;
	for (y = 0; y < img_h; y++)
	{
		for (x = 0; x < img_w; x++)
		{
			if (im.data[y * img_w + x] > 250 /*> 250/255.*/)
			{
				for (t = 0; t < 180; t++)
				{
					double ra = (((double)x - center_x) * cos((double)t * DEG2RAD)) + (((double)y - center_y) * sin((double)t * DEG2RAD));
					accu[(int)((round(ra + hough_h) * 180.0)) + t]++;
				}
			}
		}
	}
	if (threshold < 1) threshold = im.w > im.h ? im.w / 3 : im.h / 3;
	for (r = 0; r < accu_h; r++)
	{
		for (t = 0; t < accu_w; t++)
		{
			if ((int)accu[(r*accu_w) + t] >= threshold)
			{
				int ly, lx;
				/* Is this point a local maxima (9x9) */
				int max = (int)accu[(r*accu_w) + t];
				for (ly = -4; ly <= 4; ly++)
				{
					for (lx = -4; lx <= 4; lx++)
					{
						if ((ly + r >= 0 && ly + r < accu_h) && (lx + t >= 0 && lx + t < accu_w))
						{
							if ((int)accu[((r + ly)*accu_w) + (t + lx)] > max)
							{
								max = (int)accu[((r + ly)*accu_w) + (t + lx)];
								ly = lx = 5;
							}
						}
					}
				}
				if (max >(int)accu[(r*accu_w) + t])
					continue;


				int x1, y1, x2, y2;
				x1 = y1 = x2 = y2 = 0;

				if (t >= 45 && t <= 135)
				{
					/*y = (r - x cos(t)) / sin(t)*/
					x1 = 0;
					y1 = ((double)(r - (accu_h / 2)) - ((x1 - (img_w / 2)) * cos(t * DEG2RAD))) / sin(t * DEG2RAD) + (img_h / 2);
					x2 = img_w - 0;
					y2 = ((double)(r - (accu_h / 2)) - ((x2 - (img_w / 2)) * cos(t * DEG2RAD))) / sin(t * DEG2RAD) + (img_h / 2);
				}
				else
				{
					/* x = (r - y sin(t)) / cos(t);*/
					y1 = 0;
					x1 = ((double)(r - (accu_h / 2)) - ((y1 - (img_h / 2)) * sin(t * DEG2RAD))) / cos(t * DEG2RAD) + (img_w / 2);
					y2 = img_h - 0;
					x2 = ((double)(r - (accu_h / 2)) - ((y2 - (img_h / 2)) * sin(t * DEG2RAD))) / cos(t * DEG2RAD) + (img_w / 2);
				}
				pts.x = x1; pts.y = y1;
				SySetPut(&aLines, &pts);
				pts.x = x2; pts.y = y2;
				SySetPut(&aLines, &pts);
			}
		}
	}
	free(accu);
	*nPts = (int)SySetUsed(&aLines);
	return (sod_pts *)SySetBasePtr(&aLines);
}

/* Connectivity detection, needed for Hilditch thinning function below */

static int hilditch_func_nc8(int *b)
{
	int n_odd[4] = { 1, 3, 5, 7 };  /* odd-number neighbors */
	int i, j, sum, d[10];           /* control variable */

	for (i = 0; i <= 9; i++) {
		j = i;
		if (i == 9) j = 1;
		if (abs(*(b + j)) == 1) {
			d[i] = 1;
		}
		else {
			d[i] = 0;
		}
	}
	sum = 0;
	for (i = 0; i < 4; i++) {
		j = n_odd[i];
		sum = sum + d[j] - d[j] * d[j + 1] * d[j + 2];
	}
	return sum;
}

/* Hilditch thinning */
/* Input MUST BE A BINARY IMAGE! */

sod_img sod_hilditch_thin_image(sod_img im)
{
	/* thinning of binary image via Hilditch's algorithm */
	int offset[9][2] = { { 0,0 },{ 1,0 },{ 1,-1 },{ 0,-1 },{ -1,-1 },
	{ -1,0 },{ -1,1 },{ 0,1 },{ 1,1 } }; /* offsets for neighbors */
	int n_odd[4] = { 1, 3, 5, 7 };      /* odd-number neighbors */
	int px, py;                         /* X/Y coordinates  */
	int b[9];                           /* gray levels for 9 neighbors */
	int condition[6];                   /* valid for conditions 1-6 */
	int counter;                        /* number of changing points  */
	int i, x, y, copy, sum;             /* control variable          */
	sod_img out;

	if (im.data == 0 || im.c != SOD_IMG_GRAYSCALE) {
		/* Must be a binary image (canny_edge, thresholding, etc..) */
		return sod_make_empty_image(0, 0, 0);
	}
	/* initialization of output */
	out = sod_copy_image(im);
	if (out.data == 0) {
		return sod_make_empty_image(0, 0, 0);
	}
	/* processing starts */
	do {
		counter = 0;
		for (y = 0; y < im.h; y++) {
			for (x = 0; x < im.w; x++) {
				/* substitution of 9-neighbor gray values */
				for (i = 0; i < 9; i++) {
					b[i] = 0;
					px = x + offset[i][0];
					py = y + offset[i][1];
					if (px >= 0 && px < im.w &&
						py >= 0 && py < im.h) {
						if (out.data[py * im.w + px] == 0) {
							b[i] = 1;
						}
						else if (out.data[py * im.w + px] == 2 /* Temp marker */) {
							b[i] = -1;
						}
					}
				}
				for (i = 0; i < 6; i++) {
					condition[i] = 0;
				}

				/* condition 1: figure point */
				if (b[0] == 1) condition[0] = 1;

				/* condition 2: boundary point */
				sum = 0;
				for (i = 0; i < 4; i++) {
					sum = sum + 1 - abs(b[n_odd[i]]);
				}
				if (sum >= 1) condition[1] = 1;

				/* condition 3: endpoint conservation */
				sum = 0;
				for (i = 1; i <= 8; i++) {
					sum = sum + abs(b[i]);
				}
				if (sum >= 2) condition[2] = 1;

				/* condition 4: isolated point conservation */
				sum = 0;
				for (i = 1; i <= 8; i++) {
					if (b[i] == 1) sum++;
				}
				if (sum >= 1) condition[3] = 1;

				/* condition 5: connectivity conservation */
				if (hilditch_func_nc8(b) == 1) condition[4] = 1;

				/* condition 6: one-side elimination for line-width of two */
				sum = 0;
				for (i = 1; i <= 8; i++) {
					if (b[i] != -1) {
						sum++;
					}
					else {
						copy = b[i];
						b[i] = 0;
						if (hilditch_func_nc8(b) == 1) sum++;
						b[i] = copy;
					}
				}
				if (sum == 8) condition[5] = 1;

				/* final decision */
				if (condition[0] && condition[1] && condition[2] &&
					condition[3] && condition[4] && condition[5]) {
					out.data[y * im.w + x] = 2; /* Temp */
					counter++;
				}
			} /* end of x */
		} /* end of y */

		if (counter != 0) {
			for (y = 0; y < im.h; y++) {
				for (x = 0; x < im.w; x++) {
					if (out.data[y * im.w + x] == 2) out.data[y *im.w + x] = 255;
				}
			}
		}
	} while (counter != 0);

	return out;
}
