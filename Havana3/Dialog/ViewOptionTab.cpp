
#include "ViewOptionTab.h"

#include <Havana3/MainWindow.h>
#include <Havana3/Configuration.h>
#include <Havana3/QStreamTab.h>
#include <Havana3/QResultTab.h>
#include <Havana3/QViewTab.h>
//#include <Havana3/QProcessingTab.h>

#include <Havana3/Dialog/SettingDlg.h>
#include <Havana3/Viewer/QImageView.h>

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/FLImProcess/FLImProcess.h>

#include <ippcore.h>
#include <ippvm.h>
#include <ipps.h>

#define CIRCULAR_VIEW		-2
#define RECTANGULAR_VIEW	-3


ViewOptionTab::ViewOptionTab(bool is_streaming, QWidget *parent) :
    QDialog(parent), m_pStreamTab(nullptr), m_pResultTab(nullptr), m_pViewTab(nullptr)
{
    // Set configuration objects
	if (is_streaming)
	{
        m_pStreamTab = dynamic_cast<SettingDlg*>(parent)->getStreamTab();
        m_pConfig = m_pStreamTab->getMainWnd()->m_pConfiguration;
        m_pViewTab = m_pStreamTab->getViewTab();
	}
	else
	{
        m_pResultTab = dynamic_cast<SettingDlg*>(parent)->getResultTab();
        m_pConfig = m_pResultTab->getMainWnd()->m_pConfiguration;
        m_pViewTab = m_pResultTab->getViewTab();
	}

    // Create FLIM visualization option tab
    createFlimVisualizationOptionTab(is_streaming);

    // Create OCT visualization option tab
    createOctVisualizationOptionTab(is_streaming);
	
    // Set layout
    m_pGroupBox_ViewOption = new QGroupBox;
    m_pGroupBox_ViewOption->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
    m_pGroupBox_ViewOption->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    QVBoxLayout *pVBoxLayout = new QVBoxLayout;
    pVBoxLayout->setSpacing(1);

    pVBoxLayout->addWidget(m_pGroupBox_FlimVisualization);
    pVBoxLayout->addWidget(m_pGroupBox_OctVisualization);
    pVBoxLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));

    m_pGroupBox_ViewOption->setLayout(pVBoxLayout);
}

ViewOptionTab::~ViewOptionTab()
{
}


