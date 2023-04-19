
#include "PulseReviewTab.h"

#include <Havana3/MainWindow.h>
#include <Havana3/Configuration.h>
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
} pulse_type;

// Labeling enumerate
typedef enum plaque_type {
	none = 0,
	normal,
	fibrous,
	loose_fibrous,
	calcification,
	focal_mac,
	lipid_mac,
	other
} plaque_type;

uint32_t plaque_type_color[9] = { 0xf0f0f0, // none
								  ML_NORMAL_COLOR, // normal
								  ML_FIBROUS_COLOR, // fibrous
								  ML_LOOSE_FIBROUS_COLOR, // loose fibrous
								  ML_CALCIFICATION_COLOR, // calcification
								  ML_MACROPHAGE_COLOR, // focal mac
								  ML_LIPID_MAC_COLOR, // lipid + mac
								  ML_SHEATH_COLOR }; // other 


PulseReviewTab::PulseReviewTab(QWidget *parent) :
    QDialog(parent), m_pConfigTemp(nullptr), 
	m_pResultTab(nullptr), m_pViewTab(nullptr), 
	m_pFLIm(nullptr), m_totalRois(0), m_bFromTable(false)
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
	//createHistogram();

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
	
	m_pLabel_PulseType = new QLabel(this);
	m_pLabel_PulseType->setText("Pulse Type");

	m_pComboBox_PulseType = new QComboBox(this);
	m_pComboBox_PulseType->addItem("Crop");
	m_pComboBox_PulseType->addItem("BG Sub");
	m_pComboBox_PulseType->addItem("Mask");
	m_pComboBox_PulseType->addItem("Spline");
	m_pComboBox_PulseType->addItem("Filter");
	m_pComboBox_PulseType->setCurrentIndex(1);
	
	m_pLabel_CurrentAline = new QLabel(this);
	QString str; str.sprintf("Current A-line : %4d / %4d (%3.2f deg)  ", 4 * (0 + 1), m_pFLIm->_resize.ny * 4,
		360.0 * (double)(4 * 0) / (double)m_pConfigTemp->octAlines);
	m_pLabel_CurrentAline->setText(str);
	m_pLabel_CurrentAline->setFixedWidth(250);

	m_pSlider_CurrentAline = new QSlider(this);
	m_pSlider_CurrentAline->setOrientation(Qt::Horizontal);
	m_pSlider_CurrentAline->setRange(0, m_pFLIm->_resize.ny - 1);
	m_pSlider_CurrentAline->setValue(int(m_pViewTab->getCurrentAline() / 4));

	m_pLabel_DelayTimeOffset = new QLabel(this);
	m_pLabel_DelayTimeOffset->setText("Delay Time Offset ");

	for (int i = 0; i < 3; i++)
	{
		m_pLineEdit_DelayTimeOffset[i] = new QLineEdit(this);
		m_pLineEdit_DelayTimeOffset[i]->setFixedWidth(65);
		m_pLineEdit_DelayTimeOffset[i]->setText(QString::number(m_pConfigTemp->flimDelayOffset[i], 'f', 3));
		m_pLineEdit_DelayTimeOffset[i]->setAlignment(Qt::AlignCenter);
	}

	m_pLabel_NanoSec = new QLabel(" nsec  ", this);
	m_pLabel_NanoSec->setFixedWidth(30);

	m_pPushButton_Update = new QPushButton(this);
	m_pPushButton_Update->setText("Update");
	m_pPushButton_Update->setFixedWidth(55);

	// Create table widget for FLIm parameters of current pixel (or roi) 
	m_pTableWidget_CurrentResult = new QTableWidget(this);
	m_pTableWidget_CurrentResult->setFixedSize(419, 114);
	m_pTableWidget_CurrentResult->clearContents();
	m_pTableWidget_CurrentResult->setRowCount(4);
	m_pTableWidget_CurrentResult->setColumnCount(3);

	QStringList rowLabels, colLabels;
	rowLabels << "Intensity (AU)" << "Lifetime (nsec)" << "IntProp (AU)" << "IntRatio (AU)";
	colLabels << "Ch 1 (390)" << "Ch 2 (450)" << "Ch 3 (540)";
	m_pTableWidget_CurrentResult->setVerticalHeaderLabels(rowLabels);
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
	m_pTableWidget_CurrentResult->setColumnWidth(0, 105);
	m_pTableWidget_CurrentResult->setColumnWidth(1, 105);
	m_pTableWidget_CurrentResult->setColumnWidth(2, 105);

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			QTableWidgetItem *pItem = new QTableWidgetItem;
			pItem->setText(QString::number(0, 'f', 3));
			pItem->setTextAlignment(Qt::AlignCenter);
			m_pTableWidget_CurrentResult->setItem(i, j, pItem);
		}
	}

	// Widgets for arc ROI labeling
	m_pToggleButton_AddRoi = new QPushButton(this);
	m_pToggleButton_AddRoi->setText("Add ROI");
	m_pToggleButton_AddRoi->setFixedWidth(80);
	m_pToggleButton_AddRoi->setCheckable(true);

	m_pToggleButton_ModifyRoi = new QPushButton(this);
	m_pToggleButton_ModifyRoi->setText("Modify ROI");
	m_pToggleButton_ModifyRoi->setFixedWidth(80);
	m_pToggleButton_ModifyRoi->setCheckable(true);
	m_pToggleButton_ModifyRoi->setDisabled(true);

	m_pPushButton_DeleteRoi = new QPushButton(this);
	m_pPushButton_DeleteRoi->setText("Delete ROI");
	m_pPushButton_DeleteRoi->setFixedWidth(80);
	m_pPushButton_DeleteRoi->setDisabled(true);

	m_pPushButton_Set = new QPushButton(this);
	m_pPushButton_Set->setText("Set");
	m_pPushButton_Set->setFixedWidth(60);
	m_pPushButton_Set->setDisabled(true);

	m_pPushButton_Cancel = new QPushButton(this);
	m_pPushButton_Cancel->setText("Cancel");
	m_pPushButton_Cancel->setFixedWidth(60);
	m_pPushButton_Cancel->setDisabled(true);

	m_pLabel_PlaqueType = new QLabel(this);
	m_pLabel_PlaqueType->setText(" Plaque Type  ");
	m_pLabel_PlaqueType->setDisabled(true);

	m_pComboBox_PlaqueType = new QComboBox(this);
	m_pComboBox_PlaqueType->addItem("None");
	m_pComboBox_PlaqueType->addItem("Normal");
	m_pComboBox_PlaqueType->addItem("Fibrous");
	m_pComboBox_PlaqueType->addItem("Loose Fibrous");
	m_pComboBox_PlaqueType->addItem("Calcification");
	m_pComboBox_PlaqueType->addItem("Focal mac");
	m_pComboBox_PlaqueType->addItem("Lipid + mac");	
	m_pComboBox_PlaqueType->addItem("Other");
	m_pComboBox_PlaqueType->setCurrentIndex(0);
	m_pComboBox_PlaqueType->setFixedWidth(120);
	m_pComboBox_PlaqueType->setDisabled(true);
		
	m_pTableWidget_RoiList = new QTableWidget(this);
	m_pTableWidget_RoiList->setFixedSize(419, 180);
	m_pTableWidget_RoiList->clearContents();
	m_pTableWidget_RoiList->setRowCount(0);
	m_pTableWidget_RoiList->setColumnCount(6);
	
	m_pLabel_Comments = new QLabel(this);
	m_pLabel_Comments->setText("   Comments  ");
	m_pLabel_Comments->setDisabled(true);

	m_pLineEdit_Comments = new QLineEdit(this);
	m_pLineEdit_Comments->setFixedWidth(80);
	m_pLineEdit_Comments->setDisabled(true);

	QStringList rowLabels1;
	rowLabels1 << "#" << "Frame" << "Start" << "End" << "Type" << "Comments";
	m_pTableWidget_RoiList->setHorizontalHeaderLabels(rowLabels1);
	QHeaderView* pVh = new QHeaderView(Qt::Vertical);
	pVh->hide();
	m_pTableWidget_RoiList->setVerticalHeader(pVh);

	m_pTableWidget_RoiList->setAlternatingRowColors(true);
	m_pTableWidget_RoiList->setSelectionMode(QAbstractItemView::SingleSelection);
	m_pTableWidget_RoiList->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_pTableWidget_RoiList->setSelectionBehavior(QAbstractItemView::SelectRows);

	m_pTableWidget_RoiList->setShowGrid(true);
	m_pTableWidget_RoiList->setGridStyle(Qt::DotLine);
	m_pTableWidget_RoiList->setSortingEnabled(true);
	m_pTableWidget_RoiList->setCornerButtonEnabled(false);

	m_pTableWidget_RoiList->verticalHeader()->setDefaultSectionSize(23);
	m_pTableWidget_RoiList->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);
	m_pTableWidget_RoiList->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	m_pTableWidget_RoiList->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	m_pTableWidget_RoiList->horizontalHeader()->setStretchLastSection(true);
	m_pTableWidget_RoiList->setColumnWidth(0, 20);
	m_pTableWidget_RoiList->setColumnWidth(1, 45);
	m_pTableWidget_RoiList->setColumnWidth(2, 45);
	m_pTableWidget_RoiList->setColumnWidth(3, 45);
	m_pTableWidget_RoiList->setColumnWidth(4, 110);

	// Set layout	
	pGridLayout_PulseView->addWidget(m_pScope_PulseView, 0, 0, 1, 4);
	pGridLayout_PulseView->addWidget(m_pCheckBox_ShowWindow, 1, 0);
	pGridLayout_PulseView->addWidget(m_pCheckBox_ShowMeanDelay, 1, 1);
	pGridLayout_PulseView->addWidget(m_pLabel_PulseType, 1, 2);
	pGridLayout_PulseView->addWidget(m_pComboBox_PulseType, 1, 3);
	pGridLayout_PulseView->addWidget(m_pLabel_CurrentAline, 2, 0, 1, 2);
	pGridLayout_PulseView->addWidget(m_pSlider_CurrentAline, 2, 2, 1, 2);

	QHBoxLayout *pHBoxLayout_DelayOffset = new QHBoxLayout;
	pHBoxLayout_DelayOffset->setSpacing(2);
	
	pHBoxLayout_DelayOffset->addWidget(m_pLabel_DelayTimeOffset);
	pHBoxLayout_DelayOffset->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	for (int i = 0; i < 3; i++)
		pHBoxLayout_DelayOffset->addWidget(m_pLineEdit_DelayTimeOffset[i]);	
	pHBoxLayout_DelayOffset->addWidget(m_pLabel_NanoSec);
	pHBoxLayout_DelayOffset->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout_DelayOffset->addWidget(m_pPushButton_Update);

	pGridLayout_PulseView->addItem(pHBoxLayout_DelayOffset, 3, 0, 1, 4);

	QVBoxLayout *pVBoxLayout_Roi = new QVBoxLayout;
	pVBoxLayout_Roi->setSpacing(2);

	QHBoxLayout *pHBoxLayout_RoiSelect = new QHBoxLayout;
	pHBoxLayout_RoiSelect->setSpacing(2);

	pHBoxLayout_RoiSelect->addWidget(m_pToggleButton_AddRoi);
	pHBoxLayout_RoiSelect->addWidget(m_pToggleButton_ModifyRoi);
	pHBoxLayout_RoiSelect->addWidget(m_pPushButton_DeleteRoi);
	pHBoxLayout_RoiSelect->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout_RoiSelect->addWidget(m_pPushButton_Set);
	pHBoxLayout_RoiSelect->addWidget(m_pPushButton_Cancel);

	QHBoxLayout *pHBoxLayout_Label = new QHBoxLayout;
	pHBoxLayout_Label->setSpacing(2);

	pHBoxLayout_Label->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout_Label->addWidget(m_pLabel_PlaqueType);
	pHBoxLayout_Label->addWidget(m_pComboBox_PlaqueType);
	pHBoxLayout_Label->addWidget(m_pLabel_Comments);
	pHBoxLayout_Label->addWidget(m_pLineEdit_Comments);

	pVBoxLayout_Roi->addItem(pHBoxLayout_RoiSelect);
	pVBoxLayout_Roi->addItem(pHBoxLayout_Label);
	pVBoxLayout_Roi->addWidget(m_pTableWidget_RoiList);

	m_pVBoxLayout->addItem(pGridLayout_PulseView);
	m_pVBoxLayout->addWidget(m_pTableWidget_CurrentResult);
	m_pVBoxLayout->addItem(pVBoxLayout_Roi);
	
	// Connect
	connect(m_pCheckBox_ShowWindow, SIGNAL(toggled(bool)), this, SLOT(showWindow(bool)));
	connect(m_pCheckBox_ShowMeanDelay, SIGNAL(toggled(bool)), this, SLOT(showMeanDelay(bool)));
	connect(m_pSlider_CurrentAline, SIGNAL(valueChanged(int)), this, SLOT(drawPulse(int)));
	connect(m_pComboBox_PulseType, SIGNAL(currentIndexChanged(int)), this, SLOT(changePulseType()));
	for (int i = 0; i < 3; i++)	
		connect(m_pLineEdit_DelayTimeOffset[i], SIGNAL(editingFinished()), this, SLOT(restoreOffsetValues()));
	connect(m_pPushButton_Update, SIGNAL(clicked(bool)), this, SLOT(updateDelayOffset()));
	connect(m_pToggleButton_AddRoi, SIGNAL(toggled(bool)), this, SLOT(addRoi(bool)));
	connect(m_pToggleButton_ModifyRoi, SIGNAL(toggled(bool)), this, SLOT(modifyRoi(bool)));
	connect(m_pPushButton_DeleteRoi, SIGNAL(clicked(bool)), this, SLOT(deleteRoi()));
	connect(m_pPushButton_Set, SIGNAL(clicked(bool)), this, SLOT(set()));
	connect(m_pPushButton_Cancel, SIGNAL(clicked(bool)), this, SLOT(cancel()));
	connect(m_pComboBox_PlaqueType, SIGNAL(currentIndexChanged(int)), this, SLOT(changePlaqueType(int)));
	connect(m_pTableWidget_RoiList, SIGNAL(currentCellChanged(int, int, int, int)), this, SLOT(selectRow(int, int, int, int)));
		
	// Load roi data
	loadRois();
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
	bool isVibCorrted = m_pResultTab->getVibCorrectionButton()->isChecked();
	int delay = !isVibCorrted ? m_pConfigTemp->flimDelaySync : 0;
	int delay_frame = delay / m_pConfigTemp->octAlines;
	int delay_aline = (delay % m_pConfigTemp->octAlines) / 4;
	
	int frame0 = m_pViewTab->getCurrentFrame();
	int frame = frame0 - delay_frame;
	frame = (frame < 0) ? 0 : frame;
	frame = (frame >= m_pConfigTemp->frames) ? m_pConfigTemp->frames - 1 : frame;

	int aline0 = aline + (m_pConfigTemp->rotatedAlines / 4);
	while (aline0 >= m_pConfig->flimAlines)
		aline0 -= m_pConfig->flimAlines;

	int aline1 = aline + (m_pViewTab->m_vibCorrIdx(frame0) / 4) - delay_aline;
	while (aline1 < 0)
		aline1 += m_pConfig->flimAlines;
	while (aline1 >= m_pConfig->flimAlines)
		aline1 -= m_pConfig->flimAlines;

	// Select pulse type		
	np::FloatArray2 pulse;
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
	//auto pulse_power = m_pViewTab->m_pulsepowerMap;
	auto intensity = m_pViewTab->m_intensityMap;
	auto mean_delay = m_pViewTab->m_meandelayMap;
	auto lifetime = m_pViewTab->m_lifetimeMap;
	auto int_prop = m_pViewTab->m_intensityProportionMap;
	auto int_ratio = m_pViewTab->m_intensityRatioMap;
	
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
				(mean_delay.at(i)(aline0, frame) - m_pFLIm->_params.ch_start_ind[0]) * factor;
	}

	// Set widget
	QString str; str.sprintf("Current A-line : %4d / %4d (%3.2f deg)  ", 4 * (aline + 1), m_pFLIm->_resize.ny * 4,
        360.0 * (double)(4 * aline) / (double)m_pConfigTemp->octAlines);
	m_pLabel_CurrentAline->setText(str);

	// Draw data
    m_pScope_PulseView->drawData(&pulse(0, aline1));

    m_pViewTab->getCircImageView()->setVerticalLine(1, 4 * aline);
    m_pViewTab->getCircImageView()->getRender()->update();

	m_pViewTab->getEnFaceImageView()->setHorizontalLine(1, aline);
	m_pViewTab->getEnFaceImageView()->getRender()->update();

	m_pViewTab->visualizeLongiImage(4 * aline);

	// Update current data 
	if (m_pViewTab->getCircImageView()->getRender()->m_nClicked != 2)
	{
		for (int i = 0; i < 3; i++)
		{
			m_pTableWidget_CurrentResult->item(0, i)->setText(QString::number(intensity.at(i)(aline0, frame), 'f', 3));
			m_pTableWidget_CurrentResult->item(1, i)->setText(QString::number(lifetime.at(i)(aline0, frame), 'f', 3));
			m_pTableWidget_CurrentResult->item(2, i)->setText(QString::number(int_prop.at(i)(aline0, frame), 'f', 3));
			m_pTableWidget_CurrentResult->item(3, i)->setText(QString::number(int_ratio.at(i)(aline0, frame), 'f', 3));
		}
	}
	else
	{
		int start4 = m_pViewTab->getCircImageView()->getRender()->m_RLines[0] / 4;
		int end4 = aline;
		bool cw = m_pViewTab->getCircImageView()->getRender()->m_bCW;
		
		for (int i = 0; i < 3; i++)
		{
			float mi, ml, mp, mr;
			float si, sl, sp, sr;
			if (!cw)
			{
				if (start4 < end4)
				{					
					ippsMeanStdDev_32f(&intensity.at(i)(start4, frame), end4 - start4 + 1, &mi, &si, ippAlgHintAccurate);
					ippsMeanStdDev_32f(&lifetime.at(i)(start4, frame), end4 - start4 + 1, &ml, &sl, ippAlgHintAccurate);
					ippsMeanStdDev_32f(&int_prop.at(i)(start4, frame), end4 - start4 + 1, &mp, &sp, ippAlgHintAccurate);
					ippsMeanStdDev_32f(&int_ratio.at(i)(start4, frame), end4 - start4 + 1, &mr, &sr, ippAlgHintAccurate);
				}
				else
				{
					FloatArray temp_arr(m_pConfigTemp->flimAlines - start4 + end4);
					
					memcpy(&temp_arr(0), &intensity.at(i)(start4, frame), sizeof(float) * (m_pConfigTemp->flimAlines - start4));
					memcpy(&temp_arr(m_pConfigTemp->flimAlines - start4), &intensity.at(i)(0, frame), sizeof(float) * end4);
					ippsMeanStdDev_32f(&temp_arr(0), temp_arr.length(), &mi, &si, ippAlgHintAccurate);

					memcpy(&temp_arr(0), &lifetime.at(i)(start4, frame), sizeof(float) * (m_pConfigTemp->flimAlines - start4));
					memcpy(&temp_arr(m_pConfigTemp->flimAlines - start4), &lifetime.at(i)(0, frame), sizeof(float) * end4);
					ippsMeanStdDev_32f(&temp_arr(0), temp_arr.length(), &ml, &sl, ippAlgHintAccurate);
					
					memcpy(&temp_arr(0), &int_prop.at(i)(start4, frame), sizeof(float) * (m_pConfigTemp->flimAlines - start4));
					memcpy(&temp_arr(m_pConfigTemp->flimAlines - start4), &int_prop.at(i)(0, frame), sizeof(float) * end4);
					ippsMeanStdDev_32f(&temp_arr(0), temp_arr.length(), &mp, &sp, ippAlgHintAccurate);

					memcpy(&temp_arr(0), &int_ratio.at(i)(start4, frame), sizeof(float) * (m_pConfigTemp->flimAlines - start4));
					memcpy(&temp_arr(m_pConfigTemp->flimAlines - start4), &int_ratio.at(i)(0, frame), sizeof(float) * end4);
					ippsMeanStdDev_32f(&temp_arr(0), temp_arr.length(), &mr, &sr, ippAlgHintAccurate);

					//float temp1, temp2;

					//ippsSum_32f(&intensity.at(i)(start4, frame), m_pConfigTemp->flimAlines - start4, &temp1, ippAlgHintAccurate);
					//ippsSum_32f(&intensity.at(i)(0, frame), end4, &temp2, ippAlgHintAccurate);
					//mi = (temp1 + temp2) / (m_pConfigTemp->flimAlines - start4 + end4);

					//ippsSum_32f(&lifetime.at(i)(start4, frame), m_pConfigTemp->flimAlines - start4, &temp1, ippAlgHintAccurate);
					//ippsSum_32f(&lifetime.at(i)(0, frame), end4, &temp2, ippAlgHintAccurate);
					//ml = (temp1 + temp2) / (m_pConfigTemp->flimAlines - start4 + end4);

					//ippsSum_32f(&int_prop.at(i)(start4, frame), m_pConfigTemp->flimAlines - start4, &temp1, ippAlgHintAccurate);
					//ippsSum_32f(&int_prop.at(i)(0, frame), end4, &temp2, ippAlgHintAccurate);
					//mp = (temp1 + temp2) / (m_pConfigTemp->flimAlines - start4 + end4);

					//ippsSum_32f(&int_ratio.at(i)(start4, frame), m_pConfigTemp->flimAlines - start4, &temp1, ippAlgHintAccurate);
					//ippsSum_32f(&int_ratio.at(i)(0, frame), end4, &temp2, ippAlgHintAccurate);
					//mr = (temp1 + temp2) / (m_pConfigTemp->flimAlines - start4 + end4);
				}
			}
			else
			{
				if (start4 > end4)
				{
					ippsMeanStdDev_32f(&intensity.at(i)(end4, frame), start4 - end4 + 1, &mi, &si, ippAlgHintAccurate);
					ippsMeanStdDev_32f(&lifetime.at(i)(end4, frame), start4 - end4 + 1, &ml, &sl, ippAlgHintAccurate);
					ippsMeanStdDev_32f(&int_prop.at(i)(end4, frame), start4 - end4 + 1, &mp, &sp, ippAlgHintAccurate);
					ippsMeanStdDev_32f(&int_ratio.at(i)(end4, frame), start4 - end4 + 1, &mr, &sr, ippAlgHintAccurate);
				}
				else
				{
					FloatArray temp_arr(m_pConfigTemp->flimAlines - end4 + start4);
					
					memcpy(&temp_arr(0), &intensity.at(i)(0, frame), sizeof(float) * start4);
					memcpy(&temp_arr(start4), &intensity.at(i)(end4, frame), sizeof(float) * (m_pConfigTemp->flimAlines - end4));
					ippsMeanStdDev_32f(&temp_arr(0), temp_arr.length(), &mi, &si, ippAlgHintAccurate);

					memcpy(&temp_arr(0), &lifetime.at(i)(0, frame), sizeof(float) * start4);
					memcpy(&temp_arr(start4), &lifetime.at(i)(end4, frame), sizeof(float) * (m_pConfigTemp->flimAlines - end4));
					ippsMeanStdDev_32f(&temp_arr(0), temp_arr.length(), &ml, &sl, ippAlgHintAccurate);

					memcpy(&temp_arr(0), &int_prop.at(i)(0, frame), sizeof(float) * start4);
					memcpy(&temp_arr(start4), &int_prop.at(i)(end4, frame), sizeof(float) * (m_pConfigTemp->flimAlines - end4));
					ippsMeanStdDev_32f(&temp_arr(0), temp_arr.length(), &mp, &sp, ippAlgHintAccurate);

					memcpy(&temp_arr(0), &int_ratio.at(i)(0, frame), sizeof(float) * start4);
					memcpy(&temp_arr(start4), &int_ratio.at(i)(end4, frame), sizeof(float) * (m_pConfigTemp->flimAlines - end4));
					ippsMeanStdDev_32f(&temp_arr(0), temp_arr.length(), &mr, &sr, ippAlgHintAccurate);

					//float temp1, temp2;
					//ippsSum_32f(&intensity.at(i)(0, frame), start4, &temp1, ippAlgHintAccurate);
					//ippsSum_32f(&intensity.at(i)(end4, frame), m_pConfigTemp->flimAlines - end4, &temp2, ippAlgHintAccurate);
					//mi = (temp1 + temp2) / (m_pConfigTemp->flimAlines - end4 + start4);

					//ippsSum_32f(&lifetime.at(i)(0, frame), start4, &temp1, ippAlgHintAccurate);
					//ippsSum_32f(&lifetime.at(i)(end4, frame), m_pConfigTemp->flimAlines - end4, &temp2, ippAlgHintAccurate);
					//ml = (temp1 + temp2) / (m_pConfigTemp->flimAlines - end4 + start4);

					//ippsSum_32f(&int_prop.at(i)(0, frame), start4, &temp1, ippAlgHintAccurate);
					//ippsSum_32f(&int_prop.at(i)(end4, frame), m_pConfigTemp->flimAlines - end4, &temp2, ippAlgHintAccurate);
					//mp = (temp1 + temp2) / (m_pConfigTemp->flimAlines - end4 + start4);

					//ippsSum_32f(&int_ratio.at(i)(0, frame), start4, &temp1, ippAlgHintAccurate);
					//ippsSum_32f(&int_ratio.at(i)(end4, frame), m_pConfigTemp->flimAlines - end4, &temp2, ippAlgHintAccurate);
					//mr = (temp1 + temp2) / (m_pConfigTemp->flimAlines - end4 + start4);
				}
			}
			
			m_pTableWidget_CurrentResult->item(0, i)->setText(QString::number(mi, 'f', 3) + " + " + QString::number(si, 'f', 3));
			m_pTableWidget_CurrentResult->item(1, i)->setText(QString::number(ml, 'f', 3) + " + " + QString::number(sl, 'f', 3));
			m_pTableWidget_CurrentResult->item(2, i)->setText(QString::number(mp, 'f', 3) + " + " + QString::number(sp, 'f', 3));
			m_pTableWidget_CurrentResult->item(3, i)->setText(QString::number(mr, 'f', 3) + " + " + QString::number(sr, 'f', 3));
		}
	}	
		
	//// Histogram
	//for (int i = 0; i < 3; i++)
	//{
	//	float* scanIntensity = &intensity.at(i)(0, frame);
	//	float* scanLifetime = &lifetime.at(i)(0, frame);

	//	(*m_pHistogramIntensity)(scanIntensity, m_pRenderArea_FluIntensity->m_pData[i],
	//		m_pConfig->flimIntensityRange[i].min, m_pConfig->flimIntensityRange[i].max);
	//	(*m_pHistogramLifetime)(scanLifetime, m_pRenderArea_FluLifetime->m_pData[i],
	//		m_pConfig->flimLifetimeRange[i].min, m_pConfig->flimLifetimeRange[i].max);

	//	Ipp32f mean, stdev;
	//	auto res = ippsMeanStdDev_32f(scanIntensity, m_pConfig->flimAlines, &mean, &stdev, ippAlgHintFast);
	//	m_pLabel_FluIntensityVal[i]->setText(QString::fromLocal8Bit("Ch%1: %2 ¡¾ %3").arg(i + 1).arg(mean, 4, 'f', 3).arg(stdev, 4, 'f', 3));
	//	res = ippsMeanStdDev_32f(scanLifetime, m_pConfig->flimAlines, &mean, &stdev, ippAlgHintFast);
	//	m_pLabel_FluLifetimeVal[i]->setText(QString::fromLocal8Bit("Ch%1: %2 ¡¾ %3").arg(i + 1).arg(mean, 4, 'f', 3).arg(stdev, 4, 'f', 3));
	//}
	//m_pRenderArea_FluIntensity->update();
	//m_pRenderArea_FluLifetime->update();

	//m_pLabel_FluIntensityMin->setText(QString::number(std::min(m_pConfig->flimIntensityRange[0].min, std::min(m_pConfig->flimIntensityRange[1].min, m_pConfig->flimIntensityRange[2].min)), 'f', 1));
	//m_pLabel_FluIntensityMax->setText(QString::number(std::max(m_pConfig->flimIntensityRange[0].max, std::max(m_pConfig->flimIntensityRange[1].max, m_pConfig->flimIntensityRange[2].max)), 'f', 1));
	//m_pLabel_FluLifetimeMin->setText(QString::number(std::min(m_pConfig->flimLifetimeRange[0].min, std::min(m_pConfig->flimLifetimeRange[1].min, m_pConfig->flimLifetimeRange[2].min)), 'f', 1));
	//m_pLabel_FluLifetimeMax->setText(QString::number(std::max(m_pConfig->flimLifetimeRange[0].max, std::max(m_pConfig->flimLifetimeRange[1].max, m_pConfig->flimLifetimeRange[2].max)), 'f', 1));

	//m_pColorbar_FluLifetime->resetColormap(ColorTable::colortable(LIFETIME_COLORTABLE));
}

