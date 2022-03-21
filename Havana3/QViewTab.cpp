
#include "QViewTab.h"

#include <Havana3/MainWindow.h>
#include <Havana3/QStreamTab.h>
#include <Havana3/QResultTab.h>
#include <Havana3/Viewer/QImageView.h>
#include <Havana3/Dialog/SettingDlg.h>
#include <Havana3/Dialog/ViewOptionTab.h>
#include <Havana3/Dialog/PulseReviewTab.h>
#include <Havana3/Dialog/IvusViewerDlg.h>

#include <DataAcquisition/DataProcessing.h>

#include <ipps.h>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>


QViewTab::QViewTab(bool is_streaming, QWidget *parent) :
	QDialog(parent), m_pStreamTab(nullptr), m_pResultTab(nullptr), m_pConfigTemp(nullptr),
	m_pImgObjRectImage(nullptr), m_pImgObjCircImage(nullptr), m_pImgObjIntensity(nullptr), m_pImgObjLifetime(nullptr),
	m_pImgObjOctProjection(nullptr), m_pImgObjIntensityMap(nullptr), m_pImgObjLifetimeMap(nullptr), m_pImgObjIntensityPropMap(nullptr), m_pImgObjIntensityRatioMap(nullptr), m_pImgObjLongiImage(nullptr),
	m_pCirc(nullptr), m_pMedfiltRect(nullptr), m_pMedfiltIntensityMap(nullptr), m_pMedfiltLifetimeMap(nullptr), m_pMedfiltLongi(nullptr), _running(false) //, m_pAnn(nullptr)
{
	// Set configuration objects
	if (is_streaming)
		m_pStreamTab = (QStreamTab*)parent;
	else
		m_pResultTab = (QResultTab*)parent;
	m_pConfig = is_streaming ? m_pStreamTab->getMainWnd()->m_pConfiguration : m_pResultTab->getMainWnd()->m_pConfiguration;

    // Create view tab widgets
    createViewTabWidgets(is_streaming);

     // Create visualization buffers
    if (is_streaming) setStreamingBuffersObjects();		
}

QViewTab::~QViewTab()
{
	disconnect(this, SIGNAL(playingDone(void)), 0, 0);
	if (playing.joinable())
	{
		_running = false;
		playing.join();
	}

	if (m_pImgObjRectImage) delete m_pImgObjRectImage;
	if (m_pImgObjCircImage) delete m_pImgObjCircImage;
	if (m_pImgObjIntensity) delete m_pImgObjIntensity;
	if (m_pImgObjLifetime) delete m_pImgObjLifetime;	

	if (m_pImgObjOctProjection) delete m_pImgObjOctProjection;
	if (m_pImgObjIntensityMap) delete m_pImgObjIntensityMap;
	if (m_pImgObjLifetimeMap) delete m_pImgObjLifetimeMap;
	if (m_pImgObjIntensityPropMap) delete m_pImgObjIntensityPropMap;
	if (m_pImgObjIntensityRatioMap) delete m_pImgObjIntensityRatioMap;

    if (m_pImgObjLongiImage) delete m_pImgObjLongiImage;
	
	if (m_pCirc) delete m_pCirc;
	if (m_pMedfiltRect) delete m_pMedfiltRect;
	if (m_pMedfiltIntensityMap) delete m_pMedfiltIntensityMap;
	if (m_pMedfiltLifetimeMap) delete m_pMedfiltLifetimeMap;
    if (m_pMedfiltLongi) delete m_pMedfiltLongi;
    ///if (m_pAnn) delete m_pAnn;
}


void QViewTab::createViewTabWidgets(bool is_streaming)
{
    // Create image view : cross-section
#ifndef NEXT_GEN_SYSTEM
	int diameter = 2 * m_pConfig->octScans;
#else
	int diameter = m_pConfig->octScansFFT;
#endif

	m_pImageView_CircImage = new QImageView(ColorTable::colortable(OCT_COLORTABLE), diameter, diameter, true, this);
	if (is_streaming)		
		m_pImageView_CircImage->setMinimumSize(810, 810);
	else
#ifndef ALTERNATIVE_VIEW
		m_pImageView_CircImage->setMinimumSize(650, 650);
#else
		m_pImageView_CircImage->setMinimumSize(620, 620);
#endif

	m_pImageView_CircImage->setSquare(true);
	m_pImageView_CircImage->setCircle(1, OUTER_SHEATH_POSITION);

#ifdef ALTERNATIVE_VIEW
	if (!is_streaming)
	{
		for (int i = 0; i < 4; i++)
		{
			m_pWidget[i] = new QWidget(this);
			m_pWidget[i]->setFixedWidth(274);
			m_pWidget[i]->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
		}
	}
#endif

    if (!is_streaming)
    {
        // Create image view : en-face
        ColorTable temp_ctable;
        m_pImageView_EnFace = new QImageView(ColorTable::colortable(LIFETIME_COLORTABLE), 1, m_pConfig->flimAlines, true, this);
#ifndef ALTERNATIVE_VIEW
        m_pImageView_EnFace->setMinimumSize(500, 250);
#else
		m_pImageView_EnFace->setMinimumSize(250, 160); 
		m_pImageView_EnFace->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
#endif
		        
        // Create image view : longitudinal
        m_pImageView_Longi = new QImageView(ColorTable::colortable(OCT_COLORTABLE), 1, diameter, true, this);
		m_pImageView_Longi->getRender()->m_bCenterGrid = true;
#ifndef ALTERNATIVE_VIEW		
        m_pImageView_Longi->setMinimumSize(500, 350);		
#else
		m_pImageView_Longi->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
#endif

		// Create image view : IVUS
		m_pImageView_Ivus = new QImageView(ColorTable::colortable(IVUS_COLORTABLE), IVUS_IMG_SIZE, IVUS_IMG_SIZE, false, this);
		m_pImageView_Ivus->setSquare(true);
		m_pImageView_Ivus->setVisible(false);
		m_pImageView_Ivus->setMinimumSize(258, 258);
		m_pImageView_Ivus->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
		m_pImageView_Ivus->getRender()->m_bCanBeMagnified = true;
		
        // Create image view : colorbar
        uint8_t color[256 * 4];
        for (int i = 0; i < 256 * 4; i++)
            color[i] = 255 - i / 4;

		if (m_pConfig->flimVisualizationMode == VisualizationMode::_LIFETIME_)
	        m_pImageView_ColorBar = new QImageView(ColorTable::colortable(LIFETIME_COLORTABLE), 4, 256, false, this);
		else if (m_pConfig->flimVisualizationMode == VisualizationMode::_INTENSITY_PROP_)
			m_pImageView_ColorBar = new QImageView(ColorTable::colortable(INTENSITY_PROP_COLORTABLE), 4, 256, false, this);
		else if (m_pConfig->flimVisualizationMode == VisualizationMode::_INTENSITY_RATIO_)
			m_pImageView_ColorBar = new QImageView(ColorTable::colortable(INTENSITY_RATIO_COLORTABLE), 4, 256, false, this);
        m_pImageView_ColorBar->getRender()->setFixedWidth(30);
        m_pImageView_ColorBar->drawImage(color);
        m_pImageView_ColorBar->setFixedWidth(20);
		
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
        ///m_pSlider_SelectFrame->setTickPosition(QSlider::TicksBothSides);
        ///m_pSlider_SelectFrame->setTickInterval(20);		
        m_pSlider_SelectFrame->setOrientation(Qt::Horizontal);
        QPalette pal = m_pSlider_SelectFrame->palette();
        pal.setColor(QPalette::Highlight, QColor(42, 42, 42));
		///pal.setColor(QPalette::Window, QColor(255, 255, 255));
        m_pSlider_SelectFrame->setPalette(pal);
        m_pSlider_SelectFrame->setValue(0);		
		
        // Create push buttons for measuring distance & area
        m_pToggleButton_MeasureDistance = new QPushButton(this);
        m_pToggleButton_MeasureDistance->setCheckable(true);
        m_pToggleButton_MeasureDistance->setText("Measure Distance");
	    m_pToggleButton_MeasureDistance->setFixedWidth(120);
		
		m_pToggleButton_MeasureArea = new QPushButton(this);
		m_pToggleButton_MeasureArea->setCheckable(true);
		m_pToggleButton_MeasureArea->setText("Measure Area");
		m_pToggleButton_MeasureArea->setFixedWidth(120);

        // Create widgets for FLIm emission control
        m_pComboBox_ViewMode = new QComboBox(this);
        m_pComboBox_ViewMode->addItem("Ch 1 Lifetime");
        m_pComboBox_ViewMode->addItem("Ch 2 Lifetime");
        m_pComboBox_ViewMode->addItem("Ch 3 Lifetime");
		m_pComboBox_ViewMode->addItem("Ch 1 IntProportion");
		m_pComboBox_ViewMode->addItem("Ch 2 IntProportion");
		m_pComboBox_ViewMode->addItem("Ch 3 IntProportion");
		m_pComboBox_ViewMode->addItem("Ch 1/3 IntRatio");
		m_pComboBox_ViewMode->addItem("Ch 2/1 IntRatio");
		m_pComboBox_ViewMode->addItem("Ch 3/2 IntRatio");
        m_pComboBox_ViewMode->setCurrentIndex(3 * m_pConfig->flimVisualizationMode + m_pConfig->flimEmissionChannel - 1);
        m_pComboBox_ViewMode->setFixedWidth(120);

        m_pLabel_ViewMode = new QLabel("View Mode ", this);
        m_pLabel_ViewMode->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
        m_pLabel_ViewMode->setBuddy(m_pComboBox_ViewMode);
    }

    // Set layout
    QGridLayout *pGridLayout = new QGridLayout;
    pGridLayout->setSpacing(4);
	
    if (!is_streaming)
    {
#ifndef ALTERNATIVE_VIEW
		pGridLayout->addWidget(m_pImageView_CircImage, 0, 0, 3, 1);

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
		pHBoxLayout1->addWidget(m_pToggleButton_MeasureArea);

        QHBoxLayout *pHBoxLayout2 = new QHBoxLayout;
        pHBoxLayout2->setSpacing(4);

        pHBoxLayout2->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
        pHBoxLayout2->addWidget(m_pLabel_ViewMode);
        pHBoxLayout2->addWidget(m_pComboBox_ViewMode);

        pGridLayout->addItem(pHBoxLayout1, 3, 0);
        pGridLayout->addItem(pHBoxLayout2, 3, 1);
#else
		QGridLayout *pGridLayout1 = new QGridLayout;
		pGridLayout1->setSpacing(4);

		QLabel *pLabel_Dummy = new QLabel(this);
		pLabel_Dummy->setFixedHeight(25);

		pGridLayout1->addWidget(m_pImageView_Ivus, 0, 0, 1, 4);
		pGridLayout1->addWidget(pLabel_Dummy, 1, 0, 1, 4);
		pGridLayout1->addWidget(m_pPushButton_Decrement, 2, 0);
		pGridLayout1->addWidget(m_pToggleButton_Play, 2, 1);
		pGridLayout1->addWidget(m_pPushButton_Increment, 2, 2);
		pGridLayout1->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 2, 3);

		m_pWidget[1]->setLayout(pGridLayout1);
		
		QHBoxLayout *pHBoxLayout2 = new QHBoxLayout;
		pHBoxLayout2->setSpacing(4);
		
		pHBoxLayout2->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
		pHBoxLayout2->addWidget(m_pToggleButton_MeasureDistance);
		pHBoxLayout2->addWidget(m_pToggleButton_MeasureArea);
		
		QHBoxLayout *pHBoxLayout3 = new QHBoxLayout;
		pHBoxLayout3->setSpacing(4);

		pHBoxLayout3->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
		pHBoxLayout3->addWidget(m_pLabel_ViewMode);
		pHBoxLayout3->addWidget(m_pComboBox_ViewMode);

		QVBoxLayout *pVBoxLayout23 = new QVBoxLayout;
		pVBoxLayout23->setSpacing(4);

		pVBoxLayout23->addWidget(m_pImageView_EnFace);
		pVBoxLayout23->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Maximum));
		pVBoxLayout23->addItem(pHBoxLayout2);
		pVBoxLayout23->addItem(pHBoxLayout3);

		m_pWidget[3]->setLayout(pVBoxLayout23);
		
		QGridLayout *pGridLayout_CircView = new QGridLayout;
		pGridLayout_CircView->setSpacing(4);

		pGridLayout_CircView->addWidget(m_pWidget[0], 0, 0);
		pGridLayout_CircView->addWidget(m_pWidget[1], 1, 0, Qt::AlignBottom | Qt::AlignLeft);
		pGridLayout_CircView->addWidget(m_pImageView_CircImage, 0, 1, 2, 1, Qt::AlignCenter);
		pGridLayout_CircView->addWidget(m_pWidget[2], 0, 2);
		pGridLayout_CircView->addWidget(m_pWidget[3], 1, 2, Qt::AlignBottom | Qt::AlignRight);

		pGridLayout->addItem(pGridLayout_CircView, 0, 0, 1, 4);

		pGridLayout->addWidget(m_pSlider_SelectFrame, 1, 0, 1, 3);
		pGridLayout->addWidget(m_pImageView_Longi, 2, 0, 1, 3);
		pGridLayout->addWidget(m_pImageView_ColorBar, 2, 3);