void ViewOptionTab::setWidgetsValue()
{
	m_pComboBox_EmissionChannel->setCurrentIndex(m_pConfig->flimEmissionChannel - 1);
	
	if (m_pStreamTab)
	{
		m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
		m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
	}
	else if (m_pResultTab)
	{
		if (!m_pCheckBox_IntensityRatio->isChecked())
		{
			m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
			m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
		}
		else
		{
			m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1]
				[m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]].max, 'f', 1));
			m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1]
				[m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]].min, 'f', 1));
		}
	}
	m_pLineEdit_LifetimeMax->setText(QString::number(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
	m_pLineEdit_LifetimeMin->setText(QString::number(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
}


void ViewOptionTab::createFlimVisualizationOptionTab(bool is_streaming)
{
    // Create widgets for FLIm visualization option tab
    m_pGroupBox_FlimVisualization = new QGroupBox;
    m_pGroupBox_FlimVisualization->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	m_pGroupBox_FlimVisualization->setTitle("  FLIm Visualization  ");
	m_pGroupBox_FlimVisualization->setStyleSheet("QGroupBox { padding-top: 15px; margin-top: -10px; } QGroupBox::title { subcontrol-origin: margin; left: 8px; top: 2px; }");
	m_pGroupBox_FlimVisualization->setEnabled(is_streaming);
    QGridLayout *pGridLayout_FlimVisualization = new QGridLayout;
    pGridLayout_FlimVisualization->setSpacing(3);

    // Create widgets for FLIm emission control
    m_pComboBox_EmissionChannel = new QComboBox(this);
    m_pComboBox_EmissionChannel->addItem("Ch 1");
    m_pComboBox_EmissionChannel->addItem("Ch 2");
    m_pComboBox_EmissionChannel->addItem("Ch 3");
    m_pComboBox_EmissionChannel->setCurrentIndex(m_pConfig->flimEmissionChannel - 1);
    m_pComboBox_EmissionChannel->setFixedWidth(60);
	m_pComboBox_EmissionChannel->setEnabled(is_streaming);
    m_pLabel_EmissionChannel = new QLabel("FLIm Emission Channel ", this);
    m_pLabel_EmissionChannel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    m_pLabel_EmissionChannel->setBuddy(m_pComboBox_EmissionChannel);
	m_pLabel_EmissionChannel->setEnabled(is_streaming);
		
	if (!is_streaming)
	{
		// Create widgets for FLIm intensity ratio image
		m_pCheckBox_IntensityRatio = new QCheckBox(this);
		m_pCheckBox_IntensityRatio->setText("Intensity Ratio Map");
		m_pCheckBox_IntensityRatio->setEnabled(is_streaming);

		m_pComboBox_IntensityRef = new QComboBox(this);
		for (int i = 0; i < 3; i++)
			if (i != m_pConfig->flimEmissionChannel - 1)
				m_pComboBox_IntensityRef->addItem(QString("Ch %1").arg(i + 1));
		m_pComboBox_IntensityRef->setCurrentIndex(m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]);
		m_pComboBox_IntensityRef->setFixedWidth(50);
		m_pComboBox_IntensityRef->setEnabled(is_streaming);

		// Create widgets for FLIm intensity-weighted lifetime map	
		m_pCheckBox_IntensityWeightedLifetimeMap = new QCheckBox(this);
		m_pCheckBox_IntensityWeightedLifetimeMap->setText("Intensity Weighted Lifetime Map");
		m_pCheckBox_IntensityWeightedLifetimeMap->setEnabled(is_streaming);

		// Create widgets for FLIm pulse reivew dialog
		m_pPushButton_PulseReview = new QPushButton(this);
		m_pPushButton_PulseReview->setText("Pulse Review...");
		m_pPushButton_PulseReview->setFixedWidth(110);
		m_pPushButton_PulseReview->setEnabled(is_streaming);

		// Create widgets for FLIm based classification 
		m_pCheckBox_Classification = new QCheckBox(this);
		m_pCheckBox_Classification->setText("Classification");
		m_pCheckBox_Classification->setDisabled(true);
	}

	if (is_streaming)
	{
		// Create line edit widgets for FLIm contrast adjustment
		m_pLineEdit_IntensityMax = new QLineEdit(this);
        m_pLineEdit_IntensityMax->setFixedWidth(35);
		m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
		m_pLineEdit_IntensityMax->setAlignment(Qt::AlignCenter);
		m_pLineEdit_IntensityMin = new QLineEdit(this);
        m_pLineEdit_IntensityMin->setFixedWidth(35);
		m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
		m_pLineEdit_IntensityMin->setAlignment(Qt::AlignCenter);
		m_pLineEdit_LifetimeMax = new QLineEdit(this);
        m_pLineEdit_LifetimeMax->setFixedWidth(35);
		m_pLineEdit_LifetimeMax->setText(QString::number(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
		m_pLineEdit_LifetimeMax->setAlignment(Qt::AlignCenter);
		m_pLineEdit_LifetimeMin = new QLineEdit(this);
        m_pLineEdit_LifetimeMin->setFixedWidth(35);
		m_pLineEdit_LifetimeMin->setText(QString::number(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
		m_pLineEdit_LifetimeMin->setAlignment(Qt::AlignCenter);

		// Create color bar for FLIM visualization
		uint8_t color[256];
		for (int i = 0; i < 256; i++)
			color[i] = i;

		m_pImageView_IntensityColorbar = new QImageView(ColorTable::colortable(INTENSITY_COLORTABLE), 256, 1);
        m_pImageView_IntensityColorbar->setFixedSize(180, 20);
		m_pImageView_IntensityColorbar->drawImage(color);
		m_pImageView_LifetimeColorbar = new QImageView(ColorTable::colortable(LIFETIME_COLORTABLE), 256, 1);
        m_pImageView_LifetimeColorbar->setFixedSize(180, 20);
		m_pImageView_LifetimeColorbar->drawImage(color);
		m_pLabel_NormIntensity = new QLabel(QString("Ch%1 Intensity (AU) ").arg(m_pConfig->flimEmissionChannel), this);
        m_pLabel_NormIntensity->setFixedWidth(120);
		m_pLabel_NormIntensity->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
		m_pLabel_Lifetime = new QLabel(QString("Ch%1 Lifetime (nsec) ").arg(m_pConfig->flimEmissionChannel), this);
        m_pLabel_Lifetime->setFixedWidth(120);
		m_pLabel_Lifetime->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
	}

    // Set layout
    QHBoxLayout *pHBoxLayout_FlimVisualization1 = new QHBoxLayout;
	pHBoxLayout_FlimVisualization1->setSpacing(3);
    pHBoxLayout_FlimVisualization1->addWidget(m_pLabel_EmissionChannel);
    pHBoxLayout_FlimVisualization1->addWidget(m_pComboBox_EmissionChannel);

	QGridLayout *pGridLayout_FlimVisualization2 = new QGridLayout;
	QHBoxLayout *pHBoxLayout_IntensityColorbar = new QHBoxLayout;
	QHBoxLayout *pHBoxLayout_LifetimeColorbar = new QHBoxLayout;

	if (!is_streaming)
	{
		pGridLayout_FlimVisualization2->setSpacing(3);
		pGridLayout_FlimVisualization2->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 0);
		pGridLayout_FlimVisualization2->addWidget(m_pCheckBox_Classification, 0, 1);
		pGridLayout_FlimVisualization2->addWidget(new QLabel(" ", this), 0, 2);
		pGridLayout_FlimVisualization2->addWidget(m_pCheckBox_IntensityRatio, 0, 3);
		pGridLayout_FlimVisualization2->addWidget(m_pComboBox_IntensityRef, 0, 4);

		pGridLayout_FlimVisualization2->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 0);
		pGridLayout_FlimVisualization2->addWidget(m_pPushButton_PulseReview, 1, 1);
		pGridLayout_FlimVisualization2->addWidget(new QLabel(" ", this), 1, 2);
		pGridLayout_FlimVisualization2->addWidget(m_pCheckBox_IntensityWeightedLifetimeMap, 1, 3, 1, 2);
	}
	else
	{
		pHBoxLayout_IntensityColorbar->setSpacing(1);
		pHBoxLayout_IntensityColorbar->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
		pHBoxLayout_IntensityColorbar->addWidget(m_pLabel_NormIntensity);
		pHBoxLayout_IntensityColorbar->addWidget(m_pLineEdit_IntensityMin);
		pHBoxLayout_IntensityColorbar->addWidget(m_pImageView_IntensityColorbar);
		pHBoxLayout_IntensityColorbar->addWidget(m_pLineEdit_IntensityMax);

		pHBoxLayout_LifetimeColorbar->setSpacing(1);
		pHBoxLayout_LifetimeColorbar->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
		pHBoxLayout_LifetimeColorbar->addWidget(m_pLabel_Lifetime);
		pHBoxLayout_LifetimeColorbar->addWidget(m_pLineEdit_LifetimeMin);
		pHBoxLayout_LifetimeColorbar->addWidget(m_pImageView_LifetimeColorbar);
		pHBoxLayout_LifetimeColorbar->addWidget(m_pLineEdit_LifetimeMax);
	}
	  
	if (!is_streaming)
	{
		pGridLayout_FlimVisualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 0);
		pGridLayout_FlimVisualization->addItem(pHBoxLayout_FlimVisualization1, 0, 1);
		pGridLayout_FlimVisualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 0);
		pGridLayout_FlimVisualization->addItem(pGridLayout_FlimVisualization2, 1, 1);
	}
	else
	{
		pGridLayout_FlimVisualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 0);
		pGridLayout_FlimVisualization->addItem(pHBoxLayout_FlimVisualization1, 0, 1);
		pGridLayout_FlimVisualization->addItem(pHBoxLayout_IntensityColorbar, 1, 0, 1, 2);
		pGridLayout_FlimVisualization->addItem(pHBoxLayout_LifetimeColorbar, 2, 0, 1, 2);
	}

    m_pGroupBox_FlimVisualization->setLayout(pGridLayout_FlimVisualization);

    // Connect signal and slot
    connect(m_pComboBox_EmissionChannel, SIGNAL(currentIndexChanged(int)), this, SLOT(changeEmissionChannel(int)));
	if (!is_streaming)
	{
		connect(m_pCheckBox_IntensityRatio, SIGNAL(toggled(bool)), this, SLOT(enableIntensityRatioMode(bool)));
		connect(m_pComboBox_IntensityRef, SIGNAL(currentIndexChanged(int)), this, SLOT(changeIntensityRatioRef(int)));
		connect(m_pCheckBox_IntensityWeightedLifetimeMap, SIGNAL(toggled(bool)), this, SLOT(enableIntensityWeightingMode(bool)));
		connect(m_pPushButton_PulseReview, SIGNAL(clicked(bool)), this, SLOT(createPulseReviewDlg()));
		connect(m_pCheckBox_Classification, SIGNAL(toggled(bool)), this, SLOT(enableClassification(bool)));
	}
	else
	{
		connect(m_pLineEdit_IntensityMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
		connect(m_pLineEdit_IntensityMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
		connect(m_pLineEdit_LifetimeMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
		connect(m_pLineEdit_LifetimeMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
	}
}

void ViewOptionTab::createOctVisualizationOptionTab(bool is_streaming)
{
    // Create widgets for OCT visualization option tab
    m_pGroupBox_OctVisualization = new QGroupBox;
    m_pGroupBox_OctVisualization->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	m_pGroupBox_OctVisualization->setTitle("  OCT Visualization  ");
	m_pGroupBox_OctVisualization->setStyleSheet("QGroupBox { padding-top: 20px; margin-top: -10px; } QGroupBox::title { subcontrol-origin: margin; left: 8px; top: 2px; }");
	m_pGroupBox_OctVisualization->setEnabled(is_streaming);
    QGridLayout *pGridLayout_OctVisualization = new QGridLayout;
    pGridLayout_OctVisualization->setSpacing(3);

    // Create widgets for OCT visualization
	m_pRadioButton_CircularView = new QRadioButton(this);
    m_pRadioButton_CircularView->setText("Circular View (x-y) ");
	m_pRadioButton_CircularView->setEnabled(is_streaming);
	m_pRadioButton_RectangularView = new QRadioButton(this);
	m_pRadioButton_RectangularView->setText(QString::fromLocal8Bit("Rectangular View (еш-r)"));
	m_pRadioButton_RectangularView->setEnabled(is_streaming);

	m_pButtonGroup_ViewMode = new QButtonGroup(this);
	m_pButtonGroup_ViewMode->addButton(m_pRadioButton_CircularView, CIRCULAR_VIEW);
	m_pButtonGroup_ViewMode->addButton(m_pRadioButton_RectangularView, RECTANGULAR_VIEW);
	m_pRadioButton_CircularView->setChecked(true);
		
	// Create widgets for OCT cross section rotation
	if (!is_streaming)
	{
		m_pScrollBar_Rotation = new QScrollBar(this);
		m_pScrollBar_Rotation->setOrientation(Qt::Horizontal);
		m_pScrollBar_Rotation->setRange(0, m_pConfig->octAlines - 1);
		m_pScrollBar_Rotation->setSingleStep(1);
		m_pScrollBar_Rotation->setPageStep(m_pScrollBar_Rotation->maximum() / 10);
		m_pScrollBar_Rotation->setFocusPolicy(Qt::StrongFocus);
		m_pScrollBar_Rotation->setDisabled(true);

		QString str; str.sprintf("Rotation %4d / %4d ", 0, m_pConfig->octAlines - 1);
		m_pLabel_Rotation = new QLabel(str, this);
		m_pLabel_Rotation->setBuddy(m_pScrollBar_Rotation);
		m_pLabel_Rotation->setFixedWidth(110);
		m_pLabel_Rotation->setDisabled(true);
	}

	// Create line edit widgets for OCT contrast adjustment
	if (is_streaming)
	{
		m_pLineEdit_DecibelMax = new QLineEdit(this);
        m_pLineEdit_DecibelMax->setFixedWidth(35);
		m_pLineEdit_DecibelMax->setText(QString::number(m_pConfig->axsunDbRange.max));
		m_pLineEdit_DecibelMax->setAlignment(Qt::AlignCenter);
		m_pLineEdit_DecibelMax->setDisabled(true);
		m_pLineEdit_DecibelMin = new QLineEdit(this);
        m_pLineEdit_DecibelMin->setFixedWidth(35);
		m_pLineEdit_DecibelMin->setText(QString::number(m_pConfig->axsunDbRange.min));
		m_pLineEdit_DecibelMin->setAlignment(Qt::AlignCenter);
		m_pLineEdit_DecibelMin->setDisabled(true);
	}

	// Create color bar for OCT visualization
	uint8_t color[256];
	for (int i = 0; i < 256; i++)
		color[i] = (uint8_t)i;

	m_pImageView_Colorbar = new QImageView(ColorTable::colortable(OCT_COLORTABLE), 256, 1);
    m_pImageView_Colorbar->setFixedSize(180, 20);
	m_pImageView_Colorbar->drawImage(color);
	m_pImageView_Colorbar->setDisabled(true);
	m_pLabel_DecibelRange = new QLabel("OCT Contrast (dB) ", this);
    m_pLabel_DecibelRange->setFixedWidth(120);
	m_pLabel_DecibelRange->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
	m_pLabel_DecibelRange->setBuddy(m_pImageView_Colorbar);
	m_pLabel_DecibelRange->setDisabled(true);
	
    // Set layout
    pGridLayout_OctVisualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 0);
    pGridLayout_OctVisualization->addWidget(m_pRadioButton_CircularView, 0, 1);
	pGridLayout_OctVisualization->addWidget(m_pRadioButton_RectangularView, 0, 2);
	
	if (is_streaming)
	{
		QHBoxLayout *pHBoxLayout_Decibel = new QHBoxLayout;
		pHBoxLayout_Decibel->setSpacing(1);

		pHBoxLayout_Decibel->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
		pHBoxLayout_Decibel->addWidget(m_pLabel_DecibelRange);
		pHBoxLayout_Decibel->addWidget(m_pLineEdit_DecibelMin);
		pHBoxLayout_Decibel->addWidget(m_pImageView_Colorbar);
		pHBoxLayout_Decibel->addWidget(m_pLineEdit_DecibelMax);

		pGridLayout_OctVisualization->addItem(pHBoxLayout_Decibel, 1, 0, 1, 3);
	}

	if (!is_streaming)
    {
		QHBoxLayout *pHBoxLayout_Rotation = new QHBoxLayout;
		pHBoxLayout_Rotation->setSpacing(1);

		pHBoxLayout_Rotation->addWidget(m_pLabel_Rotation);
		pHBoxLayout_Rotation->addWidget(m_pScrollBar_Rotation);

        pGridLayout_OctVisualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 0);
        pGridLayout_OctVisualization->addItem(pHBoxLayout_Rotation, 1, 1, 1, 2);
	}

    m_pGroupBox_OctVisualization->setLayout(pGridLayout_OctVisualization);

    // Connect signal and slot
	connect(m_pButtonGroup_ViewMode, SIGNAL(buttonClicked(int)), this, SLOT(changeViewMode(int)));	
	if (is_streaming) connect(m_pLineEdit_DecibelMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustDecibelRange()));
	if (is_streaming) connect(m_pLineEdit_DecibelMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustDecibelRange()));
	if (!is_streaming) connect(m_pScrollBar_Rotation, SIGNAL(valueChanged(int)), this, SLOT(rotateImage(int)));
}


//void ViewOptionTab::setOctDecibelContrastWidgets(bool enabled)
//{
//	m_pLabel_DecibelRange->setEnabled(enabled);
//	m_pLineEdit_DecibelMax->setEnabled(enabled);
//	m_pLineEdit_DecibelMin->setEnabled(enabled);
//	m_pImageView_Colorbar->setEnabled(enabled);
//}

//void ViewOptionTab::setWidgetsEnabled(bool enabled, Configuration* pConfig)
//{
//	// Set visualization widgets
////	if (pConfig)
////	{
////		m_pImageView_RectImage->setEnabled(enabled);
////		m_pImageView_RectImage->setUpdatesEnabled(enabled);
////		m_pImageView_CircImage->setEnabled(enabled);
////		m_pImageView_CircImage->setUpdatesEnabled(enabled);
////		if (enabled)
////		{
////			if (m_pResultTab)
////			{
////				m_pImageView_RectImage->setMagnDefault();
////				m_pImageView_CircImage->setMagnDefault();
////			}
////		}

////		m_pImageView_OctProjection->setEnabled(enabled);
////		m_pImageView_OctProjection->setUpdatesEnabled(enabled);
////		m_pImageView_IntensityMap->setEnabled(enabled);
////		m_pImageView_IntensityMap->setUpdatesEnabled(enabled);
////		m_pImageView_LifetimeMap->setEnabled(enabled);
////		m_pImageView_LifetimeMap->setUpdatesEnabled(enabled);
////	}

//	if (enabled && pConfig)
//	{
////		m_pImageView_RectImage->resetSize(pConfig->octAlines, pConfig->octScans);
////		m_pImageView_CircImage->resetSize(2 * pConfig->octScans, 2 * pConfig->octScans);

////		m_pImageView_OctProjection->resetSize(pConfig->octAlines, pConfig->frames);
////		m_pImageView_IntensityMap->resetSize(pConfig->flimAlines, pConfig->frames);
////		m_pImageView_LifetimeMap->resetSize(pConfig->flimAlines, pConfig->frames);
////	}

////	// Navigation widgets
////	m_pGroupBox_VisualizationWidgets->setEnabled(enabled);
////	m_pToggleButton_MeasureDistance->setEnabled(false);
////	m_pLabel_SelectFrame->setEnabled(enabled);
////	if (pConfig)
////	{
////		QString str; str.sprintf("Current Frame : %3d / %3d", 1, pConfig->frames);
////		m_pLabel_SelectFrame->setText(str);
////	}
////	m_pSlider_SelectFrame->setEnabled(enabled);
////	if (enabled && pConfig)
////	{
////		m_pSlider_SelectFrame->setRange(0, pConfig->frames - 1);
////		m_pSlider_SelectFrame->setValue(0);

////		int id = m_pButtonGroup_ViewMode->checkedId();
////		switch (id)
////		{
////			case CIRCULAR_VIEW:
////				m_pToggleButton_MeasureDistance->setEnabled(true);
////				break;
////			case RECTANGULAR_VIEW:
////				m_pToggleButton_MeasureDistance->setEnabled(false);
////				break;
////		}
////	}

//	// FLIm visualization widgets
//	m_pGroupBox_FlimVisualization->setEnabled(enabled);
//	m_pLabel_EmissionChannel->setEnabled(enabled);
//	m_pComboBox_EmissionChannel->setEnabled(enabled);
	
//	m_pCheckBox_IntensityRatio->setEnabled(enabled);
//	if (m_pCheckBox_IntensityRatio->isChecked() && enabled)
//		m_pComboBox_IntensityRef->setEnabled(true);
//	else
//		m_pComboBox_IntensityRef->setEnabled(false);
//	m_pCheckBox_IntensityWeightedLifetimeMap->setEnabled(enabled);
//	m_pPushButton_PulseReview->setEnabled(enabled);
//	m_pCheckBox_Classification->setEnabled(enabled);

//	m_pLineEdit_IntensityMax->setEnabled(enabled);
//	m_pLineEdit_IntensityMin->setEnabled(enabled);
//	m_pLineEdit_LifetimeMax->setEnabled(enabled);
//	m_pLineEdit_LifetimeMin->setEnabled(enabled);

//	// OCT visualization widgets
//	m_pGroupBox_OctVisualization->setEnabled(enabled);
//	m_pRadioButton_CircularView->setEnabled(enabled);
//	m_pRadioButton_RectangularView->setEnabled(enabled);
	
//	m_pLineEdit_OctGrayMax->setEnabled(enabled);
//	m_pLineEdit_OctGrayMin->setEnabled(enabled);

////	m_pPushButton_LongitudinalView->setEnabled(enabled);

//	QString str; str.sprintf("Rotation %4d / %4d ", 0, pConfig->octAlines - 1);
//	m_pLabel_Rotation->setText(str);
//	m_pLabel_Rotation->setEnabled(enabled);

//	m_pScrollBar_Rotation->setRange(0, pConfig->octAlines - 1);
//	m_pScrollBar_Rotation->setValue(0);
//	m_pScrollBar_Rotation->setEnabled(enabled);

//	// En face widgets
////	m_pLabel_OctProjection->setEnabled(enabled);
////	m_pLabel_IntensityMap->setEnabled(enabled);
////	m_pLabel_LifetimeMap->setEnabled(enabled);
//}


void ViewOptionTab::enableIntensityRatioMode(bool toggled)
{
	// Set widget
	m_pComboBox_IntensityRef->setEnabled(toggled);

	if (toggled)
	{
//		if (m_pResultTab)
//			m_pLabel_IntensityMap->setText(QString::fromLocal8Bit("FLIm Ch%1 / Ch%2 Intensity Ratio Map (еш-z) (AU)").arg(m_pConfig->flimEmissionChannel)
//				.arg(ratio_index[m_pConfig->flimEmissionChannel - 1][m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]]));

		m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1]
			[m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]].min, 'f', 1));
		m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1]
			[m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]].max, 'f', 1));
	}
	else
	{
//		if (m_pResultTab)
//			m_pLabel_IntensityMap->setText(QString::fromLocal8Bit("FLIm Ch%1 Intensity Map (еш-z) (AU)").arg(m_pConfig->flimEmissionChannel));

		m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
		m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
	}

	// Only result tab function
