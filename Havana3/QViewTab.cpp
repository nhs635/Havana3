
#include "QViewTab.h"

#include <Havana3/MainWindow.h>
#include <Havana3/Configuration.h>
#include <Havana3/QStreamTab.h>
#include <Havana3/QResultTab.h>
#include <Havana3/Viewer/QImageView.h>

//#include <DataAcquisition/DataAcquisition.h>
//#include <DataAcquisition/FLImProcess/FLImProcess.h>

#include <ippcore.h>
#include <ippvm.h>
#include <ipps.h>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

#define ROUND_UP_4S(x) ((x + 3) >> 2) << 2


QViewTab::QViewTab(bool is_streaming, QWidget *parent) :
    QDialog(parent), m_pStreamTab(nullptr), m_pResultTab(nullptr),
    m_pImgObjRectImage(nullptr), m_pImgObjCircImage(nullptr), m_pImgObjIntensity(nullptr), m_pImgObjLifetime(nullptr),
	m_pImgObjOctProjection(nullptr), m_pImgObjIntensityMap(nullptr), m_pImgObjLifetimeMap(nullptr), m_pImgObjIntensityWeightedLifetimeMap(nullptr),
    m_pImgObjLongiImage(nullptr), m_pImgObjLongiIntensity(nullptr), m_pImgObjLongiLifetime(nullptr), m_pImgObjLongiIntensityWeightedLifetime(nullptr),
    m_pCirc(nullptr), m_pMedfiltRect(nullptr), m_pMedfiltIntensityMap(nullptr), m_pMedfiltLifetimeMap(nullptr), m_pMedfiltLongi(nullptr), m_pAnn(nullptr)
{
    // Set configuration objects
	if (is_streaming)
		m_pStreamTab = (QStreamTab*)parent;
	else	
		m_pResultTab = (QResultTab*)parent;
	m_pConfig = is_streaming? m_pStreamTab->getMainWnd()->m_pConfiguration : m_pResultTab->getMainWnd()->m_pConfiguration;

    // Create view tab widgets
    createViewTabWidgets(is_streaming);

     // Create visualization buffers
    if (is_streaming) setStreamingBuffersObjects();		
}

QViewTab::~QViewTab()
{
	if (m_pImgObjCircImage) delete m_pImgObjCircImage;
	if (m_pImgObjIntensity) delete m_pImgObjIntensity;
	if (m_pImgObjLifetime) delete m_pImgObjLifetime;	

	if (m_pImgObjOctProjection) delete m_pImgObjOctProjection;
	if (m_pImgObjIntensityMap) delete m_pImgObjIntensityMap;
	if (m_pImgObjLifetimeMap) delete m_pImgObjLifetimeMap;
	if (m_pImgObjIntensityWeightedLifetimeMap) delete m_pImgObjIntensityWeightedLifetimeMap;

    if (m_pImgObjLongiImage) delete m_pImgObjLongiImage;
    if (m_pImgObjLongiIntensity) delete m_pImgObjLongiIntensity;
    if (m_pImgObjLongiLifetime) delete m_pImgObjLongiLifetime;
    if (m_pImgObjLongiIntensityWeightedLifetime) delete m_pImgObjLongiIntensityWeightedLifetime;

	if (m_pCirc) delete m_pCirc;
	if (m_pMedfiltRect) delete m_pMedfiltRect;
	if (m_pMedfiltIntensityMap) delete m_pMedfiltIntensityMap;
	if (m_pMedfiltLifetimeMap) delete m_pMedfiltLifetimeMap;
    if (m_pMedfiltLongi) delete m_pMedfiltLongi;
    if (m_pAnn) delete m_pAnn;
}


//void QViewTab::setWidgetsValue()
//{
//	m_pComboBox_EmissionChannel->setCurrentIndex(m_pConfig->flimEmissionChannel - 1);
	
//	if (m_pStreamTab)
//	{
//		m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
//		m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
//	}
//	else if (m_pResultTab)
//	{
//		if (!m_pCheckBox_IntensityRatio->isChecked())
//		{
//			m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
//			m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
//		}
//		else
//		{
//			m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1]
//				[m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]].max, 'f', 1));
//			m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1]
//				[m_pConfig->flimIntensityRatioRefIdx[m_pConfig->flimEmissionChannel - 1]].min, 'f', 1));
//		}
//	}
//	m_pLineEdit_LifetimeMax->setText(QString::number(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
    //	m_pLineEdit_LifetimeMin->setText(QString::number(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
//}