#endif
    }
	else
		pGridLayout->addWidget(m_pImageView_CircImage, 0, 0, 3, 1);

    m_pViewWidget = new QWidget(this);
    m_pViewWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_pViewWidget->setLayout(pGridLayout);


    // Connect signal and slot
	connect(this, SIGNAL(drawImage(uint8_t*, float*, float*)), this, SLOT(visualizeImage(uint8_t*, float*, float*)));  // streaming
	connect(this, SIGNAL(makeCirc()), this, SLOT(constructCircImage()));
	connect(this, SIGNAL(paintCircImage(uint8_t*)), m_pImageView_CircImage, SLOT(drawRgbImage(uint8_t*)));

    if (!is_streaming)
    {
		connect(m_pToggleButton_Play, SIGNAL(toggled(bool)), this, SLOT(play(bool)));
		connect(this, &QViewTab::playingDone, [&]() { m_pToggleButton_Play->setChecked(false); });
        connect(m_pPushButton_Increment, &QPushButton::clicked, [&]() { m_pSlider_SelectFrame->setValue(m_pSlider_SelectFrame->value() + 1); });
        connect(m_pPushButton_Decrement, &QPushButton::clicked, [&]() { m_pSlider_SelectFrame->setValue(m_pSlider_SelectFrame->value() - 1); });

        connect(m_pSlider_SelectFrame, SIGNAL(valueChanged(int)), this, SLOT(visualizeImage(int)));
        connect(m_pToggleButton_MeasureDistance, SIGNAL(toggled(bool)), this, SLOT(measureDistance(bool)));
		connect(m_pToggleButton_MeasureArea, SIGNAL(toggled(bool)), this, SLOT(measureArea(bool)));

        connect(m_pComboBox_ViewMode, SIGNAL(currentIndexChanged(int)), this, SLOT(changeEmissionChannel(int)));

        connect(this, SIGNAL(paintEnFaceMap(uint8_t*)), m_pImageView_EnFace, SLOT(drawRgbImage(uint8_t*)));
        connect(this, SIGNAL(paintLongiImage(uint8_t*)), m_pImageView_Longi, SLOT(drawRgbImage(uint8_t*)));
    }
}


void QViewTab::invalidate()
{
	if (!m_pConfigTemp)
	{
		if (m_pResultTab)
		{
			m_pConfigTemp = m_pResultTab->getDataProcessing()->getConfigTemp();
			m_pImageView_Longi->getRender()->m_nPullbackLength = m_pConfigTemp->pullbackLength;
		}
	}

	visualizeEnFaceMap(true);
	visualizeImage(getCurrentFrame());
	visualizeLongiImage(getCurrentAline());
	
	if (m_pConfig->flimVisualizationMode == VisualizationMode::_LIFETIME_)
	{
#ifndef ALTERNATIVE_VIEW
		m_pImageView_EnFace->setEnterCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 4),
			QString::fromLocal8Bit("2-D FLIm En Face Chemogram - Ch %1 Lifetime (z-θ)").arg(m_pConfig->flimEmissionChannel)); });
#else
		m_pImageView_EnFace->setEnterCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 4),
			QString::fromLocal8Bit("2-D FLIm En Face Chemogram\n- Ch %1 Lifetime (z-θ)").arg(m_pConfig->flimEmissionChannel)); });
#endif

#ifndef ALTERNATIVE_VIEW
		m_pImageView_ColorBar->setText(QPoint(0, 0), QString("%1               Lifetime (nsec)               %2")
#else
		m_pImageView_ColorBar->setText(QPoint(0, 0), QString("%1            Lifetime (nsec)            %2")
#endif
			.arg(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max, 2, 'f', 1)
			.arg(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min, 2, 'f', 1), true);
		m_pImageView_ColorBar->resetColormap(ColorTable::colortable(LIFETIME_COLORTABLE));
	}
	else if (m_pConfig->flimVisualizationMode == VisualizationMode::_INTENSITY_PROP_)
	{
#ifndef ALTERNATIVE_VIEW
		m_pImageView_EnFace->setEnterCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 4),
			QString::fromLocal8Bit("2-D FLIm En Face Chemogram - Ch %1 Intensity Proportion (z-θ)").arg(m_pConfig->flimEmissionChannel)); });
