
#include "QVisualizationTab.h"

#include <Havana3/MainWindow.h>
#include <Havana3/QStreamTab.h>
#include <Havana3/QOperationTab.h>
#include <Havana3/QDeviceControlTab.h>
#include <Havana3/QResultTab.h>
#include <Havana3/QProcessingTab.h>

#include <Havana3/Dialog/FlimCalibDlg.h>
#include <Havana3/Dialog/PulseReviewDlg.h>
#include <Havana3/Dialog/LongitudinalViewDlg.h>
#include <Havana3/Viewer/QImageView.h>

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/FLImProcess/FLImProcess.h>

#include <ippcore.h>
#include <ippvm.h>
#include <ipps.h>

#define CIRCULAR_VIEW		-2
#define RECTANGULAR_VIEW	-3


QVisualizationTab::QVisualizationTab(bool is_streaming, QWidget *parent) :
    QDialog(parent), m_pStreamTab(nullptr), m_pResultTab(nullptr),
	m_pImgObjRectImage(nullptr), m_pImgObjCircImage(nullptr), m_pImgObjIntensity(nullptr), m_pImgObjLifetime(nullptr),
	m_pImgObjIntensityMap(nullptr), m_pImgObjLifetimeMap(nullptr), m_pImgObjIntensityWeightedLifetimeMap(nullptr), 
	m_pPulseReviewDlg(nullptr), m_pLongitudinalViewDlg(nullptr),
	m_pCirc(nullptr), m_pMedfilt(nullptr), m_pMedfiltIntensityMap(nullptr), m_pMedfiltLifetimeMap(nullptr), m_pAnn(nullptr)
{
    // Set configuration objects
	if (is_streaming)
	{
		m_pStreamTab = (QStreamTab*)parent;
		m_pConfig = m_pStreamTab->getMainWnd()->m_pConfiguration;
	}
	else
	{
		m_pResultTab = (QResultTab*)parent;
		m_pConfig = m_pResultTab->getMainWnd()->m_pConfiguration;
	}

    // Create layout
    m_pVBoxLayout = new QVBoxLayout;
    m_pVBoxLayout->setSpacing(1);

    // Create image view
    m_pImageView_RectImage = new QImageView(ColorTable::colortable(OCT_COLORTABLE), m_pConfig->octAlines, m_pConfig->octScans, true);
    m_pImageView_RectImage->setMinimumSize(875, 875);
    m_pImageView_RectImage->setSquare(true);
	if (is_streaming)
		m_pImageView_RectImage->setMovedMouseCallback([&] (QPoint& p) { m_pStreamTab->getMainWnd()->m_pStatusLabel_ImagePos->setText(QString("(%1, %2)").arg(p.x(), 4).arg(p.y(), 4)); });
	else
	{
		m_pImageView_RectImage->setMovedMouseCallback([&](QPoint& p) { m_pResultTab->getMainWnd()->m_pStatusLabel_ImagePos->setText(QString("(%1, %2)").arg(p.x(), 4).arg(p.y(), 4)); });
		m_pImageView_RectImage->getRender()->m_bCanBeMagnified = true;
	}
	m_pImageView_RectImage->hide();

	m_pImageView_CircImage = new QImageView(ColorTable::colortable(OCT_COLORTABLE), 2 * m_pConfig->octScans, 2 * m_pConfig->octScans, true);
    m_pImageView_CircImage->setMinimumSize(875, 875);
    m_pImageView_CircImage->setSquare(true);
	if (is_streaming)
		m_pImageView_CircImage->setMovedMouseCallback([&](QPoint& p) { m_pStreamTab->getMainWnd()->m_pStatusLabel_ImagePos->setText(QString("(%1, %2)").arg(p.x(), 4).arg(p.y(), 4)); });
	else
	{
		m_pImageView_CircImage->setMovedMouseCallback([&](QPoint& p) { m_pResultTab->getMainWnd()->m_pStatusLabel_ImagePos->setText(QString("(%1, %2)").arg(p.x(), 4).arg(p.y(), 4)); });
		m_pImageView_CircImage->setWheelMouseCallback([&]() { m_pToggleButton_MeasureDistance->setChecked(false); });
		m_pImageView_CircImage->getRender()->m_bCanBeMagnified = true;
	}


	QLabel *pNullLabel = new QLabel("", this);
	pNullLabel->setMinimumWidth(875);
	pNullLabel->setFixedHeight(0);
	pNullLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);


	// Create Navigation tab
	if (!is_streaming) createNavigationTab();

    // Create FLIM visualization option tab
    createFlimVisualizationOptionTab(is_streaming);

    // Create OCT visualization option tab
    createOctVisualizationOptionTab(is_streaming);

	// Create En Face tab
	if (!is_streaming) createEnFaceMapTab();

	if (is_streaming)
	{
		// Create visualization buffers
		m_visImage = np::Uint8Array2(m_pConfig->octScans, m_pConfig->octAlines);
		m_visIntensity = np::FloatArray2(m_pConfig->flimAlines, 4); memset(m_visIntensity.raw_ptr(), 0, sizeof(float) * m_visIntensity.length());
		m_visMeanDelay = np::FloatArray2(m_pConfig->flimAlines, 4); memset(m_visMeanDelay.raw_ptr(), 0, sizeof(float) * m_visMeanDelay.length());
		m_visLifetime = np::FloatArray2(m_pConfig->flimAlines, 3); memset(m_visLifetime.raw_ptr(), 0, sizeof(float) * m_visLifetime.length());

		// Create image visualization buffers
		ColorTable temp_ctable;
		m_pImgObjRectImage = new ImageObject(m_pConfig->octAlines, m_pConfig->octScans, temp_ctable.m_colorTableVector.at(temp_ctable.gray));
		m_pImgObjCircImage = new ImageObject(2 * m_pConfig->octScans, 2 * m_pConfig->octScans, temp_ctable.m_colorTableVector.at(temp_ctable.gray));
		m_pImgObjIntensity = new ImageObject(m_pConfig->flimAlines, RING_THICKNESS, temp_ctable.m_colorTableVector.at(INTENSITY_COLORTABLE));
		m_pImgObjLifetime = new ImageObject(m_pConfig->flimAlines, RING_THICKNESS, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE));

		m_pCirc = new circularize(m_pConfig->octScans, m_pConfig->octAlines, false);
		m_pMedfilt = new medfilt(m_pConfig->octAlines, m_pConfig->octScans, 3, 3);
	}
	
    // Set layout
    m_pGroupBox_VisualizationWidgets = new QGroupBox;
    m_pGroupBox_VisualizationWidgets->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_pGroupBox_VisualizationWidgets->setTitle("  Visualization Options  ");
    m_pGroupBox_VisualizationWidgets->setStyleSheet("QGroupBox { padding-top: 18px; margin-top: -10px; } QGroupBox::title { subcontrol-origin: margin; left: 8px; top: 2px; }");
	m_pGroupBox_VisualizationWidgets->setFixedWidth(341);
	m_pGroupBox_VisualizationWidgets->setEnabled(is_streaming);

    QVBoxLayout *pVBoxLayout = new QVBoxLayout;
    pVBoxLayout->setSpacing(1);

	if (!is_streaming) pVBoxLayout->addWidget(m_pGroupBox_Navigation);
    pVBoxLayout->addWidget(m_pGroupBox_FlimVisualization);
    pVBoxLayout->addWidget(m_pGroupBox_OctVisualization);

    m_pGroupBox_VisualizationWidgets->setLayout(pVBoxLayout);

    m_pVBoxLayout->addWidget(m_pImageView_RectImage);
    m_pVBoxLayout->addWidget(m_pImageView_CircImage);
	m_pVBoxLayout->addWidget(pNullLabel);

    setLayout(m_pVBoxLayout);

    // Connect signal and slot
	connect(this, SIGNAL(setWidgets(bool, Configuration*)), this, SLOT(setWidgetsEnabled(bool, Configuration*)));
	connect(this, SIGNAL(drawImage(uint8_t*, float*, float*)), this, SLOT(visualizeImage(uint8_t*, float*, float*)));
	connect(this, SIGNAL(makeRgb(ImageObject*, ImageObject*, ImageObject*, ImageObject*)),
		this, SLOT(constructRgbImage(ImageObject*, ImageObject*, ImageObject*, ImageObject*)));
}

