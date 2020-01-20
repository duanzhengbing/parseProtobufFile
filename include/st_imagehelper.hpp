/*
 * @file   st_imagehelper.hpp
 * @author SenseTime Group Limited
 * @brief  Image IO helper for SenseTime C API.
 *
 * Copyright (c) 2014-2015, SenseTime Group Limited. All Rights Reserved.
 */

#ifndef IMAGEHELPER_HPP_8FVB3L0U
#define IMAGEHELPER_HPP_8FVB3L0U

#include <cstdlib>
#include <cmath>
#include <cstring>

#ifndef DISABLE_TIMING
#include <ctime>
#include <cstdio>

#ifdef _MSC_VER
#define __TIC__() double __timing_start = clock()
#define __TOC__()                                                 \
	do {                                                      \
		double __timing_end = clock();            \
		fprintf(stdout, "TIME(ms): %lf\n",                \
			(__timing_end - __timing_start)   \
				/ CLOCKS_PER_SEC * 1000);         \
	} while (0)
#else
#include <unistd.h>
#include <sys/time.h>

#define __TIC__()                                    \
	struct timeval __timing_start, __timing_end; \
	gettimeofday(&__timing_start, NULL);

#define __TOC__()                                                        \
	do {                                                             \
		gettimeofday(&__timing_end, NULL);                       \
		double __timing_gap = (__timing_end.tv_sec -     \
					       __timing_start.tv_sec) *  \
					      1000.0 +                     \
				      (__timing_end.tv_usec -    \
					       __timing_start.tv_usec) / \
					      1000.0;                    \
		fprintf(stdout, "TIME(ms): %lf\n", __timing_gap);        \
	} while (0)

#endif

#else
#define __TIC__()
#define __TOC__()
#endif

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

#if (defined __ANDROID__) \
	|| (defined TARGET_IPHONE_SIMULATOR) \
	|| (defined TARGET_OS_IPHONE)
#define __ST_HELPER_MOBILE 1
#endif

#ifdef _MSC_VER
#ifndef USE_GDIPLUS
#define USE_GDIPLUS
#endif
#elif __ST_HELPER_MOBILE
#ifndef USE_STB
#define USE_STB
#endif
//#elif USE_STB
// nothing
//#else
//#ifndef USE_OPENCV
//#define USE_OPENCV
//#endif
#endif

#ifdef USE_OPENCV
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#endif

#ifdef USE_GDIPLUS
#include <Windows.h>
#pragma comment(lib, "gdiplus.lib")
#include <gdiplus.h>
#endif

#ifdef USE_STB
#include "codec/stb_image.h"
#include "codec/stb_image_write.h"
#include "codec/jpge.h"
#endif

namespace sdktest {

class Image {
	public:
	enum ImageType { Image_RGBA, Image_BGR , Image_Gray};
	int cols;
	int rows;
	int stride;
	unsigned char *data;
	ImageType type;
};

class ImageF {
	public:
	// Note that ImageF always has BGR order
	enum ImageType { Image_BGR, Image_Gray };
	int cols;
	int rows;
	int stride;
	float *data;
	ImageType type;
};

enum ImageFormat {
	ImageFormatUnknown = 0,
	ImageFormatPNG,
	ImageFormatJPEG,
};

// on mobile platforms (e.g. iOS and Android), the native byte order
// is RGBA. On both OpenCV and Windows (Gdiplus), it is BGRA.
#if __ST_HELPER_MOBILE
#define __ST_IMAGE_RGBA 1
#else
#define __ST_IMAGE_BGRA 1
#endif

#ifdef __ST_IMAGE_RGBA
struct _ColorQuad {
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
};
#else
struct _ColorQuad {
	unsigned char b;
	unsigned char g;
	unsigned char r;
	unsigned char a;
};
#endif

struct ColorQuad : public _ColorQuad {
	ColorQuad(int _b, int _g, int _r, int _a = 255) {
		b = _b; g = _g; r = _r; a = _a;
	}
};

template <typename T>
static inline T clip(T x, T a, T b) {
	if (x < a) return a;
	if (x > b) return b;
	return x;
}

class ImageHelper {
	public:
	virtual Image *LoadRGBAImage(const char *fn) = 0;
	virtual Image *LoadGrayImage(const char *fn) = 0;
	virtual Image *LoadBGRImage(const char *fn) = 0;
	virtual bool SaveImage(const char *fn, const Image *image) = 0;
	virtual Image *CreateImage(int width, int height,
				   Image::ImageType type) {
		Image *img = new Image();
		size_t t = 1;
		if( type == Image::Image_BGR)
			t = 3;
		else if(type == Image::Image_RGBA)
			t = 4;
		img->type = type;
		img->stride = width * t;
		img->rows = height;
		img->cols = width;
		size_t len = img->stride * img->rows;
		img->data = new unsigned char[len];

		return img;
	}
	virtual ImageF *CreateImageF(int width, int height,
				   ImageF::ImageType type) {
		ImageF *img = new ImageF();
		size_t t = type == ImageF::Image_Gray ? 1 : 3;
		img->type = type;
		img->stride = width * sizeof(float) * t;
		img->rows = height;
		img->cols = width;
		img->data = new float[img->rows * img->cols * t];

		return img;
	}

