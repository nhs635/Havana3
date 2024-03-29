﻿
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
#include <DataAcquisition/DataProcessingDotter.h>

#include <ipps.h>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

#include <mkl_service.h>
#include <mkl_df.h>


QViewTab::QViewTab(bool is_streaming, QWidget *parent) :
	QDialog(parent), m_pStreamTab(nullptr), m_pResultTab(nullptr), m_pConfigTemp(nullptr),
	m_pImgObjRectImage(nullptr), m_pImgObjCircImage(nullptr), m_pImgObjIntensity(nullptr), m_pImgObjLifetime(nullptr),
	m_pImgObjOctProjection(nullptr), m_pImgObjIntensityMap(nullptr), m_pImgObjLifetimeMap(nullptr), 
	m_pImgObjIntensityPropMap(nullptr), m_pImgObjIntensityRatioMap(nullptr), m_pImgObjLongiImage(nullptr), m_pImgObjPlaqueCompositionMap(nullptr), 
	m_pCirc(nullptr), m_pMedfiltRect(nullptr), m_pMedfiltIntensityMap(nullptr), m_pMedfiltLifetimeMap(nullptr), m_pMedfiltLongi(nullptr),
	m_pLumenDetection(nullptr), m_pForest(nullptr), m_pSVM(nullptr), 
	m_pDialog_SetRange(nullptr), m_bRePrediction(true), _running(false)
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
	if (m_pImgObjPlaqueCompositionMap) delete m_pImgObjPlaqueCompositionMap;

    if (m_pImgObjLongiImage) delete m_pImgObjLongiImage;
	
	if (m_pCirc) delete m_pCirc;
	if (m_pMedfiltRect) delete m_pMedfiltRect;
	if (m_pMedfiltIntensityMap) delete m_pMedfiltIntensityMap;
	if (m_pMedfiltLifetimeMap) delete m_pMedfiltLifetimeMap;
    if (m_pMedfiltLongi) delete m_pMedfiltLongi;
	if (m_pForest) delete m_pForest;
	if (m_pSVM) delete m_pSVM;
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
		m_pImageView_CircImage->setMinimumSize(620, 620);

	m_pImageView_CircImage->setSquare(true);
	m_pImageView_CircImage->getRender()->m_dPixelResol = !m_pConfig->is_dotter ? PIXEL_RESOLUTION_SAMSUNG : PIXEL_RESOLUTION_DOTTER;
	m_pImageView_CircImage->setCircle(1, int(OUTER_SHEATH_POSITION / m_pImageView_CircImage->getRender()->m_dPixelResol));

	if (!is_streaming)
	{
		for (int i = 0; i < 4; i++)
		{
			m_pWidget[i] = new QWidget(this);
			m_pWidget[i]->setFixedWidth(274);
			m_pWidget[i]->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
		}
	}

    if (!is_streaming)
    {
        // Create image view : en-face
        ColorTable temp_ctable;
		if (m_pConfig->flimColormapType == FlimColormapType::_HSV_)
	        m_pImageView_EnFace = new QImageView(ColorTable::colortable(LIFETIME_COLORTABLE), 1, m_pConfig->flimAlines, true, this);
		else if (m_pConfig->flimColormapType == FlimColormapType::_TCT_NEW_)
			m_pImageView_EnFace = new QImageView(ColorTable::colortable(NEW_LIFETIME_COLORTABLE + m_pConfig->flimEmissionChannel - 1), 1, m_pConfig->flimAlines, true, this);
		m_pImageView_EnFace->setMinimumSize(250, 160); 
		m_pImageView_EnFace->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
		        
        // Create image view : longitudinal
        m_pImageView_Longi = new QImageView(ColorTable::colortable(OCT_COLORTABLE), 1, diameter, true, this);
		m_pImageView_Longi->getRender()->m_bCenterGrid = true;
		m_pImageView_Longi->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

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

		if (m_pConfig->flimParameterMode == FLImParameters::_LIFETIME_)
		{
			if (m_pConfig->flimColormapType == FlimColormapType::_HSV_)
				m_pImageView_ColorBar = new QImageView(ColorTable::colortable(LIFETIME_COLORTABLE), 4, 256, false, this);
			else if (m_pConfig->flimColormapType == FlimColormapType::_TCT_NEW_)
				m_pImageView_ColorBar = new QImageView(ColorTable::colortable(NEW_LIFETIME_COLORTABLE + m_pConfig->flimEmissionChannel - 1), 4, 256, false, this);
		}
		else if (m_pConfig->flimParameterMode == FLImParameters::_INTENSITY_PROP_)
			m_pImageView_ColorBar = new QImageView(ColorTable::colortable(INTENSITY_PROP_COLORTABLE), 4, 256, false, this);
		else if (m_pConfig->flimParameterMode == FLImParameters::_INTENSITY_RATIO_)
			m_pImageView_ColorBar = new QImageView(ColorTable::colortable(INTENSITY_RATIO_COLORTABLE), 4, 256, false, this);
		else if (m_pConfig->flimParameterMode == FLImParameters::_NONE_)
			m_pImageView_ColorBar = new QImageView(ColorTable::colortable(OCT_COLORTABLE), 4, 256, false, this);
		else
			m_pImageView_ColorBar = new QImageView(ColorTable::colortable(OCT_COLORTABLE), 4, 256, false, this);
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

		m_pPushButton_Pick = new QPushButton(this);
		m_pPushButton_Pick->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
		m_pPushButton_Pick->setFixedSize(40, 25);

		m_pPushButton_Backward = new QPushButton(this);
		m_pPushButton_Backward->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
		m_pPushButton_Backward->setFixedSize(40, 25);

		m_pPushButton_Forward = new QPushButton(this);
		m_pPushButton_Forward->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));
		m_pPushButton_Forward->setFixedSize(40, 25);

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

		m_pPushButton_LumenDetection = new QPushButton(this);		
		m_pPushButton_LumenDetection->setText("Lumen Detection");
		m_pPushButton_LumenDetection->setFixedWidth(120);

		m_pToggleButton_AutoContour = new QPushButton(this);
		m_pToggleButton_AutoContour->setCheckable(true);
		m_pToggleButton_AutoContour->setText("Auto Contour");
		m_pToggleButton_AutoContour->setFixedWidth(120);

		m_pToggleButton_Statistics = new QPushButton(this);
		m_pToggleButton_Statistics->setCheckable(true);
		m_pToggleButton_Statistics->setText("Statistics");
		m_pToggleButton_Statistics->setFixedWidth(120);		

		m_pLabel_Statistics = new QLabel(this);
		m_pLabel_Statistics->setFixedSize(256, 78);
		m_pLabel_Statistics->setStyleSheet("QLabel{font-size:8pt}");

		m_pToggleButton_DiameterView = new QPushButton(this);
		m_pToggleButton_DiameterView->setCheckable(true);
		m_pToggleButton_DiameterView->setText("Diameter View");
		m_pToggleButton_DiameterView->setFixedWidth(120);
		m_pToggleButton_DiameterView->setDisabled(true);

        // Create widgets for FLIm parameters control
        m_pComboBox_FLImParameters = new QComboBox(this);
		m_pComboBox_FLImParameters->addItem("Ch 1 Lifetime");
		m_pComboBox_FLImParameters->addItem("Ch 2 Lifetime");
		m_pComboBox_FLImParameters->addItem("Ch 3 Lifetime");
		m_pComboBox_FLImParameters->addItem("Ch 1 IntProportion");
		m_pComboBox_FLImParameters->addItem("Ch 2 IntProportion");
		m_pComboBox_FLImParameters->addItem("Ch 3 IntProportion");
		m_pComboBox_FLImParameters->addItem("Ch 1/3 IntRatio");
		m_pComboBox_FLImParameters->addItem("Ch 2/1 IntRatio");
		m_pComboBox_FLImParameters->addItem("Ch 3/2 IntRatio");
		m_pComboBox_FLImParameters->addItem("None");
		m_pComboBox_FLImParameters->setCurrentIndex(3 * m_pConfig->flimParameterMode + m_pConfig->flimEmissionChannel - 1);
		m_pComboBox_FLImParameters->setFixedWidth(120);

        m_pRadioButton_FLImParameters = new QRadioButton(this);
		m_pRadioButton_FLImParameters->setText("FLIm Parameters ");
		m_pRadioButton_FLImParameters->setFixedWidth(120);
		m_pRadioButton_FLImParameters->setChecked(true);
		
		// Create widgets for ML prediction control
		m_pComboBox_MLPrediction = new QComboBox(this);
		m_pComboBox_MLPrediction->addItem("Random Forest");
		m_pComboBox_MLPrediction->addItem("SVM Softmax");
		m_pComboBox_MLPrediction->addItem("SVM Logistics");
		m_pComboBox_MLPrediction->setCurrentIndex(m_pConfig->mlPredictionMode);
		m_pComboBox_MLPrediction->setFixedWidth(120);
		m_pComboBox_MLPrediction->setDisabled(true);

		m_pRadioButton_MLPrediction = new QRadioButton(this);
		m_pRadioButton_MLPrediction->setText("ML Prediction ");
		m_pRadioButton_MLPrediction->setFixedWidth(120);

		m_pButtonGroup_VisualizationMode = new QButtonGroup(this);
		m_pButtonGroup_VisualizationMode->addButton(m_pRadioButton_FLImParameters, _FLIM_PARAMETERS_);
		m_pButtonGroup_VisualizationMode->addButton(m_pRadioButton_MLPrediction, _ML_PREDICTION_);
    }

	// Copy label callback
	if (!m_pStreamTab)
	{
		QString pt_name = m_pResultTab->getRecordInfo().patientName;
		QString acq_date = m_pResultTab->getRecordInfo().date;

		auto get_flim_info = [&]() -> QString {
			
			int vis_mode = m_pRadioButton_MLPrediction->isChecked();
			int flim_mode = m_pConfig->flimParameterMode;
			int ch = m_pConfig->flimEmissionChannel;
			int ml_mode = m_pConfig->mlPredictionMode;

			QString flim_type;
			float min = 0, max = 0;
			if (vis_mode == VisualizationMode::_FLIM_PARAMETERS_)
			{
				if (flim_mode == FLImParameters::_LIFETIME_)
				{
					flim_type = QString("Ch%1 Lifetime").arg(ch);
					if (m_pConfig->flimColormapType == FlimColormapType::_HSV_)
					{
						min = m_pConfig->flimLifetimeRange[ch - 1].min;
						max = m_pConfig->flimLifetimeRange[ch - 1].max;
					}
					else
					{
						min = m_pConfig->flimLifetimeRangeNew[ch - 1].min;
						max = m_pConfig->flimLifetimeRangeNew[ch - 1].max;
					}
				}
				else if (flim_mode == FLImParameters::_INTENSITY_PROP_)
				{
					flim_type = QString("Ch%1 IntenProp").arg(ch);
					min = m_pConfig->flimIntensityPropRange[ch - 1].min;
					max = m_pConfig->flimIntensityPropRange[ch - 1].max;
				}
				else if (flim_mode == FLImParameters::_INTENSITY_RATIO_)
				{
					flim_type = QString("Ch%1/%2 IntenRatio").arg(ch).arg((ch + 1) % 3 + 1);
					min = m_pConfig->flimIntensityRatioRange[ch - 1].min;
					max = m_pConfig->flimIntensityRatioRange[ch - 1].max;
				}
			}
			else if (vis_mode == VisualizationMode::_ML_PREDICTION_)
			{
				if (ml_mode == MLPrediction::_RF_COMPO_)
					flim_type = "ML Random Forest";
				else if (ml_mode == MLPrediction::_SVM_SOFTMAX_)
					flim_type = "ML SVM Softmax";
				else if (ml_mode == MLPrediction::_SVM_LOGISTICS_)
					flim_type = "ML SVM Logistics";
			}

			QString flim_info = QString("%1 [%2 %3]").arg(flim_type).arg(min, 2, 'f', 1).arg(max, 2, 'f', 1);
			return flim_info;
		};
				
		m_pImageView_CircImage->DidCopyLabel += [&, pt_name, acq_date, get_flim_info]() {

			int frame = getCurrentFrame() + 1;
			int total_frame = (int)m_vectorOctImage.size();

			QString label = QString("[%1] %2 (%3 %4 / %5)\n%6 (rot: %7)").arg(pt_name).arg(acq_date).arg("circ:").arg(frame).arg(total_frame).arg(get_flim_info()).arg(m_pConfigTemp->rotatedAlines);
			QApplication::clipboard()->setText(label);
		};
		m_pImageView_Longi->DidCopyLabel += [&, pt_name, acq_date, get_flim_info]() {
			
			int aline = getCurrentAline() + 1;
			int total_aline = m_vectorOctImage.at(0).size(1);

			QString label = QString("[%1] %2 (%3 %4 / %5)\n%6 (rot: %7)").arg(pt_name).arg(acq_date).arg("longi:").arg(aline).arg(total_aline).arg(get_flim_info()).arg(m_pConfigTemp->rotatedAlines);
			QApplication::clipboard()->setText(label);
		};
		m_pImageView_EnFace->DidCopyLabel += [&, pt_name, acq_date, get_flim_info]() {
			
			QString label = QString("[%1] %2\n%3 (rot: %4)").arg(pt_name).arg(acq_date).arg(get_flim_info()).arg(m_pConfigTemp->rotatedAlines);;
			QApplication::clipboard()->setText(label);
		};
		m_pImageView_Ivus->DidCopyLabel += [&, pt_name, acq_date]() {

			IvusViewerDlg* pIvusViewerDlg = m_pResultTab->getIvusViewerDlg();
			if (pIvusViewerDlg)
			{
				int k = 0;
				foreach(QStringList _matches, pIvusViewerDlg->m_vectorMatches)
				{
					if (_matches.at(0).toInt() == getCurrentFrame() + 1)
						break;
					k++;
				}

				if (k != pIvusViewerDlg->m_vectorMatches.size())
				{
					QStringList matches = pIvusViewerDlg->m_vectorMatches.at(k);

					int ivus_frame = matches.at(1).toInt() - 1;
					if (ivus_frame != -1)
					{
						int ivus_total_frame = (int)pIvusViewerDlg->m_vectorIvusImages.size();
						int ivus_rotation = matches.at(2).toInt();

						QString label = QString("[%1] %2 (%3 %4 / %5) CCW %6 deg\n(%7)").arg(pt_name).arg(acq_date)
							.arg("ivus:").arg(ivus_frame + 1).arg(ivus_total_frame).arg(ivus_rotation).arg(m_pConfigTemp->ivusPath);
						QApplication::clipboard()->setText(label);
					}
				}
			}
		};
	}

    // Set layout
    QGridLayout *pGridLayout = new QGridLayout;
    pGridLayout->setSpacing(4);
	
    if (!is_streaming)
    {
		QGridLayout *pGridLayout1 = new QGridLayout;
		pGridLayout1->setSpacing(4);

		QLabel *pLabel_Dummy = new QLabel(this);
		pLabel_Dummy->setFixedHeight(25);

		pGridLayout1->addWidget(m_pImageView_Ivus, 0, 0, 1, 4);
		pGridLayout1->addWidget(pLabel_Dummy, 1, 0, 1, 4);
		pGridLayout1->addWidget(m_pPushButton_Backward, 2, 0);
		pGridLayout1->addWidget(m_pPushButton_Pick, 2, 1);
		pGridLayout1->addWidget(m_pPushButton_Forward, 2, 2);
		pGridLayout1->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 2, 4);
		pGridLayout1->addWidget(m_pPushButton_Decrement, 3, 0);
		pGridLayout1->addWidget(m_pToggleButton_Play, 3, 1);
		pGridLayout1->addWidget(m_pPushButton_Increment, 3, 2);
		pGridLayout1->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 2, 4);		

		m_pWidget[1]->setLayout(pGridLayout1);
		
		QGridLayout *pGridLayout2 = new QGridLayout;
		pGridLayout2->setSpacing(2);
		
		pGridLayout2->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 0);
		pGridLayout2->addWidget(m_pToggleButton_MeasureDistance, 0, 1);
		pGridLayout2->addWidget(m_pToggleButton_MeasureArea, 0, 2);
		pGridLayout2->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 0);
		pGridLayout2->addWidget(m_pPushButton_LumenDetection, 1, 1);
		pGridLayout2->addWidget(m_pToggleButton_AutoContour, 1, 2);
		pGridLayout2->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 2, 0);
		pGridLayout2->addWidget(m_pToggleButton_Statistics, 2, 1);
		pGridLayout2->addWidget(m_pToggleButton_DiameterView, 2, 2);
				
		QGridLayout *pGridLayout3 = new QGridLayout;
		pGridLayout3->setSpacing(4);

		pGridLayout3->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 0);
		pGridLayout3->addWidget(m_pRadioButton_FLImParameters, 0, 1);
		pGridLayout3->addWidget(m_pComboBox_FLImParameters, 0, 2);
		pGridLayout3->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 0);
		pGridLayout3->addWidget(m_pRadioButton_MLPrediction, 1, 1);
		pGridLayout3->addWidget(m_pComboBox_MLPrediction, 1, 2);

		QVBoxLayout *pVBoxLayout23 = new QVBoxLayout;
		pVBoxLayout23->setSpacing(4);

		pVBoxLayout23->addWidget(m_pLabel_Statistics);
		pVBoxLayout23->addWidget(m_pImageView_EnFace);
		pVBoxLayout23->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Maximum));
		pVBoxLayout23->addItem(pGridLayout2);
		pVBoxLayout23->addItem(pGridLayout3);

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
		connect(m_pPushButton_Pick, &QPushButton::clicked, [&]() { pickFrame(m_vectorPickFrames, getCurrentFrame() + 1, 0, 0, true); }); 
		connect(m_pPushButton_Forward, &QPushButton::clicked, [&]() { seekPickFrame(true); });
		connect(m_pPushButton_Backward, &QPushButton::clicked, [&]() { seekPickFrame(false); });
		
        connect(m_pSlider_SelectFrame, SIGNAL(valueChanged(int)), this, SLOT(visualizeImage(int)));
        connect(m_pToggleButton_MeasureDistance, SIGNAL(toggled(bool)), this, SLOT(measureDistance(bool)));
		connect(m_pToggleButton_MeasureArea, SIGNAL(toggled(bool)), this, SLOT(measureArea(bool)));
		connect(m_pPushButton_LumenDetection, SIGNAL(clicked(bool)), this, SLOT(lumenContourDetection()));
		connect(m_pToggleButton_AutoContour, SIGNAL(toggled(bool)), this, SLOT(autoContouring(bool)));
		connect(m_pToggleButton_Statistics, SIGNAL(toggled(bool)), this, SLOT(calculateStatistics(bool)));
		connect(m_pToggleButton_DiameterView, SIGNAL(toggled(bool)), this, SLOT(setDiameterView(bool)));

		connect(m_pButtonGroup_VisualizationMode, SIGNAL(buttonClicked(int)), this, SLOT(changeVisualizationMode(int)));
        connect(m_pComboBox_FLImParameters, SIGNAL(currentIndexChanged(int)), this, SLOT(changeEmissionChannel(int)));
		connect(m_pComboBox_MLPrediction, SIGNAL(currentIndexChanged(int)), this, SLOT(changeMLPrediction(int)));

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
			if (m_pConfigTemp)
			{
				m_pImageView_Longi->getRender()->m_nPullbackLength = m_pConfigTemp->pullbackLength;
			}
			else
			{
				// Dotter device
				m_pConfigTemp = m_pResultTab->getDataProcessingDotter()->getConfigTemp();
				m_pImageView_Longi->getRender()->m_nPullbackLength = 55; // Dotter pullback length (fixed)
			}
		}
	}
		
	int vis_mode = m_pRadioButton_MLPrediction->isChecked();
	
	if (vis_mode == VisualizationMode::_FLIM_PARAMETERS_)
	{
		m_pImageView_EnFace->setShadingRegion(m_pConfigTemp->quantitationRange.min + 1, m_pConfigTemp->quantitationRange.max + 1);
		m_pImageView_Longi->setShadingRegion(m_pConfigTemp->quantitationRange.min + 1, m_pConfigTemp->quantitationRange.max + 1);

		if (!m_pDialog_SetRange)
		{
			QString menu_name("Set Range");
			m_pImageView_ColorBar->setCustomContextMenu(menu_name, [&]() {

				if (m_pDialog_SetRange == nullptr)
				{
					m_pDialog_SetRange = new QDialog(this);
					{
						QLineEdit *pLineEdit_Start = new QLineEdit(this);
						pLineEdit_Start->setAlignment(Qt::AlignCenter);
						pLineEdit_Start->setFixedWidth(40);
						pLineEdit_Start->setText(QString::number(m_pConfigTemp->quantitationRange.min + 1));

						QLineEdit *pLineEdit_End = new QLineEdit(this);
						pLineEdit_End->setAlignment(Qt::AlignCenter);
						pLineEdit_End->setFixedWidth(40);
						pLineEdit_End->setText(QString::number(m_pConfigTemp->quantitationRange.max + 1));

						QPushButton *pPushButton_Apply = new QPushButton(this);
						pPushButton_Apply->setText("Apply");
						pPushButton_Apply->setFixedWidth(55);

						QPushButton *pPushButton_Pick = new QPushButton(this);
						pPushButton_Pick->setText("Pick");
						pPushButton_Pick->setFixedWidth(55);

						QGridLayout *pGridLayout = new QGridLayout;
						pGridLayout->addWidget(new QLabel("Start", this), 0, 0);
						pGridLayout->addWidget(pLineEdit_Start, 0, 1);
						pGridLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 2);
						pGridLayout->addWidget(pPushButton_Pick, 0, 3);

						pGridLayout->addWidget(new QLabel("End", this), 1, 0);
						pGridLayout->addWidget(pLineEdit_End, 1, 1);
						pGridLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 2);
						pGridLayout->addWidget(pPushButton_Apply, 1, 3);

						m_pDialog_SetRange->setLayout(pGridLayout);

						// Connect
						connect(pLineEdit_Start, &QLineEdit::editingFinished, [&, pLineEdit_Start]()
						{
							int val = pLineEdit_Start->text().toInt() - 1;
							if (val < 1) val = 0;
							if (val > m_pConfigTemp->quantitationRange.max) val = m_pConfigTemp->quantitationRange.max;
							m_pConfigTemp->quantitationRange.min = val;
							if (val + 1 != pLineEdit_Start->text().toInt())
								pLineEdit_Start->setText(QString::number(val + 1));

							m_pImageView_EnFace->setShadingRegion(m_pConfigTemp->quantitationRange.min + 1, m_pConfigTemp->quantitationRange.max + 1);
							m_pImageView_EnFace->update();
							m_pImageView_Longi->setShadingRegion(m_pConfigTemp->quantitationRange.min + 1, m_pConfigTemp->quantitationRange.max + 1);
							m_pImageView_Longi->update();
						});
						connect(pLineEdit_End, &QLineEdit::editingFinished, [&, pLineEdit_End]()
						{
							int val = pLineEdit_End->text().toInt() - 1;
							if (val > m_pConfigTemp->frames) val = m_pConfigTemp->frames - 1;
							if (val < m_pConfigTemp->quantitationRange.min) val = m_pConfigTemp->quantitationRange.min;
							m_pConfigTemp->quantitationRange.max = val;
							if (val + 1 != pLineEdit_End->text().toInt())
								pLineEdit_End->setText(QString::number(val + 1));

							m_pImageView_EnFace->setShadingRegion(m_pConfigTemp->quantitationRange.min + 1, m_pConfigTemp->quantitationRange.max + 1);
							m_pImageView_EnFace->update();
							m_pImageView_Longi->setShadingRegion(m_pConfigTemp->quantitationRange.min + 1, m_pConfigTemp->quantitationRange.max + 1);
							m_pImageView_Longi->update();
						});

						connect(pPushButton_Pick, &QPushButton::clicked, [&, pLineEdit_Start, pLineEdit_End]()
						{
							static int which = 0;
							if (!(which++ % 2))
							{
								pLineEdit_Start->setText(QString::number(getCurrentFrame() + 1));
								emit pLineEdit_Start->editingFinished();
							}
							else
							{
								pLineEdit_End->setText(QString::number(getCurrentFrame() + 1));
								emit pLineEdit_End->editingFinished();
							}
						});
						connect(pPushButton_Apply, &QPushButton::clicked, [&]()
						{
							invalidate();
						});
						connect(m_pDialog_SetRange, &QDialog::finished, [&]()
						{
							m_pDialog_SetRange->deleteLater();
							m_pDialog_SetRange = nullptr;
						});
					}
					m_pDialog_SetRange->setWindowTitle("Set Range");
					m_pDialog_SetRange->setFixedSize(170, 75);
					m_pDialog_SetRange->show();
				}
				m_pDialog_SetRange->raise();
				m_pDialog_SetRange->activateWindow();
			});
		}

		disconnect(m_pComboBox_FLImParameters, SIGNAL(currentIndexChanged(int)), 0, 0);
		m_pComboBox_FLImParameters->setCurrentIndex(3 * m_pConfig->flimParameterMode + m_pConfig->flimEmissionChannel - 1);
		connect(m_pComboBox_FLImParameters, SIGNAL(currentIndexChanged(int)), this, SLOT(changeEmissionChannel(int)));

		m_pImageView_EnFace->resetSize(m_pConfigTemp->frames, m_pConfigTemp->flimAlines);
		if (m_pConfig->flimParameterMode == FLImParameters::_LIFETIME_)
		{
			m_pImageView_EnFace->setEnterCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 4),
				QString::fromLocal8Bit("2-D FLIm En Face Chemogram\n- Ch %1 Lifetime (z-θ)").arg(m_pConfig->flimEmissionChannel)); });
			
			if (m_pConfig->flimColormapType == FlimColormapType::_HSV_)
			{
				m_pImageView_ColorBar->setText(QPoint(0, 0), QString("%1            Lifetime (nsec)            %2")
					.arg(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max, 2, 'f', 1)
					.arg(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min, 2, 'f', 1), true);
				m_pImageView_ColorBar->resetColormap(ColorTable::colortable(LIFETIME_COLORTABLE));
			}
			else if (m_pConfig->flimColormapType == FlimColormapType::_TCT_NEW_)
			{
				m_pImageView_ColorBar->setText(QPoint(0, 0), QString("%1            Lifetime (nsec)            %2")
					.arg(m_pConfig->flimLifetimeRangeNew[m_pConfig->flimEmissionChannel - 1].max, 2, 'f', 1)
					.arg(m_pConfig->flimLifetimeRangeNew[m_pConfig->flimEmissionChannel - 1].min, 2, 'f', 1), true);
				m_pImageView_ColorBar->resetColormap(ColorTable::colortable(NEW_LIFETIME_COLORTABLE + m_pConfig->flimEmissionChannel - 1));
			}
		}
		else if (m_pConfig->flimParameterMode == FLImParameters::_INTENSITY_PROP_)
		{
			m_pImageView_EnFace->setEnterCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 4),
				QString::fromLocal8Bit("2-D FLIm En Face Chemogram\n- Ch %1 Intensity Proportion (z-θ)").arg(m_pConfig->flimEmissionChannel)); });

			m_pImageView_ColorBar->setText(QPoint(0, 0), QString("%1    Intensity Proportion (AU)    %2")
				.arg(m_pConfig->flimIntensityPropRange[m_pConfig->flimEmissionChannel - 1].max, 2, 'f', 1)
				.arg(m_pConfig->flimIntensityPropRange[m_pConfig->flimEmissionChannel - 1].min, 2, 'f', 1), true);
			m_pImageView_ColorBar->resetColormap(ColorTable::colortable(INTENSITY_PROP_COLORTABLE));
		}
		else if (m_pConfig->flimParameterMode == FLImParameters::_INTENSITY_RATIO_)
		{
			m_pImageView_EnFace->setEnterCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 4),
				QString::fromLocal8Bit("2-D FLIm En Face Chemogram\n- Ch %1/%2 Intensity Ratio (z-θ)").arg(m_pConfig->flimEmissionChannel)
				.arg((m_pConfig->flimEmissionChannel == 1) ? 3 : m_pConfig->flimEmissionChannel - 1)); });

			m_pImageView_ColorBar->setText(QPoint(0, 0), QString("%1       Intensity Ratio (AU)       %2")
				.arg(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1].max, 2, 'f', 1)
				.arg(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1].min, 2, 'f', 1), true);
			m_pImageView_ColorBar->resetColormap(ColorTable::colortable(INTENSITY_RATIO_COLORTABLE));
		}
		else if (m_pConfig->flimParameterMode == FLImParameters::_NONE_)
		{
			m_pImageView_EnFace->resetSize(m_pConfigTemp->frames, m_pConfigTemp->octAlines);
			m_pImageView_EnFace->setEnterCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 4),
				QString::fromLocal8Bit("2-D OCT Max Projection (z-θ)")); });

			m_pImageView_ColorBar->setText(QPoint(0, 0), "", true);
			m_pImageView_ColorBar->resetColormap(ColorTable::colortable(OCT_COLORTABLE));
		}
		m_pImageView_ColorBar->DidCopyLabel.clear();
	}
	else if (vis_mode == VisualizationMode::_ML_PREDICTION_)
	{
		m_pImageView_EnFace->setShadingRegion(m_pConfigTemp->quantitationRange.min + 1, m_pConfigTemp->quantitationRange.max + 1);	
		m_pImageView_Longi->setShadingRegion(m_pConfigTemp->quantitationRange.min + 1, m_pConfigTemp->quantitationRange.max + 1);

		if (!m_pDialog_SetRange)
		{
            QString menu_name("Set Range");
            m_pImageView_ColorBar->setCustomContextMenu(menu_name, [&]() {

				if (m_pDialog_SetRange == nullptr)
				{
					m_pDialog_SetRange = new QDialog(this);
					{
						QLineEdit *pLineEdit_Start = new QLineEdit(this);
						pLineEdit_Start->setAlignment(Qt::AlignCenter);
						pLineEdit_Start->setFixedWidth(40);
						pLineEdit_Start->setText(QString::number(m_pConfigTemp->quantitationRange.min + 1));

						QLineEdit *pLineEdit_End = new QLineEdit(this);
						pLineEdit_End->setAlignment(Qt::AlignCenter);
						pLineEdit_End->setFixedWidth(40);
						pLineEdit_End->setText(QString::number(m_pConfigTemp->quantitationRange.max + 1));

						QPushButton *pPushButton_Apply = new QPushButton(this);
						pPushButton_Apply->setText("Apply");
						pPushButton_Apply->setFixedWidth(55);
						
						QPushButton *pPushButton_Pick = new QPushButton(this);
						pPushButton_Pick->setText("Pick");
						pPushButton_Pick->setFixedWidth(55);

						QGridLayout *pGridLayout = new QGridLayout;
						pGridLayout->addWidget(new QLabel("Start", this), 0, 0);
						pGridLayout->addWidget(pLineEdit_Start, 0, 1);
						pGridLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 2);
						pGridLayout->addWidget(pPushButton_Pick, 0, 3);

						pGridLayout->addWidget(new QLabel("End", this), 1, 0);
						pGridLayout->addWidget(pLineEdit_End, 1, 1);
						pGridLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 2);
						pGridLayout->addWidget(pPushButton_Apply, 1, 3);

						m_pDialog_SetRange->setLayout(pGridLayout);

						// Connect
						connect(pLineEdit_Start, &QLineEdit::editingFinished, [&, pLineEdit_Start]()
						{
							int val = pLineEdit_Start->text().toInt() - 1;
							if (val < 1) val = 0;
							if (val > m_pConfigTemp->quantitationRange.max) val = m_pConfigTemp->quantitationRange.max;
							m_pConfigTemp->quantitationRange.min = val;
							if (val + 1 != pLineEdit_Start->text().toInt())
								pLineEdit_Start->setText(QString::number(val + 1));

							m_pImageView_EnFace->setShadingRegion(m_pConfigTemp->quantitationRange.min + 1, m_pConfigTemp->quantitationRange.max + 1);
							m_pImageView_EnFace->update();
							m_pImageView_Longi->setShadingRegion(m_pConfigTemp->quantitationRange.min + 1, m_pConfigTemp->quantitationRange.max + 1);
							m_pImageView_Longi->update();
						});
						connect(pLineEdit_End, &QLineEdit::editingFinished, [&, pLineEdit_End]()
						{
							int val = pLineEdit_End->text().toInt() - 1;
							if (val > m_pConfigTemp->frames) val = m_pConfigTemp->frames - 1;
							if (val < m_pConfigTemp->quantitationRange.min) val = m_pConfigTemp->quantitationRange.min;
							m_pConfigTemp->quantitationRange.max = val;
							if (val + 1 != pLineEdit_End->text().toInt())
								pLineEdit_End->setText(QString::number(val + 1));

							m_pImageView_EnFace->setShadingRegion(m_pConfigTemp->quantitationRange.min + 1, m_pConfigTemp->quantitationRange.max + 1);
							m_pImageView_EnFace->update();
							m_pImageView_Longi->setShadingRegion(m_pConfigTemp->quantitationRange.min + 1, m_pConfigTemp->quantitationRange.max + 1);
							m_pImageView_Longi->update();
						});

						connect(pPushButton_Pick, &QPushButton::clicked, [&, pLineEdit_Start, pLineEdit_End]() 
						{ 
							static int which = 0;
							if (!(which++ % 2))
							{
								pLineEdit_Start->setText(QString::number(getCurrentFrame() + 1));
								emit pLineEdit_Start->editingFinished();
							}
							else
							{
								pLineEdit_End->setText(QString::number(getCurrentFrame() + 1));
								emit pLineEdit_End->editingFinished();
							}
						});
						connect(pPushButton_Apply, &QPushButton::clicked, [&]() 
						{ 
							invalidate(); 						
						});
						connect(m_pDialog_SetRange, &QDialog::finished, [&]()
						{
							m_pDialog_SetRange->deleteLater();
							m_pDialog_SetRange = nullptr;
						});
					}
					m_pDialog_SetRange->setWindowTitle("Set Range");
					m_pDialog_SetRange->setFixedSize(170, 75);
					m_pDialog_SetRange->show();
				}
				m_pDialog_SetRange->raise();
				m_pDialog_SetRange->activateWindow();
			});
		}

		int ml_mode = m_pConfig->mlPredictionMode;
		if (ml_mode == MLPrediction::_RF_COMPO_)
		{
			// Random Forest definition
			if (!m_pForest)
			{
				m_pForest = new RandomForest();
				m_pForest->createForest(RF_N_TREES, ML_N_FEATURES, ML_N_CATS, CLASSIFICATION); // Create forest model for classification
				
				if (!m_pForest->load(RF_COMPO_MODEL_NAME)) // Load pre-trained model
				{
					QMessageBox msg_box(QMessageBox::NoIcon, "RF Model Training...", "", QMessageBox::NoButton, this);
					msg_box.setStandardButtons(0);
					msg_box.setWindowModality(Qt::WindowModal);
					msg_box.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
					msg_box.move(QApplication::desktop()->screen()->rect().center() - msg_box.rect().center());
					msg_box.setFixedSize(msg_box.width(), msg_box.height());
					msg_box.show();

					if (m_pForest->train(ML_COMPO_DATASET_NAME)) // If not, new training is initiated.
						m_pForest->save(RF_COMPO_MODEL_NAME); // Then, the trained model is written.
					else
					{
						delete m_pForest; // If failed to train, forest object is deleted.
						m_pForest = nullptr; // and pointed to null.
					}
				}
			}

			// RF prediction: Plaque composition classification
			if (m_pForest)
			{
				m_pForest->setColormap(ML_N_CATS,
					m_pConfigTemp->showPlaqueComposition[0] ? ML_NORMAL_COLOR : 0,
					m_pConfigTemp->showPlaqueComposition[1] ? (!m_pConfigTemp->mergeNorFib ? ML_FIBROUS_COLOR : ML_NORMAL_COLOR) : 0,
					m_pConfigTemp->showPlaqueComposition[2] ? ML_LOOSE_FIBROUS_COLOR : 0,
					m_pConfigTemp->showPlaqueComposition[3] ? ML_CALCIFICATION_COLOR : 0,
					m_pConfigTemp->showPlaqueComposition[4] ? ML_MACROPHAGE_COLOR : 0,
					m_pConfigTemp->showPlaqueComposition[5] ? (!m_pConfigTemp->mergeMacTcfa ? ML_LIPID_MAC_COLOR : ML_MACROPHAGE_COLOR) : 0,
					m_pConfigTemp->showPlaqueComposition[6] ? ML_SHEATH_COLOR : 0);

				if ((m_plaqueCompositionMap.at(0).length() == 0) || m_bRePrediction) // Prediction is only made when the buffer is empty or when the flag is true
				{
					QMessageBox msg_box(QMessageBox::NoIcon, "RF Model Prediction...", "", QMessageBox::NoButton, this);
					msg_box.setStandardButtons(0);
					msg_box.setWindowModality(Qt::WindowModal);
					msg_box.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
					msg_box.move(QApplication::desktop()->screen()->rect().center() - msg_box.rect().center());
					msg_box.setFixedSize(msg_box.width(), msg_box.height());
					msg_box.show();

					// Make prediction		
					m_plaqueCompositionProbMap.at(0) = np::FloatArray2(ML_N_CATS * m_pConfigTemp->flimAlines, m_pConfigTemp->frames);
					m_plaqueCompositionMap.at(0) = np::FloatArray2(3 * m_pConfigTemp->flimAlines, m_pConfigTemp->frames);
					m_pForest->predict(m_featVectors, m_plaqueCompositionProbMap.at(0)); // RF prediction for plaque composition classification		

					m_bRePrediction = false;
				}

				m_pForest->pseudocolor(m_plaqueCompositionProbMap.at(0), m_plaqueCompositionMap.at(0)); //m_plaqueCompositionMap.at(0), 
			}
		}
		else
		{
			// Support Vector Machine definition
			if (!m_pSVM)
			{
				m_pSVM = new SupportVectorMachine();
				m_pSVM->createMachine(ML_N_FEATURES, ML_N_CATS);			

				if (!m_pSVM->load(SVM_COMPO_MODEL_NAME))
				{
					QMessageBox msg_box(QMessageBox::NoIcon, "SVM Model Training...", "", QMessageBox::NoButton, this);
					msg_box.setStandardButtons(0);
					msg_box.setWindowModality(Qt::WindowModal);
					msg_box.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
					msg_box.move(QApplication::desktop()->screen()->rect().center() - msg_box.rect().center());
					msg_box.setFixedSize(msg_box.width(), msg_box.height());
					msg_box.show();

					if (m_pSVM->train(ML_COMPO_DATASET_NAME)) // If not, new training is initiated.
						m_pSVM->save(SVM_COMPO_MODEL_NAME); // Then, the trained model is written.
					else
					{
						delete m_pSVM; // If failed to train, forest object is deleted.
						m_pSVM = nullptr; // and pointed to null.
					}
				}
			}

			// SVM prediction: Plaque composition classification
			if (m_pSVM)
			{
				m_pSVM->setColormap(ML_N_CATS, 
					m_pConfigTemp->showPlaqueComposition[0] ? ML_NORMAL_COLOR : 0, 
					m_pConfigTemp->showPlaqueComposition[1] ? (!m_pConfigTemp->mergeNorFib ? ML_FIBROUS_COLOR : ML_NORMAL_COLOR) : 0,
					m_pConfigTemp->showPlaqueComposition[2] ? ML_LOOSE_FIBROUS_COLOR : 0,
					m_pConfigTemp->showPlaqueComposition[3] ? ML_CALCIFICATION_COLOR : 0,
					m_pConfigTemp->showPlaqueComposition[4] ? ML_MACROPHAGE_COLOR : 0,
					m_pConfigTemp->showPlaqueComposition[5] ? (!m_pConfigTemp->mergeMacTcfa ? ML_LIPID_MAC_COLOR : ML_MACROPHAGE_COLOR) : 0,
					m_pConfigTemp->showPlaqueComposition[6] ? ML_SHEATH_COLOR : 0);
				
				if ((m_plaqueCompositionMap.at(1).length() == 0) || m_bRePrediction) // Prediction is only made when the buffer is empty or when the flag is true
				{
					QMessageBox msg_box(QMessageBox::NoIcon, "SVM Model Prediction...", "", QMessageBox::NoButton, this);
					msg_box.setStandardButtons(0);
					msg_box.setWindowModality(Qt::WindowModal);
					msg_box.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
					msg_box.move(QApplication::desktop()->screen()->rect().center() - msg_box.rect().center());
					msg_box.setFixedSize(msg_box.width(), msg_box.height());
					msg_box.show();

					// Make prediction	
					for (int c = 1; c < 3; c++)
					{
						m_plaqueCompositionProbMap.at(c) = np::FloatArray2(ML_N_CATS * m_pConfigTemp->flimAlines, m_pConfigTemp->frames);
						m_plaqueCompositionMap.at(c) = np::FloatArray2(3 * m_pConfigTemp->flimAlines, m_pConfigTemp->frames);
					}
					m_pSVM->predict(m_featVectors, m_plaqueCompositionProbMap.at(1), m_plaqueCompositionProbMap.at(2)); // SVM prediction for plaque composition classification	
					
					m_bRePrediction = false;
				}

				m_pSVM->pseudocolor(m_plaqueCompositionProbMap.at(1), m_plaqueCompositionMap.at(1),
					m_plaqueCompositionProbMap.at(2), m_plaqueCompositionMap.at(2), m_pConfigTemp->normalizeLogistics);
			}
		}

		// Set default gray map
		{
			IppiSize roi_flimproj = { m_intensityMap.at(0).size(0), m_intensityMap.at(0).size(1) };
			IppiSize roi_flimproj4 = { roi_flimproj.width, ((roi_flimproj.height + 3) >> 2) << 2 };

			np::FloatArray2 shift_temp(roi_flimproj4.width, roi_flimproj4.height);
			np::Uint8Array2 scale_temp(roi_flimproj4.width, roi_flimproj4.height);
			np::Uint8Array2 trans_temp(roi_flimproj4.height, roi_flimproj4.width);

			bool isVibCorrted = m_pResultTab->getVibCorrectionButton()->isChecked();
			int delaySync = !isVibCorrted ? m_pConfigTemp->flimDelaySync : 0;
						
			makeDelay(m_intensityMap.at(1), shift_temp, delaySync);
			ippiScale_32f8u_C1R(shift_temp, sizeof(float) * roi_flimproj.width,
				scale_temp, sizeof(uint8_t) * roi_flimproj4.width, roi_flimproj,
				m_pConfig->flimIntensityRange[1].min, m_pConfig->flimIntensityRange[1].max);
			ippiTranspose_8u_C1R(scale_temp.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.width,
				trans_temp.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.height, roi_flimproj4);
			circShift(trans_temp, int(m_pConfigTemp->rotatedAlines / 4));			
			ippiTranspose_8u_C1R(trans_temp.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.height,
				scale_temp.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.width, { roi_flimproj4.height, roi_flimproj4.width });
			ippsConvert_8u32f(scale_temp.raw_ptr(), m_grayMap.raw_ptr(), m_grayMap.length());
			ippsDivC_32f_I(255.0f, m_grayMap.raw_ptr(), m_grayMap.length());
		}
		
		// Calculate composition ratio 		
		m_plaqueCompositionRatio.at(ml_mode) = np::FloatArray(ML_N_CATS + 1);
		m_plaqueCompositionRatio.at(ml_mode)(0) = 0;
		{
			// Aggregated intensity map for weighting
			int range_length = m_pConfigTemp->quantitationRange.max - m_pConfigTemp->quantitationRange.min + 1;
			np::FloatArray overallIntensityVector(m_pConfigTemp->flimAlines * range_length);
			memset(overallIntensityVector, 0, sizeof(float) * overallIntensityVector.length());
			ippsAdd_32f_I(&m_grayMap.at(0, m_pConfigTemp->quantitationRange.min), overallIntensityVector, overallIntensityVector.length());

			Ipp32f denumerator;
			ippsSum_32f(overallIntensityVector, overallIntensityVector.length(), &denumerator, ippAlgHintAccurate);

			np::FloatArray2 plaqueCompositionProbVector(&m_plaqueCompositionProbMap.at(ml_mode)(0, m_pConfigTemp->quantitationRange.min), ML_N_CATS, m_pConfigTemp->flimAlines * range_length);
			for (int i = 0; i < ML_N_CATS; i++)
			{
				np::FloatArray weightMulVector(m_pConfigTemp->flimAlines * range_length);
				ippiMul_32f_C1R(&plaqueCompositionProbVector(i, 0), sizeof(float) * ML_N_CATS, 
					overallIntensityVector, sizeof(float) * 1, weightMulVector, sizeof(float) * 1,
					{ 1, weightMulVector.length() });
							
				Ipp32f numerator;
				ippsSum_32f(weightMulVector, weightMulVector.length(), &numerator, ippAlgHintAccurate);
							
				m_plaqueCompositionRatio.at(ml_mode)(i + 1) = m_plaqueCompositionRatio.at(ml_mode)(i) + 255.0f * numerator / denumerator;
				//printf("%f ", m_plaqueCompositionRatio(i + 1));
			}
			//printf("\n");
		}			

		// Colorbar
		m_pImageView_EnFace->setEnterCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 4),
			QString::fromLocal8Bit("2-D Plaque Composition\nEn Face Chemogram (z-θ)")); });
		m_pImageView_ColorBar->setText(QPoint(0, 0), "", true);
		
		uint32_t colors[ML_N_CATS + 1] = { ML_NORMAL_COLOR, ML_FIBROUS_COLOR, ML_LOOSE_FIBROUS_COLOR, 
			ML_CALCIFICATION_COLOR, ML_MACROPHAGE_COLOR, ML_LIPID_MAC_COLOR, ML_SHEATH_COLOR, 0x343434 }; // 343434 for logistics
		
		if (m_pConfigTemp->mergeNorFib)
			colors[1] = colors[0];
		if (m_pConfigTemp->mergeMacTcfa)
			colors[5] = colors[4];
		for (int i = 0; i < ML_N_CATS; i++)
			if (!m_pConfigTemp->showPlaqueComposition[i])
				colors[i] = 0x343434;

		np::Uint8Array2 compo_color(3, ML_N_CATS + 1);
		for (int i = 0; i < ML_N_CATS + 1; i++)
			for (int j = 0; j < 3; j++)
				compo_color(j, i) = (colors[i] >> (8 * (2 - j))) & 0xff;
		
		QVector<QRgb> temp_vector;
		float sum = m_plaqueCompositionRatio.at(ml_mode).at(ML_N_CATS) / 255.0f * 100.0f;
		float comp = sum > 100.0f ? 100.0f / sum : 1.0f;
		for (int i = 0; i < 256; i++)
		{
			int j;
			for (j = 0; j < ML_N_CATS; j++)
				if (((float)i >= m_plaqueCompositionRatio.at(ml_mode)(j)*comp) && ((float)i < m_plaqueCompositionRatio.at(ml_mode)(j + 1)*comp))
					break;

			QRgb color = qRgb(compo_color(0, j), compo_color(1, j), compo_color(2, j));
			temp_vector.push_back(color);
		}			
		m_pImageView_ColorBar->changeColormap(ColorTable::colortable(COMPOSITION_COLORTABLE), temp_vector);
		m_pImageView_ColorBar->resetColormap(ColorTable::colortable(COMPOSITION_COLORTABLE));

		if (m_pConfigTemp->mergeMacTcfa)
			m_plaqueCompositionRatio.at(ml_mode)(5) = m_plaqueCompositionRatio.at(ml_mode)(6);

		QString qval("");		
		for (int i = 0; i < ML_N_CATS; i++)
			qval += QString("%1%\n").arg(sum * (m_plaqueCompositionRatio.at(ml_mode)(i + 1) - m_plaqueCompositionRatio.at(ml_mode)(i)) / 255.0f, 5, 'f', 2);
		
		m_pImageView_ColorBar->setToolTip(qval);

		QString pt_name = m_pResultTab->getRecordInfo().patientName;
		QString acq_date = m_pResultTab->getRecordInfo().date;

		QString flim_type;
		if (ml_mode == MLPrediction::_RF_COMPO_)
			flim_type = "ML Random Forest";
		else if (ml_mode == MLPrediction::_SVM_SOFTMAX_)
			flim_type = "ML SVM Softmax";
		else if (ml_mode == MLPrediction::_SVM_LOGISTICS_)
			flim_type = "ML SVM Logistics";

		m_pImageView_ColorBar->DidCopyLabel += [&, pt_name, acq_date, flim_type, qval]() {
			QString qval0 = qval;
			qval0.replace("\n", "\t");
			QString label = QString("[%1] %2 %3 range [%4 %5]\n%6").arg(pt_name).arg(acq_date).arg(flim_type)
				.arg(m_pConfigTemp->quantitationRange.min).arg(m_pConfigTemp->quantitationRange.max).arg(qval0);
			QApplication::clipboard()->setText(label);
		};	
	}
	m_pImageView_ColorBar->update();
	
	// Statistcs initialization
	m_peakLifetime = np::FloatArray2();
	m_avgLifetime = np::FloatArray2();

	// Visualization
	visualizeEnFaceMap(true);
	visualizeImage(getCurrentFrame());
	visualizeLongiImage(getCurrentAline());
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
	if (m_pConfig->flimColormapType == FlimColormapType::_HSV_)
		m_pImgObjLifetime = new ImageObject(m_pConfig->flimAlines, 1, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE));
	else if (m_pConfig->flimColormapType == FlimColormapType::_TCT_NEW_)
		m_pImgObjLifetime = new ImageObject(m_pConfig->flimAlines, 1, temp_ctable.m_colorTableVector.at(NEW_LIFETIME_COLORTABLE + m_pConfig->flimEmissionChannel - 1));

	if (m_pCirc) delete m_pCirc;
	m_pCirc = new circularize(diameter / 2, m_pConfig->octAlines, false);
	if (m_pMedfiltRect) delete m_pMedfiltRect;
	m_pMedfiltRect = new medfilt(diameter / 2, m_pConfig->octAlines, 3, 3);

	m_pConfig->writeToLog("Streaming buffers and objects are initialized.");
}