QVisualizationTab::~QVisualizationTab()
{
	if (m_pPulseReviewDlg) m_pPulseReviewDlg->close();
	if (m_pLongitudinalViewDlg) m_pLongitudinalViewDlg->close();

	if (m_pImgObjRectImage) delete m_pImgObjRectImage;
	if (m_pImgObjCircImage) delete m_pImgObjCircImage;
	if (m_pImgObjIntensity) delete m_pImgObjIntensity;
	if (m_pImgObjLifetime) delete m_pImgObjLifetime;	

	if (m_pImgObjIntensityMap) delete m_pImgObjIntensityMap;
	if (m_pImgObjLifetimeMap) delete m_pImgObjLifetimeMap;
	if (m_pImgObjIntensityWeightedLifetimeMap) delete m_pImgObjIntensityWeightedLifetimeMap;
	
	if (m_pCirc) delete m_pCirc;
	if (m_pMedfilt) delete m_pMedfilt;
	if (m_pMedfiltIntensityMap) delete m_pMedfiltIntensityMap;
	if (m_pMedfiltLifetimeMap) delete m_pMedfiltLifetimeMap;
}


void QVisualizationTab::setWidgetsValue()
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


void QVisualizationTab::createNavigationTab()
{
	// Create widgets for navigation tab
	m_pGroupBox_Navigation = new QGroupBox;
	m_pGroupBox_Navigation->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	m_pGroupBox_Navigation->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	QGridLayout *pGridLayout_Navigation = new QGridLayout;
	pGridLayout_Navigation->setSpacing(3);

	// Create slider for exploring frames
	m_pSlider_SelectFrame = new QSlider(this);
	m_pSlider_SelectFrame->setOrientation(Qt::Horizontal);
	m_pSlider_SelectFrame->setValue(0);
	m_pSlider_SelectFrame->setDisabled(true);

	m_pLabel_SelectFrame = new QLabel(this);
	QString str; str.sprintf("Current Frame : %3d / %3d", 1, 1);
	m_pLabel_SelectFrame->setText(str);
	m_pLabel_SelectFrame->setFixedWidth(200);
	m_pLabel_SelectFrame->setBuddy(m_pSlider_SelectFrame);
	m_pLabel_SelectFrame->setDisabled(true);

	// Create push button for measuring distance
	m_pToggleButton_MeasureDistance = new QPushButton(this);
	m_pToggleButton_MeasureDistance->setCheckable(true);
	m_pToggleButton_MeasureDistance->setText("Measure Distance");
	m_pToggleButton_MeasureDistance->setFixedWidth(120);
	m_pToggleButton_MeasureDistance->setDisabled(true);
	
	// Set layout
	pGridLayout_Navigation->addWidget(m_pLabel_SelectFrame, 0, 0);
	pGridLayout_Navigation->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 1);
	pGridLayout_Navigation->addWidget(m_pToggleButton_MeasureDistance, 0, 2);
	pGridLayout_Navigation->addWidget(m_pSlider_SelectFrame, 1, 0, 1, 3);

	m_pGroupBox_Navigation->setLayout(pGridLayout_Navigation);

	// Connect signal and slot
	connect(m_pSlider_SelectFrame, SIGNAL(valueChanged(int)), this, SLOT(visualizeImage(int)));
	connect(m_pToggleButton_MeasureDistance, SIGNAL(toggled(bool)), this, SLOT(measureDistance(bool)));
}

void QVisualizationTab::createFlimVisualizationOptionTab(bool is_streaming)
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
	m_pComboBox_EmissionChannel->setFixedWidth(50);
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
		m_pLineEdit_IntensityMax->setFixedWidth(30);
		m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
		m_pLineEdit_IntensityMax->setAlignment(Qt::AlignCenter);
		m_pLineEdit_IntensityMin = new QLineEdit(this);
		m_pLineEdit_IntensityMin->setFixedWidth(30);
		m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
		m_pLineEdit_IntensityMin->setAlignment(Qt::AlignCenter);
		m_pLineEdit_LifetimeMax = new QLineEdit(this);
		m_pLineEdit_LifetimeMax->setFixedWidth(30);
		m_pLineEdit_LifetimeMax->setText(QString::number(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
		m_pLineEdit_LifetimeMax->setAlignment(Qt::AlignCenter);
		m_pLineEdit_LifetimeMin = new QLineEdit(this);
		m_pLineEdit_LifetimeMin->setFixedWidth(30);
		m_pLineEdit_LifetimeMin->setText(QString::number(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
		m_pLineEdit_LifetimeMin->setAlignment(Qt::AlignCenter);

		// Create color bar for FLIM visualization
		uint8_t color[256];
		for (int i = 0; i < 256; i++)
			color[i] = i;

		m_pImageView_IntensityColorbar = new QImageView(ColorTable::colortable(INTENSITY_COLORTABLE), 256, 1);
		m_pImageView_IntensityColorbar->setFixedSize(130, 15);
		m_pImageView_IntensityColorbar->drawImage(color);
		m_pImageView_LifetimeColorbar = new QImageView(ColorTable::colortable(LIFETIME_COLORTABLE), 256, 1);
		m_pImageView_LifetimeColorbar->setFixedSize(130, 15);
		m_pImageView_LifetimeColorbar->drawImage(color);
		m_pLabel_NormIntensity = new QLabel(QString("Ch%1 Intensity (AU) ").arg(m_pConfig->flimEmissionChannel), this);
		m_pLabel_NormIntensity->setFixedWidth(100);
		m_pLabel_NormIntensity->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
		m_pLabel_Lifetime = new QLabel(QString("Ch%1 Lifetime (nsec) ").arg(m_pConfig->flimEmissionChannel), this);
		m_pLabel_Lifetime->setFixedWidth(100);
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

void QVisualizationTab::createOctVisualizationOptionTab(bool is_streaming)
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
	m_pRadioButton_CircularView->setText("Circular View (x-y)");
	m_pRadioButton_CircularView->setEnabled(is_streaming);
	m_pRadioButton_RectangularView = new QRadioButton(this);
	m_pRadioButton_RectangularView->setText(QString::fromLocal8Bit("Rectangular View (еш-r)"));
	m_pRadioButton_RectangularView->setEnabled(is_streaming);

	m_pButtonGroup_ViewMode = new QButtonGroup(this);
	m_pButtonGroup_ViewMode->addButton(m_pRadioButton_CircularView, CIRCULAR_VIEW);
	m_pButtonGroup_ViewMode->addButton(m_pRadioButton_RectangularView, RECTANGULAR_VIEW);
	m_pRadioButton_CircularView->setChecked(true);
		
	// Create widegts for OCT longitudinal visualization
	if (!is_streaming)
	{
		m_pPushButton_LongitudinalView = new QPushButton(this);
		m_pPushButton_LongitudinalView->setText("Longitudinal View (z-r)...");
		m_pPushButton_LongitudinalView->setEnabled(is_streaming);
	}

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
		m_pLineEdit_DecibelMax->setFixedWidth(30);
		m_pLineEdit_DecibelMax->setText(QString::number(m_pConfig->axsunDbRange.max));
		m_pLineEdit_DecibelMax->setAlignment(Qt::AlignCenter);
		m_pLineEdit_DecibelMax->setDisabled(true);
		m_pLineEdit_DecibelMin = new QLineEdit(this);
		m_pLineEdit_DecibelMin->setFixedWidth(30);
		m_pLineEdit_DecibelMin->setText(QString::number(m_pConfig->axsunDbRange.min));
		m_pLineEdit_DecibelMin->setAlignment(Qt::AlignCenter);
		m_pLineEdit_DecibelMin->setDisabled(true);
	}

	// Create color bar for OCT visualization
	uint8_t color[256];
	for (int i = 0; i < 256; i++)
		color[i] = (uint8_t)i;

	m_pImageView_Colorbar = new QImageView(ColorTable::colortable(OCT_COLORTABLE), 256, 1);
	m_pImageView_Colorbar->setFixedSize(130, 15);
	m_pImageView_Colorbar->drawImage(color);
	m_pImageView_Colorbar->setDisabled(true);
	m_pLabel_DecibelRange = new QLabel("OCT Contrast (dB) ", this);
	m_pLabel_DecibelRange->setFixedWidth(100);
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
		pGridLayout_OctVisualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 0);
		pGridLayout_OctVisualization->addWidget(m_pPushButton_LongitudinalView, 1, 1, 1, 2);

		QHBoxLayout *pHBoxLayout_Rotation = new QHBoxLayout;
		pHBoxLayout_Rotation->setSpacing(1);

		pHBoxLayout_Rotation->addWidget(m_pLabel_Rotation);
		pHBoxLayout_Rotation->addWidget(m_pScrollBar_Rotation);

		pGridLayout_OctVisualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 2, 0);
		pGridLayout_OctVisualization->addItem(pHBoxLayout_Rotation, 2, 1, 1, 2);
	}

    m_pGroupBox_OctVisualization->setLayout(pGridLayout_OctVisualization);

    // Connect signal and slot
	connect(m_pButtonGroup_ViewMode, SIGNAL(buttonClicked(int)), this, SLOT(changeViewMode(int)));
	if (!is_streaming) connect(m_pPushButton_LongitudinalView, SIGNAL(clicked(bool)), this, SLOT(createLongitudinalViewDlg()));
	if (is_streaming) connect(m_pLineEdit_DecibelMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustDecibelRange()));
	if (is_streaming) connect(m_pLineEdit_DecibelMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustDecibelRange()));
	if (!is_streaming) connect(m_pScrollBar_Rotation, SIGNAL(valueChanged(int)), this, SLOT(rotateImage(int)));
}