#else
		m_pImageView_EnFace->setEnterCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 4),
			QString::fromLocal8Bit("2-D FLIm En Face Chemogram\n- Ch %1 Intensity Proportion (z-θ)").arg(m_pConfig->flimEmissionChannel)); });
#endif

#ifndef ALTERNATIVE_VIEW
		m_pImageView_ColorBar->setText(QPoint(0, 0), QString("%1         Intensity Proportion (AU)         %2")
#else
		m_pImageView_ColorBar->setText(QPoint(0, 0), QString("%1    Intensity Proportion (AU)    %2")
#endif
			.arg(m_pConfig->flimIntensityPropRange[m_pConfig->flimEmissionChannel - 1].max, 2, 'f', 1)
			.arg(m_pConfig->flimIntensityPropRange[m_pConfig->flimEmissionChannel - 1].min, 2, 'f', 1), true);
		m_pImageView_ColorBar->resetColormap(ColorTable::colortable(INTENSITY_PROP_COLORTABLE));
	}
	else if (m_pConfig->flimVisualizationMode == VisualizationMode::_INTENSITY_RATIO_)
	{
#ifndef ALTERNATIVE_VIEW
		m_pImageView_EnFace->setEnterCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 4),
			QString::fromLocal8Bit("2-D FLIm En Face Chemogram - Ch %1/%2 Intensity Ratio (z-θ)").arg(m_pConfig->flimEmissionChannel).arg((m_pConfig->flimEmissionChannel == 1) ? 3 : m_pConfig->flimEmissionChannel - 1)); });
#else
		m_pImageView_EnFace->setEnterCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 4),
			QString::fromLocal8Bit("2-D FLIm En Face Chemogram\n- Ch %1/%2 Intensity Ratio (z-θ)").arg(m_pConfig->flimEmissionChannel).arg((m_pConfig->flimEmissionChannel == 1) ? 3 : m_pConfig->flimEmissionChannel - 1)); });
#endif

#ifndef ALTERNATIVE_VIEW
		m_pImageView_ColorBar->setText(QPoint(0, 0), QString("%1           Intensity Ratio (AU)           %2")
#else
		m_pImageView_ColorBar->setText(QPoint(0, 0), QString("%1       Intensity Ratio (AU)       %2")
#endif
			.arg(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1].max, 2, 'f', 1)
			.arg(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1].min, 2, 'f', 1), true);
		m_pImageView_ColorBar->resetColormap(ColorTable::colortable(INTENSITY_RATIO_COLORTABLE));
	}
	m_pImageView_ColorBar->update();
}


void QViewTab::setStreamingBuffersObjects()
{
#ifndef NEXT_GEN_SYSTEM
	int diameter = 2 * m_pConfig->octScans;
#else
	int diameter = m_pConfig->octScansFFT;
#endif

    // Create visualization buffers
	m_visImage = np::Uint8Array2(diameter / 2, m_pConfig->octAlines);
    m_visIntensity = np::FloatArray2(m_pConfig->flimAlines, 4); memset(m_visIntensity.raw_ptr(), 0, sizeof(float) * m_visIntensity.length());
    m_visMeanDelay = np::FloatArray2(m_pConfig->flimAlines, 4); memset(m_visMeanDelay.raw_ptr(), 0, sizeof(float) * m_visMeanDelay.length());
    m_visLifetime = np::FloatArray2(m_pConfig->flimAlines, 3); memset(m_visLifetime.raw_ptr(), 0, sizeof(float) * m_visLifetime.length());

    // Create image visualization buffers
    ColorTable temp_ctable;
	if (m_pImgObjRectImage) delete m_pImgObjRectImage;
	m_pImgObjRectImage = new ImageObject(diameter / 2, m_pConfig->octAlines, temp_ctable.m_colorTableVector.at(OCT_COLORTABLE));
	if (m_pImgObjCircImage) delete m_pImgObjCircImage;
	m_pImgObjCircImage = new ImageObject(diameter, diameter, temp_ctable.m_colorTableVector.at(OCT_COLORTABLE));
	if (m_pImgObjIntensity) delete m_pImgObjIntensity;
    m_pImgObjIntensity = new ImageObject(m_pConfig->flimAlines, 1, temp_ctable.m_colorTableVector.at(ColorTable::gray));
	if (m_pImgObjLifetime) delete m_pImgObjLifetime;
    m_pImgObjLifetime = new ImageObject(m_pConfig->flimAlines, 1, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE));

	if (m_pCirc) delete m_pCirc;
	m_pCirc = new circularize(diameter / 2, m_pConfig->octAlines, false);
	if (m_pMedfiltRect) delete m_pMedfiltRect;
	m_pMedfiltRect = new medfilt(diameter / 2, m_pConfig->octAlines, 3, 3);

	m_pConfig->writeToLog("Streaming buffers and objects are initialized.");
}

void QViewTab::setBuffers(Configuration* pConfig)
{
#ifndef NEXT_GEN_SYSTEM
	int diameter = 2 * m_pConfig->octScans;
#else
	int diameter = m_pConfig->octScansFFT;
#endif

	// Clear existed buffers
#ifndef NEXT_GEN_SYSTEM
	{std::vector<np::Uint8Array2> clear_vector;
	clear_vector.swap(m_vectorOctImage); }
#else
	{std::vector<np::FloatArray2> clear_vector;
	clear_vector.swap(m_vectorOctImage)};
#endif
	{std::vector<np::FloatArray2> clear_vector;
	clear_vector.swap(m_intensityMap); }
	{std::vector<np::FloatArray2> clear_vector;
	clear_vector.swap(m_meandelayMap); }
	{std::vector<np::FloatArray2> clear_vector;
	clear_vector.swap(m_lifetimeMap); }
	{std::vector<np::FloatArray2> clear_vector;
	clear_vector.swap(m_intensityProportionMap); }
	{std::vector<np::FloatArray2> clear_vector;
	clear_vector.swap(m_intensityRatioMap); }

	{std::vector<np::FloatArray2> clear_vector;
	clear_vector.swap(m_vectorPulseCrop); }
	{std::vector<np::FloatArray2> clear_vector;
	clear_vector.swap(m_vectorPulseBgSub); }
	{std::vector<np::FloatArray2> clear_vector;
	clear_vector.swap(m_vectorPulseMask); }
	{std::vector<np::FloatArray2> clear_vector;
	clear_vector.swap(m_vectorPulseSpline); }
	{std::vector<np::FloatArray2> clear_vector;
	clear_vector.swap(m_vectorPulseFilter); }

	// Data buffers
	for (int i = 0; i < pConfig->frames; i++)
	{
#ifndef NEXT_GEN_SYSTEM
		np::Uint8Array2 buffer = np::Uint8Array2(diameter / 2, pConfig->octAlines);
		memset(buffer, 0, sizeof(uint8_t) * diameter / 2 * pConfig->octAlines);
#else
		np::FloatArray2 buffer = np::FloatArray2(diameter / 2, pConfig->octAlines);
		memset(buffer, 0, sizeof(float) * diameter / 2 * pConfig->octAlines);
#endif
		m_vectorOctImage.push_back(buffer);
	}
#ifndef NEXT_GEN_SYSTEM
	m_octProjection = np::Uint8Array2(pConfig->octAlines, pConfig->frames);
#else
	m_octProjection = np::FloatArray2(pConfig->octAlines, pConfig->frames);
#endif

	for (int i = 0; i < 3; i++)
	{
		np::FloatArray2 intensity = np::FloatArray2(pConfig->flimAlines, pConfig->frames);
		np::FloatArray2 lifetime = np::FloatArray2(pConfig->flimAlines, pConfig->frames);
		np::FloatArray2 intensity_proportion = np::FloatArray2(pConfig->flimAlines, pConfig->frames);
		np::FloatArray2 intensity_ratio = np::FloatArray2(pConfig->flimAlines, pConfig->frames);
		m_intensityMap.push_back(intensity);
		m_lifetimeMap.push_back(lifetime);
		m_intensityProportionMap.push_back(intensity_proportion);
		m_intensityRatioMap.push_back(intensity_ratio);
	}
	for (int i = 0; i < 4; i++)
	{
		np::FloatArray2 meandelay = np::FloatArray2(pConfig->flimAlines, pConfig->frames);
		m_meandelayMap.push_back(meandelay);
	}

	m_pConfig->writeToLog("Reviewing buffers are initialized.");
}

