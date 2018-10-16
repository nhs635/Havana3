
#include "QVisualizationTab.h"

#include <Havana3/MainWindow.h>
#include <Havana3/QStreamTab.h>
#include <Havana3/QOperationTab.h>
#include <Havana3/QDeviceControlTab.h>
#include <Havana3/QResultTab.h>
#include <Havana3/QProcessingTab.h>

#include <Havana3/Dialog/FlimCalibDlg.h>
#include <Havana3/Dialog/PulseReviewDlg.h>
#include <Havana3/Viewer/QImageView.h>

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/FLImProcess/FLImProcess.h>

#include <ippcore.h>
#include <ippvm.h>
#include <ipps.h>


QVisualizationTab::QVisualizationTab(bool is_streaming, QWidget *parent) :
    QDialog(parent), m_pStreamTab(nullptr), m_pResultTab(nullptr),
	m_pImgObjRectImage(nullptr), m_pImgObjCircImage(nullptr), m_pImgObjIntensity(nullptr), m_pImgObjLifetime(nullptr),
	m_pImgObjIntensityMap(nullptr), m_pImgObjLifetimeMap(nullptr), m_pImgObjHsvEnhancedMap(nullptr),
	m_pCirc(nullptr), m_pMedfilt(nullptr), m_pMedfiltIntensityMap(nullptr), m_pMedfiltLifetimeMap(nullptr)
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
    m_pImageView_RectImage = new QImageView(ColorTable::colortable(m_pConfig->octColorTable), m_pConfig->octAlines, m_pConfig->octScans, true);
    m_pImageView_RectImage->setMinimumSize(500, 500);
	m_pImageView_RectImage->setHorizontalLine(1, OUTER_SHEATH_POSITION);
    m_pImageView_RectImage->setSquare(true);
	if (is_streaming)
		m_pImageView_RectImage->setMovedMouseCallback([&] (QPoint& p) { m_pStreamTab->getMainWnd()->m_pStatusLabel_ImagePos->setText(QString("(%1, %2)").arg(p.x(), 4).arg(p.y(), 4)); });
	else
		m_pImageView_RectImage->setMovedMouseCallback([&](QPoint& p) { m_pResultTab->getMainWnd()->m_pStatusLabel_ImagePos->setText(QString("(%1, %2)").arg(p.x(), 4).arg(p.y(), 4)); });


	m_pImageView_CircImage = new QImageView(ColorTable::colortable(m_pConfig->octColorTable), 2 * m_pConfig->octScans, 2 * m_pConfig->octScans, true);
    m_pImageView_CircImage->setMinimumSize(500, 500);
	m_pImageView_CircImage->setCircle(1, OUTER_SHEATH_POSITION);
    m_pImageView_CircImage->setSquare(true);
	if (is_streaming)
		m_pImageView_CircImage->setMovedMouseCallback([&](QPoint& p) { m_pStreamTab->getMainWnd()->m_pStatusLabel_ImagePos->setText(QString("(%1, %2)").arg(p.x(), 4).arg(p.y(), 4)); });
	else
		m_pImageView_CircImage->setMovedMouseCallback([&](QPoint& p) { m_pResultTab->getMainWnd()->m_pStatusLabel_ImagePos->setText(QString("(%1, %2)").arg(p.x(), 4).arg(p.y(), 4)); });

    m_pImageView_CircImage->hide();

	QLabel *pNullLabel = new QLabel("", this);
	pNullLabel->setMinimumWidth(500);
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
		m_pImgObjLifetime = new ImageObject(m_pConfig->flimAlines, RING_THICKNESS, temp_ctable.m_colorTableVector.at(m_pConfig->flimLifetimeColorTable));

		m_pCirc = new circularize(m_pConfig->octScans, m_pConfig->octAlines, false);
		m_pMedfilt = new medfilt(m_pConfig->octAlines, m_pConfig->octScans, 3, 3);
	}
	
    // Set layout
    m_pGroupBox_VisualizationWidgets = new QGroupBox;
    m_pGroupBox_VisualizationWidgets->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_pGroupBox_VisualizationWidgets->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");

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
	if (m_pImgObjRectImage) delete m_pImgObjRectImage;
	if (m_pImgObjCircImage) delete m_pImgObjCircImage;
	if (m_pImgObjIntensity) delete m_pImgObjIntensity;
	if (m_pImgObjLifetime) delete m_pImgObjLifetime;	

	if (m_pImgObjIntensityMap) delete m_pImgObjIntensityMap;
	if (m_pImgObjLifetimeMap) delete m_pImgObjLifetimeMap;
	if (m_pImgObjHsvEnhancedMap) delete m_pImgObjHsvEnhancedMap;
	
	if (m_pCirc) delete m_pCirc;
	if (m_pMedfilt) delete m_pMedfilt;
	if (m_pMedfiltIntensityMap) delete m_pMedfiltIntensityMap;
	if (m_pMedfiltLifetimeMap) delete m_pMedfiltLifetimeMap;
}