void QVisualizationTab::createEnFaceMapTab()
{
	// En face map tab widgets
	m_pGroupBox_EnFaceWidgets = new QGroupBox;
	m_pGroupBox_EnFaceWidgets->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
	m_pGroupBox_EnFaceWidgets->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	m_pGroupBox_EnFaceWidgets->setFixedWidth(341);
	QGridLayout *pGridLayout_EnFace = new QGridLayout;
	pGridLayout_EnFace->setSpacing(0);

	uint8_t color[256 * 4];
	for (int i = 0; i < 256 * 4; i++)
	    color[i] = 255 - i / 4;

	// Create widgets for OCT projection map
	m_pImageView_OctProjection = new QImageView(ColorTable::colortable(OCT_COLORTABLE), m_pConfig->octAlines, 1);
	m_pImageView_OctProjection->setHLineChangeCallback([&](int frame) { m_pSlider_SelectFrame->setValue(frame); });

	m_pLineEdit_OctGrayMax = new QLineEdit(this);
	m_pLineEdit_OctGrayMax->setText(QString::number(m_pConfig->octGrayRange.max));
	m_pLineEdit_OctGrayMax->setStyleSheet("font: 7pt");
	m_pLineEdit_OctGrayMax->setAlignment(Qt::AlignCenter);
	m_pLineEdit_OctGrayMax->setFixedWidth(25);
	m_pLineEdit_OctGrayMax->setDisabled(true);
	m_pLineEdit_OctGrayMin = new QLineEdit(this);
	m_pLineEdit_OctGrayMin->setText(QString::number(m_pConfig->octGrayRange.min));
	m_pLineEdit_OctGrayMin->setStyleSheet("font: 7pt");
	m_pLineEdit_OctGrayMin->setAlignment(Qt::AlignCenter);
	m_pLineEdit_OctGrayMin->setFixedWidth(25);
	m_pLineEdit_OctGrayMin->setDisabled(true);

	m_pImageView_OctGrayColorbar = new QImageView(ColorTable::colortable(OCT_COLORTABLE), 4, 256);
	m_pImageView_OctGrayColorbar->getRender()->setFixedWidth(10);
	m_pImageView_OctGrayColorbar->drawImage(color);
	m_pImageView_OctGrayColorbar->setFixedWidth(20);

	m_pLabel_OctProjection = new QLabel(this);
	m_pLabel_OctProjection->setText(QString::fromLocal8Bit("OCT Maximum Projection Map (еш-z) (AU)"));
	m_pLabel_OctProjection->setDisabled(true);

	// Create widgets for FLIM intensity map
	m_pImageView_IntensityMap = new QImageView(ColorTable::colortable(INTENSITY_COLORTABLE), m_pConfig->flimAlines, 1);
	m_pImageView_IntensityMap->setHLineChangeCallback([&](int frame) { m_pSlider_SelectFrame->setValue(frame); });
	m_pImageView_IntensityMap->getRender()->m_colorLine = 0x00ff00;

	m_pLineEdit_IntensityMax = new QLineEdit(this);
	m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
	m_pLineEdit_IntensityMax->setStyleSheet("font: 7pt");
	m_pLineEdit_IntensityMax->setAlignment(Qt::AlignCenter);
	m_pLineEdit_IntensityMax->setFixedWidth(25);
	m_pLineEdit_IntensityMax->setDisabled(true);
	m_pLineEdit_IntensityMin = new QLineEdit(this);
	m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
	m_pLineEdit_IntensityMin->setStyleSheet("font: 7pt");
	m_pLineEdit_IntensityMin->setAlignment(Qt::AlignCenter);
	m_pLineEdit_IntensityMin->setFixedWidth(25);
	m_pLineEdit_IntensityMin->setDisabled(true);

	m_pImageView_IntensityColorbar = new QImageView(ColorTable::colortable(INTENSITY_COLORTABLE), 4, 256, false);
	m_pImageView_IntensityColorbar->getRender()->setFixedWidth(10);
	m_pImageView_IntensityColorbar->drawImage(color);
	m_pImageView_IntensityColorbar->setFixedWidth(20);

	m_pLabel_IntensityMap = new QLabel(this);

	m_pLabel_IntensityMap->setText(QString::fromLocal8Bit("FLIm Ch%1 Intensity Map (еш-z) (AU)").arg(m_pConfig->flimEmissionChannel));
	m_pLabel_IntensityMap->setDisabled(true);

	// Create widgets for FLIM lifetime map
	ColorTable temp_ctable;
	m_pImageView_LifetimeMap = new QImageView(ColorTable::colortable(LIFETIME_COLORTABLE), m_pConfig->flimAlines, 1, true);
	m_pImageView_LifetimeMap->setHLineChangeCallback([&](int frame) { m_pSlider_SelectFrame->setValue(frame); });
	m_pImageView_LifetimeMap->getRender()->m_colorLine = 0xffffff;

	m_pLineEdit_LifetimeMax = new QLineEdit(this);
	m_pLineEdit_LifetimeMax->setText(QString::number(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
	m_pLineEdit_LifetimeMax->setStyleSheet("font: 7pt");
	m_pLineEdit_LifetimeMax->setAlignment(Qt::AlignCenter);
	m_pLineEdit_LifetimeMax->setFixedWidth(25);
	m_pLineEdit_LifetimeMax->setDisabled(true);
	m_pLineEdit_LifetimeMin = new QLineEdit(this);
	m_pLineEdit_LifetimeMin->setText(QString::number(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
	m_pLineEdit_LifetimeMin->setStyleSheet("font: 7pt");
	m_pLineEdit_LifetimeMin->setAlignment(Qt::AlignCenter);
	m_pLineEdit_LifetimeMin->setFixedWidth(25);
	m_pLineEdit_LifetimeMin->setDisabled(true);

	m_pImageView_LifetimeColorbar = new QImageView(ColorTable::colortable(LIFETIME_COLORTABLE), 4, 256, false);
	m_pImageView_LifetimeColorbar->getRender()->setFixedWidth(10);
	m_pImageView_LifetimeColorbar->drawImage(color);
	m_pImageView_LifetimeColorbar->setFixedWidth(20);

	m_pLabel_LifetimeMap = new QLabel(this);
	m_pLabel_LifetimeMap->setText(QString::fromLocal8Bit("FLIm Ch%1 Lifetime Map (еш-z) (nsec)").arg(m_pConfig->flimEmissionChannel));
	m_pLabel_LifetimeMap->setDisabled(true);


	// Set layout
	pGridLayout_EnFace->addWidget(m_pLabel_OctProjection, 0, 0, 1, 3);
	pGridLayout_EnFace->addWidget(m_pImageView_OctProjection, 1, 0);
	pGridLayout_EnFace->addWidget(m_pImageView_OctGrayColorbar, 1, 1);
	QVBoxLayout *pVBoxLayout_Colorbar1 = new QVBoxLayout;
	pVBoxLayout_Colorbar1->setSpacing(0);
	pVBoxLayout_Colorbar1->addWidget(m_pLineEdit_OctGrayMax);
	pVBoxLayout_Colorbar1->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
	pVBoxLayout_Colorbar1->addWidget(m_pLineEdit_OctGrayMin);
	pGridLayout_EnFace->addItem(pVBoxLayout_Colorbar1, 1, 2);

	pGridLayout_EnFace->addWidget(m_pLabel_IntensityMap, 2, 0, 1, 3);
	pGridLayout_EnFace->addWidget(m_pImageView_IntensityMap, 3, 0);
	pGridLayout_EnFace->addWidget(m_pImageView_IntensityColorbar, 3, 1);
	QVBoxLayout *pVBoxLayout_Colorbar2 = new QVBoxLayout;
	pVBoxLayout_Colorbar2->setSpacing(0);
	pVBoxLayout_Colorbar2->addWidget(m_pLineEdit_IntensityMax);
	pVBoxLayout_Colorbar2->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
	pVBoxLayout_Colorbar2->addWidget(m_pLineEdit_IntensityMin);
	pGridLayout_EnFace->addItem(pVBoxLayout_Colorbar2, 3, 2);

	pGridLayout_EnFace->addWidget(m_pLabel_LifetimeMap, 4, 0, 1, 3);
	pGridLayout_EnFace->addWidget(m_pImageView_LifetimeMap, 5, 0);
	pGridLayout_EnFace->addWidget(m_pImageView_LifetimeColorbar, 5, 1);
	QVBoxLayout *pVBoxLayout_Colorbar3 = new QVBoxLayout;
	pVBoxLayout_Colorbar3->setSpacing(0);
	pVBoxLayout_Colorbar3->addWidget(m_pLineEdit_LifetimeMax);
	pVBoxLayout_Colorbar3->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
	pVBoxLayout_Colorbar3->addWidget(m_pLineEdit_LifetimeMin);
	pGridLayout_EnFace->addItem(pVBoxLayout_Colorbar3, 5, 2);
	
	m_pGroupBox_EnFaceWidgets->setLayout(pGridLayout_EnFace);

	// Connect signal and slot
	connect(this, SIGNAL(paintOctProjection(uint8_t*)), m_pImageView_OctProjection, SLOT(drawImage(uint8_t*)));
	connect(this, SIGNAL(paintIntensityMap(uint8_t*)), m_pImageView_IntensityMap, SLOT(drawImage(uint8_t*)));
	connect(this, SIGNAL(paintLifetimeMap(uint8_t*)), m_pImageView_LifetimeMap, SLOT(drawImage(uint8_t*)));

	connect(m_pLineEdit_OctGrayMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustOctGrayContrast()));
	connect(m_pLineEdit_OctGrayMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustOctGrayContrast()));
	connect(m_pLineEdit_IntensityMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
	connect(m_pLineEdit_IntensityMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
	connect(m_pLineEdit_LifetimeMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
	connect(m_pLineEdit_LifetimeMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
}


void QVisualizationTab::setBuffers(Configuration* pConfig)
{
	// Clear existed buffers
	std::vector<np::Uint8Array2> clear_vector;
	clear_vector.swap(m_vectorOctImage);
	std::vector<np::FloatArray2> clear_vector1;
	clear_vector1.swap(m_intensityMap);
	std::vector<np::FloatArray2> clear_vector2;
	clear_vector2.swap(m_lifetimeMap);
	std::vector<np::FloatArray2> clear_vector3;
	clear_vector3.swap(m_vectorPulseCrop);
	std::vector<np::FloatArray2> clear_vector4;
	clear_vector4.swap(m_vectorPulseMask);

	// Data buffers
	for (int i = 0; i < pConfig->frames; i++)
	{
		np::Uint8Array2 buffer = np::Uint8Array2(pConfig->octAlines, pConfig->octScans);
		m_vectorOctImage.push_back(buffer);
	}
	m_octProjection = np::Uint8Array2(pConfig->octAlines, pConfig->frames);

	for (int i = 0; i < 3; i++)
	{
		np::FloatArray2 intensity = np::FloatArray2(pConfig->flimAlines, pConfig->frames);
		np::FloatArray2 lifetime = np::FloatArray2(pConfig->flimAlines, pConfig->frames);
		m_intensityMap.push_back(intensity);
		m_lifetimeMap.push_back(lifetime);
	}

	// Visualization buffers
	ColorTable temp_ctable;

	if (m_pImgObjRectImage) delete m_pImgObjRectImage;
	m_pImgObjRectImage = new ImageObject(pConfig->octAlines, pConfig->octScans, temp_ctable.m_colorTableVector.at(OCT_COLORTABLE));
	if (m_pImgObjCircImage) delete m_pImgObjCircImage;
	m_pImgObjCircImage = new ImageObject(2 * m_pConfig->octScans, 2 * m_pConfig->octScans, temp_ctable.m_colorTableVector.at(OCT_COLORTABLE));
	if (m_pImgObjIntensity) delete m_pImgObjIntensity;
	m_pImgObjIntensity = new ImageObject(pConfig->flimAlines, RING_THICKNESS, temp_ctable.m_colorTableVector.at(INTENSITY_COLORTABLE));
	if (m_pImgObjLifetime) delete m_pImgObjLifetime;
	m_pImgObjLifetime = new ImageObject(pConfig->flimAlines, RING_THICKNESS, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE));

	// En face map visualization buffers
	m_visOctProjection = np::Uint8Array2(pConfig->octAlines, pConfig->frames);

	if (m_pImgObjIntensityMap) delete m_pImgObjIntensityMap;
	m_pImgObjIntensityMap = new ImageObject(pConfig->flimAlines, pConfig->frames, temp_ctable.m_colorTableVector.at(INTENSITY_COLORTABLE));
	if (m_pImgObjLifetimeMap) delete m_pImgObjLifetimeMap;
	m_pImgObjLifetimeMap = new ImageObject(pConfig->flimAlines, pConfig->frames, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE));
	if (m_pImgObjIntensityWeightedLifetimeMap) delete m_pImgObjIntensityWeightedLifetimeMap;
	m_pImgObjIntensityWeightedLifetimeMap = new ImageObject(pConfig->flimAlines, pConfig->frames, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE));

	// Circ & Medfilt objects
	if (m_pCirc) delete m_pCirc;
	m_pCirc = new circularize(pConfig->octScans, pConfig->octAlines, false);

	if (m_pMedfilt) delete m_pMedfilt;
	m_pMedfilt = new medfilt(pConfig->octAlines, pConfig->octScans, 3, 3);

	if (m_pMedfiltIntensityMap) delete m_pMedfiltIntensityMap;
	m_pMedfiltIntensityMap = new medfilt(pConfig->flimAlines, pConfig->frames, 5, 3); // 5 3
	if (m_pMedfiltLifetimeMap) delete m_pMedfiltLifetimeMap;
	m_pMedfiltLifetimeMap = new medfilt(pConfig->flimAlines, pConfig->frames, 11, 7); // 11 7
}

void QVisualizationTab::setOctDecibelContrastWidgets(bool enabled)
{
	m_pLabel_DecibelRange->setEnabled(enabled);
	m_pLineEdit_DecibelMax->setEnabled(enabled);
	m_pLineEdit_DecibelMin->setEnabled(enabled);
	m_pImageView_Colorbar->setEnabled(enabled);
}

void QVisualizationTab::setOuterSheathLines(bool setting)
{
	if (setting)
	{
		m_pImageView_RectImage->setHorizontalLine(1, OUTER_SHEATH_POSITION);
		m_pImageView_CircImage->setCircle(1, OUTER_SHEATH_POSITION);
	}
	else
	{
		m_pImageView_RectImage->setHorizontalLine(0);
		m_pImageView_CircImage->setCircle(0);
	}
}

void QVisualizationTab::setWidgetsEnabled(bool enabled, Configuration* pConfig)
{
	// Set visualization widgets
	if (pConfig)
	{
		m_pImageView_RectImage->setEnabled(enabled);
		m_pImageView_RectImage->setUpdatesEnabled(enabled);
		m_pImageView_CircImage->setEnabled(enabled);
		m_pImageView_CircImage->setUpdatesEnabled(enabled);
		if (enabled)
		{
			if (m_pResultTab)
			{
				m_pImageView_RectImage->setMagnDefault();
				m_pImageView_CircImage->setMagnDefault();
			}
		}

		m_pImageView_OctProjection->setEnabled(enabled);
		m_pImageView_OctProjection->setUpdatesEnabled(enabled);
		m_pImageView_IntensityMap->setEnabled(enabled);
		m_pImageView_IntensityMap->setUpdatesEnabled(enabled);
		m_pImageView_LifetimeMap->setEnabled(enabled);
		m_pImageView_LifetimeMap->setUpdatesEnabled(enabled);
	}

	if (enabled && pConfig)
	{
		m_pImageView_RectImage->resetSize(pConfig->octAlines, pConfig->octScans);
		m_pImageView_CircImage->resetSize(2 * pConfig->octScans, 2 * pConfig->octScans);

		m_pImageView_OctProjection->resetSize(pConfig->octAlines, pConfig->frames);
		m_pImageView_IntensityMap->resetSize(pConfig->flimAlines, pConfig->frames);
		m_pImageView_LifetimeMap->resetSize(pConfig->flimAlines, pConfig->frames);
	}

	// Navigation widgets
	m_pGroupBox_VisualizationWidgets->setEnabled(enabled);
	m_pToggleButton_MeasureDistance->setEnabled(false);
	m_pLabel_SelectFrame->setEnabled(enabled);
	if (pConfig)
	{
		QString str; str.sprintf("Current Frame : %3d / %3d", 1, pConfig->frames);
		m_pLabel_SelectFrame->setText(str);
	}
	m_pSlider_SelectFrame->setEnabled(enabled);
	if (enabled && pConfig)
	{
		m_pSlider_SelectFrame->setRange(0, pConfig->frames - 1);
		m_pSlider_SelectFrame->setValue(0);

		int id = m_pButtonGroup_ViewMode->checkedId();
		switch (id)
		{
			case CIRCULAR_VIEW:
				m_pToggleButton_MeasureDistance->setEnabled(true);
				break;
			case RECTANGULAR_VIEW:
				m_pToggleButton_MeasureDistance->setEnabled(false);
				break;
		}
	}

	// FLIm visualization widgets
	m_pGroupBox_FlimVisualization->setEnabled(enabled);
	m_pLabel_EmissionChannel->setEnabled(enabled);
	m_pComboBox_EmissionChannel->setEnabled(enabled);
	
	m_pCheckBox_IntensityRatio->setEnabled(enabled);
	if (m_pCheckBox_IntensityRatio->isChecked() && enabled)
		m_pComboBox_IntensityRef->setEnabled(true);
	else
		m_pComboBox_IntensityRef->setEnabled(false);
	m_pCheckBox_IntensityWeightedLifetimeMap->setEnabled(enabled);
	m_pPushButton_PulseReview->setEnabled(enabled);
	m_pCheckBox_Classification->setEnabled(enabled);

	m_pLineEdit_IntensityMax->setEnabled(enabled);
	m_pLineEdit_IntensityMin->setEnabled(enabled);
	m_pLineEdit_LifetimeMax->setEnabled(enabled);
	m_pLineEdit_LifetimeMin->setEnabled(enabled);

	// OCT visualization widgets
	m_pGroupBox_OctVisualization->setEnabled(enabled);
	m_pRadioButton_CircularView->setEnabled(enabled);
	m_pRadioButton_RectangularView->setEnabled(enabled);
	
	m_pLineEdit_OctGrayMax->setEnabled(enabled);
	m_pLineEdit_OctGrayMin->setEnabled(enabled);

	m_pPushButton_LongitudinalView->setEnabled(enabled);

	QString str; str.sprintf("Rotation %4d / %4d ", 0, pConfig->octAlines - 1);
	m_pLabel_Rotation->setText(str);
	m_pLabel_Rotation->setEnabled(enabled);

	m_pScrollBar_Rotation->setRange(0, pConfig->octAlines - 1);
	m_pScrollBar_Rotation->setValue(0);
	m_pScrollBar_Rotation->setEnabled(enabled);

	// En face widgets
	m_pLabel_OctProjection->setEnabled(enabled);
	m_pLabel_IntensityMap->setEnabled(enabled);
	m_pLabel_LifetimeMap->setEnabled(enabled);
}


void QVisualizationTab::visualizeEnFaceMap(bool scaling)
{
	if (m_octProjection.size(0) != 0)
	{
		if (scaling)
		{
			IppiSize roi_proj = { m_octProjection.size(0), m_octProjection.size(1) };

			// Scaling OCT projection
			np::FloatArray2 scale_temp(roi_proj.width, roi_proj.height);
			ippsConvert_8u32f(m_octProjection.raw_ptr(), scale_temp.raw_ptr(), scale_temp.length());
			ippiScale_32f8u_C1R(scale_temp, sizeof(float) * roi_proj.width, m_visOctProjection, sizeof(uint8_t) * roi_proj.width,
				roi_proj, m_pConfig->octGrayRange.min, m_pConfig->octGrayRange.max);
			ippiMirror_8u_C1IR(m_visOctProjection, sizeof(uint8_t) * roi_proj.width, roi_proj, ippAxsHorizontal);

			// Rotation - OCT projection map
			int shift = m_pScrollBar_Rotation->value();
			if (shift > 0)
			{
				int roi_proj_width_non4 = m_pImageView_RectImage->getRender()->m_pImage->width();
				for (int i = 0; i < roi_proj.height; i++)
				{
					uint8_t* pImg = m_visOctProjection.raw_ptr() + i * roi_proj.width;
					std::rotate(pImg, pImg + shift, pImg + roi_proj_width_non4);
				}
			}

			// Scaling FLIM map
			IppiSize roi_flimproj = { m_intensityMap.at(0).size(0), m_intensityMap.at(0).size(1) };
			
			// Intensity map
			if (!m_pCheckBox_IntensityRatio->isChecked())
			{
				ippiScale_32f8u_C1R(m_intensityMap.at(m_pComboBox_EmissionChannel->currentIndex()), sizeof(float) * roi_flimproj.width,
					m_pImgObjIntensityMap->arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj.width, roi_flimproj,
					m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min,
					m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max);
			}
			// Intensity ratio map
			else
			{
				int num = m_pConfig->flimEmissionChannel - 1;
				int den = ratio_index[num][m_pConfig->flimIntensityRatioRefIdx[num]] - 1;
				np::Uint8Array2 den_zero(roi_flimproj.width, roi_flimproj.height);
				np::FloatArray2 ratio(roi_flimproj.width, roi_flimproj.height);
				ippiCompareC_32f_C1R(m_intensityMap.at(den), sizeof(float) * roi_flimproj.width, 0.0f, 
					den_zero, sizeof(uint8_t) * roi_flimproj.width, roi_flimproj, ippCmpGreater);
				ippsDivC_8u_ISfs(IPP_MAX_8U, den_zero, den_zero.length(), 0);
				ippsDiv_32f(m_intensityMap.at(den), m_intensityMap.at(num), ratio.raw_ptr(), ratio.length());
			
				ippiScale_32f8u_C1R(ratio.raw_ptr(), sizeof(float) * roi_flimproj.width,
					m_pImgObjIntensityMap->arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj.width, roi_flimproj,
					m_pConfig->flimIntensityRatioRange[num][m_pConfig->flimIntensityRatioRefIdx[num]].min,
					m_pConfig->flimIntensityRatioRange[num][m_pConfig->flimIntensityRatioRefIdx[num]].max);
				ippsMul_8u_ISfs(den_zero, m_pImgObjIntensityMap->arr.raw_ptr(), den_zero.length(), 0);
			}

			ippiMirror_8u_C1IR(m_pImgObjIntensityMap->arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj.width, roi_flimproj, ippAxsHorizontal);
            (*m_pMedfiltIntensityMap)(m_pImgObjIntensityMap->arr.raw_ptr());

			// Rotation - FLIM Intensity map
			if (shift > 0)
			{
				for (int i = 0; i < roi_flimproj.height; i++)
				{
					uint8_t* pImg = m_pImgObjIntensityMap->arr.raw_ptr() + i * roi_flimproj.width;
					std::rotate(pImg, pImg + shift / 4, pImg + roi_flimproj.width);
				}
			}

			// Lifetime map
			ippiScale_32f8u_C1R(m_lifetimeMap.at(m_pComboBox_EmissionChannel->currentIndex()), sizeof(float) * roi_flimproj.width,
				m_pImgObjLifetimeMap->arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj.width, roi_flimproj, 
				m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min, 
				m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max);
			ippiMirror_8u_C1IR(m_pImgObjLifetimeMap->arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj.width, roi_flimproj, ippAxsHorizontal);

			(*m_pMedfiltLifetimeMap)(m_pImgObjLifetimeMap->arr.raw_ptr());

			// Rotation - FLIM Lifetime map
			if (shift > 0)
			{
				for (int i = 0; i < roi_flimproj.height; i++)
				{
					uint8_t* pImg = m_pImgObjLifetimeMap->arr.raw_ptr() + i * roi_flimproj.width;
					std::rotate(pImg, pImg + shift / 4, pImg + roi_flimproj.width);
				}
			}
			
			// RGB conversion
			m_pImgObjLifetimeMap->convertRgb();

			///*************************************************************************************************************************************************************************///
			///Ipp32f mean, std;			
			///ippsMeanStdDev_32f(&m_intensityMap.at(m_pComboBox_EmissionChannel->currentIndex())(0, m_pSlider_SelectFrame->value()), m_pConfig->flimAlines, &mean, &std, ippAlgHintFast);
			///printf("inten: %f %f\n", mean, std);
			///ippsMeanStdDev_32f(&m_lifetimeMap.at(m_pComboBox_EmissionChannel->currentIndex())(0, m_pSlider_SelectFrame->value()), m_pConfig->flimAlines, &mean, &std, ippAlgHintFast);
			///printf("lifet: %f %f\n", mean, std);
			///*************************************************************************************************************************************************************************///

			// Intensity-weight lifetime map
			if (m_pCheckBox_IntensityWeightedLifetimeMap->isChecked())
			{
				ColorTable temp_ctable;
				ImageObject tempImgObj(m_pImgObjIntensityWeightedLifetimeMap->getWidth(), m_pImgObjIntensityWeightedLifetimeMap->getHeight(), temp_ctable.m_colorTableVector.at(ColorTable::gray));

				m_pImgObjLifetimeMap->convertRgb();
				memcpy(tempImgObj.qindeximg.bits(), m_pImgObjIntensityMap->arr.raw_ptr(), tempImgObj.qindeximg.byteCount());
				tempImgObj.convertRgb();

				ippsMul_8u_Sfs(m_pImgObjLifetimeMap->qrgbimg.bits(), tempImgObj.qrgbimg.bits(), m_pImgObjIntensityWeightedLifetimeMap->qrgbimg.bits(), tempImgObj.qrgbimg.byteCount(), 8);
			}

			// FLIM-based classification map (neural network)
			if (m_pCheckBox_Classification->isChecked())
			{
				// Ch1, ch2 lifetime and intensity ratio
				np::FloatArray2 ch1_lifetime(m_lifetimeMap.at(0).size(0), m_lifetimeMap.at(0).size(1));
				np::FloatArray2 ch2_lifetime(m_lifetimeMap.at(1).size(0), m_lifetimeMap.at(1).size(1));
				np::FloatArray2 ch1_intensity(m_intensityMap.at(0).size(0), m_intensityMap.at(0).size(1));
				np::FloatArray2 ch2_intensity(m_intensityMap.at(1).size(0), m_intensityMap.at(1).size(1));
				np::FloatArray2 intensity_ratio(m_intensityMap.at(1).size(0), m_intensityMap.at(1).size(1));

				memcpy(ch1_lifetime.raw_ptr(), m_lifetimeMap.at(0).raw_ptr(), sizeof(float) * m_lifetimeMap.at(0).length());
				memcpy(ch2_lifetime.raw_ptr(), m_lifetimeMap.at(1).raw_ptr(), sizeof(float) * m_lifetimeMap.at(1).length());
				memcpy(ch1_intensity.raw_ptr(), m_intensityMap.at(0).raw_ptr(), sizeof(float) * m_intensityMap.at(0).length());
				memcpy(ch2_intensity.raw_ptr(), m_intensityMap.at(1).raw_ptr(), sizeof(float) * m_intensityMap.at(1).length());

				(*m_pMedfiltLifetimeMap)(ch1_lifetime.raw_ptr());
				(*m_pMedfiltLifetimeMap)(ch2_lifetime.raw_ptr());
				(*m_pMedfiltIntensityMap)(ch1_intensity.raw_ptr());
				(*m_pMedfiltIntensityMap)(ch2_intensity.raw_ptr());

				ippsDiv_32f(ch1_intensity.raw_ptr(), ch2_intensity.raw_ptr(), intensity_ratio.raw_ptr(), intensity_ratio.length());

				// Make ann object
				int w = ch1_lifetime.size(0), h = ch1_lifetime.size(1);

				if (m_pAnn) delete m_pAnn;
				m_pAnn = new ann(w, h, m_pConfig->clfAnnXNode, m_pConfig->clfAnnHNode, m_pConfig->clfAnnYNode);

				(*m_pAnn)(ch1_lifetime, ch2_lifetime, intensity_ratio);

				// Rotation - FLIM Classification map
				if (shift > 0)
				{
					for (int i = 0; i < roi_flimproj.height; i++)
					{
						uint8_t* pImg = m_pAnn->GetClfMapPtr() + i * 3 * roi_flimproj.width;
						std::rotate(pImg, pImg + 3 * int(shift / 4), pImg + 3 * roi_flimproj.width);
					}
				}

				// Classification map
				memcpy(m_pImgObjIntensityWeightedLifetimeMap->qrgbimg.bits(), m_pAnn->GetClfMapPtr(), m_pImgObjIntensityWeightedLifetimeMap->qrgbimg.byteCount());

				// Intensity weighting
				if (m_pCheckBox_IntensityWeightedLifetimeMap->isChecked())
				{
					ColorTable temp_ctable;
					ImageObject tempImgObj(m_pImgObjIntensityWeightedLifetimeMap->getWidth(), m_pImgObjIntensityWeightedLifetimeMap->getHeight(), temp_ctable.m_colorTableVector.at(ColorTable::gray));

					memcpy(tempImgObj.qindeximg.bits(), m_pImgObjIntensityMap->arr.raw_ptr(), tempImgObj.qindeximg.byteCount());
					tempImgObj.convertRgb();

					ippsMul_8u_ISfs(tempImgObj.qrgbimg.bits(), m_pImgObjIntensityWeightedLifetimeMap->qrgbimg.bits(), tempImgObj.qrgbimg.byteCount(), 8);
				}

				//ColorTable temp_ctable;
				//ImageObject tempImgObj(m_pImgObjIntensityWeightedLifetimeMap->getWidth(), m_pImgObjIntensityWeightedLifetimeMap->getHeight(), temp_ctable.m_colorTableVector.at(ColorTable::clf));

				//memcpy(tempImgObj.qindeximg.bits(), m_pAnn->GetClfMapPtr(), tempImgObj.qindeximg.byteCount());
				//tempImgObj.convertRgb();

				//memcpy(m_pImgObjIntensityWeightedLifetimeMap->qrgbimg.bits(), tempImgObj.qrgbimg.bits(), tempImgObj.qrgbimg.byteCount());
			}
		}

		emit paintOctProjection(m_visOctProjection);
		emit paintIntensityMap(m_pImgObjIntensityMap->arr.raw_ptr());
		emit paintLifetimeMap((!m_pCheckBox_IntensityWeightedLifetimeMap->isChecked() && !m_pCheckBox_Classification->isChecked()) ? 
								m_pImgObjLifetimeMap->qrgbimg.bits() : m_pImgObjIntensityWeightedLifetimeMap->qrgbimg.bits());
	}
}

void QVisualizationTab::visualizeImage(uint8_t* oct_im, float* intensity, float* lifetime)
{
	IppiSize roi_oct = { m_pConfig->octScans, m_pConfig->octAlines };

	// OCT Visualization
	ippiTranspose_8u_C1R(oct_im, roi_oct.width * sizeof(uint8_t), m_pImgObjRectImage->arr.raw_ptr(), roi_oct.height * sizeof(uint8_t), roi_oct);
	(*m_pMedfilt)(m_pImgObjRectImage->arr.raw_ptr());
	
	// FLIm Visualization
	IppiSize roi_flim = { m_pConfig->flimAlines, 1 };

	float* scanIntensity = intensity + (1 + m_pComboBox_EmissionChannel->currentIndex()) * roi_flim.width;
	uint8_t* rectIntensity = m_pImgObjIntensity->arr.raw_ptr();
	ippiScale_32f8u_C1R(scanIntensity, roi_flim.width * sizeof(float), rectIntensity, roi_flim.width * sizeof(uint8_t), roi_flim, 
		m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min, m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max);

	float* scanLifetime = lifetime + (m_pComboBox_EmissionChannel->currentIndex()) * roi_flim.width;
	uint8_t* rectLifetime = m_pImgObjLifetime->arr.raw_ptr();
	ippiScale_32f8u_C1R(scanLifetime, roi_flim.width * sizeof(float), rectLifetime, roi_flim.width * sizeof(uint8_t), roi_flim, 
		m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min, m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max);

	for (int i = 1; i < RING_THICKNESS; i++)
	{
		memcpy(&m_pImgObjIntensity->arr(0, i), rectIntensity, sizeof(uint8_t) * roi_flim.width);
		memcpy(&m_pImgObjLifetime->arr(0, i), rectLifetime, sizeof(uint8_t) * roi_flim.width);
	}

	emit makeRgb(m_pImgObjRectImage, m_pImgObjCircImage, m_pImgObjIntensity, m_pImgObjLifetime);
}

void QVisualizationTab::visualizeImage(int frame)
{
	if (m_vectorOctImage.size() != 0)
	{
		IppiSize roi_oct = { m_pImgObjRectImage->getWidth(), m_pImgObjRectImage->getHeight() };

		// OCT Visualization
		np::FloatArray2 scale_temp(roi_oct.width, roi_oct.height);
		ippsConvert_8u32f(m_vectorOctImage.at(frame).raw_ptr(), scale_temp.raw_ptr(), scale_temp.length());
		ippiTranspose_32f_C1IR(scale_temp.raw_ptr(), roi_oct.width * sizeof(float), roi_oct);
		ippiScale_32f8u_C1R(scale_temp.raw_ptr(), roi_oct.width * sizeof(float),
			m_pImgObjRectImage->arr.raw_ptr(), roi_oct.width * sizeof(uint8_t), roi_oct, (float)m_pConfig->octGrayRange.min, (float)m_pConfig->octGrayRange.max);
		(*m_pMedfilt)(m_pImgObjRectImage->arr.raw_ptr());

		// Rotation - OCT Image
		int shift = m_pScrollBar_Rotation->value();
		if (shift > 0)
		{
			int roi_oct_height_non4 = m_pImageView_RectImage->getRender()->m_pImage->width();
			for (int i = 0; i < roi_oct.width; i++)
			{
				uint8_t* pImg = m_pImgObjRectImage->arr.raw_ptr() + i * roi_oct.height;
				std::rotate(pImg, pImg + shift, pImg + roi_oct_height_non4);
			}
		}

		// FLIM Visualization		
		uint8_t* rectIntensity = &m_pImgObjIntensityMap->arr(0, m_pImgObjIntensityMap->arr.size(1) - 1 - frame);
		uint8_t* rectLifetime = &m_pImgObjLifetimeMap->arr(0, m_pImgObjLifetimeMap->arr.size(1) - 1 - frame);
		for (int i = 0; i < RING_THICKNESS; i++)
		{
			memcpy(&m_pImgObjIntensity->arr(0, i), rectIntensity, sizeof(uint8_t) * m_pImgObjIntensityMap->arr.size(0));
			memcpy(&m_pImgObjLifetime->arr(0, i), rectLifetime, sizeof(uint8_t) * m_pImgObjLifetimeMap->arr.size(0));
		}
		emit makeRgb(m_pImgObjRectImage, m_pImgObjCircImage, m_pImgObjIntensity, m_pImgObjLifetime);

		// En Face Lines
		m_pImageView_OctProjection->setHorizontalLine(1, m_visOctProjection.size(1) - frame);
		m_pImageView_OctProjection->getRender()->update();
		m_pImageView_IntensityMap->setHorizontalLine(1, m_pImgObjIntensityMap->arr.size(1) - frame);
		m_pImageView_IntensityMap->getRender()->update();
		m_pImageView_LifetimeMap->setHorizontalLine(1, m_pImgObjLifetimeMap->arr.size(1) - frame);
		m_pImageView_LifetimeMap->getRender()->update();
		
		// Pulse Reviews
		if (m_pPulseReviewDlg)
			m_pPulseReviewDlg->drawPulse(m_pPulseReviewDlg->getCurrentAline());

		// Longitudinval View
		if (m_pLongitudinalViewDlg)
		{
			m_pLongitudinalViewDlg->drawLongitudinalImage(m_pLongitudinalViewDlg->getCurrentAline());
			m_pLongitudinalViewDlg->getImageView()->setVerticalLine(1, frame);
			m_pLongitudinalViewDlg->getImageView()->getRender()->update();
		}

		// Status Update
		QString str; str.sprintf("Current Frame : %3d / %3d", frame + 1, (int)m_vectorOctImage.size());
		m_pLabel_SelectFrame->setText(str);
	}
}

void QVisualizationTab::constructRgbImage(ImageObject *rectObj, ImageObject *circObj, ImageObject *intObj, ImageObject *lftObj)
{
	// Convert RGB
	rectObj->convertRgb();
	intObj->convertScaledRgb();
	lftObj->convertScaledRgb();

	// Paste FLIM color ring to RGB rect image
	if ((m_pStreamTab != nullptr) || (!m_pCheckBox_IntensityWeightedLifetimeMap->isChecked() && !m_pCheckBox_Classification->isChecked()))
	{
		memcpy(rectObj->qrgbimg.bits() + 3 * rectObj->arr.size(0) * (rectObj->arr.size(1) - 2 * RING_THICKNESS), intObj->qrgbimg.bits(), intObj->qrgbimg.byteCount());
		memcpy(rectObj->qrgbimg.bits() + 3 * rectObj->arr.size(0) * (rectObj->arr.size(1) - 1 * RING_THICKNESS), lftObj->qrgbimg.bits(), lftObj->qrgbimg.byteCount());
	}
	else // Intensity weighting or classification in ResultTab
	{
		ImageObject hsvObj(lftObj->getWidth(), 1, lftObj->getColorTable());
		memcpy(hsvObj.qrgbimg.bits(),
			m_pImgObjIntensityWeightedLifetimeMap->qrgbimg.bits() + (m_pImgObjIntensityWeightedLifetimeMap->qrgbimg.height() - m_pSlider_SelectFrame->value() - 1) * 3 * m_pImgObjIntensityWeightedLifetimeMap->qrgbimg.width(),
			3 * m_pImgObjIntensityWeightedLifetimeMap->qrgbimg.width());
		hsvObj.scaledRgb4();

		for (int i = 0; i < (int)(RING_THICKNESS); i++)
			memcpy(rectObj->qrgbimg.bits() + 3 * rectObj->arr.size(0) * (rectObj->arr.size(1) - i - 1), hsvObj.qrgbimg.bits(), hsvObj.qrgbimg.byteCount());
	}
		
	int id = m_pButtonGroup_ViewMode->checkedId();
	// Circ View
	if (id == CIRCULAR_VIEW)
	{
		np::Uint8Array2 rect_temp(rectObj->qrgbimg.bits(), 3 * rectObj->arr.size(0), rectObj->arr.size(1));
		(*m_pCirc)(rect_temp, circObj->qrgbimg.bits(), "vertical", "rgb");

		// Draw image
		if (m_pImageView_CircImage->isEnabled()) m_pImageView_CircImage->drawImage(circObj->qrgbimg.bits());
	}
	// Rect View
	else if (id == RECTANGULAR_VIEW)
	{
		// Draw image
		if (m_pImageView_RectImage->isEnabled()) m_pImageView_RectImage->drawImage(rectObj->qrgbimg.bits());
	}
} 


void QVisualizationTab::measureDistance(bool toggled)
{
	m_pImageView_CircImage->getRender()->m_bMeasureDistance = toggled;
	if (!toggled)
	{
		m_pImageView_CircImage->getRender()->m_nClicked = 0;
		m_pImageView_CircImage->getRender()->update();
	}
}

void QVisualizationTab::enableIntensityRatioMode(bool toggled)
{
	// Set widget
	m_pComboBox_IntensityRef->setEnabled(toggled);

	if (toggled)
	{
		if (m_pResultTab)
			m_pLabel_IntensityMap->setText(QString::fromLocal8Bit("FLIm Ch%1 / Ch%2 Intensity Ratio Map (еш-z) (AU)").arg(m_pConfig->flimEmissionChannel)
				.arg(ratio_index[m_pConfig->flimEmissionChannel - 1][m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]]));

		m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1]
			[m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]].min, 'f', 1));
		m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1]
			[m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]].max, 'f', 1));
	}
	else
	{
		if (m_pResultTab)
			m_pLabel_IntensityMap->setText(QString::fromLocal8Bit("FLIm Ch%1 Intensity Map (еш-z) (AU)").arg(m_pConfig->flimEmissionChannel));

		m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
		m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
	}

	// Only result tab function
	visualizeEnFaceMap(true);
	visualizeImage(m_pSlider_SelectFrame->value());
}