	static inline void rotate(float theta, int x_in, int y_in, int c_x,
			int c_y, int *x_out, int *y_out) {
		float f[3][3];
		float th;
		int x = x_in - c_x;
		int y = y_in - c_y;
		th = -theta / 180.0f * 3.1415927f;
		f[0][0] = cosf(th);
		f[0][1] = sinf(th);
		f[0][2] = 0.0;
		f[1][0] = -sinf(th);
		f[1][1] = cosf(th);
		f[2][0] = 0.0;
		f[2][1] = 0.0;
		f[2][2] = 1.0;
		int xx = x * f[0][0] + y * f[1][0] + f[2][0];
		int yy = x * f[0][1] + y * f[1][1] + f[2][1];
		*x_out = xx + c_x;
		*y_out = yy + c_y;
	}

	static inline ImageFormat ImageFormatFromFilename(const char *fn) {
		size_t fn_len = strlen(fn);
		if (fn_len >= 4
				&& fn[fn_len - 4] == '.'
				&& fn[fn_len - 3] == 'p'
				&& fn[fn_len - 2] == 'n'
				&& fn[fn_len - 1] == 'g'
		   ) {
			return ImageFormatPNG;
		} else if (fn_len >= 4
				&& fn[fn_len - 4] == '.'
				&& fn[fn_len - 3] == 'j'
				&& fn[fn_len - 2] == 'p'
				&& fn[fn_len - 1] == 'g'
			  ) {
			return ImageFormatJPEG;
		} else {
			return ImageFormatUnknown;
		}
	}

	inline void SetPixel(Image *image, int x, int y, const _ColorQuad &color) {
		if (!image) return;
		if (x < 0 || x >= image->cols
			|| y < 0 || y >= image->rows)
			return;
		if (image->type == Image::Image_Gray) {
			image->data[y * image->stride + x] = color.a;
		} else if(image->type == Image::Image_BGR) {
			int offset = y * image->stride + x * 3;
			unsigned char * p = image->data + offset;
			*p++ = color.b;
			*p++ = color.g;
			*p++ = color.r;
		} else {
			/* green */
			int offset = y * image->stride + x * 4;
			*(reinterpret_cast<unsigned int*>(&image->data[offset])) =
				*(reinterpret_cast<const unsigned int*>(&color));
		}
	}

	inline _ColorQuad GetPixel(Image *image, int x, int y) {
		_ColorQuad c;
		c.b = c.g = c.r = c.a = 0;
		if (!image || x < 0 || x >= image->cols
			|| y < 0 || y >= image->rows) {
			return c;
		}
		if (image->type == Image::Image_Gray) {
			c.a = image->data[image->stride * y + x];
		} else if (image->type == Image::Image_BGR) {
			int offset = y * image->stride + x * 3;
			c.b = image->data[offset];
			c.g = image->data[offset + 1];
			c.r = image->data[offset + 2];
		} else {
			int offset = y * image->stride + x * 4;
			c = *(reinterpret_cast<_ColorQuad*>(&image->data[offset]));
		}
		return c;
	}

