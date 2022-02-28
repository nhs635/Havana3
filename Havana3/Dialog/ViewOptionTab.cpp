
#include "ViewOptionTab.h"

#include <Havana3/MainWindow.h>
#include <Havana3/Configuration.h>
#include <Havana3/QStreamTab.h>
#include <Havana3/QResultTab.h>
#include <Havana3/QViewTab.h>

#include <Havana3/Dialog/SettingDlg.h>
#include <Havana3/Dialog/PulseReviewTab.h>
#include <Havana3/Viewer/QScope.h>
#include <Havana3/Viewer/QImageView.h>

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/FLImProcess/FLImProcess.h>
#include <DataAcquisition/DataProcessing.h>
#include <DeviceControl/DeviceControl.h>


ViewOptionTab::ViewOptionTab(QWidget *parent) :
	QDialog(parent), m_pPatientSummaryTab(nullptr), m_pStreamTab(nullptr), m_pResultTab(nullptr), m_pViewTab(nullptr)
{
    // Set configuration objects
	QString parent_name, parent_title = parent->windowTitle();
	if (parent_title.contains("Summary"))
	{
		parent_name = "Summary";
		m_pPatientSummaryTab = dynamic_cast<QPatientSummaryTab*>(parent);
		m_pConfig = m_pPatientSummaryTab->getMainWnd()->m_pConfiguration;
	}
	else if (parent_title.contains("Streaming"))
	{
		parent_name = "Streaming";
		m_pStreamTab = dynamic_cast<QStreamTab*>(parent);
		m_pConfig = m_pStreamTab->getMainWnd()->m_pConfiguration;
		m_pViewTab = m_pStreamTab->getViewTab();
	}
	else if (parent_title.contains("Review"))
	{
		parent_name = "Review";
		m_pResultTab = dynamic_cast<QResultTab*>(parent);
		m_pConfig = m_pResultTab->getMainWnd()->m_pConfiguration;
		m_pConfigTemp = m_pResultTab->getDataProcessing()->getConfigTemp();
		m_pViewTab = m_pResultTab->getViewTab();
	}
			
	// Create layout
	m_pGroupBox_ViewOption = new QGroupBox;
	m_pGroupBox_ViewOption->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	m_pGroupBox_ViewOption->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    m_pVBoxLayout_ViewOption = new QVBoxLayout;
	m_pVBoxLayout_ViewOption->setSpacing(3);
	
	// Create FLIM visualization option tab
	createFlimVisualizationOptionTab();

	// Create OCT visualization option tab
	createOctVisualizationOptionTab();

	// Create Sync visualization option tab
	if (!m_pStreamTab) createSyncVisualizationOptionTab();

	// Set layout
	m_pVBoxLayout_ViewOption->addStretch(1);
	m_pGroupBox_ViewOption->setLayout(m_pVBoxLayout_ViewOption);
}

ViewOptionTab::~ViewOptionTab()
{
}


