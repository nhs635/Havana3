
#include "SaveResultDlg.h"

#include <Havana3/MainWindow.h>
#include <Havana3/QResultTab.h>
#include <Havana3/QProcessingTab.h>
#include <Havana3/QVisualizationTab.h>

#include <Havana3/Viewer/QImageView.h>

#include <Havana3/Dialog/LongitudinalViewDlg.h>

#include <ippi.h>
#include <ippcc.h>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

#include <iostream>
#include <thread>
#include <chrono>
#include <utility>


SaveResultDlg::SaveResultDlg(QWidget *parent) :
    QDialog(parent), m_defaultTransformation(Qt::FastTransformation), m_bIsSaving(false)
{
    // Set default size & frame
    setFixedSize(410, 160);
    setWindowFlags(Qt::Tool);
	setWindowTitle("Save Result");

    // Set main window objects
    m_pProcessingTab = (QProcessingTab*)parent;
	m_pConfig = m_pProcessingTab->getResultTab()->getMainWnd()->m_pConfiguration;
	m_pVisTab = m_pProcessingTab->getResultTab()->getVisualizationTab();

    // Create widgets for saving results (save Ccoss-sections)
	m_pPushButton_SaveCrossSections = new QPushButton(this);
	m_pPushButton_SaveCrossSections->setText("Save Cross-Sections");
	
	m_pCheckBox_RectImage = new QCheckBox(this);
	m_pCheckBox_RectImage->setText("Rect Image");
	m_pCheckBox_CircImage = new QCheckBox(this);
	m_pCheckBox_CircImage->setText("Circ Image");
	m_pCheckBox_LongiImage = new QCheckBox(this);
	m_pCheckBox_LongiImage->setText("Longi Image");

	m_pCheckBox_ResizeRectImage = new QCheckBox(this);
	m_pCheckBox_ResizeRectImage->setText("Resize (w x h)");
	m_pCheckBox_ResizeRectImage->setEnabled(false);
	m_pCheckBox_ResizeCircImage = new QCheckBox(this);
	m_pCheckBox_ResizeCircImage->setText("Resize (diameter)");
	m_pCheckBox_ResizeCircImage->setEnabled(false);
	m_pCheckBox_ResizeLongiImage = new QCheckBox(this);
	m_pCheckBox_ResizeLongiImage->setText("Resize (w x h)");
	m_pCheckBox_ResizeLongiImage->setEnabled(false);

	m_pLineEdit_RectWidth = new QLineEdit(this);
	m_pLineEdit_RectWidth->setFixedWidth(35);
	m_pLineEdit_RectWidth->setText(QString::number(m_pVisTab->m_vectorOctImage.at(0).size(1)));
	m_pLineEdit_RectWidth->setAlignment(Qt::AlignCenter);
	m_pLineEdit_RectWidth->setDisabled(true);
	m_pLineEdit_RectHeight = new QLineEdit(this);
	m_pLineEdit_RectHeight->setFixedWidth(35);
	m_pLineEdit_RectHeight->setText(QString::number(m_pVisTab->m_vectorOctImage.at(0).size(0)));
	m_pLineEdit_RectHeight->setAlignment(Qt::AlignCenter);
	m_pLineEdit_RectHeight->setDisabled(true);
	m_pLineEdit_CircDiameter = new QLineEdit(this);
	m_pLineEdit_CircDiameter->setFixedWidth(35);
	m_pLineEdit_CircDiameter->setText(QString::number(2 * m_pVisTab->m_vectorOctImage.at(0).size(0)));
	m_pLineEdit_CircDiameter->setAlignment(Qt::AlignCenter);
	m_pLineEdit_CircDiameter->setDisabled(true);
	m_pLineEdit_LongiWidth = new QLineEdit(this);
	m_pLineEdit_LongiWidth->setFixedWidth(35);
	m_pLineEdit_LongiWidth->setText(QString::number(m_pVisTab->m_vectorOctImage.size()));
	m_pLineEdit_LongiWidth->setAlignment(Qt::AlignCenter);
	m_pLineEdit_LongiWidth->setDisabled(true);
	m_pLineEdit_LongiHeight = new QLineEdit(this);
	m_pLineEdit_LongiHeight->setFixedWidth(35);
	m_pLineEdit_LongiHeight->setText(QString::number(2 * m_pVisTab->m_vectorOctImage.at(0).size(0)));
	m_pLineEdit_LongiHeight->setAlignment(Qt::AlignCenter);
	m_pLineEdit_LongiHeight->setDisabled(true);

	m_pCheckBox_CrossSectionCh1 = new QCheckBox(this);
	m_pCheckBox_CrossSectionCh1->setText("Channel 1");
	m_pCheckBox_CrossSectionCh2 = new QCheckBox(this);
	m_pCheckBox_CrossSectionCh2->setText("Channel 2");
	m_pCheckBox_CrossSectionCh3 = new QCheckBox(this);
	m_pCheckBox_CrossSectionCh3->setText("Channel 3");

	m_pCheckBox_IntensityWeighting = new QCheckBox(this);
	m_pCheckBox_IntensityWeighting->setText("Intensity Weighted Lifetime Visualization");
	m_pCheckBox_IntensityWeighting->setChecked(false);

	// Set Range
	m_pLabel_Range = new QLabel("Range  ", this);
	m_pLineEdit_RangeStart = new QLineEdit(this);
	m_pLineEdit_RangeStart->setFixedWidth(30);
	m_pLineEdit_RangeStart->setText(QString::number(1));
	m_pLineEdit_RangeStart->setAlignment(Qt::AlignCenter);
	m_pLineEdit_RangeEnd = new QLineEdit(this);
	m_pLineEdit_RangeEnd->setFixedWidth(30);
	m_pLineEdit_RangeEnd->setText(QString::number(m_pVisTab->m_vectorOctImage.size()));
	m_pLineEdit_RangeEnd->setAlignment(Qt::AlignCenter);

	// Save En Face Maps
	m_pPushButton_SaveEnFaceMaps = new QPushButton(this);
	m_pPushButton_SaveEnFaceMaps->setText("Save En Face Maps");

	m_pCheckBox_RawData = new QCheckBox(this);
	m_pCheckBox_RawData->setText("Raw Data");
	m_pCheckBox_ScaledImage = new QCheckBox(this);
	m_pCheckBox_ScaledImage->setText("Scaled Image");

	m_pCheckBox_EnFaceCh1 = new QCheckBox(this);
	m_pCheckBox_EnFaceCh1->setText("Channel 1");
	m_pCheckBox_EnFaceCh2 = new QCheckBox(this);
	m_pCheckBox_EnFaceCh2->setText("Channel 2");
	m_pCheckBox_EnFaceCh3 = new QCheckBox(this);
	m_pCheckBox_EnFaceCh3->setText("Channel 3");

	m_pCheckBox_OctMaxProjection = new QCheckBox(this);
	m_pCheckBox_OctMaxProjection->setText("OCT Max Projection");
	m_pCheckBox_OctMaxProjection->setChecked(false);


	// Set layout
	QGridLayout *pGridLayout = new QGridLayout;
	pGridLayout->setSpacing(3);

	QHBoxLayout *pHBoxLayout_Range = new QHBoxLayout;
	pHBoxLayout_Range->addWidget(m_pLabel_Range);
	pHBoxLayout_Range->addWidget(m_pLineEdit_RangeStart);
	pHBoxLayout_Range->addWidget(m_pLineEdit_RangeEnd);
	pHBoxLayout_Range->addStretch(1);


	pGridLayout->addWidget(m_pPushButton_SaveCrossSections, 0, 0);
	
	QHBoxLayout *pHBoxLayout_RectResize = new QHBoxLayout;
	pHBoxLayout_RectResize->addWidget(m_pCheckBox_ResizeRectImage);
	pHBoxLayout_RectResize->addWidget(m_pLineEdit_RectWidth);
	pHBoxLayout_RectResize->addWidget(m_pLineEdit_RectHeight);
	pHBoxLayout_RectResize->addStretch(1);
	//pHBoxLayout_RectResize->addItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Fixed));

	pGridLayout->addWidget(m_pCheckBox_RectImage, 0, 1);
	pGridLayout->addItem(pHBoxLayout_RectResize, 0, 2, 1, 2);
	pGridLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 4);

	QHBoxLayout *pHBoxLayout_CircResize = new QHBoxLayout;
	pHBoxLayout_CircResize->addWidget(m_pCheckBox_ResizeCircImage);
	pHBoxLayout_CircResize->addWidget(m_pLineEdit_CircDiameter);
	pHBoxLayout_CircResize->addStretch(1);
	//pHBoxLayout_CircResize->addItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Fixed));

	pGridLayout->addWidget(m_pCheckBox_CircImage, 1, 1);
	pGridLayout->addItem(pHBoxLayout_CircResize, 1, 2, 1, 2);

	QHBoxLayout *pHBoxLayout_LongiResize = new QHBoxLayout;
	pHBoxLayout_LongiResize->addWidget(m_pCheckBox_ResizeLongiImage);
	pHBoxLayout_LongiResize->addWidget(m_pLineEdit_LongiWidth);
	pHBoxLayout_LongiResize->addWidget(m_pLineEdit_LongiHeight);
	pHBoxLayout_LongiResize->addStretch(1);
	//pHBoxLayout_RectResize->addItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Fixed));

	pGridLayout->addWidget(m_pCheckBox_LongiImage, 2, 1);
	pGridLayout->addItem(pHBoxLayout_LongiResize, 2, 2, 1, 2);
	
	pGridLayout->addWidget(m_pCheckBox_CrossSectionCh1, 3, 1);
	pGridLayout->addWidget(m_pCheckBox_CrossSectionCh2, 3, 2);
	pGridLayout->addWidget(m_pCheckBox_CrossSectionCh3, 3, 3);

	pGridLayout->addWidget(m_pCheckBox_IntensityWeighting, 4, 1, 1, 3);
	
	pGridLayout->addItem(pHBoxLayout_Range, 1, 0);


	pGridLayout->addWidget(m_pPushButton_SaveEnFaceMaps, 5, 0);

	pGridLayout->addWidget(m_pCheckBox_RawData, 5, 1);
	pGridLayout->addWidget(m_pCheckBox_ScaledImage, 5, 2);

	pGridLayout->addWidget(m_pCheckBox_EnFaceCh1, 6, 1);
	pGridLayout->addWidget(m_pCheckBox_EnFaceCh2, 6, 2);
	pGridLayout->addWidget(m_pCheckBox_EnFaceCh3, 6, 3);

	pGridLayout->addWidget(m_pCheckBox_OctMaxProjection, 7, 1, 1, 2);
	
    setLayout(pGridLayout);

    // Connect
	connect(m_pPushButton_SaveCrossSections, SIGNAL(clicked(bool)), this, SLOT(saveCrossSections()));
	connect(m_pPushButton_SaveEnFaceMaps, SIGNAL(clicked(bool)), this, SLOT(saveEnFaceMaps()));
	
	connect(m_pLineEdit_RangeStart, SIGNAL(textChanged(const QString &)), this, SLOT(setRange()));
	connect(m_pLineEdit_RangeEnd, SIGNAL(textChanged(const QString &)), this, SLOT(setRange()));

	connect(m_pCheckBox_RectImage, SIGNAL(toggled(bool)), this, SLOT(checkRectImage(bool)));
	connect(m_pCheckBox_CircImage, SIGNAL(toggled(bool)), this, SLOT(checkCircImage(bool)));
	connect(m_pCheckBox_LongiImage, SIGNAL(toggled(bool)), this, SLOT(checkLongiImage(bool)));

	connect(m_pCheckBox_ResizeRectImage, SIGNAL(toggled(bool)), this, SLOT(enableRectResize(bool)));
	connect(m_pCheckBox_ResizeCircImage, SIGNAL(toggled(bool)), this, SLOT(enableCircResize(bool)));
	connect(m_pCheckBox_ResizeLongiImage, SIGNAL(toggled(bool)), this, SLOT(enableLongiResize(bool)));

	connect(m_pLineEdit_RectWidth, SIGNAL(textEdited(const QString &)), this, SLOT(checkResizeValue()));
	connect(m_pLineEdit_RectHeight, SIGNAL(textEdited(const QString &)), this, SLOT(checkResizeValue()));
	connect(m_pLineEdit_CircDiameter, SIGNAL(textEdited(const QString &)), this, SLOT(checkResizeValue()));
	connect(m_pLineEdit_LongiWidth, SIGNAL(textEdited(const QString &)), this, SLOT(checkResizeValue()));
	connect(m_pLineEdit_LongiHeight, SIGNAL(textEdited(const QString &)), this, SLOT(checkResizeValue()));

	connect(m_pCheckBox_CrossSectionCh1, SIGNAL(toggled(bool)), this, SLOT(checkChannels()));
	connect(m_pCheckBox_CrossSectionCh2, SIGNAL(toggled(bool)), this, SLOT(checkChannels()));
	connect(m_pCheckBox_CrossSectionCh3, SIGNAL(toggled(bool)), this, SLOT(checkChannels()));

	connect(m_pCheckBox_RawData, SIGNAL(toggled(bool)), this, SLOT(checkEnFaceOptions()));
	connect(m_pCheckBox_ScaledImage, SIGNAL(toggled(bool)), this, SLOT(checkEnFaceOptions()));
	connect(m_pCheckBox_EnFaceCh1, SIGNAL(toggled(bool)), this, SLOT(checkEnFaceOptions()));
	connect(m_pCheckBox_EnFaceCh2, SIGNAL(toggled(bool)), this, SLOT(checkEnFaceOptions()));
	connect(m_pCheckBox_EnFaceCh3, SIGNAL(toggled(bool)), this, SLOT(checkEnFaceOptions()));
	connect(m_pCheckBox_OctMaxProjection, SIGNAL(toggled(bool)), this, SLOT(checkEnFaceOptions()));

	connect(this, SIGNAL(setWidgets(bool)), this, SLOT(setWidgetsEnabled(bool)));
	connect(this, SIGNAL(savedSingleFrame(int)), m_pProcessingTab->getProgressBar(), SLOT(setValue(int)));

	// Initialize
	m_pCheckBox_CircImage->setChecked(true);
	m_pCheckBox_LongiImage->setChecked(true);
	m_pCheckBox_CrossSectionCh1->setChecked(true);
	m_pCheckBox_CrossSectionCh2->setChecked(true);

	m_pCheckBox_ScaledImage->setChecked(true);
	m_pCheckBox_EnFaceCh1->setChecked(true);
	m_pCheckBox_EnFaceCh2->setChecked(true);
}