void QViewTab::setBuffers(Configuration* pConfig)
{
#ifndef NEXT_GEN_SYSTEM
	int diameter = 2 * pConfig->octRadius;
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
	clear_vector.swap(m_pulsepowerMap); }
	{std::vector<np::FloatArray2> clear_vector;
	clear_vector.swap(m_meandelayMap); }

	{std::vector<np::FloatArray2> clear_vector;
	clear_vector.swap(m_intensityMap); }
	{std::vector<np::FloatArray2> clear_vector;
	clear_vector.swap(m_lifetimeMap); }
	{std::vector<np::FloatArray2> clear_vector;
	clear_vector.swap(m_intensityProportionMap); }
	{std::vector<np::FloatArray2> clear_vector;
	clear_vector.swap(m_intensityRatioMap); }

	{std::vector<np::FloatArray2> clear_vector;
	clear_vector.swap(m_plaqueCompositionProbMap); }
	{std::vector<np::FloatArray2> clear_vector;
	clear_vector.swap(m_plaqueCompositionMap); }
	{std::vector<np::FloatArray> clear_vector;
	clear_vector.swap(m_plaqueCompositionRatio); }

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

	m_grayMap = np::FloatArray2(pConfig->flimAlines, pConfig->frames);
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
		np::FloatArray2 pulsepower = np::FloatArray2(pConfig->flimAlines, pConfig->frames);		
		np::FloatArray2 meandelay = np::FloatArray2(pConfig->flimAlines, pConfig->frames);

		m_pulsepowerMap.push_back(pulsepower);
		m_meandelayMap.push_back(meandelay);
	}

	m_featVectors = np::FloatArray2(9, pConfig->flimAlines * pConfig->frames);

	for (int i = 0; i < 3; i++)
	{
		np::FloatArray2 compo_prob_map; //= np::FloatArray2(ML_N_CATS * pConfig->flimAlines, pConfig->frames);
		np::FloatArray2 compo_map; //= np::FloatArray2(3 * pConfig->flimAlines, pConfig->frames);
		np::FloatArray ratio; //= np::FloatArray(ML_N_CATS + 1);

		m_plaqueCompositionProbMap.push_back(compo_prob_map);
		m_plaqueCompositionMap.push_back(compo_map);
		m_plaqueCompositionRatio.push_back(ratio);
	}

	m_vibCorrIdx = np::Uint16Array((int)m_vectorOctImage.size());
	memset(m_vibCorrIdx, 0, sizeof(uint16_t) * m_vibCorrIdx.length());

	m_pConfig->writeToLog("Reviewing buffers are initialized.");
}