void PulseReviewTab::changePulseType()
{
	drawPulse(m_pSlider_CurrentAline->value());
}

void PulseReviewTab::restoreOffsetValues()
{
	for (int i = 0; i < 3; i++)
		if (m_pLineEdit_DelayTimeOffset[i]->text() == "")
			m_pLineEdit_DelayTimeOffset[i]->setText(QString::number(m_pConfigTemp->flimDelayOffset[i], 'f', 3));
}

void PulseReviewTab::updateDelayOffset()
{
	for (int i = 0; i < 3; i++)
	{
		float delayOffset = m_pLineEdit_DelayTimeOffset[i]->text().toFloat();
		if (m_pConfigTemp->flimDelayOffset0[i] == 0.0f) // only record the original value
			m_pConfigTemp->flimDelayOffset0[i] = m_pConfigTemp->flimDelayOffset[i];

		ippsAddC_32f_I(m_pConfigTemp->flimDelayOffset[i], m_pViewTab->m_lifetimeMap.at(i).raw_ptr(), m_pViewTab->m_lifetimeMap.at(i).length());
		ippsSubC_32f_I(delayOffset, m_pViewTab->m_lifetimeMap.at(i).raw_ptr(), m_pViewTab->m_lifetimeMap.at(i).length());
		
		m_pConfigTemp->flimDelayOffset[i] = delayOffset;			
	}
	m_pResultTab->getDataProcessing()->calculateFlimParameters();
	m_pViewTab->invalidate();
	m_pConfigTemp->setConfigFile(m_pResultTab->getDataProcessing()->getIniName());
}