void QViewTab::createViewTabWidgets(bool is_streaming)
{
    // Create image view : cross-section
    m_pImageView_CircImage = new QImageView(ColorTable::colortable(OCT_COLORTABLE), 2 * m_pConfig->octScans, 2 * m_pConfig->octScans, true);    
    if (is_streaming)
        m_pImageView_CircImage->setMinimumSize(810, 810);
    else
        m_pImageView_CircImage->setMinimumSize(650, 650);    
	m_pImageView_CircImage->setSquare(true);
	m_pImageView_CircImage->setCircle(1, OUTER_SHEATH_POSITION);

    if (!is_streaming)
    {
        // Create image view : en-face
        ColorTable temp_ctable;
        m_pImageView_EnFace = new QImageView(ColorTable::colortable(LIFETIME_COLORTABLE), 1, m_pConfig->flimAlines, true);
        m_pImageView_EnFace->setMinimumSize(500, 250);

        m_pLabel_EnFace = new QLabel(this);
		m_pLabel_EnFace->setText(QString::fromLocal8Bit("FLIm Ch%1 Lifetime Map (z-еш) (nsec)").arg(m_pConfig->flimEmissionChannel));
        
        // Create image view : longitudinal
        m_pImageView_Longi = new QImageView(ColorTable::colortable(OCT_COLORTABLE), 1, 2 * m_pConfig->octScans, true);
        m_pImageView_Longi->setMinimumSize(500, 350);		

		m_pLabel_Longi = new QLabel(this);
		m_pLabel_Longi->setText("Longitudinal Cross-sections");

        // Create image view : colorbar
        uint8_t color[256 * 4];
        for (int i = 0; i < 256 * 4; i++)
            color[i] = 255 - i / 4;

        m_pImageView_ColorBar = new QImageView(ColorTable::colortable(LIFETIME_COLORTABLE), 4, 256, false);
        m_pImageView_ColorBar->getRender()->setFixedWidth(30);
        m_pImageView_ColorBar->drawImage(color);
        m_pImageView_ColorBar->setFixedWidth(20);

		m_pLabel_ColorBar = new QLabel(this);
		m_pLabel_ColorBar->setText("Lifetime (nsec)");

        // Create push buttons for exploring frames
        m_pToggleButton_Play = new QPushButton(this);
        m_pToggleButton_Play->setCheckable(true);
        m_pToggleButton_Play->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        m_pToggleButton_Play->setFixedSize(40, 25);

        m_pPushButton_Increment = new QPushButton(this);
        m_pPushButton_Increment->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));
        m_pPushButton_Increment->setFixedSize(40, 25);

        m_pPushButton_Decrement = new QPushButton(this);
        m_pPushButton_Decrement->setIcon(style()->standardIcon(QStyle::SP_MediaSeekBackward));
        m_pPushButton_Decrement->setFixedSize(40, 25);

        // Create slider for exploring frames
        m_pSlider_SelectFrame = new QSlider(this);
        m_pSlider_SelectFrame->setTickPosition(QSlider::TicksBothSides);
        m_pSlider_SelectFrame->setTickInterval(20);		
        m_pSlider_SelectFrame->setOrientation(Qt::Horizontal);
        QPalette pal = m_pSlider_SelectFrame->palette();
        pal.setColor(QPalette::Highlight, QColor(42, 42, 42));
		//pal.setColor(QPalette::Window, QColor(255, 255, 255));
        m_pSlider_SelectFrame->setPalette(pal);
        m_pSlider_SelectFrame->setValue(0);		

        m_pLabel_SelectFrame = new QLabel(this);
        QString str; str.sprintf("Frame : %3d / %3d", 1, 1);
        m_pLabel_SelectFrame->setAlignment(Qt::AlignBottom | Qt::AlignRight);
        m_pLabel_SelectFrame->setText(str);
        m_pLabel_SelectFrame->setFixedSize(m_pImageView_CircImage->width() - 10, m_pImageView_CircImage->height() - 10);
        m_pLabel_SelectFrame->setStyleSheet("QLabel { font-weight: bold; }");
        m_pLabel_SelectFrame->setBuddy(m_pSlider_SelectFrame);

        // Create push button for measuring distance
        m_pToggleButton_MeasureDistance = new QPushButton(this);
        m_pToggleButton_MeasureDistance->setCheckable(true);
        m_pToggleButton_MeasureDistance->setText("Measure Distance");
        m_pToggleButton_MeasureDistance->setFixedWidth(120);

        // Create widgets for FLIm emission control
        m_pComboBox_ViewMode = new QComboBox(this);
        m_pComboBox_ViewMode->addItem("Ch 1");
        m_pComboBox_ViewMode->addItem("Ch 2");
        m_pComboBox_ViewMode->addItem("Ch 3");
        m_pComboBox_ViewMode->setCurrentIndex(m_pConfig->flimEmissionChannel - 1);
        m_pComboBox_ViewMode->setFixedWidth(60);

        m_pLabel_ViewMode = new QLabel("View Mode ", this);
        m_pLabel_ViewMode->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
        m_pLabel_ViewMode->setBuddy(m_pComboBox_ViewMode);
    }

    // Set layout
    QGridLayout *pGridLayout = new QGridLayout;
    pGridLayout->setSpacing(4);

    pGridLayout->addWidget(m_pImageView_CircImage, 0, 0, 3, 1);

    if (!is_streaming)
    {
        //pGridLayout->addWidget(m_pLabel_SelectFrame, 0, 0, 3, 1);

        pGridLayout->addWidget(m_pImageView_EnFace, 0, 1);
        pGridLayout->addWidget(m_pSlider_SelectFrame, 1, 1);
        pGridLayout->addWidget(m_pImageView_Longi, 2, 1);

        pGridLayout->addWidget(m_pImageView_ColorBar, 0, 2);

        QHBoxLayout *pHBoxLayout1 = new QHBoxLayout;
        pHBoxLayout1->setSpacing(4);

        pHBoxLayout1->addWidget(m_pPushButton_Decrement);
        pHBoxLayout1->addWidget(m_pToggleButton_Play);
        pHBoxLayout1->addWidget(m_pPushButton_Increment);
        pHBoxLayout1->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
        pHBoxLayout1->addWidget(m_pToggleButton_MeasureDistance);

        QHBoxLayout *pHBoxLayout2 = new QHBoxLayout;
        pHBoxLayout2->setSpacing(4);

        pHBoxLayout2->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
        pHBoxLayout2->addWidget(m_pLabel_ViewMode);
        pHBoxLayout2->addWidget(m_pComboBox_ViewMode);

        pGridLayout->addItem(pHBoxLayout1, 3, 0);
        pGridLayout->addItem(pHBoxLayout2, 3, 1);
    }

    m_pViewWidget = new QWidget(this);
    m_pViewWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_pViewWidget->setLayout(pGridLayout);


    // Connect signal and slot
	connect(this, SIGNAL(drawImage(uint8_t*, float*, float*)), this, SLOT(visualizeImage(uint8_t*, float*, float*)));  // streaming
	connect(this, SIGNAL(makeRgb(ImageObject*, ImageObject*, ImageObject*, ImageObject*)),
		this, SLOT(constructRgbImage(ImageObject*, ImageObject*, ImageObject*, ImageObject*)));
	connect(this, SIGNAL(paintCircImage(uint8_t*)), m_pImageView_CircImage, SLOT(drawRgbImage(uint8_t*)));

    if (!is_streaming)
    {
		connect(m_pToggleButton_Play, SIGNAL(toggled(bool)), this, SLOT(play(bool)));
        connect(m_pPushButton_Increment, &QPushButton::clicked, [&]() { m_pSlider_SelectFrame->setValue(m_pSlider_SelectFrame->value() + 1); });
        connect(m_pPushButton_Decrement, &QPushButton::clicked, [&]() { m_pSlider_SelectFrame->setValue(m_pSlider_SelectFrame->value() - 1); });

        connect(m_pSlider_SelectFrame, SIGNAL(valueChanged(int)), this, SLOT(visualizeImage(int)));
        connect(m_pToggleButton_MeasureDistance, SIGNAL(toggled(bool)), this, SLOT(measureDistance(bool)));

        connect(m_pComboBox_ViewMode, SIGNAL(currentIndexChanged(int)), this, SLOT(changeEmissionChannel(int)));

        connect(this, SIGNAL(paintEnFaceMap(uint8_t*)), m_pImageView_EnFace, SLOT(drawRgbImage(uint8_t*)));
        connect(this, SIGNAL(paintLongiImage(uint8_t*)), m_pImageView_Longi, SLOT(drawRgbImage(uint8_t*)));
    }
}