void ViewOptionTab::createFlimVisualizationOptionTab()
{
	QGroupBox *pGroupBox_FlimVisualization = new QGroupBox;
	pGroupBox_FlimVisualization->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	pGroupBox_FlimVisualization->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    // Create widgets for FLIm emission control
    m_pComboBox_EmissionChannel = new QComboBox(this);
    m_pComboBox_EmissionChannel->addItem("Ch 1");
    m_pComboBox_EmissionChannel->addItem("Ch 2");
    m_pComboBox_EmissionChannel->addItem("Ch 3");
    m_pComboBox_EmissionChannel->setCurrentIndex(m_pConfig->flimEmissionChannel - 1);
    m_pComboBox_EmissionChannel->setFixedWidth(60);

    m_pLabel_EmissionChannel = new QLabel("Emission Channel  ", this);
    m_pLabel_EmissionChannel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    m_pLabel_EmissionChannel->setBuddy(m_pComboBox_EmissionChannel);
		
	if (!m_pStreamTab)
	{
		// Create widgets for FLIm intensity ratio image
		m_pLabel_VisualizationMode = new QLabel("Visualization Mode   ", this);
		m_pLabel_VisualizationMode->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

		m_pRadioButton_Lifetime = new QRadioButton(this);
		m_pRadioButton_Lifetime->setText("Lifetime");
		m_pRadioButton_Lifetime->setChecked(m_pConfig->flimVisualizationMode == VisualizationMode::_LIFETIME_);
		m_pRadioButton_IntensityRatio = new QRadioButton(this);
		m_pRadioButton_IntensityRatio->setText(QString("Intensity Ratio (%1/%2)").arg(m_pConfig->flimEmissionChannel).arg((m_pConfig->flimEmissionChannel == 1) ? 3 : m_pConfig->flimEmissionChannel - 1));
		m_pRadioButton_IntensityRatio->setChecked(m_pConfig->flimVisualizationMode == VisualizationMode::_INTENSITY_RATIO_);
		m_pButtonGroup_Visualization = new QButtonGroup(this);
		m_pButtonGroup_Visualization->addButton(m_pRadioButton_Lifetime, 0);
		m_pButtonGroup_Visualization->addButton(m_pRadioButton_IntensityRatio, 1);
		
		// Create widgets for FLIm based classification 
		m_pCheckBox_Classification = new QCheckBox(this);
		m_pCheckBox_Classification->setText(" Plaque Component Characterization");
		m_pCheckBox_Classification->setDisabled(true); /* currently not supported */
	}
		
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
	if (!m_pStreamTab)
	{
		m_pLineEdit_IntensityRatioMax = new QLineEdit(this);
		m_pLineEdit_IntensityRatioMax->setFixedWidth(35);
		m_pLineEdit_IntensityRatioMax->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
		m_pLineEdit_IntensityRatioMax->setAlignment(Qt::AlignCenter);
		m_pLineEdit_IntensityRatioMin = new QLineEdit(this);
		m_pLineEdit_IntensityRatioMin->setFixedWidth(35);
		m_pLineEdit_IntensityRatioMin->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
		m_pLineEdit_IntensityRatioMin->setAlignment(Qt::AlignCenter);
		m_pLineEdit_IntensityRatioMin->setDisabled(true);
	}

	// Create color bar for FLIM visualization
	uint8_t color[256];
	for (int i = 0; i < 256; i++)
		color[i] = i;

	m_pImageView_IntensityColorbar = new QImageView(ColorTable::colortable(INTENSITY_COLORTABLE), 256, 1, false, this);
    m_pImageView_IntensityColorbar->setFixedSize(190, 20);
	m_pImageView_IntensityColorbar->drawImage(color);
	m_pImageView_LifetimeColorbar = new QImageView(ColorTable::colortable(LIFETIME_COLORTABLE), 256, 1, false, this);
    m_pImageView_LifetimeColorbar->setFixedSize(190, 20);
	m_pImageView_LifetimeColorbar->drawImage(color);
	if (!m_pStreamTab)
	{
		m_pImageView_IntensityRatioColorbar = new QImageView(ColorTable::colortable(INTENSITY_RATIO_COLORTABLE), 256, 1, false, this);
		m_pImageView_IntensityRatioColorbar->setFixedSize(190, 20);
		m_pImageView_IntensityRatioColorbar->drawImage(color);
	}
	m_pLabel_NormIntensity = new QLabel(QString("Ch%1 Intensity (AU) ").arg(m_pConfig->flimEmissionChannel), this);
    m_pLabel_NormIntensity->setFixedWidth(140);
	m_pLabel_NormIntensity->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	m_pLabel_Lifetime = new QLabel(QString("Ch%1 Lifetime (nsec) ").arg(m_pConfig->flimEmissionChannel), this);
    m_pLabel_Lifetime->setFixedWidth(140);
	m_pLabel_Lifetime->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	if (!m_pStreamTab)
	{
		m_pLabel_IntensityRatio = new QLabel(QString("Ch%1/%2 IntRatio (AU) ").arg(m_pConfig->flimEmissionChannel).arg((m_pConfig->flimEmissionChannel == 1) ? 3 : m_pConfig->flimEmissionChannel - 1), this);
		m_pLabel_IntensityRatio->setFixedWidth(140);
		m_pLabel_IntensityRatio->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	}
		
    // Set layout
    QHBoxLayout *pHBoxLayout_FlimVisualization1 = new QHBoxLayout;
	pHBoxLayout_FlimVisualization1->setSpacing(3);
    pHBoxLayout_FlimVisualization1->addWidget(m_pLabel_EmissionChannel);
	pHBoxLayout_FlimVisualization1->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
    pHBoxLayout_FlimVisualization1->addWidget(m_pComboBox_EmissionChannel);

	QHBoxLayout *pHBoxLayout_IntensityColorbar = new QHBoxLayout;
	QHBoxLayout *pHBoxLayout_LifetimeColorbar = new QHBoxLayout;
	QHBoxLayout *pHBoxLayout_IntensityRatioColorbar = new QHBoxLayout;

	pHBoxLayout_IntensityColorbar->setSpacing(1);
	pHBoxLayout_IntensityColorbar->addWidget(m_pLabel_NormIntensity);
	pHBoxLayout_IntensityColorbar->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout_IntensityColorbar->addWidget(m_pLineEdit_IntensityMin);
	pHBoxLayout_IntensityColorbar->addWidget(m_pImageView_IntensityColorbar);
	pHBoxLayout_IntensityColorbar->addWidget(m_pLineEdit_IntensityMax);

	pHBoxLayout_LifetimeColorbar->setSpacing(1);
	pHBoxLayout_LifetimeColorbar->addWidget(m_pLabel_Lifetime);
	pHBoxLayout_LifetimeColorbar->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout_LifetimeColorbar->addWidget(m_pLineEdit_LifetimeMin);
	pHBoxLayout_LifetimeColorbar->addWidget(m_pImageView_LifetimeColorbar);
	pHBoxLayout_LifetimeColorbar->addWidget(m_pLineEdit_LifetimeMax);

	if (!m_pStreamTab)
	{
		pHBoxLayout_IntensityRatioColorbar->setSpacing(1);
		pHBoxLayout_IntensityRatioColorbar->addWidget(m_pLabel_IntensityRatio);
		pHBoxLayout_IntensityRatioColorbar->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
		pHBoxLayout_IntensityRatioColorbar->addWidget(m_pLineEdit_IntensityRatioMin);
		pHBoxLayout_IntensityRatioColorbar->addWidget(m_pImageView_IntensityRatioColorbar);
		pHBoxLayout_IntensityRatioColorbar->addWidget(m_pLineEdit_IntensityRatioMax);
	}
  
	QVBoxLayout *pVBoxLayout_FlimVisualization = new QVBoxLayout;
	pVBoxLayout_FlimVisualization->setSpacing(5);

	pVBoxLayout_FlimVisualization->addItem(pHBoxLayout_FlimVisualization1);
	if (!m_pStreamTab)
	{
		QHBoxLayout *pHBoxLayout_FlimVisualization2 = new QHBoxLayout;
		pHBoxLayout_FlimVisualization2->setSpacing(3);

		pHBoxLayout_FlimVisualization2->addWidget(m_pLabel_VisualizationMode);
		pHBoxLayout_FlimVisualization2->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
		pHBoxLayout_FlimVisualization2->addWidget(m_pRadioButton_Lifetime);
		pHBoxLayout_FlimVisualization2->addWidget(m_pRadioButton_IntensityRatio);

		pVBoxLayout_FlimVisualization->addItem(pHBoxLayout_FlimVisualization2);
		pVBoxLayout_FlimVisualization->addWidget(m_pCheckBox_Classification);
	}
	pVBoxLayout_FlimVisualization->addItem(pHBoxLayout_IntensityColorbar);
	pVBoxLayout_FlimVisualization->addItem(pHBoxLayout_LifetimeColorbar);
	if (!m_pStreamTab) pVBoxLayout_FlimVisualization->addItem(pHBoxLayout_IntensityRatioColorbar);

	pGroupBox_FlimVisualization->setLayout(pVBoxLayout_FlimVisualization);

	m_pVBoxLayout_ViewOption->addWidget(pGroupBox_FlimVisualization);

    // Connect signal and slot
    connect(m_pComboBox_EmissionChannel, SIGNAL(currentIndexChanged(int)), this, SLOT(changeEmissionChannel(int)));
	if (!m_pStreamTab)
	{
		connect(m_pButtonGroup_Visualization, SIGNAL(buttonClicked(int)), this, SLOT(setVisualizationMode(int)));
		connect(m_pCheckBox_Classification, SIGNAL(toggled(bool)), this, SLOT(enableClassification(bool)));
	}
	connect(m_pLineEdit_IntensityMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
	connect(m_pLineEdit_IntensityMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
	connect(m_pLineEdit_LifetimeMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
	connect(m_pLineEdit_LifetimeMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
	if (!m_pStreamTab)
	{
		connect(m_pLineEdit_IntensityRatioMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
		connect(m_pLineEdit_IntensityRatioMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
		
		// Initialization
		setVisualizationMode(m_pConfig->flimVisualizationMode);
	}
}

void ViewOptionTab::createOctVisualizationOptionTab()
{		
	QGroupBox *pGroupBox_OctVisualization = new QGroupBox;
	pGroupBox_OctVisualization->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	pGroupBox_OctVisualization->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	// Create widgets for OCT cross section rotation
	if (!m_pStreamTab)
	{
		m_pScrollBar_Rotation = new QScrollBar(this);
		m_pScrollBar_Rotation->setOrientation(Qt::Horizontal);
		m_pScrollBar_Rotation->setRange(0, m_pConfig->octAlines - 1);
		m_pScrollBar_Rotation->setValue(m_pConfigTemp->rotatedAlines);
		m_pScrollBar_Rotation->setSingleStep(1);
		m_pScrollBar_Rotation->setPageStep(m_pScrollBar_Rotation->maximum() / 10);
		m_pScrollBar_Rotation->setFocusPolicy(Qt::StrongFocus);
		m_pScrollBar_Rotation->setFixedSize(200, 18);

		QString str; str.sprintf("Rotation %4d / %4d ", m_pConfigTemp->rotatedAlines, m_pConfig->octAlines - 1);
		m_pLabel_Rotation = new QLabel(str, this);
		m_pLabel_Rotation->setBuddy(m_pScrollBar_Rotation);
	}

	// Create widgets for OCT vertical mirroring
	if (m_pStreamTab)
	{
		m_pCheckBox_VerticalMirroring = new QCheckBox(this);
		m_pCheckBox_VerticalMirroring->setText(" OCT Image Veritcal Mirroring");
		m_pCheckBox_VerticalMirroring->setChecked(m_pConfig->verticalMirroring);
	}

	// Create line edit widgets for OCT contrast adjustment
#ifndef NEXT_GEN_SYSTEM
	if (m_pStreamTab)
	{
#endif
		m_pLineEdit_DecibelMax = new QLineEdit(this);
        m_pLineEdit_DecibelMax->setFixedWidth(35);
		m_pLineEdit_DecibelMax->setText(QString::number(m_pConfig->axsunDbRange.max));
		m_pLineEdit_DecibelMax->setAlignment(Qt::AlignCenter);
		m_pLineEdit_DecibelMin = new QLineEdit(this);
        m_pLineEdit_DecibelMin->setFixedWidth(35);
		m_pLineEdit_DecibelMin->setText(QString::number(m_pConfig->axsunDbRange.min));
		m_pLineEdit_DecibelMin->setAlignment(Qt::AlignCenter);
#ifndef NEXT_GEN_SYSTEM
	}
	else
	{
		m_pLineEdit_OctGrayMax = new QLineEdit(this);
		m_pLineEdit_OctGrayMax->setFixedWidth(35);
		m_pLineEdit_OctGrayMax->setText(QString::number(m_pConfigTemp->octGrayRange.max));
		m_pLineEdit_OctGrayMax->setAlignment(Qt::AlignCenter);
		m_pLineEdit_OctGrayMin = new QLineEdit(this);
		m_pLineEdit_OctGrayMin->setFixedWidth(35);
		m_pLineEdit_OctGrayMin->setText(QString::number(m_pConfigTemp->octGrayRange.min));
		m_pLineEdit_OctGrayMin->setAlignment(Qt::AlignCenter);
	}
#endif

	// Create color bar for OCT visualization
	uint8_t color[256];
	for (int i = 0; i < 256; i++)
		color[i] = (uint8_t)i;

	m_pImageView_OctColorbar = new QImageView(ColorTable::colortable(OCT_COLORTABLE), 256, 1, false, this);
	m_pImageView_OctColorbar->setFixedSize(190, 20);
	m_pImageView_OctColorbar->drawImage(color);

#ifndef NEXT_GEN_SYSTEM
	if (m_pStreamTab)
#endif
		m_pLabel_OctContrast = new QLabel("OCT Contrast (dB) ", this);
#ifndef NEXT_GEN_SYSTEM
	else
		m_pLabel_OctContrast = new QLabel("OCT Gray Range (AU) ", this);
#endif
	m_pLabel_OctContrast->setFixedWidth(140);
	m_pLabel_OctContrast->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	m_pLabel_OctContrast->setBuddy(m_pImageView_OctColorbar);
	

    // Set layout	
	QHBoxLayout *pHBoxLayout_OctContrast = new QHBoxLayout;
	pHBoxLayout_OctContrast->setSpacing(1);
	pHBoxLayout_OctContrast->addWidget(m_pLabel_OctContrast);
	pHBoxLayout_OctContrast->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
#ifndef NEXT_GEN_SYSTEM
	pHBoxLayout_OctContrast->addWidget(m_pStreamTab ? m_pLineEdit_DecibelMin : m_pLineEdit_OctGrayMin);
	pHBoxLayout_OctContrast->addWidget(m_pImageView_OctColorbar);
	pHBoxLayout_OctContrast->addWidget(m_pStreamTab ? m_pLineEdit_DecibelMax : m_pLineEdit_OctGrayMax);
#else
	pHBoxLayout_OctContrast->addWidget(m_pLineEdit_DecibelMin);
	pHBoxLayout_OctContrast->addWidget(m_pImageView_OctColorbar);
	pHBoxLayout_OctContrast->addWidget(m_pLineEdit_DecibelMax);
#endif


	QVBoxLayout *pVBoxLayout_OctVisualization = new QVBoxLayout;
	pVBoxLayout_OctVisualization->setSpacing(5);

	if (m_pStreamTab) pVBoxLayout_OctVisualization->addWidget(m_pCheckBox_VerticalMirroring);
	if (!m_pStreamTab)
	{
		QHBoxLayout *pHBoxLayout_Rotation = new QHBoxLayout;
		pHBoxLayout_Rotation->setSpacing(3);
		if (!m_pStreamTab)
		{
			pHBoxLayout_Rotation->addWidget(m_pLabel_Rotation);
			pHBoxLayout_Rotation->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
			pHBoxLayout_Rotation->addWidget(m_pScrollBar_Rotation);
		}
		pVBoxLayout_OctVisualization->addItem(pHBoxLayout_Rotation);
	}
	pVBoxLayout_OctVisualization->addItem(pHBoxLayout_OctContrast);

	pGroupBox_OctVisualization->setLayout(pVBoxLayout_OctVisualization);

	m_pVBoxLayout_ViewOption->addWidget(pGroupBox_OctVisualization);
	
    // Connect signal and slot
	if (!m_pStreamTab) connect(m_pScrollBar_Rotation, SIGNAL(valueChanged(int)), this, SLOT(rotateImage(int)));
	if (m_pStreamTab) connect(m_pCheckBox_VerticalMirroring, SIGNAL(toggled(bool)), this, SLOT(verticalMirriong(bool)));
#ifndef NEXT_GEN_SYSTEM
	if (m_pStreamTab) connect(m_pLineEdit_DecibelMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustDecibelRange()));
	if (m_pStreamTab) connect(m_pLineEdit_DecibelMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustDecibelRange()));
	if (!m_pStreamTab) connect(m_pLineEdit_OctGrayMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustOctGrayContrast()));
	if (!m_pStreamTab) connect(m_pLineEdit_OctGrayMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustOctGrayContrast()));
#else
	connect(m_pLineEdit_DecibelMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustDecibelRange()));
	connect(m_pLineEdit_DecibelMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustDecibelRange()));
#endif
}