void PulseReviewTab::addRoi(bool toggled)
{
	QImageView *pCircView = m_pViewTab->getCircImageView();
	pCircView->getRender()->m_bArcRoiShow = false;
	pCircView->getRender()->m_bArcRoiSelect = toggled;
	pCircView->getRender()->m_nClicked = 0;

	m_pToggleButton_AddRoi->setDisabled(toggled);
	m_pToggleButton_ModifyRoi->setDisabled(true);
	m_pPushButton_DeleteRoi->setDisabled(true);
	m_pPushButton_Set->setEnabled(toggled);
	m_pPushButton_Cancel->setEnabled(toggled);
	m_pLabel_PlaqueType->setEnabled(toggled);
	m_pComboBox_PlaqueType->setEnabled(toggled);
	m_pLabel_Comments->setEnabled(toggled);
	m_pLineEdit_Comments->setEnabled(toggled);
	m_pTableWidget_RoiList->setDisabled(toggled);

	if (!toggled)
	{
		m_pComboBox_PlaqueType->setCurrentIndex(0);
		m_pLineEdit_Comments->setText("");		
	}
	pCircView->getRender()->m_colorALine = plaque_type_color[0];
	pCircView->getRender()->update();
}

void PulseReviewTab::modifyRoi(bool toggled)
{
	QImageView *pCircView = m_pViewTab->getCircImageView();
	pCircView->getRender()->m_bArcRoiShow = false;
	pCircView->getRender()->m_bArcRoiSelect = toggled;

	m_pToggleButton_AddRoi->setDisabled(toggled);
	m_pToggleButton_ModifyRoi->setDisabled(true);
	m_pPushButton_DeleteRoi->setDisabled(true);
	m_pPushButton_Set->setEnabled(toggled);
	m_pPushButton_Cancel->setEnabled(toggled);
	m_pLabel_PlaqueType->setEnabled(toggled);
	m_pComboBox_PlaqueType->setEnabled(toggled);
	m_pLabel_Comments->setEnabled(toggled);
	m_pLineEdit_Comments->setEnabled(toggled);
	m_pTableWidget_RoiList->setDisabled(toggled);

	if (toggled)
	{
		int row = m_pTableWidget_RoiList->currentRow();
		m_pComboBox_PlaqueType->setCurrentIndex(m_vectorRois.at(row).at(4).toInt());
		m_pLineEdit_Comments->setText(m_vectorRois.at(row).at(5));		
	}
	else
	{
		m_pComboBox_PlaqueType->setCurrentIndex(0);
		m_pLineEdit_Comments->setText("");
	}
	pCircView->getRender()->update();
}

