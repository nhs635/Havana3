
#include "PulseReviewTab.h"

#include <Havana3/MainWindow.h>
#include <Havana3/QResultTab.h>
#include <Havana3/QViewTab.h>

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/DataProcessing.h>
#include <DataAcquisition/SignatecDAQ/SignatecDAQ.h>
#include <DataAcquisition/FLImProcess/FLImProcess.h>


static int roi_width = 0;

// Pulse type Enumerate
typedef enum pulse_type {
	cropped = 0,
	bg_subtracted,
	masked,
	spline_interpolated,
	filtered
} bypass_mode;


PulseReviewTab::PulseReviewTab(QWidget *parent) :
    QDialog(parent), m_pConfigTemp(nullptr), m_pResultTab(nullptr), m_pViewTab(nullptr), m_pFLIm(nullptr)
{
	// Set configuration objects
	m_pResultTab = dynamic_cast<QResultTab*>(parent);
	m_pViewTab = m_pResultTab->getViewTab();
	m_pConfig = m_pResultTab->getMainWnd()->m_pConfiguration;
	m_pConfigTemp = m_pResultTab->getDataProcessing()->getConfigTemp();
	m_pFLIm = m_pViewTab->getResultTab()->getDataProcessing()->getFLImProcess();

	// Create layout
	m_pVBoxLayout = new QVBoxLayout;
	m_pVBoxLayout->setSpacing(15);
	setWindowTitle("Pulse Review");

	// Create widgets for pulse and result review
	createPulseView();
	createHistogram();

	// Set layout
	m_pGroupBox_PulseReviewTab = new QGroupBox(this);
	m_pGroupBox_PulseReviewTab->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	m_pGroupBox_PulseReviewTab->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	m_pGroupBox_PulseReviewTab->setLayout(m_pVBoxLayout);
	
	// Draw pulse data
	m_pSlider_CurrentAline->valueChanged(int(m_pViewTab->getCurrentAline() / 4));
}

PulseReviewTab::~PulseReviewTab()
{
}


