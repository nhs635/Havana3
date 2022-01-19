
#include "QScope.h"

QScope::QScope(QWidget *parent) :
	QDialog(parent)
{
    // Do not use
}

QScope::QScope(QRange x_range, QRange y_range,
               int num_x_ticks, int num_y_ticks,
               double x_interval, double y_interval, double x_offset, double y_offset,
               QString x_unit, QString y_unit, bool mask_use, bool _64_use, QWidget *parent) :
	QDialog(parent)
{
    // Set default size
    resize(400, 300);

    // Create layout
    m_pGridLayout = new QGridLayout(this);
	m_pGridLayout->setSpacing(1);

    // Create render area
    m_pRenderArea = new QRenderArea(this);
	m_pRenderArea->m_bMaskUse = mask_use;
	m_pRenderArea->m_b64Use = _64_use;
    m_pRenderArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    // Set Axis
    setAxis(x_range, y_range, num_x_ticks, num_y_ticks,
            x_interval, y_interval, x_offset, y_offset, x_unit, y_unit);

    // Set layout
    m_pGridLayout->addWidget(m_pRenderArea, 0, 1);
    setLayout(m_pGridLayout);

    // Initialization
    setUpdatesEnabled(true);
}

QScope::~QScope()
{
	if (m_pRenderArea->m_pData)
		delete[] m_pRenderArea->m_pData;
	if (m_pRenderArea->m_pDataX)
		delete[] m_pRenderArea->m_pDataX;
	if (m_pRenderArea->m_pMask)
		delete[] m_pRenderArea->m_pMask;
	if (m_pRenderArea->m_pData64)
		delete[] m_pRenderArea->m_pData64;
	if (m_pRenderArea->m_pSelectedRegion)
		delete[] m_pRenderArea->m_pSelectedRegion;
}

void QScope::keyPressEvent(QKeyEvent * e)
{
	if (e->key() != Qt::Key_Escape)
		QDialog::keyPressEvent(e);
}


void QScope::setAxis(QRange x_range, QRange y_range,
                     int num_x_ticks, int num_y_ticks,
                     double x_interval, double y_interval, double x_offset, double y_offset,
                     QString x_unit, QString y_unit)
{
    // Set size
	m_pRenderArea->setSize(x_range, y_range);
	
    // Set x ticks
    double val; QString str;
    if (num_x_ticks > 1)
    {
        for (int i = 0; i < num_x_ticks; i++)
        {
            QLabel *xlabel = new QLabel(this);
            val = x_interval * (m_pRenderArea->m_xRange.min + i * m_pRenderArea->m_sizeGraph.width() / (num_x_ticks - 1)) + x_offset;
            xlabel->setText(QString("%1%2").arg(val, 4).arg(x_unit));
			if (i == 0)
				xlabel->setAlignment(Qt::AlignLeft);
			else if (i == num_x_ticks - 1)
				xlabel->setAlignment(Qt::AlignRight);
			else
				xlabel->setAlignment(Qt::AlignCenter);

            m_pLabelVector_x.push_back(xlabel);
        }

        QHBoxLayout *pHBoxLayout = new QHBoxLayout;
        pHBoxLayout->addWidget(m_pLabelVector_x.at(0));
        for (int i = 1; i < num_x_ticks; i++)
        {
            pHBoxLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
            pHBoxLayout->addWidget(m_pLabelVector_x.at(i));
        };
        m_pGridLayout->addItem(pHBoxLayout, 1, 1);
    }
	
    // Set y ticks
    if (num_y_ticks > 1)
    {
        for (int i = num_y_ticks - 1; i >= 0; i--)
        {
            QLabel *ylabel = new QLabel(this);
            val = y_interval * (m_pRenderArea->m_yRange.min + i * m_pRenderArea->m_sizeGraph.height() / (num_y_ticks - 1)) + y_offset;
            val = double(int(val * 100)) / 100;
            ylabel->setText(QString("%1%2").arg(val, 4).arg(y_unit));
            ylabel->setFixedWidth(30);
            ylabel->setAlignment(Qt::AlignRight);

            m_pLabelVector_y.push_back(ylabel);
        }

        QVBoxLayout *pVBoxLayout = new QVBoxLayout;
        pVBoxLayout->addWidget(m_pLabelVector_y.at(0));
        for (int i = 1; i < num_y_ticks; i++)
        {
            pVBoxLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
            pVBoxLayout->addWidget(m_pLabelVector_y.at(i));
        }
        m_pGridLayout->addItem(pVBoxLayout, 0, 0);
    }
}

