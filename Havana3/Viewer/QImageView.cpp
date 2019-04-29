

#include "QImageView.h"
#include <ipps.h>


ColorTable::ColorTable()
{
	// Color table list	
	m_cNameVector.push_back("gray"); // 0
	m_cNameVector.push_back("invgray");
	m_cNameVector.push_back("sepia");
	m_cNameVector.push_back("jet");
	m_cNameVector.push_back("parula");
	m_cNameVector.push_back("hot"); // 5
	m_cNameVector.push_back("fire"); // 6
	m_cNameVector.push_back("hsv");
	m_cNameVector.push_back("smart");
	m_cNameVector.push_back("bor");
	m_cNameVector.push_back("cool");
	m_cNameVector.push_back("gem");
	m_cNameVector.push_back("gfb");
	m_cNameVector.push_back("ice");
	m_cNameVector.push_back("lifetime2");
	m_cNameVector.push_back("vessel");
	m_cNameVector.push_back("hsv1");
	m_cNameVector.push_back("magenta"); // 17
	m_cNameVector.push_back("blue_hot"); // 18
	// 새로운 파일 이름 추가 하기

	for (int i = 0; i < m_cNameVector.size(); i++)
	{
		QFile file("ColorTable/" + m_cNameVector.at(i) + ".colortable");
		file.open(QIODevice::ReadOnly);
		np::Uint8Array2 rgb(256, 3);
		file.read(reinterpret_cast<char*>(rgb.raw_ptr()), sizeof(uint8_t) * rgb.length());
		file.close();

		QVector<QRgb> temp_vector;
		for (int j = 0; j < 256; j++)
		{
			QRgb color = qRgb(rgb(j, 0), rgb(j, 1), rgb(j, 2));
			temp_vector.push_back(color);
		}
		m_colorTableVector.push_back(temp_vector);
	}
}


QImageView::QImageView(QWidget *parent) :
	QDialog(parent)
{
    // Disabled
}

QImageView::QImageView(ColorTable::colortable ctable, int width, int height, bool rgb, QWidget *parent) :
    QDialog(parent), m_bSquareConstraint(false), m_bRgbUsed(rgb)
{
    // Set default size
    resize(400, 400);
	
    // Set image size
    m_width = width;
    m_width4 = ((width + 3) >> 2) << 2;
    m_height = height;
	
    // Create layout
	m_pHBoxLayout = new QHBoxLayout(this);
    m_pHBoxLayout->setSpacing(0);
    m_pHBoxLayout->setContentsMargins(1, 1, 1, 1); // Remove bezel area

    // Create render area
    m_pRenderImage = new QRenderImage(this);
	m_pRenderImage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	m_pHBoxLayout->addWidget(m_pRenderImage);	

    // Create QImage object  
	if (!m_bRgbUsed)
	{
		m_pRenderImage->m_pImage = new QImage(m_width, m_height, QImage::Format_Indexed8);
		m_pRenderImage->m_pImage->setColorCount(256);
		m_pRenderImage->m_pImage->setColorTable(m_colorTable.m_colorTableVector.at(ctable));
	}
	else
        m_pRenderImage->m_pImage = new QImage(m_width, m_height, QImage::Format_RGB888);

	memset(m_pRenderImage->m_pImage->bits(), 0, m_pRenderImage->m_pImage->byteCount());

	// Set layout
	setLayout(m_pHBoxLayout);

    // Initialization
    setUpdatesEnabled(true);
}

QImageView::~QImageView()
{

}

void QImageView::resizeEvent(QResizeEvent *)
{
	if (m_bSquareConstraint)
	{
		int w = this->width();
		int h = this->height();

		if (w < h)
			resize(w, w);
		else
			resize(h, h);
	}
}

void QImageView::resetSize(int width, int height)
{
	// Set image size
	m_width = width;
    m_width4 = ((width + 3) >> 2) << 2;
	m_height = height;	

	// Create QImage object
	QRgb rgb[256];
	if (!m_bRgbUsed)
	{
		for (int i = 0; i < 256; i++)
			rgb[i] = m_pRenderImage->m_pImage->color(i);
	}

	if (m_pRenderImage->m_pImage)
		delete m_pRenderImage->m_pImage;

	if (!m_bRgbUsed)
	{
		m_pRenderImage->m_pImage = new QImage(m_width, m_height, QImage::Format_Indexed8);
		m_pRenderImage->m_pImage->setColorCount(256);
		for (int i = 0; i < 256; i++)
			m_pRenderImage->m_pImage->setColor(i, rgb[i]);
	}
	else
        m_pRenderImage->m_pImage = new QImage(m_width, m_height, QImage::Format_RGB888);

	memset(m_pRenderImage->m_pImage->bits(), 0, m_pRenderImage->m_pImage->byteCount());

}