void QViewTab::setStreamingBuffersObjects()
{
    // Create visualization buffers
    m_visImage = np::Uint8Array2(m_pConfig->octScans, m_pConfig->octAlines);
    m_visIntensity = np::FloatArray2(m_pConfig->flimAlines, 4); memset(m_visIntensity.raw_ptr(), 0, sizeof(float) * m_visIntensity.length());
    m_visMeanDelay = np::FloatArray2(m_pConfig->flimAlines, 4); memset(m_visMeanDelay.raw_ptr(), 0, sizeof(float) * m_visMeanDelay.length());
    m_visLifetime = np::FloatArray2(m_pConfig->flimAlines, 3); memset(m_visLifetime.raw_ptr(), 0, sizeof(float) * m_visLifetime.length());

    // Create image visualization buffers
    ColorTable temp_ctable;
	if (m_pImgObjCircImage) delete m_pImgObjCircImage;
    m_pImgObjCircImage = new ImageObject(2 * m_pConfig->octScans, 2 * m_pConfig->octScans, temp_ctable.m_colorTableVector.at(temp_ctable.gray));
	if (m_pImgObjIntensity) delete m_pImgObjIntensity;
    m_pImgObjIntensity = new ImageObject(m_pConfig->flimAlines, RING_THICKNESS, temp_ctable.m_colorTableVector.at(INTENSITY_COLORTABLE));
	if (m_pImgObjLifetime) delete m_pImgObjLifetime;
    m_pImgObjLifetime = new ImageObject(m_pConfig->flimAlines, RING_THICKNESS, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE));

	if (m_pCirc) delete m_pCirc;
    m_pCirc = new circularize(m_pConfig->octScans, m_pConfig->octAlines, false);
	if (m_pMedfiltRect) delete m_pMedfiltRect;
	m_pMedfiltRect = new medfilt(m_pConfig->octAlines, m_pConfig->octScans, 3, 3);
}