void ViewOptionTab::createSyncVisualizationOptionTab()
{
	QGroupBox *pGroupBox_SyncVisualization = new QGroupBox;
	pGroupBox_SyncVisualization->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	pGroupBox_SyncVisualization->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	// Create widgets for OCT-FLIm synchronization
	if (!m_pStreamTab)
	{
		m_pScrollBar_IntraFrameSync = new QScrollBar(this);
		m_pScrollBar_IntraFrameSync->setOrientation(Qt::Horizontal);
		m_pScrollBar_IntraFrameSync->setRange(0, m_pConfig->octAlines - 1);
		m_pScrollBar_IntraFrameSync->setValue(m_pConfigTemp->intraFrameSync);
		m_pScrollBar_IntraFrameSync->setSingleStep(1);
		m_pScrollBar_IntraFrameSync->setPageStep(m_pScrollBar_Rotation->maximum() / 10);
		m_pScrollBar_IntraFrameSync->setFocusPolicy(Qt::StrongFocus);
		m_pScrollBar_IntraFrameSync->setFixedSize(200, 18);

		QString str; str.sprintf("Intra Sync %4d / %4d ", m_pConfigTemp->intraFrameSync, m_pConfig->octAlines - 1);
		m_pLabel_IntraFrameSync = new QLabel(str, this);
		m_pLabel_IntraFrameSync->setBuddy(m_pScrollBar_Rotation);

		m_pScrollBar_InterFrameSync = new QScrollBar(this);
		m_pScrollBar_InterFrameSync->setOrientation(Qt::Horizontal);
		m_pScrollBar_InterFrameSync->setRange(-m_pConfigTemp->frames + 1, m_pConfigTemp->frames - 1);
		m_pScrollBar_InterFrameSync->setValue(m_pConfigTemp->interFrameSync);
		m_pScrollBar_InterFrameSync->setSingleStep(1);
		m_pScrollBar_InterFrameSync->setPageStep(m_pScrollBar_Rotation->maximum() / 10);
		m_pScrollBar_InterFrameSync->setFocusPolicy(Qt::StrongFocus);
		m_pScrollBar_InterFrameSync->setFixedSize(200, 18);

		str.sprintf("Inter Sync %4d /|%3d| ", m_pConfigTemp->interFrameSync, m_pConfigTemp->frames - 1);
		m_pLabel_InterFrameSync = new QLabel(str, this);
		m_pLabel_InterFrameSync->setBuddy(m_pScrollBar_Rotation);
	}


	// Set layout	
	if (!m_pStreamTab)
	{
		QGridLayout *pGridLayout_FrameSync = new QGridLayout;
		pGridLayout_FrameSync->setSpacing(3);

		pGridLayout_FrameSync->addWidget(m_pLabel_IntraFrameSync, 0, 0);
		pGridLayout_FrameSync->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 1);
		pGridLayout_FrameSync->addWidget(m_pScrollBar_IntraFrameSync, 0, 2);
		pGridLayout_FrameSync->addWidget(m_pLabel_InterFrameSync, 1, 0);
		pGridLayout_FrameSync->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 1);
		pGridLayout_FrameSync->addWidget(m_pScrollBar_InterFrameSync, 1, 2);
		
		pGroupBox_SyncVisualization->setLayout(pGridLayout_FrameSync);
		m_pVBoxLayout_ViewOption->addWidget(pGroupBox_SyncVisualization);

		// Connect signal and slot
		connect(m_pScrollBar_IntraFrameSync, SIGNAL(valueChanged(int)), this, SLOT(setIntraFrameSync(int)));
		connect(m_pScrollBar_InterFrameSync, SIGNAL(valueChanged(int)), this, SLOT(setInterFrameSync(int)));
	}
}