//	visualizeEnFaceMap(true);
//	visualizeImage(m_pSlider_SelectFrame->value());
}

void ViewOptionTab::changeIntensityRatioRef(int index)
{
	m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1] = index;

	// Set widget
//	if (m_pResultTab)
//		m_pLabel_IntensityMap->setText(QString::fromLocal8Bit("FLIm Ch%1 / Ch%2 Intensity Ratio Map (еш-z) (AU)").arg(m_pConfig->flimEmissionChannel)
//			.arg(ratio_index[m_pConfig->flimEmissionChannel - 1][m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]]));

	m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1]
		[m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]].min, 'f', 1));
	m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1]
		[m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]].max, 'f', 1));

	// Only result tab function
//	visualizeEnFaceMap(true);
//	visualizeImage(m_pSlider_SelectFrame->value());
}

void ViewOptionTab::enableIntensityWeightingMode(bool toggled)
{
//	if (!toggled)
//		m_pLabel_LifetimeMap->setText(QString::fromLocal8Bit("FLIm Ch%1 Lifetime Map (еш-z) (nsec)").arg(m_pConfig->flimEmissionChannel));
//	else
//		m_pLabel_LifetimeMap->setText(QString::fromLocal8Bit("FLIm Ch%1 Intensity-Weighted Lifetime Map (еш-z) (nsec)").arg(m_pConfig->flimEmissionChannel));

//	// Only result tab function
//	visualizeEnFaceMap(true);
//	visualizeImage(m_pSlider_SelectFrame->value());
}