void QViewTab::setObjects(Configuration* pConfig)
{
	// Non-4's multiple width
	int frames4 = ROUND_UP_4S(pConfig->frames);

#ifndef NEXT_GEN_SYSTEM
	int diameter = 2 * pConfig->octRadius;
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
	if (m_pConfig->flimColormapType == FlimColormapType::_HSV_)
		m_pImgObjLifetimeMap = new ImageObject(frames4, pConfig->flimAlines, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE));
	else if (m_pConfig->flimColormapType == FlimColormapType::_TCT_NEW_)
		m_pImgObjLifetimeMap = new ImageObject(frames4, pConfig->flimAlines, temp_ctable.m_colorTableVector.at(NEW_LIFETIME_COLORTABLE + m_pConfig->flimEmissionChannel - 1));

	if (m_pImgObjIntensityPropMap) delete m_pImgObjIntensityPropMap;
	m_pImgObjIntensityPropMap = new ImageObject(frames4, pConfig->flimAlines, temp_ctable.m_colorTableVector.at(INTENSITY_PROP_COLORTABLE));
	if (m_pImgObjIntensityRatioMap) delete m_pImgObjIntensityRatioMap;
	m_pImgObjIntensityRatioMap = new ImageObject(frames4, pConfig->flimAlines, temp_ctable.m_colorTableVector.at(INTENSITY_RATIO_COLORTABLE));
	if (m_pImgObjPlaqueCompositionMap) delete m_pImgObjPlaqueCompositionMap;
	m_pImgObjPlaqueCompositionMap = new ImageObject(frames4, pConfig->flimAlines, temp_ctable.m_colorTableVector.at(COMPOSITION_COLORTABLE));

	// Longitudinal image visualization buffers
	if (m_pImgObjLongiImage) delete m_pImgObjLongiImage;
	m_pImgObjLongiImage = new ImageObject(frames4, diameter, temp_ctable.m_colorTableVector.at(OCT_COLORTABLE));
	
	// Circ & Medfilt objects
	if (m_pCirc) delete m_pCirc;
	m_pCirc = new circularize(diameter / 2, pConfig->octAlines, false);

	if (m_pMedfiltRect) delete m_pMedfiltRect;
	m_pMedfiltRect = new medfilt(diameter / 2, pConfig->octAlines, 3, 3);
	if (m_pMedfiltIntensityMap) delete m_pMedfiltIntensityMap;
	m_pMedfiltIntensityMap = new medfilt(3 * pConfig->flimAlines, pConfig->frames, 5, 3);
	if (m_pMedfiltLifetimeMap) delete m_pMedfiltLifetimeMap;
	m_pMedfiltLifetimeMap = new medfilt(3 * pConfig->flimAlines, pConfig->frames, 7, 5);
	if (m_pMedfiltLongi) delete m_pMedfiltLongi;
	m_pMedfiltLongi = new medfilt(frames4, diameter, 3, 3);

	m_pConfig->writeToLog("Reviewing objects are initialized.");
}