void ViewOptionTab::changeEmissionChannel(int ch)
{
    m_pConfig->flimEmissionChannel = ch + 1;
	
	m_pLabel_NormIntensity->setText(QString("Ch%1 Intensity (AU) ").arg(ch + 1));
	m_pLabel_Lifetime->setText(QString("Ch%1 Lifetime (nsec) ").arg(ch + 1));
	m_pLabel_IntensityRatio->setText(QString("Ch%1/%2 IntRatio (AU) ").arg(m_pConfig->flimEmissionChannel).arg((m_pConfig->flimEmissionChannel == 1) ? 3 : m_pConfig->flimEmissionChannel - 1));

	m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRange[ch].min, 'f', 1));
	m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRange[ch].max, 'f', 1));
	m_pLineEdit_LifetimeMin->setText(QString::number(m_pConfig->flimLifetimeRange[ch].min, 'f', 1));
	m_pLineEdit_LifetimeMax->setText(QString::number(m_pConfig->flimLifetimeRange[ch].max, 'f', 1));
	m_pLineEdit_IntensityRatioMin->setText(QString::number(m_pConfig->flimIntensityRatioRange[ch].min, 'f', 1));
	m_pLineEdit_IntensityRatioMax->setText(QString::number(m_pConfig->flimIntensityRatioRange[ch].max, 'f', 1));

	if (m_pStreamTab)
	{
	}
	else if (m_pResultTab)
	{
		if (m_pViewTab)
		{
			int mode = (int)m_pRadioButton_IntensityRatio->isChecked();
			m_pViewTab->setViewMode(ch + 3 * mode);
			m_pViewTab->invalidate();
		}

		{
			//disconnect(m_pComboBox_IntensityRef, 0, this, 0);

			//m_pComboBox_IntensityRef->removeItem(0);
			//m_pComboBox_IntensityRef->removeItem(0);
			//for (int i = 0; i < 3; i++)
			//	if (i != ch)
			//		m_pComboBox_IntensityRef->addItem(QString("Ch %1").arg(i + 1));

			//connect(m_pComboBox_IntensityRef, SIGNAL(currentIndexChanged(int)), this, SLOT(changeIntensityRatioRef(int)));

			//m_pComboBox_IntensityRef->setCurrentIndex(m_pConfig->flimIntensityRatioRefIdx[ch]);
		}
	}

	m_pConfig->writeToLog(QString("Emission channel changed: ch %1").arg(ch + 1));