void PulseReviewTab::deleteRoi()
{
	QImageView *pCircView = m_pResultTab->getViewTab()->getCircImageView();

	int row = m_pTableWidget_RoiList->currentRow();
	
	m_pToggleButton_ModifyRoi->setDisabled(true);
	m_pPushButton_DeleteRoi->setDisabled(true);
	m_pPushButton_Cancel->setDisabled(true);
	
	m_vectorRois.erase(m_vectorRois.begin() + row);

	m_pTableWidget_RoiList->removeRow(row);
	m_pTableWidget_RoiList->clearSelection();
	
	pCircView->getRender()->m_bArcRoiSelect = false;
	pCircView->getRender()->m_bArcRoiShow = false;
	pCircView->getRender()->m_nClicked = 0;
	pCircView->getRender()->update();

	saveRois();
}

void PulseReviewTab::set()
{
	QImageView *pCircView = m_pResultTab->getViewTab()->getCircImageView();

	if (m_pToggleButton_AddRoi->isChecked()) // add ROI
	{
		if (pCircView->getRender()->m_nClicked != 2)
			return;
		
		// Add roi vector
		QStringList rois;
		rois << QString::number(++m_totalRois) // 0: roi num
			<< QString::number(m_pViewTab->getCurrentFrame() + 1) // 1: frame
			<< QString::number(pCircView->getRender()->m_RLines[0]) // 2: start
			<< QString::number(pCircView->getRender()->m_RLines[1]) // 3: end
			<< QString::number(m_pComboBox_PlaqueType->currentIndex()) // 4: type
			<< m_pLineEdit_Comments->text() // 5: comments
			<< QString::number((int)pCircView->getRender()->m_bCW) // 6: cw or ccw?
			<< QString::number(m_pConfigTemp->rotatedAlines) // 7: rotated alines
			<< QString::number(m_pViewTab->m_vibCorrIdx(m_pViewTab->getCurrentFrame())); // 8: vib corr 
		
		m_vectorRois.push_back(rois);

		// Add row to table widget
		int rowCount = m_pTableWidget_RoiList->rowCount();
		m_pTableWidget_RoiList->insertRow(rowCount);

		QTableWidgetItem *pNumItem = new QTableWidgetItem;
		QTableWidgetItem *pFrameItem = new QTableWidgetItem;
		QTableWidgetItem *pStartItem = new QTableWidgetItem;
		QTableWidgetItem *pEndItem = new QTableWidgetItem;
		QTableWidgetItem *pTypeItem = new QTableWidgetItem;
		QTableWidgetItem *pCommentsItem = new QTableWidgetItem;

		pNumItem->setData(Qt::DisplayRole, rois.at(0).toInt());
		pFrameItem->setData(Qt::DisplayRole, rois.at(1).toInt());
		pStartItem->setData(Qt::DisplayRole, rois.at(2).toInt());
		pEndItem->setData(Qt::DisplayRole, rois.at(3).toInt());
		pEndItem->setToolTip(rois.at(6).toInt() == 1 ? "CW" : "CCW");
		pTypeItem->setText(m_pComboBox_PlaqueType->itemText(rois.at(4).toInt()));
		pTypeItem->setToolTip(rois.at(4));
		pCommentsItem->setText(rois.at(5));
				
		pNumItem->setTextAlignment(Qt::AlignCenter);
		pFrameItem->setTextAlignment(Qt::AlignCenter);
		pStartItem->setTextAlignment(Qt::AlignCenter);
		pEndItem->setTextAlignment(Qt::AlignCenter);
		pTypeItem->setTextAlignment(Qt::AlignCenter);
		pCommentsItem->setTextAlignment(Qt::AlignCenter);

		m_pTableWidget_RoiList->setItem(rowCount, 0, pNumItem);
		m_pTableWidget_RoiList->setItem(rowCount, 1, pFrameItem);
		m_pTableWidget_RoiList->setItem(rowCount, 2, pStartItem);
		m_pTableWidget_RoiList->setItem(rowCount, 3, pEndItem);
		m_pTableWidget_RoiList->setItem(rowCount, 4, pTypeItem);
		m_pTableWidget_RoiList->setItem(rowCount, 5, pCommentsItem);

		// Set widgets
		m_pToggleButton_AddRoi->setChecked(false);
		m_pComboBox_PlaqueType->setCurrentIndex(0);
		m_pLineEdit_Comments->setText("");
		
		pCircView->getRender()->m_nClicked = 0;
		pCircView->getRender()->update();
		m_pTableWidget_RoiList->clearSelection();		
	}
	else if (m_pToggleButton_ModifyRoi->isChecked())
	{
		if (pCircView->getRender()->m_nClicked != 2)
			return;

		// Get row & roi_num
		int rowCount = m_pTableWidget_RoiList->currentRow();
		int roi_num = m_pTableWidget_RoiList->item(rowCount, 0)->text().toInt();
		int i;
		for (i = 0; i < m_vectorRois.size(); i++)
			if (m_vectorRois.at(i).at(0).toInt() == roi_num)
				break;

		// Modify roi data
		QStringList rois_new;
		rois_new << QString::number(roi_num) // 0: roi num
			<< QString::number(m_pViewTab->getCurrentFrame() + 1) // 1: frame
			<< QString::number(pCircView->getRender()->m_RLines[0]) // 2: start
			<< QString::number(pCircView->getRender()->m_RLines[1]) // 3: end
			<< QString::number(m_pComboBox_PlaqueType->currentIndex()) // 4: type
			<< m_pLineEdit_Comments->text() // 5: comments
			<< QString::number((int)pCircView->getRender()->m_bCW) // 6: cw or ccw?
			<< QString::number(m_pConfigTemp->rotatedAlines) // 7: rotated alines
			<< QString::number(m_pViewTab->m_vibCorrIdx(m_pViewTab->getCurrentFrame())); // 8: vib corr 

		m_vectorRois.at(i) = rois_new;

		// Modify row of table widget
		m_pTableWidget_RoiList->item(rowCount, 0)->setData(Qt::DisplayRole, rois_new.at(0).toInt());
		m_pTableWidget_RoiList->item(rowCount, 1)->setData(Qt::DisplayRole, rois_new.at(1).toInt());
		m_pTableWidget_RoiList->item(rowCount, 2)->setData(Qt::DisplayRole, rois_new.at(2).toInt());
		m_pTableWidget_RoiList->item(rowCount, 3)->setData(Qt::DisplayRole, rois_new.at(3).toInt());
		m_pTableWidget_RoiList->item(rowCount, 3)->setToolTip(rois_new.at(6).toInt() == 1 ? "CW" : "CCW");
		m_pTableWidget_RoiList->item(rowCount, 4)->setText(m_pComboBox_PlaqueType->itemText(rois_new.at(4).toInt()));
		m_pTableWidget_RoiList->item(rowCount, 4)->setToolTip(rois_new.at(4));
		m_pTableWidget_RoiList->item(rowCount, 5)->setText(rois_new.at(5));

		// Set widgets
		m_pToggleButton_ModifyRoi->setChecked(false);
		m_pComboBox_PlaqueType->setCurrentIndex(0);
		m_pLineEdit_Comments->setText("");

		pCircView->getRender()->m_nClicked = 0;
		pCircView->getRender()->update();
	}

	saveRois();
}