void QViewTab::setObjects(Configuration * pConfig)
{
	// Non-4's multiple width
	int frames4 = ROUND_UP_4S(pConfig->frames);

#ifndef NEXT_GEN_SYSTEM
	int diameter = 2 * m_pConfig->octScans;
#else
	int diameter = m_pConfig->octScansFFT;
#endif

	// Visualization buffers
	ColorTable temp_ctable;
	if (m_pImgObjRectImage) delete m_pImgObjRectImage;
	m_pImgObjRectImage = new ImageObject(diameter / 2, pConfig->octAlines, temp_ctable.m_colorTableVector.at(OCT_COLORTABLE));
	if (m_pImgObjCircImage) delete m_pImgObjCircImage;
	m_pImgObjCircImage = new ImageObject(diameter, diameter, temp_ctable.m_colorTableVector.at(OCT_COLORTABLE));
	///if (m_pImgObjIntensity) delete m_pImgObjIntensity;
	///m_pImgObjIntensity = new ImageObject(RING_THICKNESS, pConfig->flimAlines, temp_ctable.m_colorTableVector.at(ColorTable::gray));
	///if (m_pImgObjLifetime) delete m_pImgObjLifetime;
	///m_pImgObjLifetime = new ImageObject(RING_THICKNESS, pConfig->flimAlines, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE));

	// En face map visualization buffers
	if (m_pImgObjOctProjection) delete m_pImgObjOctProjection;
	m_pImgObjOctProjection = new ImageObject(frames4, pConfig->octAlines, temp_ctable.m_colorTableVector.at(OCT_COLORTABLE));
	if (m_pImgObjIntensityMap) delete m_pImgObjIntensityMap;
	m_pImgObjIntensityMap = new ImageObject(frames4, pConfig->flimAlines, temp_ctable.m_colorTableVector.at(INTENSITY_COLORTABLE));
	if (m_pImgObjLifetimeMap) delete m_pImgObjLifetimeMap;
	m_pImgObjLifetimeMap = new ImageObject(frames4, pConfig->flimAlines, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE));
	if (m_pImgObjIntensityPropMap) delete m_pImgObjIntensityPropMap;
	m_pImgObjIntensityPropMap = new ImageObject(frames4, pConfig->flimAlines, temp_ctable.m_colorTableVector.at(INTENSITY_PROP_COLORTABLE));
	if (m_pImgObjIntensityRatioMap) delete m_pImgObjIntensityRatioMap;
	m_pImgObjIntensityRatioMap = new ImageObject(frames4, pConfig->flimAlines, temp_ctable.m_colorTableVector.at(INTENSITY_RATIO_COLORTABLE));

	// Longitudinal image visualization buffers
	if (m_pImgObjLongiImage) delete m_pImgObjLongiImage;
	m_pImgObjLongiImage = new ImageObject(frames4, diameter, temp_ctable.m_colorTableVector.at(OCT_COLORTABLE));
	
	// Circ & Medfilt objects
	if (m_pCirc) delete m_pCirc;
	m_pCirc = new circularize(diameter / 2, pConfig->octAlines, false);

	if (m_pMedfiltRect) delete m_pMedfiltRect;
	m_pMedfiltRect = new medfilt(diameter / 2, pConfig->octAlines, 3, 3);
	if (m_pMedfiltIntensityMap) delete m_pMedfiltIntensityMap;
	//m_pMedfiltIntensityMap = new medfilt(frames4, pConfig->flimAlines, 3, 5);
	m_pMedfiltIntensityMap = new medfilt(pConfig->flimAlines, pConfig->frames, 5, 3);
	if (m_pMedfiltLifetimeMap) delete m_pMedfiltLifetimeMap;
	//m_pMedfiltLifetimeMap = new medfilt(frames4, pConfig->flimAlines, 5, 7);
	m_pMedfiltLifetimeMap = new medfilt(pConfig->flimAlines, pConfig->frames, 7, 5);
	if (m_pMedfiltLongi) delete m_pMedfiltLongi;
	m_pMedfiltLongi = new medfilt(frames4, diameter, 3, 3);

	m_pConfig->writeToLog("Reviewing objects are initialized.");
}

void QViewTab::setWidgets(Configuration* pConfig)
{
	// Non-4's multiple width
	int frames4 = (pConfig->frames);

#ifndef NEXT_GEN_SYSTEM
	int diameter = 2 * m_pConfig->octScans;
#else
	int diameter = m_pConfig->octScansFFT;
#endif

	// Set slider 
	m_pSlider_SelectFrame->setRange(0, pConfig->frames - 1);
	m_pSlider_SelectFrame->setValue(0);
	m_pSlider_SelectFrame->setTickInterval(int(pConfig->frames / 4));

	// ImageView objects
	m_pImageView_CircImage->resetSize(diameter, diameter);
	m_pImageView_CircImage->setRLineChangeCallback([&](int aline) { 
		visualizeLongiImage(aline); 
		if (m_pResultTab->getSettingDlg())
			m_pResultTab->getSettingDlg()->getPulseReviewTab()->setCurrentAline(aline / 4);
	});
	m_pImageView_CircImage->setVerticalLine(1, pConfig->octAlines / 8);
	m_pImageView_CircImage->getRender()->m_bRadial = true;
	m_pImageView_CircImage->getRender()->m_bDiametric = true;
	m_pImageView_CircImage->getRender()->m_rMax = pConfig->octAlines;
	///m_pImageView_CircImage->setWheelMouseCallback([&]() { m_pToggleButton_MeasureDistance->setChecked(false); });
	m_pImageView_CircImage->getRender()->m_bCanBeMagnified = true;
	m_pImageView_CircImage->setScaleBar(int(1000.0 / PIXEL_RESOLUTION));

	m_pImageView_EnFace->resetSize(frames4, pConfig->flimAlines);
	m_pImageView_EnFace->setVLineChangeCallback([&](int frame) { m_pSlider_SelectFrame->setValue(frame); });
	m_pImageView_EnFace->setVerticalLine(1, 0);	
	m_pImageView_EnFace->setHLineChangeCallback([&](int aline) { visualizeLongiImage(4 * aline); });
	m_pImageView_EnFace->setHorizontalLine(1, 0);
	if (m_pConfig->flimVisualizationMode == VisualizationMode::_LIFETIME_)
	{
#ifndef ALTERNATIVE_VIEW
		m_pImageView_EnFace->setEnterCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 4),
			QString::fromLocal8Bit("2-D FLIm En Face Chemogram - Ch %1 Lifetime (z-θ)").arg(m_pConfig->flimEmissionChannel)); });
#else
		m_pImageView_EnFace->setEnterCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 4),
			QString::fromLocal8Bit("2-D FLIm En Face Chemogram\n- Ch %1 Lifetime (z-θ)").arg(m_pConfig->flimEmissionChannel)); });
#endif
	}
	else if (m_pConfig->flimVisualizationMode == VisualizationMode::_INTENSITY_PROP_)
	{
#ifndef ALTERNATIVE_VIEW
		m_pImageView_EnFace->setEnterCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 4),
			QString::fromLocal8Bit("2-D FLIm En Face Chemogram - Ch %1 Intensity Proportion (z-θ)").arg(m_pConfig->flimEmissionChannel)); });
#else
		m_pImageView_EnFace->setEnterCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 4),
			QString::fromLocal8Bit("2-D FLIm En Face Chemogram\n- Ch %1 Intensity Proportion (z-θ)").arg(m_pConfig->flimEmissionChannel)); });
#endif
	}
	else if (m_pConfig->flimVisualizationMode == VisualizationMode::_INTENSITY_RATIO_)
	{
#ifndef ALTERNATIVE_VIEW
		m_pImageView_EnFace->setEnterCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 4),
			QString::fromLocal8Bit("2-D FLIm En Face Chemogram - Ch %1/%2 Intensity Ratio (z-θ)").arg(m_pConfig->flimEmissionChannel).arg((m_pConfig->flimEmissionChannel == 1) ? 3 : m_pConfig->flimEmissionChannel - 1)); });
#else
		m_pImageView_EnFace->setEnterCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 4),
			QString::fromLocal8Bit("2-D FLIm En Face Chemogram\n- Ch %1/%2 Intensity Ratio (z-θ)").arg(m_pConfig->flimEmissionChannel).arg((m_pConfig->flimEmissionChannel == 1) ? 3 : m_pConfig->flimEmissionChannel - 1)); });