////		if (!m_pCheckBox_IntensityWeightedLifetimeMap->isChecked())
////			m_pLabel_LifetimeMap->setText(QString::fromLocal8Bit("FLIm Ch%1 Lifetime Map (θ-z) (nsec)").arg(ch + 1));
////		else
////			m_pLabel_LifetimeMap->setText(QString::fromLocal8Bit("FLIm Ch%1 Intensity-Weighted Lifetime Map (θ-z) (nsec)").arg(ch + 1));
//
//		if (!m_pCheckBox_IntensityRatio->isChecked())
//		{
////			m_pLabel_IntensityMap->setText(QString::fromLocal8Bit("FLIm Ch%1 Intensity Map (θ-z) (AU)").arg(ch + 1));
//			m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRange[ch].min, 'f', 1));
//			m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRange[ch].max, 'f', 1));
//		}
//		else
//		{			
////			m_pLabel_IntensityMap->setText(QString::fromLocal8Bit("FLIm Ch%1 / Ch%2 Intensity Ratio Map (θ-z) (AU)").arg(ch + 1)
////				.arg(ratio_index[ch][m_pConfig->flimIntensityRatioRefIdx[ch]]));
//			m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRatioRange[ch]
//				[m_pConfig->flimIntensityRatioRefIdx[ch]].min, 'f', 1));
//			m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRatioRange[ch]
//				[m_pConfig->flimIntensityRatioRefIdx[ch]].max, 'f', 1));
//		}
		