void QScope::resetAxis(QRange x_range, QRange y_range,
                       double x_interval, double y_interval, double x_offset, double y_offset,
                       QString x_unit, QString y_unit)
{
	// Set size
	m_pRenderArea->setSize(x_range, y_range);

	// Set x ticks
    double val; QString str;
	int num_x_ticks = m_pLabelVector_x.size();
	if (num_x_ticks > 1)
	{
		for (int i = 0; i < num_x_ticks; i++)
		{
			QLabel *xlabel = m_pLabelVector_x.at(i);
			val = x_interval * (m_pRenderArea->m_xRange.min + i * m_pRenderArea->m_sizeGraph.width() / (num_x_ticks - 1)) + x_offset;
			xlabel->setText(QString("%1%2").arg(val, 4).arg(x_unit));
		}
	}

	// Set y ticks
    int num_y_ticks = m_pLabelVector_y.size();
	if (num_y_ticks > 1)
	{
		for (int i = 0; i < num_y_ticks; i++)
		{
			QLabel *ylabel = m_pLabelVector_y.at(num_y_ticks - 1 - i);
			val = y_interval * (m_pRenderArea->m_yRange.min + i * m_pRenderArea->m_sizeGraph.height() / (num_y_ticks - 1)) + y_offset;
			val = double(int(val * 100)) / 100;
			ylabel->setText(QString("%1%2").arg(val, 4).arg(y_unit));
		}
	}
}

void QScope::setWindowLine(int len, ...)
{
	m_pRenderArea->m_winLineLen = len;

	va_list ap;
	va_start(ap, len);
	for (int i = 0; i < len; i++)
	{
		int n = va_arg(ap, int);
		m_pRenderArea->m_pWinLineInd[i] = n;
	}
	va_end(ap);
}

void QScope::setMeanDelayLine(int len, ...)
{
	m_pRenderArea->m_mdLineLen = len;

	va_list ap;
	va_start(ap, len);
	for (int i = 0; i < len; i++)
	{
		float f = va_arg(ap, float);
		m_pRenderArea->m_pMdLineInd[i] = f;
	}
	va_end(ap);
}

void QScope::setDcLine(float dcLine)
{
	m_pRenderArea->m_dcLine = dcLine;
}

void QScope::drawData(const float* pData)
{
	if (m_pRenderArea->m_pData != nullptr)
		memcpy(m_pRenderArea->m_pData, pData, sizeof(float) * (int)m_pRenderArea->m_buff_len);

	m_pRenderArea->update();
}

void QScope::drawData(const float* pData, const float* pDataX, bool foo)
{
	if (m_pRenderArea->m_pData != nullptr)
		memcpy(m_pRenderArea->m_pData, pData, sizeof(float) * (int)m_pRenderArea->m_buff_len);
	if (m_pRenderArea->m_pDataX != nullptr)
		memcpy(m_pRenderArea->m_pDataX, pDataX, sizeof(float) * (int)m_pRenderArea->m_buff_len);

	m_pRenderArea->update();
}

/* FLIM Calib Purpose */
void QScope::drawData(const float* pData, const float* pMask)
{
	if (m_pRenderArea->m_pData != nullptr)
		memcpy(m_pRenderArea->m_pData, pData, sizeof(float) * (int)m_pRenderArea->m_buff_len);

	if (m_pRenderArea->m_pMask != nullptr)
		memcpy(m_pRenderArea->m_pMask, pMask, sizeof(float) * (int)m_pRenderArea->m_buff_len);

	m_pRenderArea->update();
}

void QScope::drawData(const double* pData64)
{
	if (m_pRenderArea->m_pData64 != nullptr)
		memcpy(m_pRenderArea->m_pData64, pData64, sizeof(double) * (int)m_pRenderArea->m_buff_len);

	m_pRenderArea->update();
}