SaveResultDlg::~SaveResultDlg()
{
}

void SaveResultDlg::closeEvent(QCloseEvent *e)
{
	if (m_bIsSaving)
		e->ignore();
	else
		finished(0);
}

void SaveResultDlg::keyPressEvent(QKeyEvent *e)
{
	if (e->key() != Qt::Key_Escape)
		QDialog::keyPressEvent(e);
}


void SaveResultDlg::saveCrossSections()
{
	std::thread t1([&]() {
		
		// Check Status /////////////////////////////////////////////////////////////////////////////
		CrossSectionCheckList checkList;
		checkList.bRect = m_pCheckBox_RectImage->isChecked();
		checkList.bCirc = m_pCheckBox_CircImage->isChecked();
		checkList.bLongi = m_pCheckBox_LongiImage->isChecked();
		checkList.bRectResize = m_pCheckBox_ResizeRectImage->isChecked();
		checkList.bCircResize = m_pCheckBox_ResizeCircImage->isChecked();
		checkList.bLongiResize = m_pCheckBox_ResizeLongiImage->isChecked();
		checkList.nRectWidth = m_pLineEdit_RectWidth->text().toInt();
		checkList.nRectHeight = m_pLineEdit_RectHeight->text().toInt();
		checkList.nCircDiameter = m_pLineEdit_CircDiameter->text().toInt();
		checkList.nLongiWidth = m_pLineEdit_LongiWidth->text().toInt();
		checkList.nLongiHeight = m_pLineEdit_LongiHeight->text().toInt();

		checkList.bCh[0] = m_pCheckBox_CrossSectionCh1->isChecked();
		checkList.bCh[1] = m_pCheckBox_CrossSectionCh2->isChecked();
		checkList.bCh[2] = m_pCheckBox_CrossSectionCh3->isChecked();
		checkList.bMulti = m_pCheckBox_IntensityWeighting->isChecked();
		
		// Median filtering for FLIM maps ///////////////////////////////////////////////////////////
		IppiSize roi_flimproj = { m_pVisTab->m_intensityMap.at(0).size(0), m_pVisTab->m_intensityMap.at(0).size(1) };

		std::vector<np::Uint8Array2> intensityMap;
		std::vector<np::Uint8Array2> lifetimeMap;
		for (int i = 0; i < 3; i++)
		{			
			np::Uint8Array2 intensity = np::Uint8Array2(roi_flimproj.width, roi_flimproj.height);
			np::Uint8Array2 lifetime = np::Uint8Array2(roi_flimproj.width, roi_flimproj.height);

			// Intensity map
			if (!m_pVisTab->getIntensityRatioMode())
			{
				ippiScale_32f8u_C1R(m_pVisTab->m_intensityMap.at(i), sizeof(float) * roi_flimproj.width,
					intensity.raw_ptr(), sizeof(uint8_t) * roi_flimproj.width, roi_flimproj,
					m_pConfig->flimIntensityRange[i].min, m_pConfig->flimIntensityRange[i].max);
			}
			// Intensity ratio map
			else
			{
				int num = i;
				int den = ratio_index[num][m_pConfig->flimIntensityRatioRefIdx[num]] - 1;
				np::Uint8Array2 den_zero(roi_flimproj.width, roi_flimproj.height);
				np::FloatArray2 ratio(roi_flimproj.width, roi_flimproj.height);
				ippiCompareC_32f_C1R(m_pVisTab->m_intensityMap.at(den), sizeof(float) * roi_flimproj.width, 0.0f,
					den_zero, sizeof(uint8_t) * roi_flimproj.width, roi_flimproj, ippCmpGreater);
				ippsDivC_8u_ISfs(IPP_MAX_8U, den_zero, den_zero.length(), 0);
				ippsDiv_32f(m_pVisTab->m_intensityMap.at(den), m_pVisTab->m_intensityMap.at(num), ratio.raw_ptr(), ratio.length());

				ippiScale_32f8u_C1R(ratio.raw_ptr(), sizeof(float) * roi_flimproj.width,
					intensity.raw_ptr(), sizeof(uint8_t) * roi_flimproj.width, roi_flimproj,
					m_pConfig->flimIntensityRatioRange[num][m_pConfig->flimIntensityRatioRefIdx[num]].min,
					m_pConfig->flimIntensityRatioRange[num][m_pConfig->flimIntensityRatioRefIdx[num]].max);
				ippsMul_8u_ISfs(den_zero, intensity.raw_ptr(), den_zero.length(), 0);
			}

			(*m_pVisTab->m_pMedfiltIntensityMap)(intensity.raw_ptr());		

			// Lifetime map
			ippiScale_32f8u_C1R(m_pVisTab->m_lifetimeMap.at(i), sizeof(float) * roi_flimproj.width,
				lifetime.raw_ptr(), sizeof(uint8_t) * roi_flimproj.width, roi_flimproj, 
				m_pConfig->flimLifetimeRange[i].min, m_pConfig->flimLifetimeRange[i].max);

			(*m_pVisTab->m_pMedfiltLifetimeMap)(lifetime.raw_ptr());

			// Push to the queue
			intensityMap.push_back(intensity);
			lifetimeMap.push_back(lifetime);			
		}	

		// Set Widgets //////////////////////////////////////////////////////////////////////////////
		emit setWidgets(false);
		emit m_pProcessingTab->setWidgets(false, nullptr);
		emit m_pVisTab->setWidgets(false, nullptr);
		if (m_pVisTab->getLongitudinalViewDlg()) m_pVisTab->getLongitudinalViewDlg()->setWidgets(false);
		m_nSavedFrames = 0;

		// Scaling Images ///////////////////////////////////////////////////////////////////////////
		std::thread scaleImages([&]() { scaling(m_pVisTab->m_vectorOctImage, intensityMap, lifetimeMap, checkList); });

		// Converting Images ////////////////////////////////////////////////////////////////////////
		std::thread convertImages([&]() { converting(checkList); });

		// Rect Writing /////////////////////////////////////////////////////////////////////////////
		std::thread writeRectImages([&]() { rectWriting(checkList); });

		// Circularizing ////////////////////////////////////////////////////////////////////////////		
		std::thread circularizeImages([&]() { circularizing(checkList); });

		// Circ Writing /////////////////////////////////////////////////////////////////////////////
		std::thread writeCircImages([&]() { circWriting(checkList); });

		// Wait for threads end /////////////////////////////////////////////////////////////////////
		scaleImages.join();
		convertImages.join();
		writeRectImages.join();
		circularizeImages.join();
		writeCircImages.join();		

		// Reset Widgets ////////////////////////////////////////////////////////////////////////////
		emit setWidgets(true);
		emit m_pVisTab->setWidgets(true, nullptr);
		emit m_pProcessingTab->setWidgets(true, nullptr);
		if (m_pVisTab->getLongitudinalViewDlg()) m_pVisTab->getLongitudinalViewDlg()->setWidgets(true);

		std::vector<np::Uint8Array2> clear_vector1;
		clear_vector1.swap(intensityMap);
		std::vector<np::Uint8Array2> clear_vector2;
		clear_vector2.swap(lifetimeMap);
	});

	t1.detach();
}