void ViewOptionTab::changeEmissionChannel(int ch)
{
    m_pConfig->flimEmissionChannel = ch + 1;

	if (m_pStreamTab)
	{
		m_pLabel_NormIntensity->setText(QString("Ch%1 Intensity (AU) ").arg(ch + 1));
		m_pLabel_Lifetime->setText(QString("Ch%1 Lifetime (nsec) ").arg(ch + 1));

		m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRange[ch].min, 'f', 1));
		m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRange[ch].max, 'f', 1));
	}
	else if (m_pResultTab)
	{
		disconnect(m_pComboBox_IntensityRef, 0, this, 0);

		m_pComboBox_IntensityRef->removeItem(0);
		m_pComboBox_IntensityRef->removeItem(0);
		for (int i = 0; i < 3; i++)
			if (i != ch)
				m_pComboBox_IntensityRef->addItem(QString("Ch %1").arg(i + 1));

		connect(m_pComboBox_IntensityRef, SIGNAL(currentIndexChanged(int)), this, SLOT(changeIntensityRatioRef(int)));

		m_pComboBox_IntensityRef->setCurrentIndex(m_pConfig->flimIntensityRatioRefIdx[ch]);

//		if (!m_pCheckBox_IntensityWeightedLifetimeMap->isChecked())
//			m_pLabel_LifetimeMap->setText(QString::fromLocal8Bit("FLIm Ch%1 Lifetime Map (еш-z) (nsec)").arg(ch + 1));
//		else
//			m_pLabel_LifetimeMap->setText(QString::fromLocal8Bit("FLIm Ch%1 Intensity-Weighted Lifetime Map (еш-z) (nsec)").arg(ch + 1));

		if (!m_pCheckBox_IntensityRatio->isChecked())
		{
//			m_pLabel_IntensityMap->setText(QString::fromLocal8Bit("FLIm Ch%1 Intensity Map (еш-z) (AU)").arg(ch + 1));
			m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRange[ch].min, 'f', 1));
			m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRange[ch].max, 'f', 1));
		}
		else
		{			
//			m_pLabel_IntensityMap->setText(QString::fromLocal8Bit("FLIm Ch%1 / Ch%2 Intensity Ratio Map (еш-z) (AU)").arg(ch + 1)
//				.arg(ratio_index[ch][m_pConfig->flimIntensityRatioRefIdx[ch]]));
			m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRatioRange[ch]
				[m_pConfig->flimIntensityRatioRefIdx[ch]].min, 'f', 1));
			m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRatioRange[ch]
				[m_pConfig->flimIntensityRatioRefIdx[ch]].max, 'f', 1));
		}
	}

	m_pLineEdit_LifetimeMin->setText(QString::number(m_pConfig->flimLifetimeRange[ch].min, 'f', 1));
	m_pLineEdit_LifetimeMax->setText(QString::number(m_pConfig->flimLifetimeRange[ch].max, 'f', 1));
	
	if (m_pStreamTab)
	{
		// Transfer to FLIm calibration dlg
//		QDeviceControlTab* pDeviceControlTab = m_pStreamTab->getDeviceControlTab();
//		FLImProcess* pFLIm = m_pStreamTab->getOperationTab()->getDataAcq()->getFLIm();

//		if (pDeviceControlTab->getFlimCalibDlg())
//			emit pDeviceControlTab->getFlimCalibDlg()->plotRoiPulse(pFLIm, 0);

//		visualizeImage(m_visImage.raw_ptr(), m_visIntensity.raw_ptr(), m_visLifetime.raw_ptr());
	}
	else if (m_pResultTab)
	{
//		visualizeEnFaceMap(true);
//		visualizeImage(m_pSlider_SelectFrame->value());
	}
}