QRenderArea::QRenderArea(QWidget *parent) :
    QWidget(parent), m_pData(nullptr), m_pDataX(nullptr), m_pMask(nullptr), m_pData64(nullptr), m_pSelectedRegion(nullptr),
	m_bSelectionAvailable(false), m_bMaskUse(false), m_b64Use(false), m_buff_len(0), m_winLineLen(0), m_mdLineLen(0), m_dcLine(0),
	m_nHMajorGrid(8), m_nHMinorGrid(64), m_nVMajorGrid(4), m_bZeroLine(false), m_bScattered(false)
{
    QPalette pal = this->palette();
    pal.setColor(QPalette::Background, QColor(0x282d30));
    setPalette(pal);
    setAutoFillBackground(true);

	m_pWinLineInd = new int[10];
	m_pMdLineInd = new float[10];

	m_selected[0] = -1; m_selected[1] = -1; 
}

QRenderArea::~QRenderArea()
{
	delete[] m_pWinLineInd;
	delete[] m_pMdLineInd;
}

void QRenderArea::setSize(QRange xRange, QRange yRange, int len)
{
	// Set range
	m_xRange = xRange;
	m_yRange = yRange;

	// Set graph size
	m_sizeGraph = { m_xRange.max - m_xRange.min , m_yRange.max - m_yRange.min };

	// Buffer length
	m_buff_len = (len != 0) ? len : m_sizeGraph.width();

	// Allocate data buffer
	if (!m_b64Use)
	{
		if (m_pData) { delete[] m_pData; m_pData = nullptr; }
		m_pData = new float[m_buff_len];
		memset(m_pData, 0, sizeof(float) * m_buff_len);

		if (m_pDataX) { delete[] m_pDataX; m_pDataX = nullptr; }
		m_pDataX = new float[m_buff_len];
		memset(m_pDataX, 0, sizeof(float) * m_buff_len);
	}
	else
	{
		if (m_pData64) { delete[] m_pData64; m_pData64 = nullptr; }
		m_pData64 = new double[m_buff_len];
		memset(m_pData64, 0, sizeof(double) * m_buff_len);
	}

	if (m_pSelectedRegion) { delete[] m_pSelectedRegion; m_pSelectedRegion = nullptr; }
	m_pSelectedRegion = new uint8_t[m_buff_len];
	memset(m_pSelectedRegion, 0, sizeof(uint8_t) * m_buff_len);
	
	if (m_bMaskUse)
	{
		if (m_pMask) { delete[] m_pMask; m_pMask = nullptr; }
		m_pMask = new float[m_buff_len];
		memset(m_pMask, 0, sizeof(float) * m_buff_len);
	}

	this->update();
}

void QRenderArea::setGrid(int nHMajorGrid, int nHMinorGrid, int nVMajorGrid, bool zeroLine)
{
	m_nHMajorGrid = nHMajorGrid;
	m_nHMinorGrid = nHMinorGrid;
	m_nVMajorGrid = nVMajorGrid;
	m_bZeroLine = zeroLine;
}