void QImageView::resetColormap(ColorTable::colortable ctable)
{
	m_pRenderImage->m_pImage->setColorTable(m_colorTable.m_colorTableVector.at(ctable));

	m_pRenderImage->update();
}

void QImageView::setHorizontalLine(int len, ...)
{
	m_pRenderImage->m_hLineLen = len;

	va_list ap;
	va_start(ap, len);
	for (int i = 0; i < len; i++)
	{
		int n = va_arg(ap, int);
		m_pRenderImage->m_pHLineInd[i] = n;
	}
	va_end(ap);
}

void QImageView::setVerticalLine(int len, ...)
{
	m_pRenderImage->m_vLineLen = len;

	va_list ap;
	va_start(ap, len);
	for (int i = 0; i < len; i++)
	{
		int n = va_arg(ap, int);
		m_pRenderImage->m_pVLineInd[i] = n;
	}
	va_end(ap);
}

void QImageView::setCircle(int len, ...)
{
	m_pRenderImage->m_circLen = len;

	va_list ap;
	va_start(ap, len);
	for (int i = 0; i < len; i++)
	{
		int n = va_arg(ap, int);
		m_pRenderImage->m_pHLineInd[i] = n;
	}
	va_end(ap);
}

void QImageView::setContour(int len, uint16_t* pContour)
{
    m_pRenderImage->m_contour = np::Uint16Array(len);
    if (len > 0)
        memcpy(m_pRenderImage->m_contour.raw_ptr(), pContour, sizeof(uint16_t) * len);
}

void QImageView::setHLineChangeCallback(const std::function<void(int)> &slot) 
{ 
	m_pRenderImage->DidChangedHLine.clear();
	m_pRenderImage->DidChangedHLine += slot; 
}

void QImageView::setVLineChangeCallback(const std::function<void(int)> &slot)
{ 
	m_pRenderImage->DidChangedVLine.clear();
	m_pRenderImage->DidChangedVLine += slot; 
}

void QImageView::setRLineChangeCallback(const std::function<void(int)> &slot)
{
	m_pRenderImage->DidChangedRLine.clear();
	m_pRenderImage->DidChangedRLine += slot;
}

void QImageView::setMovedMouseCallback(const std::function<void(QPoint&)> &slot)
{
	m_pRenderImage->DidMovedMouse.clear();
	m_pRenderImage->DidMovedMouse += slot;

	m_pRenderImage->setMouseTracking(true);
}

void QImageView::setDoubleClickedMouseCallback(const std::function<void(void)> &slot)
{
	m_pRenderImage->DidDoubleClickedMouse.clear();
	m_pRenderImage->DidDoubleClickedMouse += slot;
}

void QImageView::drawImage(uint8_t* pImage)
{
	memcpy(m_pRenderImage->m_pImage->bits(), pImage, m_pRenderImage->m_pImage->byteCount());	
	m_pRenderImage->update();
}

void QImageView::drawRgbImage(uint8_t* pImage)
{		
    QImage *pImg = new QImage(pImage, m_width4, m_height, QImage::Format_RGB888);
    if (m_width4 == m_width)
        m_pRenderImage->m_pImage = std::move(pImg);
    else
        memcpy(m_pRenderImage->m_pImage->bits(), pImg->copy(0, 0, m_width, m_height).bits(), m_pRenderImage->m_pImage->byteCount());
	m_pRenderImage->update();
}




QRenderImage::QRenderImage(QWidget *parent) :
	QWidget(parent), m_pImage(nullptr), m_colorLine(0xff0000),
	m_bMeasureDistance(false), m_nClicked(0),
    m_hLineLen(0), m_vLineLen(0), m_circLen(0), m_bRadial(false), m_bDiametric(false)
{
	m_pHLineInd = new int[10];
    m_pVLineInd = new int[10];
}

QRenderImage::~QRenderImage()
{
	delete m_pHLineInd;
    delete m_pVLineInd;
}