void ViewOptionTab::adjustFlimContrast()
{
	if (m_pStreamTab)
	{
		m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min = m_pLineEdit_IntensityMin->text().toFloat();
		m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max = m_pLineEdit_IntensityMax->text().toFloat();
		m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min = m_pLineEdit_LifetimeMin->text().toFloat();
		m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max = m_pLineEdit_LifetimeMax->text().toFloat();

//		visualizeImage(m_visImage.raw_ptr(), m_visIntensity.raw_ptr(), m_visLifetime.raw_ptr());
	}
	else if (m_pResultTab)
	{
		if (!m_pCheckBox_IntensityRatio->isChecked())
		{
			m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min = m_pLineEdit_IntensityMin->text().toFloat();
			m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max = m_pLineEdit_IntensityMax->text().toFloat();
		}
		else
		{
			m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1]
				[m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]].min = m_pLineEdit_IntensityMin->text().toFloat();
			m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1]
				[m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]].max = m_pLineEdit_IntensityMax->text().toFloat();
		}
		m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min = m_pLineEdit_LifetimeMin->text().toFloat();
		m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max = m_pLineEdit_LifetimeMax->text().toFloat();

//		visualizeEnFaceMap(true);
//		visualizeImage(m_pSlider_SelectFrame->value());
	}
}