void QViewTab::setBuffers(Configuration* pConfig)
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
		memset(buffer, 0, sizeof(uint8_t) * pConfig->octAlines * pConfig->octScans);
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
}

void QViewTab::setObjects(Configuration * pConfig)
{
	// Non-4's multiple width
	int frames4 = ROUND_UP_4S(pConfig->frames);

	// Visualization buffers
	ColorTable temp_ctable;

	if (m_pImgObjRectImage) delete m_pImgObjRectImage;
	m_pImgObjRectImage = new ImageObject(pConfig->octAlines, pConfig->octScans, temp_ctable.m_colorTableVector.at(OCT_COLORTABLE));
	if (m_pImgObjCircImage) delete m_pImgObjCircImage;
	m_pImgObjCircImage = new ImageObject(2 * m_pConfig->octScans, 2 * m_pConfig->octScans, temp_ctable.m_colorTableVector.at(OCT_COLORTABLE));
	if (m_pImgObjIntensity) delete m_pImgObjIntensity;
	m_pImgObjIntensity = new ImageObject(pConfig->flimAlines, RING_THICKNESS, temp_ctable.m_colorTableVector.at(INTENSITY_COLORTABLE));
	if (m_pImgObjLifetime) delete m_pImgObjLifetime;
	m_pImgObjLifetime = new ImageObject(RING_THICKNESS, pConfig->flimAlines, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE));

	// En face map visualization buffers
	if (m_pImgObjOctProjection) delete m_pImgObjOctProjection;
	m_pImgObjOctProjection = new ImageObject(frames4, pConfig->octAlines, temp_ctable.m_colorTableVector.at(OCT_COLORTABLE));
	if (m_pImgObjIntensityMap) delete m_pImgObjIntensityMap;
	m_pImgObjIntensityMap = new ImageObject(frames4, pConfig->flimAlines, temp_ctable.m_colorTableVector.at(INTENSITY_COLORTABLE));
	if (m_pImgObjLifetimeMap) delete m_pImgObjLifetimeMap;
	m_pImgObjLifetimeMap = new ImageObject(frames4, pConfig->flimAlines, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE));
	if (m_pImgObjIntensityWeightedLifetimeMap) delete m_pImgObjIntensityWeightedLifetimeMap;
	m_pImgObjIntensityWeightedLifetimeMap = new ImageObject(frames4, pConfig->flimAlines, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE));

	// Longitudinal image visualization buffers
	if (m_pImgObjLongiImage) delete m_pImgObjLongiImage;
	m_pImgObjLongiImage = new ImageObject(frames4, 2 * pConfig->octScans, temp_ctable.m_colorTableVector.at(OCT_COLORTABLE));
	if (m_pImgObjLongiIntensity) delete m_pImgObjLongiIntensity;
	m_pImgObjLongiIntensity = new ImageObject(frames4, 2 * RING_THICKNESS, temp_ctable.m_colorTableVector.at(INTENSITY_COLORTABLE));
	if (m_pImgObjLongiLifetime) delete m_pImgObjLongiLifetime;
	m_pImgObjLongiLifetime = new ImageObject(frames4, 2 * RING_THICKNESS, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE));
	if (m_pImgObjLongiIntensityWeightedLifetime) delete m_pImgObjLongiIntensityWeightedLifetime;
	m_pImgObjLongiIntensityWeightedLifetime = new ImageObject(frames4, 2 * RING_THICKNESS, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE));
	
	// Circ & Medfilt objects
	if (m_pCirc) delete m_pCirc;
	m_pCirc = new circularize(pConfig->octAlines, pConfig->octScans, false);

	if (m_pMedfiltRect) delete m_pMedfiltRect;
	m_pMedfiltRect = new medfilt(pConfig->octAlines, pConfig->octScans, 3, 3);
	if (m_pMedfiltIntensityMap) delete m_pMedfiltIntensityMap;
	m_pMedfiltIntensityMap = new medfilt(frames4, pConfig->flimAlines, 3, 5);
	if (m_pMedfiltLifetimeMap) delete m_pMedfiltLifetimeMap;
	m_pMedfiltLifetimeMap = new medfilt(frames4, pConfig->flimAlines, 5, 7); 
	if (m_pMedfiltLongi) delete m_pMedfiltLongi;
	m_pMedfiltLongi = new medfilt(frames4, 2 * pConfig->octScans, 3, 3); 
}