#endif
	}
	m_pImageView_EnFace->setLeaveCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 18), ""); });

	m_pImageView_Longi->resetSize(frames4, diameter);
	//m_pImageView_Longi->resetSize(LONGI_WIDTH, LONGI_HEIGHT);
	m_pImageView_Longi->setVLineChangeCallback([&](int frame) { m_pSlider_SelectFrame->setValue(frame); });
	m_pImageView_Longi->setVerticalLine(1, 0); 
	m_pImageView_Longi->getRender()->m_bDiametric = true;
	///m_pImageView_Longi->setHorizontalLine(2, diameter / 2 + OUTER_SHEATH_POSITION, diameter / 2 - OUTER_SHEATH_POSITION);
	m_pImageView_Longi->setEnterCallback([&]() { m_pImageView_Longi->setText(QPoint(8, 4), "Longitudinal View (z-r)"); });
	m_pImageView_Longi->setLeaveCallback([&]() { m_pImageView_Longi->setText(QPoint(8, 4), ""); });
	
	if (m_pConfig->flimVisualizationMode == VisualizationMode::_LIFETIME_)
	{
#ifndef ALTERNATIVE_VIEW
		m_pImageView_ColorBar->setText(QPoint(0, 0), QString("%1               Lifetime (nsec)               %2")
#else
		m_pImageView_ColorBar->setText(QPoint(0, 0), QString("%1            Lifetime (nsec)            %2")
#endif
			.arg(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max, 2, 'f', 1)
			.arg(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min, 2, 'f', 1), true);
		m_pImageView_ColorBar->resetColormap(ColorTable::colortable(LIFETIME_COLORTABLE));
	}
	else if (m_pConfig->flimVisualizationMode == VisualizationMode::_INTENSITY_PROP_)
	{
#ifndef ALTERNATIVE_VIEW
		m_pImageView_ColorBar->setText(QPoint(0, 0), QString("%1       Intensity Proportion (AU)       %2")
#else
		m_pImageView_ColorBar->setText(QPoint(0, 0), QString("%1    Intensity Proportion (AU)    %2")
#endif
			.arg(m_pConfig->flimIntensityPropRange[m_pConfig->flimEmissionChannel - 1].max, 2, 'f', 1)
			.arg(m_pConfig->flimIntensityPropRange[m_pConfig->flimEmissionChannel - 1].min, 2, 'f', 1), true);
		m_pImageView_ColorBar->resetColormap(ColorTable::colortable(INTENSITY_PROP_COLORTABLE));
	}
	else if (m_pConfig->flimVisualizationMode == VisualizationMode::_INTENSITY_RATIO_)
	{
#ifndef ALTERNATIVE_VIEW
		m_pImageView_ColorBar->setText(QPoint(0, 0), QString("%1           Intensity Ratio (AU)           %2")
#else
		m_pImageView_ColorBar->setText(QPoint(0, 0), QString("%1       Intensity Ratio (AU)       %2")
#endif
			.arg(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1].max, 2, 'f', 1)
			.arg(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1].min, 2, 'f', 1), true);
		m_pImageView_ColorBar->resetColormap(ColorTable::colortable(INTENSITY_RATIO_COLORTABLE));
	}

	m_pConfig->writeToLog("Reviewing widgets are initialized.");
}

void QViewTab::setCircImageViewClickedMouseCallback(const std::function<void(void)> &slot)
{
	m_pImageView_CircImage->setClickedMouseCallback(slot);
}

void QViewTab::setEnFaceImageViewClickedMouseCallback(const std::function<void(void)> &slot)
{
	m_pImageView_EnFace->setClickedMouseCallback(slot);
}

void QViewTab::setLongiImageViewClickedMouseCallback(const std::function<void(void)> &slot)
{
	m_pImageView_Longi->setClickedMouseCallback(slot);
}


void QViewTab::visualizeEnFaceMap(bool scaling)
{
	if (m_vectorOctImage.size() != 0)
	{
		if (scaling)
		{
			// Scaling OCT projection
			///IppiSize roi_proj = { m_octProjection.size(0), m_octProjection.size(1) };
			///IppiSize roi_proj4 = { roi_proj.width, ROUND_UP_4S(roi_proj.height) };

			///np::FloatArray2 scale_temp32(roi_proj.width, roi_proj.height);
			///np::Uint8Array2 scale_temp8(roi_proj4.width, roi_proj4.height);

			///ippsConvert_8u32f(m_octProjection.raw_ptr(), scale_temp32.raw_ptr(), scale_temp32.length());
			///ippiScale_32f8u_C1R(scale_temp32.raw_ptr(), sizeof(float) * roi_proj.width,
			///	scale_temp8.raw_ptr(), sizeof(uint8_t) * roi_proj4.width, roi_proj,
			///	m_pConfig->octGrayRange.min, m_pConfig->octGrayRange.max);
			///ippiTranspose_8u_C1R(scale_temp8.raw_ptr(), sizeof(uint8_t) * roi_proj4.width,
			///	m_pImgObjOctProjection->arr.raw_ptr(), sizeof(uint8_t) * roi_proj4.height, roi_proj4);
			///circShift(m_pImgObjOctProjection->arr, m_pConfig->rotatedAlines);

			// Scaling FLIM map
			scaleFLImEnFaceMap(m_pImgObjIntensityMap, m_pImgObjLifetimeMap, m_pImgObjIntensityPropMap, m_pImgObjIntensityRatioMap, m_pConfig->flimEmissionChannel - 1, m_pConfig->flimVisualizationMode);
		}

		if (m_pConfig->flimVisualizationMode == VisualizationMode::_LIFETIME_)
			emit paintEnFaceMap(m_pImgObjLifetimeMap->qrgbimg.bits());
		else if (m_pConfig->flimVisualizationMode == VisualizationMode::_INTENSITY_PROP_)
			emit paintEnFaceMap(m_pImgObjIntensityPropMap->qrgbimg.bits());
		else if (m_pConfig->flimVisualizationMode == VisualizationMode::_INTENSITY_RATIO_)
			emit paintEnFaceMap(m_pImgObjIntensityRatioMap->qrgbimg.bits());
	}
}

void QViewTab::visualizeImage(uint8_t* oct_im, float* intensity, float* lifetime)
{
	// OCT Visualization
#ifndef NEXT_GEN_SYSTEM
	IppiSize roi_oct = { m_pConfig->octScans, m_pConfig->octAlines };
#else
	IppiSize roi_oct = { m_pConfig->octScansFFT / 2, m_pConfig->octAlines };
#endif	

	ippiCopy_8u_C1R(oct_im + m_pConfig->innerOffsetLength, roi_oct.width, m_pImgObjRectImage->arr.raw_ptr(), roi_oct.width,
		{ roi_oct.width - m_pConfig->innerOffsetLength, roi_oct.height });
	(*m_pMedfiltRect)(m_pImgObjRectImage->arr.raw_ptr());
		
	// FLIm Visualization
	IppiSize roi_flim = { 1, m_pConfig->flimAlines };

	float* scanIntensity = intensity + m_pConfig->flimEmissionChannel * roi_flim.height;
	ippiScale_32f8u_C1R(scanIntensity, roi_flim.width * sizeof(float), m_pImgObjIntensity->arr.raw_ptr(), roi_flim.width * sizeof(uint8_t), roi_flim,
		m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min, m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max);

	float* scanLifetime = lifetime + (m_pConfig->flimEmissionChannel - 1) * roi_flim.height;
	ippiScale_32f8u_C1R(scanLifetime, roi_flim.width * sizeof(float), m_pImgObjLifetime->arr.raw_ptr(), roi_flim.width * sizeof(uint8_t), roi_flim,
		m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min, m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max);

	// Convert RGB
	m_pImgObjRectImage->convertRgb();
	m_pImgObjIntensity->convertScaledRgb();
	m_pImgObjLifetime->convertScaledRgb();
	
	// Intensity-weight lifetime
	ippsMul_8u_ISfs(m_pImgObjIntensity->qrgbimg.bits(), m_pImgObjLifetime->qrgbimg.bits(), m_pImgObjIntensity->qrgbimg.byteCount(), 8);

	// Copy FLIM rings
	tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)RING_THICKNESS),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
		{
			ippiCopy_8u_C3R(m_pImgObjLifetime->qrgbimg.constBits(), 3 * roi_flim.width,
				m_pImgObjRectImage->qrgbimg.bits() + 3 * (m_pImgObjRectImage->arr.size(0) - RING_THICKNESS + i), 3 * m_pImgObjRectImage->arr.size(0), { 1, m_pImgObjRectImage->arr.size(1) });
		}
	});
	
	// Make circularzing
	emit makeCirc();
}

