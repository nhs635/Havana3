
#include "FlimCalibTab.h"

#include <Havana3/MainWindow.h>
#include <Havana3/Configuration.h>
#include <Havana3/QStreamTab.h>

#include <Havana3/Dialog/SettingDlg.h>

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/SignatecDAQ/SignatecDAQ.h>
#include <DataAcquisition/FLImProcess/FLImProcess.h>

#include <iostream>
#include <thread>


FlimCalibTab::FlimCalibTab(QWidget *parent) :
    QDialog(parent), m_pHistogramIntensity(nullptr), m_pHistogramLifetime(nullptr)
{
    // Set main window objects
    m_pStreamTab = dynamic_cast<SettingDlg*>(parent)->getStreamTab();
    m_pConfig = m_pStreamTab->getMainWnd()->m_pConfiguration;
    m_pFLIm = m_pStreamTab->getDataAcquisition()->getFLIm();
			
    // Create layout
    m_pVBoxLayout = new QVBoxLayout;
    m_pVBoxLayout->setSpacing(15);
	setWindowTitle("FLIm Calibration");

    // Create widgets for pulse view and FLIM calibration
    createPulseView();
    createCalibWidgets();
    createHistogram();

    // Set layout
    m_pGroupBox_FlimCalibTab = new QGroupBox(this);
    m_pGroupBox_FlimCalibTab->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
    m_pGroupBox_FlimCalibTab->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    m_pGroupBox_FlimCalibTab->setLayout(m_pVBoxLayout);
	
	// Draw pulse data
	drawRoiPulse(0);
}

FlimCalibTab::~FlimCalibTab()
{
    if (m_pHistogramIntensity) delete m_pHistogramIntensity;
    if (m_pHistogramLifetime) delete m_pHistogramLifetime;
}


void FlimCalibTab::createPulseView()
{
    // Create widgets for FLIM pulse view layout
    QVBoxLayout *pVBoxLayout_PulseView = new QVBoxLayout;
	pVBoxLayout_PulseView->setSpacing(2);

    // Create widgets for FLIM pulse view
    m_pScope_PulseView = new QScope({ 0, (double)m_pFLIm->_resize.crop_src.size(0) }, { -POWER_2(12), POWER_2(16) },
                                    2, 2, m_pFLIm->_params.samp_intv, PX14_VOLT_RANGE / (double)(POWER_2(16)), 0, 0, " nsec", " V", true);
    m_pScope_PulseView->getRender()->m_bMaskUse = false;
	m_pScope_PulseView->setFixedWidth(420);
    m_pScope_PulseView->setMinimumHeight(200);
    m_pScope_PulseView->getRender()->setGrid(8, 32, 1, true);
	m_pScope_PulseView->setDcLine((float)POWER_2(12));
    //m_pScope_PulseView->setDcLine(m_pFLIm->_params.bg);

    m_pStart = &m_pScope_PulseView->getRender()->m_start;
    m_pEnd = &m_pScope_PulseView->getRender()->m_end;


    m_pCheckBox_ShowWindow = new QCheckBox(this);
    m_pCheckBox_ShowWindow->setText("Show Window  ");
    m_pCheckBox_ShowMeanDelay = new QCheckBox(this);
    m_pCheckBox_ShowMeanDelay->setText("Show Mean Delay  ");
	m_pCheckBox_ShowMeanDelay->setDisabled(true);
    m_pCheckBox_SplineView = new QCheckBox(this);
    m_pCheckBox_SplineView->setText("Spline View");

    // Create widgets for pulse mask view & modification
    m_pCheckBox_ShowMask = new QCheckBox(this);
    m_pCheckBox_ShowMask->setText("Show Mask  ");
    m_pPushButton_ModifyMask = new QPushButton(this);
    m_pPushButton_ModifyMask->setText("  Modify Mask  ");
    m_pPushButton_ModifyMask->setCheckable(true);
    m_pPushButton_ModifyMask->setDisabled(true);
    m_pPushButton_AddMask = new QPushButton(this);
    m_pPushButton_AddMask->setText("+");
    m_pPushButton_AddMask->setFixedWidth(20);
    m_pPushButton_AddMask->setDisabled(true);
    m_pPushButton_RemoveMask = new QPushButton(this);
    m_pPushButton_RemoveMask->setText("-");
    m_pPushButton_RemoveMask->setFixedWidth(20);
    m_pPushButton_RemoveMask->setDisabled(true);

    // Set layout
	pVBoxLayout_PulseView->addWidget(m_pScope_PulseView);

	QHBoxLayout *pHBoxLayout_VisOption = new QHBoxLayout;
	pHBoxLayout_VisOption->setSpacing(0);

	pHBoxLayout_VisOption->addWidget(m_pCheckBox_ShowWindow);
	pHBoxLayout_VisOption->addWidget(m_pCheckBox_ShowMeanDelay);
	pHBoxLayout_VisOption->addWidget(m_pCheckBox_SplineView);
	pHBoxLayout_VisOption->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));

	QHBoxLayout *pHBoxLayout_MaskingOption = new QHBoxLayout;
	pHBoxLayout_MaskingOption->setSpacing(0);

	pHBoxLayout_MaskingOption->addWidget(m_pCheckBox_ShowMask);
	pHBoxLayout_MaskingOption->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
	pHBoxLayout_MaskingOption->addWidget(m_pPushButton_ModifyMask);
	pHBoxLayout_MaskingOption->addWidget(m_pPushButton_AddMask);
	pHBoxLayout_MaskingOption->addWidget(m_pPushButton_RemoveMask);

	pVBoxLayout_PulseView->addItem(pHBoxLayout_VisOption);
	pVBoxLayout_PulseView->addItem(pHBoxLayout_MaskingOption);

	m_pVBoxLayout->addItem(pVBoxLayout_PulseView);

    // Connect
    connect(this, SIGNAL(plotRoiPulse(int)), this, SLOT(drawRoiPulse(int)));

    connect(m_pCheckBox_ShowWindow, SIGNAL(toggled(bool)), this, SLOT(showWindow(bool)));
    connect(m_pCheckBox_ShowMeanDelay, SIGNAL(toggled(bool)), this, SLOT(showMeanDelay(bool)));
    connect(m_pCheckBox_SplineView, SIGNAL(toggled(bool)), this, SLOT(splineView(bool)));

    connect(m_pCheckBox_ShowMask, SIGNAL(toggled(bool)), this, SLOT(showMask(bool)));
    connect(m_pPushButton_ModifyMask, SIGNAL(toggled(bool)), this, SLOT(modifyMask(bool)));
    connect(m_pPushButton_AddMask, SIGNAL(clicked(bool)), this, SLOT(addMask()));
    connect(m_pPushButton_RemoveMask, SIGNAL(clicked(bool)), this, SLOT(removeMask()));
}