//	if (m_pStreamTab)
//	{
//		// Transfer to FLIm calibration dlg
////		QDeviceControlTab* pDeviceControlTab = m_pStreamTab->getDeviceControlTab();
////		FLImProcess* pFLIm = m_pStreamTab->getOperationTab()->getDataAcq()->getFLIm();
//
////		if (pDeviceControlTab->getFlimCalibDlg())
////			emit pDeviceControlTab->getFlimCalibDlg()->plotRoiPulse(pFLIm, 0);
//
////		visualizeImage(m_visImage.raw_ptr(), m_visIntensity.raw_ptr(), m_visLifetime.raw_ptr());
//	}
//	else if (m_pResultTab)
//	{
////		visualizeEnFaceMap(true);
////		visualizeImage(m_pSlider_SelectFrame->value());
//	}
}


void ViewOptionTab::setVisualizationMode(int mode)
{
	if (mode == VisualizationMode::_LIFETIME_)
	{
		m_pLabel_Lifetime->setVisible(true);
		m_pLineEdit_LifetimeMin->setVisible(true);
		m_pImageView_LifetimeColorbar->setVisible(true);
		m_pLineEdit_LifetimeMax->setVisible(true);

		m_pLabel_IntensityRatio->setVisible(false);
		m_pLineEdit_IntensityRatioMin->setVisible(false);
		m_pImageView_IntensityRatioColorbar->setVisible(false);
		m_pLineEdit_IntensityRatioMax->setVisible(false);
	}
	else if (mode == VisualizationMode::_INTENSITY_RATIO_)
	{
		m_pLabel_Lifetime->setVisible(false);
		m_pLineEdit_LifetimeMin->setVisible(false);
		m_pImageView_LifetimeColorbar->setVisible(false);
		m_pLineEdit_LifetimeMax->setVisible(false);

		m_pLabel_IntensityRatio->setVisible(true);
		m_pLineEdit_IntensityRatioMin->setVisible(true);
		m_pImageView_IntensityRatioColorbar->setVisible(true);
		m_pLineEdit_IntensityRatioMax->setVisible(true);
	}

	changeEmissionChannel(m_pConfig->flimEmissionChannel - 1);
	adjustFlimContrast();

	//// Set widget
	//m_pComboBox_IntensityRef->setEnabled(toggled);

	//if (toggled)
	//{
	//	//		if (m_pResultTab)
	//	//			m_pLabel_IntensityMap->setText(QString::fromLocal8Bit("FLIm Ch%1 / Ch%2 Intensity Ratio Map (θ-z) (AU)").arg(m_pConfig->flimEmissionChannel)
	//	//				.arg(ratio_index[m_pConfig->flimEmissionChannel - 1][m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]]));

	//	m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1]
	//		[m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]].min, 'f', 1));
	//	m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1]
	//		[m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]].max, 'f', 1));
	//}
	//else
	//{
	//	//		if (m_pResultTab)
	//	//			m_pLabel_IntensityMap->setText(QString::fromLocal8Bit("FLIm Ch%1 Intensity Map (θ-z) (AU)").arg(m_pConfig->flimEmissionChannel));

	//	m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
	//	m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
	//}

	//// Only result tab function
	////	visualizeEnFaceMap(true);
	////	visualizeImage(m_pSlider_SelectFrame->value());
}