void QViewTab::visualizeImage(int frame)
{
    if (m_vectorOctImage.size() != 0)
    {
		// Measure state uncheck
		if (m_pToggleButton_MeasureDistance->isChecked()) m_pToggleButton_MeasureDistance->setChecked(false);
		if (m_pToggleButton_MeasureArea->isChecked()) m_pToggleButton_MeasureArea->setChecked(false);

        // OCT Visualization
		IppiSize roi_oct = { m_pImgObjRectImage->getWidth(), m_pImgObjRectImage->getHeight() };

#ifndef NEXT_GEN_SYSTEM
        np::FloatArray2 scale_temp(roi_oct.width, roi_oct.height);
        ippsConvert_8u32f(m_vectorOctImage.at(frame).raw_ptr(), scale_temp.raw_ptr(), scale_temp.length());
        ippiScale_32f8u_C1R(scale_temp.raw_ptr(), roi_oct.width * sizeof(float),
            m_pImgObjRectImage->arr.raw_ptr(), roi_oct.width * sizeof(uint8_t), roi_oct, 
			(float)m_pConfigTemp->octGrayRange.min, (float)m_pConfigTemp->octGrayRange.max);
#else
		ippiScale_32f8u_C1R(m_vectorOctImage.at(frame).raw_ptr(), roi_oct.width * sizeof(float),
			m_pImgObjRectImage->arr.raw_ptr(), roi_oct.width * sizeof(uint8_t), roi_oct, (float)m_pConfig->axsunDbRange.min, (float)m_pConfig->axsunDbRange.max);
#endif
		circShift(m_pImgObjRectImage->arr, (m_pConfigTemp->rotatedAlines + m_pConfigTemp->intraFrameSync) % m_pConfig->octAlines);
        (*m_pMedfiltRect)(m_pImgObjRectImage->arr.raw_ptr());
		
		// Convert RGB
		m_pImgObjRectImage->convertRgb();

        // FLIM Visualization
		IppiSize roi_flimring = { 1, m_pImgObjIntensityMap->arr.size(1) };
		
		ImageObject *temp_imgobj;
		if (m_pConfig->flimVisualizationMode == VisualizationMode::_LIFETIME_)
		{
			temp_imgobj = new ImageObject(roi_flimring.height, 1, m_pImgObjLifetimeMap->getColorTable());
			ippiCopy_8u_C3R(m_pImgObjLifetimeMap->qrgbimg.constBits() + 3 * frame, 3 * m_pImgObjLifetimeMap->arr.size(0),
				temp_imgobj->qrgbimg.bits(), 3, roi_flimring);
		}
		else if (m_pConfig->flimVisualizationMode == VisualizationMode::_INTENSITY_PROP_)
		{
			temp_imgobj = new ImageObject(roi_flimring.height, 1, m_pImgObjIntensityPropMap->getColorTable());
			ippiCopy_8u_C3R(m_pImgObjIntensityPropMap->qrgbimg.constBits() + 3 * frame, 3 * m_pImgObjIntensityPropMap->arr.size(0),
				temp_imgobj->qrgbimg.bits(), 3, roi_flimring);
		}
		else if (m_pConfig->flimVisualizationMode == VisualizationMode::_INTENSITY_RATIO_)
		{
			temp_imgobj = new ImageObject(roi_flimring.height, 1, m_pImgObjIntensityRatioMap->getColorTable());
			ippiCopy_8u_C3R(m_pImgObjIntensityRatioMap->qrgbimg.constBits() + 3 * frame, 3 * m_pImgObjIntensityRatioMap->arr.size(0),
				temp_imgobj->qrgbimg.bits(), 3, roi_flimring);
		}
		temp_imgobj->scaledRgb4();

		tbb::parallel_for(tbb::blocked_range<size_t>(1, (size_t)RING_THICKNESS),
			[&](const tbb::blocked_range<size_t>& r) {
			for (size_t i = r.begin(); i != r.end(); ++i)
			{
				ippiCopy_8u_C3R(temp_imgobj->qrgbimg.constBits(), 3 * roi_flimring.width,
					m_pImgObjRectImage->qrgbimg.bits() + 3 * (m_pImgObjRectImage->arr.size(0) - RING_THICKNESS + i), 
					3 * m_pImgObjRectImage->arr.size(0), { 1, m_pImgObjRectImage->arr.size(1) });
			}
		});
				
        // Make circularzing
        emit makeCirc();

        // En Face Lines
        m_pImageView_EnFace->setVerticalLine(1, frame);
		m_pImageView_EnFace->getRender()->DidClickedMouse();
        m_pImageView_EnFace->getRender()->update();
				
        // Longitudinval Lines
        m_pImageView_Longi->setVerticalLine(1, frame);
		m_pImageView_Longi->getRender()->update();

		// Pulse review tab
		if (m_pResultTab->getSettingDlg())
		{
			if (!m_pResultTab->getSettingDlg()->getPulseReviewTab()->m_bFromTable)
				m_pResultTab->getSettingDlg()->getPulseReviewTab()->cancel();
			m_pResultTab->getSettingDlg()->getPulseReviewTab()->drawPulse(getCurrentAline() / 4);
		}
		
		// Matched IVUS image
		if (m_pResultTab->getIvusViewerDlg())
		{
			int k = 0;
			IvusViewerDlg* pIvusViewerDlg = m_pResultTab->getIvusViewerDlg();
			foreach(QStringList _matches, pIvusViewerDlg->m_vectorMatches)
			{
				if (_matches.at(0).toInt() == getCurrentFrame() + 1)
					break;
				k++;
			}

			np::Uint8Array2 ivusImage(IVUS_IMG_SIZE, IVUS_IMG_SIZE);
			if (k != pIvusViewerDlg->m_vectorMatches.size())
			{
				QStringList matches = pIvusViewerDlg->m_vectorMatches.at(k);

				int ivus_frame = matches.at(1).toInt() - 1;
				int ivus_rotation = matches.at(2).toInt();
				pIvusViewerDlg->getIvusImage(ivus_frame, ivus_rotation, ivusImage);

				m_pImageView_Ivus->setEnterCallback([&, pIvusViewerDlg, ivus_frame, ivus_rotation]() { m_pImageView_Ivus->setText(QPoint(8, 230),
					QString("%1 / %2 (CCW %3 deg)").arg(ivus_frame + 1).arg((int)pIvusViewerDlg->m_vectorIvusImages.size()).arg(ivus_rotation)); });	
				m_pImageView_Ivus->setLeaveCallback([&]() { m_pImageView_Ivus->setText(QPoint(8, 230), ""); });
			}
			else
			{
				ippsSet_8u(52, ivusImage, ivusImage.length());
				m_pImageView_Ivus->setEnterCallback([&]() { m_pImageView_Ivus->setText(QPoint(8, 230), ""); });
			}

			m_pImageView_Ivus->drawImage(ivusImage);
		}

        // Status Update
		QString str; str.sprintf("Frame : %3d / %3d", frame + 1, (int)m_vectorOctImage.size());
#ifndef ALTERNATIVE_VIEW
		m_pImageView_CircImage->setText(QPoint(15, 620), str);
#else
		m_pImageView_CircImage->setText(QPoint(15, 590), str);
#endif
    }
}

void QViewTab::visualizeLongiImage(int aline)
{
	// Pre-determined values
	int frames = (int)m_vectorOctImage.size();
	int octScans = m_pConfigTemp->octScans; 
    int octAlines = m_octProjection.size(0);

    // Specified A line
	int aline_ = (aline + octAlines / 2) % octAlines; // aline & aline_ paired

	int aline0 = (aline + m_pConfigTemp->rotatedAlines + m_pConfigTemp->intraFrameSync) % octAlines;
    int aline1 = (aline0 + octAlines / 2) % octAlines; // aline0 & aline1 paired

    // Make longitudinal - OCT
	IppiSize roi_longi = { m_pImgObjLongiImage->getHeight(), m_pImgObjLongiImage->getWidth() };

	np::FloatArray2 longi_temp(roi_longi.width, roi_longi.height);
	np::Uint8Array2 scale_temp(roi_longi.width, roi_longi.height);
	tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)frames),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
		{
#ifndef NEXT_GEN_SYSTEM
			memcpy(&scale_temp(0, (int)i), &m_vectorOctImage.at((int)i)(0, aline0), sizeof(uint8_t) * octScans);
			memcpy(&scale_temp(octScans, (int)i), &m_vectorOctImage.at((int)i)(0, aline1), sizeof(uint8_t) * octScans);
			ippsFlip_8u_I(&scale_temp(0, (int)i), octScans);
#else
			memcpy(&longi_temp(0, (int)i), &m_vectorOctImage.at((int)i)(0, aline0), sizeof(float) * octScans);
			memcpy(&longi_temp(octScans, (int)i), &m_vectorOctImage.at((int)i)(0, aline1), sizeof(float) * octScans);
			ippsFlip_32f_I(&longi_temp(0, (int)i), octScans);
#endif
		}
	});

#ifndef NEXT_GEN_SYSTEM
    ippsConvert_8u32f(scale_temp.raw_ptr(), longi_temp.raw_ptr(), scale_temp.length());
    ippiScale_32f8u_C1R(longi_temp.raw_ptr(), roi_longi.width * sizeof(float),
        scale_temp.raw_ptr(), roi_longi.width * sizeof(uint8_t), roi_longi, m_pConfigTemp->octGrayRange.min, m_pConfigTemp->octGrayRange.max);