void QRenderImage::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
	painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, true);

    // Area size
    int w = this->width();
    int h = this->height();

    // Draw image
    if (m_pImage)
        painter.drawImage(QRect(0, 0, w, h), *m_pImage);

	// Draw assitive lines
	for (int i = 0; i < m_hLineLen; i++)
	{
		QPointF p1; p1.setX(0.0);       p1.setY((double)(m_pHLineInd[i] * h) / (double)m_pImage->height());
		QPointF p2; p2.setX((double)w); p2.setY((double)(m_pHLineInd[i] * h) / (double)m_pImage->height());

		painter.setPen(m_colorLine);
		painter.drawLine(p1, p2);
	}
	for (int i = 0; i < m_vLineLen; i++)
	{
		QPointF p1, p2;
		if (!m_bRadial)
		{
			p1.setX((double)(m_pVLineInd[i] * w) / (double)m_pImage->width()); p1.setY(0.0);
			p2.setX((double)(m_pVLineInd[i] * w) / (double)m_pImage->width()); p2.setY((double)h);			
		}
		else
		{			
			int circ_x = (double)(w / 2) + (double)(w / 2) * cos((double)m_pVLineInd[i] / (double)m_rMax * IPP_2PI);
			int circ_y = (double)(h / 2) - (double)(h / 2) * sin((double)m_pVLineInd[i] / (double)m_rMax * IPP_2PI);
			p1.setX(w / 2); p1.setY(h / 2);
			p2.setX(circ_x); p2.setY(circ_y);			
		}

		painter.setPen(m_colorLine);
		painter.drawLine(p1, p2);

		if (m_bDiametric)
		{
			QPointF p1, p2;
			if (!m_bRadial)
			{
				p1.setX((double)((m_pVLineInd[i] + m_pImage->width() / 2) * w) / (double)m_pImage->width()); p1.setY(0.0);
				p2.setX((double)((m_pVLineInd[i] + m_pImage->width() / 2) * w) / (double)m_pImage->width()); p2.setY((double)h);
			}
			else
			{
				int circ_x = (double)(w / 2) + (double)(w / 2) * cos((double)(m_pVLineInd[i] + m_rMax / 2) / (double)m_rMax * IPP_2PI);
				int circ_y = (double)(h / 2) - (double)(h / 2) * sin((double)(m_pVLineInd[i] + m_rMax / 2) / (double)m_rMax * IPP_2PI);
				p1.setX(w / 2); p1.setY(h / 2);
				p2.setX(circ_x); p2.setY(circ_y);
			}

			painter.setPen(m_colorLine);
			painter.drawLine(p1, p2);
		}
	}
	for (int i = 0; i < m_circLen; i++)
	{
		QPointF center; center.setX(w / 2); center.setY(h / 2);
		double radius = (double)(m_pHLineInd[i] * h) / (double)m_pImage->height();

		painter.setPen(m_colorLine);
		painter.drawEllipse(center, radius, radius);
	}
    if (m_contour.length() != 0)
    {
        QPen pen; pen.setColor(Qt::green);
        painter.setPen(pen);
        for (int i = 0; i < m_contour.length() - 1; i++)
        {
            QPointF x0, x1;
            x0.setX((float)(i) / (float)m_contour.length() * (float)w);
            x0.setY((float)(m_contour[i]) / (float)m_pImage->height() * (float)h);
            x1.setX((float)(i + 1) / (float)m_contour.length() * w);
            x1.setY((float)(m_contour[i + 1]) / (float)m_pImage->height() * (float)h);

            painter.drawLine(x0, x1);
        }
    }
	
	// Measure distance
	if (m_bMeasureDistance)
	{
		QPen pen; pen.setColor(Qt::red); pen.setWidth(3);
		painter.setPen(pen);

		QPointF p[2];
		for (int i = 0; i < m_nClicked; i++)
		{
			p[i] = QPointF(m_point[i][0], m_point[i][1]);
			painter.drawPoint(p[i]);
			
			if (i == 1)
			{
				// Connecting line
				QPen pen; pen.setColor(Qt::red); pen.setWidth(1);
				painter.setPen(pen);
				painter.drawLine(p[0], p[1]);
				
				// Euclidean distance
				double dist = sqrt((p[0].x() - p[1].x()) * (p[0].x() - p[1].x())
					+ (p[0].y() - p[1].y()) * (p[0].y() - p[1].y())) * (double)m_pImage->height() / (double)this->height();
				dist *= PIXEL_RESOLUTION;
				//printf("Measured distance: %.1f um\n", dist);

				QFont font; font.setBold(true);
				painter.setFont(font);
				painter.drawText((p[0] + p[1]) / 2, QString("%1 um").arg(dist, 0, 'f', 1));
			}
		}
	}
}