void PulseReviewTab::cancel()
{
	QImageView *pCircView = m_pResultTab->getViewTab()->getCircImageView();

	if (m_pToggleButton_AddRoi->isChecked())
	{
		m_pToggleButton_AddRoi->setChecked(false);
		m_pTableWidget_RoiList->clearSelection();
		m_pTableWidget_RoiList->clearFocus();
	}
	else if (m_pToggleButton_ModifyRoi->isChecked())
	{
		m_pToggleButton_ModifyRoi->setChecked(false);
	}
	else
	{
		pCircView->getRender()->m_bArcRoiShow = false;
		pCircView->getRender()->m_nClicked = 0;
		m_pToggleButton_ModifyRoi->setDisabled(true);
		m_pPushButton_DeleteRoi->setDisabled(true);
		m_pPushButton_Cancel->setDisabled(true);
		m_pTableWidget_RoiList->clearSelection();
		m_pTableWidget_RoiList->clearFocus();
	}

	m_pLineEdit_Comments->setText("");
	m_pComboBox_PlaqueType->setCurrentIndex(0);
	pCircView->getRender()->update();
}

void PulseReviewTab::changePlaqueType(int type)
{
	QImageView *pCircView = m_pResultTab->getViewTab()->getCircImageView();
	pCircView->getRender()->m_colorALine = plaque_type_color[type];
	pCircView->getRender()->update();
}