void QVisualizationTab::changeIntensityRatioRef(int index)
{
	m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1] = index;

	// Set widget
	if (m_pResultTab)
		m_pLabel_IntensityMap->setText(QString::fromLocal8Bit("FLIm Ch%1 / Ch%2 Intensity Ratio Map (еш-z) (AU)").arg(m_pConfig->flimEmissionChannel)
			.arg(ratio_index[m_pConfig->flimEmissionChannel - 1][m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]]));

	m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1]
		[m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]].min, 'f', 1));
	m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1]
		[m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]].max, 'f', 1));

	// Only result tab function
	visualizeEnFaceMap(true);
	visualizeImage(m_pSlider_SelectFrame->value());
}

void QVisualizationTab::enableIntensityWeightingMode(bool toggled)
{
	if (!toggled)
		m_pLabel_LifetimeMap->setText(QString::fromLocal8Bit("FLIm Ch%1 Lifetime Map (еш-z) (nsec)").arg(m_pConfig->flimEmissionChannel));
	else
		m_pLabel_LifetimeMap->setText(QString::fromLocal8Bit("FLIm Ch%1 Intensity-Weighted Lifetime Map (еш-z) (nsec)").arg(m_pConfig->flimEmissionChannel));

	// Only result tab function
	visualizeEnFaceMap(true);
	visualizeImage(m_pSlider_SelectFrame->value());
}