void QVisualizationTab::setWidgetsValue()
{
	m_pComboBox_EmissionChannel->setCurrentIndex(m_pConfig->flimEmissionChannel - 1);
	
	m_pComboBox_LifetimeColorTable->setCurrentIndex(m_pConfig->flimLifetimeColorTable);
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
	
	m_pComboBox_OctColorTable->setCurrentIndex(m_pConfig->octColorTable);
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
    m_pGroupBox_FlimVisualization->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
    QGridLayout *pGridLayout_FlimVisualization = new QGridLayout;
    pGridLayout_FlimVisualization->setSpacing(3);

    // Create widgets for FLIm emission control
    m_pComboBox_EmissionChannel = new QComboBox(this);
    m_pComboBox_EmissionChannel->addItem("Ch 1");
    m_pComboBox_EmissionChannel->addItem("Ch 2");
    m_pComboBox_EmissionChannel->addItem("Ch 3");
    m_pComboBox_EmissionChannel->setCurrentIndex(m_pConfig->flimEmissionChannel - 1);
	m_pComboBox_EmissionChannel->setEnabled(is_streaming);
    m_pLabel_EmissionChannel = new QLabel("Em Channel  ", this);
    m_pLabel_EmissionChannel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    m_pLabel_EmissionChannel->setBuddy(m_pComboBox_EmissionChannel);
	m_pLabel_EmissionChannel->setEnabled(is_streaming);

    // Create widgets for FLIm lifetime colortable selection
    ColorTable temp_ctable;
    m_pComboBox_LifetimeColorTable = new QComboBox(this);
    for (int i = 0; i < temp_ctable.m_cNameVector.size(); i++)
        m_pComboBox_LifetimeColorTable->addItem(temp_ctable.m_cNameVector.at(i));
    m_pComboBox_LifetimeColorTable->setCurrentIndex(m_pConfig->flimLifetimeColorTable);
	m_pComboBox_LifetimeColorTable->setEnabled(is_streaming);
    m_pLabel_LifetimeColorTable = new QLabel("    Lifetime Colortable  ", this);
    m_pLabel_LifetimeColorTable->setBuddy(m_pComboBox_LifetimeColorTable);
	m_pLabel_LifetimeColorTable->setEnabled(is_streaming);
	
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
		m_pComboBox_IntensityRef->setEnabled(is_streaming);

		// Create widgets for FLIm HSV enhanced map	
		m_pCheckBox_HsvEnhancedMap = new QCheckBox(this);
		m_pCheckBox_HsvEnhancedMap->setText("HSV Enhanced Map");
		m_pCheckBox_HsvEnhancedMap->setEnabled(is_streaming);

        // Create widgets for FLIm sync adjust slider
		m_pLineEdit_FlimInterFrameSync = new QLineEdit(this);
		m_pLineEdit_FlimInterFrameSync->setFixedWidth(30);
		m_pLineEdit_FlimInterFrameSync->setText(QString::number(m_pConfig->flimSyncInterFrame));
		m_pLineEdit_FlimInterFrameSync->setAlignment(Qt::AlignCenter);
		m_pLineEdit_FlimInterFrameSync->setEnabled(is_streaming);
		
		m_pLabel_FlimInterFrameSync = new QLabel("Frame Sync ", this);
		m_pLabel_FlimInterFrameSync->setBuddy(m_pLineEdit_FlimInterFrameSync);
		m_pLabel_FlimInterFrameSync->setEnabled(is_streaming);

        m_pSlider_FlimSyncAdjust = new QSlider(this);
        m_pSlider_FlimSyncAdjust->setFixedWidth(100);
        m_pSlider_FlimSyncAdjust->setOrientation(Qt::Horizontal);
        m_pSlider_FlimSyncAdjust->setRange(0, FLIM_ALINES - 1);
        m_pSlider_FlimSyncAdjust->setValue(0);
        m_pSlider_FlimSyncAdjust->setEnabled(is_streaming);

        m_pLabel_FlimSyncAdjust = new QLabel(QString(" Sync Adjust (%1)").arg(0, 3, 10), this);
        m_pLabel_FlimSyncAdjust->setFixedWidth(90);
        m_pLabel_FlimSyncAdjust->setBuddy(m_pSlider_FlimSyncAdjust);
        m_pLabel_FlimSyncAdjust->setEnabled(is_streaming);
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
		m_pImageView_IntensityColorbar->setFixedSize(150, 15);
		m_pImageView_IntensityColorbar->drawImage(color);
		m_pImageView_LifetimeColorbar = new QImageView(ColorTable::colortable(m_pConfig->flimLifetimeColorTable), 256, 1);
		m_pImageView_LifetimeColorbar->setFixedSize(150, 15);
		m_pImageView_LifetimeColorbar->drawImage(color);
		m_pLabel_NormIntensity = new QLabel(QString("Ch%1 Intensity ").arg(m_pConfig->flimEmissionChannel), this);
		m_pLabel_NormIntensity->setFixedWidth(70);
		m_pLabel_NormIntensity->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
		m_pLabel_Lifetime = new QLabel(QString("Ch%1 Lifetime ").arg(m_pConfig->flimEmissionChannel), this);
		m_pLabel_Lifetime->setFixedWidth(70);
		m_pLabel_Lifetime->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
	}

    // Set layout
    QHBoxLayout *pHBoxLayout_FlimVisualization1 = new QHBoxLayout;
	pHBoxLayout_FlimVisualization1->setSpacing(3);
    pHBoxLayout_FlimVisualization1->addWidget(m_pLabel_EmissionChannel);
    pHBoxLayout_FlimVisualization1->addWidget(m_pComboBox_EmissionChannel);
    pHBoxLayout_FlimVisualization1->addWidget(m_pLabel_LifetimeColorTable);
    pHBoxLayout_FlimVisualization1->addWidget(m_pComboBox_LifetimeColorTable);

	QHBoxLayout *pHBoxLayout_FlimVisualization2 = new QHBoxLayout;
    QHBoxLayout *pHBoxLayout_FlimVisualization3 = new QHBoxLayout;
	QHBoxLayout *pHBoxLayout_IntensityColorbar = new QHBoxLayout;
	QHBoxLayout *pHBoxLayout_LifetimeColorbar = new QHBoxLayout;

	if (!is_streaming)
	{
		pHBoxLayout_FlimVisualization2->setSpacing(3);
		pHBoxLayout_FlimVisualization2->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
		pHBoxLayout_FlimVisualization2->addWidget(m_pCheckBox_IntensityRatio);
		pHBoxLayout_FlimVisualization2->addWidget(m_pComboBox_IntensityRef);
		pHBoxLayout_FlimVisualization2->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
		pHBoxLayout_FlimVisualization2->addWidget(m_pCheckBox_HsvEnhancedMap);

        pHBoxLayout_FlimVisualization3->setSpacing(3);
		pHBoxLayout_FlimVisualization3->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
		pHBoxLayout_FlimVisualization3->addWidget(m_pLabel_FlimInterFrameSync);
		pHBoxLayout_FlimVisualization3->addWidget(m_pLineEdit_FlimInterFrameSync);
        pHBoxLayout_FlimVisualization3->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
        pHBoxLayout_FlimVisualization3->addWidget(m_pLabel_FlimSyncAdjust);
        pHBoxLayout_FlimVisualization3->addWidget(m_pSlider_FlimSyncAdjust);
	}
	else
	{
		pHBoxLayout_IntensityColorbar->setSpacing(3);
		pHBoxLayout_IntensityColorbar->addWidget(m_pLabel_NormIntensity);
		pHBoxLayout_IntensityColorbar->addWidget(m_pLineEdit_IntensityMin);
		pHBoxLayout_IntensityColorbar->addWidget(m_pImageView_IntensityColorbar);
		pHBoxLayout_IntensityColorbar->addWidget(m_pLineEdit_IntensityMax);

		pHBoxLayout_LifetimeColorbar->setSpacing(3);
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
		pGridLayout_FlimVisualization->addItem(pHBoxLayout_FlimVisualization2, 1, 1);
        pGridLayout_FlimVisualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 2, 0);
        pGridLayout_FlimVisualization->addItem(pHBoxLayout_FlimVisualization3, 2, 1);
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
    connect(m_pComboBox_LifetimeColorTable, SIGNAL(currentIndexChanged(int)), this, SLOT(changeLifetimeColorTable(int)));
	if (!is_streaming)
	{
		connect(m_pCheckBox_IntensityRatio, SIGNAL(toggled(bool)), this, SLOT(enableIntensityRatioMode(bool)));
		connect(m_pComboBox_IntensityRef, SIGNAL(currentIndexChanged(int)), this, SLOT(changeIntensityRatioRef(int)));
		connect(m_pCheckBox_HsvEnhancedMap, SIGNAL(toggled(bool)), this, SLOT(enableHsvEnhancingMode(bool)));
		connect(m_pLineEdit_FlimInterFrameSync, SIGNAL(textEdited(const QString &)), this, SLOT(setFlimInterFrameSync(const QString &)));
        connect(m_pSlider_FlimSyncAdjust, SIGNAL(valueChanged(int)), this, SLOT(setFlimSyncAdjust(int)));
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
    m_pGroupBox_OctVisualization->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
    QGridLayout *pGridLayout_OctVisualization = new QGridLayout;
    pGridLayout_OctVisualization->setSpacing(3);

    // Create widgets for OCT visualization
    m_pCheckBox_CircularizeImage = new QCheckBox(this);
    m_pCheckBox_CircularizeImage->setText("Circularize Image");
	m_pCheckBox_CircularizeImage->setEnabled(is_streaming);

    // Create widgets for OCT color table
    m_pComboBox_OctColorTable = new QComboBox(this);
    m_pComboBox_OctColorTable->addItem("gray");
    m_pComboBox_OctColorTable->addItem("invgray");
    m_pComboBox_OctColorTable->addItem("sepia");
    m_pComboBox_OctColorTable->setCurrentIndex(m_pConfig->octColorTable);
	m_pComboBox_OctColorTable->setEnabled(is_streaming);
    m_pLabel_OctColorTable = new QLabel("   OCT Colortable  ", this);
    m_pLabel_OctColorTable->setBuddy(m_pComboBox_OctColorTable);
	m_pLabel_OctColorTable->setEnabled(is_streaming);
	
    // Set layout
    pGridLayout_OctVisualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 0);
    pGridLayout_OctVisualization->addWidget(m_pCheckBox_CircularizeImage, 0, 1);
    pGridLayout_OctVisualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed), 0, 2);
    pGridLayout_OctVisualization->addWidget(m_pLabel_OctColorTable, 0, 3);
    pGridLayout_OctVisualization->addWidget(m_pComboBox_OctColorTable, 0, 4);

    m_pGroupBox_OctVisualization->setLayout(pGridLayout_OctVisualization);

    // Connect signal and slot
    connect(m_pCheckBox_CircularizeImage, SIGNAL(toggled(bool)), this, SLOT(changeVisImage(bool)));
    connect(m_pComboBox_OctColorTable, SIGNAL(currentIndexChanged(int)), this, SLOT(changeOctColorTable(int)));
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
	m_pImageView_OctProjection = new QImageView(ColorTable::colortable(m_pConfig->octColorTable), m_pConfig->octAlines, 1);
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

	m_pImageView_OctGrayColorbar = new QImageView(ColorTable::colortable(m_pConfig->octColorTable), 4, 256);
	m_pImageView_OctGrayColorbar->getRender()->setFixedWidth(10);
	m_pImageView_OctGrayColorbar->drawImage(color);
	m_pImageView_OctGrayColorbar->setFixedWidth(20);

	m_pLabel_OctProjection = new QLabel(this);
	m_pLabel_OctProjection->setText("OCT Maximum Projection Map");
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
	m_pLabel_IntensityMap->setText(QString("FLIm Ch%1 Intensity Map").arg(m_pConfig->flimEmissionChannel));
	m_pLabel_IntensityMap->setDisabled(true);

	// Create widgets for FLIM lifetime map
	ColorTable temp_ctable;
	m_pImageView_LifetimeMap = new QImageView(ColorTable::colortable(m_pConfig->flimLifetimeColorTable), m_pConfig->flimAlines, 1, true);
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

	m_pImageView_LifetimeColorbar = new QImageView(ColorTable::colortable(m_pConfig->flimLifetimeColorTable), 4, 256, false);
	m_pImageView_LifetimeColorbar->getRender()->setFixedWidth(10);
	m_pImageView_LifetimeColorbar->drawImage(color);
	m_pImageView_LifetimeColorbar->setFixedWidth(20);

	m_pLabel_LifetimeMap = new QLabel(this);
	m_pLabel_LifetimeMap->setText(QString("FLIm Ch%1 Lifetime Map").arg(m_pConfig->flimEmissionChannel));
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
	m_pImgObjRectImage = new ImageObject(pConfig->octAlines, pConfig->octScans, temp_ctable.m_colorTableVector.at(m_pComboBox_OctColorTable->currentIndex()));
	if (m_pImgObjCircImage) delete m_pImgObjCircImage;
	m_pImgObjCircImage = new ImageObject(2 * m_pConfig->octScans, 2 * m_pConfig->octScans, temp_ctable.m_colorTableVector.at(m_pComboBox_OctColorTable->currentIndex()));
	if (m_pImgObjIntensity) delete m_pImgObjIntensity;
	m_pImgObjIntensity = new ImageObject(pConfig->flimAlines, RING_THICKNESS, temp_ctable.m_colorTableVector.at(INTENSITY_COLORTABLE));
	if (m_pImgObjLifetime) delete m_pImgObjLifetime;
	m_pImgObjLifetime = new ImageObject(pConfig->flimAlines, RING_THICKNESS, temp_ctable.m_colorTableVector.at(m_pComboBox_LifetimeColorTable->currentIndex()));

	// En face map visualization buffers
	m_visOctProjection = np::Uint8Array2(pConfig->octAlines, pConfig->frames);

	if (m_pImgObjIntensityMap) delete m_pImgObjIntensityMap;
	m_pImgObjIntensityMap = new ImageObject(pConfig->flimAlines, pConfig->frames, temp_ctable.m_colorTableVector.at(INTENSITY_COLORTABLE));
	if (m_pImgObjLifetimeMap) delete m_pImgObjLifetimeMap;
	m_pImgObjLifetimeMap = new ImageObject(pConfig->flimAlines, pConfig->frames, temp_ctable.m_colorTableVector.at(m_pComboBox_LifetimeColorTable->currentIndex()));
	if (m_pImgObjHsvEnhancedMap) delete m_pImgObjHsvEnhancedMap;
	m_pImgObjHsvEnhancedMap = new ImageObject(pConfig->flimAlines, pConfig->frames, temp_ctable.m_colorTableVector.at(m_pComboBox_LifetimeColorTable->currentIndex()));

	// Circ & Medfilt objects
	if (m_pCirc) delete m_pCirc;
	m_pCirc = new circularize(pConfig->octScans, pConfig->octAlines, false);

	if (m_pMedfilt) delete m_pMedfilt;
	m_pMedfilt = new medfilt(pConfig->octAlines, pConfig->octScans, 3, 3);

	if (m_pMedfiltIntensityMap) delete m_pMedfiltIntensityMap;
	m_pMedfiltIntensityMap = new medfilt(pConfig->flimAlines, pConfig->frames, 3, 3); // 5 3
	if (m_pMedfiltLifetimeMap) delete m_pMedfiltLifetimeMap;
	m_pMedfiltLifetimeMap = new medfilt(pConfig->flimAlines, pConfig->frames, 7, 5); // 11 7
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

		if (m_pCheckBox_CircularizeImage->isChecked())
			m_pToggleButton_MeasureDistance->setEnabled(enabled);
	}

	// FLIm visualization widgets
	m_pLabel_EmissionChannel->setEnabled(enabled);
	m_pComboBox_EmissionChannel->setEnabled(enabled);

	m_pLabel_LifetimeColorTable->setEnabled(enabled);
	m_pComboBox_LifetimeColorTable->setEnabled(enabled);

	m_pCheckBox_IntensityRatio->setEnabled(enabled);
	if (m_pCheckBox_IntensityRatio->isChecked() && enabled)
		m_pComboBox_IntensityRef->setEnabled(true);
	else
		m_pComboBox_IntensityRef->setEnabled(false);
	m_pCheckBox_HsvEnhancedMap->setEnabled(enabled);
	m_pLabel_FlimInterFrameSync->setEnabled(enabled);
	m_pLineEdit_FlimInterFrameSync->setEnabled(enabled);
    m_pLabel_FlimSyncAdjust->setEnabled(enabled);
    m_pSlider_FlimSyncAdjust->setEnabled(enabled);

	m_pLineEdit_IntensityMax->setEnabled(enabled);
	m_pLineEdit_IntensityMin->setEnabled(enabled);
	m_pLineEdit_LifetimeMax->setEnabled(enabled);
	m_pLineEdit_LifetimeMin->setEnabled(enabled);

	// OCT visualization widgets
	m_pCheckBox_CircularizeImage->setEnabled(enabled);

	m_pLabel_OctColorTable->setEnabled(enabled);
	m_pComboBox_OctColorTable->setEnabled(enabled);

	m_pLineEdit_OctGrayMax->setEnabled(enabled);
	m_pLineEdit_OctGrayMin->setEnabled(enabled);

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

			// Scaling FLIM map
			IppiSize roi_flimproj = { m_intensityMap.at(0).size(0), m_intensityMap.at(0).size(1) };
			
			if (!m_pCheckBox_IntensityRatio->isChecked())
			{
				ippiScale_32f8u_C1R(m_intensityMap.at(m_pComboBox_EmissionChannel->currentIndex()), sizeof(float) * roi_flimproj.width,
					m_pImgObjIntensityMap->arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj.width, roi_flimproj,
					m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min,
					m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max);
				ippiMirror_8u_C1IR(m_pImgObjIntensityMap->arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj.width, roi_flimproj, ippAxsHorizontal);
			}
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
				ippiMirror_8u_C1IR(m_pImgObjIntensityMap->arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj.width, roi_flimproj, ippAxsHorizontal);
			}
            (*m_pMedfiltIntensityMap)(m_pImgObjIntensityMap->arr.raw_ptr());
			if (m_pConfig->flimSyncInterFrame < m_pImgObjIntensityMap->arr.size(1))
			{
				memcpy(&m_pImgObjIntensityMap->arr(0, 0), &m_pImgObjIntensityMap->arr(0, m_pConfig->flimSyncInterFrame),
					m_pImgObjIntensityMap->arr.size(0) * (m_pImgObjIntensityMap->arr.size(1) - m_pConfig->flimSyncInterFrame));
				memset(&m_pImgObjIntensityMap->arr(0, m_pImgObjIntensityMap->arr.size(1) - m_pConfig->flimSyncInterFrame), 0,
					m_pImgObjIntensityMap->arr.size(0) * m_pConfig->flimSyncInterFrame);
			}

            int adjust = m_pSlider_FlimSyncAdjust->value();
            for (int i = 0; i < roi_flimproj.height; i++)
            {
                uint8_t* pImg = m_pImgObjIntensityMap->arr.raw_ptr() + i * roi_flimproj.width;
                std::rotate(pImg, pImg + adjust, pImg + roi_flimproj.width);
            }

			ippiScale_32f8u_C1R(m_lifetimeMap.at(m_pComboBox_EmissionChannel->currentIndex()), sizeof(float) * roi_flimproj.width,
				m_pImgObjLifetimeMap->arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj.width, roi_flimproj, 
				m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min, 
				m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max);
			ippiMirror_8u_C1IR(m_pImgObjLifetimeMap->arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj.width, roi_flimproj, ippAxsHorizontal);

			(*m_pMedfiltLifetimeMap)(m_pImgObjLifetimeMap->arr.raw_ptr());
			if (m_pConfig->flimSyncInterFrame < m_pImgObjIntensityMap->arr.size(1))
			{
				memcpy(&m_pImgObjLifetimeMap->arr(0, 0), &m_pImgObjLifetimeMap->arr(0, m_pConfig->flimSyncInterFrame),
					m_pImgObjLifetimeMap->arr.size(0) * (m_pImgObjLifetimeMap->arr.size(1) - m_pConfig->flimSyncInterFrame));
				memset(&m_pImgObjLifetimeMap->arr(0, m_pImgObjLifetimeMap->arr.size(1) - m_pConfig->flimSyncInterFrame), 0,
					m_pImgObjLifetimeMap->arr.size(0) * m_pConfig->flimSyncInterFrame);
			}

            for (int i = 0; i < roi_flimproj.height; i++)
            {
                uint8_t* pImg = m_pImgObjLifetimeMap->arr.raw_ptr() + i * roi_flimproj.width;
                std::rotate(pImg, pImg + adjust, pImg + roi_flimproj.width);
            }

			m_pImgObjLifetimeMap->convertRgb();

			///*************************************************************************************************************************************************************************///
			///Ipp32f mean, std;			
			///ippsMeanStdDev_32f(&m_intensityMap.at(m_pComboBox_EmissionChannel->currentIndex())(0, m_pSlider_SelectFrame->value()), m_pConfig->flimAlines, &mean, &std, ippAlgHintFast);
			///printf("inten: %f %f\n", mean, std);
			///ippsMeanStdDev_32f(&m_lifetimeMap.at(m_pComboBox_EmissionChannel->currentIndex())(0, m_pSlider_SelectFrame->value()), m_pConfig->flimAlines, &mean, &std, ippAlgHintFast);
			///printf("lifet: %f %f\n", mean, std);
			///*************************************************************************************************************************************************************************///

			if (m_pCheckBox_HsvEnhancedMap->isChecked())
			{
///				// HSV channel setting
///				ImageObject tempImgObj(m_pImgObjHsvEnhancedMap->getWidth(), m_pImgObjHsvEnhancedMap->getHeight(), m_pImgObjHsvEnhancedMap->getColorTable());

///				memset(tempImgObj.qrgbimg.bits(), 255, tempImgObj.qrgbimg.byteCount()); // Saturation is set to be 255.
///				tempImgObj.setRgbChannelData(m_pImgObjLifetimeMap->qindeximg.bits(), 0); // Hue
///				uint8_t *pIntensity = new uint8_t[m_pImgObjIntensityMap->qindeximg.byteCount()];
///				memcpy(pIntensity, m_pImgObjIntensityMap->qindeximg.bits(), m_pImgObjIntensityMap->qindeximg.byteCount());
///				ippsMulC_8u_ISfs(1.0, pIntensity, m_pImgObjIntensityMap->qindeximg.byteCount(), 0);
///				tempImgObj.setRgbChannelData(pIntensity, 2); // Value
///				delete[] pIntensity;

///				ippiHSVToRGB_8u_C3R(tempImgObj.qrgbimg.bits(), 3 * roi_flimproj.width, m_pImgObjHsvEnhancedMap->qrgbimg.bits(), 3 * roi_flimproj.width, roi_flimproj);

				// Non HSV intensity-weight map
				ColorTable temp_ctable;
				ImageObject tempImgObj(m_pImgObjHsvEnhancedMap->getWidth(), m_pImgObjHsvEnhancedMap->getHeight(), temp_ctable.m_colorTableVector.at(ColorTable::gray));

				m_pImgObjLifetimeMap->convertRgb();
				memcpy(tempImgObj.qindeximg.bits(), m_pImgObjIntensityMap->arr.raw_ptr(), tempImgObj.qindeximg.byteCount());
				tempImgObj.convertRgb();

				ippsMul_8u_Sfs(m_pImgObjLifetimeMap->qrgbimg.bits(), tempImgObj.qrgbimg.bits(), m_pImgObjHsvEnhancedMap->qrgbimg.bits(), tempImgObj.qrgbimg.byteCount(), 8);
			}
		}

		emit paintOctProjection(m_visOctProjection);
		emit paintIntensityMap(m_pImgObjIntensityMap->arr.raw_ptr());
		emit paintLifetimeMap((!m_pCheckBox_HsvEnhancedMap->isChecked()) ? m_pImgObjLifetimeMap->qrgbimg.bits() : m_pImgObjHsvEnhancedMap->qrgbimg.bits());
	}
}