void QViewTab::setWidgets(Configuration* pConfig)
{
	// Non-4's multiple width
	int frames4 = (pConfig->frames);

#ifndef NEXT_GEN_SYSTEM
	int diameter = 2 * pConfig->octRadius;
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
	m_pImageView_CircImage->getRender()->m_dPixelResol = !pConfig->is_dotter ? PIXEL_RESOLUTION_SAMSUNG : PIXEL_RESOLUTION_DOTTER;
	m_pImageView_CircImage->setScaleBar(int(1000.0 / m_pImageView_CircImage->getRender()->m_dPixelResol));

	m_pImageView_EnFace->resetSize(frames4, pConfig->flimAlines);
	m_pImageView_EnFace->setVLineChangeCallback([&](int frame) { m_pSlider_SelectFrame->setValue(frame); });
	m_pImageView_EnFace->setVerticalLine(1, 0);
	m_pImageView_EnFace->setHLineChangeCallback([&](int aline) { visualizeLongiImage(4 * aline / (m_pConfig->flimParameterMode == FLImParameters::_NONE_ ? 4 : 1)); });
	m_pImageView_EnFace->setHorizontalLine(1, 0);
	if (m_pConfig->flimParameterMode == FLImParameters::_LIFETIME_)
	{
		m_pImageView_EnFace->setEnterCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 4),
			QString::fromLocal8Bit("2-D FLIm En Face Chemogram\n- Ch %1 Lifetime (z-θ)").arg(m_pConfig->flimEmissionChannel)); });
	}
	else if (m_pConfig->flimParameterMode == FLImParameters::_INTENSITY_PROP_)
	{
		m_pImageView_EnFace->setEnterCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 4),
			QString::fromLocal8Bit("2-D FLIm En Face Chemogram\n- Ch %1 Intensity Proportion (z-θ)").arg(m_pConfig->flimEmissionChannel)); });
	}
	else if (m_pConfig->flimParameterMode == FLImParameters::_INTENSITY_RATIO_)
	{
		m_pImageView_EnFace->setEnterCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 4),
			QString::fromLocal8Bit("2-D FLIm En Face Chemogram\n- Ch %1/%2 Intensity Ratio (z-θ)").arg(m_pConfig->flimEmissionChannel)
			.arg((m_pConfig->flimEmissionChannel == 1) ? 3 : m_pConfig->flimEmissionChannel - 1)); });
	}
	else if (m_pConfig->flimParameterMode == FLImParameters::_NONE_)
	{
		m_pImageView_EnFace->resetSize(frames4, pConfig->octAlines);
		m_pImageView_EnFace->setEnterCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 4),
			QString::fromLocal8Bit("2-D OCT Max Projection (z-θ)")); });
	}
	m_pImageView_EnFace->setLeaveCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 18), ""); });

	m_pImageView_Longi->resetSize(frames4, diameter);
	//m_pImageView_Longi->resetSize(LONGI_WIDTH, LONGI_HEIGHT);
	m_pImageView_Longi->setVLineChangeCallback([&](int frame) { m_pSlider_SelectFrame->setValue(frame); });
	m_pImageView_Longi->setVerticalLine(1, 0);
	m_pImageView_Longi->getRender()->m_bDiametric = true;
	m_pImageView_Longi->setScaleBar(int(1000.0 / m_pImageView_CircImage->getRender()->m_dPixelResol));
	///m_pImageView_Longi->setHorizontalLine(2, diameter / 2 + OUTER_SHEATH_POSITION, diameter / 2 - OUTER_SHEATH_POSITION);
	m_pImageView_Longi->setEnterCallback([&]() { m_pImageView_Longi->setText(QPoint(8, 4), "Longitudinal View (z-r)"); });
	m_pImageView_Longi->setLeaveCallback([&]() { m_pImageView_Longi->setText(QPoint(8, 4), ""); });

	int vis_mode = m_pRadioButton_MLPrediction->isChecked();
	if (vis_mode == VisualizationMode::_FLIM_PARAMETERS_)
	{
		if (m_pConfig->flimParameterMode == FLImParameters::_LIFETIME_)
		{
			if (m_pConfig->flimColormapType == FlimColormapType::_HSV_)
			{
				m_pImageView_ColorBar->setText(QPoint(0, 0), QString("%1            Lifetime (nsec)            %2")
					.arg(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max, 2, 'f', 1)
					.arg(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min, 2, 'f', 1), true);
				m_pImageView_ColorBar->resetColormap(ColorTable::colortable(LIFETIME_COLORTABLE));
			}
			else if (m_pConfig->flimColormapType == FlimColormapType::_TCT_NEW_)
			{
				m_pImageView_ColorBar->setText(QPoint(0, 0), QString("%1            Lifetime (nsec)            %2")
					.arg(m_pConfig->flimLifetimeRangeNew[m_pConfig->flimEmissionChannel - 1].max, 2, 'f', 1)
					.arg(m_pConfig->flimLifetimeRangeNew[m_pConfig->flimEmissionChannel - 1].min, 2, 'f', 1), true);
				m_pImageView_ColorBar->resetColormap(ColorTable::colortable(NEW_LIFETIME_COLORTABLE + m_pConfig->flimEmissionChannel - 1));
			}
		}
		else if (m_pConfig->flimParameterMode == FLImParameters::_INTENSITY_PROP_)
		{
			m_pImageView_ColorBar->setText(QPoint(0, 0), QString("%1    Intensity Proportion (AU)    %2")
				.arg(m_pConfig->flimIntensityPropRange[m_pConfig->flimEmissionChannel - 1].max, 2, 'f', 1)
				.arg(m_pConfig->flimIntensityPropRange[m_pConfig->flimEmissionChannel - 1].min, 2, 'f', 1), true);
			m_pImageView_ColorBar->resetColormap(ColorTable::colortable(INTENSITY_PROP_COLORTABLE));
		}
		else if (m_pConfig->flimParameterMode == FLImParameters::_INTENSITY_RATIO_)
		{
			m_pImageView_ColorBar->setText(QPoint(0, 0), QString("%1       Intensity Ratio (AU)       %2")
				.arg(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1].max, 2, 'f', 1)
				.arg(m_pConfig->flimIntensityRatioRange[m_pConfig->flimEmissionChannel - 1].min, 2, 'f', 1), true);
			m_pImageView_ColorBar->resetColormap(ColorTable::colortable(INTENSITY_RATIO_COLORTABLE));
		}
		else if (m_pConfig->flimParameterMode == FLImParameters::_NONE_)
		{
			m_pImageView_ColorBar->setText(QPoint(0, 0), "", true);
			m_pImageView_ColorBar->resetColormap(ColorTable::colortable(OCT_COLORTABLE));
		}
	}
	else if (vis_mode == VisualizationMode::_ML_PREDICTION_)
	{
		m_pImageView_EnFace->setEnterCallback([&]() { m_pImageView_EnFace->setText(QPoint(8, 4),
			QString::fromLocal8Bit("2-D Plaque Composition\nEn Face Chemogram (z-θ)")); });
		m_pImageView_ColorBar->setText(QPoint(0, 0), "", true);
		m_pImageView_ColorBar->resetColormap(ColorTable::colortable(COMPOSITION_COLORTABLE));	
	}

	m_pConfig->writeToLog("Reviewing widgets are initialized.");
}

void QViewTab::resetImgObjLifetime(Configuration* pConfig)
{
	// Non-4's multiple width
	int frames4 = ROUND_UP_4S(pConfig->frames);

	ColorTable temp_ctable;
	if (m_pImgObjLifetimeMap) delete m_pImgObjLifetimeMap;
	if (m_pConfig->flimColormapType == FlimColormapType::_HSV_)
		m_pImgObjLifetimeMap = new ImageObject(frames4, pConfig->flimAlines, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE));
	else if (m_pConfig->flimColormapType == FlimColormapType::_TCT_NEW_)
		m_pImgObjLifetimeMap = new ImageObject(frames4, pConfig->flimAlines, temp_ctable.m_colorTableVector.at(NEW_LIFETIME_COLORTABLE + m_pConfig->flimEmissionChannel - 1));
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
		int vis_mode = m_pRadioButton_MLPrediction->isChecked();

		if (scaling)
		{
			// Scaling OCT projection
			IppiSize roi_proj = { m_octProjection.size(0), m_octProjection.size(1) };
			IppiSize roi_proj4 = { roi_proj.width, ROUND_UP_4S(roi_proj.height) };

			np::FloatArray2 scale_temp32(roi_proj.width, roi_proj.height);
			np::Uint8Array2 scale_temp8(roi_proj4.width, roi_proj4.height);

			ippsConvert_8u32f(m_octProjection.raw_ptr(), scale_temp32.raw_ptr(), scale_temp32.length());
			ippiScale_32f8u_C1R(scale_temp32.raw_ptr(), sizeof(float) * roi_proj.width,
				scale_temp8.raw_ptr(), sizeof(uint8_t) * roi_proj4.width, roi_proj,
				m_pConfig->octGrayRange.min, m_pConfig->octGrayRange.max * (m_pConfigTemp->reflectionRemoval ? 1.1f : 1.0f));
			ippiTranspose_8u_C1R(scale_temp8.raw_ptr(), sizeof(uint8_t) * roi_proj4.width,
				m_pImgObjOctProjection->arr.raw_ptr(), sizeof(uint8_t) * roi_proj4.height, roi_proj4);
			circShift(m_pImgObjOctProjection->arr, m_pConfig->rotatedAlines);
			m_pImgObjOctProjection->convertRgb();

			// Scaling FLIM map
			scaleFLImEnFaceMap(m_pImgObjIntensityMap, m_pImgObjLifetimeMap,
				m_pImgObjIntensityPropMap, m_pImgObjIntensityRatioMap,
				m_pImgObjPlaqueCompositionMap, 
				vis_mode, m_pConfig->flimEmissionChannel - 1, m_pConfig->flimParameterMode, m_pConfig->mlPredictionMode);				
		}			
		
		if (vis_mode == VisualizationMode::_FLIM_PARAMETERS_)
		{
			if (m_pConfig->flimParameterMode == FLImParameters::_LIFETIME_)
				emit paintEnFaceMap(m_pImgObjLifetimeMap->qrgbimg.bits());
			else if (m_pConfig->flimParameterMode == FLImParameters::_INTENSITY_PROP_)
				emit paintEnFaceMap(m_pImgObjIntensityPropMap->qrgbimg.bits());
			else if (m_pConfig->flimParameterMode == FLImParameters::_INTENSITY_RATIO_)
				emit paintEnFaceMap(m_pImgObjIntensityRatioMap->qrgbimg.bits());
			else if (m_pConfig->flimParameterMode == FLImParameters::_NONE_)
				emit paintEnFaceMap(m_pImgObjOctProjection->qrgbimg.bits());
				
		}
		else if (vis_mode == VisualizationMode::_ML_PREDICTION_)
		{			
			emit paintEnFaceMap(m_pImgObjPlaqueCompositionMap->qrgbimg.bits());			
		}
	}
}

void QViewTab::visualizeImage(uint8_t* oct_im, float* intensity, float* lifetime) // Streaming mode
{
	// OCT Visualization
#ifndef NEXT_GEN_SYSTEM
	IppiSize roi_oct = { m_pConfig->octRadius, m_pConfig->octAlines };
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
		m_pConfig->flimColormapType == FlimColormapType::_HSV_ ? m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min
		: m_pConfig->flimLifetimeRangeNew[m_pConfig->flimEmissionChannel - 1].min,
		m_pConfig->flimColormapType == FlimColormapType::_HSV_ ? m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max
		: m_pConfig->flimLifetimeRangeNew[m_pConfig->flimEmissionChannel - 1].max);

	// Convert RGB
	m_pImgObjRectImage->convertRgb();
	m_pImgObjIntensity->convertScaledRgb();
	m_pImgObjLifetime->convertScaledRgb();
	
	// Intensity-weight lifetime
	ippsMul_8u_ISfs(m_pImgObjIntensity->qrgbimg.bits(), m_pImgObjLifetime->qrgbimg.bits(), m_pImgObjIntensity->qrgbimg.byteCount(), 8);

	// Copy FLIM rings
	int ring_thickness = (RING_THICKNESS * m_pConfigTemp->octRadius) / 1024;
	tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)ring_thickness),
		[&, ring_thickness](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
		{
			ippiCopy_8u_C3R(m_pImgObjLifetime->qrgbimg.constBits(), 3 * roi_flim.width,
				m_pImgObjRectImage->qrgbimg.bits() + 3 * (m_pImgObjRectImage->arr.size(0) - ring_thickness + i), 3 * m_pImgObjRectImage->arr.size(0), { 1, m_pImgObjRectImage->arr.size(1) });
		}
	});
	
	// Make circularzing
	emit makeCirc();
}