void QVisualizationTab::changeEmissionChannel(int ch)
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

		if (!m_pCheckBox_IntensityWeightedLifetimeMap->isChecked())
			m_pLabel_LifetimeMap->setText(QString::fromLocal8Bit("FLIm Ch%1 Lifetime Map (еш-z) (nsec)").arg(ch + 1));
		else
			m_pLabel_LifetimeMap->setText(QString::fromLocal8Bit("FLIm Ch%1 Intensity-Weighted Lifetime Map (еш-z) (nsec)").arg(ch + 1));

		if (!m_pCheckBox_IntensityRatio->isChecked())
		{
			m_pLabel_IntensityMap->setText(QString::fromLocal8Bit("FLIm Ch%1 Intensity Map (еш-z) (AU)").arg(ch + 1));
			m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRange[ch].min, 'f', 1));
			m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRange[ch].max, 'f', 1));
		}
		else
		{			
			m_pLabel_IntensityMap->setText(QString::fromLocal8Bit("FLIm Ch%1 / Ch%2 Intensity Ratio Map (еш-z) (AU)").arg(ch + 1)
				.arg(ratio_index[ch][m_pConfig->flimIntensityRatioRefIdx[ch]]));
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
		QDeviceControlTab* pDeviceControlTab = m_pStreamTab->getDeviceControlTab();
		FLImProcess* pFLIm = m_pStreamTab->getOperationTab()->getDataAcq()->getFLIm();

		if (pDeviceControlTab->getFlimCalibDlg())
			emit pDeviceControlTab->getFlimCalibDlg()->plotRoiPulse(pFLIm, 0);

		visualizeImage(m_visImage.raw_ptr(), m_visIntensity.raw_ptr(), m_visLifetime.raw_ptr());
	}
	else if (m_pResultTab)
	{
		visualizeEnFaceMap(true);
		visualizeImage(m_pSlider_SelectFrame->value());
	}
}