void FlimCalibTab::createCalibWidgets()
{
    // Create widgets for FLIM calibration layout
	QHBoxLayout *pHBoxLayout_Background = new QHBoxLayout;
	pHBoxLayout_Background->setSpacing(2);
    QGridLayout *pGridLayout_CalibWidgets = new QGridLayout;
	pGridLayout_CalibWidgets->setSpacing(2);

    // Create widgets for FLIM calibration
#ifndef NEXT_GEN_SYSTEM
	m_pSlider_PX14DcOffset = new QSlider(this);
	m_pSlider_PX14DcOffset->setOrientation(Qt::Horizontal);
	m_pSlider_PX14DcOffset->setFixedWidth(90);
	m_pSlider_PX14DcOffset->setRange(0, 4095);
	m_pSlider_PX14DcOffset->setValue(m_pConfig->px14DcOffset);

	m_pLabel_PX14DcOffset = new QLabel(QString(" DC Offset (%1) ").arg(m_pConfig->px14DcOffset, 4, 10), this);
	m_pLabel_PX14DcOffset->setBuddy(m_pSlider_PX14DcOffset);
#endif

    m_pPushButton_CaptureBackground = new QPushButton(this);
    m_pPushButton_CaptureBackground->setText("  Capture Background  ");
    m_pLineEdit_Background = new QLineEdit(this);
    m_pLineEdit_Background->setText(QString::number(m_pFLIm->_params.bg, 'f', 2));
    m_pLineEdit_Background->setFixedWidth(60);
    m_pLineEdit_Background->setAlignment(Qt::AlignCenter);
		
    m_pLabel_ChStart = new QLabel("Channel Start  ", this);
    m_pLabel_DelayTimeOffset = new QLabel("Delay Time Offset", this);

    m_pLabel_Ch[0] = new QLabel("Ch 0 (IRF)", this);
    m_pLabel_Ch[0]->setFixedWidth(65);
    m_pLabel_Ch[0]->setAlignment(Qt::AlignCenter);
    m_pLabel_Ch[1] = new QLabel("Ch 1 (390)", this);
    m_pLabel_Ch[1]->setFixedWidth(65);
    m_pLabel_Ch[1]->setAlignment(Qt::AlignCenter);
    m_pLabel_Ch[2] = new QLabel("Ch 2 (450)", this);
    m_pLabel_Ch[2]->setFixedWidth(65);
    m_pLabel_Ch[2]->setAlignment(Qt::AlignCenter);
    m_pLabel_Ch[3] = new QLabel("Ch 3 (540)", this);
    m_pLabel_Ch[3]->setFixedWidth(65);
    m_pLabel_Ch[3]->setAlignment(Qt::AlignCenter);

    for (int i = 0; i < 4; i++)
    {
        m_pSpinBox_ChStart[i] = new QMySpinBox2(this);
        m_pSpinBox_ChStart[i]->setFixedWidth(65);
        m_pSpinBox_ChStart[i]->setRange(0.0, m_pConfig->flimScans * m_pFLIm->_params.samp_intv);
        m_pSpinBox_ChStart[i]->setSingleStep(m_pFLIm->_params.samp_intv);
        m_pSpinBox_ChStart[i]->setValue((float)m_pFLIm->_params.ch_start_ind[i] * m_pFLIm->_params.samp_intv);
        m_pSpinBox_ChStart[i]->setDecimals(2);
        m_pSpinBox_ChStart[i]->setAlignment(Qt::AlignCenter);
						
        if (i != 0)
        {
            m_pLineEdit_DelayTimeOffset[i - 1] = new QLineEdit(this);
            m_pLineEdit_DelayTimeOffset[i - 1]->setFixedWidth(65);
            m_pLineEdit_DelayTimeOffset[i - 1]->setText(QString::number(m_pFLIm->_params.delay_offset[i - 1], 'f', 3));
            m_pLineEdit_DelayTimeOffset[i - 1]->setAlignment(Qt::AlignCenter);
        }
    }
    ///resetChStart0((double)m_pFLIm->_params.ch_start_ind[0] * (double)m_pFLIm->_params.samp_intv);
    ///resetChStart1((double)m_pFLIm->_params.ch_start_ind[1] * (double)m_pFLIm->_params.samp_intv);
    ///resetChStart2((double)m_pFLIm->_params.ch_start_ind[2] * (double)m_pFLIm->_params.samp_intv);
    ///resetChStart3((double)m_pFLIm->_params.ch_start_ind[3] * (double)m_pFLIm->_params.samp_intv);
	
    m_pLabel_NanoSec[0] = new QLabel("nsec", this);
    m_pLabel_NanoSec[0]->setFixedWidth(30);
    m_pLabel_NanoSec[1] = new QLabel("nsec", this);
    m_pLabel_NanoSec[1]->setFixedWidth(30);

    // Set layout
#ifndef NEXT_GEN_SYSTEM
	pHBoxLayout_Background->addWidget(m_pLabel_PX14DcOffset);
	pHBoxLayout_Background->addWidget(m_pSlider_PX14DcOffset);
#endif
	pHBoxLayout_Background->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
    pHBoxLayout_Background->addWidget(m_pPushButton_CaptureBackground);
    pHBoxLayout_Background->addWidget(m_pLineEdit_Background);
		
	pGridLayout_CalibWidgets->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 0);
	pGridLayout_CalibWidgets->addWidget(m_pLabel_ChStart, 1, 1);
	pGridLayout_CalibWidgets->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 2, 0);
	pGridLayout_CalibWidgets->addWidget(m_pLabel_DelayTimeOffset, 2, 1, 1, 2);

    for (int i = 0; i < 4; i++)
    {
		pGridLayout_CalibWidgets->addWidget(m_pLabel_Ch[i], 0, i + 2);
		pGridLayout_CalibWidgets->addWidget(m_pSpinBox_ChStart[i], 1, i + 2);
    }
    for (int i = 0; i < 3; i++)
		pGridLayout_CalibWidgets->addWidget(m_pLineEdit_DelayTimeOffset[i], 2, i + 3);
 
	pGridLayout_CalibWidgets->addWidget(m_pLabel_NanoSec[0], 1, 6);
	pGridLayout_CalibWidgets->addWidget(m_pLabel_NanoSec[1], 2, 6);

	m_pVBoxLayout->addItem(pHBoxLayout_Background);
    m_pVBoxLayout->addItem(pGridLayout_CalibWidgets);

    // Connect