//void ViewOptionTab::changeIntensityRatioRef(int index)
//{
	//m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1] = index;

	//// Set widget
	////	if (m_pResultTab)
	////		m_pLabel_IntensityMap->setText(QString::fromLocal8Bit("FLIm Ch%1 / Ch%2 Intensity Ratio Map (θ-z) (AU)").arg(m_pConfig->flimEmissionChannel)
	////			.arg(ratio_index[m_pConfig->flimEmissionChannel - 1][m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]]));

	//m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1]
	//	[m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]].min, 'f', 1));
	//m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1]
	//	[m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]].max, 'f', 1));

	//// Only result tab function
	////	visualizeEnFaceMap(true);
	////	visualizeImage(m_pSlider_SelectFrame->value());

	//(void)index;
//}

void ViewOptionTab::enableClassification(bool toggled)
{
	////	if (!toggled)
	////		m_pLabel_LifetimeMap->setText(QString::fromLocal8Bit("FLIm Ch%1 Lifetime Map (θ-z) (nsec)").arg(m_pConfig->flimEmissionChannel));
	////	else
	////		m_pLabel_LifetimeMap->setText(QString::fromLocal8Bit("FLIm-based Classification Map (θ-z)"));

	//// Set enable state of associated widgets 	
	//if (toggled) m_pCheckBox_IntensityRatio->setChecked(false);
	////m_pCheckBox_IntensityWeightedLifetimeMap->setEnabled(!toggled);
	//m_pLabel_EmissionChannel->setEnabled(!toggled);
	//m_pComboBox_EmissionChannel->setEnabled(!toggled);
	//m_pCheckBox_IntensityRatio->setEnabled(!toggled);
	////m_pLineEdit_IntensityMax->setEnabled(!toggled);
	////m_pLineEdit_IntensityMin->setEnabled(!toggled);
	//m_pLineEdit_LifetimeMax->setEnabled(!toggled);
	//m_pLineEdit_LifetimeMin->setEnabled(!toggled);
	//if (toggled)
	//	m_pImageView_LifetimeColorbar->resetColormap(ColorTable::colortable::clf);
	//else
	//	m_pImageView_LifetimeColorbar->resetColormap(ColorTable::colortable(LIFETIME_COLORTABLE));

	//// Only result tab function
	////	visualizeEnFaceMap(true);
	////	visualizeImage(m_pSlider_SelectFrame->value());

	(void)toggled;
}

void ViewOptionTab::adjustFlimContrast()
{
	if (m_pStreamTab)
	{
		m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min = m_pLineEdit_IntensityMin->text().toFloat();
		m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max = m_pLineEdit_IntensityMax->text().toFloat();
		m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min = m_pLineEdit_LifetimeMin->text().toFloat();
		m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max = m_pLineEdit_LifetimeMax->text().toFloat();

///		visualizeImage(m_visImage.raw_ptr(), m_visIntensity.raw_ptr(), m_visLifetime.raw_ptr());
	}
	else if (m_pResultTab)
	{
		if (m_pRadioButton_Lifetime->isChecked())
		{
			m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min = m_pLineEdit_IntensityMin->text().toFloat();
			m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max = m_pLineEdit_IntensityMax->text().toFloat();
			m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min = m_pLineEdit_LifetimeMin->text().toFloat();
			m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max = m_pLineEdit_LifetimeMax->text().toFloat();
		}
		else if (m_pRadioButton_IntensityRatio->isChecked())
		{
			if (m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1].min == m_pLineEdit_IntensityRatioMin->text().toFloat())
			{
				m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1].min = -m_pLineEdit_IntensityRatioMax->text().toFloat();
				m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1].max = m_pLineEdit_IntensityRatioMax->text().toFloat();

				m_pLineEdit_IntensityRatioMin->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
			}
			else if (m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1].max == m_pLineEdit_IntensityRatioMax->text().toFloat())
			{
				// 구현 필요가 없음.
			}
		}		

		if (m_pViewTab) m_pViewTab->invalidate();
	}

	m_pConfig->writeToLog(QString("FLIm contrast range set: ch%1 i[%2 %3] l[%4 %5] ir[%6 %7]")
		.arg(m_pConfig->flimEmissionChannel)
		.arg(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min)
		.arg(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max)
		.arg(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min)
		.arg(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max)
		.arg(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1].min)
		.arg(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1].max));
}