void PulseReviewTab::createPulseView()
{
	// Create widgets for pulse review layout
	QGridLayout *pGridLayout_PulseView = new QGridLayout;
	pGridLayout_PulseView->setSpacing(2);

	// Create widgets
	m_pScope_PulseView = new QScope({ 0, (double)m_pFLIm->_resize.crop_src.size(0) }, { -POWER_2(12), POWER_2(16) },
		2, 2, 1, PX14_VOLT_RANGE / (double)POWER_2(16), 0, 0, "", " V", false);
	m_pScope_PulseView->setFixedWidth(420);
	m_pScope_PulseView->setMinimumHeight(200);
	m_pScope_PulseView->getRender()->setGrid(8, 32, 1, true);
	m_pScope_PulseView->setDcLine((float)POWER_2(12));

	m_pCheckBox_ShowWindow = new QCheckBox(this);
	m_pCheckBox_ShowWindow->setText("Show Window  ");
	m_pCheckBox_ShowMeanDelay = new QCheckBox(this);
	m_pCheckBox_ShowMeanDelay->setText("Show Mean Delay  ");
	//m_pCheckBox_ShowMeanDelay->setDisabled(true);

	m_pLabel_CurrentAline = new QLabel(this);
	QString str; str.sprintf("Current A-line : %4d / %4d (%3.2f deg)  ", 4 * (0 + 1), m_pFLIm->_resize.ny * 4,
		360.0 * (double)(4 * 0) / (double)m_pConfigTemp->octAlines);
	m_pLabel_CurrentAline->setText(str);
	m_pLabel_CurrentAline->setFixedWidth(250);

	m_pSlider_CurrentAline = new QSlider(this);
	m_pSlider_CurrentAline->setOrientation(Qt::Horizontal);
	m_pSlider_CurrentAline->setRange(0, m_pFLIm->_resize.ny - 1);
	m_pSlider_CurrentAline->setValue(int(m_pViewTab->getCurrentAline() / 4));

	m_pLabel_PulseType = new QLabel(this);
	m_pLabel_PulseType->setText("Pulse Type");

	m_pComboBox_PulseType = new QComboBox(this);
	m_pComboBox_PulseType->addItem("Crop");
	m_pComboBox_PulseType->addItem("BG Sub");
	m_pComboBox_PulseType->addItem("Mask");
	m_pComboBox_PulseType->addItem("Spline");
	m_pComboBox_PulseType->addItem("Filter");

	m_pTableWidget_CurrentResult = new QTableWidget(this);
	m_pTableWidget_CurrentResult->setFixedSize(419, 91);
	m_pTableWidget_CurrentResult->clearContents();
	m_pTableWidget_CurrentResult->setRowCount(3);
	m_pTableWidget_CurrentResult->setColumnCount(4);

	QStringList rawLabels, colLabels;
	rawLabels << "Intensity (AU)" << "Mean Delay (nsec)" << "Lifetime (nsec)";
	colLabels << "Ch 0 (IRF)" << "Ch 1 (390)" << "Ch 2 (450)" << "Ch 3 (540)";
	m_pTableWidget_CurrentResult->setVerticalHeaderLabels(rawLabels);
	m_pTableWidget_CurrentResult->setHorizontalHeaderLabels(colLabels);

	m_pTableWidget_CurrentResult->setSelectionMode(QAbstractItemView::NoSelection);
	m_pTableWidget_CurrentResult->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_pTableWidget_CurrentResult->setSelectionBehavior(QAbstractItemView::SelectItems);

	m_pTableWidget_CurrentResult->setShowGrid(true);
	m_pTableWidget_CurrentResult->setGridStyle(Qt::DotLine);
	m_pTableWidget_CurrentResult->setSortingEnabled(false);
	m_pTableWidget_CurrentResult->setCornerButtonEnabled(false);

	m_pTableWidget_CurrentResult->verticalHeader()->setDefaultSectionSize(23);
	m_pTableWidget_CurrentResult->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);
	m_pTableWidget_CurrentResult->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	m_pTableWidget_CurrentResult->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	m_pTableWidget_CurrentResult->setColumnWidth(0, 73);
	m_pTableWidget_CurrentResult->setColumnWidth(1, 73);
	m_pTableWidget_CurrentResult->setColumnWidth(2, 73);
	m_pTableWidget_CurrentResult->setColumnWidth(3, 73);

	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			QTableWidgetItem *pItem = new QTableWidgetItem;
			pItem->setText(QString::number(0, 'f', 3));
			pItem->setTextAlignment(Qt::AlignCenter);
			m_pTableWidget_CurrentResult->setItem(i, j, pItem);
		}
	}
	m_pTableWidget_CurrentResult->item(0, 0)->setText("-");
	m_pTableWidget_CurrentResult->item(2, 0)->setText("-");

	// Set layout	
	pGridLayout_PulseView->addWidget(m_pScope_PulseView, 0, 0, 1, 4);
	pGridLayout_PulseView->addWidget(m_pCheckBox_ShowWindow, 1, 0);
	pGridLayout_PulseView->addWidget(m_pCheckBox_ShowMeanDelay, 1, 1);
	pGridLayout_PulseView->addWidget(m_pLabel_PulseType, 1, 2);
	pGridLayout_PulseView->addWidget(m_pComboBox_PulseType, 1, 3);
	pGridLayout_PulseView->addWidget(m_pLabel_CurrentAline, 2, 0, 1, 2);
	pGridLayout_PulseView->addWidget(m_pSlider_CurrentAline, 2, 2, 1, 2);

	m_pVBoxLayout->addItem(pGridLayout_PulseView);
	m_pVBoxLayout->addWidget(m_pTableWidget_CurrentResult);
	
	// Connect
	connect(m_pCheckBox_ShowWindow, SIGNAL(toggled(bool)), this, SLOT(showWindow(bool)));
	connect(m_pCheckBox_ShowMeanDelay, SIGNAL(toggled(bool)), this, SLOT(showMeanDelay(bool)));
	connect(m_pSlider_CurrentAline, SIGNAL(valueChanged(int)), this, SLOT(drawPulse(int)));
	connect(m_pComboBox_PulseType, SIGNAL(currentIndexChanged(int)), this, SLOT(changeType()));
}

void PulseReviewTab::createHistogram()
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
	m_pRenderArea_FluIntensity->setSize({ 0, (double)N_BINS }, { 0, (double)m_pConfigTemp->flimAlines });
	m_pRenderArea_FluIntensity->setFixedSize(210, 130);

	m_pHistogramIntensity = new Histogram(N_BINS, m_pConfigTemp->flimAlines);

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
	m_pRenderArea_FluLifetime->setSize({ 0, (double)N_BINS }, { 0, (double)m_pConfigTemp->flimAlines });
	m_pRenderArea_FluLifetime->setFixedSize(210, 130);

	m_pHistogramLifetime = new Histogram(N_BINS, m_pConfigTemp->flimAlines);

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