	void DrawPoint(Image *image, int x, int y, int stroke_width,
		const ColorQuad &color = ColorQuad(0, 255, 0)) {
		for (int h = x - stroke_width; h <= x + stroke_width; h++) {
			for (int v = y - stroke_width; v <= y + stroke_width;
			     v++) {
				int dx = h - x, dy = v - y;
				if (dx * dx + dy * dy <=
				    stroke_width * stroke_width)
					SetPixel(image, h, v, color);
			}
		}
	}
	void DrawRect(Image *image, int top, int left, int right, int bottom,
		      int stroke_width,
		      const ColorQuad &color = ColorQuad(0, 255, 0)) {
		for (int i = top; i <= bottom; i++) {
			for (int sw = 0; sw < stroke_width; sw++) {
				int u = left + sw;
				int d = right - sw;
				SetPixel(image, u, i, color);
				SetPixel(image, d, i, color);
			}
		}

		for (int i = left; i <= right; i++) {
			for (int sw = 0; sw < stroke_width; sw++) {
				SetPixel(image, i, top + sw, color);
				SetPixel(image, i, bottom - sw, color);
			}
		}
	}
	void DrawRotatedRect(Image *image, int top, int left, int right, int bottom, int roll,
		      int stroke_width,
		      const ColorQuad &color = ColorQuad(0, 255, 0)) {
		int x, y, xx, yy;
		for (int i = left; i < right; i++) {
			if (((i - left) < (right - left) / 4) || ((i - left) > (right - left) * 3/ 4))
			{
				rotate(roll, i, top, (left + right) / 2, (top + bottom)/2, &x, &y);
				DrawPoint(image, x, y, stroke_width, color);
			}
		}

		for (int i = left; i < right; i++) {
			if (((i - left) < (right - left) / 4) || ((i - left) > (right - left) * 3/ 4))
			{
				rotate(roll, i, bottom, (left + right) / 2, (top + bottom)/2, &x, &y);
				DrawPoint(image, x, y, stroke_width, color);
			}
		}

		for (int i = top; i < bottom; i++) {
			if (((i - top) < (bottom - top) / 4) || ((i - top) > (bottom - top) * 3/ 4))
			{
				rotate(roll, left, i, (left + right) / 2, (top + bottom)/2, &x, &y);
				DrawPoint(image, x, y, stroke_width, color);
			}
		}
		for (int i = top; i < bottom; i++) {
			if (((i - top) < (bottom - top) / 4) || ((i - top) > (bottom - top) * 3/ 4))
			{
				rotate(roll, right, i, (left + right) / 2, (top + bottom)/2, &x, &y);
				DrawPoint(image, x, y, stroke_width, color);
			}
		}
	}
	void DrawMask(Image *image, Image *mask) {
		if (!((image->type == Image::Image_RGBA || image->type == Image::Image_BGR)
		    && mask->type == Image::Image_Gray))
			return;
		for (int r = 0; r < image->rows; r++)
			for (int c = 0; c < image->cols; c++) {
				if (mask->data[mask->stride * r + c])
					continue;
				SetPixel(image, c, r, ColorQuad(0, 0, 0));
			}
	}

	ImageF *ToFloatImage(const Image *image) {
		if (image->type == Image::Image_Gray) {
			ImageF *img = CreateImageF(image->cols, image->rows, ImageF::Image_Gray);
			for (int i = 0; i < image->rows; i++) {
				const unsigned char *ps = image->data + i * image->stride;
				float *pd = img->data + i * image->cols;
				for (int j = 0; j < image->cols; j++)
					pd[j] = ps[j] / 255.0f;
			}
			return img;
		} else {
			ImageF *img = CreateImageF(image->cols, image->rows, ImageF::Image_BGR);
			for (int i = 0; i < image->rows; i++) {
				const unsigned char *ps = image->data + i * image->stride;
				float *pd = img->data + i * image->cols * 3;
				for (int j = 0; j < image->cols; j++) {
#ifdef __ST_IMAGE_BGRA
					pd[3 * j + 0] = ps[4 * j + 0] / 255.0f;
					pd[3 * j + 1] = ps[4 * j + 1] / 255.0f;
					pd[3 * j + 2] = ps[4 * j + 2] / 255.0f;
#else
					pd[3 * j + 0] = ps[4 * j + 2] / 255.0f;
					pd[3 * j + 1] = ps[4 * j + 1] / 255.0f;
					pd[3 * j + 2] = ps[4 * j + 0] / 255.0f;
#endif
				}
			}
			return img;
		}
	}