void QVisualizationTab::adjustFlimContrast()
{
	if (m_pStreamTab)
	{
		m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min = m_pLineEdit_IntensityMin->text().toFloat();
		m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max = m_pLineEdit_IntensityMax->text().toFloat();
		m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min = m_pLineEdit_LifetimeMin->text().toFloat();
		m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max = m_pLineEdit_LifetimeMax->text().toFloat();

		visualizeImage(m_visImage.raw_ptr(), m_visIntensity.raw_ptr(), m_visLifetime.raw_ptr());
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

		visualizeEnFaceMap(true);
		visualizeImage(m_pSlider_SelectFrame->value());
	}
}

void QVisualizationTab::createPulseReviewDlg()
{
	if (m_pPulseReviewDlg == nullptr)
	{
		m_pPulseReviewDlg = new PulseReviewDlg(this);
		connect(m_pPulseReviewDlg, SIGNAL(finished(int)), this, SLOT(deletePulseReviewDlg()));
		m_pPulseReviewDlg->show();

		m_pResultTab->getVisualizationTab()->getRectImageView()->setVLineChangeCallback([&](int aline) { m_pPulseReviewDlg->setCurrentAline(aline / 4); });
		m_pResultTab->getVisualizationTab()->getRectImageView()->setVerticalLine(1, 0);
		m_pResultTab->getVisualizationTab()->getRectImageView()->getRender()->update();

		m_pResultTab->getVisualizationTab()->getCircImageView()->setRLineChangeCallback([&](int aline) { m_pPulseReviewDlg->setCurrentAline(aline / 4); });
		m_pResultTab->getVisualizationTab()->getCircImageView()->setVerticalLine(1, 0);
		m_pResultTab->getVisualizationTab()->getCircImageView()->getRender()->m_bRadial = true;
		m_pResultTab->getVisualizationTab()->getCircImageView()->getRender()->m_rMax = m_pResultTab->getProcessingTab()->getFLImProcess()->_resize.ny * 4;
		m_pResultTab->getVisualizationTab()->getCircImageView()->getRender()->update();
	}
	m_pPulseReviewDlg->raise();
	m_pPulseReviewDlg->activateWindow();

	m_pPushButton_LongitudinalView->setDisabled(true);
	m_pToggleButton_MeasureDistance->setChecked(false);
	m_pToggleButton_MeasureDistance->setDisabled(true);
}

