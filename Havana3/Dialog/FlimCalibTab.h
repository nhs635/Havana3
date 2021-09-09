#ifndef FLIMCALIBTAB_H
#define FLIMCALIBTAB_H

#include <QtWidgets>
#include <QtCore>

#include <Havana3/Viewer/QScope.h>
#include <Havana3/Viewer/QImageView.h>

#include <Common/callback.h>

#include <ipps.h>
#include <ippi.h>
#include <ippvm.h>

#define N_BINS 50


class Configuration;
class QStreamTab;
class FLImProcess;

class QMySpinBox2 : public QDoubleSpinBox
{
public:
    explicit QMySpinBox2(QWidget *parent = nullptr) : QDoubleSpinBox(parent)
    {
        lineEdit()->setReadOnly(true);
    }
    virtual ~QMySpinBox2() {}
};

class QRenderArea3 : public QWidget
{
public:
	explicit QRenderArea3(QWidget *) 
		: m_nHMajorGrid(4), m_nHMinorGrid(16), m_nVMajorGrid(1)
	{
		QPalette pal = this->palette();
		pal.setColor(QPalette::Background, QColor(0x282d30));
		setPalette(pal);
		setAutoFillBackground(true);

		m_pData[0] = nullptr;
		m_pData[1] = nullptr;
		m_pData[2] = nullptr;

		color[0] = 0xd900ff;
		color[1] = 0x0088ff;
		color[2] = 0x88ff00;
	}

	virtual ~QRenderArea3()
	{
	}

protected:
	void paintEvent(QPaintEvent *)
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

		// Draw graph
		if (m_pData != nullptr) // single case
		{
			for (int i = 0; i < 3; i++)
			{
				painter.setPen(QColor(color[i])); // data graph (yellow)
				for (int j = 0; j < m_buff_len - 1; j++)
				{
					QPointF x0, x1;

					x0.setX((float)(j) / (float)m_sizeGraph.width() * w);
					x0.setY((float)(m_yRange.max - m_pData[i][j]) * (float)h / (float)(m_yRange.max - m_yRange.min));
					x1.setX((float)(j + 1) / (float)m_sizeGraph.width() * w);
					x1.setY((float)(m_yRange.max - m_pData[i][j + 1]) * (float)h / (float)(m_yRange.max - m_yRange.min));

					painter.drawLine(x0, x1);
				}
			}
		}
	}

public:
	void setSize(QRange xRange, QRange yRange, int len = 0)
	{
		// Set range
		m_xRange = xRange;
		m_yRange = yRange;

		// Set graph size
		m_sizeGraph = { m_xRange.max - m_xRange.min , m_yRange.max - m_yRange.min };

		// Buffer length
		m_buff_len = (len != 0) ? len : m_sizeGraph.width();

		// Allocate data buffer
		for (int i = 0; i < 3; i++)
		{
			if (m_pData[i]) { delete[] m_pData[i]; m_pData[i] = nullptr; }
			m_pData[i] = new float[m_buff_len];
			memset(m_pData[i], 0, sizeof(float) * m_buff_len);
		}

		this->update();
	}
		
public:
	float* m_pData[3];
	int color[3];

	int m_nHMajorGrid;
	int m_nHMinorGrid;
	int m_nVMajorGrid;

	QRange m_xRange;
	QRange m_yRange;
	QSizeF m_sizeGraph;
	int m_buff_len;
};

struct Histogram
{
public:
    Histogram() : lowerLevel(0), upperLevel(0), pHistObj(nullptr), pBuffer(nullptr), pLevels(nullptr)
    {
    }

    Histogram(int _nBins, int _length) : pHistObj(nullptr), pBuffer(nullptr), pLevels(nullptr), lowerLevel(0), upperLevel(0)
    {
        initialize(_nBins, _length);
    }