void QViewTab::setWidgets(Configuration* pConfig)
{
	// Non-4's multiple width
	int frames4 = (pConfig->frames);

	// Set slider and select frame
	QString str; str.sprintf("Frame : %3d / %3d", 1, pConfig->frames);
	m_pLabel_SelectFrame->setText(str);

	m_pSlider_SelectFrame->setRange(0, pConfig->frames - 1);
	m_pSlider_SelectFrame->setValue(0);
	m_pSlider_SelectFrame->setTickInterval(int(pConfig->frames / 5));

	// ImageView objects
	m_pImageView_CircImage->resetSize(2 * pConfig->octScans, 2 * pConfig->octScans);
	m_pImageView_CircImage->setRLineChangeCallback([&](int aline) { visualizeLongiImage(aline); });
	m_pImageView_CircImage->setVerticalLine(1, 0);
	m_pImageView_CircImage->getRender()->m_bRadial = true;
	m_pImageView_CircImage->getRender()->m_bDiametric = true;
	m_pImageView_CircImage->getRender()->m_rMax = pConfig->octAlines;
	m_pImageView_CircImage->setWheelMouseCallback([&]() { m_pToggleButton_MeasureDistance->setChecked(false); });
	m_pImageView_CircImage->getRender()->m_bCanBeMagnified = true;
	m_pImageView_CircImage->setScaleBar(int(2.0 * OUTER_SHEATH_POSITION / 1.1));

	m_pImageView_EnFace->resetSize(frames4, pConfig->flimAlines);
	m_pImageView_EnFace->setVLineChangeCallback([&](int frame) { m_pSlider_SelectFrame->setValue(frame); });
	m_pImageView_EnFace->setVerticalLine(1, 0);
	m_pImageView_EnFace->setHLineChangeCallback([&](int aline) { visualizeLongiImage(4 * aline); });
	m_pImageView_EnFace->setHorizontalLine(1, 0);
	m_pImageView_EnFace->setEnterCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 18), QString::fromLocal8Bit("2-D FLIm En Face Mapping (z-еш)")); });
	m_pImageView_EnFace->setLeaveCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 18), ""); });

	m_pImageView_Longi->resetSize(frames4, 2 * pConfig->octScans);
	m_pImageView_Longi->setVLineChangeCallback([&](int frame) { m_pSlider_SelectFrame->setValue(frame); });
	m_pImageView_Longi->setVerticalLine(1, 0);
	m_pImageView_Longi->setHorizontalLine(2, pConfig->octScans + OUTER_SHEATH_POSITION, pConfig->octScans - OUTER_SHEATH_POSITION);
	m_pImageView_Longi->setEnterCallback([&]() { m_pImageView_Longi->setText(QPoint(8, 18), "Longitudinal View (z-r)"); });
	m_pImageView_Longi->setLeaveCallback([&]() { m_pImageView_Longi->setText(QPoint(8, 18), ""); });

	m_pImageView_ColorBar->setText(QPoint(0, 0), QString("%1               Lifetime (nsec)               %2")
		.arg(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max, 2, 'f', 1)
		.arg(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min, 2, 'f', 1), true);
}