void QViewTab::visualizeImage(int frame) // Post-processing mode
{
    if (m_vectorOctImage.size() != 0)
    {
		// Measure state uncheck
		if (m_pToggleButton_MeasureDistance->isChecked()) m_pToggleButton_MeasureDistance->setChecked(false);
		if (m_pToggleButton_MeasureArea->isChecked()) m_pToggleButton_MeasureArea->setChecked(false);
		if (m_contourMap.length() == 0)	if (m_pToggleButton_AutoContour->isChecked()) m_pToggleButton_AutoContour->setChecked(false);

        // OCT Visualization
		scaleOctImage(m_vectorOctImage.at(frame), m_pImgObjRectImage->arr, m_pConfigTemp->reflectionRemoval);		
		circShift(m_pImgObjRectImage->arr, (m_pConfigTemp->rotatedAlines) % m_pConfigTemp->octAlines);  ///  + m_pConfigTemp->intraFrameSync
        (*m_pMedfiltRect)(m_pImgObjRectImage->arr.raw_ptr());
		
		// Convert RGB
		m_pImgObjRectImage->convertRgb();

        // FLIM Visualization
		int vis_mode = m_pRadioButton_MLPrediction->isChecked();
		int ring_thickness = (RING_THICKNESS * m_pConfigTemp->octRadius) / 1024;
		IppiSize roi_flimring = { 1, m_pImgObjIntensityMap->arr.size(1) };
		
		ImageObject *temp_imgobj;
		if (vis_mode == VisualizationMode::_FLIM_PARAMETERS_)
		{
			if (m_pConfig->flimParameterMode == FLImParameters::_LIFETIME_)
			{
				temp_imgobj = new ImageObject(roi_flimring.height, 1, m_pImgObjLifetimeMap->getColorTable());
				ippiCopy_8u_C3R(m_pImgObjLifetimeMap->qrgbimg.constBits() + 3 * frame, 3 * m_pImgObjLifetimeMap->arr.size(0),
					temp_imgobj->qrgbimg.bits(), 3, roi_flimring);
			}
			else if (m_pConfig->flimParameterMode == FLImParameters::_INTENSITY_PROP_)
			{
				temp_imgobj = new ImageObject(roi_flimring.height, 1, m_pImgObjIntensityPropMap->getColorTable());
				ippiCopy_8u_C3R(m_pImgObjIntensityPropMap->qrgbimg.constBits() + 3 * frame, 3 * m_pImgObjIntensityPropMap->arr.size(0),
					temp_imgobj->qrgbimg.bits(), 3, roi_flimring);
			}
			else if (m_pConfig->flimParameterMode == FLImParameters::_INTENSITY_RATIO_)
			{
				temp_imgobj = new ImageObject(roi_flimring.height, 1, m_pImgObjIntensityRatioMap->getColorTable());
				ippiCopy_8u_C3R(m_pImgObjIntensityRatioMap->qrgbimg.constBits() + 3 * frame, 3 * m_pImgObjIntensityRatioMap->arr.size(0),
					temp_imgobj->qrgbimg.bits(), 3, roi_flimring);
			}
			else if (m_pConfig->flimParameterMode == FLImParameters::_NONE_)
			{
				temp_imgobj = new ImageObject(roi_flimring.height, 1, m_pImgObjIntensityMap->getColorTable());
				memset(temp_imgobj->qrgbimg.bits(), 0, sizeof(uint8_t) * 3 * roi_flimring.height);
			}
		}
		else if (vis_mode == VisualizationMode::_ML_PREDICTION_)
		{			
			temp_imgobj = new ImageObject(roi_flimring.height, 1, m_pImgObjPlaqueCompositionMap->getColorTable());
			ippiCopy_8u_C3R(m_pImgObjPlaqueCompositionMap->qrgbimg.constBits() + 3 * frame, 3 * m_pImgObjPlaqueCompositionMap->arr.size(0),
				temp_imgobj->qrgbimg.bits(), 3, roi_flimring);			
		}
		temp_imgobj->scaledRgb4();

		if ((vis_mode == VisualizationMode::_ML_PREDICTION_) || (m_pConfig->flimParameterMode != FLImParameters::_NONE_))
		{
			tbb::parallel_for(tbb::blocked_range<size_t>(1, (size_t)ring_thickness),
				[&, ring_thickness](const tbb::blocked_range<size_t>& r) {
				for (size_t i = r.begin(); i != r.end(); ++i)
				{
					ippiCopy_8u_C3R(temp_imgobj->qrgbimg.constBits(), 3 * roi_flimring.width,
						m_pImgObjRectImage->qrgbimg.bits() + 3 * (m_pImgObjRectImage->arr.size(0) - ring_thickness + i),
						3 * m_pImgObjRectImage->arr.size(0), { 1, m_pImgObjRectImage->arr.size(1) });
				}
			});
		}

		if (temp_imgobj) delete temp_imgobj;
				
		// Contour Lines
		if (m_pToggleButton_AutoContour->isChecked())
		{
			if (m_contourMap.length() != 0)
			{
				{
					np::Uint16Array contour_16u(m_pConfigTemp->octAlines);
					ippsConvert_32f16u_Sfs(&m_contourMap(0, getCurrentFrame()), contour_16u, contour_16u.length(), ippRndNear, 0);
					ippsAddC_16s_ISfs(m_pConfigTemp->circOffset, (Ipp16s *)contour_16u.raw_ptr(), contour_16u.length(), 0);
					std::rotate(&contour_16u(0), &contour_16u((m_vibCorrIdx(getCurrentFrame()) + m_pConfigTemp->rotatedAlines) % m_pConfigTemp->octAlines), &contour_16u(contour_16u.length()));
					m_pImageView_CircImage->setContour(m_pConfigTemp->octAlines, contour_16u);					
				}
				{
					std::vector<int> gwp = m_gwPoss.at(getCurrentFrame());
					for (int i = 0; i < gwp.size(); i++)
					{
						gwp.at(i) = gwp.at(i) - m_vibCorrIdx(getCurrentFrame()) - m_pConfigTemp->rotatedAlines;
						while (gwp.at(i) < 0)
							gwp.at(i) += m_pConfigTemp->octAlines;
					}
					m_pImageView_CircImage->setGwPos(gwp);
				}
				m_pImageView_CircImage->getRender()->update();
			}
		}

		// Make circularzing
		emit makeCirc();

        // En Face Lines
        m_pImageView_EnFace->setVerticalLine(1, frame);
		m_pImageView_EnFace->getRender()->DidClickedMouse();
        m_pImageView_EnFace->getRender()->update();
				
        // Longitudinval Lines
        m_pImageView_Longi->setVerticalLine(1, frame);
		m_pImageView_Longi->getRender()->m_vecPickFrames.clear();
		if (m_vectorPickFrames.size() > 0)
			for (int i = 0; i < m_vectorPickFrames.size(); i++)
				m_pImageView_Longi->getRender()->m_vecPickFrames.push_back(m_vectorPickFrames.at(i).at(0).toInt());
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
				qDebug() << matches;

				int ivus_frame = matches.at(1).toInt() - 1;
				if (ivus_frame != -1)
				{
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
			}
			else
			{
				ippsSet_8u(52, ivusImage, ivusImage.length());
				m_pImageView_Ivus->setEnterCallback([&]() { m_pImageView_Ivus->setText(QPoint(8, 230), ""); });
			}

			m_pImageView_Ivus->drawImage(ivusImage);
		}

        // Status Update
		bool is_pick_frame = false;
		if (m_vectorPickFrames.size() > 0)
		{
			for (int i = 0; i < m_vectorPickFrames.size(); i++)
			{
				if (m_vectorPickFrames.at(i).at(0).toInt() == frame + 1)
				{
					is_pick_frame = true;
					break;
				}
			}	
		}
		QString str; str.sprintf("Frame : %3d / %3d", frame + 1, (int)m_vectorOctImage.size());
		m_pImageView_CircImage->setText(QPoint(15, 590), str, false, is_pick_frame ? Qt::magenta : Qt::white);

		if (!is_pick_frame)
			m_pPushButton_Pick->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
		else
			m_pPushButton_Pick->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));

		if (m_pToggleButton_Statistics->isChecked())
			calculateStatistics(true);
    }
}

void QViewTab::visualizeLongiImage(int aline)
{
	// Pre-determined values
	int frames = (int)m_vectorOctImage.size();
	int octScans = m_pConfigTemp->octRadius;
    int octAlines = m_octProjection.size(0);

    // Specified A line
	int aline_ = (aline + octAlines / 2) % octAlines; // aline & aline_ paired

	int aline0 = (aline + m_pConfigTemp->rotatedAlines) % octAlines;  ///  + m_pConfigTemp->intraFrameSync
    int aline1 = (aline0 + octAlines / 2) % octAlines; // aline0 & aline1 paired

    // Make longitudinal - OCT
	IppiSize roi_longi = { m_pImgObjLongiImage->getHeight(), m_pImgObjLongiImage->getWidth() };

	np::Uint8Array2 scale_temp(roi_longi.width, roi_longi.height);
	memset(scale_temp, 0, sizeof(uint8_t) * scale_temp.length());

	if (!m_pToggleButton_DiameterView->isChecked())
	{	
		tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)frames),
			[&](const tbb::blocked_range<size_t>& r) {
			for (size_t i = r.begin(); i != r.end(); ++i)
			{
#ifndef NEXT_GEN_SYSTEM
				if (m_pConfigTemp->circOffset > 0)
				{
					memcpy(&scale_temp(m_pConfigTemp->circOffset, (int)i), &m_vectorOctImage.at((int)i)(0, aline0), sizeof(uint8_t) * (octScans - m_pConfigTemp->circOffset));
					memcpy(&scale_temp(octScans + m_pConfigTemp->circOffset, (int)i), &m_vectorOctImage.at((int)i)(0, aline1), sizeof(uint8_t) * (octScans - m_pConfigTemp->circOffset));
				}
				else
				{
					memcpy(&scale_temp(0, (int)i), &m_vectorOctImage.at((int)i)(-m_pConfigTemp->circOffset, aline0), sizeof(uint8_t) * (octScans + m_pConfigTemp->circOffset));
					memcpy(&scale_temp(octScans, (int)i), &m_vectorOctImage.at((int)i)(-m_pConfigTemp->circOffset, aline1), sizeof(uint8_t) * (octScans + m_pConfigTemp->circOffset));
				}
				ippsFlip_8u_I(&scale_temp(0, (int)i), octScans);
#else
				memcpy(&longi_temp(0, (int)i), &m_vectorOctImage.at((int)i)(0, aline0), sizeof(float) * octScans);
				memcpy(&longi_temp(octScans, (int)i), &m_vectorOctImage.at((int)i)(0, aline1), sizeof(float) * octScans);
				ippsFlip_32f_I(&longi_temp(0, (int)i), octScans);
#endif
			}
		});

#ifndef NEXT_GEN_SYSTEM
		np::FloatArray2 longi_temp(roi_longi.width, roi_longi.height);
		ippsConvert_8u32f(scale_temp.raw_ptr(), longi_temp.raw_ptr(), scale_temp.length());
		if (m_pConfigTemp->reflectionRemoval)
		{
			np::FloatArray2 reflection_temp(roi_longi.width, roi_longi.height);
			ippiCopy_32f_C1R(&longi_temp(0, 0), sizeof(float) * longi_temp.size(0),
				&reflection_temp(m_pConfigTemp->reflectionDistance, 0), sizeof(float) * reflection_temp.size(0),
				{ roi_longi.width / 2 - m_pConfigTemp->reflectionDistance, roi_longi.height });
			ippiCopy_32f_C1R(&longi_temp(roi_longi.width / 2 + m_pConfigTemp->reflectionDistance, 0), sizeof(float) * longi_temp.size(0),
				&reflection_temp(roi_longi.width / 2, 0), sizeof(float) * reflection_temp.size(0),
				{ roi_longi.width / 2 - m_pConfigTemp->reflectionDistance, roi_longi.height });
			ippsMulC_32f_I(m_pConfigTemp->reflectionLevel, reflection_temp, reflection_temp.length());
			ippsSub_32f_I(reflection_temp, longi_temp, longi_temp.length());
			ippiScale_32f8u_C1R(longi_temp.raw_ptr(), roi_longi.width * sizeof(float),
				scale_temp.raw_ptr(), roi_longi.width * sizeof(uint8_t), roi_longi, (float)m_pConfigTemp->octGrayRange.min, 0.9f * (float)m_pConfigTemp->octGrayRange.max);
		}
		else
			ippiScale_32f8u_C1R(longi_temp.raw_ptr(), roi_longi.width * sizeof(float),
				scale_temp.raw_ptr(), roi_longi.width * sizeof(uint8_t), roi_longi, m_pConfigTemp->octGrayRange.min, m_pConfigTemp->octGrayRange.max);

		///ippsConvert_8u32f(scale_temp.raw_ptr(), longi_temp.raw_ptr(), scale_temp.length());
		///ippiScale_32f8u_C1R(longi_temp.raw_ptr(), roi_longi.width * sizeof(float),
		///    scale_temp.raw_ptr(), roi_longi.width * sizeof(uint8_t), roi_longi, m_pConfigTemp->octGrayRange.min, m_pConfigTemp->octGrayRange.max);
#else
		ippiScale_32f8u_C1R(longi_temp.raw_ptr(), roi_longi.width * sizeof(float),
			scale_temp.raw_ptr(), roi_longi.width * sizeof(uint8_t), roi_longi, m_pConfig->axsunDbRange.min, m_pConfig->axsunDbRange.max);