void ViewOptionTab::enableClassification(bool toggled)
{
//	if (!toggled)
//		m_pLabel_LifetimeMap->setText(QString::fromLocal8Bit("FLIm Ch%1 Lifetime Map (еш-z) (nsec)").arg(m_pConfig->flimEmissionChannel));
//	else
//		m_pLabel_LifetimeMap->setText(QString::fromLocal8Bit("FLIm-based Classification Map (еш-z)"));

	// Set enable state of associated widgets 	
	if (toggled) m_pCheckBox_IntensityWeightedLifetimeMap->setChecked(false);
	if (toggled) m_pCheckBox_IntensityRatio->setChecked(false);
	//m_pCheckBox_IntensityWeightedLifetimeMap->setEnabled(!toggled);
	m_pLabel_EmissionChannel->setEnabled(!toggled);
	m_pComboBox_EmissionChannel->setEnabled(!toggled);
	m_pCheckBox_IntensityRatio->setEnabled(!toggled);
	//m_pLineEdit_IntensityMax->setEnabled(!toggled);
	//m_pLineEdit_IntensityMin->setEnabled(!toggled);
	m_pLineEdit_LifetimeMax->setEnabled(!toggled);
	m_pLineEdit_LifetimeMin->setEnabled(!toggled);
	if (toggled)
		m_pImageView_LifetimeColorbar->resetColormap(ColorTable::colortable::clf);
	else
		m_pImageView_LifetimeColorbar->resetColormap(ColorTable::colortable(LIFETIME_COLORTABLE));
	
	// Only result tab function
//	visualizeEnFaceMap(true);
//	visualizeImage(m_pSlider_SelectFrame->value());
}