#else
	ippiScale_32f8u_C1R(longi_temp.raw_ptr(), roi_longi.width * sizeof(float),
		scale_temp.raw_ptr(), roi_longi.width * sizeof(uint8_t), roi_longi, m_pConfig->axsunDbRange.min, m_pConfig->axsunDbRange.max);
#endif
    ///if (aline0 > (octAlines / 2))
    ///    ippiMirror_8u_C1IR(scale_temp.raw_ptr(), sizeof(uint8_t) * roi_longi.width, roi_longi, ippAxsVertical);
    ippiTranspose_8u_C1R(scale_temp.raw_ptr(), roi_longi.width * sizeof(uint8_t), m_pImgObjLongiImage->arr.raw_ptr(), roi_longi.height * sizeof(uint8_t), roi_longi);
    (*m_pMedfiltLongi)(m_pImgObjLongiImage->arr.raw_ptr());

    m_pImgObjLongiImage->convertRgb();

    // Make longitudinal - FLIM
	if (m_pConfig->flimVisualizationMode == VisualizationMode::_LIFETIME_)
	{
		tbb::parallel_for(tbb::blocked_range<size_t>(1, (size_t)RING_THICKNESS),
			[&](const tbb::blocked_range<size_t>& r) {
			for (size_t i = r.begin(); i != r.end(); ++i)
			{
				memcpy(m_pImgObjLongiImage->qrgbimg.bits() + 3 * (int)i * m_pImgObjLongiImage->getWidth(),
					m_pImgObjLifetimeMap->qrgbimg.bits() + 3 * int(aline / 4) * m_pImgObjLifetimeMap->getWidth(), sizeof(uint8_t) * 3 * m_pImgObjLongiImage->getWidth());
				memcpy(m_pImgObjLongiImage->qrgbimg.bits() + 3 * (int)(m_pImgObjLongiImage->getHeight() - i) * m_pImgObjLongiImage->getWidth(),
					m_pImgObjLifetimeMap->qrgbimg.bits() + 3 * int(aline_ / 4) * m_pImgObjLifetimeMap->getWidth(), sizeof(uint8_t) * 3 * m_pImgObjLongiImage->getWidth());
			}
		});
	}
	else if (m_pConfig->flimVisualizationMode == VisualizationMode::_INTENSITY_PROP_)
	{
		tbb::parallel_for(tbb::blocked_range<size_t>(1, (size_t)RING_THICKNESS),
			[&](const tbb::blocked_range<size_t>& r) {
			for (size_t i = r.begin(); i != r.end(); ++i)
			{
				memcpy(m_pImgObjLongiImage->qrgbimg.bits() + 3 * (int)i * m_pImgObjLongiImage->getWidth(),
					m_pImgObjIntensityPropMap->qrgbimg.bits() + 3 * int(aline / 4) * m_pImgObjIntensityPropMap->getWidth(), sizeof(uint8_t) * 3 * m_pImgObjLongiImage->getWidth());
				memcpy(m_pImgObjLongiImage->qrgbimg.bits() + 3 * (int)(m_pImgObjLongiImage->getHeight() - i) * m_pImgObjLongiImage->getWidth(),
					m_pImgObjIntensityPropMap->qrgbimg.bits() + 3 * int(aline_ / 4) * m_pImgObjIntensityPropMap->getWidth(), sizeof(uint8_t) * 3 * m_pImgObjLongiImage->getWidth());
			}
		});
	}
	else if (m_pConfig->flimVisualizationMode == VisualizationMode::_INTENSITY_RATIO_)
	{
		tbb::parallel_for(tbb::blocked_range<size_t>(1, (size_t)RING_THICKNESS),
			[&](const tbb::blocked_range<size_t>& r) {
			for (size_t i = r.begin(); i != r.end(); ++i)
			{
				memcpy(m_pImgObjLongiImage->qrgbimg.bits() + 3 * (int)i * m_pImgObjLongiImage->getWidth(),
					m_pImgObjIntensityRatioMap->qrgbimg.bits() + 3 * int(aline / 4) * m_pImgObjIntensityRatioMap->getWidth(), sizeof(uint8_t) * 3 * m_pImgObjLongiImage->getWidth());
				memcpy(m_pImgObjLongiImage->qrgbimg.bits() + 3 * (int)(m_pImgObjLongiImage->getHeight() - i) * m_pImgObjLongiImage->getWidth(),
					m_pImgObjIntensityRatioMap->qrgbimg.bits() + 3 * int(aline_ / 4) * m_pImgObjIntensityRatioMap->getWidth(), sizeof(uint8_t) * 3 * m_pImgObjLongiImage->getWidth());
			}
		});
	}
	
	// Draw longitudinal view
    emit paintLongiImage(m_pImgObjLongiImage->qrgbimg.bits());

	// Circular view lines
    m_pImageView_CircImage->setVerticalLine(1, aline);
    m_pImageView_CircImage->getRender()->update();

	// En Face lines
	m_pImageView_EnFace->setHorizontalLine(1, int(aline / 4));
	m_pImageView_EnFace->getRender()->update();
}


void QViewTab::constructCircImage()
{
    // Circularizing
    np::Uint8Array2 rect_temp(m_pImgObjRectImage->qrgbimg.bits(), 3 * m_pImgObjRectImage->arr.size(0), m_pImgObjRectImage->arr.size(1));
    (*m_pCirc)(rect_temp, m_pImgObjCircImage->qrgbimg.bits(), false, true); // depth-aline domain, rgb coordinate
	
    // Draw image
    emit paintCircImage(m_pImgObjCircImage->qrgbimg.bits());
} 


void QViewTab::play(bool enabled)
{
	if (enabled)
	{
		m_pToggleButton_Play->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
		m_pToggleButton_MeasureDistance->setDisabled(true);
		m_pToggleButton_MeasureArea->setDisabled(true);
		
		int cur_frame = m_pSlider_SelectFrame->value();
		int end_frame = (int)m_vectorOctImage.size();

		if (cur_frame + 1 == end_frame)
			cur_frame = 0;

		_running = true;
		playing = std::thread([&, cur_frame, end_frame]() {
			for (int i = cur_frame; i < end_frame; i++)
			{
				m_pSlider_SelectFrame->setValue(i);
				std::this_thread::sleep_for(std::chrono::milliseconds(100));

				if (!_running) break;
			}
			emit playingDone();
		});

		m_pConfig->writeToLog(QString("Playing mode: frame: [%1 ==> %2].").arg(cur_frame).arg(end_frame));
	}
	else
	{
		if (playing.joinable())
		{
			_running = false;
			playing.join();
		}

		m_pToggleButton_Play->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
		m_pToggleButton_MeasureDistance->setDisabled(false);
		m_pToggleButton_MeasureArea->setDisabled(false);

		m_pConfig->writeToLog("Stop playing mode.");
	}
}

void QViewTab::measureDistance(bool toggled)
{
	m_pImageView_CircImage->getRender()->m_bMeasureDistance = toggled;
	m_pImageView_Longi->getRender()->m_bMeasureDistance = toggled;
	if (!toggled)
	{
		m_pImageView_CircImage->setRLineChangeCallback([&](int aline) {
			visualizeLongiImage(aline);
			if (m_pResultTab->getSettingDlg())
				m_pResultTab->getSettingDlg()->getPulseReviewTab()->setCurrentAline(aline / 4);
		});
		m_pImageView_Longi->setVLineChangeCallback([&](int frame) { m_pSlider_SelectFrame->setValue(frame); });

		m_pImageView_CircImage->getRender()->m_nClicked = 0;
		m_pImageView_CircImage->getRender()->m_vecPoint.clear();
		m_pImageView_CircImage->getRender()->update();

		m_pImageView_Longi->getRender()->m_nClicked = 0;
		m_pImageView_Longi->getRender()->m_vecPoint.clear();
		m_pImageView_Longi->getRender()->update();

		m_pConfig->writeToLog("Measure distance off.");
	}
	else
	{
		if (m_pToggleButton_MeasureArea->isChecked()) m_pToggleButton_MeasureArea->setChecked(false);

		m_pImageView_CircImage->setRLineChangeCallback([&](int aline) { (void)aline; });
		m_pImageView_Longi->setVLineChangeCallback([&](int frame) { (void)frame; });
		
		m_pConfig->writeToLog("Measure distance on.");
	}
}