	Image *FromFloatImage(const ImageF *image) {
		if (image->type == ImageF::Image_Gray) {
			Image *img = CreateImage(image->cols, image->rows, Image::Image_Gray);
			for (int i = 0; i < image->rows; i++) {
				const float *ps = image->data + i * image->cols;
				unsigned char *pd = img->data + i * img->stride;
				for (int j = 0; j < image->cols; j++)
					pd[j] = clip(ps[j] * 255.0f, 0.0f, 255.0f);
			}
			return img;
		} else {
			Image *img = CreateImage(image->cols, image->rows, Image::Image_RGBA);
			for (int i = 0; i < image->rows; i++) {
				const float *ps = image->data + i * image->cols * 3;
				unsigned char *pd = img->data + i * img->stride;
				for (int j = 0; j < image->cols; j++) {
#ifdef __ST_IMAGE_BGRA
					pd[4 * j + 0] = clip(ps[3 * j + 0] * 255.0f, 0.0f, 255.0f);
					pd[4 * j + 1] = clip(ps[3 * j + 1] * 255.0f, 0.0f, 255.0f);
					pd[4 * j + 2] = clip(ps[3 * j + 2] * 255.0f, 0.0f, 255.0f);
#else
					pd[4 * j + 0] = clip(ps[3 * j + 2] * 255.0f, 0.0f, 255.0f);
					pd[4 * j + 1] = clip(ps[3 * j + 1] * 255.0f, 0.0f, 255.0f);
					pd[4 * j + 2] = clip(ps[3 * j + 0] * 255.0f, 0.0f, 255.0f);
#endif
					pd[4 * j + 3] = 255;
				}
			}
			return img;
		}
	}


	void FreeImage(Image *image) {
		if (!image) return;
		if (image->data) delete[] image->data;
		delete image;
	}

	void FreeImageF(ImageF *image) {
		if (!image) return;
		if (image->data) delete[] image->data;
		delete image;
	}

	virtual ~ImageHelper() {}
};

#ifdef USE_STB
class ImageHelperSTB : public ImageHelper {
	private:
	static void swap_red_blue(unsigned char *data, int w, int h, int nchannel) {
		// convert to BGRA
		for (int i = 0; i < h; i++) {
			stbi_uc *p = w * i * nchannel + data;
			for (int j = 0; j < w; j++) {
				unsigned char t = p[nchannel * j + 0];
				p[nchannel * j + 0] = p[nchannel * j + 2];
				p[nchannel * j + 2] = t;
			}
		}
	}

	static bool save_png(const char *fn, const Image *image) {
		if (image->type == Image::Image_Gray) {
			int ret = stbi_write_png(fn, image->cols, image->rows, 1,
					image->data, image->stride);
			return ret != 0;
		} else if (image->type == Image::Image_BGR) {
			unsigned char *data = image->data;
			swap_red_blue(data, image->cols, image->rows, 3);
			int ret = stbi_write_png(fn, image->cols, image->rows, 3,
					data, image->stride);
			return ret != 0;
		} else {
#ifdef __ST_IMAGE_BGRA
			size_t len = image->rows * image->stride;
			unsigned char *data = new unsigned char[len];
			memcpy(data, image->data, len);
			// convert to BGRA
			swap_red_blue(data, image->cols, image->rows, 4);
#else
			unsigned char *data = image->data;
#endif
			int ret = stbi_write_png(fn, image->cols, image->rows, 4,
					data, image->stride);
#ifdef __ST_IMAGE_BGRA
			delete [] data;
#endif
			return ret != 0;
		}

	}