void ViewOptionTab::changeViewMode(int id)
{
//    switch (id)
//    {
//		case CIRCULAR_VIEW:
//			m_pImageView_CircImage->show();
//			m_pImageView_RectImage->hide();
//			break;
//		case RECTANGULAR_VIEW:
//			if (m_pResultTab) m_pToggleButton_MeasureDistance->setChecked(false);
//			m_pImageView_CircImage->hide();
//			m_pImageView_RectImage->show();
//			break;
//    }

//	if (m_pStreamTab)
//		visualizeImage(m_visImage.raw_ptr(), m_visIntensity.raw_ptr(), m_visLifetime.raw_ptr());
//	else if (m_pResultTab)
//	{
//		visualizeImage(m_pSlider_SelectFrame->value());
//		m_pToggleButton_MeasureDistance->setEnabled(id == CIRCULAR_VIEW);
//	}
}

void ViewOptionTab::adjustDecibelRange()
{	
	double min = m_pLineEdit_DecibelMin->text().toDouble();
	double max = m_pLineEdit_DecibelMax->text().toDouble();

	m_pConfig->axsunDbRange.min = min;
	m_pConfig->axsunDbRange.max = max;

//	m_pStreamTab->getDeviceControlTab()->adjustDecibelRange();
}

void ViewOptionTab::adjustOctGrayContrast()
{
	// Only result tab function
	m_pConfig->octGrayRange.min = m_pLineEdit_OctGrayMin->text().toFloat();
	m_pConfig->octGrayRange.max = m_pLineEdit_OctGrayMax->text().toFloat();

//	visualizeEnFaceMap(true);
//	visualizeImage(m_pSlider_SelectFrame->value());
}

void ViewOptionTab::rotateImage(int shift)
{
	QString str; str.sprintf("Rotation %4d / %4d ", shift, m_pScrollBar_Rotation->maximum());
	m_pLabel_Rotation->setText(str);

//	visualizeEnFaceMap(true);
//	visualizeImage(m_pSlider_SelectFrame->value());
}