void SaveResultDlg::saveEnFaceMaps()
{
	std::thread t1([&]() {

		// Check Status /////////////////////////////////////////////////////////////////////////////
		EnFaceCheckList checkList;
		checkList.bRaw = m_pCheckBox_RawData->isChecked();
		checkList.bScaled = m_pCheckBox_ScaledImage->isChecked();
		checkList.bCh[0] = m_pCheckBox_EnFaceCh1->isChecked();
		checkList.bCh[1] = m_pCheckBox_EnFaceCh2->isChecked();
		checkList.bCh[2] = m_pCheckBox_EnFaceCh3->isChecked();
		checkList.bOctProj = m_pCheckBox_OctMaxProjection->isChecked();
		
		int start = m_pLineEdit_RangeStart->text().toInt();
		int end = m_pLineEdit_RangeEnd->text().toInt();

		// Set Widgets //////////////////////////////////////////////////////////////////////////////
		emit setWidgets(false);

		// Create directory for saving en face maps /////////////////////////////////////////////////
		QString enFacePath = m_pProcessingTab->m_path + "/en_face/";
		if (checkList.bRaw || checkList.bScaled) QDir().mkdir(enFacePath);

		// Raw en face map writing //////////////////////////////////////////////////////////////////
		if (checkList.bRaw)
		{
			for (int i = 0; i < 3; i++)
			{
				if (checkList.bCh[i])
				{
					QFile fileIntensity(enFacePath + QString("intensity_range[%1 %2]_ch%3.enface").arg(start).arg(end).arg(i + 1));
					if (false != fileIntensity.open(QIODevice::WriteOnly))
					{
						np::FloatArray2 intensity_map(m_pVisTab->m_intensityMap.at(i).size(0), m_pVisTab->m_intensityMap.at(i).size(1));
						memcpy(intensity_map.raw_ptr(), m_pVisTab->m_intensityMap.at(i).raw_ptr(), sizeof(float) * m_pVisTab->m_intensityMap.at(i).length());
						
						fileIntensity.write(reinterpret_cast<char*>(&intensity_map(0, start - 1)), sizeof(float) * intensity_map.size(0) * (end - start + 1));
						fileIntensity.close();
					}

					QFile fileLifetime(enFacePath + QString("lifetime_range[%1 %2]_ch%3.enface").arg(start).arg(end).arg(i + 1));
					if (false != fileLifetime.open(QIODevice::WriteOnly))
					{
						np::FloatArray2 lifetime_map(m_pVisTab->m_lifetimeMap.at(i).size(0), m_pVisTab->m_lifetimeMap.at(i).size(1));
						memcpy(lifetime_map.raw_ptr(), m_pVisTab->m_lifetimeMap.at(i).raw_ptr(), sizeof(float) * m_pVisTab->m_lifetimeMap.at(i).length());

						fileLifetime.write(reinterpret_cast<char*>(&lifetime_map(0, start - 1)), sizeof(float) * lifetime_map.size(0) * (end - start + 1));
						fileLifetime.close();
					}
				}
			}

			if (checkList.bOctProj)
			{
				QFile fileOctMaxProj(enFacePath + QString("oct_max_projection_range[%1 %2].enface").arg(start).arg(end));
				if (false != fileOctMaxProj.open(QIODevice::WriteOnly))
				{
					fileOctMaxProj.write(reinterpret_cast<char*>(&m_pVisTab->m_octProjection(0, start - 1)), sizeof(uint8_t) * m_pVisTab->m_octProjection.size(0) * (end - start + 1));
					fileOctMaxProj.close();
				}
			}
		}

		// Scaled en face map writing ///////////////////////////////////////////////////////////////
		if (checkList.bScaled)
		{
			ColorTable temp_ctable;
			IppiSize roi_flimproj = { m_pVisTab->m_intensityMap.at(0).size(0), m_pVisTab->m_intensityMap.at(0).size(1) };

			for (int i = 0; i < 3; i++)
			{
				if (checkList.bCh[i])
				{
					ImageObject imgObjIntensity(roi_flimproj.width, roi_flimproj.height, temp_ctable.m_colorTableVector.at(INTENSITY_COLORTABLE));
					ImageObject imgObjIntensityRatio(roi_flimproj.width, roi_flimproj.height, temp_ctable.m_colorTableVector.at(INTENSITY_COLORTABLE));
					ImageObject imgObjLifetime(roi_flimproj.width, roi_flimproj.height, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE));
					ImageObject imgObjTemp(roi_flimproj.width, roi_flimproj.height, temp_ctable.m_colorTableVector.at(ColorTable::hsv));
					ImageObject imgObjHsv(roi_flimproj.width, roi_flimproj.height, temp_ctable.m_colorTableVector.at(ColorTable::hsv));

					// Intensity map
					ippiScale_32f8u_C1R(m_pVisTab->m_intensityMap.at(i), sizeof(float) * roi_flimproj.width,
						imgObjIntensity.arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj.width, roi_flimproj, 
						m_pConfig->flimIntensityRange[i].min, m_pConfig->flimIntensityRange[i].max);
					ippiMirror_8u_C1IR(imgObjIntensity.arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj.width, roi_flimproj, ippAxsHorizontal);

					(*m_pVisTab->m_pMedfiltIntensityMap)(imgObjIntensity.arr.raw_ptr());

					imgObjIntensity.qindeximg.copy(0, roi_flimproj.height - end, roi_flimproj.width, end - start + 1)
						.save(enFacePath + QString("intensity_range[%1 %2]_ch%3_[%4 %5].bmp").arg(start).arg(end).arg(i + 1)
						.arg(m_pConfig->flimIntensityRange[i].min, 2, 'f', 1).arg(m_pConfig->flimIntensityRange[i].max, 2, 'f', 1), "bmp");

					// Intensity ratio map
					int num = i;
					for (int j = 0; j < 2; j++)
					{
						int den = ratio_index[num][j] - 1;

						np::Uint8Array2 den_zero(roi_flimproj.width, roi_flimproj.height);
						np::FloatArray2 ratio(roi_flimproj.width, roi_flimproj.height);
						ippiCompareC_32f_C1R(m_pVisTab->m_intensityMap.at(den), sizeof(float) * roi_flimproj.width, 0.0f,
							den_zero, sizeof(uint8_t) * roi_flimproj.width, roi_flimproj, ippCmpGreater);
						ippsDivC_8u_ISfs(IPP_MAX_8U, den_zero, den_zero.length(), 0);
						ippsDiv_32f(m_pVisTab->m_intensityMap.at(den), m_pVisTab->m_intensityMap.at(num), ratio.raw_ptr(), ratio.length());

						ippiScale_32f8u_C1R(ratio.raw_ptr(), sizeof(float) * roi_flimproj.width,
							imgObjIntensityRatio.arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj.width, roi_flimproj,
							m_pConfig->flimIntensityRatioRange[num][j].min, m_pConfig->flimIntensityRatioRange[num][j].max);
						ippsMul_8u_ISfs(den_zero, imgObjIntensityRatio.arr.raw_ptr(), den_zero.length(), 0);
						ippiMirror_8u_C1IR(imgObjIntensityRatio.arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj.width, roi_flimproj, ippAxsHorizontal);

						(*m_pVisTab->m_pMedfiltIntensityMap)(imgObjIntensityRatio.arr.raw_ptr());

						imgObjIntensityRatio.qindeximg.copy(0, roi_flimproj.height - end, roi_flimproj.width, end - start + 1)
							.save(enFacePath + QString("intensity_ratio_range[%1 %2]_ch%3_%4_[%5 %6].bmp").arg(start).arg(end).arg(num + 1).arg(den + 1)
							.arg(m_pConfig->flimIntensityRatioRange[num][j].min, 2, 'f', 1)
							.arg(m_pConfig->flimIntensityRatioRange[num][j].max, 2, 'f', 1), "bmp");
					}

					// Lifetime map
					ippiScale_32f8u_C1R(m_pVisTab->m_lifetimeMap.at(i), sizeof(float) * roi_flimproj.width,
						imgObjLifetime.arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj.width,
						roi_flimproj, m_pConfig->flimLifetimeRange[i].min, m_pConfig->flimLifetimeRange[i].max);
					ippiMirror_8u_C1IR(imgObjLifetime.arr.raw_ptr(), sizeof(uint8_t) * roi_flimproj.width, roi_flimproj, ippAxsHorizontal);

					(*m_pVisTab->m_pMedfiltLifetimeMap)(imgObjLifetime.arr.raw_ptr());

					imgObjLifetime.qindeximg.copy(0, roi_flimproj.height - end, roi_flimproj.width, end - start + 1)
						.save(enFacePath + QString("lifetime_range[%1 %2]_ch%3_[%4 %5].bmp").arg(start).arg(end).arg(i + 1)
						.arg(m_pConfig->flimLifetimeRange[i].min, 2, 'f', 1).arg(m_pConfig->flimLifetimeRange[i].max, 2, 'f', 1), "bmp");

					/// HSV enhanced map
					///memset(imgObjTemp.qrgbimg.bits(), 255, imgObjTemp.qrgbimg.byteCount()); // Saturation is set to be 255.
					///imgObjTemp.setRgbChannelData(imgObjLifetime.qindeximg.bits(), 0); // Hue	
					///uint8_t *pIntensity = new uint8_t[imgObjIntensity.qindeximg.byteCount()];
					///memcpy(pIntensity, imgObjIntensity.qindeximg.bits(), imgObjIntensity.qindeximg.byteCount());
					///ippsMulC_8u_ISfs(1.0, pIntensity, imgObjIntensity.qindeximg.byteCount(), 0);
					///imgObjTemp.setRgbChannelData(pIntensity, 2); // Value
					///delete[] pIntensity;

					///ippiHSVToRGB_8u_C3R(imgObjTemp.qrgbimg.bits(), 3 * roi_flimproj.width, imgObjHsv.qrgbimg.bits(), 3 * roi_flimproj.width, roi_flimproj);

					// Non HSV intensity-weight map
					ImageObject tempImgObj(roi_flimproj.width, roi_flimproj.height, temp_ctable.m_colorTableVector.at(ColorTable::gray));

					imgObjLifetime.convertRgb();
					memcpy(tempImgObj.qindeximg.bits(), imgObjIntensity.arr.raw_ptr(), tempImgObj.qindeximg.byteCount());
					tempImgObj.convertRgb();

					ippsMul_8u_Sfs(imgObjLifetime.qrgbimg.bits(), tempImgObj.qrgbimg.bits(), imgObjHsv.qrgbimg.bits(), imgObjHsv.qrgbimg.byteCount(), 8);

					imgObjHsv.qrgbimg.copy(0, roi_flimproj.height - end, roi_flimproj.width, end - start + 1)
						.save(enFacePath + QString("flim_map_range[%1 %2]_ch%3_i[%4 %5]_t[%6 %7].bmp").arg(start).arg(end).arg(i + 1)
						.arg(m_pConfig->flimIntensityRange[i].min, 2, 'f', 1).arg(m_pConfig->flimIntensityRange[i].max, 2, 'f', 1)
						.arg(m_pConfig->flimLifetimeRange[i].min, 2, 'f', 1).arg(m_pConfig->flimLifetimeRange[i].max, 2, 'f', 1), "bmp");
				}
			}

			if (checkList.bOctProj)
			{
				IppiSize roi_proj = { m_pVisTab->m_octProjection.size(0), m_pVisTab->m_octProjection.size(1) };
				ImageObject imgObjOctMaxProj(roi_proj.width, roi_proj.height, temp_ctable.m_colorTableVector.at(OCT_COLORTABLE));

				np::FloatArray2 scale_temp(roi_proj.width, roi_proj.height);
				ippsConvert_8u32f(m_pVisTab->m_octProjection.raw_ptr(), scale_temp.raw_ptr(), scale_temp.length());
				ippiScale_32f8u_C1R(scale_temp, sizeof(float) * roi_proj.width, imgObjOctMaxProj.arr.raw_ptr(), sizeof(uint8_t) * roi_proj.width,
					roi_proj, m_pConfig->octGrayRange.min, m_pConfig->octGrayRange.max);
				ippiMirror_8u_C1IR(imgObjOctMaxProj.arr.raw_ptr(), sizeof(uint8_t) * roi_proj.width, roi_proj, ippAxsHorizontal);
				
				imgObjOctMaxProj.qindeximg.copy(0, roi_proj.height - end, roi_proj.width, end - start + 1)
					.save(enFacePath + QString("oct_max_projection_range[%1 %2]_gray[%3 %4].bmp")
						.arg(start).arg(end).arg(m_pConfig->octGrayRange.min).arg(m_pConfig->octGrayRange.max), "bmp");
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		// Scaling MATLAB Script ////////////////////////////////////////////////////////////////////
		//if (false == QFile::copy("scale_indicator.m", enFacePath + "scale_indicator.m"))
		//	printf("Error occurred while copying matlab sciprt.\n");

		// Reset Widgets ////////////////////////////////////////////////////////////////////////////
		emit setWidgets(true);
	});

	t1.detach();
}

void SaveResultDlg::setRange()
{
	int start = m_pLineEdit_RangeStart->text().toInt();
	int end = m_pLineEdit_RangeEnd->text().toInt();

	if (start < 1)
		m_pLineEdit_RangeStart->setText(QString::number(1));
	if (end > m_pVisTab->m_vectorOctImage.size())
		m_pLineEdit_RangeEnd->setText(QString::number(m_pVisTab->m_vectorOctImage.size()));
}

void SaveResultDlg::checkRectImage(bool toggled)
{
	if (m_pCheckBox_ResizeRectImage->isChecked()) m_pCheckBox_ResizeRectImage->setChecked(false);
	m_pCheckBox_ResizeRectImage->setEnabled(toggled);

	if (!m_pCheckBox_RectImage->isChecked() && !m_pCheckBox_CircImage->isChecked() && !m_pCheckBox_LongiImage->isChecked())
	{
		m_pPushButton_SaveCrossSections->setEnabled(false);		
		m_pLabel_Range->setEnabled(false);
		m_pLineEdit_RangeStart->setEnabled(false);
		m_pLineEdit_RangeEnd->setEnabled(false);
		m_pCheckBox_CrossSectionCh1->setEnabled(false);
		m_pCheckBox_CrossSectionCh2->setEnabled(false);
		m_pCheckBox_CrossSectionCh3->setEnabled(false);
		m_pCheckBox_IntensityWeighting->setEnabled(false);
	}
	else
	{
		m_pPushButton_SaveCrossSections->setEnabled(true);		
		m_pLabel_Range->setEnabled(true);
		m_pLineEdit_RangeStart->setEnabled(true);
		m_pLineEdit_RangeEnd->setEnabled(true);
		m_pCheckBox_CrossSectionCh1->setEnabled(true);
		m_pCheckBox_CrossSectionCh2->setEnabled(true);
		m_pCheckBox_CrossSectionCh3->setEnabled(true);
		checkChannels();
	}
}

void SaveResultDlg::checkCircImage(bool toggled)
{
	if (m_pCheckBox_ResizeCircImage->isChecked()) m_pCheckBox_ResizeCircImage->setChecked(false);
	m_pCheckBox_ResizeCircImage->setEnabled(toggled);

	if (!m_pCheckBox_RectImage->isChecked() && !m_pCheckBox_CircImage->isChecked() && !m_pCheckBox_LongiImage->isChecked())
	{
		m_pPushButton_SaveCrossSections->setEnabled(false);
		m_pLabel_Range->setEnabled(false);
		m_pLineEdit_RangeStart->setEnabled(false);
		m_pLineEdit_RangeEnd->setEnabled(false);
		m_pCheckBox_CrossSectionCh1->setEnabled(false);
		m_pCheckBox_CrossSectionCh2->setEnabled(false);
		m_pCheckBox_CrossSectionCh3->setEnabled(false);
		m_pCheckBox_IntensityWeighting->setEnabled(false);
	}
	else
	{
		m_pPushButton_SaveCrossSections->setEnabled(true);
		m_pLabel_Range->setEnabled(true);
		m_pLineEdit_RangeStart->setEnabled(true);
		m_pLineEdit_RangeEnd->setEnabled(true);
		m_pCheckBox_CrossSectionCh1->setEnabled(true);
		m_pCheckBox_CrossSectionCh2->setEnabled(true);
		m_pCheckBox_CrossSectionCh3->setEnabled(true);
		checkChannels();
	}
}

void SaveResultDlg::checkLongiImage(bool toggled)
{
	if (m_pCheckBox_ResizeLongiImage->isChecked()) m_pCheckBox_ResizeLongiImage->setChecked(false);
	m_pCheckBox_ResizeLongiImage->setEnabled(toggled);

	if (!m_pCheckBox_RectImage->isChecked() && !m_pCheckBox_CircImage->isChecked() && !m_pCheckBox_LongiImage->isChecked())
	{
		m_pPushButton_SaveCrossSections->setEnabled(false);
		m_pLabel_Range->setEnabled(false);
		m_pLineEdit_RangeStart->setEnabled(false);
		m_pLineEdit_RangeEnd->setEnabled(false);
		m_pCheckBox_CrossSectionCh1->setEnabled(false);
		m_pCheckBox_CrossSectionCh2->setEnabled(false);
		m_pCheckBox_CrossSectionCh3->setEnabled(false); 
		m_pCheckBox_IntensityWeighting->setEnabled(false);
	}
	else
	{
		m_pPushButton_SaveCrossSections->setEnabled(true);
		m_pLabel_Range->setEnabled(true);
		m_pLineEdit_RangeStart->setEnabled(true);
		m_pLineEdit_RangeEnd->setEnabled(true);
		m_pCheckBox_CrossSectionCh1->setEnabled(true);
		m_pCheckBox_CrossSectionCh2->setEnabled(true);
		m_pCheckBox_CrossSectionCh3->setEnabled(true); 
		checkChannels();
	}
}

void SaveResultDlg::enableRectResize(bool checked)
{
	m_pLineEdit_RectWidth->setEnabled(checked);
	m_pLineEdit_RectHeight->setEnabled(checked);

	if (!checked)
	{
		m_pLineEdit_RectWidth->setText(QString::number(m_pVisTab->m_vectorOctImage.at(0).size(1)));
		m_pLineEdit_RectHeight->setText(QString::number(m_pVisTab->m_vectorOctImage.at(0).size(0)));
	}
}

void SaveResultDlg::enableCircResize(bool checked)
{
	m_pLineEdit_CircDiameter->setEnabled(checked);

	if (!checked)
		m_pLineEdit_CircDiameter->setText(QString::number(2 * m_pVisTab->m_vectorOctImage.at(0).size(0)));
}

void SaveResultDlg::enableLongiResize(bool checked)
{
	m_pLineEdit_LongiWidth->setEnabled(checked);
	m_pLineEdit_LongiHeight->setEnabled(checked);

	if (!checked)
	{
		m_pLineEdit_LongiWidth->setText(QString::number(m_pVisTab->m_vectorOctImage.size()));
		m_pLineEdit_LongiHeight->setText(QString::number(2 * m_pVisTab->m_vectorOctImage.at(0).size(0)));
	}
}

void SaveResultDlg::checkResizeValue()
{
	int rect_width = m_pLineEdit_RectWidth->text().toInt();
	int rect_height = m_pLineEdit_RectHeight->text().toInt();
	int circ_diameter = m_pLineEdit_CircDiameter->text().toInt();
	int longi_width = m_pLineEdit_LongiWidth->text().toInt();
	int longi_height = m_pLineEdit_LongiHeight->text().toInt();

	if (rect_width < 1) m_pLineEdit_RectWidth->setText(QString::number(1));
	if (rect_height < 1) m_pLineEdit_RectHeight->setText(QString::number(1));
	if (circ_diameter < 1) m_pLineEdit_CircDiameter->setText(QString::number(1));
	if (longi_width < 1) m_pLineEdit_LongiWidth->setText(QString::number(1));
	if (longi_height < 1) m_pLineEdit_LongiHeight->setText(QString::number(1));
}

void SaveResultDlg::checkChannels()
{
	if (!m_pCheckBox_CrossSectionCh1->isChecked() && !m_pCheckBox_CrossSectionCh2->isChecked() && !m_pCheckBox_CrossSectionCh3->isChecked())
	{
		if (m_pCheckBox_IntensityWeighting->isChecked()) m_pCheckBox_IntensityWeighting->setChecked(false);
		m_pCheckBox_IntensityWeighting->setEnabled(false);
	}
	else
		m_pCheckBox_IntensityWeighting->setEnabled(true);
}

void SaveResultDlg::checkEnFaceOptions()
{
	if (!m_pCheckBox_RawData->isChecked() && !m_pCheckBox_ScaledImage->isChecked())
	{
		m_pPushButton_SaveEnFaceMaps->setEnabled(false);
		m_pCheckBox_EnFaceCh1->setEnabled(false);
		m_pCheckBox_EnFaceCh2->setEnabled(false);
		m_pCheckBox_EnFaceCh3->setEnabled(false);
		m_pCheckBox_OctMaxProjection->setEnabled(false);
	}
	else
	{
		m_pPushButton_SaveEnFaceMaps->setEnabled(true);
		m_pCheckBox_EnFaceCh1->setEnabled(true);
		m_pCheckBox_EnFaceCh2->setEnabled(true);
		m_pCheckBox_EnFaceCh3->setEnabled(true);
		m_pCheckBox_OctMaxProjection->setEnabled(true);
	}

	if (!m_pCheckBox_EnFaceCh1->isChecked() && !m_pCheckBox_EnFaceCh2->isChecked() && !m_pCheckBox_EnFaceCh3->isChecked() && !m_pCheckBox_OctMaxProjection->isChecked())
		m_pPushButton_SaveEnFaceMaps->setEnabled(false);
	else
		if (m_pCheckBox_RawData->isChecked() || m_pCheckBox_ScaledImage->isChecked())
			m_pPushButton_SaveEnFaceMaps->setEnabled(true);
}

void SaveResultDlg::setWidgetsEnabled(bool enabled)
{
	// Set State
	m_bIsSaving = !enabled;

	// Save Cross-sections
	m_pPushButton_SaveCrossSections->setEnabled(enabled);
	m_pCheckBox_RectImage->setEnabled(enabled);
	m_pCheckBox_CircImage->setEnabled(enabled);
	m_pCheckBox_LongiImage->setEnabled(enabled);

	m_pCheckBox_ResizeRectImage->setEnabled(enabled);
	m_pCheckBox_ResizeCircImage->setEnabled(enabled);
	m_pCheckBox_ResizeLongiImage->setEnabled(enabled);

	m_pLineEdit_RectWidth->setEnabled(enabled);
	m_pLineEdit_RectHeight->setEnabled(enabled);
	m_pLineEdit_CircDiameter->setEnabled(enabled);
	m_pLineEdit_LongiWidth->setEnabled(enabled);
	m_pLineEdit_LongiHeight->setEnabled(enabled);
	if (enabled)
	{
		if (!m_pCheckBox_ResizeRectImage->isChecked())
		{
			m_pLineEdit_RectWidth->setEnabled(false);
			m_pLineEdit_RectHeight->setEnabled(false);
		}
		if (!m_pCheckBox_ResizeCircImage->isChecked())
			m_pLineEdit_CircDiameter->setEnabled(false);
		if (!m_pCheckBox_ResizeLongiImage->isChecked())
		{
			m_pLineEdit_LongiWidth->setEnabled(false);
			m_pLineEdit_LongiHeight->setEnabled(false);
		}
	}	

	m_pCheckBox_CrossSectionCh1->setEnabled(enabled);
	m_pCheckBox_CrossSectionCh2->setEnabled(enabled);
	m_pCheckBox_CrossSectionCh3->setEnabled(enabled);
	m_pCheckBox_IntensityWeighting->setEnabled(enabled);
	
	// Set Range
	m_pLabel_Range->setEnabled(enabled);
	m_pLineEdit_RangeStart->setEnabled(enabled);
	m_pLineEdit_RangeEnd->setEnabled(enabled);


	// Save En Face Maps
	m_pPushButton_SaveEnFaceMaps->setEnabled(enabled);
	m_pCheckBox_RawData->setEnabled(enabled);
	m_pCheckBox_ScaledImage->setEnabled(enabled);

	m_pCheckBox_EnFaceCh1->setEnabled(enabled);
	m_pCheckBox_EnFaceCh2->setEnabled(enabled);
	m_pCheckBox_EnFaceCh3->setEnabled(enabled);

	m_pCheckBox_OctMaxProjection->setEnabled(enabled);
}


void SaveResultDlg::scaling(std::vector<np::Uint8Array2>& vectorOctImage, 
	std::vector<np::Uint8Array2>& intensityMap, std::vector<np::Uint8Array2>& lifetimeMap, CrossSectionCheckList checkList)
{
	int nTotalFrame = (int)vectorOctImage.size();
	ColorTable temp_ctable;
	
	int frameCount = 0;
	while (frameCount < nTotalFrame)
	{
		// Create Image Object Array for threading operation
		IppiSize roi_oct = { vectorOctImage.at(0).size(0), vectorOctImage.at(0).size(1) };

		ImgObjVector* pImgObjVec = new ImgObjVector;

		// Image objects for OCT Images
		pImgObjVec->push_back(new ImageObject(roi_oct.height, roi_oct.width, temp_ctable.m_colorTableVector.at(OCT_COLORTABLE)));

		// Image objects for Ch1 FLIM
		pImgObjVec->push_back(new ImageObject(roi_oct.height / 4, RING_THICKNESS, temp_ctable.m_colorTableVector.at(INTENSITY_COLORTABLE)));
		pImgObjVec->push_back(new ImageObject(roi_oct.height / 4, RING_THICKNESS, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE)));
		// Image objects for Ch2 FLIM
		pImgObjVec->push_back(new ImageObject(roi_oct.height / 4, RING_THICKNESS, temp_ctable.m_colorTableVector.at(INTENSITY_COLORTABLE)));
		pImgObjVec->push_back(new ImageObject(roi_oct.height / 4, RING_THICKNESS, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE)));
		// Image objects for Ch3 FLIM
		pImgObjVec->push_back(new ImageObject(roi_oct.height / 4, RING_THICKNESS, temp_ctable.m_colorTableVector.at(INTENSITY_COLORTABLE)));
		pImgObjVec->push_back(new ImageObject(roi_oct.height / 4, RING_THICKNESS, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE)));

		// OCT Visualization
		np::FloatArray2 scale_temp(roi_oct.width, roi_oct.height);
		ippsConvert_8u32f(vectorOctImage.at(frameCount).raw_ptr(), scale_temp.raw_ptr(), scale_temp.length());
		ippiTranspose_32f_C1IR(scale_temp.raw_ptr(), roi_oct.width * sizeof(float), roi_oct);
		ippiScale_32f8u_C1R(scale_temp.raw_ptr(), roi_oct.width * sizeof(float),
			pImgObjVec->at(0)->arr.raw_ptr(), roi_oct.width * sizeof(uint8_t), roi_oct, (float)m_pConfig->octGrayRange.min, (float)m_pConfig->octGrayRange.max);
		(*m_pVisTab->m_pMedfilt)(pImgObjVec->at(0)->arr.raw_ptr());
		
		// FLIM Visualization		
		IppiSize roi_flim = { roi_oct.height / 4, 1 };
		for (int i = 0; i < 3; i++)
		{
			if (checkList.bCh[i])
			{
				uint8_t* rectIntensity = &intensityMap.at(i)(0, frameCount);
				uint8_t* rectLifetime = &lifetimeMap.at(i)(0, frameCount);
				for (int j = 0; j < RING_THICKNESS; j++)
				{
					memcpy(&pImgObjVec->at(1 + 2 * i)->arr(0, j), rectIntensity, sizeof(uint8_t) * roi_flim.width);
					memcpy(&pImgObjVec->at(2 + 2 * i)->arr(0, j), rectLifetime, sizeof(uint8_t) * roi_flim.width);
				}
			}
		}

		frameCount++;

		// Push the buffers to sync Queues
		m_syncQueueConverting.push(pImgObjVec);

	}
}