void QVisualizationTab::visualizeImage(uint8_t* oct_im, float* intensity, float* lifetime)
{
	std::chrono::system_clock::time_point StartTime = std::chrono::system_clock::now();

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

	std::chrono::system_clock::time_point EndTime = std::chrono::system_clock::now();
	std::chrono::microseconds micro = std::chrono::duration_cast<std::chrono::microseconds>(EndTime - StartTime);
	//printf("[visualizae image] %d microsec\n", micro.count());

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
		if (m_pResultTab->getProcessingTab()->getPulseReviewDlg())
			m_pResultTab->getProcessingTab()->getPulseReviewDlg()->drawPulse(m_pResultTab->getProcessingTab()->getPulseReviewDlg()->getCurrentAline());

		// Status Update
		QString str; str.sprintf("Current Frame : %3d / %3d", frame + 1, (int)m_vectorOctImage.size());
		m_pLabel_SelectFrame->setText(str);
	}
}

void QVisualizationTab::constructRgbImage(ImageObject *rectObj, ImageObject *circObj, ImageObject *intObj, ImageObject *lftObj)
{
	std::chrono::system_clock::time_point StartTime = std::chrono::system_clock::now();

	// Convert RGB
	rectObj->convertRgb();
	intObj->convertScaledRgb();
	lftObj->convertScaledRgb();

	// Paste FLIM color ring to RGB rect image
	if ((m_pStreamTab != nullptr) || (!m_pCheckBox_HsvEnhancedMap->isChecked()))
	{
		memcpy(rectObj->qrgbimg.bits() + 3 * rectObj->arr.size(0) * (rectObj->arr.size(1) - 2 * RING_THICKNESS), intObj->qrgbimg.bits(), intObj->qrgbimg.byteCount());
		memcpy(rectObj->qrgbimg.bits() + 3 * rectObj->arr.size(0) * (rectObj->arr.size(1) - 1 * RING_THICKNESS), lftObj->qrgbimg.bits(), lftObj->qrgbimg.byteCount());
	}
	else // HSV Enhancing in ResultTab
	{
		ImageObject hsvObj(lftObj->getWidth(), 1, lftObj->getColorTable());
		memcpy(hsvObj.qrgbimg.bits(),
			m_pImgObjHsvEnhancedMap->qrgbimg.bits() + (m_pImgObjHsvEnhancedMap->qrgbimg.height() - m_pSlider_SelectFrame->value() - 1) * 3 * m_pImgObjHsvEnhancedMap->qrgbimg.width(),
			3 * m_pImgObjHsvEnhancedMap->qrgbimg.width());
		hsvObj.scaledRgb4();

		for (int i = 0; i < (int)(RING_THICKNESS * 1.5); i++)
			memcpy(rectObj->qrgbimg.bits() + 3 * rectObj->arr.size(0) * (rectObj->arr.size(1) - i - 1), hsvObj.qrgbimg.bits(), hsvObj.qrgbimg.byteCount());
	}

	// Rect View
	if (!m_pCheckBox_CircularizeImage->isChecked())
	{
		// Draw image
		if (m_pImageView_RectImage->isEnabled()) m_pImageView_RectImage->drawImage(rectObj->qrgbimg.bits());
	}
	// Circ View
	else
	{
		np::Uint8Array2 rect_temp(rectObj->qrgbimg.bits(), 3 * rectObj->arr.size(0), rectObj->arr.size(1));
		(*m_pCirc)(rect_temp, circObj->qrgbimg.bits(), "vertical", "rgb");

		// Draw image
		if (m_pImageView_CircImage->isEnabled()) m_pImageView_CircImage->drawImage(circObj->qrgbimg.bits());
	}

	std::chrono::system_clock::time_point EndTime = std::chrono::system_clock::now();
	std::chrono::microseconds micro = std::chrono::duration_cast<std::chrono::microseconds>(EndTime - StartTime);
	//printf("[construct rgb] %d microsec\n", micro.count());
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
			m_pLabel_IntensityMap->setText(QString("FLIm Ch%1 / Ch%2 Intensity Ratio Map").arg(m_pConfig->flimEmissionChannel)
				.arg(ratio_index[m_pConfig->flimEmissionChannel - 1][m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]]));

		m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1]
			[m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]].min, 'f', 1));
		m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1]
			[m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]].max, 'f', 1));
	}
	else
	{
		if (m_pResultTab)
			m_pLabel_IntensityMap->setText(QString("FLIm Ch%1 Intensity Map").arg(m_pConfig->flimEmissionChannel));

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
		m_pLabel_IntensityMap->setText(QString("FLIm Ch%1 / Ch%2 Intensity Ratio Map").arg(m_pConfig->flimEmissionChannel)
			.arg(ratio_index[m_pConfig->flimEmissionChannel - 1][m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]]));

	m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1]
		[m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]].min, 'f', 1));
	m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1]
		[m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]].max, 'f', 1));

	// Only result tab function
	visualizeEnFaceMap(true);
	visualizeImage(m_pSlider_SelectFrame->value());
}