#endif
		///if (aline0 > (octAlines / 2))
		///    ippiMirror_8u_C1IR(scale_temp.raw_ptr(), sizeof(uint8_t) * roi_longi.width, roi_longi, ippAxsVertical);
		ippiTranspose_8u_C1R(scale_temp.raw_ptr(), roi_longi.width * sizeof(uint8_t), m_pImgObjLongiImage->arr.raw_ptr(), roi_longi.height * sizeof(uint8_t), roi_longi);
		(*m_pMedfiltLongi)(m_pImgObjLongiImage->arr.raw_ptr());		
	}
	else
	{
		ippsSet_8u(150, scale_temp, scale_temp.length());

		tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)frames),
			[&](const tbb::blocked_range<size_t>& r) {
			for (size_t i = r.begin(); i != r.end(); ++i)
			{
				float mean_radius;
				ippsMean_32f(&m_contourMap(0, i), m_pConfigTemp->octAlines, &mean_radius, ippAlgHintAccurate);
				mean_radius += (float)m_pConfigTemp->circOffset;
				if (mean_radius > octScans)
					mean_radius = octScans;
			
				ippsSet_8u(0, &scale_temp(octScans - (int)round(mean_radius), i), (int)round(mean_radius) * 2);
			}
		});

		ippiTranspose_8u_C1R(scale_temp.raw_ptr(), roi_longi.width * sizeof(uint8_t), m_pImgObjLongiImage->arr.raw_ptr(), roi_longi.height * sizeof(uint8_t), roi_longi);
	}

	m_pImgObjLongiImage->convertRgb();

    // Make longitudinal - FLIM
	int vis_mode = m_pRadioButton_MLPrediction->isChecked();	
	int ring_thickness = (RING_THICKNESS * m_pConfigTemp->octRadius) / 1024;

	if (vis_mode == VisualizationMode::_FLIM_PARAMETERS_)
	{
		if (m_pConfig->flimParameterMode == FLImParameters::_LIFETIME_)
		{
			tbb::parallel_for(tbb::blocked_range<size_t>(1, (size_t)ring_thickness),
				[&, ring_thickness](const tbb::blocked_range<size_t>& r) {
				for (size_t i = r.begin(); i != r.end(); ++i)
				{
					memcpy(m_pImgObjLongiImage->qrgbimg.bits() + 3 * (int)i * m_pImgObjLongiImage->getWidth(),
						m_pImgObjLifetimeMap->qrgbimg.bits() + 3 * int(aline / 4) * m_pImgObjLifetimeMap->getWidth(), sizeof(uint8_t) * 3 * m_pImgObjLongiImage->getWidth());
					memcpy(m_pImgObjLongiImage->qrgbimg.bits() + 3 * (int)(m_pImgObjLongiImage->getHeight() - i) * m_pImgObjLongiImage->getWidth(),
						m_pImgObjLifetimeMap->qrgbimg.bits() + 3 * int(aline_ / 4) * m_pImgObjLifetimeMap->getWidth(), sizeof(uint8_t) * 3 * m_pImgObjLongiImage->getWidth());
				}
			});
		}
		else if (m_pConfig->flimParameterMode == FLImParameters::_INTENSITY_PROP_)
		{
			tbb::parallel_for(tbb::blocked_range<size_t>(1, (size_t)ring_thickness),
				[&, ring_thickness](const tbb::blocked_range<size_t>& r) {
				for (size_t i = r.begin(); i != r.end(); ++i)
				{
					memcpy(m_pImgObjLongiImage->qrgbimg.bits() + 3 * (int)i * m_pImgObjLongiImage->getWidth(),
						m_pImgObjIntensityPropMap->qrgbimg.bits() + 3 * int(aline / 4) * m_pImgObjIntensityPropMap->getWidth(), sizeof(uint8_t) * 3 * m_pImgObjLongiImage->getWidth());
					memcpy(m_pImgObjLongiImage->qrgbimg.bits() + 3 * (int)(m_pImgObjLongiImage->getHeight() - i) * m_pImgObjLongiImage->getWidth(),
						m_pImgObjIntensityPropMap->qrgbimg.bits() + 3 * int(aline_ / 4) * m_pImgObjIntensityPropMap->getWidth(), sizeof(uint8_t) * 3 * m_pImgObjLongiImage->getWidth());
				}
			});
		}
		else if (m_pConfig->flimParameterMode == FLImParameters::_INTENSITY_RATIO_)
		{
			tbb::parallel_for(tbb::blocked_range<size_t>(1, (size_t)ring_thickness),
				[&, ring_thickness](const tbb::blocked_range<size_t>& r) {
				for (size_t i = r.begin(); i != r.end(); ++i)
				{
					memcpy(m_pImgObjLongiImage->qrgbimg.bits() + 3 * (int)i * m_pImgObjLongiImage->getWidth(),
						m_pImgObjIntensityRatioMap->qrgbimg.bits() + 3 * int(aline / 4) * m_pImgObjIntensityRatioMap->getWidth(), sizeof(uint8_t) * 3 * m_pImgObjLongiImage->getWidth());
					memcpy(m_pImgObjLongiImage->qrgbimg.bits() + 3 * (int)(m_pImgObjLongiImage->getHeight() - i) * m_pImgObjLongiImage->getWidth(),
						m_pImgObjIntensityRatioMap->qrgbimg.bits() + 3 * int(aline_ / 4) * m_pImgObjIntensityRatioMap->getWidth(), sizeof(uint8_t) * 3 * m_pImgObjLongiImage->getWidth());
				}
			});
		}
		else if (m_pConfig->flimParameterMode == FLImParameters::_NONE_)
		{
			tbb::parallel_for(tbb::blocked_range<size_t>(1, (size_t)ring_thickness),
				[&, ring_thickness](const tbb::blocked_range<size_t>& r) {
				for (size_t i = r.begin(); i != r.end(); ++i)
				{
					memset(m_pImgObjLongiImage->qrgbimg.bits() + 3 * (int)i * m_pImgObjLongiImage->getWidth(), 0, 
						sizeof(uint8_t) * 3 * m_pImgObjLongiImage->getWidth());
					memset(m_pImgObjLongiImage->qrgbimg.bits() + 3 * (int)(m_pImgObjLongiImage->getHeight() - i) * m_pImgObjLongiImage->getWidth(),	0, 
						sizeof(uint8_t) * 3 * m_pImgObjLongiImage->getWidth());
				}
			});
		}
	}
	else if (vis_mode == VisualizationMode::_ML_PREDICTION_)
	{
		tbb::parallel_for(tbb::blocked_range<size_t>(1, (size_t)ring_thickness),
			[&, ring_thickness](const tbb::blocked_range<size_t>& r) {
			for (size_t i = r.begin(); i != r.end(); ++i)
			{
				memcpy(m_pImgObjLongiImage->qrgbimg.bits() + 3 * (int)i * m_pImgObjLongiImage->getWidth(),
					m_pImgObjPlaqueCompositionMap->qrgbimg.bits() + 3 * int(aline / 4) * m_pImgObjPlaqueCompositionMap->getWidth(), sizeof(uint8_t) * 3 * m_pImgObjLongiImage->getWidth());
				memcpy(m_pImgObjLongiImage->qrgbimg.bits() + 3 * (int)(m_pImgObjLongiImage->getHeight() - i) * m_pImgObjLongiImage->getWidth(),
					m_pImgObjPlaqueCompositionMap->qrgbimg.bits() + 3 * int(aline_ / 4) * m_pImgObjPlaqueCompositionMap->getWidth(), sizeof(uint8_t) * 3 * m_pImgObjLongiImage->getWidth());
			}
		});	
	}
	
	// Draw contours
	if (m_pToggleButton_AutoContour->isChecked() && !m_pToggleButton_DiameterView->isChecked())
	{
		if (m_contourMap.length() != 0)
		{
			np::Uint16Array lcontour_16u(2 * m_pConfigTemp->frames);
			for (int i = 0; i < m_pConfigTemp->frames; i++)
			{
				lcontour_16u(i) = m_contourMap((aline0 + (int)m_vibCorrIdx(i)) % m_pConfigTemp->octAlines, i);
				lcontour_16u(i + m_pConfigTemp->frames) = m_contourMap((aline1 + (int)m_vibCorrIdx(i)) % m_pConfigTemp->octAlines, i);
			}
			ippsAddC_16s_ISfs(m_pConfigTemp->circOffset, (Ipp16s *)lcontour_16u.raw_ptr(), lcontour_16u.length(), 0);

			m_pImageView_Longi->setContour(2 * m_pConfigTemp->frames, lcontour_16u);
			m_pImageView_Longi->getRender()->update();
		}
	}
	else
	{
		m_pImageView_Longi->setContour(0, nullptr);
		m_pImageView_Longi->getRender()->update();
	}
	
	// Draw longitudinal view
    emit paintLongiImage(m_pImgObjLongiImage->qrgbimg.bits());

	// Circular view lines
    m_pImageView_CircImage->setVerticalLine(1, aline);
    m_pImageView_CircImage->getRender()->update();

	// En Face lines
	m_pImageView_EnFace->setHorizontalLine(1, int(aline / 4 * (m_pConfig->flimParameterMode == FLImParameters::_NONE_ ? 4 : 1)));
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
		m_pPushButton_LumenDetection->setDisabled(true);
		m_pToggleButton_AutoContour->setDisabled(true);
		m_pToggleButton_DiameterView->setDisabled(true);
		
		int cur_frame = m_pSlider_SelectFrame->value();
		int end_frame = (int)m_vectorOctImage.size();

		if (cur_frame + 1 == end_frame)
			cur_frame = 0;

		_running = true;
		playing = std::thread([&, cur_frame, end_frame]() {
			for (int i = cur_frame; i < end_frame; i++)
			{
				m_pSlider_SelectFrame->setValue(i);
				std::this_thread::sleep_for(std::chrono::milliseconds(m_pConfig->playInterval)); // 40

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
		m_pToggleButton_AutoContour->setDisabled(false);
		if (m_contourMap.length() == 0)
			m_pPushButton_LumenDetection->setEnabled(true);
		else
			m_pToggleButton_DiameterView->setEnabled(true);

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
		if (m_pToggleButton_AutoContour->isChecked()) m_pToggleButton_AutoContour->setChecked(false);

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
		if (m_pToggleButton_AutoContour->isChecked()) m_pToggleButton_AutoContour->setChecked(false);

		m_pImageView_CircImage->setRLineChangeCallback([&](int aline) { (void)aline; });

		m_pConfig->writeToLog("Measure area off.");
	}
}

void QViewTab::autoContouring(bool toggled)
{
	if (toggled)
	{		
		if (m_pToggleButton_MeasureDistance->isChecked()) m_pToggleButton_MeasureDistance->setChecked(false);
		if (m_pToggleButton_MeasureArea->isChecked()) m_pToggleButton_MeasureArea->setChecked(false);

		// Set lumen contour in circ image
		np::Uint16Array contour_16u(m_pConfigTemp->octAlines);		
		if (m_contourMap.length() == 0)
		{
			if (!m_pLumenDetection)
			{				
				m_pLumenDetection = new LumenDetection(int(OUTER_SHEATH_POSITION / m_pImageView_CircImage->getRender()->m_dPixelResol),  /// - m_pConfigTemp->circOffset, 
					!m_pConfigTemp->is_dotter ? m_pConfigTemp->innerOffsetLength : 0, false);  ///true, true, m_pConfigTemp->reflectionDistance, m_pConfigTemp->reflectionLevel);

				np::Uint8Array2 oct_image(m_pConfigTemp->octRadius, m_pConfigTemp->octAlines);
				scaleOctImage(m_vectorOctImage.at(getCurrentFrame()), oct_image, m_pConfigTemp->reflectionRemoval);

				np::FloatArray contour;
				(*m_pLumenDetection)(oct_image, contour);
								
				ippsConvert_32f16u_Sfs(contour, contour_16u, contour.length(), ippRndNear, 0);
			}
		}
		else
		{
			ippsConvert_32f16u_Sfs(&m_contourMap(0, getCurrentFrame()), contour_16u, contour_16u.length(), ippRndNear, 0);
			ippsAddC_16s_ISfs(m_pConfigTemp->circOffset, (Ipp16s *)contour_16u.raw_ptr(), contour_16u.length(), 0);
			std::rotate(&contour_16u(0), &contour_16u((m_vibCorrIdx(getCurrentFrame()) + m_pConfigTemp->rotatedAlines) % m_pConfigTemp->octAlines), &contour_16u(contour_16u.length()));
		}							
		m_pImageView_CircImage->setContour(m_pConfigTemp->octAlines, contour_16u);		
		
		// Set lumen contour in longi image
		np::Uint16Array lcontour_16u(2 * m_pConfigTemp->frames);
		if (m_contourMap.length() != 0)
		{
			visualizeLongiImage(getCurrentAline());
			///ippiConvert_32f16u_C1RSfs(&m_contourMap(getCurrentAline(), 0), sizeof(float) * m_contourMap.size(0), 
			///	lcontour_16u, sizeof(uint16_t), { 1, m_pConfigTemp->frames }, ippRndNear, 0);
			///ippiConvert_32f16u_C1RSfs(&m_contourMap((getCurrentAline() + m_pConfigTemp->octAlines / 2) % m_pConfigTemp->octAlines, 0), sizeof(float) * m_contourMap.size(0),
			///	lcontour_16u + m_pConfigTemp->frames, sizeof(uint16_t), { 1, m_pConfigTemp->frames }, ippRndNear, 0);

			///ippsAddC_16s_ISfs(m_pConfigTemp->circOffset, (Ipp16s *)lcontour_16u.raw_ptr(), lcontour_16u.length(), 0);
			///m_pImageView_Longi->setContour(2 * m_pConfigTemp->frames, lcontour_16u);
		}
	}
	else
	{
		if (m_contourMap.length() == 0)
		{
			if (m_pLumenDetection)
			{
				delete m_pLumenDetection;
				m_pLumenDetection = nullptr;				
			}
		}
		m_pImageView_CircImage->setContour(0, nullptr);
		m_pImageView_Longi->setContour(0, nullptr);
	}

	m_pImageView_CircImage->getRender()->update();
	m_pImageView_Longi->getRender()->update();
}

void QViewTab::lumenContourDetection()
{
	QString gw_path = m_pResultTab->getRecordInfo().filename;
	if (!m_pConfigTemp->is_dotter)
		gw_path.replace("pullback.data", "gw_pos.csv");
	else
		gw_path.replace(".xml", "/gw_pos.csv");
	
	QString lumen_contour_path = m_pResultTab->getRecordInfo().filename;
	if (!m_pConfigTemp->is_dotter)
		lumen_contour_path.replace("pullback.data", "lumen_contour.map");
	else
		lumen_contour_path.replace(".xml", "/lumen_contour.map");

	QString area_path = m_pResultTab->getRecordInfo().filename;
	if (!m_pConfigTemp->is_dotter)
		area_path.replace("pullback.data", "lumen_area.csv");
	else
		area_path.replace(".xml", "/lumen_area.csv");


	QFileInfo check_file(lumen_contour_path);

	if (!(check_file.exists()))
	{
		if (m_contourMap.length() == 0)
		{						
			QMessageBox msg_box(QMessageBox::NoIcon, "Lumen Contour Detection...", "", QMessageBox::NoButton, this);
			msg_box.setStandardButtons(0);
			msg_box.setWindowModality(Qt::WindowModal);
			msg_box.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
			msg_box.move(QApplication::desktop()->screen()->rect().center() - msg_box.rect().center());
			msg_box.setFixedSize(msg_box.width(), msg_box.height());
			msg_box.show();
								
			m_contourMap = np::FloatArray2(m_pConfigTemp->octAlines, m_pConfigTemp->frames);
			for (int i = 0; i < m_pConfigTemp->frames; i++)
				m_gwPoss.push_back(std::vector<int>());

			QFile match_file(gw_path);
			if (!match_file.open(QFile::WriteOnly))
				return;

			int n_pieces = 10;
			np::Array<int> pieces(n_pieces + 1);
			pieces[0] = 0;
			for (int p = 0; p < n_pieces; p++)
				pieces[p + 1] = (int)((double)m_vectorOctImage.size() / (double)n_pieces * (double)(p + 1));

			std::mutex _mutex;

			std::thread *plumdet = new std::thread[n_pieces];
			for (int p = 0; p < n_pieces; p++)
			{
				plumdet[p] = std::thread([&, p, pieces]() {

					LumenDetection *pLumenDetection = new LumenDetection(int(OUTER_SHEATH_POSITION / m_pImageView_CircImage->getRender()->m_dPixelResol), /// - m_pConfigTemp->circOffset, 
						!m_pConfigTemp->is_dotter ? m_pConfigTemp->innerOffsetLength : 0, false);
						///true, m_pConfigTemp->reflectionDistance, m_pConfigTemp->reflectionLevel);

					for (int i = pieces[p]; i < pieces[p + 1]; i++)
					{						
						// Lumen contour detection
						np::FloatArray contour(&m_contourMap(0, i), m_contourMap.size(0));

						np::Uint8Array2 oct_image(m_pConfigTemp->octRadius, m_pConfigTemp->octAlines);
						scaleOctImage(m_vectorOctImage.at(i), oct_image, m_pConfigTemp->reflectionRemoval);
						(*pLumenDetection)(oct_image, contour);
						std::rotate(&contour(0), &contour(contour.length() - m_vibCorrIdx(i)), &contour(contour.length()));
						
						// Writing GW position
						{
							std::unique_lock<std::mutex> lock(_mutex);
							{
								QTextStream stream(&match_file);

								stream << i + 1 << "\t";								
								for (int j = 0; j < pLumenDetection->gw_peaks_exp.size(); j++)
								{				
									int gp = pLumenDetection->gw_peaks_exp.at(j) - m_pConfigTemp->octAlines / 2;
									gp += m_vibCorrIdx(i);
									if (gp > m_pConfigTemp->octAlines)
										gp -= m_pConfigTemp->octAlines;

									m_gwPoss.at(i).push_back(gp);
									stream << gp << "\t";
								}
								stream << "\n";								
							}
						}						
						///qDebug() << QString("%1").arg(i + 1, 3, 10, (QChar)'0') << pLumenDetection->gw_peaks_exp;
					}

					delete pLumenDetection;
					pLumenDetection = nullptr;
				});
			}
			for (int p = 0; p < n_pieces; p++)
				plumdet[p].join();

			// Compensating circ offset
			ippsAddC_32f_I(-m_pConfigTemp->circOffset, m_contourMap.raw_ptr(), m_contourMap.length());

			// Recording
			QFile file(lumen_contour_path);
			file.open(QIODevice::WriteOnly);
			file.write(reinterpret_cast<const char*>(m_contourMap.raw_ptr()), sizeof(float) * m_contourMap.length());
			file.close();

			match_file.close();
		}
	}
	else
	{
		m_contourMap = np::FloatArray2(m_pConfigTemp->octAlines, m_pConfigTemp->frames);
		for (int i = 0; i < m_pConfigTemp->frames; i++)
			m_gwPoss.push_back(std::vector<int>());

		// Loading (contour)
		{
			QFile file(lumen_contour_path);
			file.open(QIODevice::ReadOnly);
			file.read(reinterpret_cast<char*>(m_contourMap.raw_ptr()), sizeof(float) * m_contourMap.length());
			file.close();
		}

		// Loading (gw pos)
		{
			QFile file(gw_path);
			if (file.open(QFile::ReadOnly))
			{
				QTextStream stream(&file);

				stream.readLine();
				while (!stream.atEnd())
				{
					QStringList gw_pos = stream.readLine().split('\t');
					
					for (int i = 1; i < gw_pos.size()-1; i++)
						m_gwPoss.at(gw_pos.at(0).toInt() - 1).push_back(gw_pos.at(i).toInt());
				}
				file.close();
			}
		}
	}

	// Find lumen area
	np::FloatArray2 contourMap1(m_pConfigTemp->octAlines - 1, m_pConfigTemp->frames);
	np::FloatArray2 contourMap2(m_pConfigTemp->octAlines - 1, m_pConfigTemp->frames);

	ippiCopy_32f_C1R(&m_contourMap(0, 0), sizeof(float) * m_contourMap.size(0), 
		contourMap1, sizeof(float) * contourMap1.size(0), { m_pConfigTemp->octAlines - 1, m_pConfigTemp->frames });
	ippiCopy_32f_C1R(&m_contourMap(1, 0), sizeof(float) * m_contourMap.size(0), 
		contourMap2, sizeof(float) * contourMap2.size(0), { m_pConfigTemp->octAlines - 1, m_pConfigTemp->frames });

	ippsAddC_32f_I((float)m_pConfigTemp->circOffset, contourMap1, contourMap1.length());
	ippsAddC_32f_I((float)m_pConfigTemp->circOffset, contourMap2, contourMap2.length());
	
	ippsMul_32f_I(contourMap1, contourMap2, contourMap2.length());

	np::FloatArray lumen_area(m_pConfigTemp->frames);
	for (int i = 0; i < m_pConfigTemp->frames; i++)
		ippsSum_32f(&contourMap2(0, i), contourMap2.size(0), &lumen_area(i), ippAlgHintAccurate);
		
	ippsMulC_32f_I(0.5 * sin(IPP_2PI / m_pConfigTemp->octAlines), lumen_area, lumen_area.length());
	ippsMulC_32f_I(m_pImageView_CircImage->getRender()->m_dPixelResol / 1000.0, lumen_area, lumen_area.length());
	ippsMulC_32f_I(m_pImageView_CircImage->getRender()->m_dPixelResol / 1000.0, lumen_area, lumen_area.length());

	QFile area_file(area_path);
	if (area_file.open(QFile::WriteOnly))
	{
		QTextStream stream(&area_file);	
		for (int i = 0; i < lumen_area.size(0); i++)
			stream << i + 1 << "\t" << lumen_area.at(i) << "\n";
		area_file.close();
	}

	/// Vectorization of guide-wire positions
	///m_gwVec.clear();
	///for (int i = 0; i < m_gwPoss.size(); i++)
	///	for (int j = 0; j < m_gwPoss.at(i).size(); j++)
	///		m_gwVec.push_back(i * m_pConfigTemp->octAlines + m_gwPoss.at(i).at(j));

	///m_gwVecDiff.clear();
	///for (int i = 0; i < m_gwVec.size() - 1; i++)	
	///	m_gwVecDiff.push_back(m_gwVec.at(i + 1) - m_gwVec.at(i));

	// Set widgets
	m_pToggleButton_AutoContour->setChecked(true);	
	m_pPushButton_LumenDetection->setDisabled(true);
	m_pToggleButton_DiameterView->setEnabled(true);

	invalidate();
}

void QViewTab::calculateStatistics(bool toggled)
{
	if (toggled)
	{
		if (m_peakLifetime.length() == 0)
		{
			m_peakLifetime = np::FloatArray2(m_pConfigTemp->frames, 3);
			m_avgLifetime = np::FloatArray2(m_pConfigTemp->frames, 3);

			for (int ch = 0; ch < 3; ch++)
			{
				// intensity & lifetime map at certain channel
				np::FloatArray2& intensity = m_intensityMap.at(ch);
				np::FloatArray2& lifetime = m_lifetimeMap.at(ch);

				// norm intensity to calculate weighted average lifetime
				np::Uint8Array2 norm_intensity0(intensity.size(0), intensity.size(1));
				np::FloatArray2 norm_intensity(intensity.size(0), intensity.size(1));

				ippiScale_32f8u_C1R(intensity, sizeof(float) * intensity.size(0), norm_intensity0, sizeof(uint8_t) * norm_intensity0.size(0),
				{ intensity.size(0), intensity.size(1) }, m_pConfig->flimIntensityRange[ch].min, m_pConfig->flimIntensityRange[ch].max);
				ippsConvert_8u32f(norm_intensity0, norm_intensity, norm_intensity.length());
				ippsDivC_32f_I(255.0f, norm_intensity, norm_intensity.length());

				np::FloatArray2 weighted_lifetime(lifetime.size(0), lifetime.size(1));
				ippsMul_32f(norm_intensity, lifetime, weighted_lifetime, lifetime.length());

				// sort lifetime to calculate peak lifetime
				np::FloatArray2 sort_lifetime(lifetime.size(0), lifetime.size(1));
				memcpy(sort_lifetime, lifetime, sizeof(float) * lifetime.length());
				
				float mask_thres = 0.5f;
				np::Uint8Array2 weight_mask0(intensity.size(0), intensity.size(1));
				np::FloatArray2 weight_mask(intensity.size(0), intensity.size(1));				
				ippiCompareC_32f_C1R(norm_intensity, sizeof(float) * norm_intensity.size(0), mask_thres,
					weight_mask0, sizeof(uint8_t) * weight_mask0.size(0), { norm_intensity.size(0), norm_intensity.size(1) }, ippCmpGreaterEq);
				ippsConvert_8u32f(weight_mask0, weight_mask, weight_mask.length());
				ippsDivC_32f_I(255.0f, weight_mask, weight_mask.length());
				ippsMul_32f_I(weight_mask, sort_lifetime, sort_lifetime.length());

				for (int i = 0; i < lifetime.size(1); i++)
				{
					// Find peak lifetime at certain frame
					ippsSortDescend_32f_I(&sort_lifetime(0, i), sort_lifetime.size(0));
					ippsMean_32f(&sort_lifetime(0, i), 10, &m_peakLifetime(i, ch), ippAlgHintAccurate);

					// Find average lifetime at certain frame
					float sum_temp;
					ippsSum_32f(&weighted_lifetime(0, i), lifetime.size(0), &m_avgLifetime(i, ch), ippAlgHintAccurate);
					ippsSum_32f(&norm_intensity(0, i), intensity.size(0), &sum_temp, ippAlgHintAccurate);
					m_avgLifetime(i, ch) /= sum_temp;
				}
			}
		}

		int range_start = m_pConfigTemp->quantitationRange.min;
		int range_length = m_pConfigTemp->quantitationRange.max - m_pConfigTemp->quantitationRange.min + 1;

		QString str = QString("[Statistics] - %1 (%2, %3)\n\tpeak\tavg\tpeakavg\tavgpeak\n")
			.arg(getCurrentFrame() + 1)
			.arg(m_pConfigTemp->quantitationRange.min + 1).arg(m_pConfigTemp->quantitationRange.max + 1);
		for (int ch = 0; ch < 3; ch++)
		{
			float peak_avg;
			np::FloatArray avg_lifetime(range_length);
			memcpy(avg_lifetime, &m_avgLifetime(range_start, ch), sizeof(float) * range_length);
			ippsSortDescend_32f_I(avg_lifetime, avg_lifetime.length());
			ippsMean_32f(avg_lifetime, 10, &peak_avg, ippAlgHintAccurate);

			float avg_peak;
			ippsMean_32f(&m_peakLifetime(m_pConfigTemp->quantitationRange.min, ch),
				range_length, &avg_peak, ippAlgHintAccurate);

			str += QString("ch%1 lt :\t%2\t%3\t%4\t%5\tns\n").arg(ch + 1)
				.arg(m_peakLifetime(getCurrentFrame(), ch), 4, 'f', 3)
				.arg(m_avgLifetime(getCurrentFrame(), ch), 4, 'f', 3)
				.arg(peak_avg, 4, 'f', 3).arg(avg_peak, 4, 'f', 3);
		}
		m_pLabel_Statistics->setText(str);

		QString pt_name = m_pResultTab->getRecordInfo().patientName;
		QString acq_date = m_pResultTab->getRecordInfo().date;

		QString label = QString("[%1] %2\n").arg(pt_name).arg(acq_date) + str;
		QApplication::clipboard()->setText(label);
	}
	else
		m_pLabel_Statistics->setText("");
}

void QViewTab::setDiameterView(bool toggled)
{
	visualizeLongiImage(getCurrentAline());
}

void QViewTab::changeVisualizationMode(int mode)
{
	if (mode == _FLIM_PARAMETERS_)
	{
		m_pComboBox_FLImParameters->setEnabled(true);
		m_pComboBox_MLPrediction->setDisabled(true);
	}
	else if (mode == _ML_PREDICTION_)
	{
		m_pComboBox_FLImParameters->setCurrentIndex(1); // Alway designating ch2 lifetime

		m_pComboBox_FLImParameters->setDisabled(true);
		m_pComboBox_MLPrediction->setEnabled(true);
	}
	
	if (m_pResultTab->getSettingDlg())
	{
		if (m_pResultTab->getSettingDlg()->getViewOptionTab())
		{
			if (mode == _FLIM_PARAMETERS_)
				m_pResultTab->getSettingDlg()->getViewOptionTab()->getRadioButtonFLImParameters()->setChecked(true);
			else if (mode == _ML_PREDICTION_)
				m_pResultTab->getSettingDlg()->getViewOptionTab()->getRadioButtonMLPrediction()->setChecked(true);
			m_pResultTab->getSettingDlg()->getViewOptionTab()->changeVisualizationMode(mode + 2);
		}
	}
	else
		invalidate();
}

void QViewTab::changeEmissionChannel(int index)
{	
	int ch = index % 3;
	int mode = index / 3;

	m_pConfig->writeToLog(QString("Changed emission channel: [%1 ==> %2].").arg(m_pConfig->flimEmissionChannel).arg(ch + 1));

	m_pConfig->flimEmissionChannel = ch + 1;
	m_pConfig->flimParameterMode = mode;
		
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
			m_pResultTab->getSettingDlg()->getViewOptionTab()->changeEmissionChannel(ch + 3);
		}
	}
	else
		invalidate();
}