void QViewTab::visualizeEnFaceMap(bool scaling)
{
	if (m_vectorOctImage.size() != 0)
	{
		if (scaling)
		{
			// Scaling OCT projection
			{
				IppiSize roi_proj = { m_octProjection.size(0), m_octProjection.size(1) };
				IppiSize roi_proj4 = { roi_proj.width, ROUND_UP_4S(roi_proj.height) };

				np::FloatArray2 scale_temp32(roi_proj.width, roi_proj.height);
				np::Uint8Array2 scale_temp8(roi_proj4.width, roi_proj4.height);

				ippsConvert_8u32f(m_octProjection.raw_ptr(), scale_temp32.raw_ptr(), scale_temp32.length());
				ippiScale_32f8u_C1R(scale_temp32.raw_ptr(), sizeof(float) * roi_proj.width,
					scale_temp8.raw_ptr(), sizeof(uint8_t) * roi_proj4.width, roi_proj,
					m_pConfig->octGrayRange.min, m_pConfig->octGrayRange.max);
				ippiTranspose_8u_C1R(scale_temp8.raw_ptr(), sizeof(uint8_t) * roi_proj4.width,
					m_pImgObjOctProjection->arr.raw_ptr(), sizeof(uint8_t) * roi_proj4.height, roi_proj4);
				circShift(m_pImgObjOctProjection->arr, 0 /* shift */);
			}

			// Scaling FLIM map
			{
				IppiSize roi_flimproj = { m_intensityMap.at(0).size(0), m_intensityMap.at(0).size(1) };
				IppiSize roi_flimproj4 = { roi_flimproj.width, ((roi_flimproj.height + 3) >> 2) << 2 };

				np::Uint8Array2 scale_temp(roi_flimproj4.width, roi_flimproj4.height);

				// Intensity map			
				ippiScale_32f8u_C1R(m_intensityMap.at(m_pComboBox_ViewMode->currentIndex()), sizeof(float) * roi_flimproj.width,
					scale_temp.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.width, roi_flimproj,
					m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min,
					m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max);
				ippiTranspose_8u_C1R(scale_temp.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.width,
					m_pImgObjIntensityMap->arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.height, roi_flimproj4);
				circShift(m_pImgObjIntensityMap->arr, 0 /* shift */);
				(*m_pMedfiltIntensityMap)(m_pImgObjIntensityMap->arr.raw_ptr());

				// Lifetime map
				ippiScale_32f8u_C1R(m_lifetimeMap.at(m_pComboBox_ViewMode->currentIndex()), sizeof(float) * roi_flimproj.width,
					scale_temp.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.width, roi_flimproj,
					m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min,
					m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max);
				ippiTranspose_8u_C1R(scale_temp.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.width,
					m_pImgObjLifetimeMap->arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.height, roi_flimproj);
				circShift(m_pImgObjLifetimeMap->arr, 0 /* shift */);
				(*m_pMedfiltLifetimeMap)(m_pImgObjLifetimeMap->arr.raw_ptr());

				// RGB conversion
				m_pImgObjLifetimeMap->convertRgb();
			}

			// Intensity-weight lifetime map
			{
				ColorTable temp_ctable;
				ImageObject tempImgObj(m_pImgObjIntensityWeightedLifetimeMap->getWidth(), m_pImgObjIntensityWeightedLifetimeMap->getHeight(), temp_ctable.m_colorTableVector.at(ColorTable::gray));

				m_pImgObjLifetimeMap->convertRgb();
				memcpy(tempImgObj.qindeximg.bits(), m_pImgObjIntensityMap->arr.raw_ptr(), tempImgObj.qindeximg.byteCount());
				tempImgObj.convertRgb();

				ippsMul_8u_Sfs(m_pImgObjLifetimeMap->qrgbimg.bits(), tempImgObj.qrgbimg.bits(), m_pImgObjIntensityWeightedLifetimeMap->qrgbimg.bits(), tempImgObj.qrgbimg.byteCount(), 8);
			}
		}

		emit paintEnFaceMap(m_pImgObjIntensityWeightedLifetimeMap->qrgbimg.bits());
	}
}

void QViewTab::visualizeImage(uint8_t* oct_im, float* intensity, float* lifetime)
{
//	IppiSize roi_oct = { m_pConfig->octScans, m_pConfig->octAlines };

//	// OCT Visualization
//	ippiTranspose_8u_C1R(oct_im, roi_oct.width * sizeof(uint8_t), m_pImgObjRectImage->arr.raw_ptr(), roi_oct.height * sizeof(uint8_t), roi_oct);
//	(*m_pMedfilt)(m_pImgObjRectImage->arr.raw_ptr());
	
//	// FLIm Visualization
//	IppiSize roi_flim = { m_pConfig->flimAlines, 1 };

//	float* scanIntensity = intensity + (1 + m_pComboBox_EmissionChannel->currentIndex()) * roi_flim.width;
//	uint8_t* rectIntensity = m_pImgObjIntensity->arr.raw_ptr();
//	ippiScale_32f8u_C1R(scanIntensity, roi_flim.width * sizeof(float), rectIntensity, roi_flim.width * sizeof(uint8_t), roi_flim,
//		m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min, m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max);

//	float* scanLifetime = lifetime + (m_pComboBox_EmissionChannel->currentIndex()) * roi_flim.width;
//	uint8_t* rectLifetime = m_pImgObjLifetime->arr.raw_ptr();
//	ippiScale_32f8u_C1R(scanLifetime, roi_flim.width * sizeof(float), rectLifetime, roi_flim.width * sizeof(uint8_t), roi_flim,
//		m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min, m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max);

//	for (int i = 1; i < RING_THICKNESS; i++)
//	{
//		memcpy(&m_pImgObjIntensity->arr(0, i), rectIntensity, sizeof(uint8_t) * roi_flim.width);
//		memcpy(&m_pImgObjLifetime->arr(0, i), rectLifetime, sizeof(uint8_t) * roi_flim.width);
//	}

//	// Convert RGB
//	m_pImgObjRectImage->convertRgb();
//	m_pImgObjIntensity->convertScaledRgb();
//	m_pImgObjLifetime->convertScaledRgb();

//	emit makeRgb(m_pImgObjRectImage, m_pImgObjCircImage, m_pImgObjIntensity, m_pImgObjLifetime);
}