    ~Histogram()
    {
        if (pHistObj) ippsFree(pHistObj);
        if (pBuffer) ippsFree(pBuffer);
        if (pLevels) ippsFree(pLevels);
        if (pHistTemp) ippsFree(pHistTemp);
    }

public:
    void operator() (const Ipp32f* pSrc, Ipp32f* pHist, Ipp32f _lowerLevel, Ipp32f _upperLevel)
    {
        if ((lowerLevel != _lowerLevel) || (upperLevel != _upperLevel))
        {
            // set vars
            lowerLevel = _lowerLevel;
            upperLevel = _upperLevel;

            // initialize spec
            ippiHistogramUniformInit(ipp32f, &_lowerLevel, &_upperLevel, &nLevels, 1, pHistObj);

            // check levels of bins
            ippiHistogramGetLevels(pHistObj, &pLevels);
        }

        // calculate histogram
        ippiHistogram_32f_C1R(pSrc, sizeof(Ipp32f) * roiSize.width, roiSize, pHistTemp, pHistObj, pBuffer);
        ippsConvert_32s32f((Ipp32s*)pHistTemp, pHist, nBins);
    }

public:
    void initialize(int _nBins, int _length)
    {
        // init vars
        roiSize = { _length, 1 };
        nBins = _nBins; nLevels = nBins + 1;
        pLevels = ippsMalloc_32f(nLevels);

        // get sizes for spec and buffer
        ippiHistogramGetBufferSize(ipp32f, roiSize, &nLevels, 1/*nChan*/, 1/*uniform*/, &sizeHistObj, &sizeBuffer);

        pHistObj = (IppiHistogramSpec*)ippsMalloc_8u(sizeHistObj);
        pBuffer = (Ipp8u*)ippsMalloc_8u(sizeBuffer);
        pHistTemp = ippsMalloc_32u(nBins);
    }

private:
    IppiSize roiSize;
    int nBins, nLevels;
    int sizeHistObj, sizeBuffer;
    Ipp32f lowerLevel, upperLevel;
    IppiHistogramSpec* pHistObj;
    Ipp8u* pBuffer;
    Ipp32f* pLevels;
    Ipp32u* pHistTemp;
};


class FlimCalibTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit FlimCalibTab(QWidget *parent = nullptr);
    virtual ~FlimCalibTab();

// Methods //////////////////////////////////////////////
public:
    inline QStreamTab* getStreamTab() const { return m_pStreamTab; }
    inline QGroupBox* getLayoutBox() const { return m_pGroupBox_FlimCalibTab; }

private:
    void createPulseView();
    void createCalibWidgets();
    void createHistogram();
	
public slots : // widgets
    void drawRoiPulse(FLImProcess*, int);

    void showWindow(bool);
    void showMeanDelay(bool);
    void splineView(bool);

    void showMask(bool);
    void modifyMask(bool);
    void addMask();
    void removeMask();

	void setPX14DcOffset(int);
    void captureBackground();
    void captureBackground(const QString &);

    void resetChStart0(double);
    void resetChStart1(double);
    void resetChStart2(double);
    void resetChStart3(double);
    void resetDelayTimeOffset();

signals:
    void plotRoiPulse(FLImProcess*, int);

public:
	// Callbacks
	callback<const char*> SendStatusMessage;

// Variables ////////////////////////////////////////////
private:
    Configuration* m_pConfig;
    QStreamTab* m_pStreamTab;;
    FLImProcess* m_pFLIm;

private:
    int* m_pStart;
    int* m_pEnd;

private:
    Histogram* m_pHistogramIntensity;
    Histogram* m_pHistogramLifetime;

private:
    // Layout
    QVBoxLayout *m_pVBoxLayout;
    QGroupBox *m_pGroupBox_FlimCalibTab;

    // Pulse view widgets
    QScope *m_pScope_PulseView;

    QCheckBox *m_pCheckBox_ShowWindow;
    QCheckBox *m_pCheckBox_ShowMeanDelay;
    QCheckBox *m_pCheckBox_SplineView;
    QCheckBox *m_pCheckBox_ShowMask;
    QPushButton *m_pPushButton_ModifyMask;
    QPushButton *m_pPushButton_AddMask;
    QPushButton *m_pPushButton_RemoveMask;

    // FLIM calibration widgets
	QLabel *m_pLabel_PX14DcOffset;
	QSlider *m_pSlider_PX14DcOffset;

    QPushButton *m_pPushButton_CaptureBackground;
    QLineEdit *m_pLineEdit_Background;

    QLabel *m_pLabel_ChStart;
    QLabel *m_pLabel_DelayTimeOffset;
    QLabel *m_pLabel_Ch[4];
    QMySpinBox2 *m_pSpinBox_ChStart[4];
    QLineEdit *m_pLineEdit_DelayTimeOffset[3];
    QLabel *m_pLabel_NanoSec[2];

    // Histogram widgets
    QLabel *m_pLabel_FluIntensity;
    QRenderArea3 *m_pRenderArea_FluIntensity;
    QImageView *m_pColorbar_FluIntensity;
    QLabel *m_pLabel_FluIntensityMin;
    QLabel *m_pLabel_FluIntensityMax;
    QLabel *m_pLabel_FluIntensityVal[3];

    QLabel *m_pLabel_FluLifetime;
    QRenderArea3 *m_pRenderArea_FluLifetime;
    QImageView *m_pColorbar_FluLifetime;
    QLabel *m_pLabel_FluLifetimeMin;
    QLabel *m_pLabel_FluLifetimeMax;
	QLabel *m_pLabel_FluLifetimeVal[3];
};

#endif // FLIMCALIBTAB_H