void ViewOptionTab::rotateImage(int shift)
{
	// Only result tab function
	m_pConfigTemp->rotatedAlines = shift;

	QString str; str.sprintf("Rotation %4d / %4d ", shift, m_pScrollBar_Rotation->maximum());
	m_pLabel_Rotation->setText(str);

	if (m_pViewTab) m_pViewTab->invalidate();
	if (m_pResultTab->getSettingDlg()->getPulseReviewTab())
		m_pResultTab->getSettingDlg()->getPulseReviewTab()->loadRois();

	m_pConfig->writeToLog(QString("Rotated alines set: %1").arg(shift));
}

void ViewOptionTab::verticalMirriong(bool enabled)
{
	// Only stream tab function
	m_pConfig->verticalMirroring = enabled;

	m_pConfig->writeToLog(QString("Vertical mirroring set: %1").arg(enabled ? "true" : "false"));
}

void ViewOptionTab::adjustDecibelRange()
{	
	// Only stream tab function
	m_pConfig->axsunDbRange.min = m_pLineEdit_DecibelMin->text().toDouble();
	m_pConfig->axsunDbRange.max = m_pLineEdit_DecibelMax->text().toDouble();

#ifndef NEXT_GEN_SYSTEM
	m_pStreamTab->getDeviceControl()->adjustDecibelRange(m_pConfig->axsunDbRange.min, m_pConfig->axsunDbRange.max);
#else
	if (m_pResultTab) 
		if (m_pViewTab) 
			m_pViewTab->invalidate();
#endif

	m_pConfig->writeToLog(QString("OCT decibel range set: [%1 %2]")
		.arg(m_pConfig->axsunDbRange.min, 3, 'f', 2).arg(m_pConfig->axsunDbRange.max, 3, 'f', 2));

#ifdef DEVELOPER_MODE
#ifndef NEXT_GEN_SYSTEM
	//if (m_pStreamTab) m_pStreamTab->getAlineScope()->resetAxis({ 0, (double)m_pConfig->octScans }, { m_pConfig->axsunDbRange.min, m_pConfig->axsunDbRange.max }, 2, 2, 1, 1, 0, 0);
#else
	if (m_pStreamTab) m_pStreamTab->getAlineScope()->resetAxis({ 0, (double)m_pConfig->octScansFFT / 2.0 }, { m_pConfig->axsunDbRange.min, m_pConfig->axsunDbRange.max }, 2, 2, 1, 1, 0, 0);
#endif		
#endif
}

void ViewOptionTab::adjustOctGrayContrast()
{
#ifndef NEXT_GEN_SYSTEM
	// Only result tab function
	m_pConfigTemp->octGrayRange.min = m_pLineEdit_OctGrayMin->text().toFloat();
	m_pConfigTemp->octGrayRange.max = m_pLineEdit_OctGrayMax->text().toFloat();

	if (m_pViewTab) m_pViewTab->invalidate();

	m_pConfig->writeToLog(QString("OCT decibel range set: [%1 %2]")
		.arg(m_pConfigTemp->octGrayRange.min).arg(m_pConfigTemp->octGrayRange.max));
#endif
}

void ViewOptionTab::setIntraFrameSync(int sync)
{
	// Only result tab function
	m_pConfigTemp->intraFrameSync = sync;
	
	QString str; str.sprintf("Intra Sync %4d / %4d ", m_pConfigTemp->intraFrameSync, m_pScrollBar_IntraFrameSync->maximum());
	m_pLabel_IntraFrameSync->setText(str);

	if (m_pViewTab) m_pViewTab->invalidate();

	m_pConfig->writeToLog(QString("Intra frame sync set: %1").arg(sync));
}

void ViewOptionTab::setInterFrameSync(int sync)
{
	// Only result tab function
	m_pConfigTemp->interFrameSync = sync;

	QString str; str.sprintf("Inter Sync %4d /|%3d| ", m_pConfigTemp->interFrameSync, m_pScrollBar_InterFrameSync->maximum());
	m_pLabel_InterFrameSync->setText(str);

	if (m_pViewTab) m_pViewTab->invalidate();

	m_pConfig->writeToLog(QString("Inter frame sync set: %1").arg(sync));
}