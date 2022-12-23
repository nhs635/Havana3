
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
	pGroupBox_FlimVisualization->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

	// Create widgets for visualization mode control
	m_pLabel_VisualizationMode = new QLabel("Visualization Mode    ", this);
	m_pLabel_VisualizationMode->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

	if (!m_pStreamTab)
	{
		m_pRadioButton_FLImParameters = new QRadioButton(this);
		m_pRadioButton_FLImParameters->setText("FLIm Parameters");
		m_pRadioButton_RFPrediction = new QRadioButton(this);
		m_pRadioButton_RFPrediction->setText("RF Prediction");
		if (m_pViewTab->getVisualizationMode() == _FLIM_PARAMETERS_)
			m_pRadioButton_FLImParameters->setChecked(true);
		else if (m_pViewTab->getVisualizationMode() == _RF_PREDICTION_)
			m_pRadioButton_RFPrediction->setChecked(true);

		m_pButtonGroup_VisualizationMode = new QButtonGroup(this);
		m_pButtonGroup_VisualizationMode->addButton(m_pRadioButton_FLImParameters, _FLIM_PARAMETERS_);
		m_pButtonGroup_VisualizationMode->addButton(m_pRadioButton_RFPrediction, _RF_PREDICTION_);
	}

    // Create widgets for FLIm emission control
    m_pComboBox_EmissionChannel = new QComboBox(this);
    m_pComboBox_EmissionChannel->addItem("Ch 1");
    m_pComboBox_EmissionChannel->addItem("Ch 2");
    m_pComboBox_EmissionChannel->addItem("Ch 3");
    m_pComboBox_EmissionChannel->setCurrentIndex(m_pConfig->flimEmissionChannel - 1);
    m_pComboBox_EmissionChannel->setFixedWidth(60);
	m_pComboBox_EmissionChannel->setDisabled(m_pConfig->flimParameterMode == FLImParameters::_NONE_);

    m_pLabel_EmissionChannel = new QLabel("Emission Channel  ", this);
    m_pLabel_EmissionChannel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    m_pLabel_EmissionChannel->setBuddy(m_pComboBox_EmissionChannel);
		
	if (!m_pStreamTab)
	{
		// Create widgets for FLIm intensity ratio image
		m_pLabel_FLImParameters = new QLabel("FLIm Parameters    ", this);
		m_pLabel_FLImParameters->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

		m_pRadioButton_Lifetime = new QRadioButton(this);
		m_pRadioButton_Lifetime->setText("Lifetime");
		m_pRadioButton_Lifetime->setChecked(m_pConfig->flimParameterMode == FLImParameters::_LIFETIME_);
		m_pRadioButton_IntensityProp = new QRadioButton(this);
		m_pRadioButton_IntensityProp->setText("Intensity Proportion");
		m_pRadioButton_IntensityProp->setChecked(m_pConfig->flimParameterMode == FLImParameters::_INTENSITY_PROP_);
		m_pRadioButton_IntensityRatio = new QRadioButton(this);
		m_pRadioButton_IntensityRatio->setText(QString("Intensity Ratio (%1/%2)").arg(m_pConfig->flimEmissionChannel).arg((m_pConfig->flimEmissionChannel == 1) ? 3 : m_pConfig->flimEmissionChannel - 1));
		m_pRadioButton_IntensityRatio->setChecked(m_pConfig->flimParameterMode == FLImParameters::_INTENSITY_RATIO_);
		m_pRadioButton_None = new QRadioButton(this);
		m_pRadioButton_None->setText("None");
		m_pRadioButton_None->setChecked(m_pConfig->flimParameterMode == FLImParameters::_NONE_);

		m_pButtonGroup_FLImParameters = new QButtonGroup(this);
		m_pButtonGroup_FLImParameters->addButton(m_pRadioButton_Lifetime, _LIFETIME_);
		m_pButtonGroup_FLImParameters->addButton(m_pRadioButton_IntensityProp, _INTENSITY_PROP_);
		m_pButtonGroup_FLImParameters->addButton(m_pRadioButton_IntensityRatio, _INTENSITY_RATIO_);
		m_pButtonGroup_FLImParameters->addButton(m_pRadioButton_None, _NONE_);
		
		// Create widgets for RF prediction
		m_pLabel_RFPrediction = new QLabel("RF Prediction   ", this);
		m_pLabel_RFPrediction->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
		m_pLabel_RFPrediction->setDisabled(true);

		m_pRadioButton_PlaqueComposition = new QRadioButton(this);
		m_pRadioButton_PlaqueComposition->setText("Plaque Composition");
		m_pRadioButton_PlaqueComposition->setDisabled(true);
		m_pRadioButton_Inflammation = new QRadioButton(this);
		m_pRadioButton_Inflammation->setText("Inflammation");
		m_pRadioButton_Inflammation->setDisabled(true);

		if (m_pViewTab->getRFPrediction() == _PLAQUE_COMPOSITION_)
			m_pRadioButton_PlaqueComposition->setChecked(true);
		else if (m_pViewTab->getRFPrediction() == _INFLAMMATION_)
			m_pRadioButton_Inflammation->setChecked(true);
		
		m_pButtonGroup_RFPrediction = new QButtonGroup(this);
		m_pButtonGroup_RFPrediction->addButton(m_pRadioButton_PlaqueComposition, _PLAQUE_COMPOSITION_);
		m_pButtonGroup_RFPrediction->addButton(m_pRadioButton_Inflammation, _INFLAMMATION_);
	}
		
	// Create line edit widgets for FLIm contrast adjustment
	m_pLineEdit_IntensityMax = new QLineEdit(this);
    m_pLineEdit_IntensityMax->setFixedWidth(35);
	m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 2));
	m_pLineEdit_IntensityMax->setAlignment(Qt::AlignCenter);
	m_pLineEdit_IntensityMin = new QLineEdit(this);
    m_pLineEdit_IntensityMin->setFixedWidth(35);
	m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 2));
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
		m_pLineEdit_IntensityPropMax = new QLineEdit(this);
		m_pLineEdit_IntensityPropMax->setFixedWidth(35);
		m_pLineEdit_IntensityPropMax->setText(QString::number(m_pConfig->flimIntensityPropRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
		m_pLineEdit_IntensityPropMax->setAlignment(Qt::AlignCenter);
		m_pLineEdit_IntensityPropMin = new QLineEdit(this);
		m_pLineEdit_IntensityPropMin->setFixedWidth(35);
		m_pLineEdit_IntensityPropMin->setText(QString::number(m_pConfig->flimIntensityPropRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
		m_pLineEdit_IntensityPropMin->setAlignment(Qt::AlignCenter);

		m_pLineEdit_IntensityRatioMax = new QLineEdit(this);
		m_pLineEdit_IntensityRatioMax->setFixedWidth(35);
		m_pLineEdit_IntensityRatioMax->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
		m_pLineEdit_IntensityRatioMax->setAlignment(Qt::AlignCenter);
		m_pLineEdit_IntensityRatioMin = new QLineEdit(this);
		m_pLineEdit_IntensityRatioMin->setFixedWidth(35);
		m_pLineEdit_IntensityRatioMin->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
		m_pLineEdit_IntensityRatioMin->setAlignment(Qt::AlignCenter);
		m_pLineEdit_IntensityRatioMin->setDisabled(true);

		//m_pLineEdit_InflammationMax = new QLineEdit(this);
		//m_pLineEdit_InflammationMax->setFixedWidth(35);
		//m_pLineEdit_InflammationMax->setText(QString::number(m_pConfig->rfInflammationRange.max, 'f', 1));
		//m_pLineEdit_InflammationMax->setAlignment(Qt::AlignCenter);
		//m_pLineEdit_InflammationMin = new QLineEdit(this);
		//m_pLineEdit_InflammationMin->setFixedWidth(35);
		//m_pLineEdit_InflammationMin->setText(QString::number(m_pConfig->rfInflammationRange.min, 'f', 1));
		//m_pLineEdit_InflammationMin->setAlignment(Qt::AlignCenter);
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
		m_pImageView_IntensityPropColorbar = new QImageView(ColorTable::colortable(INTENSITY_PROP_COLORTABLE), 256, 1, false, this);
		m_pImageView_IntensityPropColorbar->setFixedSize(190, 20);
		m_pImageView_IntensityPropColorbar->drawImage(color);
		m_pImageView_IntensityRatioColorbar = new QImageView(ColorTable::colortable(INTENSITY_RATIO_COLORTABLE), 256, 1, false, this);
		m_pImageView_IntensityRatioColorbar->setFixedSize(190, 20);
		m_pImageView_IntensityRatioColorbar->drawImage(color);
		m_pImageView_PlaqueCompositionColorbar = new QImageView(ColorTable::colortable(COMPOSITION_COLORTABLE), 256, 1, false, this);
		m_pImageView_PlaqueCompositionColorbar->setFixedSize(255, 24);
		m_pImageView_PlaqueCompositionColorbar->drawImage(color);
		m_pImageView_InflammationColorbar = new QImageView(ColorTable::colortable(INFLAMMATION_COLORTABLE), 256, 1, false, this);
		m_pImageView_InflammationColorbar->setFixedSize(255, 24);
		m_pImageView_InflammationColorbar->drawImage(color);
	}
	m_pLabel_NormIntensity = new QLabel(QString("Ch%1 Intensity (AU) ").arg(m_pConfig->flimEmissionChannel), this);
    m_pLabel_NormIntensity->setFixedWidth(140);
	m_pLabel_NormIntensity->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	m_pLabel_Lifetime = new QLabel(QString("Ch%1 Lifetime (nsec) ").arg(m_pConfig->flimEmissionChannel), this);
    m_pLabel_Lifetime->setFixedWidth(140);
	m_pLabel_Lifetime->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	if (!m_pStreamTab)
	{
		m_pLabel_IntensityProp = new QLabel(QString("Ch%1 IntProp (AU) ").arg(m_pConfig->flimEmissionChannel), this);
		m_pLabel_IntensityProp->setFixedWidth(140);
		m_pLabel_IntensityProp->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

		m_pLabel_IntensityRatio = new QLabel(QString("Ch%1/%2 IntRatio (AU) ").arg(m_pConfig->flimEmissionChannel).arg((m_pConfig->flimEmissionChannel == 1) ? 3 : m_pConfig->flimEmissionChannel - 1), this);
		m_pLabel_IntensityRatio->setFixedWidth(140);
		m_pLabel_IntensityRatio->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
		
		m_pLabel_PlaqueComposition = new QLabel("Plaque Composition", this);
		m_pLabel_PlaqueComposition->setFixedWidth(140);
		m_pLabel_PlaqueComposition->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

		m_pLabel_Inflammation = new QLabel("Inflammation", this);
		m_pLabel_Inflammation->setFixedWidth(140);
		m_pLabel_Inflammation->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	}
		
    // Set layout
	QHBoxLayout *pHBoxLayout_FlimVisualization0 = new QHBoxLayout;
	if (!m_pStreamTab)
	{
		pHBoxLayout_FlimVisualization0->setSpacing(3);
		pHBoxLayout_FlimVisualization0->addWidget(m_pLabel_VisualizationMode);
		pHBoxLayout_FlimVisualization0->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
		pHBoxLayout_FlimVisualization0->addWidget(m_pRadioButton_FLImParameters);
		pHBoxLayout_FlimVisualization0->addWidget(m_pRadioButton_RFPrediction);
	}

    QHBoxLayout *pHBoxLayout_FlimVisualization1 = new QHBoxLayout;
	pHBoxLayout_FlimVisualization1->setSpacing(3);
    pHBoxLayout_FlimVisualization1->addWidget(m_pLabel_EmissionChannel);
	pHBoxLayout_FlimVisualization1->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
    pHBoxLayout_FlimVisualization1->addWidget(m_pComboBox_EmissionChannel);

	QHBoxLayout *pHBoxLayout_IntensityColorbar = new QHBoxLayout;
	QHBoxLayout *pHBoxLayout_LifetimeColorbar = new QHBoxLayout;
	QHBoxLayout *pHBoxLayout_IntensityPropColorbar = new QHBoxLayout;
	QHBoxLayout *pHBoxLayout_IntensityRatioColorbar = new QHBoxLayout;
	QHBoxLayout *pHBoxLayout_PlaqueCompositionColorbar = new QHBoxLayout;
	QHBoxLayout *pHBoxLayout_InflammationColorbar = new QHBoxLayout;

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
		pHBoxLayout_IntensityPropColorbar->setSpacing(1);
		pHBoxLayout_IntensityPropColorbar->addWidget(m_pLabel_IntensityProp);
		pHBoxLayout_IntensityPropColorbar->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
		pHBoxLayout_IntensityPropColorbar->addWidget(m_pLineEdit_IntensityPropMin);
		pHBoxLayout_IntensityPropColorbar->addWidget(m_pImageView_IntensityPropColorbar);
		pHBoxLayout_IntensityPropColorbar->addWidget(m_pLineEdit_IntensityPropMax);

		pHBoxLayout_IntensityRatioColorbar->setSpacing(1);
		pHBoxLayout_IntensityRatioColorbar->addWidget(m_pLabel_IntensityRatio);
		pHBoxLayout_IntensityRatioColorbar->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
		pHBoxLayout_IntensityRatioColorbar->addWidget(m_pLineEdit_IntensityRatioMin);
		pHBoxLayout_IntensityRatioColorbar->addWidget(m_pImageView_IntensityRatioColorbar);
		pHBoxLayout_IntensityRatioColorbar->addWidget(m_pLineEdit_IntensityRatioMax);

		pHBoxLayout_PlaqueCompositionColorbar->setSpacing(1);
		pHBoxLayout_PlaqueCompositionColorbar->addWidget(m_pLabel_PlaqueComposition);
		pHBoxLayout_PlaqueCompositionColorbar->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
		pHBoxLayout_PlaqueCompositionColorbar->addWidget(m_pImageView_PlaqueCompositionColorbar);

		pHBoxLayout_InflammationColorbar->setSpacing(1);
		pHBoxLayout_InflammationColorbar->addWidget(m_pLabel_Inflammation);
		pHBoxLayout_InflammationColorbar->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
		//pHBoxLayout_InflammationColorbar->addWidget(m_pLineEdit_InflammationMin);
		pHBoxLayout_InflammationColorbar->addWidget(m_pImageView_InflammationColorbar);
		//pHBoxLayout_InflammationColorbar->addWidget(m_pLineEdit_InflammationMax);
	}
  
	QVBoxLayout *pVBoxLayout_FlimVisualization = new QVBoxLayout;
	pVBoxLayout_FlimVisualization->setSpacing(5);

	if (!m_pStreamTab) pVBoxLayout_FlimVisualization->addItem(pHBoxLayout_FlimVisualization0);
	pVBoxLayout_FlimVisualization->addItem(pHBoxLayout_FlimVisualization1);
	if (!m_pStreamTab)
	{
		QGridLayout *pGridLayout_FlimVisualization2 = new QGridLayout;
		pGridLayout_FlimVisualization2->setSpacing(3);

		pGridLayout_FlimVisualization2->addWidget(m_pLabel_FLImParameters, 0, 0);
		pGridLayout_FlimVisualization2->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 1);
		pGridLayout_FlimVisualization2->addWidget(m_pRadioButton_Lifetime, 0, 2);
		pGridLayout_FlimVisualization2->addWidget(m_pRadioButton_IntensityProp, 0, 3);
		pGridLayout_FlimVisualization2->addWidget(m_pRadioButton_None, 1, 2);
		pGridLayout_FlimVisualization2->addWidget(m_pRadioButton_IntensityRatio, 1, 3);

		QGridLayout *pGridLayout_FlimVisualization3 = new QGridLayout;
		pGridLayout_FlimVisualization3->setSpacing(3);

		pGridLayout_FlimVisualization3->addWidget(m_pLabel_RFPrediction, 0, 0);
		pGridLayout_FlimVisualization3->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 1);
		pGridLayout_FlimVisualization3->addWidget(m_pRadioButton_PlaqueComposition, 0, 2);
		pGridLayout_FlimVisualization3->addWidget(m_pRadioButton_Inflammation, 0, 3);

		pVBoxLayout_FlimVisualization->addItem(pGridLayout_FlimVisualization2);
		pVBoxLayout_FlimVisualization->addItem(pGridLayout_FlimVisualization3);
	}
	pVBoxLayout_FlimVisualization->addItem(pHBoxLayout_IntensityColorbar);
	pVBoxLayout_FlimVisualization->addItem(pHBoxLayout_LifetimeColorbar);
	if (!m_pStreamTab) pVBoxLayout_FlimVisualization->addItem(pHBoxLayout_IntensityPropColorbar);
	if (!m_pStreamTab) pVBoxLayout_FlimVisualization->addItem(pHBoxLayout_IntensityRatioColorbar);
	if (!m_pStreamTab) pVBoxLayout_FlimVisualization->addItem(pHBoxLayout_PlaqueCompositionColorbar);
	if (!m_pStreamTab) pVBoxLayout_FlimVisualization->addItem(pHBoxLayout_InflammationColorbar);

	pGroupBox_FlimVisualization->setLayout(pVBoxLayout_FlimVisualization);

	m_pVBoxLayout_ViewOption->addWidget(pGroupBox_FlimVisualization);

    // Connect signal and slot
    connect(m_pComboBox_EmissionChannel, SIGNAL(currentIndexChanged(int)), this, SLOT(changeEmissionChannel(int)));
	if (!m_pStreamTab)
	{
		connect(m_pButtonGroup_VisualizationMode, SIGNAL(buttonClicked(int)), this, SLOT(changeVisualizationMode(int)));
		connect(m_pButtonGroup_FLImParameters, SIGNAL(buttonClicked(int)), this, SLOT(changeFLImParameters(int)));
		connect(m_pButtonGroup_RFPrediction, SIGNAL(buttonClicked(int)), this, SLOT(changeRFPrediction(int)));
	}
	connect(m_pLineEdit_IntensityMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
	connect(m_pLineEdit_IntensityMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
	connect(m_pLineEdit_LifetimeMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
	connect(m_pLineEdit_LifetimeMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
	if (!m_pStreamTab)
	{
		connect(m_pLineEdit_IntensityPropMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
		connect(m_pLineEdit_IntensityPropMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
		connect(m_pLineEdit_IntensityRatioMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
		connect(m_pLineEdit_IntensityRatioMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));

		//connect(m_pLineEdit_InflammationMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustInflammationContrast()));
		//connect(m_pLineEdit_InflammationMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustInflammationContrast()));
		
		// Initialization
		changeVisualizationMode(m_pRadioButton_RFPrediction->isChecked());
		if (m_pRadioButton_RFPrediction->isChecked() == false)
			changeFLImParameters(m_pConfig->flimParameterMode);
		else
			changeRFPrediction(m_pRadioButton_Inflammation->isChecked());
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
	
	// Create other features for OCT visualization
	if (!m_pStreamTab)
	{
		m_pCheckBox_ReflectionRemoval = new QCheckBox(this);
		m_pCheckBox_ReflectionRemoval->setText(" Remove OCT Reflection Artifact");
		m_pCheckBox_ReflectionRemoval->setChecked(m_pConfigTemp->reflectionRemoval);

		m_pLineEdit_ReflectionDistance = new QLineEdit(this);
		m_pLineEdit_ReflectionDistance->setFixedWidth(35);
		m_pLineEdit_ReflectionDistance->setText(QString::number(m_pConfigTemp->reflectionDistance));
		m_pLineEdit_ReflectionDistance->setAlignment(Qt::AlignCenter);
		m_pLineEdit_ReflectionDistance->setEnabled(m_pConfigTemp->reflectionRemoval);

		m_pLineEdit_ReflectionLevel = new QLineEdit(this);
		m_pLineEdit_ReflectionLevel->setFixedWidth(35);
		m_pLineEdit_ReflectionLevel->setText(QString::number(m_pConfigTemp->reflectionLevel));
		m_pLineEdit_ReflectionLevel->setAlignment(Qt::AlignCenter);
		m_pLineEdit_ReflectionLevel->setEnabled(m_pConfigTemp->reflectionRemoval);
	}

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
	if (!m_pStreamTab)
	{
		QHBoxLayout *pHBoxLayout_Reflection = new QHBoxLayout;
		pHBoxLayout_Reflection->setSpacing(3);
		if (!m_pStreamTab)
		{
			pHBoxLayout_Reflection->addWidget(m_pCheckBox_ReflectionRemoval);
			pHBoxLayout_Reflection->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
			pHBoxLayout_Reflection->addWidget(m_pLineEdit_ReflectionDistance);
			pHBoxLayout_Reflection->addWidget(m_pLineEdit_ReflectionLevel);
		}
		pVBoxLayout_OctVisualization->addItem(pHBoxLayout_Reflection);
	}
	
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
	if (!m_pStreamTab) connect(m_pCheckBox_ReflectionRemoval, SIGNAL(toggled(bool)), this, SLOT(reflectionRemoval(bool)));
	if (!m_pStreamTab) connect(m_pLineEdit_ReflectionDistance, SIGNAL(textEdited(const QString &)), this, SLOT(changeReflectionDistance(const QString &)));
	if (!m_pStreamTab) connect(m_pLineEdit_ReflectionLevel, SIGNAL(textEdited(const QString &)), this, SLOT(changeReflectionLevel(const QString &)));
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
		m_pScrollBar_IntraFrameSync->setDisabled(true);

		QString str; str.sprintf("Intra Sync %4d / %4d ", m_pConfigTemp->intraFrameSync, m_pConfig->octAlines - 1);
		m_pLabel_IntraFrameSync = new QLabel(str, this);
		m_pLabel_IntraFrameSync->setBuddy(m_pScrollBar_IntraFrameSync);
		m_pLabel_IntraFrameSync->setDisabled(true);

		m_pScrollBar_InterFrameSync = new QScrollBar(this);
		m_pScrollBar_InterFrameSync->setOrientation(Qt::Horizontal);
		m_pScrollBar_InterFrameSync->setRange(-m_pConfigTemp->frames + 1, m_pConfigTemp->frames - 1);
		m_pScrollBar_InterFrameSync->setValue(m_pConfigTemp->interFrameSync);
		m_pScrollBar_InterFrameSync->setSingleStep(1);
		m_pScrollBar_InterFrameSync->setPageStep(1); // m_pScrollBar_Rotation->maximum() / 10);
		m_pScrollBar_InterFrameSync->setFocusPolicy(Qt::StrongFocus);
		m_pScrollBar_InterFrameSync->setFixedSize(200, 18);
		m_pScrollBar_InterFrameSync->setDisabled(true);

		str.sprintf("Inter Sync %4d /|%3d| ", m_pConfigTemp->interFrameSync, m_pConfigTemp->frames - 1);
		m_pLabel_InterFrameSync = new QLabel(str, this);
		m_pLabel_InterFrameSync->setBuddy(m_pScrollBar_InterFrameSync);
		m_pLabel_InterFrameSync->setDisabled(true);

		bool isVibCorrted = m_pResultTab->getVibCorrectionButton()->isChecked();

		m_pScrollBar_FlimDelaySync = new QScrollBar(this);
		m_pScrollBar_FlimDelaySync->setOrientation(Qt::Horizontal);
		m_pScrollBar_FlimDelaySync->setRange(0, 3 * m_pConfig->octAlines - 1);
		m_pScrollBar_FlimDelaySync->setValue(m_pConfigTemp->flimDelaySync);
		m_pScrollBar_FlimDelaySync->setSingleStep(1);
		m_pScrollBar_FlimDelaySync->setPageStep(10);
		m_pScrollBar_FlimDelaySync->setFocusPolicy(Qt::StrongFocus);
		m_pScrollBar_FlimDelaySync->setFixedSize(200, 18);
		m_pScrollBar_FlimDelaySync->setDisabled(isVibCorrted);

		str.sprintf("FLIm Sync %4d / %4d ", m_pConfigTemp->flimDelaySync, 3 * m_pConfig->octAlines - 1);
		m_pLabel_FlimDelaySync = new QLabel(str, this);
		m_pLabel_FlimDelaySync->setBuddy(m_pScrollBar_FlimDelaySync);
		m_pLabel_FlimDelaySync->setDisabled(isVibCorrted);
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
		pGridLayout_FrameSync->addWidget(m_pLabel_FlimDelaySync, 2, 0);
		pGridLayout_FrameSync->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 2, 1);
		pGridLayout_FrameSync->addWidget(m_pScrollBar_FlimDelaySync, 2, 2);
		
		pGroupBox_SyncVisualization->setLayout(pGridLayout_FrameSync);
		m_pVBoxLayout_ViewOption->addWidget(pGroupBox_SyncVisualization);

		// Connect signal and slot
		connect(m_pScrollBar_IntraFrameSync, SIGNAL(valueChanged(int)), this, SLOT(setIntraFrameSync(int)));
		connect(m_pScrollBar_InterFrameSync, SIGNAL(valueChanged(int)), this, SLOT(setInterFrameSync(int)));
		connect(m_pScrollBar_FlimDelaySync, SIGNAL(valueChanged(int)), this, SLOT(setFlimDelaySync(int)));
	}
}


void ViewOptionTab::changeVisualizationMode(int mode)
{
	if (m_pStreamTab)
	{
	}
	else if (m_pResultTab)
	{
		if (m_pViewTab)
		{
			int mode0 = mode % 2;
			int signaling = (mode / 2) == 0;

			if (mode0 == _FLIM_PARAMETERS_)
			{
				m_pLabel_EmissionChannel->setEnabled(true);
				m_pComboBox_EmissionChannel->setEnabled(true);

				m_pLabel_FLImParameters->setEnabled(true);
				m_pRadioButton_Lifetime->setEnabled(true);
				m_pRadioButton_IntensityProp->setEnabled(true);
				m_pRadioButton_IntensityRatio->setEnabled(true);
				m_pRadioButton_None->setEnabled(true);

				m_pLabel_RFPrediction->setDisabled(true);
				m_pRadioButton_PlaqueComposition->setDisabled(true);
				m_pRadioButton_Inflammation->setDisabled(true);

				changeFLImParameters(m_pConfig->flimParameterMode);
			}
			else if (mode0 == _RF_PREDICTION_)
			{
				m_pLabel_EmissionChannel->setDisabled(true);
				m_pComboBox_EmissionChannel->setDisabled(true);

				m_pLabel_FLImParameters->setDisabled(true);
				m_pRadioButton_Lifetime->setDisabled(true);
				m_pRadioButton_IntensityProp->setDisabled(true);
				m_pRadioButton_IntensityRatio->setDisabled(true);
				m_pRadioButton_None->setDisabled(true);

				m_pLabel_RFPrediction->setEnabled(true);
				m_pRadioButton_PlaqueComposition->setEnabled(true);
				m_pRadioButton_Inflammation->setEnabled(true);

				changeRFPrediction(m_pRadioButton_Inflammation->isChecked());
			}

			if (signaling) m_pViewTab->setVisualizationMode(mode);
			m_pViewTab->invalidate();

			m_pConfig->writeToLog(QString("Visualization mode changed: %1").arg(mode));
		}
	}
}

void ViewOptionTab::changeEmissionChannel(int ch)
{
	int ch0 = ch % 3;
	int signaling = (ch / 3) == 0;

    m_pConfig->flimEmissionChannel = ch0 + 1;
	
	m_pLabel_NormIntensity->setText(QString("Ch%1 Intensity (AU) ").arg(ch0 + 1));
	m_pLabel_Lifetime->setText(QString("Ch%1 Lifetime (nsec) ").arg(ch0 + 1));

	m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRange[ch0].min, 'f', 2));
	m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRange[ch0].max, 'f', 2));
	m_pLineEdit_LifetimeMin->setText(QString::number(m_pConfig->flimLifetimeRange[ch0].min, 'f', 1));
	m_pLineEdit_LifetimeMax->setText(QString::number(m_pConfig->flimLifetimeRange[ch0].max, 'f', 1));

	if (m_pStreamTab)
	{
	}
	else if (m_pResultTab)
	{
		m_pLabel_IntensityProp->setText(QString("Ch%1 IntProp (AU) ").arg(ch0 + 1));
		m_pLabel_IntensityRatio->setText(QString("Ch%1/%2 IntRatio (AU) ").arg(m_pConfig->flimEmissionChannel).arg((m_pConfig->flimEmissionChannel == 1) ? 3 : m_pConfig->flimEmissionChannel - 1));

		m_pLineEdit_IntensityPropMin->setText(QString::number(m_pConfig->flimIntensityPropRange[ch0].min, 'f', 1));
		m_pLineEdit_IntensityPropMax->setText(QString::number(m_pConfig->flimIntensityPropRange[ch0].max, 'f', 1));
		m_pLineEdit_IntensityRatioMin->setText(QString::number(m_pConfig->flimIntensityRatioRange[ch0].min, 'f', 1));
		m_pLineEdit_IntensityRatioMax->setText(QString::number(m_pConfig->flimIntensityRatioRange[ch0].max, 'f', 1));

		if (m_pViewTab)
		{
			int mode;
			if (m_pRadioButton_Lifetime->isChecked())
				mode = _LIFETIME_;
			else if (m_pRadioButton_IntensityProp->isChecked())
				mode = _INTENSITY_PROP_;
			else if (m_pRadioButton_IntensityRatio->isChecked())
				mode = _INTENSITY_RATIO_;
			else if (m_pRadioButton_None->isChecked())
				mode = _NONE_;
			
			if (signaling) m_pViewTab->setFLImParameters(ch0 + 3 * mode);
			m_pViewTab->invalidate();

			m_pConfig->writeToLog(QString("FLIm parameters changed: %1").arg(mode));
		}
	}

	m_pConfig->writeToLog(QString("Emission channel changed: ch %1").arg(ch0 + 1));
			
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


void ViewOptionTab::changeFLImParameters(int mode)
{
	m_pLabel_NormIntensity->setVisible(true);
	m_pLineEdit_IntensityMin->setVisible(true);
	m_pImageView_IntensityColorbar->setVisible(true);
	m_pLineEdit_IntensityMax->setVisible(true);

	m_pComboBox_EmissionChannel->setEnabled(true);

	if (mode == FLImParameters::_LIFETIME_)
	{
		m_pLabel_Lifetime->setVisible(true);
		m_pLineEdit_LifetimeMin->setVisible(true);
		m_pImageView_LifetimeColorbar->setVisible(true);
		m_pLineEdit_LifetimeMax->setVisible(true);

		m_pLabel_IntensityProp->setVisible(false);
		m_pLineEdit_IntensityPropMin->setVisible(false);
		m_pImageView_IntensityPropColorbar->setVisible(false);
		m_pLineEdit_IntensityPropMax->setVisible(false);

		m_pLabel_IntensityRatio->setVisible(false);
		m_pLineEdit_IntensityRatioMin->setVisible(false);
		m_pImageView_IntensityRatioColorbar->setVisible(false);
		m_pLineEdit_IntensityRatioMax->setVisible(false);
		
		m_pLabel_PlaqueComposition->setVisible(false);
		m_pImageView_PlaqueCompositionColorbar->setVisible(false);
		
		m_pLabel_Inflammation->setVisible(false);
		//m_pLineEdit_InflammationMin->setVisible(false);
		m_pImageView_InflammationColorbar->setVisible(false);
		//m_pLineEdit_InflammationMax->setVisible(false);
	}
	else if (mode == FLImParameters::_INTENSITY_PROP_)
	{
		m_pLabel_Lifetime->setVisible(false);
		m_pLineEdit_LifetimeMin->setVisible(false);
		m_pImageView_LifetimeColorbar->setVisible(false);
		m_pLineEdit_LifetimeMax->setVisible(false);

		m_pLabel_IntensityProp->setVisible(true);
		m_pLineEdit_IntensityPropMin->setVisible(true);
		m_pImageView_IntensityPropColorbar->setVisible(true);
		m_pLineEdit_IntensityPropMax->setVisible(true);

		m_pLabel_IntensityRatio->setVisible(false);
		m_pLineEdit_IntensityRatioMin->setVisible(false);
		m_pImageView_IntensityRatioColorbar->setVisible(false);
		m_pLineEdit_IntensityRatioMax->setVisible(false);
		
		m_pLabel_PlaqueComposition->setVisible(false);
		m_pImageView_PlaqueCompositionColorbar->setVisible(false);
		
		m_pLabel_Inflammation->setVisible(false);
		//m_pLineEdit_InflammationMin->setVisible(false);
		m_pImageView_InflammationColorbar->setVisible(false);
		//m_pLineEdit_InflammationMax->setVisible(false);
	}
	else if (mode == FLImParameters::_INTENSITY_RATIO_)
	{
		m_pLabel_Lifetime->setVisible(false);
		m_pLineEdit_LifetimeMin->setVisible(false);
		m_pImageView_LifetimeColorbar->setVisible(false);
		m_pLineEdit_LifetimeMax->setVisible(false);

		m_pLabel_IntensityProp->setVisible(false);
		m_pLineEdit_IntensityPropMin->setVisible(false);
		m_pImageView_IntensityPropColorbar->setVisible(false);
		m_pLineEdit_IntensityPropMax->setVisible(false);

		m_pLabel_IntensityRatio->setVisible(true);
		m_pLineEdit_IntensityRatioMin->setVisible(true);
		m_pImageView_IntensityRatioColorbar->setVisible(true);
		m_pLineEdit_IntensityRatioMax->setVisible(true);
		
		m_pLabel_PlaqueComposition->setVisible(false);
		m_pImageView_PlaqueCompositionColorbar->setVisible(false);

		m_pLabel_Inflammation->setVisible(false);
		//m_pLineEdit_InflammationMin->setVisible(false);
		m_pImageView_InflammationColorbar->setVisible(false);
		//m_pLineEdit_InflammationMax->setVisible(false);
	}
	else if (mode == FLImParameters::_NONE_)
	{
		m_pLabel_Lifetime->setVisible(true);
		m_pLineEdit_LifetimeMin->setVisible(true);
		m_pImageView_LifetimeColorbar->setVisible(true);
		m_pLineEdit_LifetimeMax->setVisible(true);

		m_pLabel_IntensityProp->setVisible(false);
		m_pLineEdit_IntensityPropMin->setVisible(false);
		m_pImageView_IntensityPropColorbar->setVisible(false);
		m_pLineEdit_IntensityPropMax->setVisible(false);

		m_pLabel_IntensityRatio->setVisible(false);
		m_pLineEdit_IntensityRatioMin->setVisible(false);
		m_pImageView_IntensityRatioColorbar->setVisible(false);
		m_pLineEdit_IntensityRatioMax->setVisible(false);

		m_pLabel_PlaqueComposition->setVisible(false);
		m_pImageView_PlaqueCompositionColorbar->setVisible(false);

		m_pLabel_Inflammation->setVisible(false);
		//m_pLineEdit_InflammationMin->setVisible(false);
		m_pImageView_InflammationColorbar->setVisible(false);
		//m_pLineEdit_InflammationMax->setVisible(false);

		m_pConfig->flimEmissionChannel = 1;
		m_pComboBox_EmissionChannel->setDisabled(true);
	}

	changeEmissionChannel(m_pConfig->flimEmissionChannel - 1);
	adjustFlimContrast();
}


void ViewOptionTab::changeRFPrediction(int mode)
{
	if (m_pStreamTab)
	{
	}
	else if (m_pResultTab)
	{
		m_pLabel_NormIntensity->setVisible(true);
		m_pLineEdit_IntensityMin->setVisible(true);
		m_pImageView_IntensityColorbar->setVisible(true);
		m_pLineEdit_IntensityMax->setVisible(true);

		if (m_pViewTab)
		{
			int mode0 = mode % 2;
			int signaling = (mode / 2) == 0;

			if (mode0 == _PLAQUE_COMPOSITION_)
			{
				m_pLabel_Lifetime->setVisible(false);
				m_pLineEdit_LifetimeMin->setVisible(false);
				m_pImageView_LifetimeColorbar->setVisible(false);
				m_pLineEdit_LifetimeMax->setVisible(false);

				m_pLabel_IntensityProp->setVisible(false);
				m_pLineEdit_IntensityPropMin->setVisible(false);
				m_pImageView_IntensityPropColorbar->setVisible(false);
				m_pLineEdit_IntensityPropMax->setVisible(false);

				m_pLabel_IntensityRatio->setVisible(false);
				m_pLineEdit_IntensityRatioMin->setVisible(false);
				m_pImageView_IntensityRatioColorbar->setVisible(false);
				m_pLineEdit_IntensityRatioMax->setVisible(false);

				m_pLabel_PlaqueComposition->setVisible(true);
				m_pImageView_PlaqueCompositionColorbar->setVisible(true);

				m_pLabel_Inflammation->setVisible(false);
				//m_pLineEdit_InflammationMin->setVisible(false);
				m_pImageView_InflammationColorbar->setVisible(false);
				//m_pLineEdit_InflammationMax->setVisible(false);
			}
			else if (mode0 == _INFLAMMATION_)
			{
				m_pLabel_Lifetime->setVisible(false);
				m_pLineEdit_LifetimeMin->setVisible(false);
				m_pImageView_LifetimeColorbar->setVisible(false);
				m_pLineEdit_LifetimeMax->setVisible(false);

				m_pLabel_IntensityProp->setVisible(false);
				m_pLineEdit_IntensityPropMin->setVisible(false);
				m_pImageView_IntensityPropColorbar->setVisible(false);
				m_pLineEdit_IntensityPropMax->setVisible(false);

				m_pLabel_IntensityRatio->setVisible(false);
				m_pLineEdit_IntensityRatioMin->setVisible(false);
				m_pImageView_IntensityRatioColorbar->setVisible(false);
				m_pLineEdit_IntensityRatioMax->setVisible(false);

				m_pLabel_PlaqueComposition->setVisible(false);
				m_pImageView_PlaqueCompositionColorbar->setVisible(false);

				m_pLabel_Inflammation->setVisible(true);
				//m_pLineEdit_InflammationMin->setVisible(true);
				m_pImageView_InflammationColorbar->setVisible(true);
				//m_pLineEdit_InflammationMax->setVisible(true);
			}

			if (signaling) m_pViewTab->setRFPrediction(mode);
			m_pViewTab->invalidate();

			m_pConfig->writeToLog(QString("RF prediction mode changed: %1").arg(mode));
		}
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
		else if (m_pRadioButton_IntensityProp->isChecked())
		{
			m_pConfig->flimIntensityPropRange[m_pConfig->flimEmissionChannel - 1].min = m_pLineEdit_IntensityPropMin->text().toFloat();
			m_pConfig->flimIntensityPropRange[m_pConfig->flimEmissionChannel - 1].max = m_pLineEdit_IntensityPropMax->text().toFloat();
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

	m_pConfig->writeToLog(QString("FLIm contrast range set: ch%1 i[%2 %3] l[%4 %5] ip[%6 %7] ir[%8 %9]")
		.arg(m_pConfig->flimEmissionChannel)
		.arg(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min)
		.arg(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max)
		.arg(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min)
		.arg(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max)
		.arg(m_pConfig->flimIntensityPropRange[m_pConfig->flimEmissionChannel - 1].min)
		.arg(m_pConfig->flimIntensityPropRange[m_pConfig->flimEmissionChannel - 1].max)
		.arg(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1].min)
		.arg(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1].max));
}

void ViewOptionTab::adjustInflammationContrast()
{
	//m_pConfig->rfInflammationRange.min = m_pLineEdit_InflammationMin->text().toFloat();
	//m_pConfig->rfInflammationRange.max = m_pLineEdit_InflammationMax->text().toFloat();
	
	if (m_pViewTab) m_pViewTab->invalidate();

	m_pConfig->writeToLog(QString("Inflammation contrast range set: [%1 %2]")
		.arg(m_pConfig->rfInflammationRange.min)
		.arg(m_pConfig->rfInflammationRange.max));
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

void ViewOptionTab::reflectionRemoval(bool enabled)
{
	// Only result tab function
	m_pLineEdit_ReflectionDistance->setEnabled(enabled);
	m_pLineEdit_ReflectionLevel->setEnabled(enabled);

	m_pConfigTemp->reflectionRemoval = enabled;
	m_pResultTab->getViewTab()->invalidate();

	m_pConfig->writeToLog(QString("Reflection removal set: %1").arg(enabled ? "true" : "false"));
}

void ViewOptionTab::changeReflectionDistance(const QString &str)
{
	// Only result tab function
	m_pConfigTemp->reflectionDistance = str.toInt();
	m_pResultTab->getViewTab()->invalidate();

	m_pConfig->writeToLog(QString("Reflection distance set: %1").arg(m_pConfigTemp->reflectionDistance));
}

void ViewOptionTab::changeReflectionLevel(const QString &str)
{
	// Only result tab function
	m_pConfigTemp->reflectionLevel = str.toFloat();
	m_pResultTab->getViewTab()->invalidate();

	m_pConfig->writeToLog(QString("Reflection level set: %1").arg(m_pConfigTemp->reflectionLevel));
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

void ViewOptionTab::setFlimDelaySync(int sync)
{
	// Only result tab function
	m_pConfigTemp->flimDelaySync = sync;

	QString str; str.sprintf("FLIm Sync %4d / %4d ", m_pConfigTemp->flimDelaySync, m_pScrollBar_FlimDelaySync->maximum());
	m_pLabel_FlimDelaySync->setText(str);

	if (m_pViewTab) m_pViewTab->invalidate();

	m_pConfig->writeToLog(QString("FLIm delay sync set: %1").arg(sync));
}