void QViewTab::visualizeImage(int frame)
{
    if (m_vectorOctImage.size() != 0)
    {
        // OCT Visualization
		IppiSize roi_oct = { m_pImgObjRectImage->getWidth(), m_pImgObjRectImage->getHeight() };

        np::FloatArray2 scale_temp(roi_oct.width, roi_oct.height);
        ippsConvert_8u32f(m_vectorOctImage.at(frame).raw_ptr(), scale_temp.raw_ptr(), scale_temp.length());
        ippiScale_32f8u_C1R(scale_temp.raw_ptr(), roi_oct.width * sizeof(float),
            m_pImgObjRectImage->arr.raw_ptr(), roi_oct.width * sizeof(uint8_t), roi_oct, (float)m_pConfig->octGrayRange.min, (float)m_pConfig->octGrayRange.max);
		circShift(m_pImgObjRectImage->arr, 0 /* shift */);
        (*m_pMedfiltRect)(m_pImgObjRectImage->arr.raw_ptr());
		
		// Convert RGB
		m_pImgObjRectImage->convertRgb();

        // FLIM Visualization
		IppiSize roi_flimring = { 1, m_pImgObjIntensityMap->arr.size(1) };
		
		ImageObject temp_imgobj(roi_flimring.height, 1, m_pImgObjLifetimeMap->getColorTable());
		ippiCopy_8u_C3R(m_pImgObjIntensityWeightedLifetimeMap->qrgbimg.constBits() + 3 * frame, 3 * m_pImgObjIntensityWeightedLifetimeMap->arr.size(0),
			temp_imgobj.qrgbimg.bits(), 3, roi_flimring);
		temp_imgobj.scaledRgb4();

		tbb::parallel_for(tbb::blocked_range<size_t>(1, (size_t)RING_THICKNESS),
			[&](const tbb::blocked_range<size_t>& r) {
			for (size_t i = r.begin(); i != r.end(); ++i)
			{
				ippiCopy_8u_C3R(temp_imgobj.qrgbimg.constBits(), 3 * roi_flimring.width,
					m_pImgObjRectImage->qrgbimg.bits() + 3 * (m_pImgObjRectImage->arr.size(0) - RING_THICKNESS + i), 3 * m_pImgObjRectImage->arr.size(0), { 1, m_pImgObjRectImage->arr.size(1) });
			}
		});
				
        // Make circularzing
        emit makeRgb(m_pImgObjRectImage, m_pImgObjCircImage, m_pImgObjIntensity, m_pImgObjLifetime);

        // En Face Lines
        m_pImageView_EnFace->setVerticalLine(1, frame);
        m_pImageView_EnFace->getRender()->update();
				
        // Longitudinval Lines
        m_pImageView_Longi->setVerticalLine(1, frame);
		m_pImageView_Longi->getRender()->update();
		
        // Status Update
		QString str; str.sprintf("Frame : %3d / %3d", frame + 1, (int)m_vectorOctImage.size());
		m_pImageView_CircImage->setText(QPoint(15, 635), str);
    }
}

