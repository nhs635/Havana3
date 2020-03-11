

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
	m_cNameVector.push_back("lifetime2"); // 14
	m_cNameVector.push_back("vessel");
	m_cNameVector.push_back("hsv1"); // 16
	m_cNameVector.push_back("magenta"); // 17
	m_cNameVector.push_back("blue_hot"); // 18
	m_cNameVector.push_back("clf"); // 19
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

	m_pRenderImage->m_rectMagnified = QRect(0, 0, m_width, m_height);

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

	m_pRenderImage->m_rectMagnified = QRect(0, 0, m_width, m_height);

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

void QImageView::setMagnDefault()
{
	m_pRenderImage->m_rectMagnified = QRect(0, 0, m_pRenderImage->m_pImage->width(), m_pRenderImage->m_pImage->height());
	m_pRenderImage->m_fMagnLevel = 1.0;
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

void QImageView::setWheelMouseCallback(const std::function<void(void)>& slot)
{
	m_pRenderImage->DidWheelMouse.clear();
	m_pRenderImage->DidWheelMouse += slot;
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
    m_hLineLen(0), m_vLineLen(0), m_circLen(0), 
	m_bRadial(false), m_bDiametric(false),
	m_bCanBeMagnified(false), m_rectMagnified(0, 0, 0, 0), m_fMagnLevel(1.0)
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
		painter.drawImage(QRect(0, 0, w, h), *m_pImage, m_rectMagnified);
	
	// Draw assitive lines
	for (int i = 0; i < m_hLineLen; i++)
	{
		QPointF p1, p2; 
		p1.setX(0.0);
		p1.setY((double)((m_pHLineInd[i] - m_rectMagnified.top()) * h) / (double)m_fMagnLevel / (double)m_pImage->height());
		p2.setX((double)w);
		p2.setY((double)((m_pHLineInd[i] - m_rectMagnified.top()) * h) / (double)m_fMagnLevel / (double)m_pImage->height());

		painter.setPen(m_colorLine);
		painter.drawLine(p1, p2);
	}
	for (int i = 0; i < m_vLineLen; i++)
	{
		QPointF p1, p2;
		if (!m_bRadial)
		{
			p1.setX((double)((m_pVLineInd[i] - m_rectMagnified.left()) * w) / (double)m_fMagnLevel / (double)m_pImage->width()); 
			p1.setY(0.0);
			p2.setX((double)((m_pVLineInd[i] - m_rectMagnified.left()) * w) / (double)m_fMagnLevel / (double)m_pImage->width()); 
			p2.setY((double)h);
		}
		else
		{	
			int center_x = ((double)m_pImage->width() / 2.0 - (double)m_rectMagnified.left()) * (double)w / (double)m_rectMagnified.width();
			int center_y = ((double)m_pImage->height() / 2.0 - (double)m_rectMagnified.top()) * (double)h / (double)m_rectMagnified.height();
			int circ_x = center_x + double(w / 2) * (double)m_pImage->width() / (double)m_rectMagnified.width() * cos((double)m_pVLineInd[i] / (double)m_rMax * IPP_2PI);
			int circ_y = center_y - double(h / 2) * (double)m_pImage->height() / (double)m_rectMagnified.height() * sin((double)m_pVLineInd[i] / (double)m_rMax * IPP_2PI);
			p1.setX(center_x);
			p1.setY(center_y);
			p2.setX(circ_x); 
			p2.setY(circ_y);			
		}

		painter.setPen(m_colorLine);
		painter.drawLine(p1, p2);

		if (m_bDiametric)
		{
			QPointF p1, p2;
			if (!m_bRadial)
			{
				p1.setX((double)(((m_pVLineInd[i] + m_pImage->width() / 2) - m_rectMagnified.left()) * w) / (double)m_fMagnLevel / (double)m_pImage->width());
				p1.setY(0.0);
				p2.setX((double)(((m_pVLineInd[i] + m_pImage->width() / 2) - m_rectMagnified.left()) * w) / (double)m_fMagnLevel / (double)m_pImage->width());
				p2.setY((double)h);
			}
			else
			{
				int center_x = ((double)m_pImage->width() / 2.0 - (double)m_rectMagnified.left()) * (double)w / (double)m_rectMagnified.width();
				int center_y = ((double)m_pImage->height() / 2.0 - (double)m_rectMagnified.top()) * (double)h / (double)m_rectMagnified.height();
				int circ_x = center_x + double(w / 2) * (double)m_pImage->width() / (double)m_rectMagnified.width() * cos((double)(m_pVLineInd[i] + m_rMax / 2) / (double)m_rMax * IPP_2PI);
				int circ_y = center_y - double(h / 2) * (double)m_pImage->height() / (double)m_rectMagnified.height() * sin((double)(m_pVLineInd[i] + m_rMax / 2) / (double)m_rMax * IPP_2PI);
				p1.setX(center_x);
				p1.setY(center_y);
				p2.setX(circ_x);
				p2.setY(circ_y);
			}

			painter.setPen(m_colorLine);
			painter.drawLine(p1, p2);
		}
	}
	for (int i = 0; i < m_circLen; i++)
	{
		int center_x = ((double)m_pImage->width() / 2.0 - (double)m_rectMagnified.left()) * (double)w / (double)m_rectMagnified.width();
		int center_y = ((double)m_pImage->height() / 2.0 - (double)m_rectMagnified.top()) * (double)h / (double)m_rectMagnified.height();
		QPointF center; center.setX(center_x); center.setY(center_y);
		double radius = (double)(m_pHLineInd[i] * h) / (double)m_rectMagnified.height();

		painter.setPen(m_colorLine);
		painter.drawEllipse(center, radius, radius);
	}
    ///if (m_contour.length() != 0)
    ///{
    ///    QPen pen; pen.setColor(Qt::green);
    ///    painter.setPen(pen);
    ///    for (int i = 0; i < m_contour.length() - 1; i++)
    ///    {
    ///        QPointF x0, x1;
    ///        x0.setX((float)(i) / (float)m_contour.length() * (float)w);
    ///        x0.setY((float)(m_contour[i]) / (float)m_pImage->height() * (float)h);
    ///        x1.setX((float)(i + 1) / (float)m_contour.length() * w);
    ///        x1.setY((float)(m_contour[i + 1]) / (float)m_pImage->height() * (float)h);

    ///        painter.drawLine(x0, x1);
    ///    }
    ///}
	
	// Measure distance
	if (m_bMeasureDistance)
	{
		QPen pen; pen.setColor(Qt::red); pen.setWidth(3);
		painter.setPen(pen);

		QPointF p[2], pc[2];
		for (int i = 0; i < m_nClicked; i++)
		{
			p[i] = QPointF(m_point[i][0], m_point[i][1]);
			painter.drawPoint(p[i]);
			
			int rx = (double)(p[i].x() * m_rectMagnified.width()) / (double)(this->width()) + m_rectMagnified.left();
			int ry = (double)(p[i].y() * m_rectMagnified.height()) / (double)(this->height()) + m_rectMagnified.top();
			pc[i] = QPointF(rx, ry);
			
			if (i == 1)
			{
				// Connecting line
				QPen pen; pen.setColor(Qt::red); pen.setWidth(1);
				painter.setPen(pen);
				painter.drawLine(p[0], p[1]);
				
				// Euclidean distance
				double dist = sqrt((pc[0].x() - pc[1].x()) * (pc[0].x() - pc[1].x())
								 + (pc[0].y() - pc[1].y()) * (pc[0].y() - pc[1].y()));
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
		m_pHLineInd[0] = m_pImage->height() - ((int)((double)(p.y() * m_rectMagnified.height()) / (double)this->height()) + m_rectMagnified.top());
		DidChangedHLine(m_pHLineInd[0]);
	}
	if (m_vLineLen == 1)
	{
		if (!m_bRadial)
		{
			m_pVLineInd[0] = (int)((double)(p.x() * m_rectMagnified.width()) / (double)this->width()) + m_rectMagnified.left();
			DidChangedVLine(m_pVLineInd[0]);
		}
		else
		{
			double center_x = ((double)m_pImage->width() / 2.0 - (double)m_rectMagnified.left()) * (double)(this->width()) / (double)m_rectMagnified.width();
			double center_y = ((double)m_pImage->height() / 2.0 - (double)m_rectMagnified.top()) * (double)(this->height()) / (double)m_rectMagnified.height();

			double angle = atan2(center_y - (double)p.y(), (double)p.x() - center_x);
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
		int rx = int((double)(p.x() * m_rectMagnified.width()) / (double)(this->width())) + m_rectMagnified.left();
		int ry = int((double)(p.y() * m_rectMagnified.height()) / (double)(this->height())) + m_rectMagnified.top();

		QPoint p1(rx, ry);

		DidMovedMouse(p1);
	}
}

void QRenderImage::wheelEvent(QWheelEvent *e)
{
	if (m_bCanBeMagnified)
	{
		QPoint p = e->pos();

		if (QRect(0, 0, this->width(), this->height()).contains(p))
		{
			int rx = int((double)(p.x() * m_rectMagnified.width()) / (double)(this->width()));
			int ry = int((double)(p.y() * m_rectMagnified.height()) / (double)(this->height()));
			
			QPoint a = e->angleDelta();
			if (a.y() > 0)
			{
				m_fMagnLevel = m_fMagnLevel - 0.05f;
				if (m_fMagnLevel < 0.05f)
					m_fMagnLevel = 0.05f;
			}
			else
			{
				m_fMagnLevel = m_fMagnLevel + 0.05f;
				if (m_fMagnLevel > 1.0f)
					m_fMagnLevel = 1.0f;
			}

			int magn_width = int(m_pImage->width() * m_fMagnLevel);
			int magn_height = int(m_pImage->height() * m_fMagnLevel);
			int magn_left, magn_top;

			if (a.y() > 0)
			{
				magn_left = rx - magn_width / 2;
				if (magn_left < 0) magn_left = 0;
				if (magn_left > m_rectMagnified.width() - magn_width) magn_left = m_rectMagnified.width() - magn_width;
				magn_left += m_rectMagnified.left();

				magn_top = ry - magn_height / 2;
				if (magn_top < 0) magn_top = 0;
				if (magn_top > m_rectMagnified.height() - magn_height) magn_top = m_rectMagnified.height() - magn_height;
				magn_top += m_rectMagnified.top();
			}
			else
			{
				magn_left = rx - magn_width / 2 + m_rectMagnified.left();
				if (magn_left < 0) magn_left = 0;
				if (magn_left > m_pImage->width() - magn_width) magn_left = m_pImage->width() - magn_width;

				magn_top = ry - magn_height / 2 + m_rectMagnified.top();
				if (magn_top < 0) magn_top = 0;
				if (magn_top > m_pImage->height() - magn_height) magn_top = m_pImage->height() - magn_height;
			}
				
			m_rectMagnified = QRect(magn_left, magn_top, magn_width, magn_height);
			
			DidWheelMouse();
			
			update();
		}
	}
}

///#ifdef OCT_FLIM
///if (m_pIntensity && m_pLifetime)
///{
///	//		QImage im(4 * m_pIntensity->width(), m_pIntensity->height(), QImage::Format_RGB888);		
///	//		QImage in(4 * m_pIntensity->width(), m_pIntensity->height(), QImage::Format_RGB888);
///	//		QImage lf(4 * m_pIntensity->width(), m_pIntensity->height(), QImage::Format_RGB888);
///	//	
///	//		clock_t start1 = clock();
///	//
///	////        // FLIM result image generation
///	//
///	//		im.convertToFormat(QImage::Format_RGB888);
///	//		in.convertToFormat(QImage::Format_RGB888);
///	//		lf.convertToFormat(QImage::Format_RGB888);
///	////	
///	//////        ippsAdd_8u_Sfs(intensity.bits(), lifetime.bits(), m_pFluorescence->bits(), m_pFluorescence->byteCount(), 0);	
///	//
///	//		ippsAdd_8u_Sfs(in.bits(), lf.bits(), m_pFluorescence->bits(), m_pFluorescence->byteCount(), 0);
///	//		ippsAdd_8u_Sfs(in.bits(), lf.bits(), m_pFluorescence->bits(), m_pFluorescence->byteCount(), 0);
///	//
///	//		/*m_pIntensity->convertToFormat(QImage::Format_RGB888).bits(), m_pLifetime->convertToFormat(QImage::Format_RGB888).bits(),
///	//            m_pFluorescence->bits(), m_pFluorescence->byteCount(), 0);*/
///	//
///	//		clock_t end1 = clock();
///	//		double elapsed = ((double)(end1 - start1)) / ((double)CLOCKS_PER_SEC);
///	//		printf("flim paint time : %.2f msec\n", 1000 * elapsed);
///	//
///	////
///	//////        // FLIM result image alpha adjustment
///	//////        ippsDiv_8u_Sfs(m_pFlimAlphaMap->bits(), m_pFluorescence->convertToFormat(QImage::Format_ARGB32_Premultiplied).bits(),
///	//////            m_pFlimImage->bits(), m_pFlimImage->byteCount(), 0);
///	////
///	////
///	////        // Draw FLIM result image
///	////        painter.drawImage(QRect(0, 0, w, h), *m_pFlimImage);
///}
///#endif