void PulseReviewTab::selectRow(int row, int, int row0, int)
{
	if (row0 == -1)
		return;

	QImageView *pCircView = m_pResultTab->getViewTab()->getCircImageView();

	// Set widgets
	m_pToggleButton_ModifyRoi->setEnabled(true);
	m_pPushButton_DeleteRoi->setEnabled(true);
	m_pPushButton_Cancel->setEnabled(true);

	// Get roi data & draw
	bool isVibCorrted = m_pResultTab->getVibCorrectionButton()->isChecked();
	int delay = !isVibCorrted ? m_pConfigTemp->flimDelaySync : 0;
	int delay_frame = delay / m_pConfigTemp->octAlines;
	///int delay_aline = (delay % m_pConfigTemp->octAlines) / 4;

	int frame0 = m_pTableWidget_RoiList->item(row, 1)->data(Qt::EditRole).toInt() - 1;
	int frame = frame0 - delay_frame;
	frame = (frame < 0) ? 0 : frame;
	frame = (frame >= m_pConfigTemp->frames) ? m_pConfigTemp->frames - 1 : frame;

	int start = m_pTableWidget_RoiList->item(row, 2)->data(Qt::EditRole).toInt();
	int end = m_pTableWidget_RoiList->item(row, 3)->data(Qt::EditRole).toInt();
	bool cw = m_pTableWidget_RoiList->item(row, 3)->toolTip() == "CW";
	int type = m_pTableWidget_RoiList->item(row, 4)->toolTip().toInt();

	m_bFromTable = true;
	m_pResultTab->getViewTab()->setCurrentFrame(frame0);
	m_bFromTable = false;

	pCircView->getRender()->m_bArcRoiShow = true;
	pCircView->getRender()->m_RLines[0] = start;
	pCircView->getRender()->m_RLines[1] = end;
	pCircView->getRender()->m_colorALine = plaque_type_color[type];
	pCircView->getRender()->m_bCW = cw;
	pCircView->getRender()->m_nClicked = 2;
	pCircView->getRender()->update();
	
	drawPulse(end / 4);

	// Update current data 
	auto intensity = m_pViewTab->m_intensityMap;
	auto mean_delay = m_pViewTab->m_meandelayMap;
	auto lifetime = m_pViewTab->m_lifetimeMap;
	auto int_prop = m_pViewTab->m_intensityProportionMap;
	auto int_ratio = m_pViewTab->m_intensityRatioMap;

	int start4 = start / 4;
	int end4 = end / 4;
	for (int i = 0; i < 3; i++)
	{
		float mi, ml, mp, mr;
		float si, sl, sp, sr;

		if (!cw)
		{
			if (start4 < end4)
			{
				ippsMeanStdDev_32f(&intensity.at(i)(start4, frame), end4 - start4 + 1, &mi, &si, ippAlgHintAccurate);
				ippsMeanStdDev_32f(&lifetime.at(i)(start4, frame), end4 - start4 + 1, &ml, &sl, ippAlgHintAccurate);
				ippsMeanStdDev_32f(&int_prop.at(i)(start4, frame), end4 - start4 + 1, &mp, &sp, ippAlgHintAccurate);
				ippsMeanStdDev_32f(&int_ratio.at(i)(start4, frame), end4 - start4 + 1, &mr, &sr, ippAlgHintAccurate);
			}
			else
			{
				FloatArray temp_arr(m_pConfigTemp->flimAlines - start4 + end4);

				memcpy(&temp_arr(0), &intensity.at(i)(start4, frame), sizeof(float) * (m_pConfigTemp->flimAlines - start4));
				memcpy(&temp_arr(m_pConfigTemp->flimAlines - start4), &intensity.at(i)(0, frame), sizeof(float) * end4);
				ippsMeanStdDev_32f(&temp_arr(0), temp_arr.length(), &mi, &si, ippAlgHintAccurate);

				memcpy(&temp_arr(0), &lifetime.at(i)(start4, frame), sizeof(float) * (m_pConfigTemp->flimAlines - start4));
				memcpy(&temp_arr(m_pConfigTemp->flimAlines - start4), &lifetime.at(i)(0, frame), sizeof(float) * end4);
				ippsMeanStdDev_32f(&temp_arr(0), temp_arr.length(), &ml, &sl, ippAlgHintAccurate);

				memcpy(&temp_arr(0), &int_prop.at(i)(start4, frame), sizeof(float) * (m_pConfigTemp->flimAlines - start4));
				memcpy(&temp_arr(m_pConfigTemp->flimAlines - start4), &int_prop.at(i)(0, frame), sizeof(float) * end4);
				ippsMeanStdDev_32f(&temp_arr(0), temp_arr.length(), &mp, &sp, ippAlgHintAccurate);

				memcpy(&temp_arr(0), &int_ratio.at(i)(start4, frame), sizeof(float) * (m_pConfigTemp->flimAlines - start4));
				memcpy(&temp_arr(m_pConfigTemp->flimAlines - start4), &int_ratio.at(i)(0, frame), sizeof(float) * end4);
				ippsMeanStdDev_32f(&temp_arr(0), temp_arr.length(), &mr, &sr, ippAlgHintAccurate);

				//float temp1, temp2;
				//ippsSum_32f(&intensity.at(i)(start4, frame), m_pConfigTemp->flimAlines - start4, &temp1, ippAlgHintAccurate);
				//ippsSum_32f(&intensity.at(i)(0, frame), end4, &temp2, ippAlgHintAccurate);
				//mi = (temp1 + temp2) / (m_pConfigTemp->flimAlines - start4 + end4);

				//ippsSum_32f(&lifetime.at(i)(start4, frame), m_pConfigTemp->flimAlines - start4, &temp1, ippAlgHintAccurate);
				//ippsSum_32f(&lifetime.at(i)(0, frame), end4, &temp2, ippAlgHintAccurate);
				//ml = (temp1 + temp2) / (m_pConfigTemp->flimAlines - start4 + end4);

				//ippsSum_32f(&int_prop.at(i)(start4, frame), m_pConfigTemp->flimAlines - start4, &temp1, ippAlgHintAccurate);
				//ippsSum_32f(&int_prop.at(i)(0, frame), end4, &temp2, ippAlgHintAccurate);
				//mp = (temp1 + temp2) / (m_pConfigTemp->flimAlines - start4 + end4);

				//ippsSum_32f(&int_ratio.at(i)(start4, frame), m_pConfigTemp->flimAlines - start4, &temp1, ippAlgHintAccurate);
				//ippsSum_32f(&int_ratio.at(i)(0, frame), end4, &temp2, ippAlgHintAccurate);
				//mr = (temp1 + temp2) / (m_pConfigTemp->flimAlines - start4 + end4);
			}
		}
		else
		{
			if (start4 > end4)
			{
				ippsMeanStdDev_32f(&intensity.at(i)(end4, frame), start4 - end4 + 1, &mi, &si, ippAlgHintAccurate);
				ippsMeanStdDev_32f(&lifetime.at(i)(end4, frame), start4 - end4 + 1, &ml, &sl, ippAlgHintAccurate);
				ippsMeanStdDev_32f(&int_prop.at(i)(end4, frame), start4 - end4 + 1, &mp, &sp, ippAlgHintAccurate);
				ippsMeanStdDev_32f(&int_ratio.at(i)(end4, frame), start4 - end4 + 1, &mr, &sr, ippAlgHintAccurate);
			}
			else
			{
				FloatArray temp_arr(m_pConfigTemp->flimAlines - end4 + start4);

				memcpy(&temp_arr(0), &intensity.at(i)(0, frame), sizeof(float) * start4);
				memcpy(&temp_arr(start4), &intensity.at(i)(end4, frame), sizeof(float) * (m_pConfigTemp->flimAlines - end4));
				ippsMeanStdDev_32f(&temp_arr(0), temp_arr.length(), &mi, &si, ippAlgHintAccurate);

				memcpy(&temp_arr(0), &lifetime.at(i)(0, frame), sizeof(float) * start4);
				memcpy(&temp_arr(start4), &lifetime.at(i)(end4, frame), sizeof(float) * (m_pConfigTemp->flimAlines - end4));
				ippsMeanStdDev_32f(&temp_arr(0), temp_arr.length(), &ml, &sl, ippAlgHintAccurate);

				memcpy(&temp_arr(0), &int_prop.at(i)(0, frame), sizeof(float) * start4);
				memcpy(&temp_arr(start4), &int_prop.at(i)(end4, frame), sizeof(float) * (m_pConfigTemp->flimAlines - end4));
				ippsMeanStdDev_32f(&temp_arr(0), temp_arr.length(), &mp, &sp, ippAlgHintAccurate);

				memcpy(&temp_arr(0), &int_ratio.at(i)(0, frame), sizeof(float) * start4);
				memcpy(&temp_arr(start4), &int_ratio.at(i)(end4, frame), sizeof(float) * (m_pConfigTemp->flimAlines - end4));
				ippsMeanStdDev_32f(&temp_arr(0), temp_arr.length(), &mr, &sr, ippAlgHintAccurate);

				//float temp1, temp2;
				//ippsSum_32f(&intensity.at(i)(0, frame), start4, &temp1, ippAlgHintAccurate);
				//ippsSum_32f(&intensity.at(i)(end4, frame), m_pConfigTemp->flimAlines - end4, &temp2, ippAlgHintAccurate);
				//mi = (temp1 + temp2) / (m_pConfigTemp->flimAlines - end4 + start4);

				//ippsSum_32f(&lifetime.at(i)(0, frame), start4, &temp1, ippAlgHintAccurate);
				//ippsSum_32f(&lifetime.at(i)(end4, frame), m_pConfigTemp->flimAlines - end4, &temp2, ippAlgHintAccurate);
				//ml = (temp1 + temp2) / (m_pConfigTemp->flimAlines - end4 + start4);

				//ippsSum_32f(&int_prop.at(i)(0, frame), start4, &temp1, ippAlgHintAccurate);
				//ippsSum_32f(&int_prop.at(i)(end4, frame), m_pConfigTemp->flimAlines - end4, &temp2, ippAlgHintAccurate);
				//mp = (temp1 + temp2) / (m_pConfigTemp->flimAlines - end4 + start4);

				//ippsSum_32f(&int_ratio.at(i)(0, frame), start4, &temp1, ippAlgHintAccurate);
				//ippsSum_32f(&int_ratio.at(i)(end4, frame), m_pConfigTemp->flimAlines - end4, &temp2, ippAlgHintAccurate);
				//mr = (temp1 + temp2) / (m_pConfigTemp->flimAlines - end4 + start4);
			}
		}
		
		m_pTableWidget_CurrentResult->item(0, i)->setText(QString::number(mi, 'f', 3) + " + " + QString::number(si, 'f', 3));
		m_pTableWidget_CurrentResult->item(1, i)->setText(QString::number(ml, 'f', 3) + " + " + QString::number(sl, 'f', 3));
		m_pTableWidget_CurrentResult->item(2, i)->setText(QString::number(mp, 'f', 3) + " + " + QString::number(sp, 'f', 3));
		m_pTableWidget_CurrentResult->item(3, i)->setText(QString::number(mr, 'f', 3) + " + " + QString::number(sr, 'f', 3));
	}
}