void QVisualizationTab::enableHsvEnhancingMode(bool)
{
	// Only result tab function
	visualizeEnFaceMap(true);
	visualizeImage(m_pSlider_SelectFrame->value());
}

void QVisualizationTab::changeEmissionChannel(int ch)
{
    m_pConfig->flimEmissionChannel = ch + 1;

	if (m_pStreamTab)
	{
		m_pLabel_NormIntensity->setText(QString("Ch%1 Intensity ").arg(ch + 1));
		m_pLabel_Lifetime->setText(QString("Ch%1 Lifetime ").arg(ch + 1));

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

		m_pLabel_LifetimeMap->setText(QString("FLIm Ch%1 Lifetime Map").arg(ch + 1));
		if (!m_pCheckBox_IntensityRatio->isChecked())
		{
			m_pLabel_IntensityMap->setText(QString("FLIm Ch%1 Intensity Map").arg(ch + 1));
			m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRange[ch].min, 'f', 1));
			m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRange[ch].max, 'f', 1));
		}
		else
		{			
			m_pLabel_IntensityMap->setText(QString("FLIm Ch%1 / Ch%2 Intensity Ratio Map").arg(ch + 1)
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

void QVisualizationTab::changeLifetimeColorTable(int ctable_ind)
{
    m_pConfig->flimLifetimeColorTable = ctable_ind;

	if (m_pResultTab)
		m_pImageView_LifetimeMap->resetColormap(ColorTable::colortable(ctable_ind));
    m_pImageView_LifetimeColorbar->resetColormap(ColorTable::colortable(ctable_ind));

    ColorTable temp_ctable;
    if (m_pImgObjLifetime) delete m_pImgObjLifetime;
	m_pImgObjLifetime = new ImageObject(m_pConfig->flimAlines, RING_THICKNESS, temp_ctable.m_colorTableVector.at(ctable_ind));
	if (m_pResultTab)
	{
		if (m_pImgObjLifetimeMap) delete m_pImgObjLifetimeMap;
		m_pImgObjLifetimeMap = new ImageObject(m_pImageView_LifetimeMap->getRender()->m_pImage->width(), m_pImageView_LifetimeMap->getRender()->m_pImage->height(), temp_ctable.m_colorTableVector.at(ctable_ind));
		if (m_pImgObjHsvEnhancedMap) delete m_pImgObjHsvEnhancedMap;
		m_pImgObjHsvEnhancedMap = new ImageObject(m_pImageView_LifetimeMap->getRender()->m_pImage->width(), m_pImageView_LifetimeMap->getRender()->m_pImage->height(), temp_ctable.m_colorTableVector.at(ctable_ind));
	}

	if (m_pStreamTab)
		visualizeImage(m_visImage.raw_ptr(), m_visIntensity.raw_ptr(), m_visLifetime.raw_ptr());
	else if (m_pResultTab)
	{
		visualizeEnFaceMap(true);
		visualizeImage(m_pSlider_SelectFrame->value());
	}
}

void QVisualizationTab::setFlimInterFrameSync(const QString & str)
{
	if (m_pResultTab)
	{
		m_pConfig->flimSyncInterFrame = str.toInt();		
		visualizeEnFaceMap(true);
		visualizeImage(m_pSlider_SelectFrame->value());
	}
}

void QVisualizationTab::setFlimSyncAdjust(int adjust)
{
    if (m_pResultTab)
    {
        m_pLabel_FlimSyncAdjust->setText(QString(" Sync Adjust (%1)").arg(adjust, 3, 10));
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


void QVisualizationTab::changeVisImage(bool toggled)
{
    if (toggled)
    {
        m_pImageView_CircImage->show();
        m_pImageView_RectImage->hide();
    }
    else
    {
		if (m_pResultTab) m_pToggleButton_MeasureDistance->setChecked(false);
        m_pImageView_CircImage->hide();
        m_pImageView_RectImage->show(); 
    }

	if (m_pStreamTab)
		visualizeImage(m_visImage.raw_ptr(), m_visIntensity.raw_ptr(), m_visLifetime.raw_ptr());
	else if (m_pResultTab)
	{
		visualizeImage(m_pSlider_SelectFrame->value());
		m_pToggleButton_MeasureDistance->setEnabled(toggled);
	}
}

void QVisualizationTab::changeOctColorTable(int ctable_ind)
{
    m_pConfig->octColorTable = ctable_ind;

    m_pImageView_RectImage->resetColormap(ColorTable::colortable(ctable_ind));
    m_pImageView_CircImage->resetColormap(ColorTable::colortable(ctable_ind));
	if (m_pStreamTab)
		m_pStreamTab->getDeviceControlTab()->getOctDbColorbar()->resetColormap(ColorTable::colortable(ctable_ind));
	else if (m_pResultTab)
	{
		m_pImageView_OctProjection->resetColormap(ColorTable::colortable(ctable_ind));
		m_pImageView_OctGrayColorbar->resetColormap(ColorTable::colortable(ctable_ind));
	}

    ColorTable temp_ctable;
    if (m_pImgObjRectImage) delete m_pImgObjRectImage;
    m_pImgObjRectImage = new ImageObject(m_pImageView_RectImage->getRender()->m_pImage->width(), m_pImageView_RectImage->getRender()->m_pImage->height(), temp_ctable.m_colorTableVector.at(ctable_ind));
    if (m_pImgObjCircImage) delete m_pImgObjCircImage;
    m_pImgObjCircImage = new ImageObject(m_pImageView_CircImage->getRender()->m_pImage->width(), m_pImageView_CircImage->getRender()->m_pImage->height(), temp_ctable.m_colorTableVector.at(ctable_ind));

	if (m_pStreamTab)
	    visualizeImage(m_visImage.raw_ptr(), m_visIntensity.raw_ptr(), m_visLifetime.raw_ptr());
	else if (m_pResultTab)
	{
		visualizeEnFaceMap(true);
		visualizeImage(m_pSlider_SelectFrame->value());
	}
}

void QVisualizationTab::adjustOctGrayContrast()
{
	// Only result tab function
	m_pConfig->octGrayRange.min = m_pLineEdit_OctGrayMin->text().toFloat();
	m_pConfig->octGrayRange.max = m_pLineEdit_OctGrayMax->text().toFloat();

	visualizeEnFaceMap(true);
	visualizeImage(m_pSlider_SelectFrame->value());
}