void QViewTab::changeMLPrediction(int mode)
{
	m_pConfig->writeToLog(QString("Changed ML prediction: [%1 ==> %2].").arg(m_pConfig->mlPredictionMode).arg(mode));

	m_pConfig->mlPredictionMode = mode;

	if (m_pResultTab->getSettingDlg())
	{
		if (m_pResultTab->getSettingDlg()->getViewOptionTab())
		{
			if (mode == _RF_COMPO_)
				m_pResultTab->getSettingDlg()->getViewOptionTab()->getRadioButtonRandomForest()->setChecked(true);			
			else if (mode == _SVM_SOFTMAX_)
				m_pResultTab->getSettingDlg()->getViewOptionTab()->getRadioButtonSVMSoftmax()->setChecked(true);
			else if (mode == _SVM_LOGISTICS_)
				m_pResultTab->getSettingDlg()->getViewOptionTab()->getRadioButtonSVMLogistics()->setChecked(true);
			m_pResultTab->getSettingDlg()->getViewOptionTab()->changeMLPrediction(mode + 3);
		}
	}
	else
		invalidate();
}


void QViewTab::scaleOctImage(np::Uint8Array2& oct_input, np::Uint8Array2& oct_output, bool reflection_removal)
{
	// OCT Visualization
	IppiSize roi_oct = { oct_input.size(0), oct_input.size(1) };
	np::Uint8Array2 oct_otemp(roi_oct.width, roi_oct.height);
	memset(oct_otemp, 0, sizeof(uint8_t) * oct_otemp.length());

#ifndef NEXT_GEN_SYSTEM
	np::FloatArray2 scale_temp(roi_oct.width, roi_oct.height);
	ippsConvert_8u32f(oct_input.raw_ptr(), scale_temp.raw_ptr(), scale_temp.length());
	if (reflection_removal)
	{
		np::FloatArray2 reflection_temp(roi_oct.width, roi_oct.height);
		ippiCopy_32f_C1R(&scale_temp(m_pConfigTemp->reflectionDistance, 0), sizeof(float) * scale_temp.size(0),
			&reflection_temp(0, 0), sizeof(float) * reflection_temp.size(0),
			{ roi_oct.width - m_pConfigTemp->reflectionDistance, roi_oct.height });
		ippsMulC_32f_I(m_pConfigTemp->reflectionLevel, reflection_temp, reflection_temp.length());
		ippsSub_32f_I(reflection_temp, scale_temp, scale_temp.length());
		ippiScale_32f8u_C1R(scale_temp.raw_ptr(), roi_oct.width * sizeof(float),
			oct_otemp.raw_ptr(), roi_oct.width * sizeof(uint8_t), roi_oct,
			(float)m_pConfigTemp->octGrayRange.min, (float)m_pConfigTemp->octGrayRange.max * 0.9f);
	}
	else
		ippiScale_32f8u_C1R(scale_temp.raw_ptr(), roi_oct.width * sizeof(float),
			oct_otemp.raw_ptr(), roi_oct.width * sizeof(uint8_t), roi_oct,
			(float)m_pConfigTemp->octGrayRange.min, (float)m_pConfigTemp->octGrayRange.max);

	// Circ shifting
	if (m_pConfigTemp->circOffset > 0)
		ippiCopy_8u_C1R(&oct_otemp(0, 0), sizeof(uint8_t) * oct_otemp.size(0),
			&oct_output(m_pConfigTemp->circOffset, 0), sizeof(uint8_t) * oct_output.size(0),
			{ oct_output.size(0) - m_pConfigTemp->circOffset, oct_output.size(1) });
	else
		ippiCopy_8u_C1R(&oct_otemp(-m_pConfigTemp->circOffset, 0), sizeof(uint8_t) * oct_otemp.size(0),
			&oct_output(0, 0), sizeof(uint8_t) * oct_output.size(0),
			{ oct_output.size(0) + m_pConfigTemp->circOffset, oct_output.size(1) });
#else
	ippiScale_32f8u_C1R(m_vectorOctImage.at(frame).raw_ptr(), roi_oct.width * sizeof(float),
		m_pImgObjRectImage->arr.raw_ptr(), roi_oct.width * sizeof(uint8_t), roi_oct, (float)m_pConfig->axsunDbRange.min, (float)m_pConfig->axsunDbRange.max);
#endif
}