void PulseReviewTab::saveRois()
{
	if (m_vectorRois.size() > 0)
	{
		QString roi_path = m_pResultTab->getRecordInfo().filename;
		roi_path.replace("pullback.data", "roi.csv");

		QFile roi_file(roi_path);
		if (roi_file.open(QFile::WriteOnly))
		{
			QTextStream stream(&roi_file);
			stream << "#" << "\t"
				<< "Frame" << "\t"
				<< "Start" << "\t"
				<< "End" << "\t"
				<< "Type" << "\t"
				<< "Comments" << "\t"
				<< "Clockwise" << "\t"
				<< "Rotated" << "\t"
				<< "Vib Corr" << "\n";

			for (int i = 0; i < m_vectorRois.size(); i++)
			{
				QStringList rois = m_vectorRois.at(i);

				stream << rois.at(0) << "\t" // #
					<< rois.at(1) << "\t" // Frame
					<< rois.at(2) << "\t" // Start
					<< rois.at(3) << "\t" // End
					<< rois.at(4) << "\t" // Type
					<< rois.at(5) << "\t" // Comments
					<< rois.at(6) << "\t" // CW or CCW
					<< rois.at(7) << "\t" // Rotated allines
					//<< m_pViewTab->m_vibCorrIdx(rois.at(1).toInt() - 1) << "\n"; // Vib corr
					<< rois.at(8) << "\n"; // Vib corr
			}
			stream << m_totalRois << "\n";

			roi_file.close();
		}
	}
}