void SaveResultDlg::converting(CrossSectionCheckList checkList)
{
	int nTotalFrame = (int)m_pVisTab->m_vectorOctImage.size();

	int frameCount = 0;
	while (frameCount < nTotalFrame)
	{
		// Get the buffer from the previous sync Queue
		ImgObjVector *pImgObjVec = m_syncQueueConverting.pop();

		// Converting RGB
		if (checkList.bCh[0] || checkList.bCh[1] || checkList.bCh[2])
			pImgObjVec->at(0)->convertRgb();

		tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)3),
			[&](const tbb::blocked_range<size_t>& r) {
			for (size_t i = r.begin(); i != r.end(); ++i)
			{
				if (checkList.bCh[i])
				{
					pImgObjVec->at(1 + 2 * i)->convertScaledRgb(); // intensity
					pImgObjVec->at(2 + 2 * i)->convertScaledRgb(); // lifetime

					if (checkList.bMulti)
					{
						pImgObjVec->at(1 + 2 * i)->scaled4(); // intensity
						pImgObjVec->at(2 + 2 * i)->scaled4(); // lifetime
					}
				}
			}
		});
		frameCount++;

		// Push the buffers to sync Queues
		m_syncQueueRectWriting.push(pImgObjVec);		
	}
}

void SaveResultDlg::rectWriting(CrossSectionCheckList checkList)
{
	int nTotalFrame = (int)m_pVisTab->m_vectorOctImage.size();
	QString folderName;
	for (int i = 0; i < m_pProcessingTab->m_path.length(); i++)
		if (m_pProcessingTab->m_path.at(i) == QChar('/')) folderName = m_pProcessingTab->m_path.right(m_pProcessingTab->m_path.length() - i - 1);
	
	int start = m_pLineEdit_RangeStart->text().toInt();
	int end = m_pLineEdit_RangeEnd->text().toInt();

	if (!checkList.bCh[0] && !checkList.bCh[1] && !checkList.bCh[2])
	{
		QString rectPath;
		rectPath = m_pProcessingTab->m_path + QString("/rect_image_gray[%1 %2]/").arg(m_pConfig->octGrayRange.min).arg(m_pConfig->octGrayRange.max);
		if (checkList.bRect) QDir().mkdir(rectPath);
		
		int frameCount = 0;
		while (frameCount < nTotalFrame)
		{
			// Get the buffer from the previous sync Queue
			ImgObjVector *pImgObjVec = m_syncQueueRectWriting.pop();	

			// Range test
			if (((frameCount + 1) >= start) && ((frameCount + 1) <= end))
			{
				// Write rect images
				if (checkList.bRect)
				{
					if (!checkList.bRectResize)
						pImgObjVec->at(0)->qindeximg.save(rectPath + QString("rect_%1_%2.bmp").arg(folderName).arg(frameCount + 1, 3, 10, (QChar)'0'), "bmp");
					else
						pImgObjVec->at(0)->qindeximg.scaled(checkList.nRectWidth, checkList.nRectHeight, Qt::IgnoreAspectRatio, m_defaultTransformation).
						save(rectPath + QString("rect_%1_%2.bmp").arg(folderName).arg(frameCount + 1, 3, 10, (QChar)'0'), "bmp");
				}
			}
		
			frameCount++;
			//emit savedSingleFrame(m_nSavedFrames++);

			// Push the buffers to sync Queues
			m_syncQueueCircularizing.push(pImgObjVec);
		}
	}
	else
	{
		QString rectPath[3];
		if (!checkList.bMulti)
		{
			for (int i = 0; i < 3; i++)
			{
				if (checkList.bCh[i])
				{
					if (!m_pVisTab->getIntensityRatioMode())
						rectPath[i] = m_pProcessingTab->m_path + QString("/rect_image_gray[%1 %2]_ch%3_i[%4 %5]_t[%6 %7]/")
							.arg(m_pConfig->octGrayRange.min).arg(m_pConfig->octGrayRange.max)
							.arg(i + 1).arg(m_pConfig->flimIntensityRange[i].min, 2, 'f', 1).arg(m_pConfig->flimIntensityRange[i].max, 2, 'f', 1)
							.arg(m_pConfig->flimLifetimeRange[i].min, 2, 'f', 1).arg(m_pConfig->flimLifetimeRange[i].max, 2, 'f', 1);
					else
						rectPath[i] = m_pProcessingTab->m_path + QString("/rect_image_gray[%1 %2]_ch%3_%4_ir[%5 %6]_t[%7 %8]/")
						.arg(m_pConfig->octGrayRange.min).arg(m_pConfig->octGrayRange.max)
						.arg(i + 1).arg(ratio_index[i][m_pConfig->flimIntensityRatioRefIdx[i]])
						.arg(m_pConfig->flimIntensityRatioRange[i][m_pConfig->flimIntensityRatioRefIdx[i]].min, 2, 'f', 1)
						.arg(m_pConfig->flimIntensityRatioRange[i][m_pConfig->flimIntensityRatioRefIdx[i]].max, 2, 'f', 1)
						.arg(m_pConfig->flimLifetimeRange[i].min, 2, 'f', 1).arg(m_pConfig->flimLifetimeRange[i].max, 2, 'f', 1);

					if (checkList.bRect) QDir().mkdir(rectPath[i]);
				}
			}
		}
		else
		{
			if (!m_pVisTab->getIntensityRatioMode())
				rectPath[0] = m_pProcessingTab->m_path + QString("/rect_merged_gray[%1 %2]_ch%3%4%5_i[%6 %7][%8 %9][%10 %11]_t[%12 %13][%14 %15][%16 %17]/")
					.arg(m_pConfig->octGrayRange.min).arg(m_pConfig->octGrayRange.max)
					.arg(checkList.bCh[0] ? "1" : "").arg(checkList.bCh[1] ? "2" : "").arg(checkList.bCh[2] ? "3" : "")
					.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimIntensityRange[0].min, 'f', 1) : "")
					.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimIntensityRange[0].max, 'f', 1) : "")
					.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimIntensityRange[1].min, 'f', 1) : "")
					.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimIntensityRange[1].max, 'f', 1) : "")
					.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimIntensityRange[2].min, 'f', 1) : "")
					.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimIntensityRange[2].max, 'f', 1) : "")
					.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimLifetimeRange[0].min, 'f', 1) : "")
					.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimLifetimeRange[0].max, 'f', 1) : "")
					.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimLifetimeRange[1].min, 'f', 1) : "")
					.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimLifetimeRange[1].max, 'f', 1) : "")
					.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimLifetimeRange[2].min, 'f', 1) : "")
					.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimLifetimeRange[2].max, 'f', 1) : "");
			else
				rectPath[0] = m_pProcessingTab->m_path + QString("/rect_merged_gray[%1 %2]_ch%3%4_%5%6_%7%8_ir[%9 %10][%11 %12][%13 %14]_t[%15 %16][%17 %18][%19 %20]/")
				.arg(m_pConfig->octGrayRange.min).arg(m_pConfig->octGrayRange.max)
				.arg(checkList.bCh[0] ? "1" : "").arg(checkList.bCh[0] ? QString::number(ratio_index[0][m_pConfig->flimIntensityRatioRefIdx[0]]) : "")
				.arg(checkList.bCh[1] ? "2" : "").arg(checkList.bCh[1] ? QString::number(ratio_index[1][m_pConfig->flimIntensityRatioRefIdx[1]]) : "")
				.arg(checkList.bCh[2] ? "3" : "").arg(checkList.bCh[2] ? QString::number(ratio_index[2][m_pConfig->flimIntensityRatioRefIdx[2]]) : "")
				.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimIntensityRatioRange[0][m_pConfig->flimIntensityRatioRefIdx[0]].min, 'f', 1) : "")
				.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimIntensityRatioRange[0][m_pConfig->flimIntensityRatioRefIdx[0]].max, 'f', 1) : "")
				.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimIntensityRatioRange[1][m_pConfig->flimIntensityRatioRefIdx[1]].min, 'f', 1) : "")
				.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimIntensityRatioRange[1][m_pConfig->flimIntensityRatioRefIdx[1]].max, 'f', 1) : "")
				.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimIntensityRatioRange[2][m_pConfig->flimIntensityRatioRefIdx[2]].min, 'f', 1) : "")
				.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimIntensityRatioRange[2][m_pConfig->flimIntensityRatioRefIdx[2]].max, 'f', 1) : "")
				.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimLifetimeRange[0].min, 'f', 1) : "")
				.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimLifetimeRange[0].max, 'f', 1) : "")
				.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimLifetimeRange[1].min, 'f', 1) : "")
				.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimLifetimeRange[1].max, 'f', 1) : "")
				.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimLifetimeRange[2].min, 'f', 1) : "")
				.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimLifetimeRange[2].max, 'f', 1) : "");

			if (checkList.bRect) QDir().mkdir(rectPath[0]);
		}

		int frameCount = 0;
		while (frameCount < nTotalFrame)
		{
			// Get the buffer from the previous sync Queue
			ImgObjVector *pImgObjVec = m_syncQueueRectWriting.pop();
			
			// Range test
			if (((frameCount + 1) >= start) && ((frameCount + 1) <= end))
			{
				if (!checkList.bMulti)
				{
					for (int i = 0; i < 3; i++)
					{
						if (checkList.bCh[i])
						{
							// Paste FLIM color ring to RGB rect image
							memcpy(pImgObjVec->at(0)->qrgbimg.bits() + 3 * pImgObjVec->at(0)->arr.size(0) * (pImgObjVec->at(0)->arr.size(1) - 2 * RING_THICKNESS),
								pImgObjVec->at(1 + 2 * i)->qrgbimg.bits(), pImgObjVec->at(1 + 2 * i)->qrgbimg.byteCount()); // Intensity
							memcpy(pImgObjVec->at(0)->qrgbimg.bits() + 3 * pImgObjVec->at(0)->arr.size(0) * (pImgObjVec->at(0)->arr.size(1) - 1 * RING_THICKNESS),
								pImgObjVec->at(2 + 2 * i)->qrgbimg.bits(), pImgObjVec->at(2 + 2 * i)->qrgbimg.byteCount()); // Lifetime

							if (checkList.bRect)
							{
								// Write rect images
								if (!checkList.bRectResize)
									pImgObjVec->at(0)->qrgbimg.save(rectPath[i] + QString("rect_%1_%2.bmp").arg(folderName).arg(frameCount + 1, 3, 10, (QChar)'0'), "bmp");
								else
									pImgObjVec->at(0)->qrgbimg.scaled(checkList.nRectWidth, checkList.nRectHeight, Qt::IgnoreAspectRatio, m_defaultTransformation).
									save(rectPath[i] + QString("rect_%1_%2.bmp").arg(folderName).arg(frameCount + 1, 3, 10, (QChar)'0'), "bmp");
							}
						}
					}
				}
				else
				{
					int nCh = (checkList.bCh[0] ? 1 : 0) + (checkList.bCh[1] ? 1 : 0) + (checkList.bCh[2] ? 1 : 0);
					int n = 0;

					for (int i = 0; i < 3; i++)
					{
						if (checkList.bCh[i])
						{
							/// HSV channel setting
							///ImageObject imgObjTemp(pImgObjVec->at(1 + 2 * i)->qrgbimg.width(), pImgObjVec->at(1 + 2 * i)->qrgbimg.height(), pImgObjVec->at(1 + 2 * i)->getColorTable());
							///ImageObject imgObjHsv( pImgObjVec->at(1 + 2 * i)->qrgbimg.width(), pImgObjVec->at(1 + 2 * i)->qrgbimg.height(), pImgObjVec->at(1 + 2 * i)->getColorTable());

							///IppiSize roi_flimproj = { pImgObjVec->at(1 + 2 * i)->qrgbimg.width(), pImgObjVec->at(1 + 2 * i)->qrgbimg.height() };

							///memset(imgObjTemp.qrgbimg.bits(), 255, imgObjTemp.qrgbimg.byteCount()); // Saturation is set to be 255.
							///imgObjTemp.setRgbChannelData(pImgObjVec->at(2 + 2 * i)->qindeximg.bits(), 0); // Hue	
							///uint8_t *pIntensity = new uint8_t[pImgObjVec->at(1 + 2 * i)->qindeximg.byteCount()];
							///memcpy(pIntensity, pImgObjVec->at(1 + 2 * i)->qindeximg.bits(), pImgObjVec->at(1 + 2 * i)->qindeximg.byteCount());
							///ippsMulC_8u_ISfs(1.0, pIntensity, pImgObjVec->at(1 + 2 * i)->qindeximg.byteCount(), 0);
							///imgObjTemp.setRgbChannelData(pIntensity, 2); // Value
							///delete[] pIntensity;

							///ippiHSVToRGB_8u_C3R(imgObjTemp.qrgbimg.bits(), 3 * roi_flimproj.width, imgObjHsv.qrgbimg.bits(), 3 * roi_flimproj.width, roi_flimproj);
							///*pImgObjVec->at(1 + 2 * i) = std::move(imgObjHsv);

							// Non HSV intensity-weight map
							ColorTable temp_ctable;
							ImageObject imgObjTemp(pImgObjVec->at(1 + 2 * i)->qrgbimg.width(), pImgObjVec->at(1 + 2 * i)->qrgbimg.height(), temp_ctable.m_colorTableVector.at(ColorTable::gray));
							ImageObject imgObjHsv(pImgObjVec->at(1 + 2 * i)->qrgbimg.width(), pImgObjVec->at(1 + 2 * i)->qrgbimg.height(), pImgObjVec->at(1 + 2 * i)->getColorTable());

							memcpy(imgObjTemp.qindeximg.bits(), pImgObjVec->at(1 + 2 * i)->qindeximg.bits(), imgObjTemp.qindeximg.byteCount());
							imgObjTemp.convertRgb();

							ippsMul_8u_Sfs(pImgObjVec->at(2 + 2 * i)->qrgbimg.bits(), imgObjTemp.qrgbimg.bits(), imgObjHsv.qrgbimg.bits(), imgObjTemp.qrgbimg.byteCount(), 8);
							pImgObjVec->at(1 + 2 * i)->qrgbimg = std::move(imgObjHsv.qrgbimg);

							// Paste FLIM color ring to RGB rect image
							memcpy(pImgObjVec->at(0)->qrgbimg.bits() + 3 * pImgObjVec->at(0)->arr.size(0) * (pImgObjVec->at(0)->arr.size(1) - (nCh - n++) * RING_THICKNESS),
								pImgObjVec->at(1 + 2 * i)->qrgbimg.bits(), pImgObjVec->at(1 + 2 * i)->qrgbimg.byteCount());
						}

						if (checkList.bRect)
						{
							// Write rect images
							if (!checkList.bRectResize)
								pImgObjVec->at(0)->qrgbimg.save(rectPath[0] + QString("rect_%1_%2.bmp").arg(folderName).arg(frameCount + 1, 3, 10, (QChar)'0'), "bmp");
							else
								pImgObjVec->at(0)->qrgbimg.scaled(checkList.nRectWidth, checkList.nRectHeight, Qt::IgnoreAspectRatio, m_defaultTransformation).
								save(rectPath[0] + QString("rect_%1_%2.bmp").arg(folderName).arg(frameCount + 1, 3, 10, (QChar)'0'), "bmp");
						}
					}
				}
			}

			frameCount++;
			//emit savedSingleFrame(m_nSavedFrames++);

			// Push the buffers to sync Queues
			m_syncQueueCircularizing.push(pImgObjVec);
		}
	}
}