void QVisualizationTab::deletePulseReviewDlg()
{
	m_pResultTab->getVisualizationTab()->getRectImageView()->setVerticalLine(0);
	m_pResultTab->getVisualizationTab()->getRectImageView()->getRender()->update();

	m_pResultTab->getVisualizationTab()->getCircImageView()->setVerticalLine(0);
	m_pResultTab->getVisualizationTab()->getCircImageView()->getRender()->update();

	m_pPulseReviewDlg->deleteLater();
	m_pPulseReviewDlg = nullptr;

	m_pPushButton_LongitudinalView->setEnabled(true);
	m_pToggleButton_MeasureDistance->setEnabled(true);
}

void QVisualizationTab::enableClassification(bool toggled)
{
	if (!toggled)
		m_pLabel_LifetimeMap->setText(QString::fromLocal8Bit("FLIm Ch%1 Lifetime Map (еш-z) (nsec)").arg(m_pConfig->flimEmissionChannel));
	else
		m_pLabel_LifetimeMap->setText(QString::fromLocal8Bit("FLIm-based Classification Map (еш-z)"));

	// Set enable state of associated widgets 	
	if (toggled) m_pCheckBox_IntensityWeightedLifetimeMap->setChecked(false);
	if (toggled) m_pCheckBox_IntensityRatio->setChecked(false);
64	//m_pCheckBox_IntensityWeightedLifetimeMap->setEnabled(!toggled);
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
	visualizeEnFaceMap(true);
	visualizeImage(m_pSlider_SelectFrame->value());
}