void PulseReviewTab::loadRois()
{
	disconnect(m_pTableWidget_RoiList, SIGNAL(currentCellChanged(int, int, int, int)), 0, 0);

	int currentRow = m_pTableWidget_RoiList->currentRow();

	m_vectorRois.clear();
	m_pTableWidget_RoiList->clearContents();
	m_pTableWidget_RoiList->setRowCount(0);
	
	QString roi_path = m_pResultTab->getRecordInfo().filename;
	roi_path.replace("pullback.data", "roi.csv");

	QFile roi_file(roi_path);
	if (roi_file.open(QFile::ReadOnly))
	{
		QTextStream stream(&roi_file);

		stream.readLine();
		while (!stream.atEnd())
		{
			QStringList rois = stream.readLine().split('\t');
			m_vectorRois.push_back(rois);
		}
		roi_file.close();

		QStringList rois = m_vectorRois.at(m_vectorRois.size() - 1);
		m_totalRois = rois.at(0).toInt();
		m_vectorRois.pop_back();
		
		for (int i = 0; i < m_vectorRois.size(); i++)
		{
			m_pTableWidget_RoiList->insertRow(i);

			QTableWidgetItem *pNumItem = new QTableWidgetItem;
			QTableWidgetItem *pFrameItem = new QTableWidgetItem;
			QTableWidgetItem *pStartItem = new QTableWidgetItem;
			QTableWidgetItem *pEndItem = new QTableWidgetItem;
			QTableWidgetItem *pTypeItem = new QTableWidgetItem;
			QTableWidgetItem *pCommentsItem = new QTableWidgetItem;

			QStringList rois = m_vectorRois.at(i);

			int roi_num = rois.at(0).toInt();
			int frame = rois.at(1).toInt();
			int start = rois.at(2).toInt();
			int end = rois.at(3).toInt();
			QString type = m_pComboBox_PlaqueType->itemText(rois.at(4).toInt());
			QString comments = rois.at(5);
			bool clockwise = rois.at(6).toInt() == 1;
			int rotated_alines0 = rois.at(7).toInt();
			int rotated_alines = m_pConfigTemp->rotatedAlines;
			int vib_corr0 = 0, vib_corr = 0;
			if (rois.size() > 8)
			{
				vib_corr0 = rois.at(8).toInt();
				vib_corr = m_pViewTab->m_vibCorrIdx(frame - 1);
			}

			start = start + rotated_alines0 - rotated_alines + vib_corr0 - vib_corr;
			end = end + rotated_alines0 - rotated_alines + vib_corr0 - vib_corr;

			if (start < 0) start += m_pConfigTemp->octAlines;
			if (end < 0) end += m_pConfigTemp->octAlines;

			start = start % m_pConfigTemp->octAlines;
			end = end % m_pConfigTemp->octAlines;

			pNumItem->setData(Qt::DisplayRole, roi_num);
			pFrameItem->setData(Qt::DisplayRole, frame);
			pStartItem->setData(Qt::DisplayRole, start);
			pEndItem->setData(Qt::DisplayRole, end);
			pEndItem->setToolTip(clockwise ? "CW" : "CCW");
			pTypeItem->setText(type);
			pTypeItem->setToolTip(rois.at(4));
			pCommentsItem->setText(comments);

			pNumItem->setTextAlignment(Qt::AlignCenter);
			pFrameItem->setTextAlignment(Qt::AlignCenter);
			pStartItem->setTextAlignment(Qt::AlignCenter);
			pEndItem->setTextAlignment(Qt::AlignCenter);
			pTypeItem->setTextAlignment(Qt::AlignCenter);
			pCommentsItem->setTextAlignment(Qt::AlignCenter);

			m_pTableWidget_RoiList->setItem(i, 0, pNumItem);
			m_pTableWidget_RoiList->setItem(i, 1, pFrameItem);
			m_pTableWidget_RoiList->setItem(i, 2, pStartItem);
			m_pTableWidget_RoiList->setItem(i, 3, pEndItem);
			m_pTableWidget_RoiList->setItem(i, 4, pTypeItem);
			m_pTableWidget_RoiList->setItem(i, 5, pCommentsItem);
		}	

		QImageView *pCircView = m_pResultTab->getViewTab()->getCircImageView();
		pCircView->getRender()->m_nClicked = 0;
		pCircView->getRender()->update();
	}

	connect(m_pTableWidget_RoiList, SIGNAL(currentCellChanged(int, int, int, int)), this, SLOT(selectRow(int, int, int, int)));

	if (currentRow >= 0) m_pTableWidget_RoiList->selectRow(currentRow);
}