void SaveResultDlg::circularizing(CrossSectionCheckList checkList) // with longitudinal making
{
	int nTotalFrame = (int)m_pVisTab->m_vectorOctImage.size();
	int nTotalFrame4 = ((nTotalFrame + 3) >> 2) << 2;
	ColorTable temp_ctable;

	// Image objects for longitudinal image
	ImgObjVector *pImgObjVecLongi[3];
	if (checkList.bLongi)
	{
		for (int i = 0; i < 3; i++)
		{
			pImgObjVecLongi[i] = new ImgObjVector;
			for (int j = 0; j < (int)(m_pVisTab->m_vectorOctImage.at(0).size(1) / 2); j++)
			{
				ImageObject *pLongiImgObj = new ImageObject(nTotalFrame4, 2 * m_pVisTab->m_vectorOctImage.at(0).size(0), temp_ctable.m_colorTableVector.at(OCT_COLORTABLE));
				pImgObjVecLongi[i]->push_back(pLongiImgObj);
			}
		}
	}

	int frameCount = 0;
	while (frameCount < nTotalFrame)
	{
		// Get the buffer from the previous sync Queue
		ImgObjVector *pImgObjVec = m_syncQueueCircularizing.pop();
		ImgObjVector *pImgObjVecCirc = new ImgObjVector;

		if (!checkList.bCh[0] && !checkList.bCh[1] && !checkList.bCh[2])
		{
			// ImageObject for circ writing
			ImageObject *pCircImgObj = new ImageObject(2 * m_pVisTab->m_vectorOctImage.at(frameCount).size(0), 2 * m_pVisTab->m_vectorOctImage.at(frameCount).size(0), 
				temp_ctable.m_colorTableVector.at(OCT_COLORTABLE));

			// Buffer
			np::Uint8Array2 rect_temp(pImgObjVec->at(0)->qindeximg.bits(), pImgObjVec->at(0)->arr.size(0), pImgObjVec->at(0)->arr.size(1));
			
			// Circularize
			if (checkList.bCirc)
				(*m_pVisTab->m_pCirc)(rect_temp, pCircImgObj->qindeximg.bits(), "vertical");
			
			// Longitudinal
			if (checkList.bLongi)
			{
				IppiSize roi_longi = { 1, m_pVisTab->m_vectorOctImage.at(0).size(0) };
				int n2Alines = m_pVisTab->m_vectorOctImage.at(0).size(1) / 2;

				tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)n2Alines),
					[&](const tbb::blocked_range<size_t>& r) {
					for (size_t i = r.begin(); i != r.end(); ++i)
					{
						ippiCopy_8u_C1R(&rect_temp((int)i, 0), sizeof(uint8_t) * 2 * n2Alines,
							pImgObjVecLongi[0]->at((int)i)->qindeximg.bits() + frameCount, sizeof(uint8_t) * nTotalFrame4, roi_longi);
						ippiCopy_8u_C1R(&rect_temp((int)i + n2Alines, 0), sizeof(uint8_t) * 2 * n2Alines,
							pImgObjVecLongi[0]->at((int)i)->qindeximg.bits() + m_pVisTab->m_vectorOctImage.at(0).size(0) * nTotalFrame4 + frameCount, sizeof(uint8_t) * nTotalFrame4, roi_longi);
					}
				});
			}

			// Vector pushing back
			pImgObjVecCirc->push_back(pCircImgObj);
		}
		else
		{
			// ImageObject for circ writing
			ImageObject *pCircImgObj[3];
			if (!checkList.bMulti)
			{
				for (int i = 0; i < 3; i++)
				{
					// ImageObject for circ writing
					pCircImgObj[i] = new ImageObject(2 * m_pVisTab->m_vectorOctImage.at(frameCount).size(0), 2 * m_pVisTab->m_vectorOctImage.at(frameCount).size(0),
						temp_ctable.m_colorTableVector.at(OCT_COLORTABLE));

					// Buffer
					np::Uint8Array2 rect_temp(pImgObjVec->at(0)->qrgbimg.bits(), 3 * pImgObjVec->at(0)->arr.size(0), pImgObjVec->at(0)->arr.size(1));

					if (checkList.bCh[i] && (checkList.bCirc || checkList.bLongi))
					{
						// Paste FLIM color ring to RGB rect image
						memcpy(pImgObjVec->at(0)->qrgbimg.bits() + 3 * pImgObjVec->at(0)->arr.size(0) * (pImgObjVec->at(0)->arr.size(1) - 2 * RING_THICKNESS),
							pImgObjVec->at(1 + 2 * i)->qrgbimg.bits(), pImgObjVec->at(1 + 2 * i)->qrgbimg.byteCount()); // Intensity
						memcpy(pImgObjVec->at(0)->qrgbimg.bits() + 3 * pImgObjVec->at(0)->arr.size(0) * (pImgObjVec->at(0)->arr.size(1) - 1 * RING_THICKNESS),
							pImgObjVec->at(2 + 2 * i)->qrgbimg.bits(), pImgObjVec->at(2 + 2 * i)->qrgbimg.byteCount()); // Lifetime

						// Circularize						
						(*m_pVisTab->m_pCirc)(rect_temp, pCircImgObj[i]->qrgbimg.bits(), "vertical", "rgb");
					}

					// Longitudinal
					if (checkList.bLongi)
					{
						IppiSize roi_longi = { 3, m_pVisTab->m_vectorOctImage.at(0).size(0) };
						int n2Alines = m_pVisTab->m_vectorOctImage.at(0).size(1) / 2;

						tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)n2Alines),
							[&](const tbb::blocked_range<size_t>& r) {
							for (size_t j = r.begin(); j != r.end(); ++j)
							{
								ippiCopy_8u_C1R(&rect_temp(3 * (int)j, 0), sizeof(uint8_t) * 2 * 3 * n2Alines,
									pImgObjVecLongi[i]->at((int)j)->qrgbimg.bits() + 3 * frameCount, sizeof(uint8_t) * 3 * nTotalFrame4, roi_longi);
								ippiCopy_8u_C1R(&rect_temp(3 * ((int)j + n2Alines), 0), sizeof(uint8_t) * 2 * 3 * n2Alines,
									pImgObjVecLongi[i]->at((int)j)->qrgbimg.bits() + m_pVisTab->m_vectorOctImage.at(0).size(0) * 3 * nTotalFrame4 + 3 * frameCount, sizeof(uint8_t) * 3 * nTotalFrame4, roi_longi);
							}
						});
					}

					// Vector pushing back
					pImgObjVecCirc->push_back(pCircImgObj[i]);
				}
			}
			else
			{
				int nCh = (checkList.bCh[0] ? 1 : 0) + (checkList.bCh[1] ? 1 : 0) + (checkList.bCh[2] ? 1 : 0);
				int n = 0;

				// ImageObject for circ writing
				pCircImgObj[0] = new ImageObject(2 * m_pVisTab->m_vectorOctImage.at(frameCount).size(0), 2 * m_pVisTab->m_vectorOctImage.at(frameCount).size(0),
					temp_ctable.m_colorTableVector.at(OCT_COLORTABLE));

				// Buffer
				np::Uint8Array2 rect_temp(pImgObjVec->at(0)->qrgbimg.bits(), 3 * pImgObjVec->at(0)->arr.size(0), pImgObjVec->at(0)->arr.size(1));

				// Paste FLIM color ring to RGB rect image
				for (int i = 0; i < 3; i++)
					if (checkList.bCh[i] && (checkList.bCirc || checkList.bLongi))
						memcpy(pImgObjVec->at(0)->qrgbimg.bits() + 3 * pImgObjVec->at(0)->arr.size(0) * (pImgObjVec->at(0)->arr.size(1) - (nCh - n++) * RING_THICKNESS),
							pImgObjVec->at(1 + 2 * i)->qrgbimg.bits(), pImgObjVec->at(1 + 2 * i)->qrgbimg.byteCount());				

				// Circularize
				(*m_pVisTab->m_pCirc)(rect_temp, pCircImgObj[0]->qrgbimg.bits(), "vertical", "rgb");

				// Longitudinal
				if (checkList.bLongi)
				{
					IppiSize roi_longi = { 3, m_pVisTab->m_vectorOctImage.at(0).size(0) };
					int n2Alines = m_pVisTab->m_vectorOctImage.at(0).size(1) / 2;

					tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)n2Alines),
						[&](const tbb::blocked_range<size_t>& r) {
						for (size_t j = r.begin(); j != r.end(); ++j)
						{
							ippiCopy_8u_C1R(&rect_temp(3 * (int)j, 0), sizeof(uint8_t) * 2 * 3 * n2Alines,
								pImgObjVecLongi[0]->at((int)j)->qrgbimg.bits() + 3 * frameCount, sizeof(uint8_t) * 3 * nTotalFrame4, roi_longi);
							ippiCopy_8u_C1R(&rect_temp(3 * ((int)j + n2Alines), 0), sizeof(uint8_t) * 2 * 3 * n2Alines,
								pImgObjVecLongi[0]->at((int)j)->qrgbimg.bits() + m_pVisTab->m_vectorOctImage.at(0).size(0) * 3 * nTotalFrame4 + 3 * frameCount, sizeof(uint8_t) * 3 * nTotalFrame4, roi_longi);
						}
					});
				}

				// Vector pushing back
				pImgObjVecCirc->push_back(pCircImgObj[0]);
			}
		}	

		frameCount++;

		// Push the buffers to sync Queues
		m_syncQueueCircWriting.push(pImgObjVecCirc);

		// Delete ImageObjects
		for (int i = 0; i < 7; i++)
			delete pImgObjVec->at(i);
		delete pImgObjVec;
	}

	// Write longtiduinal images
	if (checkList.bLongi)
	{
		QString folderName;
		for (int i = 0; i < m_pProcessingTab->m_path.length(); i++)
			if (m_pProcessingTab->m_path.at(i) == QChar('/')) folderName = m_pProcessingTab->m_path.right(m_pProcessingTab->m_path.length() - i - 1);

		int start = m_pLineEdit_RangeStart->text().toInt();
		int end = m_pLineEdit_RangeEnd->text().toInt();

		if (!checkList.bCh[0] && !checkList.bCh[1] && !checkList.bCh[2])
		{
			QString longiPath;
			longiPath = m_pProcessingTab->m_path + QString("/longi_image[%1 %2]_gray[%3 %4]/").arg(start).arg(end).arg(m_pConfig->octGrayRange.min).arg(m_pConfig->octGrayRange.max);
			QDir().mkdir(longiPath);

			int alineCount = 0;
			while (alineCount < m_pVisTab->m_vectorOctImage.at(0).size(1) / 2)
			{
				// Write longi images
				ippiMirror_8u_C1IR(pImgObjVecLongi[0]->at(alineCount)->qindeximg.bits(), sizeof(uint8_t) * nTotalFrame4, { nTotalFrame4, m_pVisTab->m_vectorOctImage.at(0).size(0) }, ippAxsHorizontal);
				if (!checkList.bLongiResize)
					pImgObjVecLongi[0]->at(alineCount)->qindeximg.copy(start - 1, 0, end - start + 1, 2 * m_pVisTab->m_vectorOctImage.at(0).size(0)).
					save(longiPath + QString("longi_%1_%2.bmp").arg(folderName).arg(alineCount + 1, 4, 10, (QChar)'0'), "bmp");
				else
					pImgObjVecLongi[0]->at(alineCount)->qindeximg.copy(start - 1, 0, end - start + 1, 2 * m_pVisTab->m_vectorOctImage.at(0).size(0)).
					scaled(checkList.nLongiWidth, checkList.nLongiHeight, Qt::IgnoreAspectRatio, m_defaultTransformation).
					save(longiPath + QString("longi_%1_%2.bmp").arg(folderName).arg(alineCount + 1, 4, 10, (QChar)'0'), "bmp");

				// Delete ImageObjects
				delete pImgObjVecLongi[0]->at(alineCount);
				delete pImgObjVecLongi[1]->at(alineCount);
				delete pImgObjVecLongi[2]->at(alineCount++);
			}
		}
		else
		{
			QString longiPath[3];
			if (!checkList.bMulti)
			{
				for (int i = 0; i < 3; i++)
				{
					if (checkList.bCh[i])
					{
						if (!m_pVisTab->getIntensityRatioMode())
							longiPath[i] = m_pProcessingTab->m_path + QString("/longi_image[%1 %2]_gray[%3 %4]_ch%5_i[%6 %7]_t[%8 %9]/")
								.arg(start).arg(end).arg(m_pConfig->octGrayRange.min).arg(m_pConfig->octGrayRange.max)
								.arg(i + 1).arg(m_pConfig->flimIntensityRange[i].min, 2, 'f', 1).arg(m_pConfig->flimIntensityRange[i].max, 2, 'f', 1)
								.arg(m_pConfig->flimLifetimeRange[i].min, 2, 'f', 1).arg(m_pConfig->flimLifetimeRange[i].max, 2, 'f', 1);
						else
							longiPath[i] = m_pProcessingTab->m_path + QString("/longi_image[%1 %2]_gray[%3 %4]_ch%5_%6_ir[%7 %8]_t[%9 %10]/")
							.arg(start).arg(end).arg(m_pConfig->octGrayRange.min).arg(m_pConfig->octGrayRange.max)
							.arg(i + 1).arg(ratio_index[i][m_pConfig->flimIntensityRatioRefIdx[i]])
							.arg(m_pConfig->flimIntensityRatioRange[i][m_pConfig->flimIntensityRatioRefIdx[i]].min, 2, 'f', 1)
							.arg(m_pConfig->flimIntensityRatioRange[i][m_pConfig->flimIntensityRatioRefIdx[i]].max, 2, 'f', 1)
							.arg(m_pConfig->flimLifetimeRange[i].min, 2, 'f', 1).arg(m_pConfig->flimLifetimeRange[i].max, 2, 'f', 1);

						if (checkList.bLongi) QDir().mkdir(longiPath[i]);
					}
				}
			}
			else
			{
				if (!m_pVisTab->getIntensityRatioMode())
					longiPath[0] = m_pProcessingTab->m_path + QString("/longi_merged[%1 %2]_gray[%3 %4]_ch%5%6%7_i[%8 %9][%10 %11][%12 %13]_t[%14 %15][%16 %17][%18 19]/")
						.arg(start).arg(end).arg(m_pConfig->octGrayRange.min).arg(m_pConfig->octGrayRange.max)
						.arg(checkList.bCh[0] ? "1" : "").arg(checkList.bCh[1] ? "2" : "").arg(checkList.bCh[2] ? "3" : "")
						.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimIntensityRange[0].min, 'f', 1) : "")
						.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimIntensityRange[0].max, 'f', 1) : "")
						.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimIntensityRange[1].min, 'f', 1) : "")
						.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimIntensityRange[1].max, 'f', 1) : "")
						.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimIntensityRange[2].min, 'f', 1) : "")
						.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimIntensityRange[2].max, 'f', 1) : "")
						.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimLifetimeRange[0].min, 'f', 1) : "")
						.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimLifetimeRange[0].max, 'f', 1) : "")
						.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimLifetimeRange[1].min, 'f', 1) : "")
						.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimLifetimeRange[1].max, 'f', 1) : "")
						.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimLifetimeRange[2].min, 'f', 1) : "")
						.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimLifetimeRange[2].max, 'f', 1) : "");
				else
					longiPath[0] = m_pProcessingTab->m_path + QString("/longi_merged[%1 %2]_gray[%3 %4]_ch%5%6/%7%8/%9%10_ir[%11 %12][%13 %14][%15 %16]_t[%17 %18][%19 %20][%21 22]/")
					.arg(start).arg(end).arg(m_pConfig->octGrayRange.min).arg(m_pConfig->octGrayRange.max)
					.arg(checkList.bCh[0] ? "1" : "").arg(checkList.bCh[0] ? QString::number(ratio_index[0][m_pConfig->flimIntensityRatioRefIdx[0]]) : "")
					.arg(checkList.bCh[1] ? "2" : "").arg(checkList.bCh[1] ? QString::number(ratio_index[1][m_pConfig->flimIntensityRatioRefIdx[1]]) : "")
					.arg(checkList.bCh[2] ? "3" : "").arg(checkList.bCh[2] ? QString::number(ratio_index[2][m_pConfig->flimIntensityRatioRefIdx[2]]) : "")
					.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimIntensityRatioRange[0][m_pConfig->flimIntensityRatioRefIdx[0]].min, 'f', 1) : "")
					.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimIntensityRatioRange[0][m_pConfig->flimIntensityRatioRefIdx[0]].max, 'f', 1) : "")
					.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimIntensityRatioRange[1][m_pConfig->flimIntensityRatioRefIdx[1]].min, 'f', 1) : "")
					.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimIntensityRatioRange[1][m_pConfig->flimIntensityRatioRefIdx[1]].max, 'f', 1) : "")
					.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimIntensityRatioRange[2][m_pConfig->flimIntensityRatioRefIdx[2]].min, 'f', 1) : "")
					.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimIntensityRatioRange[2][m_pConfig->flimIntensityRatioRefIdx[2]].max, 'f', 1) : "")
					.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimLifetimeRange[0].min, 'f', 1) : "")
					.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimLifetimeRange[0].max, 'f', 1) : "")
					.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimLifetimeRange[1].min, 'f', 1) : "")
					.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimLifetimeRange[1].max, 'f', 1) : "")
					.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimLifetimeRange[2].min, 'f', 1) : "")
					.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimLifetimeRange[2].max, 'f', 1) : "");
				if (checkList.bLongi) QDir().mkdir(longiPath[0]);
			}

			int alineCount = 0;
			while (alineCount < m_pVisTab->m_vectorOctImage.at(0).size(1) / 2)
			{
				if (checkList.bLongi)
				{
					if (!checkList.bMulti)
					{
						for (int i = 0; i < 3; i++)
						{
							if (checkList.bCh[i])
							{
								// Write longi images
								ippiMirror_8u_C1IR(pImgObjVecLongi[i]->at(alineCount)->qrgbimg.bits(), sizeof(uint8_t) * 3 * nTotalFrame4, { 3 * nTotalFrame4, m_pVisTab->m_vectorOctImage.at(0).size(0) }, ippAxsHorizontal);
								if (!checkList.bLongiResize)
									pImgObjVecLongi[i]->at(alineCount)->qrgbimg.copy(start - 1, 0, end - start + 1, 2 * m_pVisTab->m_vectorOctImage.at(0).size(0)).
									save(longiPath[i] + QString("longi_%1_%2.bmp").arg(folderName).arg(alineCount + 1, 4, 10, (QChar)'0'), "bmp");
								else
									pImgObjVecLongi[i]->at(alineCount)->qrgbimg.copy(start - 1, 0, end - start + 1, 2 * m_pVisTab->m_vectorOctImage.at(0).size(0)).
									scaled(checkList.nLongiWidth, checkList.nLongiHeight, Qt::IgnoreAspectRatio, m_defaultTransformation).
									save(longiPath[i] + QString("longi_%1_%2.bmp").arg(folderName).arg(alineCount + 1, 4, 10, (QChar)'0'), "bmp");
							}
						}
					}
					else
					{
						// Write longi images
						ippiMirror_8u_C1IR(pImgObjVecLongi[0]->at(alineCount)->qrgbimg.bits(), sizeof(uint8_t) * 3 * nTotalFrame4, { 3 * nTotalFrame4, m_pVisTab->m_vectorOctImage.at(0).size(0) }, ippAxsHorizontal);
						if (!checkList.bLongiResize)
							pImgObjVecLongi[0]->at(alineCount)->qrgbimg.copy(start - 1, 0, end - start + 1, 2 * m_pVisTab->m_vectorOctImage.at(0).size(0)).
							save(longiPath[0] + QString("longi_%1_%2.bmp").arg(folderName).arg(alineCount + 1, 4, 10, (QChar)'0'), "bmp");
						else
							pImgObjVecLongi[0]->at(alineCount)->qrgbimg.copy(start - 1, 0, end - start + 1, 2 * m_pVisTab->m_vectorOctImage.at(0).size(0)).
							scaled(checkList.nLongiWidth, checkList.nLongiHeight, Qt::IgnoreAspectRatio, m_defaultTransformation).
							save(longiPath[0] + QString("longi_%1_%2.bmp").arg(folderName).arg(alineCount + 1, 4, 10, (QChar)'0'), "bmp");
					}
				}

				// Delete ImageObjects
				delete pImgObjVecLongi[0]->at(alineCount);
				delete pImgObjVecLongi[1]->at(alineCount);
				delete pImgObjVecLongi[2]->at(alineCount++);
			}
		}
	}

	if (checkList.bLongi)
		for (int i = 0; i < 3; i++)
			delete pImgObjVecLongi[i];
}