void PulseReviewTab::showWindow(bool checked)
{
	if (checked)
	{
		int* ch_ind = (m_pComboBox_PulseType->currentIndex() <= 2) ? m_pFLIm->_params.ch_start_ind : m_pFLIm->_resize.ch_start_ind1;

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

void PulseReviewTab::showMeanDelay(bool checked)
{
	if (checked)
	{
		float factor = (m_pComboBox_PulseType->currentIndex() <= 2) ? 1 : m_pFLIm->_resize.ActualFactor;

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

void PulseReviewTab::drawPulse(int aline)
{
	// Select pulse type
	np::FloatArray2 pulse;
	int frame = m_pViewTab->getCurrentFrame();
	switch (m_pComboBox_PulseType->currentIndex())
	{
	case cropped:
		pulse = m_pViewTab->m_vectorPulseCrop.at(frame);
		break;
	case bg_subtracted:
		pulse = m_pViewTab->m_vectorPulseBgSub.at(frame);
		break;
	case masked:
		pulse = m_pViewTab->m_vectorPulseMask.at(frame);
		break;
	case spline_interpolated:
		pulse = m_pViewTab->m_vectorPulseSpline.at(frame);
		break;
	case filtered:
		pulse = m_pViewTab->m_vectorPulseFilter.at(frame);
		break;
	default:
		break;
	}

	// Reset pulse view scale
	if (roi_width != pulse.size(0))
	{		
		m_pScope_PulseView->resetAxis({ 0, (double)pulse.size(0) }, { -POWER_2(12), POWER_2(16) },
			1, PX14_VOLT_RANGE / (double)(POWER_2(16)), 0, 0, "", " V");
		roi_width = pulse.size(0);
	}

	// Data
	auto intensity = m_pViewTab->m_intensityMap;
	auto mean_delay = m_pViewTab->m_meandelayMap;
	auto lifetime = m_pViewTab->m_lifetimeMap;
	
	// Window
	if (m_pCheckBox_ShowWindow->isChecked())
	{
		int* ch_ind = (m_pComboBox_PulseType->currentIndex() <= 2) ? m_pFLIm->_params.ch_start_ind : m_pFLIm->_resize.ch_start_ind1;
		m_pScope_PulseView->setWindowLine(5,
			0, ch_ind[1] - ch_ind[0], ch_ind[2] - ch_ind[0],
			ch_ind[3] - ch_ind[0], ch_ind[4] - ch_ind[0]);
	}

	// Mean delay
	if (m_pCheckBox_ShowMeanDelay->isChecked())
	{
		float factor = (m_pComboBox_PulseType->currentIndex() <= 2) ? 1 : m_pFLIm->_resize.ActualFactor;
		
		m_pScope_PulseView->setMeanDelayLine(4);
		for (int i = 0; i < 4; i++)
			m_pScope_PulseView->getRender()->m_pMdLineInd[i] =
				(mean_delay.at(i)(aline, frame) - m_pFLIm->_params.ch_start_ind[0]) * factor;
	}

	// Set widget
	QString str; str.sprintf("Current A-line : %4d / %4d (%3.2f deg)  ", 4 * (aline + 1), m_pFLIm->_resize.ny * 4,
        360.0 * (double)(4 * aline) / (double)m_pConfigTemp->octAlines);
	m_pLabel_CurrentAline->setText(str);

	// Draw data
    m_pScope_PulseView->drawData(&pulse(0, aline));

    m_pViewTab->getCircImageView()->setVerticalLine(1, 4 * aline);
    m_pViewTab->getCircImageView()->getRender()->update();

	m_pViewTab->getEnFaceImageView()->setHorizontalLine(1, aline);
	m_pViewTab->getEnFaceImageView()->getRender()->update();

	// Update current data
	for (int i = 0; i < 4; i++)
	{
		if (i > 0) m_pTableWidget_CurrentResult->item(0, i)->setText(QString::number(intensity.at(i - 1)(aline, frame), 'f', 3));
		m_pTableWidget_CurrentResult->item(1, i)->setText(QString::number(mean_delay.at(i)(aline, frame), 'f', 3));
		if (i > 0) m_pTableWidget_CurrentResult->item(2, i)->setText(QString::number(lifetime.at(i - 1)(aline, frame), 'f', 3));
	}

	// Histogram
	for (int i = 0; i < 3; i++)
	{
		float* scanIntensity = &intensity.at(i)(0, frame);
		float* scanLifetime = &lifetime.at(i)(0, frame);

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

void PulseReviewTab::changeType()
{
	drawPulse(m_pSlider_CurrentAline->value());
}