#ifndef NEXT_GEN_SYSTEM
	connect(m_pSlider_PX14DcOffset, SIGNAL(valueChanged(int)), this, SLOT(setPX14DcOffset(int)));
#endif
    connect(m_pPushButton_CaptureBackground, SIGNAL(clicked(bool)), this, SLOT(captureBackground()));
    connect(m_pLineEdit_Background, SIGNAL(textChanged(const QString &)), this, SLOT(captureBackground(const QString &)));
    connect(m_pSpinBox_ChStart[0], SIGNAL(valueChanged(double)), this, SLOT(resetChStart0(double)));
    connect(m_pSpinBox_ChStart[1], SIGNAL(valueChanged(double)), this, SLOT(resetChStart1(double)));
    connect(m_pSpinBox_ChStart[2], SIGNAL(valueChanged(double)), this, SLOT(resetChStart2(double)));
    connect(m_pSpinBox_ChStart[3], SIGNAL(valueChanged(double)), this, SLOT(resetChStart3(double)));

    for (int i = 0; i < 3; i++)
        connect(m_pLineEdit_DelayTimeOffset[i], SIGNAL(textChanged(const QString &)), this, SLOT(resetDelayTimeOffset()));
}

void FlimCalibTab::createHistogram()
{
    // Create widgets for histogram layout
    QHBoxLayout *pHBoxLayout_Histogram = new QHBoxLayout;
    pHBoxLayout_Histogram->setSpacing(2);

    uint8_t color[256];
    for (int i = 0; i < 256; i++)
        color[i] = i;

    // Create widgets for histogram (intensity)
    QGridLayout *pGridLayout_IntensityHistogram = new QGridLayout;
    pGridLayout_IntensityHistogram->setSpacing(1);

    m_pLabel_FluIntensity = new QLabel("Fluorescence Intensity (AU)", this);
	m_pLabel_FluIntensity->setFixedWidth(180);

    m_pRenderArea_FluIntensity = new QRenderArea3(this);
    m_pRenderArea_FluIntensity->setSize({ 0, (double)N_BINS }, { 0, (double)m_pConfig->flimAlines });
    m_pRenderArea_FluIntensity->setFixedSize(210, 130);

    m_pHistogramIntensity = new Histogram(N_BINS, m_pConfig->flimAlines);
	
    m_pColorbar_FluIntensity = new QImageView(ColorTable::colortable(INTENSITY_COLORTABLE), 256, 1, false, this);
    m_pColorbar_FluIntensity->drawImage(color);
    m_pColorbar_FluIntensity->getRender()->setFixedSize(210, 10);

    m_pLabel_FluIntensityMin = new QLabel(this);
	m_pLabel_FluIntensityMin->setFixedWidth(80);
    m_pLabel_FluIntensityMin->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
    m_pLabel_FluIntensityMin->setAlignment(Qt::AlignLeft);
    m_pLabel_FluIntensityMax = new QLabel(this);
	m_pLabel_FluIntensityMax->setFixedWidth(80);
    m_pLabel_FluIntensityMax->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
    m_pLabel_FluIntensityMax->setAlignment(Qt::AlignRight);
	
	for (int i = 0; i < 3; i++)
	{
		m_pLabel_FluIntensityVal[i] = new QLabel(this);
		m_pLabel_FluIntensityVal[i]->setFixedWidth(180);
		m_pLabel_FluIntensityVal[i]->setText(QString::fromLocal8Bit("Ch%1: %2 ¡¾ %3").arg(i + 1).arg(0.0, 4, 'f', 3).arg(0.0, 4, 'f', 3));
		m_pLabel_FluIntensityVal[i]->setAlignment(Qt::AlignCenter);		
	}
	m_pLabel_FluIntensityVal[0]->setStyleSheet("QLabel{color:#d900ff}");
	m_pLabel_FluIntensityVal[1]->setStyleSheet("QLabel{color:#0088ff}");
	m_pLabel_FluIntensityVal[2]->setStyleSheet("QLabel{color:#88ff00}");

    // Create widgets for histogram (lifetime)
    QGridLayout *pGridLayout_LifetimeHistogram = new QGridLayout;
    pGridLayout_LifetimeHistogram->setSpacing(1);

    m_pLabel_FluLifetime = new QLabel("Fluorescence Lifetime (nsec)", this);
	m_pLabel_FluLifetime->setFixedWidth(180);

    m_pRenderArea_FluLifetime = new QRenderArea3(this);
    m_pRenderArea_FluLifetime->setSize({ 0, (double)N_BINS }, { 0, (double)m_pConfig->flimAlines });
    m_pRenderArea_FluLifetime->setFixedSize(210, 130);

    m_pHistogramLifetime = new Histogram(N_BINS, m_pConfig->flimAlines);

    m_pColorbar_FluLifetime = new QImageView(ColorTable::colortable(LIFETIME_COLORTABLE), 256, 1, false, this);
    m_pColorbar_FluLifetime->drawImage(color);
    m_pColorbar_FluLifetime->getRender()->setFixedSize(210, 10);

    m_pLabel_FluLifetimeMin = new QLabel(this);
	m_pLabel_FluLifetimeMin->setFixedWidth(90);
    m_pLabel_FluLifetimeMin->setText(QString::number(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
    m_pLabel_FluLifetimeMin->setAlignment(Qt::AlignLeft);
    m_pLabel_FluLifetimeMax = new QLabel(this);
	m_pLabel_FluLifetimeMax->setFixedWidth(90);
    m_pLabel_FluLifetimeMax->setText(QString::number(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
    m_pLabel_FluLifetimeMax->setAlignment(Qt::AlignRight);

	for (int i = 0; i < 3; i++)
	{
		m_pLabel_FluLifetimeVal[i] = new QLabel(this);
		m_pLabel_FluLifetimeVal[i]->setFixedWidth(180);
		m_pLabel_FluLifetimeVal[i]->setText(QString::fromLocal8Bit("Ch%1: %2 ¡¾ %3").arg(i + 1).arg(0.0, 4, 'f', 3).arg(0.0, 4, 'f', 3));
		m_pLabel_FluLifetimeVal[i]->setAlignment(Qt::AlignCenter);
	}
	m_pLabel_FluLifetimeVal[0]->setStyleSheet("QLabel{color:#d900ff}");
	m_pLabel_FluLifetimeVal[1]->setStyleSheet("QLabel{color:#0088ff}");
	m_pLabel_FluLifetimeVal[2]->setStyleSheet("QLabel{color:#88ff00}");
	
    // Set layout
    pGridLayout_IntensityHistogram->addWidget(m_pLabel_FluIntensity, 0, 0, 1, 4);
    pGridLayout_IntensityHistogram->addWidget(m_pRenderArea_FluIntensity, 1, 0, 1, 4);
    pGridLayout_IntensityHistogram->addWidget(m_pColorbar_FluIntensity->getRender(), 2, 0, 1, 4);

    pGridLayout_IntensityHistogram->addWidget(m_pLabel_FluIntensityMin, 3, 0, 1, 2, Qt::AlignLeft);
    pGridLayout_IntensityHistogram->addWidget(m_pLabel_FluIntensityMax, 3, 2, 1, 2, Qt::AlignRight);

	for (int i = 0; i < 3; i++)
	    pGridLayout_IntensityHistogram->addWidget(m_pLabel_FluIntensityVal[i], 4 + i, 0, 1, 4, Qt::AlignCenter);
    

    pGridLayout_LifetimeHistogram->addWidget(m_pLabel_FluLifetime, 0, 0, 1, 4);
    pGridLayout_LifetimeHistogram->addWidget(m_pRenderArea_FluLifetime, 1, 0, 1, 4);
    pGridLayout_LifetimeHistogram->addWidget(m_pColorbar_FluLifetime->getRender(), 2, 0, 1, 4);

    pGridLayout_LifetimeHistogram->addWidget(m_pLabel_FluLifetimeMin, 3, 0, 1, 2, Qt::AlignLeft);
    pGridLayout_LifetimeHistogram->addWidget(m_pLabel_FluLifetimeMax, 3, 2, 1, 2, Qt::AlignRight);
	
	for (int i = 0; i < 3; i++)
		pGridLayout_LifetimeHistogram->addWidget(m_pLabel_FluLifetimeVal[i], 4 + i, 0, 1, 4, Qt::AlignCenter);

    pHBoxLayout_Histogram->addItem(pGridLayout_IntensityHistogram);
    pHBoxLayout_Histogram->addItem(pGridLayout_LifetimeHistogram);

    m_pVBoxLayout->addItem(pHBoxLayout_Histogram);
}


void FlimCalibTab::drawRoiPulse(int aline)
{
    // Reset pulse view (if necessary)
    static int roi_width = 0;
    np::FloatArray2 data0((!m_pCheckBox_SplineView->isChecked()) ? ((!m_pCheckBox_ShowMask->isChecked()) ? m_pFLIm->_resize.crop_src : m_pFLIm->_resize.mask_src) : m_pFLIm->_resize.filt_src);
    np::FloatArray2 data(data0.size(0), data0.size(1));
    memcpy(data.raw_ptr(), data0.raw_ptr(), sizeof(float) * data0.length());

    if (roi_width != data.size(0))
    {
        float factor = (!m_pCheckBox_SplineView->isChecked()) ? 1 : m_pFLIm->_resize.upSampleFactor;

        m_pScope_PulseView->getRender()->m_bMaskUse = true;
        m_pScope_PulseView->resetAxis({ 0, (double)data.size(0) }, { -POWER_2(12), POWER_2(16) },
			m_pFLIm->_params.samp_intv / factor, PX14_VOLT_RANGE / (double)(POWER_2(16)), 0, 0, " nsec", " V");
        m_pScope_PulseView->getRender()->m_bMaskUse = m_pCheckBox_ShowMask->isChecked();
        roi_width = data.size(0);
    }

    // Mean delay
    if (m_pCheckBox_ShowMeanDelay->isChecked())
    {
        float factor = (!m_pCheckBox_SplineView->isChecked()) ? 1 : m_pFLIm->_resize.ActualFactor;
        for (int i = 0; i < 4; i++)
            m_pScope_PulseView->getRender()->m_pMdLineInd[i] =
                    (m_pFLIm->_lifetime.mean_delay(aline, i) - m_pFLIm->_params.ch_start_ind[0]) * factor;
    }

    // ROI pulse
    //ippsAddC_32f_I(pFLIm->_params.bg, data.raw_ptr(), data.length());
    m_pScope_PulseView->drawData(&data(0, aline), m_pFLIm->_resize.pMask);

    // Histogram
	for (int i = 0; i < 3; i++)
	{
		float* scanIntensity = &m_pFLIm->_intensity.intensity(0, i + 1);
		float* scanLifetime = &m_pFLIm->_lifetime.lifetime(0, i);

		(*m_pHistogramIntensity)(scanIntensity, m_pRenderArea_FluIntensity->m_pData[i],
			m_pConfig->flimIntensityRange[i].min, m_pConfig->flimIntensityRange[i].max);
		(*m_pHistogramLifetime)(scanLifetime, m_pRenderArea_FluLifetime->m_pData[i],
			m_pConfig->flimLifetimeRange[i].min, m_pConfig->flimLifetimeRange[i].max);
		
		Ipp32f mean, stdev;
		auto res = ippsMeanStdDev_32f(scanIntensity, m_pConfig->flimAlines, &mean, &stdev, ippAlgHintFast);
		m_pLabel_FluIntensityVal[i]->setText(QString::fromLocal8Bit("Ch%1: %2 ¡¾ %3").arg(i + 1).arg(mean, 4, 'f', 3).arg(stdev, 4, 'f', 3));
		res = ippsMeanStdDev_32f(scanLifetime, m_pConfig->flimAlines, &mean, &stdev, ippAlgHintFast);
		m_pLabel_FluLifetimeVal[i]->setText(QString::fromLocal8Bit("Ch%1: %2 ¡¾ %3").arg(i + 1).arg(mean, 4, 'f', 3).arg(stdev, 4, 'f', 3));
	}
	m_pRenderArea_FluIntensity->update();
	m_pRenderArea_FluLifetime->update();
	
	m_pLabel_FluIntensityMin->setText(QString::number(std::min(m_pConfig->flimIntensityRange[0].min, std::min(m_pConfig->flimIntensityRange[1].min, m_pConfig->flimIntensityRange[2].min)), 'f', 1));
	m_pLabel_FluIntensityMax->setText(QString::number(std::max(m_pConfig->flimIntensityRange[0].max, std::max(m_pConfig->flimIntensityRange[1].max, m_pConfig->flimIntensityRange[2].max)), 'f', 1));
	m_pLabel_FluLifetimeMin->setText(QString::number(std::min(m_pConfig->flimLifetimeRange[0].min, std::min(m_pConfig->flimLifetimeRange[1].min, m_pConfig->flimLifetimeRange[2].min)), 'f', 1));
	m_pLabel_FluLifetimeMax->setText(QString::number(std::max(m_pConfig->flimLifetimeRange[0].max, std::max(m_pConfig->flimLifetimeRange[1].max, m_pConfig->flimLifetimeRange[2].max)), 'f', 1));

	m_pColorbar_FluLifetime->resetColormap(ColorTable::colortable(LIFETIME_COLORTABLE));
}

void FlimCalibTab::showWindow(bool checked)
{
    if (checked)
    {
        int* ch_ind = (!m_pCheckBox_SplineView->isChecked()) ? m_pFLIm->_params.ch_start_ind : m_pFLIm->_resize.ch_start_ind1;

        m_pScope_PulseView->setWindowLine(5,
            0, ch_ind[1] - ch_ind[0], ch_ind[2] - ch_ind[0],
               ch_ind[3] - ch_ind[0], ch_ind[4] - ch_ind[0]);
    }
    else
    {
        m_pScope_PulseView->setWindowLine(0);
    }
    m_pScope_PulseView->getRender()->update();
}

void FlimCalibTab::showMeanDelay(bool checked)
{
    if (checked)
    {
        float factor = (!m_pCheckBox_SplineView->isChecked()) ? 1 : m_pFLIm->_resize.ActualFactor;

        m_pScope_PulseView->setMeanDelayLine(4);
        for (int i = 0; i < 4; i++)
			if (m_pFLIm->_lifetime.mean_delay.length() > 0)
				m_pScope_PulseView->getRender()->m_pMdLineInd[i] =
						(m_pFLIm->_lifetime.mean_delay(0, i) - m_pFLIm->_params.ch_start_ind[0]) * factor;
    }
    else
    {
        m_pScope_PulseView->setMeanDelayLine(0);
    }
    m_pScope_PulseView->getRender()->update();
}

void FlimCalibTab::splineView(bool checked)
{
    if (m_pCheckBox_ShowWindow->isChecked())
    {
        int* ch_ind = (!checked) ? m_pFLIm->_params.ch_start_ind : m_pFLIm->_resize.ch_start_ind1;

        m_pScope_PulseView->setWindowLine(5,
            0, ch_ind[1] - ch_ind[0], ch_ind[2] - ch_ind[0],
               ch_ind[3] - ch_ind[0], ch_ind[4] - ch_ind[0]);
    }
    else
    {
        m_pScope_PulseView->setWindowLine(0);		
    }
    drawRoiPulse(0);
	
	if (!checked)
		m_pCheckBox_ShowMeanDelay->setChecked(false);
	m_pCheckBox_ShowMeanDelay->setEnabled(checked);
    m_pCheckBox_ShowMask->setDisabled(checked);
}

void FlimCalibTab::showMask(bool clicked)
{
    m_pPushButton_ModifyMask->setEnabled(clicked);
    m_pCheckBox_SplineView->setDisabled(clicked);

    m_pScope_PulseView->getRender()->m_bMaskUse = clicked;
	drawRoiPulse(0);
}

void FlimCalibTab::modifyMask(bool toggled)
{
    m_pCheckBox_ShowMask->setDisabled(toggled);
    m_pPushButton_AddMask->setEnabled(toggled);
    m_pPushButton_RemoveMask->setEnabled(toggled);

    if (!toggled) { *m_pStart = -1; *m_pEnd = -1; }

    m_pScope_PulseView->getRender()->m_bSelectionAvailable = toggled;
	drawRoiPulse(0);
}

void FlimCalibTab::addMask()
{
	RESIZE* pResize = &(m_pFLIm->_resize);

	if (*m_pStart < *m_pEnd)
	{
		for (int i = *m_pStart; i < *m_pEnd; i++)
			pResize->pMask[i] = 0.0f;
	}
	else
	{
		for (int i = *m_pEnd; i < *m_pStart; i++)
			pResize->pMask[i] = 0.0f;
	}

	memset(pResize->start_ind, 0, sizeof(int) * 4);
	memset(pResize->end_ind, 0, sizeof(int) * 4);

	int start_count = 0, end_count = 0;
	for (int i = 0; i < pResize->nx - 1; i++)
	{
		if (pResize->pMask[i + 1] - pResize->pMask[i] == -1)
		{
			start_count++;
			if (start_count < 5)
				pResize->start_ind[start_count - 1] = i - 1;
		}
		if (pResize->pMask[i + 1] - pResize->pMask[i] == 1)
		{
			end_count++;
			if (end_count < 5)
				pResize->end_ind[end_count - 1] = i;
		}
	}

	for (int i = 0; i < 4; i++)
	{
		char msg[256];
		sprintf(msg, "mask %d: [%d %d]", i + 1, pResize->start_ind[i], pResize->end_ind[i]);
		SendStatusMessage(msg);
	}

	if ((start_count == 4) && (end_count == 4))
	{
		SendStatusMessage("Proper mask is selected!!");
		m_pFLIm->saveMaskData();
	}
	else
		SendStatusMessage("Improper mask: please modify the mask!");

	*m_pStart = -1; *m_pEnd = -1;
	drawRoiPulse(0);
}

void FlimCalibTab::removeMask()
{
	RESIZE* pResize = &m_pFLIm->_resize;

	if (*m_pStart < *m_pEnd)
	{
		for (int i = *m_pStart; i < *m_pEnd; i++)
			pResize->pMask[i] = 1.0f;
	}
	else
	{
		for (int i = *m_pEnd; i < *m_pStart; i++)
			pResize->pMask[i] = 1.0f;
	}

	memset(pResize->start_ind, 0, sizeof(int) * 4);
	memset(pResize->end_ind, 0, sizeof(int) * 4);

	int start_count = 0, end_count = 0;
	for (int i = 0; i < pResize->nx - 1; i++)
	{
		if (pResize->pMask[i + 1] - pResize->pMask[i] == -1)
		{
			start_count++;
			if (start_count < 5)
				pResize->start_ind[start_count - 1] = i - 1;
		}
		if (pResize->pMask[i + 1] - pResize->pMask[i] == 1)
		{
			end_count++;
			if (end_count < 5)
				pResize->end_ind[end_count - 1] = i;
		}
	}

	for (int i = 0; i < 4; i++)
	{
		char msg[256];
		sprintf(msg, "mask %d: [%d %d]", i + 1, pResize->start_ind[i], pResize->end_ind[i]);
		SendStatusMessage(msg);
	}

	if ((start_count == 4) && (end_count == 4))
	{
		SendStatusMessage("Proper mask is selected!!");
		m_pFLIm->saveMaskData();
	}
	else
		SendStatusMessage("Improper mask: please modify the mask!");

	*m_pStart = -1; *m_pEnd = -1;
	drawRoiPulse(0);
}


void FlimCalibTab::setPX14DcOffset(int offset)
{
#ifndef NEXT_GEN_SYSTEM
	DataAcquisition *pDataAcq = m_pStreamTab->getDataAcquisition();
	pDataAcq->SetDcOffset(offset);
	
	m_pConfig->px14DcOffset = offset;
	m_pLabel_PX14DcOffset->setText(QString(" DC Offset (%1) ").arg(m_pConfig->px14DcOffset, 4, 10));
#else
	(void)offset;
#endif
}

void FlimCalibTab::captureBackground()
{
    Ipp32f bg;
    ippsMean_32f(m_pFLIm->_resize.crop_src.raw_ptr(), m_pFLIm->_resize.crop_src.size(0), &bg, ippAlgHintFast);
	bg += m_pFLIm->_params.bg;

    m_pLineEdit_Background->setText(QString::number(bg, 'f', 2));

	m_pScope_PulseView->getRender()->update();
}

void FlimCalibTab::captureBackground(const QString &str)
{
    float bg = str.toFloat();
	
    m_pFLIm->_params.bg = bg;
    m_pConfig->flimBg = bg;

    ///m_pScope_PulseView->setDcLine(bg);

	m_pScope_PulseView->getRender()->update();
}

void FlimCalibTab::resetChStart0(double start)
{
    int ch_ind = (int)round(start / m_pFLIm->_params.samp_intv);
	
    m_pFLIm->_params.ch_start_ind[0] = ch_ind;
    m_pConfig->flimChStartInd[0] = ch_ind;
	
//    printf("[Ch 0] %d %d %d %d\n",
//        m_pFLIm->_params.ch_start_ind[0], m_pFLIm->_params.ch_start_ind[1],
//        m_pFLIm->_params.ch_start_ind[2], m_pFLIm->_params.ch_start_ind[3]);

    m_pSpinBox_ChStart[1]->setMinimum((double)(ch_ind + 10) * (double)m_pFLIm->_params.samp_intv);
			
    if (m_pCheckBox_ShowWindow->isChecked())
    {
        float factor = (!m_pCheckBox_SplineView->isChecked()) ? 1 : m_pFLIm->_resize.ActualFactor;

        for (int i = 0; i < 4; i++)
            m_pScope_PulseView->getRender()->m_pWinLineInd[i] = (int)round((float)(m_pFLIm->_params.ch_start_ind[i] - ch_ind) * factor);
        m_pScope_PulseView->getRender()->m_pWinLineInd[4] = (int)round((float)(m_pFLIm->_params.ch_start_ind[3] - ch_ind + FLIM_CH_START_5) * factor);
    }

    m_pScope_PulseView->getRender()->update();
}

void FlimCalibTab::resetChStart1(double start)
{
    int ch_ind = (int)round(start / m_pFLIm->_params.samp_intv);
	
    m_pFLIm->_params.ch_start_ind[1] = ch_ind;
    m_pConfig->flimChStartInd[1] = ch_ind;

    m_pFLIm->_resize.initiated = false;

//    printf("[Ch 1] %d %d %d %d\n",
//        m_pFLIm->_params.ch_start_ind[0], m_pFLIm->_params.ch_start_ind[1],
//        m_pFLIm->_params.ch_start_ind[2], m_pFLIm->_params.ch_start_ind[3]);

    m_pSpinBox_ChStart[0]->setMaximum((double)(ch_ind - 10) * (double)m_pFLIm->_params.samp_intv);
    m_pSpinBox_ChStart[2]->setMinimum((double)(ch_ind + 10) * (double)m_pFLIm->_params.samp_intv);

    if (m_pCheckBox_ShowWindow->isChecked())
    {
        float factor = (!m_pCheckBox_SplineView->isChecked()) ? 1 : m_pFLIm->_resize.ActualFactor;
        m_pScope_PulseView->getRender()->m_pWinLineInd[1] = (int)round((float)(ch_ind - m_pFLIm->_params.ch_start_ind[0]) * factor);
    }

    m_pScope_PulseView->getRender()->update();
}

void FlimCalibTab::resetChStart2(double start)
{
    int ch_ind = (int)round(start / m_pFLIm->_params.samp_intv);

    m_pFLIm->_params.ch_start_ind[2] = ch_ind;
    m_pConfig->flimChStartInd[2] = ch_ind;

    m_pFLIm->_resize.initiated = false;

//    printf("[Ch 2] %d %d %d %d\n",
//        m_pFLIm->_params.ch_start_ind[0], m_pFLIm->_params.ch_start_ind[1],
//        m_pFLIm->_params.ch_start_ind[2], m_pFLIm->_params.ch_start_ind[3]);

    m_pSpinBox_ChStart[1]->setMaximum((double)(ch_ind - 10) * (double)m_pFLIm->_params.samp_intv);
    m_pSpinBox_ChStart[3]->setMinimum((double)(ch_ind + 10) * (double)m_pFLIm->_params.samp_intv);

    if (m_pCheckBox_ShowWindow->isChecked())
    {
        float factor = (!m_pCheckBox_SplineView->isChecked()) ? 1 : m_pFLIm->_resize.ActualFactor;
        m_pScope_PulseView->getRender()->m_pWinLineInd[2] = (int)round((float)(ch_ind - m_pFLIm->_params.ch_start_ind[0]) * factor);
    }

    m_pScope_PulseView->getRender()->update();
}

void FlimCalibTab::resetChStart3(double start)
{
    int ch_ind = (int)round(start / m_pFLIm->_params.samp_intv);

    m_pFLIm->_params.ch_start_ind[3] = ch_ind;
    m_pFLIm->_params.ch_start_ind[4] = ch_ind + FLIM_CH_START_5;
    m_pConfig->flimChStartInd[3] = ch_ind;

//    printf("[Ch 3] %d %d %d %d\n",
//        m_pFLIm->_params.ch_start_ind[0], m_pFLIm->_params.ch_start_ind[1],
//        m_pFLIm->_params.ch_start_ind[2], m_pFLIm->_params.ch_start_ind[3]);
	
    m_pSpinBox_ChStart[2]->setMaximum((double)(ch_ind - 10) * (double)m_pFLIm->_params.samp_intv);

    if (m_pCheckBox_ShowWindow->isChecked())
    {
        float factor = (!m_pCheckBox_SplineView->isChecked()) ? 1 : m_pFLIm->_resize.ActualFactor;

        m_pScope_PulseView->getRender()->m_pWinLineInd[3] = (int)round((float)(ch_ind - m_pFLIm->_params.ch_start_ind[0]) * factor);
        m_pScope_PulseView->getRender()->m_pWinLineInd[4] = (int)round((float)(ch_ind - m_pFLIm->_params.ch_start_ind[0] + FLIM_CH_START_5) * factor);
    }

    m_pScope_PulseView->getRender()->update();
}

void FlimCalibTab::resetDelayTimeOffset()
{
    float delay_offset[3];
    for (int i = 0; i < 3; i++)
    {
        delay_offset[i] = m_pLineEdit_DelayTimeOffset[i]->text().toFloat();

        m_pFLIm->_params.delay_offset[i] = delay_offset[i];
        m_pConfig->flimDelayOffset[i] = delay_offset[i];
    }
}
