#ifndef IMAGE_OBJECT_H
#define IMAGE_OBJECT_H

#include <iostream>
#include <utility>

#include <QImage>
#include <QVector>
#include <QRgb>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

#include "array.h"

class ImageObject
{
public:
	ImageObject(int _width, int _height, QVector<QRgb> _colortable)
	{
		width = _width;
		height = _height;
		colortable = _colortable;

		qindeximg = QImage(width, height, QImage::Format_Indexed8);
		qindeximg.setColorCount(256);
		qindeximg.setColorTable(colortable);

		qrgbimg = QImage(width, height, QImage::Format_RGB888);
		memset(qrgbimg.bits(), 0, qrgbimg.byteCount()); 

		arr = np::Uint8Array2(qindeximg.bits(), width, height);
		memset(arr.raw_ptr(), 0, sizeof(uint8_t) * arr.length());
	}

	~ImageObject()
	{

	}

public:
	int getWidth() const { return width; }
	int getHeight() const { return height; }
        const QVector<QRgb> getColorTable() const { return colortable; }

	void convertRgb()
	{
		qrgbimg = QImage(width, height, QImage::Format_RGB888);
		np::Uint8Array2 rgbarr(qrgbimg.bits(), 3 * width, height);

		tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)height),
			[&](const tbb::blocked_range<size_t>& r) {
			for (size_t i = r.begin(); i != r.end(); ++i)
			{
				for (int j = 0; j < width; j++)
				{
					QRgb val = colortable.at(arr(j, (int)i));
					rgbarr(3 * j + 0, (int)i) = qRed(val);
					rgbarr(3 * j + 1, (int)i) = qGreen(val);
					rgbarr(3 * j + 2, (int)i) = qBlue(val);
				}
			}
		});
	}

	void convertScaledRgb()
	{
		qrgbimg = std::move(qindeximg.scaled(4 * width, height).convertToFormat(QImage::Format_RGB888));
	}

	void convertNonScaledRgb()
	{
		qrgbimg = std::move(qindeximg.convertToFormat(QImage::Format_RGB888));
	}

	void scaled4()
	{
		qindeximg = std::move(qindeximg.scaled(4 * width, height));
	}

	void scaledRgb4()
	{
		qrgbimg = std::move(qrgbimg.scaled(4 * width, height));
	}

	void setRgbChannelData(uchar* data, int ch)
	{
		for (int i = 0; i < height; i++)
		{
			int hOffset = 3 * width * i;
			for (int j = 0; j < width; j++)
			{
				*(qrgbimg.bits() + (3 * j + ch) + hOffset) = *(data + j + hOffset / 3);
			}
		}
	}

public:
	np::Uint8Array2 arr;
	QImage qindeximg;
	QImage qrgbimg;

private:
	int width;
	int height;
	QVector<QRgb> colortable;
};


#endif // IMAGE_OBJECT_H