void QVisualizationTab::changeViewMode(int id)
{
    switch (id)
    {
		case CIRCULAR_VIEW:
			m_pImageView_CircImage->show();
			m_pImageView_RectImage->hide();
			break;
		case RECTANGULAR_VIEW:
			if (m_pResultTab) m_pToggleButton_MeasureDistance->setChecked(false);
			m_pImageView_CircImage->hide();
			m_pImageView_RectImage->show();
			break;
    }

	if (m_pStreamTab)
		visualizeImage(m_visImage.raw_ptr(), m_visIntensity.raw_ptr(), m_visLifetime.raw_ptr());
	else if (m_pResultTab)
	{
		visualizeImage(m_pSlider_SelectFrame->value());
		m_pToggleButton_MeasureDistance->setEnabled(id == CIRCULAR_VIEW);
	}
}

void QVisualizationTab::adjustDecibelRange()
{	
	double min = m_pLineEdit_DecibelMin->text().toDouble();
	double max = m_pLineEdit_DecibelMax->text().toDouble();

	m_pConfig->axsunDbRange.min = min;
	m_pConfig->axsunDbRange.max = max;

	m_pStreamTab->getDeviceControlTab()->adjustDecibelRange();
}

void QVisualizationTab::adjustOctGrayContrast()
{
	// Only result tab function
	m_pConfig->octGrayRange.min = m_pLineEdit_OctGrayMin->text().toFloat();
	m_pConfig->octGrayRange.max = m_pLineEdit_OctGrayMax->text().toFloat();

	visualizeEnFaceMap(true);
	visualizeImage(m_pSlider_SelectFrame->value());
}

void QVisualizationTab::createLongitudinalViewDlg()
{
	if (m_pLongitudinalViewDlg == nullptr)
	{
		m_pLongitudinalViewDlg = new LongitudinalViewDlg(this);
		connect(m_pLongitudinalViewDlg, SIGNAL(finished(int)), this, SLOT(deleteLongitudinalViewDlg()));
		m_pLongitudinalViewDlg->show();

		m_pImageView_RectImage->setVLineChangeCallback([&](int aline) { 
			if (aline > getResultTab()->getProcessingTab()->getConfigTemp()->octAlines / 2) 
				aline -= getResultTab()->getProcessingTab()->getConfigTemp()->octAlines / 2; 
			m_pLongitudinalViewDlg->setCurrentAline(aline); 
		});
		m_pImageView_RectImage->setVerticalLine(1, 0);
		m_pImageView_RectImage->getRender()->m_bDiametric = true;
		m_pImageView_RectImage->getRender()->update();

		m_pImageView_CircImage->setRLineChangeCallback([&](int aline) { 
			if (aline > getResultTab()->getProcessingTab()->getConfigTemp()->octAlines / 2) 
				aline -= getResultTab()->getProcessingTab()->getConfigTemp()->octAlines / 2; 
			m_pLongitudinalViewDlg->setCurrentAline(aline); 
		});
		m_pImageView_CircImage->setVerticalLine(1, 0);
		m_pImageView_CircImage->getRender()->m_bRadial = true;
		m_pImageView_CircImage->getRender()->m_bDiametric = true;
		m_pImageView_CircImage->getRender()->m_rMax = getResultTab()->getProcessingTab()->getConfigTemp()->octAlines;
		m_pImageView_CircImage->getRender()->update();

		m_pLongitudinalViewDlg->drawLongitudinalImage(0);
	}
	m_pLongitudinalViewDlg->raise();
	m_pLongitudinalViewDlg->activateWindow();

	m_pPushButton_PulseReview->setDisabled(true);
	m_pToggleButton_MeasureDistance->setChecked(false);
	m_pToggleButton_MeasureDistance->setDisabled(true);
}

void QVisualizationTab::deleteLongitudinalViewDlg()
{
	m_pImageView_RectImage->setVerticalLine(0);
	m_pImageView_RectImage->getRender()->update();
	m_pImageView_RectImage->getRender()->m_bDiametric = false;

	m_pImageView_CircImage->setVerticalLine(0);
	m_pImageView_CircImage->getRender()->update();
	m_pImageView_CircImage->getRender()->m_bDiametric = false;

	m_pLongitudinalViewDlg->deleteLater();
	m_pLongitudinalViewDlg = nullptr;

	m_pPushButton_PulseReview->setEnabled(true);
	m_pToggleButton_MeasureDistance->setEnabled(true);
}

void QVisualizationTab::rotateImage(int shift)
{
	QString str; str.sprintf("Rotation %4d / %4d ", shift, m_pScrollBar_Rotation->maximum());
	m_pLabel_Rotation->setText(str);

	visualizeEnFaceMap(true);
	visualizeImage(m_pSlider_SelectFrame->value());
}