	static bool save_jpeg(const char *fn, const Image *image) {
		if (image->type == Image::Image_Gray) {
			return jpge::compress_image_to_jpeg_file(fn,
					image->cols, image->rows, 1, image->data);
		} else if(image->type == Image::Image_BGR) {
			swap_red_blue(image->data, image->cols, image->rows, 3);
			return jpge::compress_image_to_jpeg_file(fn,
					image->cols, image->rows, 3, image->data);
		} else {
			unsigned char *data = new unsigned char[image->rows * image->cols * 3];
			for (int y = 0; y < image->rows; y++) {
				unsigned char *pd = data + y * image->cols * 3;
				unsigned char *ps = image->data + image->stride * y;
				for (int x = 0; x < image->cols; x++) {
#ifdef __ST_IMAGE_BGRA
					pd[3*x+0] = ps[4*x+2];
					pd[3*x+1] = ps[4*x+1];
					pd[3*x+2] = ps[4*x+0];
#else
					pd[3*x+0] = ps[4*x+0];
					pd[3*x+1] = ps[4*x+1];
					pd[3*x+2] = ps[4*x+2];
#endif
				}
			}
			bool r = jpge::compress_image_to_jpeg_file(fn,
					image->cols, image->rows, 3, data);
			delete [] data;
			return r;
		}
	}

	public:
	virtual Image *LoadRGBAImage(const char *fn) {
		int w, h, comp;
		stbi_uc *data = stbi_load(fn, &w, &h, &comp, STBI_rgb_alpha);
		if (!data)
			return 0;
#ifdef __ST_IMAGE_BGRA
		// convert to BGRA
		swap_red_blue(data, w, h, 4);
#endif

		Image *img =
			CreateImage(w, h, Image::Image_RGBA);
		size_t len = w * h * 4;
		memcpy(img->data, data, len);

		stbi_image_free(data);

		return img;
	}

	virtual Image *LoadBGRImage(const char *fn) {
		int w, h, comp;
		stbi_uc *data = stbi_load(fn, &w, &h, &comp, STBI_rgb);
		if (!data)
			return 0;
		swap_red_blue(data, w, h, 3);
		Image *img =
			CreateImage(w, h, Image::Image_BGR);
		size_t len = w * h * 3;
		memcpy(img->data, data, len);

		stbi_image_free(data);

		return img;
	}

	virtual Image *LoadGrayImage(const char *fn) {
		int w, h, comp;
		stbi_uc *data = stbi_load(fn, &w, &h, &comp, STBI_grey);
		if (!data)
			return 0;

		Image *img =
			CreateImage(w, h, Image::Image_Gray);
		size_t len = w * h;
		memcpy(img->data, data, len);

		stbi_image_free(data);

		return img;
	}

	virtual bool SaveImage(const char *fn, const Image *image) {
		if (!image) return false;
		size_t fn_len = strlen(fn);
		ImageFormat f = ImageFormatFromFilename(fn);
		if (f == ImageFormatPNG) {
			return save_png(fn, image);
		} else if (f == ImageFormatJPEG) {
			return save_jpeg(fn, image);
		} else {
			fprintf(stderr, "warning: STB image backend only supporting .png & .jpg saving\n");
			return false;
		}
	}
};

#ifndef ImageHelperBackend
#define ImageHelperBackend ImageHelperSTB
#endif
#endif

#ifdef USE_OPENCV

class ImageHelperOpenCV : public ImageHelper {
	public:
	virtual Image *LoadRGBAImage(const char *fn) {
		cv::Mat _img = cv::imread(fn);
		if (!_img.data) return 0;
#ifdef __ST_IMAGE_RGBA
		cv::cvtColor(_img, _img, CV_BGR2RGBA);
#else
		cv::cvtColor(_img, _img, CV_BGR2BGRA);
#endif

		Image *img =
			CreateImage(_img.cols, _img.rows, Image::Image_RGBA);
		size_t len = _img.step * _img.rows;
		memcpy(img->data, _img.data, len);

		return img;
	}

	virtual Image *LoadGrayImage(const char *fn) {
		cv::Mat _img = cv::imread(fn);
		if (!_img.data) return 0;
		cv::cvtColor(_img, _img, CV_BGR2GRAY);

		Image *img =
			CreateImage(_img.cols, _img.rows, Image::Image_Gray);
		size_t len = _img.step * _img.rows;

		memcpy(img->data, _img.data, len);

		return img;
	}