void QViewTab::measureArea(bool toggled)
{
	m_pImageView_CircImage->getRender()->m_bMeasureArea = toggled;
	if (!toggled)
	{
		m_pImageView_CircImage->setRLineChangeCallback([&](int aline) {
			visualizeLongiImage(aline);
			if (m_pResultTab->getSettingDlg())
				m_pResultTab->getSettingDlg()->getPulseReviewTab()->setCurrentAline(aline / 4);
		});

		m_pImageView_CircImage->getRender()->m_nClicked = 0;
		m_pImageView_CircImage->getRender()->m_vecPoint.clear();
		m_pImageView_CircImage->getRender()->update();

		m_pConfig->writeToLog("Measure area on.");
	}
	else
	{
		if (m_pToggleButton_MeasureDistance->isChecked()) m_pToggleButton_MeasureDistance->setChecked(false);

		m_pImageView_CircImage->setRLineChangeCallback([&](int aline) { (void)aline; });

		m_pConfig->writeToLog("Measure area off.");
	}
}

void QViewTab::changeEmissionChannel(int index)
{	
	int ch = index % 3;
	int mode = index / 3;

	m_pConfig->writeToLog(QString("Changed emission channel: [%1 ==> %2].").arg(m_pConfig->flimEmissionChannel).arg(ch + 1));

	m_pConfig->flimEmissionChannel = ch + 1;
	m_pConfig->flimVisualizationMode = mode;
		
	if (m_pResultTab->getSettingDlg())
	{
		if (m_pResultTab->getSettingDlg()->getViewOptionTab())
		{
			if (mode == _LIFETIME_)
				m_pResultTab->getSettingDlg()->getViewOptionTab()->getRadioButtonLifetime()->setChecked(true);
			else if (mode == _INTENSITY_PROP_)
				m_pResultTab->getSettingDlg()->getViewOptionTab()->getRadioButtonIntensityProp()->setChecked(true);
			else if (mode == _INTENSITY_RATIO_)
				m_pResultTab->getSettingDlg()->getViewOptionTab()->getRadioButtonIntensityRatio()->setChecked(true);
			m_pResultTab->getSettingDlg()->getViewOptionTab()->getEmissionChannelComboBox()->setCurrentIndex(ch);
		}
	}
	else
		invalidate();
}

void QViewTab::scaleFLImEnFaceMap(ImageObject* pImgObjIntensityMap, ImageObject* pImgObjLifetimeMap, ImageObject* pImgObjIntensityPropMap, ImageObject* pImgObjIntensityRatioMap, int ch, int mode)
{
	IppiSize roi_flimproj = { m_intensityMap.at(0).size(0), m_intensityMap.at(0).size(1) };
	IppiSize roi_flimproj4 = { roi_flimproj.width, ((roi_flimproj.height + 3) >> 2) << 2 };

	np::Uint8Array2 scale_temp(roi_flimproj4.width, roi_flimproj4.height);

	// Intensity map			
	ippiScale_32f8u_C1R(m_intensityMap.at(ch), sizeof(float) * roi_flimproj.width,
		scale_temp.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.width, roi_flimproj,
		m_pConfig->flimIntensityRange[ch].min, m_pConfig->flimIntensityRange[ch].max);
	ippiTranspose_8u_C1R(scale_temp.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.width,
		pImgObjIntensityMap->arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.height, roi_flimproj4);
	circShift(pImgObjIntensityMap->arr, int(m_pConfigTemp->rotatedAlines / 4));
	setAxialOffset(pImgObjIntensityMap->arr, m_pConfigTemp->interFrameSync);

	if (mode == VisualizationMode::_LIFETIME_)
	{
		// Lifetime map
		ippiScale_32f8u_C1R(m_lifetimeMap.at(ch), sizeof(float) * roi_flimproj.width,
			scale_temp.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.width, roi_flimproj,
			m_pConfig->flimLifetimeRange[ch].min, m_pConfig->flimLifetimeRange[ch].max);
		ippiTranspose_8u_C1R(scale_temp.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.width,
			pImgObjLifetimeMap->arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.height, roi_flimproj);
		circShift(pImgObjLifetimeMap->arr, int(m_pConfigTemp->rotatedAlines / 4));
		setAxialOffset(pImgObjLifetimeMap->arr, m_pConfigTemp->interFrameSync);

		// RGB conversion
		pImgObjIntensityMap->convertRgb();
		pImgObjLifetimeMap->convertRgb();

		// Intensity-weight lifetime map
		ippsMul_8u_ISfs(pImgObjIntensityMap->qrgbimg.bits(), pImgObjLifetimeMap->qrgbimg.bits(), pImgObjIntensityMap->qrgbimg.byteCount(), 8);
	}
	else if (mode == VisualizationMode::_INTENSITY_PROP_)
	{
		// Intensity proportion map
		ippiScale_32f8u_C1R(m_intensityProportionMap.at(ch), sizeof(float) * roi_flimproj.width,
			scale_temp.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.width, roi_flimproj,
			m_pConfig->flimIntensityPropRange[ch].min, m_pConfig->flimIntensityPropRange[ch].max);
		ippiTranspose_8u_C1R(scale_temp.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.width,
			pImgObjIntensityPropMap->arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.height, roi_flimproj4);
		circShift(pImgObjIntensityPropMap->arr, int(m_pConfigTemp->rotatedAlines / 4));
		setAxialOffset(pImgObjIntensityPropMap->arr, m_pConfigTemp->interFrameSync);

		// RGB conversion
		pImgObjIntensityMap->convertRgb();
		pImgObjIntensityPropMap->convertRgb();

		// Intensity-weight intensity proportion map
		ippsMul_8u_ISfs(pImgObjIntensityMap->qrgbimg.bits(), pImgObjIntensityPropMap->qrgbimg.bits(), pImgObjIntensityMap->qrgbimg.byteCount(), 8);
	}
	else if (mode == VisualizationMode::_INTENSITY_RATIO_)
	{
		// Intensity ratio map
		ippiScale_32f8u_C1R(m_intensityRatioMap.at(ch), sizeof(float) * roi_flimproj.width,
			scale_temp.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.width, roi_flimproj,
			m_pConfig->flimIntensityRatioRange[ch].min, m_pConfig->flimIntensityRatioRange[ch].max);
		ippiTranspose_8u_C1R(scale_temp.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.width,
			pImgObjIntensityRatioMap->arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.height, roi_flimproj4);
		circShift(pImgObjIntensityRatioMap->arr, int(m_pConfigTemp->rotatedAlines / 4));
		setAxialOffset(pImgObjIntensityRatioMap->arr, m_pConfigTemp->interFrameSync);

		// RGB conversion
		pImgObjIntensityMap->convertRgb();
		pImgObjIntensityRatioMap->convertRgb();

		// Intensity-weight intensity ratio map
		ippsMul_8u_ISfs(pImgObjIntensityMap->qrgbimg.bits(), pImgObjIntensityRatioMap->qrgbimg.bits(), pImgObjIntensityMap->qrgbimg.byteCount(), 8);
	}
}

void QViewTab::circShift(np::Uint8Array2& image, int shift)
{
	if (shift >= 0)
	{
		np::Uint8Array2 temp(image.size(0), shift);

		ippiCopy_8u_C1R(&image(0, 0), image.size(0), &temp(0, 0), image.size(0), { image.size(0), shift });
		ippiCopy_8u_C1R(&image(0, shift), image.size(0), &image(0, 0), image.size(0), { image.size(0), image.size(1) - shift });
		ippiCopy_8u_C1R(&temp(0, 0), image.size(0), &image(0, image.size(1) - shift), image.size(0), { image.size(0), shift });
	}
}

void QViewTab::setAxialOffset(np::Uint8Array2& image, int offset)
{
	np::Uint8Array2 temp(image.size(0), image.size(1));
	memset(temp, 0, temp.length());
	if (offset > 0)
	{		
		ippiCopy_8u_C1R(&image(0, 0), image.size(0), &temp(offset, 0), temp.size(0), { image.size(0) - offset, image.size(1) });
		memcpy(&image(0, 0), &temp(0, 0), image.length());
	}
	else if (offset < 0)
	{
		ippiCopy_8u_C1R(&image(-offset, 0), image.size(0), &temp(0, 0), temp.size(0), { image.size(0) + offset, image.size(1) });
		memcpy(&image(0, 0), &temp(0, 0), image.length());
	}
}


void QViewTab::getCapture(QByteArray & arr)
{
	// Capture preview
	if (m_pImgObjCircImage)
	{
		QImage capture = m_pImgObjCircImage->qrgbimg.scaled(144, 144, Qt::KeepAspectRatio, Qt::SmoothTransformation);

		QPixmap inPixmap = QPixmap::fromImage(capture);

		QBuffer inBuffer(&arr);
		inBuffer.open(QIODevice::WriteOnly);
		inPixmap.save(&inBuffer, "BMP");

		m_pConfig->writeToLog("Preview image is captured.");
	}
}