void QRenderArea::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Area size
    int w = this->width();
    int h = this->height();

    // Draw grid
    painter.setPen(QColor(0x4f5555)); // Minor grid color (horizontal)
    for (int i = 0; i <= m_nHMinorGrid; i++)
        painter.drawLine(i * w / m_nHMinorGrid, 0, i * w / m_nHMinorGrid, h);

    painter.setPen(QColor(0x7b8585)); // Major grid color (horizontal)
    for (int i = 0; i <= m_nHMajorGrid; i++)
        painter.drawLine(i * w / m_nHMajorGrid, 0, i * w / m_nHMajorGrid, h);
    for (int i = 0; i <= m_nVMajorGrid; i++) // Major grid color (vertical)
        painter.drawLine(0, i * h / m_nVMajorGrid, w, i * h / m_nVMajorGrid);

	if (m_bZeroLine) // zero line
		painter.drawLine(0, (m_yRange.max / (m_yRange.max - m_yRange.min)) * h, w, (m_yRange.max / (m_yRange.max - m_yRange.min)) * h);
	
	// Draw horizontal assistive lines
	{
		QPointF x0, x1;
		x0.setX((float)0);
		x0.setY(((float)m_sizeGraph.height() - (float)m_dcLine) / (float)m_sizeGraph.height() * h);
		x1.setX((float)w);
		x1.setY(((float)m_sizeGraph.height() - (float)m_dcLine) / (float)m_sizeGraph.height() * h);

		painter.setPen(QColor(0x00ffff));
		painter.drawLine(x0, x1);
	}

    // Draw graph
    if (m_pData != nullptr) // single case
    {        
		painter.setPen(QColor(0xfff65d)); // data graph (yellow)
		for (int i = 0; i < m_buff_len - 1; i++)
		{			
			QPointF x0, x1;		
			if (!m_bScattered)
			{
				x0.setX((float)(i) / (float)(m_sizeGraph.width() - 1) * w);
				x0.setY((float)(m_yRange.max - m_pData[i]) * (float)h / (float)(m_yRange.max - m_yRange.min));
				x1.setX((float)(i + 1) / (float)(m_sizeGraph.width() - 1) * w);
				x1.setY((float)(m_yRange.max - m_pData[i + 1]) * (float)h / (float)(m_yRange.max - m_yRange.min));

				painter.drawLine(x0, x1);
			}
			else
			{
				if (m_pDataX != nullptr)
				{
					x0.setX((float)(m_pDataX[i]) * (float)w / (float)(m_xRange.max - m_xRange.min));
					x0.setY((float)(m_yRange.max - m_pData[i]) * (float)h / (float)(m_yRange.max - m_yRange.min));

					painter.drawPoint(x0);
				}
			}
        }
    }
	if (m_pData64 != nullptr) // double case
	{
		painter.setPen(QColor(0xfff65d)); // data graph (yellow)
		for (int i = 0; i < m_buff_len - 1; i++)
		{
			QPointF x0, x1;
			x0.setX((double)(i) / (double)(m_sizeGraph.width() - 1) * w);
			x0.setY((double)(m_yRange.max - m_pData64[i]) * (double)h / (double)(m_yRange.max - m_yRange.min));
			x1.setX((double)(i + 1) / (double)(m_sizeGraph.width() - 1) * w);
			x1.setY((double)(m_yRange.max - m_pData64[i + 1]) * (double)h / (double)(m_yRange.max - m_yRange.min));

			if (!m_bScattered)
				painter.drawLine(x0, x1);
			else
				painter.drawPoint(x0);
		}
	}
	
	if (m_bMaskUse && (m_pMask != nullptr))
	{
		QPen pen(QColor(0xff0080)); // mask region (hot pink)
		pen.setWidth(2);
		painter.setPen(pen);		
		for (int i = 0; i < m_buff_len - 1; i++)
		{
			if (m_pMask[i] == 0)
			{
				QPointF x0, x1;
				if (!m_bScattered)
				{
					x0.setX((float)(i) / (float)(m_sizeGraph.width() - 1) * w);
					x0.setY((float)(m_yRange.max - m_pData[i]) * (float)h / (float)(m_yRange.max - m_yRange.min));
					x1.setX((float)(i + 1) / (float)(m_sizeGraph.width() - 1) * w);
					x1.setY((float)(m_yRange.max - m_pData[i + 1]) * (float)h / (float)(m_yRange.max - m_yRange.min));

					painter.drawLine(x0, x1);
				}
				else
				{
					if (m_pDataX != nullptr)
					{
						x0.setX((float)(m_pDataX[i]) * (float)w / (float)(m_xRange.max - m_xRange.min));
						x0.setY((float)(m_yRange.max - m_pData[i]) * (float)h / (float)(m_yRange.max - m_yRange.min));

						painter.drawPoint(x0);
					}
				}
			}
		}
	}	
	if (m_bSelectionAvailable)
	{
		QPen pen(QColor(0xff6666)); pen.setWidth(3); // selected region (light pink)					
		painter.setPen(pen);
		for (int i = 0; i < m_buff_len - 1; i++)
		{			
			if (m_pSelectedRegion[i] == 1)
			{
				QPointF x0, x1;
				x0.setX((float)(i) / (float)(m_sizeGraph.width() - 1) * w);
				x0.setY((float)(m_yRange.max - m_pData[i]) * (float)h / (float)(m_yRange.max - m_yRange.min));
				x1.setX((float)(i + 1) / (float)(m_sizeGraph.width() - 1) * w);
				x1.setY((float)(m_yRange.max - m_pData[i + 1]) * (float)h / (float)(m_yRange.max - m_yRange.min));

				painter.drawLine(x0, x1);
			}
		}
	}

	// Draw vertical lines...
	for (int i = 0; i < m_mdLineLen; i++)
	{
		QPointF x0, x1;
		x0.setX(m_pMdLineInd[i] / (float)(m_sizeGraph.width() - 1) * w);
		x0.setY((float)0);
		x1.setX(m_pMdLineInd[i] / (float)(m_sizeGraph.width() - 1) * w);
		x1.setY((float)h);

		painter.setPen(QColor(0x00ff00));
		painter.drawLine(x0, x1);
	}
	for (int i = 0; i < m_winLineLen; i++)
	{
		QPointF x0, x1;
		x0.setX((float)(m_pWinLineInd[i]) / ((float)m_sizeGraph.width() - 1) * w);
		x0.setY((float)0);
		x1.setX((float)(m_pWinLineInd[i]) / ((float)m_sizeGraph.width() - 1) * w);
		x1.setY((float)h);

		painter.setPen(QColor(0xff0000));
		painter.drawLine(x0, x1);
	}
}