	virtual Image *LoadBGRImage(const char *fn) {
		cv::Mat _img = cv::imread(fn);
		if (!_img.data) return 0;
		Image *img =
			CreateImage(_img.cols, _img.rows, Image::Image_BGR);
		uchar* pd = img->data;
		uchar* ps = _img.data;
		memcpy(pd, ps, _img.step*_img.rows);
		return img;
	}
	virtual bool SaveImage(const char *fn, const Image *image) {
		if (!image) return false;
		int type = CV_8UC1;
		if(image->type == Image::Image_BGR)
			type = CV_8UC3;
		else if(image->type == Image::Image_RGBA)
			type = CV_8UC4;
		cv::Mat mat(image->rows, image->cols,
			    type,
				image->data);
#ifdef __ST_IMAGE_RGBA
		if (image->type == Image::Image_RGRA)
			cv::cvtColor(mat, mat, CV_RGBA2BGRA);
#endif
		return cv::imwrite(fn, mat);
	}
};

#ifndef ImageHelperBackend
#define ImageHelperBackend ImageHelperOpenCV
#endif
#endif

/* GDIPLUS store pixel in BGRA format (same as opencv) */
#ifdef USE_GDIPLUS
class ImageHelperGdiPlus : public ImageHelper {
	private:
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	int GetEncoderClsid(const WCHAR *format, CLSID *pClsid) {
		UINT num = 0;   // number of image encoders
		UINT size = 0;  // size of the image encoder array in bytes

		Gdiplus::ImageCodecInfo *pImageCodecInfo = NULL;

		Gdiplus::GetImageEncodersSize(&num, &size);
		if (size == 0) return -1;  // Failure

		pImageCodecInfo = (Gdiplus::ImageCodecInfo *)(malloc(size));
		if (pImageCodecInfo == NULL) return -1;  // Failure

		GetImageEncoders(num, size, pImageCodecInfo);

		for (UINT j = 0; j < num; ++j) {
			if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
				*pClsid = pImageCodecInfo[j].Clsid;
				free(pImageCodecInfo);
				return j;  // Success
			}
		}

		free(pImageCodecInfo);
		return -1;  // Failure
	}