void QViewTab::visualizeLongiImage(int aline)
{
	// Pre-determined values
	int frames = m_vectorOctImage.size();
	int frames4 = ((frames + 3) >> 2) << 2;
	int octScans = m_pImageView_Longi->getHeight() / 2;
    int octAlines = m_octProjection.size(0);
	int flimAlines = octAlines / 4;

    // Specified A line
    int aline0 = aline; // + m_pViewTab->getCurrentRotation();
    int aline1 = (aline0 + octAlines / 2) % octAlines;

    // Make longitudinal - OCT
	IppiSize roi_longi = { m_pImgObjLongiImage->getHeight(), m_pImgObjLongiImage->getWidth() };

	np::FloatArray2 longi_temp(roi_longi.width, roi_longi.height);
	np::Uint8Array2 scale_temp(roi_longi.width, roi_longi.height);
	tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)frames),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
		{
			memcpy(&scale_temp(0, (int)i), &m_vectorOctImage.at((int)i)(0, aline0), sizeof(uint8_t) * octScans);
			memcpy(&scale_temp(octScans, (int)i), &m_vectorOctImage.at((int)i)(0, aline1), sizeof(uint8_t) * octScans);
			ippsFlip_8u_I(&scale_temp(0, (int)i), octScans);
		}
	});

    ippsConvert_8u32f(scale_temp.raw_ptr(), longi_temp.raw_ptr(), scale_temp.length());
    ippiScale_32f8u_C1R(longi_temp.raw_ptr(), roi_longi.width * sizeof(float),
        scale_temp.raw_ptr(), roi_longi.width * sizeof(uint8_t), roi_longi, m_pConfig->octGrayRange.min, m_pConfig->octGrayRange.max);
    //if (aline0 > (octAlines / 2))
    //    ippiMirror_8u_C1IR(scale_temp.raw_ptr(), sizeof(uint8_t) * roi_longi.width, roi_longi, ippAxsVertical);
    ippiTranspose_8u_C1R(scale_temp.raw_ptr(), roi_longi.width * sizeof(uint8_t), m_pImgObjLongiImage->arr.raw_ptr(), roi_longi.height * sizeof(uint8_t), roi_longi);
    (*m_pMedfiltLongi)(m_pImgObjLongiImage->arr.raw_ptr());

    m_pImgObjLongiImage->convertRgb();

    // Make longitudinal - FLIM
    tbb::parallel_for(tbb::blocked_range<size_t>(1, (size_t)RING_THICKNESS),
        [&](const tbb::blocked_range<size_t>& r) {
        for (size_t i = r.begin(); i != r.end(); ++i)
        {
			memcpy(m_pImgObjLongiImage->qrgbimg.bits() + 3 * (int)i * m_pImgObjLongiImage->getWidth(),
				m_pImgObjIntensityWeightedLifetimeMap->qrgbimg.bits() + 3 * int(aline0 / 4) * m_pImgObjIntensityWeightedLifetimeMap->getWidth(), sizeof(uint8_t) * 3 * m_pImgObjLongiImage->getWidth());
			memcpy(m_pImgObjLongiImage->qrgbimg.bits() + 3 * (int)(m_pImgObjLongiImage->getHeight() - i) * m_pImgObjLongiImage->getWidth(),
				m_pImgObjIntensityWeightedLifetimeMap->qrgbimg.bits() + 3 * int(aline1 / 4) * m_pImgObjIntensityWeightedLifetimeMap->getWidth(), sizeof(uint8_t) * 3 * m_pImgObjLongiImage->getWidth());
        }
    });

	// Draw longitudinal view
    emit paintLongiImage(m_pImgObjLongiImage->qrgbimg.bits());

	// Circular view lines
    m_pImageView_CircImage->setVerticalLine(1, aline);
    m_pImageView_CircImage->getRender()->update();

	// En Face lines
	m_pImageView_EnFace->setHorizontalLine(1, int(aline / 4));
	m_pImageView_EnFace->getRender()->update();
}


void QViewTab::constructRgbImage(ImageObject *rectObj, ImageObject *circObj, ImageObject *intObj, ImageObject *lftObj)
{
    // Circularizing
    np::Uint8Array2 rect_temp(rectObj->qrgbimg.bits(), 3 * rectObj->arr.size(0), rectObj->arr.size(1));
    (*m_pCirc)(rect_temp, circObj->qrgbimg.bits(), false, true); // aline-frame domain, rgb coordinate

    // Draw image
    emit paintCircImage(circObj->qrgbimg.bits());
} 


void QViewTab::play(bool enabled)
{
	static bool is_stop = false;

	if (enabled)
	{
		m_pToggleButton_Play->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
		
		int cur_frame = m_pSlider_SelectFrame->value();
		int end_frame = m_vectorOctImage.size();

		std::thread playing([&, cur_frame, end_frame]() {
			for (int i = cur_frame; i < end_frame; i++)
			{
				m_pSlider_SelectFrame->setValue(i);
				std::this_thread::sleep_for(std::chrono::milliseconds(150));

				if (is_stop) break;
			}
		});
		playing.detach();
	}
	else
	{
		is_stop = true;
		m_pToggleButton_Play->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
	}
}

void QViewTab::measureDistance(bool toggled)
{
	m_pImageView_CircImage->getRender()->m_bMeasureDistance = toggled;
	if (!toggled)
	{
		m_pImageView_CircImage->setRLineChangeCallback([&](int aline) { visualizeLongiImage(aline); });
		m_pImageView_CircImage->getRender()->m_nClicked = 0;
		m_pImageView_CircImage->getRender()->update();
	}
	else
	{
		m_pImageView_CircImage->setRLineChangeCallback([&](int aline) {});		
	}
}

void QViewTab::changeEmissionChannel(int index)
{
	m_pConfig->flimEmissionChannel = index + 1;

	visualizeEnFaceMap(true);
	visualizeImage(getCurrentFrame());
	visualizeLongiImage(getCurrentAline());
}

void QViewTab::circShift(np::Uint8Array2& image, int shift)
{
	if (shift > 0)
	{
		np::Uint8Array2 temp(image.size(0), shift);

		ippiCopy_8u_C1R(&image(0, 0), image.size(0), &temp(0, 0), image.size(0), { image.size(0), shift });
		ippiCopy_8u_C1R(&image(0, shift), image.size(0), &image(0, 0), image.size(0), { image.size(0), image.size(1) - shift });
		ippiCopy_8u_C1R(&temp(0, 0), image.size(0), &image(0, image.size(1) - shift), image.size(0), { image.size(0), shift });
	}
}