void QViewTab::scaleFLImEnFaceMap(ImageObject* pImgObjIntensityMap, ImageObject* pImgObjLifetimeMap, 
	ImageObject* pImgObjIntensityPropMap, ImageObject* pImgObjIntensityRatioMap, 
	ImageObject* pImgObjPlaqueCompositionMap, 
	int vis_mode, int ch, int flim_mode, int ml_mode)
{
	IppiSize roi_flimproj = { m_intensityMap.at(0).size(0), m_intensityMap.at(0).size(1) };
	IppiSize roi_flimproj4 = { roi_flimproj.width, ((roi_flimproj.height + 3) >> 2) << 2 };

	np::FloatArray2 shift_temp(roi_flimproj4.width, roi_flimproj4.height);
	np::Uint8Array2 scale_temp(roi_flimproj4.width, roi_flimproj4.height);

	bool isVibCorrted = m_pResultTab->getVibCorrectionButton()->isChecked();
	int delaySync = !isVibCorrted ? m_pConfigTemp->flimDelaySync : 0;

	// Intensity map			
	makeDelay(m_intensityMap.at(ch), shift_temp, delaySync);
	ippiScale_32f8u_C1R(shift_temp, sizeof(float) * roi_flimproj.width,
		scale_temp, sizeof(uint8_t) * roi_flimproj4.width, roi_flimproj,
		m_pConfig->flimIntensityRange[ch].min, m_pConfig->flimIntensityRange[ch].max);
	ippiTranspose_8u_C1R(scale_temp.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.width,
		pImgObjIntensityMap->arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.height, roi_flimproj4);
	circShift(pImgObjIntensityMap->arr, int(m_pConfigTemp->rotatedAlines / 4));
	///setAxialOffset(pImgObjIntensityMap->arr, m_pConfigTemp->interFrameSync);
	pImgObjIntensityMap->convertRgb();

	///np::FloatArray2 grayMapTemp(roi_flimproj4.height, roi_flimproj4.width);
	///ippsConvert_8u32f(pImgObjIntensityMap->arr.raw_ptr(), grayMapTemp.raw_ptr(), grayMapTemp.length());
	///ippsDivC_32f_I(255.0f, grayMapTemp.raw_ptr(), grayMapTemp.length());
	///ippiTranspose_32f_C1R(grayMapTemp.raw_ptr(), sizeof(float) * grayMapTemp.size(0), 
	///	m_grayMap.raw_ptr(), sizeof(float) * m_grayMap.size(0), { grayMapTemp.size(0), grayMapTemp.size(1) });

	{
		ippiTranspose_8u_C1R(pImgObjIntensityMap->arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.height,
			scale_temp.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.width, { roi_flimproj4.height, roi_flimproj4.width });
		ippsConvert_8u32f(scale_temp.raw_ptr(), m_grayMap.raw_ptr(), m_grayMap.length());
		ippsDivC_32f_I(255.0f, m_grayMap.raw_ptr(), m_grayMap.length());
	}
	
	if (vis_mode == VisualizationMode::_FLIM_PARAMETERS_)
	{
		if (flim_mode == FLImParameters::_LIFETIME_)
		{
			// Lifetime map
			makeDelay(m_lifetimeMap.at(ch), shift_temp, delaySync);
			ippiScale_32f8u_C1R(shift_temp, sizeof(float) * roi_flimproj.width,
				scale_temp, sizeof(uint8_t) * roi_flimproj4.width, roi_flimproj,
				m_pConfig->flimColormapType == FlimColormapType::_HSV_ ? m_pConfig->flimLifetimeRange[ch].min : m_pConfig->flimLifetimeRangeNew[ch].min,
				m_pConfig->flimColormapType == FlimColormapType::_HSV_ ? m_pConfig->flimLifetimeRange[ch].max : m_pConfig->flimLifetimeRangeNew[ch].max);
			ippiTranspose_8u_C1R(scale_temp.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.width,
				pImgObjLifetimeMap->arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.height, roi_flimproj);
			circShift(pImgObjLifetimeMap->arr, int(m_pConfigTemp->rotatedAlines / 4));
			///setAxialOffset(pImgObjLifetimeMap->arr, m_pConfigTemp->interFrameSync);

			// RGB conversion			
			pImgObjLifetimeMap->convertRgb();

			// Intensity-weight lifetime map
			ippsMul_8u_ISfs(pImgObjIntensityMap->qrgbimg.bits(), pImgObjLifetimeMap->qrgbimg.bits(), pImgObjIntensityMap->qrgbimg.byteCount(), 8);
		}
		else if (flim_mode == FLImParameters::_INTENSITY_PROP_)
		{
			// Intensity proportion map
			makeDelay(m_intensityProportionMap.at(ch), shift_temp, delaySync);
			ippiScale_32f8u_C1R(shift_temp, sizeof(float) * roi_flimproj.width,
				scale_temp.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.width, roi_flimproj,
				m_pConfig->flimIntensityPropRange[ch].min, m_pConfig->flimIntensityPropRange[ch].max);
			ippiTranspose_8u_C1R(scale_temp.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.width,
				pImgObjIntensityPropMap->arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.height, roi_flimproj4);
			circShift(pImgObjIntensityPropMap->arr, int(m_pConfigTemp->rotatedAlines / 4));
			///setAxialOffset(pImgObjIntensityPropMap->arr, m_pConfigTemp->interFrameSync);

			// RGB conversion
			pImgObjIntensityPropMap->convertRgb();

			// Intensity-weight intensity proportion map
			ippsMul_8u_ISfs(pImgObjIntensityMap->qrgbimg.bits(), pImgObjIntensityPropMap->qrgbimg.bits(), pImgObjIntensityMap->qrgbimg.byteCount(), 8);
		}
		else if (flim_mode == FLImParameters::_INTENSITY_RATIO_)
		{
			// Intensity ratio map
			makeDelay(m_intensityRatioMap.at(ch), shift_temp, delaySync);
			ippiScale_32f8u_C1R(shift_temp, sizeof(float) * roi_flimproj.width,
				scale_temp.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.width, roi_flimproj,
				m_pConfig->flimIntensityRatioRange[ch].min, m_pConfig->flimIntensityRatioRange[ch].max);
			ippiTranspose_8u_C1R(scale_temp.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.width,
				pImgObjIntensityRatioMap->arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.height, roi_flimproj4);
			circShift(pImgObjIntensityRatioMap->arr, int(m_pConfigTemp->rotatedAlines / 4));
			///setAxialOffset(pImgObjIntensityRatioMap->arr, m_pConfigTemp->interFrameSync);

			// RGB conversion
			pImgObjIntensityRatioMap->convertRgb();

			// Intensity-weight intensity ratio map
			ippsMul_8u_ISfs(pImgObjIntensityMap->qrgbimg.bits(), pImgObjIntensityRatioMap->qrgbimg.bits(), pImgObjIntensityMap->qrgbimg.byteCount(), 8);
		}
	}
	else if (vis_mode == VisualizationMode::_ML_PREDICTION_)
	{		
		// Plaque composition map
		if (m_plaqueCompositionMap.at(ml_mode).length() != 0)
		{
			np::Uint8Array2 scale_temp3(3 * roi_flimproj4.width, roi_flimproj4.height);

			ippsConvert_32f8u_Sfs(m_plaqueCompositionMap.at(ml_mode), scale_temp3, m_plaqueCompositionMap.at(ml_mode).length(), ippRndNear, 0);
			ippiTranspose_8u_C3R(scale_temp3.raw_ptr(), sizeof(uint8_t) * 3 * roi_flimproj4.width,
				pImgObjPlaqueCompositionMap->qrgbimg.bits(), sizeof(uint8_t) * 3 * roi_flimproj4.height, roi_flimproj);

			for (int j = 0; j < 3; j++)
			{
				np::Uint8Array2 temp_arr(roi_flimproj4.height, roi_flimproj4.width);
				ippiCopy_8u_C3C1R(pImgObjPlaqueCompositionMap->qrgbimg.bits() + j, sizeof(uint8_t) * 3 * roi_flimproj4.height,
					temp_arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.height, { roi_flimproj4.height, roi_flimproj4.width });
				circShift(temp_arr, int(m_pConfigTemp->rotatedAlines / 4));
				///setAxialOffset(temp_arr, m_pConfigTemp->interFrameSync);
				ippiCopy_8u_C1C3R(temp_arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj4.height,
					pImgObjPlaqueCompositionMap->qrgbimg.bits() + j, sizeof(uint8_t) * 3 * roi_flimproj4.height, { roi_flimproj4.height, roi_flimproj4.width });
			}

			// Intensity-weight lifetime map
			ippsMul_8u_ISfs(pImgObjIntensityMap->qrgbimg.bits(), pImgObjPlaqueCompositionMap->qrgbimg.bits(), pImgObjIntensityMap->qrgbimg.byteCount(), 8);
		}
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

void QViewTab::makeDelay(np::FloatArray2& input, np::FloatArray2& output, int delay)
{
	if (delay != 0)
	{
		int delay0 = delay / 4;
		int delay1 = delay0 + 1;
		float weight0 = 1.0f - (((float)delay / 4.0f) - delay0);
		float weight1 = 1.0f - weight0;

		np::FloatArray temp0(output.length()); memset(temp0, 0, sizeof(float) * temp0.length());
		np::FloatArray temp1(output.length()); memset(temp1, 0, sizeof(float) * temp1.length());

		memcpy(temp0 + delay0, input, sizeof(float) * (input.length() - delay0));
		memcpy(temp1 + delay1, input, sizeof(float) * (input.length() - delay1));

		ippsMulC_32f_I(weight0, temp0, temp0.length());
		ippsMulC_32f_I(weight1, temp1, temp1.length());

		ippsAdd_32f(temp0, temp1, output, output.length());
	}
	else
	{
		memcpy(output, input, sizeof(float) * input.length());
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


void QViewTab::vibrationCorrection()
{
	// Get vib correction index
	QString vib_corr_path = m_pResultTab->getRecordInfo().filename;
	if (!m_pConfigTemp->is_dotter)
		vib_corr_path.replace("pullback.data", "vib_corr.idx");
	else
		vib_corr_path.replace(".xml", "/vib_corr.idx");
	
	QFileInfo check_file(vib_corr_path);

	// Synchronized FLIm map
	std::vector<np::FloatArray2> syncIntensityMap;
	std::vector<np::FloatArray2> syncLifetimeMap;
	for (int i = 0; i < 3; i++)
	{
		np::FloatArray2 syncIntensityMap0(m_intensityMap.at(i).size(0), m_intensityMap.at(i).size(1));
		memset(syncIntensityMap0, 0, sizeof(float) * syncIntensityMap0.length());
		makeDelay(m_intensityMap.at(i), syncIntensityMap0, m_pConfigTemp->flimDelaySync);
		syncIntensityMap.push_back(syncIntensityMap0);

		np::FloatArray2 syncLifetimeMap0(m_lifetimeMap.at(i).size(0), m_lifetimeMap.at(i).size(1));
		memset(syncLifetimeMap0, 0, sizeof(float) * syncLifetimeMap0.length());
		makeDelay(m_lifetimeMap.at(i), syncLifetimeMap0, m_pConfigTemp->flimDelaySync);
		syncLifetimeMap.push_back(syncLifetimeMap0);
	}

	if (!(check_file.exists()))
	{
		int d_smp_factor = !m_pConfigTemp->is_dotter ? 16 : 13;

		// Data size specification
		IppiSize img_size = { m_vectorOctImage.at(0).size(0), m_vectorOctImage.at(0).size(1) };
		IppiSize img_size16 = { m_vectorOctImage.at(0).size(0) / d_smp_factor, m_vectorOctImage.at(0).size(1) };

		// Spline parameters		
		MKL_INT dorder = 1;
		MKL_INT nx = (int)m_vectorOctImage.at(0).size(1) / d_smp_factor + 1; // original data length
		MKL_INT nsite = (int)m_vectorOctImage.at(0).size(1) + 1; // interpolated data length		
		float x[2] = { 0.0f, (float)nx - 1.0f }; // data range
		float* scoeff = new float[(nx - 1) * DF_PP_CUBIC];

		// Find vibration correction index
		memset(m_vibCorrIdx, 0, sizeof(uint16_t) * m_vibCorrIdx.length());
		for (int i = 0; i < (int)m_vectorOctImage.size() - 1; i++)
		{
			// Fixed
			np::Uint8Array2 fixed_8u(img_size16.width, img_size16.height);
			np::FloatArray2 fixed_32f(img_size16.width, img_size16.height);
			ippiCopy_8u_C1R(m_vectorOctImage.at(i).raw_ptr(), sizeof(uint8_t) * d_smp_factor,
				fixed_8u.raw_ptr(), sizeof(uint8_t) * 1, { 1, img_size16.width * img_size16.height });
			ippsConvert_8u32f(fixed_8u, fixed_32f, fixed_8u.length());

			Ipp32f x_mean, x_std;
			ippsMeanStdDev_32f(fixed_32f, fixed_32f.length(), &x_mean, &x_std, ippAlgHintFast);

			np::FloatArray fixed_vector(fixed_32f.length());
			ippsSubC_32f(fixed_32f, x_mean, fixed_vector, fixed_vector.length());

			// Moving
			np::Uint8Array2 moving_8u(img_size16.width, img_size16.height);
			ippiCopy_8u_C1R(m_vectorOctImage.at(i + 1).raw_ptr(), sizeof(uint8_t) * d_smp_factor,
				moving_8u.raw_ptr(), sizeof(uint8_t) * 1, { 1, img_size16.width * img_size16.height });

			// Correlation buffers
			np::FloatArray corr_coefs0(nx);
			np::FloatArray corr_coefs(nsite);

			// Shifting along A-line dimension
			for (int j = 0; j < corr_coefs0.length(); j++)
			{
				circShift(moving_8u, d_smp_factor);
				np::FloatArray2 moving_32f(img_size16.width, img_size16.height);
				ippsConvert_8u32f(moving_8u, moving_32f, moving_8u.length());

				Ipp32f y_mean, y_std;
				ippsMeanStdDev_32f(moving_32f, moving_32f.length(), &y_mean, &y_std, ippAlgHintFast);

				// Correlation coefficient
				np::FloatArray moving_vector(moving_32f.length());
				ippsSubC_32f(moving_32f, y_mean, moving_vector, moving_vector.length());
				ippsMul_32f_I(fixed_vector, moving_vector, moving_vector.length());

				Ipp32f cov;
				ippsSum_32f(moving_vector, moving_vector.length(), &cov, ippAlgHintFast);
				cov = cov / (moving_vector.length() - 1);

				int j1 = (j + 1) % corr_coefs0.length();
				corr_coefs0(j1) = cov / x_std / y_std;
			}

			// Spline interpolation		
			DFTaskPtr task1;
			dfsNewTask1D(&task1, nx, x, DF_UNIFORM_PARTITION, 1, corr_coefs0.raw_ptr(), DF_MATRIX_STORAGE_ROWS);
			dfsEditPPSpline1D(task1, DF_PP_CUBIC, DF_PP_NATURAL, DF_BC_NOT_A_KNOT, 0, DF_NO_IC, 0, scoeff, DF_NO_HINT);
			dfsConstruct1D(task1, DF_PP_SPLINE, DF_METHOD_STD);
			dfsInterpolate1D(task1, DF_INTERP, DF_METHOD_PP, nsite, x, DF_UNIFORM_PARTITION, 1, &dorder,
				DF_NO_APRIORI_INFO, corr_coefs.raw_ptr(), DF_MATRIX_STORAGE_ROWS, NULL);
			dfDeleteTask(&task1);
			mkl_free_buffers();

			// Find correction index
			Ipp32f cmax; int cidx;
			ippsMaxIndx_32f(corr_coefs.raw_ptr(), corr_coefs.length() - 1, &cmax, &cidx);

			// OCT correction
			circShift(m_vectorOctImage.at(i + 1), cidx);
			std::rotate(&m_octProjection(0, i + 1), &m_octProjection(cidx, i + 1), &m_octProjection(m_octProjection.size(0), i + 1));

			// FLIm correction
			///int i1 = i + 1 - m_pConfigTemp->interFrameSync;
			///if ((i1 > 0) && (i1 < (int)m_vectorOctImage.size()))
			int shift = cidx / 4;
			float weight = (float)cidx / 4.0f - (float)shift;

			np::FloatArray2 intensity_temp(syncIntensityMap.at(0).size(0), 2);
			np::FloatArray2 lifetime_temp(syncLifetimeMap.at(0).size(0), 2);
			for (int ch = 0; ch < 3; ch++)
			{
				memcpy(&intensity_temp(0, 0), &syncIntensityMap.at(ch)(0, i + 1), sizeof(float) * syncIntensityMap.at(ch).size(0));
				memcpy(&intensity_temp(0, 1), &syncIntensityMap.at(ch)(0, i + 1), sizeof(float) * syncIntensityMap.at(ch).size(0));
				std::rotate(&intensity_temp(0, 0), &intensity_temp(shift, 0), &intensity_temp(intensity_temp.size(0), 0));
				std::rotate(&intensity_temp(0, 1), &intensity_temp((shift + 1) % intensity_temp.size(0), 1), &intensity_temp(intensity_temp.size(0), 1));
				ippsMulC_32f_I(1.0f - weight, &intensity_temp(0, 0), intensity_temp.size(0));
				ippsMulC_32f_I(weight, &intensity_temp(0, 1), intensity_temp.size(0));
				ippsAdd_32f(&intensity_temp(0, 1), &intensity_temp(0, 0), &m_intensityMap.at(ch)(0, i + 1), intensity_temp.size(0));

				memcpy(&lifetime_temp(0, 0), &syncLifetimeMap.at(ch)(0, i + 1), sizeof(float) * syncLifetimeMap.at(ch).size(0));
				memcpy(&lifetime_temp(0, 1), &syncLifetimeMap.at(ch)(0, i + 1), sizeof(float) * syncLifetimeMap.at(ch).size(0));
				std::rotate(&lifetime_temp(0, 0), &lifetime_temp(shift, 0), &lifetime_temp(lifetime_temp.size(0), 0));
				std::rotate(&lifetime_temp(0, 1), &lifetime_temp((shift + 1) % lifetime_temp.size(0), 1), &lifetime_temp(lifetime_temp.size(0), 1));
				ippsMulC_32f_I(1.0f - weight, &lifetime_temp(0, 0), lifetime_temp.size(0));
				ippsMulC_32f_I(weight, &lifetime_temp(0, 1), lifetime_temp.size(0));
				ippsAdd_32f(&lifetime_temp(0, 1), &lifetime_temp(0, 0), &m_lifetimeMap.at(ch)(0, i + 1), lifetime_temp.size(0));
			}

			m_vibCorrIdx(i + 1) = cidx;
		}

		delete[] scoeff;

		// Recording
		QFile file(vib_corr_path);
		file.open(QIODevice::WriteOnly);
		file.write(reinterpret_cast<const char*>(m_vibCorrIdx.raw_ptr()), sizeof(uint16_t) * m_vibCorrIdx.length());
		file.close();
	}
	else
	{
		// Loading
		QFile file(vib_corr_path);
		file.open(QIODevice::ReadOnly);
		file.read(reinterpret_cast<char*>(m_vibCorrIdx.raw_ptr()), sizeof(uint16_t) * m_vibCorrIdx.length());
		file.close();

		// Vibration correction		
		for (int i = 1; i < (int)m_vectorOctImage.size(); i++)
		{
			// OCT correction
			circShift(m_vectorOctImage.at(i), m_vibCorrIdx(i));
			std::rotate(&m_octProjection(0, i), &m_octProjection(m_vibCorrIdx(i), i), &m_octProjection(m_octProjection.size(0), i));

			// FLIm correction
			int shift = m_vibCorrIdx(i) / 4;
			float weight = (float)m_vibCorrIdx(i) / 4.0f - (float)shift;

			np::FloatArray2 intensity_temp(syncIntensityMap.at(0).size(0), 2);
			np::FloatArray2 lifetime_temp(syncLifetimeMap.at(0).size(0), 2);
			for (int ch = 0; ch < 3; ch++)
			{
				memcpy(&intensity_temp(0, 0), &syncIntensityMap.at(ch)(0, i), sizeof(float) * syncIntensityMap.at(ch).size(0));
				memcpy(&intensity_temp(0, 1), &syncIntensityMap.at(ch)(0, i), sizeof(float) * syncIntensityMap.at(ch).size(0));
				std::rotate(&intensity_temp(0, 0), &intensity_temp(shift, 0), &intensity_temp(intensity_temp.size(0), 0));
				std::rotate(&intensity_temp(0, 1), &intensity_temp((shift + 1) % intensity_temp.size(0), 1), &intensity_temp(intensity_temp.size(0), 1));
				ippsMulC_32f_I(1.0f - weight, &intensity_temp(0, 0), intensity_temp.size(0));
				ippsMulC_32f_I(weight, &intensity_temp(0, 1), intensity_temp.size(0));
				ippsAdd_32f(&intensity_temp(0, 1), &intensity_temp(0, 0), &m_intensityMap.at(ch)(0, i), intensity_temp.size(0));

				memcpy(&lifetime_temp(0, 0), &syncLifetimeMap.at(ch)(0, i), sizeof(float) * syncLifetimeMap.at(ch).size(0));
				memcpy(&lifetime_temp(0, 1), &syncLifetimeMap.at(ch)(0, i), sizeof(float) * syncLifetimeMap.at(ch).size(0));
				std::rotate(&lifetime_temp(0, 0), &lifetime_temp(shift, 0), &lifetime_temp(lifetime_temp.size(0), 0));
				std::rotate(&lifetime_temp(0, 1), &lifetime_temp((shift + 1) % lifetime_temp.size(0), 1), &lifetime_temp(lifetime_temp.size(0), 1));
				ippsMulC_32f_I(1.0f - weight, &lifetime_temp(0, 0), lifetime_temp.size(0));
				ippsMulC_32f_I(weight, &lifetime_temp(0, 1), lifetime_temp.size(0));
				ippsAdd_32f(&lifetime_temp(0, 1), &lifetime_temp(0, 0), &m_lifetimeMap.at(ch)(0, i), lifetime_temp.size(0));
			}
		}
	}
}


void QViewTab::pickFrame(std::vector<QStringList>& _vector, int oct_frame, int ivus_frame, int rotation, bool allow_delete)
{
	// Add matching data to vector
	int k = 0;
	foreach(QStringList _matches, _vector)
	{
		if (_matches.at(0).toInt() == oct_frame)
			break;
		k++;
	}

	QStringList matches;
	matches << QString::number(oct_frame) // OCT frame
		<< QString::number(ivus_frame) // IVUS frame
		<< QString::number(rotation); // IVUS rotation

	if (k == _vector.size())
		_vector.push_back(matches);
	else
		if (!allow_delete)
			_vector.at(k) = matches;
		else
			if (k != 0)
				_vector.erase(_vector.begin() + k, _vector.begin() + k + 1);
			else
				_vector.clear();

	// Save matching data
	QString match_path = m_pResultTab->getRecordInfo().filename;
	if (!m_pConfigTemp->is_dotter)
		match_path.replace("pullback.data", "ivus_match.csv");
	else
		match_path.replace(".xml", "/ivus_match.csv");

	QFile match_file(match_path);
	if (match_file.open(QFile::WriteOnly))
	{
		QTextStream stream(&match_file);
		stream << "OCT Frame" << "\t"
			<< "IVUS Frame" << "\t"
			<< "Rotation" << "\n";

		for (int i = 0; i < _vector.size(); i++)
		{
			QStringList _matches = _vector.at(i);

			stream << _matches.at(0) << "\t" // OCT frame
				<< _matches.at(1) << "\t" // IVUS frame
				<< _matches.at(2) << "\n"; // IVUS rotation
		}
		match_file.close();
	}

	// Invalidate parent dialog
	invalidate();


	//// Add matching data to vector
	//int oct_frame = getCurrentFrame() + 1;

	//int k = 0;
	//foreach(QStringList _matches, m_vectorPickFrames)
	//{
	//	if (_matches.at(0).toInt() == oct_frame)
	//		break;
	//	k++;
	//}

	//QStringList matches;
	//matches << QString::number(oct_frame) // OCT frame
	//	<< 0 // IVUS frame
	//	<< 0; // IVUS rotation

	//if (k == m_vectorPickFrames.size())
	//	m_vectorPickFrames.push_back(matches);
	//else
	//	m_vectorPickFrames.at(k) = matches;

	//// Save matching data
	//QString match_path = m_pResultTab->getRecordInfo().filename;
	//match_path.replace("pullback.data", "ivus_match.csv");

	//QFile match_file(match_path);
	//if (match_file.open(QFile::WriteOnly))
	//{
	//	QTextStream stream(&match_file);
	//	stream << "OCT Frame" << "\t"
	//		<< "IVUS Frame" << "\t"
	//		<< "Rotation" << "\n";

	//	for (int i = 0; i < m_vectorPickFrames.size(); i++)
	//	{
	//		QStringList _matches = m_vectorPickFrames.at(i);

	//		stream << _matches.at(0) << "\t" // OCT frame
	//			<< _matches.at(1) << "\t" // IVUS frame
	//			<< _matches.at(2) << "\n"; // IVUS rotation
	//	}
	//	match_file.close();
	//}

	//// Invalidate parent dialog
	//invalidate();
}

void QViewTab::loadPickFrames(std::vector<QStringList>& _vector)
{
	_vector.clear();

	QString match_path = m_pResultTab->getRecordInfo().filename;
	if (!m_pConfigTemp->is_dotter)
		match_path.replace("pullback.data", "ivus_match.csv");
	else
		match_path.replace(".xml", "/ivus_match.csv");

	QFile match_file(match_path);
	if (match_file.open(QFile::ReadOnly))
	{
		QTextStream stream(&match_file);

		stream.readLine();
		while (!stream.atEnd())
		{
			QStringList matches = stream.readLine().split('\t');
			if (matches.size() > 1)
				_vector.push_back(matches);
		}
		match_file.close();
	}
}

void QViewTab::seekPickFrame(bool is_right)
{
	int cur_frame = getCurrentFrame();

	np::Array<int, 2> diff((int)m_vectorPickFrames.size() + 2, 2);
	diff(0, 0) = 0; 
	for (int i = 0; i < m_vectorPickFrames.size(); i++)
		diff(i + 1, 0) = m_vectorPickFrames.at(i).at(0).toInt() - 1;
	diff(diff.size(0) - 1, 0) = (int)m_vectorOctImage.size() - 1;
	memcpy(&diff(0, 1), &diff(0, 0), sizeof(int) * diff.size(0));
	ippsSubC_32s_ISfs(cur_frame, &diff(0, 1), diff.size(0), 0);

	if (is_right)
	{
		if (cur_frame == m_vectorOctImage.size() - 1)
			return;

		for (int i = 0; i < diff.size(0); i++)
			if (diff(i, 1) <= 0)
				diff(i, 1) = (int)m_vectorOctImage.size();
		
		Ipp32s min_val, min_idx;
		ippsMinIndx_32s(&diff(0, 1), diff.size(0), &min_val, &min_idx);
		m_pSlider_SelectFrame->setValue(diff(min_idx, 0));
	}
	else
	{
		for (int i = 0; i < diff.size(0); i++)
			if (diff(i, 1) >= 0)
				diff(i, 1) = -(int)m_vectorOctImage.size();

		Ipp32s max_val, max_idx;
		ippsMaxIndx_32s(&diff(0, 1), diff.size(0), &max_val, &max_idx);
		m_pSlider_SelectFrame->setValue(diff(max_idx, 0));
	}
}