	public:
	ImageHelperGdiPlus() {
		Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput,
					NULL);
	}
	~ImageHelperGdiPlus() {
		Gdiplus::GdiplusShutdown(gdiplusToken);
	}

	virtual Image *LoadRGBAImage(const char *fn) {
		WCHAR path[2048];
		size_t wchar_len = 0;
		mbstowcs_s(&wchar_len, path, 2048, fn, _TRUNCATE);
		Gdiplus::Bitmap *_img = Gdiplus::Bitmap::FromFile(path);
		if (!_img) return 0;

		Image *img = CreateImage(_img->GetWidth(), _img->GetHeight(),
					 Image::Image_RGBA);
		Gdiplus::BitmapData _data;
		Gdiplus::Rect _rect(0, 0, _img->GetWidth(), _img->GetHeight());
		if (Gdiplus::Ok ==
		    _img->LockBits(&_rect, Gdiplus::ImageLockModeRead,
				   PixelFormat32bppARGB, &_data)) {
			BYTE *p = reinterpret_cast<BYTE *>(_data.Scan0);
			BYTE *pd = img->data;
			memcpy(pd, p, 4 * _img->GetWidth() * _img->GetHeight());
			_img->UnlockBits(&_data);
		} else {
			FreeImage(img);
			delete _img;
			return 0;
		}
		delete _img;
		return img;
	}

	virtual Image *LoadBGRImage(const char *fn) {
		WCHAR path[2048];
		size_t wchar_len = 0;
		mbstowcs_s(&wchar_len, path, fn, 2048);
		Gdiplus::Bitmap *_img = Gdiplus::Bitmap::FromFile(path);
		if (!_img) return 0;
		Image *img = CreateImage(_img->GetWidth(), _img->GetHeight(),
					 Image::Image_BGR);
		Gdiplus::BitmapData _data;
		Gdiplus::Rect _rect(0, 0, _img->GetWidth(), _img->GetHeight());
		if (Gdiplus::Ok == _img->LockBits(&_rect, Gdiplus::ImageLockModeRead,
						  PixelFormat24bppRGB, &_data)) {
			BYTE *p = reinterpret_cast<BYTE *>(_data.Scan0);
			BYTE *pd = img->data;
			memcpy(pd, p, 3 * _img->GetWidth() * _img->GetHeight());
			for (int i = 0; i < _img->GetHeight(); i++) {
				memcpy(pd, p, img->stride);
				p += _data.Stride;
				pd += img->stride;
			}
			_img->UnlockBits(&_data);
		} else {
			FreeImage(img);
			delete _img;
			return NULL;
		}
		delete _img;
		return img;
	}
	virtual Image *LoadGrayImage(const char *fn) {
		using namespace Gdiplus;
		WCHAR path[2048];
		size_t wchar_len = 0;
		mbstowcs_s(&wchar_len, path, 2048, fn, _TRUNCATE);
		Bitmap *image = Gdiplus::Bitmap::FromFile(path);
		if (!image) {
			// *img = NULL;
			return NULL;
		}
		BitmapData _data;
		Rect _rect(0, 0, image->GetWidth(), image->GetHeight());
		Image *img = CreateImage(image->GetWidth(), image->GetHeight(),
					 Image::Image_Gray);
		BYTE *gray = img->data;
#define GREY(r, g, b) ((((r)*77) + ((g)*151) + ((b)*28)) >> 8)
		if (Ok ==
		    image->LockBits(&_rect,
				    ImageLockModeRead,
				    PixelFormat32bppARGB, &_data)) {
			BYTE *p = reinterpret_cast<BYTE *>(_data.Scan0);
			BYTE *pd = gray;
			for (UINT i = 0; i < _data.Height; i++) {
				for (UINT j = 0; j < _data.Width; j++) {
					*pd++ = GREY(p[4 * j + 2], p[4 * j + 1],
						     p[4 * j + 0]);
				}
				p += _data.Stride;
			}
			image->UnlockBits(&_data);
		} else {
			FreeImage(img);
			delete image;
			return NULL;
		}
#undef GREY
		delete image;
		return img;
	}

	virtual bool SaveImage(const char *fn, const Image *image) {
		using namespace Gdiplus;
		ImageFormat f = ImageFormatFromFilename(fn);
		if (!image) return false;
		WCHAR path[2048];
		size_t wchar_len = 0;
		mbstowcs_s(&wchar_len, path, 2048, fn, _TRUNCATE);
		CLSID pngClsid;
		// XXX png support
		if (f == ImageFormatJPEG) {
			GetEncoderClsid(L"image/jpeg", &pngClsid);
		} else if (f == ImageFormatPNG) {
			GetEncoderClsid(L"image/png", &pngClsid);
		} else {
			fprintf(stderr, "warning: Gdiplus image backend only supporting .png & .jpg saving\n");
			return false;
		}

		PixelFormat pf = image->type == PixelFormat8bppIndexed;
		if (image->type == Image::Image_RGBA)
			pf = PixelFormat32bppARGB;
		if(image->type == Image::Image_BGR)
			pf = PixelFormat24bppRGB;

		ColorPalette *pal = NULL;
		Bitmap _img(image->cols, image->rows, pf);
		if (image->type == Image::Image_Gray) {
			pal = (ColorPalette*)malloc(sizeof(ColorPalette)+256*sizeof(ARGB));
			pal->Count = 256;
			pal->Flags = 2;
			for (int i = 0; i < 256; i++)
				pal->Entries[i] = Gdiplus::Color::MakeARGB(255,i,i,i);
			_img.SetPalette(pal);
		}
		BitmapData _data;
		Rect _rect(0, 0, _img.GetWidth(), _img.GetHeight());
		if (Gdiplus::Ok ==
		    _img.LockBits(&_rect, Gdiplus::ImageLockModeWrite,
				  pf, &_data)) {
			BYTE *p = reinterpret_cast<BYTE *>(_data.Scan0);
			BYTE *ps = image->data;
			for (UINT i = 0; i < _data.Height; i++) {
				memcpy(p, ps, image->stride);
				p += _data.Stride;
				ps += image->stride;
			}
			_img.UnlockBits(&_data);
			bool r = _img.Save(path, &pngClsid) == Gdiplus::Ok;
			if(pal) free(pal);
			return r;
		} else {
			if(pal) free(pal);
			return false;
		}
	}
};

#ifndef ImageHelperBackend
#define ImageHelperBackend ImageHelperGdiPlus
#endif
#endif
}  // namespace sdktest

#endif /* end of include guard: IMAGEHELPER_HPP_8FVB3L0U */