void SaveResultDlg::circWriting(CrossSectionCheckList checkList)
{
	int nTotalFrame = (int)m_pVisTab->m_vectorOctImage.size();
	QString folderName;
	for (int i = 0; i < m_pProcessingTab->m_path.length(); i++)
		if (m_pProcessingTab->m_path.at(i) == QChar('/')) folderName = m_pProcessingTab->m_path.right(m_pProcessingTab->m_path.length() - i - 1);

	int start = m_pLineEdit_RangeStart->text().toInt();
	int end = m_pLineEdit_RangeEnd->text().toInt();

	if (!checkList.bCh[0] && !checkList.bCh[1] && !checkList.bCh[2])
	{
		QString circPath;
		circPath = m_pProcessingTab->m_path + QString("/circ_image_gray[%1 %2]/").arg(m_pConfig->octGrayRange.min).arg(m_pConfig->octGrayRange.max);;
		if (checkList.bCirc) QDir().mkdir(circPath);

		int frameCount = 0;
		while (frameCount < nTotalFrame)
		{
			// Get the buffer from the previous sync Queue
			ImgObjVector *pImgObjVecCirc = m_syncQueueCircWriting.pop();

			// Range test
			if (((frameCount + 1) >= start) && ((frameCount + 1) <= end))
			{
				// Write circ images
				if (checkList.bCirc)
				{
					if (!checkList.bCircResize)
						pImgObjVecCirc->at(0)->qindeximg.save(circPath + QString("circ_%1_%2.bmp").arg(folderName).arg(frameCount + 1, 3, 10, (QChar)'0'), "bmp");
					else
						pImgObjVecCirc->at(0)->qindeximg.scaled(checkList.nCircDiameter, checkList.nCircDiameter, Qt::IgnoreAspectRatio, m_defaultTransformation).
						save(circPath + QString("circ_%1_%2.bmp").arg(folderName).arg(frameCount + 1, 3, 10, (QChar)'0'), "bmp");
				}
			}
		
			frameCount++;
			emit savedSingleFrame(m_nSavedFrames++);

			// Delete ImageObjects
			delete pImgObjVecCirc->at(0);
			delete pImgObjVecCirc;
		}
	}
	else
	{
		QString circPath[3];
		if (!checkList.bMulti)
		{
			for (int i = 0; i < 3; i++)
			{
				if (checkList.bCh[i])
				{
					if (!m_pVisTab->getIntensityRatioMode())
						circPath[i] = m_pProcessingTab->m_path + QString("/circ_image_gray[%1 %2]_ch%3_i[%4 %5]_t[%6 %7]/")
						.arg(m_pConfig->octGrayRange.min).arg(m_pConfig->octGrayRange.max)
						.arg(i + 1).arg(m_pConfig->flimIntensityRange[i].min, 2, 'f', 1).arg(m_pConfig->flimIntensityRange[i].max, 2, 'f', 1)
						.arg(m_pConfig->flimLifetimeRange[i].min, 2, 'f', 1).arg(m_pConfig->flimLifetimeRange[i].max, 2, 'f', 1);
					else
						circPath[i] = m_pProcessingTab->m_path + QString("/circ_image_gray[%1 %2]_ch%3_%4_ir[%5 %6]_t[%7 %8]/")
						.arg(m_pConfig->octGrayRange.min).arg(m_pConfig->octGrayRange.max)
						.arg(i + 1).arg(ratio_index[i][m_pConfig->flimIntensityRatioRefIdx[i]])
						.arg(m_pConfig->flimIntensityRatioRange[i][m_pConfig->flimIntensityRatioRefIdx[i]].min, 2, 'f', 1)
						.arg(m_pConfig->flimIntensityRatioRange[i][m_pConfig->flimIntensityRatioRefIdx[i]].max, 2, 'f', 1)
						.arg(m_pConfig->flimLifetimeRange[i].min, 2, 'f', 1).arg(m_pConfig->flimLifetimeRange[i].max, 2, 'f', 1);
					
					if (checkList.bCirc) QDir().mkdir(circPath[i]);
				}
			}
		}
		else
		{
			if (!m_pVisTab->getIntensityRatioMode())
				circPath[0] = m_pProcessingTab->m_path + QString("/circ_merged_gray[%1 %2]_ch%3%4%5_i[%6 %7][%8 %9][%10 %11]_t[%12 %13][%14 %15][%16 %17]/")
				.arg(m_pConfig->octGrayRange.min).arg(m_pConfig->octGrayRange.max)
				.arg(checkList.bCh[0] ? "1" : "").arg(checkList.bCh[1] ? "2" : "").arg(checkList.bCh[2] ? "3" : "")
				.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimIntensityRange[0].min, 'f', 1) : "")
				.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimIntensityRange[0].max, 'f', 1) : "")
				.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimIntensityRange[1].min, 'f', 1) : "")
				.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimIntensityRange[1].max, 'f', 1) : "")
				.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimIntensityRange[2].min, 'f', 1) : "")
				.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimIntensityRange[2].max, 'f', 1) : "")
				.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimLifetimeRange[0].min, 'f', 1) : "")
				.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimLifetimeRange[0].max, 'f', 1) : "")
				.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimLifetimeRange[1].min, 'f', 1) : "")
				.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimLifetimeRange[1].max, 'f', 1) : "")
				.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimLifetimeRange[2].min, 'f', 1) : "")
				.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimLifetimeRange[2].max, 'f', 1) : "");
			else
				circPath[0] = m_pProcessingTab->m_path + QString("/circ_merged_gray[%1 %2]_ch%3%4/%5%6/%7%8_ir[%9 %10][%11 %12][%13 %14]_t[%15 %16][%17 %18][%19 %20]/")
				.arg(m_pConfig->octGrayRange.min).arg(m_pConfig->octGrayRange.max)
				.arg(checkList.bCh[0] ? "1" : "").arg(checkList.bCh[0] ? QString::number(ratio_index[0][m_pConfig->flimIntensityRatioRefIdx[0]]) : "")
				.arg(checkList.bCh[1] ? "2" : "").arg(checkList.bCh[1] ? QString::number(ratio_index[1][m_pConfig->flimIntensityRatioRefIdx[1]]) : "")
				.arg(checkList.bCh[2] ? "3" : "").arg(checkList.bCh[2] ? QString::number(ratio_index[2][m_pConfig->flimIntensityRatioRefIdx[2]]) : "")
				.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimIntensityRatioRange[0][m_pConfig->flimIntensityRatioRefIdx[0]].min, 'f', 1) : "")
				.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimIntensityRatioRange[0][m_pConfig->flimIntensityRatioRefIdx[0]].max, 'f', 1) : "")
				.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimIntensityRatioRange[1][m_pConfig->flimIntensityRatioRefIdx[1]].min, 'f', 1) : "")
				.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimIntensityRatioRange[1][m_pConfig->flimIntensityRatioRefIdx[1]].max, 'f', 1) : "")
				.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimIntensityRatioRange[2][m_pConfig->flimIntensityRatioRefIdx[2]].min, 'f', 1) : "")
				.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimIntensityRatioRange[2][m_pConfig->flimIntensityRatioRefIdx[2]].max, 'f', 1) : "")
				.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimLifetimeRange[0].min, 'f', 1) : "")
				.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimLifetimeRange[0].max, 'f', 1) : "")
				.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimLifetimeRange[1].min, 'f', 1) : "")
				.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimLifetimeRange[1].max, 'f', 1) : "")
				.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimLifetimeRange[2].min, 'f', 1) : "")
				.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimLifetimeRange[2].max, 'f', 1) : "");

			if (checkList.bCirc) QDir().mkdir(circPath[0]);
		}

		int frameCount = 0;
		while (frameCount < nTotalFrame)
		{
			// Get the buffer from the previous sync Queue
			ImgObjVector *pImgObjVecCirc = m_syncQueueCircWriting.pop();

			// Range test
			if (((frameCount + 1) >= start) && ((frameCount + 1) <= end))
			{
				if (checkList.bCirc)
				{
					if (!checkList.bMulti)
					{
						for (int i = 0; i < 3; i++)
						{
							if (checkList.bCh[i])
							{
								// Write circ images
								if (!checkList.bCircResize)
									pImgObjVecCirc->at(i)->qrgbimg.save(circPath[i] + QString("circ_%1_%2.bmp").arg(folderName).arg(frameCount + 1, 3, 10, (QChar)'0'), "bmp");
								else
									pImgObjVecCirc->at(i)->qrgbimg.scaled(checkList.nCircDiameter, checkList.nCircDiameter, Qt::IgnoreAspectRatio, m_defaultTransformation).
									save(circPath[i] + QString("circ_%1_%2.bmp").arg(folderName).arg(frameCount + 1, 3, 10, (QChar)'0'), "bmp");
							}
						}
					}
					else
					{
						// Write circ images
						if (!checkList.bCircResize)
							pImgObjVecCirc->at(0)->qrgbimg.save(circPath[0] + QString("circ_%1_%2.bmp").arg(folderName).arg(frameCount + 1, 3, 10, (QChar)'0'), "bmp");
						else
							pImgObjVecCirc->at(0)->qrgbimg.scaled(checkList.nCircDiameter, checkList.nCircDiameter, Qt::IgnoreAspectRatio, m_defaultTransformation).
							save(circPath[0] + QString("circ_%1_%2.bmp").arg(folderName).arg(frameCount + 1, 3, 10, (QChar)'0'), "bmp");
					}
				}
			}

			frameCount++;
			emit savedSingleFrame(m_nSavedFrames++);

			// Delete ImageObjects
			if (!checkList.bMulti)
			{
				for (int i = 0; i < 3; i++)
					delete pImgObjVecCirc->at(i);
			}
			else
				delete pImgObjVecCirc->at(0);
			delete pImgObjVecCirc;
		}
	}
}