void QRenderArea::mousePressEvent(QMouseEvent *e)
{
	if (m_bSelectionAvailable)
	{
		m_bMousePressed = true;

		QPoint p = e->pos();
		auto button = e->button();
		if ((p.x() > 0) && (p.y() > 0) && (p.x() < this->width()) && (p.y() < this->height()))
		{
			int x = (double)p.x() / (double)this->width() * (double)m_sizeGraph.width();
			
			m_selected[0] = x;
			m_selected[1] = x;

			m_bIsLeftButton = button == Qt::LeftButton;
			if (m_bIsLeftButton)
				memset(m_pSelectedRegion, 0, sizeof(uint8_t) * (int)m_sizeGraph.width());
			
			memset(&m_pSelectedRegion[x], 1, 1);

			//printf("[%d %d]\n", m_selected[0], m_selected[1]);
			//this->update();
			DidMouseEvent();
		}
	}
}

void QRenderArea::mouseMoveEvent(QMouseEvent *e)
{
	if (m_bSelectionAvailable)
	{
		if (m_bMousePressed)
		{
			QPoint p = e->pos();
			auto button = e->button();
			if ((p.x() > 0) && (p.y() > 0) && (p.x() < this->width()) && (p.y() < this->height()))
			{
				int x = (double)p.x() / (double)this->width() * (double)m_sizeGraph.width();
				m_selected[1] = x;

				// ordered
				m_start = m_selected[0], m_end = m_selected[1];
				if (m_selected[0] > m_selected[1])
				{
					m_start = m_selected[1];
					m_end = m_selected[0];
				}

				if (m_bIsLeftButton)
					memset(m_pSelectedRegion, 0, sizeof(uint8_t) * (int)m_sizeGraph.width());
				memset(&m_pSelectedRegion[m_start], 1, sizeof(uint8_t) * (m_end - m_start));

				//printf("[%d %d]\n", m_start, m_end);
				//this->update();
				DidMouseEvent();
			}
		}
	}
}

void QRenderArea::mouseReleaseEvent(QMouseEvent *e)
{
	if (m_bSelectionAvailable)
	{
		m_bMousePressed = false;
		
		QPoint p = e->pos();
		auto button = e->button();
		if ((p.x() > 0) && (p.y() > 0) && (p.x() < this->width()) && (p.y() < this->height()))
		{
			int x = (double)p.x() / (double)this->width() * (double)m_sizeGraph.width();
			m_selected[1] = x;

			// ordered
			m_start = m_selected[0], m_end = m_selected[1];
			if (m_selected[0] > m_selected[1])
			{
				m_start = m_selected[1];
				m_end = m_selected[0];
			}

			if (m_bIsLeftButton)
				memset(m_pSelectedRegion, 0, sizeof(uint8_t) * (int)m_sizeGraph.width());
			memset(&m_pSelectedRegion[m_start], 1, sizeof(uint8_t) * (m_end - m_start));

			//printf("[%d %d]\n", m_selected[0], m_selected[1]);
			//this->update();
			DidMouseEvent();
		}
	}
}