void QRenderImage::mousePressEvent(QMouseEvent *e)
{
	QPoint p = e->pos();
	if (m_hLineLen == 1)
	{
		m_pHLineInd[0] = m_pImage->height() - (int)((double)(p.y() * m_pImage->height()) / (double)this->height());
		DidChangedHLine(m_pHLineInd[0]);
	}
	if (m_vLineLen == 1)
	{
		if (!m_bRadial)
		{
			m_pVLineInd[0] = (int)((double)(p.x() * m_pImage->width()) / (double)this->width());
			DidChangedVLine(m_pVLineInd[0]);
		}
		else
		{
			double angle = atan2((double)height() / 2.0 - (double)p.y(), (double)p.x() - (double)width() / 2.0);
			if (angle < 0) angle += IPP_2PI;
			m_pVLineInd[0] = (int)(angle / IPP_2PI * m_rMax);
			DidChangedRLine(m_pVLineInd[0]);
		}
	}

	if (m_bMeasureDistance)
	{		
		if (m_nClicked == 2) m_nClicked = 0;

		m_point[m_nClicked][0] = p.x(); // (int)((double)(p.x() * m_pImage->height())) / (double)this->height();
		m_point[m_nClicked++][1] = p.y(); // (int)((double)(p.y() * m_pImage->height())) / (double)this->height();

		update();
	}
}

void QRenderImage::mouseDoubleClickEvent(QMouseEvent *)
{
	DidDoubleClickedMouse();
}

void QRenderImage::mouseMoveEvent(QMouseEvent *e)
{
	QPoint p = e->pos();

	if (QRect(0, 0, this->width(), this->height()).contains(p))
	{
		QPoint p1;
		p1.setX((int)((double)(p.x() * m_pImage->width()) / (double)this->width()));
		p1.setY((int)((double)(p.y() * m_pImage->height()) / (double)this->height()));

		DidMovedMouse(p1);
	}
}

//#ifdef OCT_FLIM
//if (m_pIntensity && m_pLifetime)
//{
//	//		QImage im(4 * m_pIntensity->width(), m_pIntensity->height(), QImage::Format_RGB888);		
//	//		QImage in(4 * m_pIntensity->width(), m_pIntensity->height(), QImage::Format_RGB888);
//	//		QImage lf(4 * m_pIntensity->width(), m_pIntensity->height(), QImage::Format_RGB888);
//	//	
//	//		clock_t start1 = clock();
//	//
//	////        // FLIM result image generation
//	//
//	//		im.convertToFormat(QImage::Format_RGB888);
//	//		in.convertToFormat(QImage::Format_RGB888);
//	//		lf.convertToFormat(QImage::Format_RGB888);
//	////	
//	//////        ippsAdd_8u_Sfs(intensity.bits(), lifetime.bits(), m_pFluorescence->bits(), m_pFluorescence->byteCount(), 0);	
//	//
//	//		ippsAdd_8u_Sfs(in.bits(), lf.bits(), m_pFluorescence->bits(), m_pFluorescence->byteCount(), 0);
//	//		ippsAdd_8u_Sfs(in.bits(), lf.bits(), m_pFluorescence->bits(), m_pFluorescence->byteCount(), 0);
//	//
//	//		/*m_pIntensity->convertToFormat(QImage::Format_RGB888).bits(), m_pLifetime->convertToFormat(QImage::Format_RGB888).bits(),
//	//            m_pFluorescence->bits(), m_pFluorescence->byteCount(), 0);*/
//	//
//	//		clock_t end1 = clock();
//	//		double elapsed = ((double)(end1 - start1)) / ((double)CLOCKS_PER_SEC);
//	//		printf("flim paint time : %.2f msec\n", 1000 * elapsed);
//	//
//	////
//	//////        // FLIM result image alpha adjustment
//	//////        ippsDiv_8u_Sfs(m_pFlimAlphaMap->bits(), m_pFluorescence->convertToFormat(QImage::Format_ARGB32_Premultiplied).bits(),
//	//////            m_pFlimImage->bits(), m_pFlimImage->byteCount(), 0);
//	////
//	////
//	////        // Draw FLIM result image
//	////        painter.drawImage(QRect(0, 0, w, h), *m_pFlimImage);
//}
//#endif
