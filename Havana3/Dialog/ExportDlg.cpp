
#include "ExportDlg.h"

#include <Havana3/MainWindow.h>
#include <Havana3/QResultTab.h>
#include <Havana3/QViewTab.h>
#include <Havana3/Dialog/ViewOptionTab.h>

#include <DataAcquisition/DataProcessing.h>

#include <ippi.h>
#include <ippcc.h>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

#include <iostream>
#include <thread>
#include <chrono>
#include <utility>


ExportDlg::ExportDlg(QWidget *parent) :
    QDialog(parent), m_defaultTransformation(Qt::SmoothTransformation), m_bIsSaving(false)
{
    // Set default size & frame
    setFixedSize(330, 400);
    setWindowFlags(Qt::Tool);
	setWindowTitle("Export");

    // Set main window objects
    m_pResultTab = dynamic_cast<QResultTab*>(parent);
	m_pConfig = m_pResultTab->getMainWnd()->m_pConfiguration;
	m_pConfigTemp = m_pResultTab->getDataProcessing()->getConfigTemp();
	m_pViewTab = m_pResultTab->getViewTab();
	
	// Exporting
	m_pPushButton_Export = new QPushButton(this);
	m_pPushButton_Export->setText("Export");

	m_pProgressBar_Export = new QProgressBar(this);
	m_pProgressBar_Export->setFormat("");
	m_pProgressBar_Export->setValue(0);
	
	m_pLabel_ExportPath = new QLabel(this);
	m_pLabel_ExportPath->setText("Export Path");

	m_pLineEdit_ExportPath = new QLineEdit(this);
	m_pLineEdit_ExportPath->setReadOnly(true);

	m_pPushButton_ExportPath = new QPushButton(this);
	m_pPushButton_ExportPath->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
	m_pPushButton_ExportPath->setFixedSize(40, 25);

	// Set Range
	m_pLabel_Range = new QLabel("Exporting Range  ", this);

	m_pLineEdit_RangeStart = new QLineEdit(this);
	m_pLineEdit_RangeStart->setFixedWidth(30);
	m_pLineEdit_RangeStart->setText(QString::number(1));
	m_pLineEdit_RangeStart->setAlignment(Qt::AlignCenter);

	m_pLineEdit_RangeEnd = new QLineEdit(this);
	m_pLineEdit_RangeEnd->setFixedWidth(30);
	m_pLineEdit_RangeEnd->setText(QString::number(m_pViewTab->m_vectorOctImage.size()));
	m_pLineEdit_RangeEnd->setAlignment(Qt::AlignCenter);

    // Create widgets for saving results (save Cross-sections)	
	m_pCheckBox_CircImage = new QCheckBox(this);
	m_pCheckBox_CircImage->setText("Circ Image");
	m_pCheckBox_LongiImage = new QCheckBox(this);
	m_pCheckBox_LongiImage->setText("Longi Image");

	m_pCheckBox_ResizeCircImage = new QCheckBox(this);
	m_pCheckBox_ResizeCircImage->setText("Resize (diameter)");
	m_pCheckBox_ResizeLongiImage = new QCheckBox(this);
	m_pCheckBox_ResizeLongiImage->setText("Resize (w x h)");

	m_pLineEdit_CircDiameter = new QLineEdit(this);
	m_pLineEdit_CircDiameter->setFixedWidth(35);
	m_pLineEdit_CircDiameter->setText(QString::number(2 * m_pViewTab->m_vectorOctImage.at(0).size(0)));
	m_pLineEdit_CircDiameter->setAlignment(Qt::AlignCenter);
	m_pLineEdit_CircDiameter->setDisabled(true);
	m_pLineEdit_LongiWidth = new QLineEdit(this);
	m_pLineEdit_LongiWidth->setFixedWidth(35);
	m_pLineEdit_LongiWidth->setText(QString::number(m_pViewTab->m_vectorOctImage.size()));
	m_pLineEdit_LongiWidth->setAlignment(Qt::AlignCenter);
	m_pLineEdit_LongiWidth->setDisabled(true);
	m_pLineEdit_LongiHeight = new QLineEdit(this);
	m_pLineEdit_LongiHeight->setFixedWidth(35);
	m_pLineEdit_LongiHeight->setText(QString::number(2 * m_pViewTab->m_vectorOctImage.at(0).size(0)));
	m_pLineEdit_LongiHeight->setAlignment(Qt::AlignCenter);
	m_pLineEdit_LongiHeight->setDisabled(true);

	m_pCheckBox_CrossSectionCh1 = new QCheckBox(this);
	m_pCheckBox_CrossSectionCh1->setText("Channel 1");
	m_pCheckBox_CrossSectionCh2 = new QCheckBox(this);
	m_pCheckBox_CrossSectionCh2->setText("Channel 2");
	m_pCheckBox_CrossSectionCh3 = new QCheckBox(this);
	m_pCheckBox_CrossSectionCh3->setText("Channel 3");

	// Save En Face Maps
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

	m_pCheckBox_RFPrediction = new QCheckBox(this);
	m_pCheckBox_RFPrediction->setText("RF Pred");
	m_pCheckBox_RFPrediction->setEnabled((m_pViewTab->m_plaqueCompositionProbMap.at(0).length() != 0) && (m_pViewTab->m_plaqueCompositionMap.at(0).length() != 0));

	m_pCheckBox_SVMSoftmax = new QCheckBox(this);
	m_pCheckBox_SVMSoftmax->setText("SVM Soft");
	m_pCheckBox_SVMSoftmax->setEnabled((m_pViewTab->m_plaqueCompositionProbMap.at(1).length() != 0) && (m_pViewTab->m_plaqueCompositionMap.at(1).length() != 0));

	m_pCheckBox_SVMLogistics = new QCheckBox(this);
	m_pCheckBox_SVMLogistics->setText("SVM Logit");
	m_pCheckBox_SVMLogistics->setEnabled((m_pViewTab->m_plaqueCompositionProbMap.at(2).length() != 0) && (m_pViewTab->m_plaqueCompositionMap.at(2).length() != 0));
		
	// Create layout
	QVBoxLayout *pVBoxLayout = new QVBoxLayout;
	pVBoxLayout->setSpacing(3);

	QGridLayout *pGridLayout_Path = new QGridLayout;
	pGridLayout_Path->setSpacing(2);
	pGridLayout_Path->addWidget(m_pLabel_ExportPath, 0, 0);
	pGridLayout_Path->addWidget(m_pLineEdit_ExportPath, 1, 0);
	pGridLayout_Path->addWidget(m_pPushButton_ExportPath, 1, 1);


	QHBoxLayout *pHBoxLayout_Range = new QHBoxLayout;
	pHBoxLayout_Range->setSpacing(1);
	pHBoxLayout_Range->addWidget(m_pLabel_Range);
	pHBoxLayout_Range->addWidget(m_pLineEdit_RangeStart);
	pHBoxLayout_Range->addWidget(m_pLineEdit_RangeEnd);
	pHBoxLayout_Range->addStretch(1);

	QGroupBox *pGroupBox_Range = new QGroupBox(this);
	pGroupBox_Range->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	pGroupBox_Range->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	pGroupBox_Range->setLayout(pHBoxLayout_Range);
		

	QGridLayout *pGridLayout_CrossSections = new QGridLayout;
	pGridLayout_CrossSections->setSpacing(1);

	QHBoxLayout *pHBoxLayout_CircResize = new QHBoxLayout;
	pHBoxLayout_CircResize->setSpacing(1);
	pHBoxLayout_CircResize->addWidget(m_pCheckBox_ResizeCircImage);
	pHBoxLayout_CircResize->addWidget(m_pLineEdit_CircDiameter);
	pHBoxLayout_CircResize->addStretch(1);
	
	QHBoxLayout *pHBoxLayout_LongiResize = new QHBoxLayout;
	pHBoxLayout_LongiResize->setSpacing(1);
	pHBoxLayout_LongiResize->addWidget(m_pCheckBox_ResizeLongiImage);
	pHBoxLayout_LongiResize->addWidget(m_pLineEdit_LongiWidth);
	pHBoxLayout_LongiResize->addWidget(m_pLineEdit_LongiHeight);
	pHBoxLayout_LongiResize->addStretch(1);

	QHBoxLayout *pHBoxLayout_CrossSectionCh = new QHBoxLayout;
	pHBoxLayout_CrossSectionCh->setSpacing(1);
	pHBoxLayout_CrossSectionCh->addWidget(m_pCheckBox_CrossSectionCh1);
	pHBoxLayout_CrossSectionCh->addWidget(m_pCheckBox_CrossSectionCh2);
	pHBoxLayout_CrossSectionCh->addWidget(m_pCheckBox_CrossSectionCh3);

	pGridLayout_CrossSections->addWidget(m_pCheckBox_CircImage, 0, 0);
	pGridLayout_CrossSections->addItem(pHBoxLayout_CircResize, 0, 1);
	pGridLayout_CrossSections->addWidget(m_pCheckBox_LongiImage, 1, 0);
	pGridLayout_CrossSections->addItem(pHBoxLayout_LongiResize, 1, 1);
	pGridLayout_CrossSections->addItem(pHBoxLayout_CrossSectionCh, 2, 0, 1, 2);

	m_pGroupBox_CrossSections = new QGroupBox(this);
	m_pGroupBox_CrossSections->setTitle("Cross Sections");
	m_pGroupBox_CrossSections->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_pGroupBox_CrossSections->setLayout(pGridLayout_CrossSections);
	

	QGridLayout *pGridLayout_EnFaceChemogram = new QGridLayout;
	pGridLayout_EnFaceChemogram->setSpacing(1);
	
	pGridLayout_EnFaceChemogram->addWidget(m_pCheckBox_RawData, 0, 0);
	pGridLayout_EnFaceChemogram->addWidget(m_pCheckBox_ScaledImage, 0, 1, 1, 2);

	QHBoxLayout *pHBoxLayout_EnFaceCh = new QHBoxLayout;
	pHBoxLayout_EnFaceCh->setSpacing(1);
	pHBoxLayout_EnFaceCh->addWidget(m_pCheckBox_EnFaceCh1);
	pHBoxLayout_EnFaceCh->addWidget(m_pCheckBox_EnFaceCh2);
	pHBoxLayout_EnFaceCh->addWidget(m_pCheckBox_EnFaceCh3);
	pGridLayout_EnFaceChemogram->addItem(pHBoxLayout_EnFaceCh, 1, 0, 1, 3);

	QHBoxLayout *pHBoxLayout_EnFaceMl = new QHBoxLayout;
	pHBoxLayout_EnFaceMl->setSpacing(1);
	pHBoxLayout_EnFaceMl->addWidget(m_pCheckBox_RFPrediction);
	pHBoxLayout_EnFaceMl->addWidget(m_pCheckBox_SVMSoftmax);
	pHBoxLayout_EnFaceMl->addWidget(m_pCheckBox_SVMLogistics);
	pGridLayout_EnFaceChemogram->addItem(pHBoxLayout_EnFaceMl, 2, 0, 1, 3);
	
	m_pGroupBox_EnFaceChemogram = new QGroupBox(this);
	m_pGroupBox_EnFaceChemogram->setTitle("En Face Chemogram");
	m_pGroupBox_EnFaceChemogram->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_pGroupBox_EnFaceChemogram->setLayout(pGridLayout_EnFaceChemogram);

	// Set layout
	pVBoxLayout->addWidget(m_pPushButton_Export);
	pVBoxLayout->addWidget(m_pProgressBar_Export);
	pVBoxLayout->addItem(pGridLayout_Path);
	pVBoxLayout->addWidget(pGroupBox_Range);
	pVBoxLayout->addWidget(m_pGroupBox_CrossSections);
	pVBoxLayout->addWidget(m_pGroupBox_EnFaceChemogram);
		
    this->setLayout(pVBoxLayout);

    // Connect
	connect(m_pPushButton_Export, SIGNAL(clicked(bool)), this, SLOT(exportResults()));
	connect(this, SIGNAL(savedSingleFrame(int)), m_pProgressBar_Export, SLOT(setValue(int)));
	
	connect(m_pPushButton_ExportPath, SIGNAL(clicked(bool)), this, SLOT(findExportPath()));

	connect(m_pLineEdit_RangeStart, SIGNAL(textChanged(const QString &)), this, SLOT(setRange()));
	connect(m_pLineEdit_RangeEnd, SIGNAL(textChanged(const QString &)), this, SLOT(setRange()));

	connect(m_pCheckBox_CircImage, SIGNAL(toggled(bool)), this, SLOT(checkCircImage(bool)));
	connect(m_pCheckBox_LongiImage, SIGNAL(toggled(bool)), this, SLOT(checkLongiImage(bool)));

	connect(m_pCheckBox_ResizeCircImage, SIGNAL(toggled(bool)), this, SLOT(enableCircResize(bool)));
	connect(m_pCheckBox_ResizeLongiImage, SIGNAL(toggled(bool)), this, SLOT(enableLongiResize(bool)));

	connect(m_pLineEdit_CircDiameter, SIGNAL(textEdited(const QString &)), this, SLOT(checkResizeValue()));
	connect(m_pLineEdit_LongiWidth, SIGNAL(textEdited(const QString &)), this, SLOT(checkResizeValue()));
	connect(m_pLineEdit_LongiHeight, SIGNAL(textEdited(const QString &)), this, SLOT(checkResizeValue()));
	
	connect(m_pCheckBox_RawData, SIGNAL(toggled(bool)), this, SLOT(checkEnFaceOptions()));
	connect(m_pCheckBox_ScaledImage, SIGNAL(toggled(bool)), this, SLOT(checkEnFaceOptions()));
	connect(m_pCheckBox_EnFaceCh1, SIGNAL(toggled(bool)), this, SLOT(checkEnFaceOptions()));
	connect(m_pCheckBox_EnFaceCh2, SIGNAL(toggled(bool)), this, SLOT(checkEnFaceOptions()));
	connect(m_pCheckBox_EnFaceCh3, SIGNAL(toggled(bool)), this, SLOT(checkEnFaceOptions()));
	connect(m_pCheckBox_RFPrediction, SIGNAL(toggled(bool)), this, SLOT(checkEnFaceOptions()));
	connect(m_pCheckBox_SVMSoftmax, SIGNAL(toggled(bool)), this, SLOT(checkEnFaceOptions()));
	connect(m_pCheckBox_SVMLogistics, SIGNAL(toggled(bool)), this, SLOT(checkEnFaceOptions()));

	connect(this, SIGNAL(setWidgets(bool, int)), this, SLOT(setWidgetsEnabled(bool, int)));

	// Initialize
	m_pCheckBox_CircImage->setChecked(false);
	m_pCheckBox_LongiImage->setChecked(false);
	//m_pCheckBox_CrossSectionCh1->setChecked(true);
	//m_pCheckBox_CrossSectionCh2->setChecked(true);
	//m_pCheckBox_CrossSectionCh3->setChecked(true);

	m_pCheckBox_RawData->setChecked(true);
	m_pCheckBox_ScaledImage->setChecked(true);
	m_pCheckBox_EnFaceCh1->setChecked(true);
	m_pCheckBox_EnFaceCh2->setChecked(true);
	m_pCheckBox_EnFaceCh3->setChecked(true);

	m_exportPath = m_pResultTab->getRecordInfo().filename;
	for (int i = m_exportPath.size() - 1; i >= 0; i--)
	{
		int slash_pos = i;
		if (m_exportPath.at(i) == '/')
		{
			m_exportPath = m_exportPath.left(slash_pos);
			break;
		}
	}

	m_pLineEdit_ExportPath->setText(m_exportPath);
	m_pLineEdit_ExportPath->setCursorPosition(0);

	//exportResults();
}

ExportDlg::~ExportDlg()
{
}

void ExportDlg::closeEvent(QCloseEvent *e)
{
	if (m_bIsSaving)
		e->ignore();
	else
		finished(0);
}

void ExportDlg::keyPressEvent(QKeyEvent *e)
{
	if (e->key() != Qt::Key_Escape)
		QDialog::keyPressEvent(e);
}


void ExportDlg::exportResults()
{
	saveEnFaceMaps();
	saveCrossSections();
}

void ExportDlg::findExportPath()
{
	QString path = QFileDialog::getExistingDirectory(nullptr, "Select Export Path...", QDir::homePath(),
		QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog);
	if (path != "")
	{
		m_exportPath = path;
		m_pLineEdit_ExportPath->setText(m_exportPath);
		m_pLineEdit_ExportPath->setCursorPosition(0);
	}
}

void ExportDlg::saveCrossSections()
{
	std::thread t1([&]() {
		
		// Check Status /////////////////////////////////////////////////////////////////////////////
		CrossSectionCheckList checkList;
		checkList.bCirc = m_pCheckBox_CircImage->isChecked();
		checkList.bLongi = m_pCheckBox_LongiImage->isChecked();
		checkList.bCircResize = m_pCheckBox_ResizeCircImage->isChecked();
		checkList.bLongiResize = m_pCheckBox_ResizeLongiImage->isChecked();
		checkList.nCircDiameter = m_pLineEdit_CircDiameter->text().toInt();
		checkList.nLongiWidth = m_pLineEdit_LongiWidth->text().toInt();
		checkList.nLongiHeight = m_pLineEdit_LongiHeight->text().toInt();

		checkList.bCh[0] = m_pCheckBox_CrossSectionCh1->isChecked();
		checkList.bCh[1] = m_pCheckBox_CrossSectionCh2->isChecked();
		checkList.bCh[2] = m_pCheckBox_CrossSectionCh3->isChecked();
		
		if (checkList.bCirc || checkList.bLongi)
		{
			// Scaling en face FLIm maps first //////////////////////////////////////////////////////////
			int frames = (int)m_pViewTab->m_vectorOctImage.size();
			int frames4 = ROUND_UP_4S(frames);
			int alines = m_pViewTab->m_lifetimeMap.at(0).size(0);
			ColorTable temp_ctable;

			ImageObject* pImgObjIntensityMap = new ImageObject(frames4, alines, temp_ctable.m_colorTableVector.at(ColorTable::gray));
			std::vector<ImageObject*> vectorLifetimeMap;
			for (int i = 0; i < 3; i++)
			{
				ImageObject* pImgObjLifetimeMap = new ImageObject(frames4, alines, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE));
				m_pViewTab->scaleFLImEnFaceMap(pImgObjIntensityMap, pImgObjLifetimeMap, nullptr, nullptr, nullptr, VisualizationMode::_FLIM_PARAMETERS_, i, FLImParameters::_LIFETIME_, 0);

				// Push to the vector
				vectorLifetimeMap.push_back(pImgObjLifetimeMap);
			}
			delete pImgObjIntensityMap;

			// Set Widgets //////////////////////////////////////////////////////////////////////////////
			emit setWidgets(false, frames + (checkList.bLongi ? alines / 2 - 1 : 0));
			m_nSavedFrames = 0;

			// Scaling Images ///////////////////////////////////////////////////////////////////////////
			std::thread scaleImages([&]() { scaling(m_pViewTab->m_vectorOctImage, vectorLifetimeMap, checkList); });

			// Converting Images ////////////////////////////////////////////////////////////////////////
			std::thread convertImages([&]() { converting(checkList); });

			// Circularizing ////////////////////////////////////////////////////////////////////////////		
			std::thread circularizeImages([&]() { circularizing(checkList); });

			// Circ Writing /////////////////////////////////////////////////////////////////////////////
			std::thread writeCircImages([&]() { circWriting(checkList); });

			// Wait for threads end /////////////////////////////////////////////////////////////////////
			scaleImages.join();
			convertImages.join();
			circularizeImages.join();
			writeCircImages.join();

			// Reset Widgets ////////////////////////////////////////////////////////////////////////////
			emit setWidgets(true, 0);

			for (auto i = vectorLifetimeMap.begin(); i != vectorLifetimeMap.end(); ++i)
				delete (*i);
		}

		// Open Folder //////////////////////////////////////////////////////////////////////////////
		QDesktopServices::openUrl(QUrl("file:///" + m_exportPath));
	});

	t1.detach();
}

void ExportDlg::saveEnFaceMaps()
{
	// Check Status /////////////////////////////////////////////////////////////////////////////
	EnFaceCheckList checkList;
	checkList.bRaw = m_pCheckBox_RawData->isChecked();
	checkList.bScaled = m_pCheckBox_ScaledImage->isChecked();
	checkList.bCh[0] = m_pCheckBox_EnFaceCh1->isChecked();
	checkList.bCh[1] = m_pCheckBox_EnFaceCh2->isChecked();
	checkList.bCh[2] = m_pCheckBox_EnFaceCh3->isChecked();
	checkList.bMlPred[0] = m_pCheckBox_RFPrediction->isChecked();
	checkList.bMlPred[1] = m_pCheckBox_SVMSoftmax->isChecked();
	checkList.bMlPred[2] = m_pCheckBox_SVMLogistics->isChecked();
		
	int start = m_pLineEdit_RangeStart->text().toInt();
	int end = m_pLineEdit_RangeEnd->text().toInt();

	int flimAlines = m_pViewTab->m_intensityMap.at(0).size(0);
	int frames = (int)m_pViewTab->m_vectorOctImage.size();

	bool isVibCorrted = m_pResultTab->getVibCorrectionButton()->isChecked();

	if (checkList.bRaw || checkList.bScaled)
	{
		// Create directory for saving en face maps /////////////////////////////////////////////////
		QString enFacePath = m_exportPath + "/en_face/";
		if (checkList.bRaw || checkList.bScaled) QDir().mkdir(enFacePath);

		// Raw en face map writing //////////////////////////////////////////////////////////////////
		if (checkList.bRaw)
		{
			for (int i = 0; i < 3; i++)
			{
				if (checkList.bCh[i])
				{
					IppiSize roi_flim = { flimAlines, frames };

					QFile fileIntensity(enFacePath + QString("intensity_range[%1 %2]_ch%3.enface").arg(start).arg(end).arg(i + 1));
					if (false != fileIntensity.open(QIODevice::WriteOnly))
					{
						np::FloatArray2 intensity_map(flimAlines, frames);
						memset(intensity_map, 0, sizeof(float) * intensity_map.length());
						m_pResultTab->getViewTab()->makeDelay(m_pViewTab->m_intensityMap.at(i), intensity_map, !isVibCorrted ? m_pConfigTemp->flimDelaySync : 0);
						///if (m_pConfigTemp->interFrameSync >= 0)
						///	ippiCopy_32f_C1R(m_pViewTab->m_intensityMap.at(i).raw_ptr(), sizeof(float) * roi_flim.width,
						///		&intensity_map(0, m_pConfigTemp->interFrameSync), sizeof(float) * roi_flim.width, { roi_flim.width, roi_flim.height - m_pConfigTemp->interFrameSync });
						///else
						///	ippiCopy_32f_C1R(&m_pViewTab->m_intensityMap.at(i)(0, -m_pConfigTemp->interFrameSync), sizeof(float) * roi_flim.width,
						///		&intensity_map(0, 0), sizeof(float) * roi_flim.width, { roi_flim.width, roi_flim.height + m_pConfigTemp->interFrameSync });
						
						if (m_pConfigTemp->rotatedAlines > 0)
						{
							for (int i = 0; i < roi_flim.height; i++)
							{
								float* pImg = intensity_map.raw_ptr() + i * roi_flim.width;
								std::rotate(pImg, pImg + m_pConfigTemp->rotatedAlines / 4, pImg + roi_flim.width);
							}
						}

						fileIntensity.write(reinterpret_cast<char*>(&intensity_map(0, start - 1)), sizeof(float) * intensity_map.size(0) * (end - start + 1));
						fileIntensity.close();
					}

					QFile fileLifetime(enFacePath + QString("lifetime_range[%1 %2]_ch%3.enface").arg(start).arg(end).arg(i + 1));
					if (false != fileLifetime.open(QIODevice::WriteOnly))
					{
						np::FloatArray2 lifetime_map(flimAlines, frames);
						memset(lifetime_map, 0, sizeof(float) * lifetime_map.length());
						m_pResultTab->getViewTab()->makeDelay(m_pViewTab->m_lifetimeMap.at(i), lifetime_map, !isVibCorrted ? m_pConfigTemp->flimDelaySync : 0);
						///if (m_pConfigTemp->interFrameSync >= 0)
						///	ippiCopy_32f_C1R(m_pViewTab->m_lifetimeMap.at(i).raw_ptr(), sizeof(float) * roi_flim.width,
						///		&lifetime_map(0, m_pConfigTemp->interFrameSync), sizeof(float) * roi_flim.width, { roi_flim.width, roi_flim.height - m_pConfigTemp->interFrameSync });
						///else
						///	ippiCopy_32f_C1R(&m_pViewTab->m_lifetimeMap.at(i)(0, -m_pConfigTemp->interFrameSync), sizeof(float) * roi_flim.width,
						///		&lifetime_map(0, 0), sizeof(float) * roi_flim.width, { roi_flim.width, roi_flim.height + m_pConfigTemp->interFrameSync });

						if (m_pConfigTemp->rotatedAlines > 0)
						{
							for (int i = 0; i < roi_flim.height; i++)
							{
								float* pImg = lifetime_map.raw_ptr() + i * roi_flim.width;
								std::rotate(pImg, pImg + m_pConfigTemp->rotatedAlines / 4, pImg + roi_flim.width);
							}
						}

						fileLifetime.write(reinterpret_cast<char*>(&lifetime_map(0, start - 1)), sizeof(float) * lifetime_map.size(0) * (end - start + 1));
						fileLifetime.close();
					}
				}
			}

			///for (int i = 0; i < 4; i++)
			///{
			///	if ((i == 0) || (checkList.bCh[i - 1]))
			///	{
			///		IppiSize roi_flim = { flimAlines, frames };

			///		QFile filePulsePower(enFacePath + QString("pulse_power_range[%1 %2]_ch%3.enface").arg(start).arg(end).arg(i));
			///		if (false != filePulsePower.open(QIODevice::WriteOnly))
			///		{
			///			np::FloatArray2 pulsepower_map(flimAlines, frames);
			///			memset(pulsepower_map, 0, sizeof(float) * pulsepower_map.length());
			///			if (m_pConfigTemp->interFrameSync >= 0)
			///				ippiCopy_32f_C1R(m_pViewTab->m_pulsepowerMap.at(i).raw_ptr(), sizeof(float) * roi_flim.width,
			///					&pulsepower_map(0, m_pConfigTemp->interFrameSync), sizeof(float) * roi_flim.width, { roi_flim.width, roi_flim.height - m_pConfigTemp->interFrameSync });
			///			else
			///				ippiCopy_32f_C1R(&m_pViewTab->m_pulsepowerMap.at(i)(0, -m_pConfigTemp->interFrameSync), sizeof(float) * roi_flim.width,
			///					&pulsepower_map(0, 0), sizeof(float) * roi_flim.width, { roi_flim.width, roi_flim.height + m_pConfigTemp->interFrameSync });

			///			if (m_pConfigTemp->rotatedAlines > 0)
			///			{
			///				for (int i = 0; i < roi_flim.height; i++)
			///				{
			///					float* pImg = pulsepower_map.raw_ptr() + i * roi_flim.width;
			///					std::rotate(pImg, pImg + m_pConfigTemp->rotatedAlines / 4, pImg + roi_flim.width);
			///				}
			///			}

			///			filePulsePower.write(reinterpret_cast<char*>(&pulsepower_map(0, start - 1)), sizeof(float) * pulsepower_map.size(0) * (end - start + 1));
			///			filePulsePower.close();
			///		}
			///	}
			///}

			if (checkList.bMlPred[0] || checkList.bMlPred[1] || checkList.bMlPred[2])
			{
				QString type[3] = { "rf", "svm_soft", "svm_logit" };
				
				for (int i = 0; i < 3; i++)
				{
					if (m_pViewTab->m_plaqueCompositionProbMap.at(i).length() != 0)
					{
						if (checkList.bMlPred[i])
						{
							IppiSize roi_flim = { ML_N_CATS * flimAlines, frames };

							QFile fileComposition(enFacePath + QString("%1_compo_range[%2 %3].enface").arg(type[i]).arg(start).arg(end));
							if (false != fileComposition.open(QIODevice::WriteOnly))
							{
								np::FloatArray2 composition_map(ML_N_CATS * flimAlines, frames);
								memset(composition_map, 0, sizeof(float) * composition_map.length());
								m_pResultTab->getViewTab()->makeDelay(m_pViewTab->m_plaqueCompositionProbMap.at(i), composition_map, !isVibCorrted ? ML_N_CATS * m_pConfigTemp->flimDelaySync : 0);
								///if (m_pConfigTemp->interFrameSync >= 0)
								///	ippiCopy_32f_C1R(m_pViewTab->m_plaqueCompositionProbMap.raw_ptr(), sizeof(float) * roi_flim.width,
								///		&composition_map(0, m_pConfigTemp->interFrameSync), sizeof(float) * roi_flim.width, { roi_flim.width, roi_flim.height - m_pConfigTemp->interFrameSync });
								///else
								///	ippiCopy_32f_C1R(&m_pViewTab->m_plaqueCompositionProbMap(0, -m_pConfigTemp->interFrameSync), sizeof(float) * roi_flim.width,
								///		&composition_map(0, 0), sizeof(float) * roi_flim.width, { roi_flim.width, roi_flim.height + m_pConfigTemp->interFrameSync });

								if (m_pConfigTemp->rotatedAlines > 0)
								{
									for (int i = 0; i < roi_flim.height; i++)
									{
										float* pImg = composition_map.raw_ptr() + i * roi_flim.width;
										std::rotate(pImg, pImg + ML_N_CATS * m_pConfigTemp->rotatedAlines / 4, pImg + roi_flim.width);
									}
								}

								fileComposition.write(reinterpret_cast<char*>(&composition_map(0, start - 1)), sizeof(float) * composition_map.size(0) * (end - start + 1));
								fileComposition.close();
							}
						}
					}
				}

				///QFile fileOctMaxProj(enFacePath + QString("oct_max_projection_range[%1 %2].enface").arg(start).arg(end));
				///if (false != fileOctMaxProj.open(QIODevice::WriteOnly))
				///{
				///	IppiSize roi_proj = { width_non4, m_pVisTab->m_octProjection.size(1) };

				///	np::Uint8Array2 octProj(roi_proj.width, roi_proj.height);
				///	ippiCopy_8u_C1R(m_pVisTab->m_octProjection.raw_ptr(), sizeof(float) * m_pVisTab->m_octProjection.size(0),
				///		octProj.raw_ptr(), sizeof(float) * octProj.size(0), roi_proj);

				///	if (shift > 0)
				///	{
				///		for (int i = 0; i < roi_proj.height; i++)
				///		{
				///			uint8_t* pImg = octProj.raw_ptr() + i * roi_proj.width;
				///			std::rotate(pImg, pImg + shift, pImg + roi_proj.width);
				///		}
				///	}

				///	fileOctMaxProj.write(reinterpret_cast<char*>(&m_pVisTab->m_octProjection(0, start - 1)), sizeof(uint8_t) * m_pVisTab->m_octProjection.size(0) * (end - start + 1));
				///	fileOctMaxProj.close();
				///}
			}
		}

		// Scaled en face map writing ///////////////////////////////////////////////////////////////
		if (checkList.bScaled)
		{
			int frame4 = ROUND_UP_4S((int)m_pViewTab->m_vectorOctImage.size());
			int alines = m_pViewTab->m_lifetimeMap.at(0).size(0);

			ColorTable temp_ctable;
			IppiSize roi_flimproj = { flimAlines, frames };

			ImageObject* pImgObjIntensityMap = new ImageObject(frame4, alines, temp_ctable.m_colorTableVector.at(ColorTable::gray));
			memset(pImgObjIntensityMap->arr, 0, sizeof(uint8_t) * frame4 * alines);
			for (int i = 0; i < 3; i++)
			{
				if (checkList.bCh[i])
				{
					// Intensity-weight lifetime map
					ImageObject* pImgObjLifetimeMap = new ImageObject(frame4, alines, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE));
					memset(pImgObjLifetimeMap->arr, 0, sizeof(uint8_t) * frame4 * alines);

					m_pViewTab->scaleFLImEnFaceMap(pImgObjIntensityMap, pImgObjLifetimeMap, nullptr, nullptr, nullptr, VisualizationMode::_FLIM_PARAMETERS_, i, FLImParameters::_LIFETIME_, 0);

					pImgObjLifetimeMap->qrgbimg.copy(start - 1, 0, end - start + 1, roi_flimproj.width)
						.save(enFacePath + QString("flim_map_range[%1 %2]_ch%3_i[%4 %5]_t[%6 %7].bmp").arg(start).arg(end).arg(i + 1)
							.arg(m_pConfig->flimIntensityRange[i].min, 2, 'f', 1).arg(m_pConfig->flimIntensityRange[i].max, 2, 'f', 1)
							.arg(m_pConfig->flimLifetimeRange[i].min, 2, 'f', 1).arg(m_pConfig->flimLifetimeRange[i].max, 2, 'f', 1), "bmp");

					delete pImgObjLifetimeMap;
				}
			}

			if (checkList.bMlPred[0] || checkList.bMlPred[1] || checkList.bMlPred[2])
			{
				QString type[3] = { "rf", "svm_soft", "svm_logit" };

				for (int i = 0; i < 3; i++)
				{
					if (m_pViewTab->m_plaqueCompositionMap.at(i).length() != 0)
					{
						if (checkList.bMlPred[i])
						{
							// Intensity-weight composition map
							ImageObject* pImgObjCompositionMap = new ImageObject(frame4, alines, temp_ctable.m_colorTableVector.at(COMPOSITION_COLORTABLE));
							m_pViewTab->scaleFLImEnFaceMap(pImgObjIntensityMap, nullptr, nullptr, nullptr, pImgObjCompositionMap, VisualizationMode::_ML_PREDICTION_, 1, 0, i);

							pImgObjCompositionMap->qrgbimg.copy(start - 1, 0, end - start + 1, roi_flimproj.width)
								.save(enFacePath + QString("%1_compo_map_range[%2 %3].bmp").arg(type[i]).arg(start).arg(end), "bmp");

							delete pImgObjCompositionMap;
						}
					}
				}

				///IppiSize roi_proj = { m_pVisTab->m_octProjection.size(0), m_pVisTab->m_octProjection.size(1) };
				///ImageObject imgObjOctMaxProj(roi_proj.width, roi_proj.height, temp_ctable.m_colorTableVector.at(OCT_COLORTABLE));

				///np::FloatArray2 scale_temp(roi_proj.width, roi_proj.height);
				///ippsConvert_8u32f(m_pVisTab->m_octProjection.raw_ptr(), scale_temp.raw_ptr(), scale_temp.length());
				///ippiScale_32f8u_C1R(scale_temp, sizeof(float) * roi_proj.width, imgObjOctMaxProj.arr.raw_ptr(), sizeof(uint8_t) * roi_proj.width,
				///	roi_proj, m_pConfig->octGrayRange.min, m_pConfig->octGrayRange.max);
				///ippiMirror_8u_C1IR(imgObjOctMaxProj.arr.raw_ptr(), sizeof(uint8_t) * roi_proj.width, roi_proj, ippAxsHorizontal);
				///
				///imgObjOctMaxProj.qindeximg.copy(0, roi_proj.height - end, roi_proj.width, end - start + 1)
				///	.save(enFacePath + QString("oct_max_projection_range[%1 %2]_gray[%3 %4].bmp")
				///		.arg(start).arg(end).arg(m_pConfig->octGrayRange.min).arg(m_pConfig->octGrayRange.max), "bmp");
			}

			delete pImgObjIntensityMap;
		}

		/// Scaling MATLAB Script ////////////////////////////////////////////////////////////////////
		///if (false == QFile::copy("scale_indicator.m", enFacePath + "scale_indicator.m"))
		///	printf("Error occurred while copying matlab sciprt.\n");
	}
}

void ExportDlg::setRange()
{
	int start = m_pLineEdit_RangeStart->text().toInt();
	int end = m_pLineEdit_RangeEnd->text().toInt();

	if (start < 1)
		m_pLineEdit_RangeStart->setText(QString::number(1));
	if (end > m_pViewTab->m_vectorOctImage.size())
		m_pLineEdit_RangeEnd->setText(QString::number(m_pViewTab->m_vectorOctImage.size()));
}

void ExportDlg::checkCircImage(bool toggled)
{
	if (m_pCheckBox_ResizeCircImage->isChecked()) m_pCheckBox_ResizeCircImage->setChecked(false);
	m_pCheckBox_ResizeCircImage->setEnabled(toggled);

	bool widget_disabled = !m_pCheckBox_CircImage->isChecked() && !m_pCheckBox_LongiImage->isChecked();

	m_pCheckBox_CrossSectionCh1->setDisabled(widget_disabled);
	m_pCheckBox_CrossSectionCh2->setDisabled(widget_disabled);
	m_pCheckBox_CrossSectionCh3->setDisabled(widget_disabled);

	bool export_disabled = widget_disabled && (!m_pCheckBox_RawData->isChecked() && !m_pCheckBox_ScaledImage->isChecked());

	m_pPushButton_Export->setDisabled(export_disabled);
	m_pLabel_Range->setDisabled(export_disabled);
	m_pLineEdit_RangeStart->setDisabled(export_disabled);
	m_pLineEdit_RangeEnd->setDisabled(export_disabled);
}

void ExportDlg::checkLongiImage(bool toggled)
{
	if (m_pCheckBox_ResizeLongiImage->isChecked()) m_pCheckBox_ResizeLongiImage->setChecked(false);
	m_pCheckBox_ResizeLongiImage->setEnabled(toggled);

	bool widget_disabled = !m_pCheckBox_CircImage->isChecked() && !m_pCheckBox_LongiImage->isChecked();
	
	m_pCheckBox_CrossSectionCh1->setDisabled(widget_disabled);
	m_pCheckBox_CrossSectionCh2->setDisabled(widget_disabled);
	m_pCheckBox_CrossSectionCh3->setDisabled(widget_disabled);

	bool export_disabled = widget_disabled && (!m_pCheckBox_RawData->isChecked() && !m_pCheckBox_ScaledImage->isChecked());

	m_pPushButton_Export->setDisabled(export_disabled);
	m_pLabel_Range->setDisabled(export_disabled);
	m_pLineEdit_RangeStart->setDisabled(export_disabled);
	m_pLineEdit_RangeEnd->setDisabled(export_disabled);
}

void ExportDlg::enableCircResize(bool checked)
{
	m_pLineEdit_CircDiameter->setEnabled(checked);

	if (!checked)
		m_pLineEdit_CircDiameter->setText(QString::number(2 * m_pViewTab->m_vectorOctImage.at(0).size(0)));
}

void ExportDlg::enableLongiResize(bool checked)
{
	m_pLineEdit_LongiWidth->setEnabled(checked);
	m_pLineEdit_LongiHeight->setEnabled(checked);

	if (!checked)
	{
		m_pLineEdit_LongiWidth->setText(QString::number(m_pViewTab->m_vectorOctImage.size()));
		m_pLineEdit_LongiHeight->setText(QString::number(2 * m_pViewTab->m_vectorOctImage.at(0).size(0)));
	}
}

void ExportDlg::checkResizeValue()
{
	int circ_diameter = m_pLineEdit_CircDiameter->text().toInt();
	int longi_width = m_pLineEdit_LongiWidth->text().toInt();
	int longi_height = m_pLineEdit_LongiHeight->text().toInt();

	if (circ_diameter < 1) m_pLineEdit_CircDiameter->setText(QString::number(1));
	if (longi_width < 1) m_pLineEdit_LongiWidth->setText(QString::number(1));
	if (longi_height < 1) m_pLineEdit_LongiHeight->setText(QString::number(1));
}

void ExportDlg::checkEnFaceOptions()
{
	bool widget_disabled = !m_pCheckBox_RawData->isChecked() && !m_pCheckBox_ScaledImage->isChecked();
				
	m_pCheckBox_EnFaceCh1->setDisabled(widget_disabled);
	m_pCheckBox_EnFaceCh2->setDisabled(widget_disabled);
	m_pCheckBox_EnFaceCh3->setDisabled(widget_disabled);
	///m_pCheckBox_OctMaxProjection->setDisabled(widget_disabled);
	
	bool export_disabled = widget_disabled && (!m_pCheckBox_CircImage->isChecked() && !m_pCheckBox_LongiImage->isChecked());

	m_pPushButton_Export->setDisabled(export_disabled);
	m_pLabel_Range->setDisabled(export_disabled);
	m_pLineEdit_RangeStart->setDisabled(export_disabled);
	m_pLineEdit_RangeEnd->setDisabled(export_disabled);
}

void ExportDlg::setWidgetsEnabled(bool enabled, int num_proc)
{
	// Set State
	m_bIsSaving = !enabled;
	m_pPushButton_Export->setEnabled(enabled);
	if (!enabled)
	{
		m_pProgressBar_Export->setFormat("Exporting... %p%");
		m_pProgressBar_Export->setRange(0, num_proc);
	}
	else
		m_pProgressBar_Export->setFormat("");
	m_pProgressBar_Export->setValue(0);

	m_pLabel_ExportPath->setEnabled(enabled);
	m_pLineEdit_ExportPath->setEnabled(enabled);
	m_pLineEdit_ExportPath->setCursorPosition(0);
	m_pPushButton_ExportPath->setEnabled(enabled);

	// Set Range
	m_pLabel_Range->setEnabled(enabled);
	m_pLineEdit_RangeStart->setEnabled(enabled);
	m_pLineEdit_RangeEnd->setEnabled(enabled);

	// Save Cross-sections
	m_pGroupBox_CrossSections->setEnabled(enabled);

	m_pCheckBox_CircImage->setEnabled(enabled);
	m_pCheckBox_LongiImage->setEnabled(enabled);

	///if ((!m_pCheckBox_CircImage->isChecked() && !m_pCheckBox_LongiImage->isChecked()) || !enabled)
	{
		m_pCheckBox_ResizeCircImage->setEnabled(enabled);
		m_pCheckBox_ResizeLongiImage->setEnabled(enabled);

		m_pLineEdit_CircDiameter->setEnabled(enabled);
		m_pLineEdit_LongiWidth->setEnabled(enabled);
		m_pLineEdit_LongiHeight->setEnabled(enabled);

		if (enabled)
		{
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
	}
	
	// Save En Face Maps
	m_pGroupBox_EnFaceChemogram->setEnabled(enabled);

	m_pCheckBox_RawData->setEnabled(enabled);
	m_pCheckBox_ScaledImage->setEnabled(enabled);

	///if ((!m_pCheckBox_RawData->isChecked() && !m_pCheckBox_ScaledImage->isChecked()) || !enabled)
	{
		m_pCheckBox_EnFaceCh1->setEnabled(enabled);
		m_pCheckBox_EnFaceCh2->setEnabled(enabled);
		m_pCheckBox_EnFaceCh3->setEnabled(enabled);

		///m_pCheckBox_OctMaxProjection->setEnabled(enabled);
	}
}


#ifndef NEXT_GEN_SYSTEM
void ExportDlg::scaling(std::vector<np::Uint8Array2>& vectorOctImage, std::vector<ImageObject*>& vectorLifetimeMap, CrossSectionCheckList checkList)
#else
void ExportDlg::scaling(std::vector<np::FloatArray2>& vectorOctImage, std::vector<ImageObject*>& vectorLifetimeMap, CrossSectionCheckList checkList)
#endif
{
	// Range parameters
	int nTotalFrame = (int)m_pViewTab->m_vectorOctImage.size();
	int start = m_pLineEdit_RangeStart->text().toInt();
	int end = m_pLineEdit_RangeEnd->text().toInt();

	// Colortable & size parameter
	ColorTable temp_ctable;
	IppiSize roi_oct = { vectorOctImage.at(0).size(0), vectorOctImage.at(0).size(1) };
	int ring_thickness = (RING_THICKNESS * m_pConfigTemp->octRadius) / 1024;
	
	int frameCount = 0;
	while (frameCount < nTotalFrame)
	{
		ImgObjVector* pImgObjVec = nullptr;

		// Range test
		if (((frameCount + 1) >= start) && ((frameCount + 1) <= end))
		{
			// Create vecotr for image objects (multithreading exporting)
			pImgObjVec = new ImgObjVector;

			// Image objects for OCT Images
			pImgObjVec->push_back(new ImageObject(roi_oct.width, roi_oct.height, temp_ctable.m_colorTableVector.at(OCT_COLORTABLE)));
			// Image objects for Ch1 FLIM
			pImgObjVec->push_back(new ImageObject(ring_thickness, roi_oct.height / 4, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE)));
			// Image objects for Ch2 FLIM		
			pImgObjVec->push_back(new ImageObject(ring_thickness, roi_oct.height / 4, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE)));
			// Image objects for Ch3 FLIM
			pImgObjVec->push_back(new ImageObject(ring_thickness, roi_oct.height / 4, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE)));

			// OCT Visualization
			np::FloatArray2 scale_temp(roi_oct.width, roi_oct.height);
#ifndef NEXT_GEN_SYSTEM
			ippsConvert_8u32f(vectorOctImage.at(frameCount).raw_ptr(), scale_temp.raw_ptr(), scale_temp.length());

			if (m_pConfigTemp->reflectionRemoval)
			{
				np::FloatArray2 reflection_temp(roi_oct.width, roi_oct.height);
				ippiCopy_32f_C1R(&scale_temp(m_pConfigTemp->reflectionDistance, 0), sizeof(float) * scale_temp.size(0),
					&reflection_temp(0, 0), sizeof(float) * reflection_temp.size(0),
					{ roi_oct.width - m_pConfigTemp->reflectionDistance, roi_oct.height });
				ippsMulC_32f_I(m_pConfigTemp->reflectionLevel, reflection_temp, reflection_temp.length());
				ippsSub_32f_I(reflection_temp, scale_temp, scale_temp.length());
				ippiScale_32f8u_C1R(scale_temp.raw_ptr(), roi_oct.width * sizeof(float),
					pImgObjVec->at(0)->arr.raw_ptr(), roi_oct.width * sizeof(uint8_t), roi_oct,
					(float)m_pConfigTemp->octGrayRange.min, (float)m_pConfigTemp->octGrayRange.max * 0.9f);
			}
			else
				ippiScale_32f8u_C1R(scale_temp.raw_ptr(), roi_oct.width * sizeof(float),
					pImgObjVec->at(0)->arr.raw_ptr(), roi_oct.width * sizeof(uint8_t), roi_oct,
					(float)m_pConfigTemp->octGrayRange.min, (float)m_pConfigTemp->octGrayRange.max);
#else
			ippiScale_32f8u_C1R(vectorOctImage.at(frameCount).raw_ptr(), roi_oct.width * sizeof(float),
				pImgObjVec->at(0)->arr.raw_ptr(), roi_oct.width * sizeof(uint8_t), roi_oct, (float)m_pConfig->axsunDbRange.min, (float)m_pConfig->axsunDbRange.max);
#endif
			m_pViewTab->circShift(pImgObjVec->at(0)->arr, (m_pConfigTemp->rotatedAlines) % m_pConfigTemp->octAlines);  ///  + m_pConfigTemp->intraFrameSync
			(*m_pViewTab->getMedfiltRect())(pImgObjVec->at(0)->arr.raw_ptr());

			// FLIM Visualization		
			IppiSize roi_flimring = { 1, roi_oct.height / 4 };
			for (int i = 0; i < 3; i++)
			{
				if (checkList.bCh[i])
				{
					tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)ring_thickness),
						[&, ring_thickness](const tbb::blocked_range<size_t>& r) {
						for (size_t j = r.begin(); j != r.end(); ++j)
						{
							ippiCopy_8u_C3R(vectorLifetimeMap.at(i)->qrgbimg.constBits() + 3 * frameCount, 3 * vectorLifetimeMap.at(i)->arr.size(0),
								pImgObjVec->at(i + 1)->qrgbimg.bits() + 3 * j, 3 * ring_thickness, roi_flimring);
						}
					});
				}
			}
		}

		frameCount++;

		// Push the buffers to sync Queues
		m_syncQueueConverting.push(pImgObjVec);
	}
}

void ExportDlg::converting(CrossSectionCheckList checkList)
{
	// Range parameters
	int nTotalFrame = (int)m_pViewTab->m_vectorOctImage.size();
	
	int frameCount = 0;
	while (frameCount < nTotalFrame)
	{
		// Get the buffer from the previous sync Queue
		ImgObjVector *pImgObjVec = m_syncQueueConverting.pop();

		if (pImgObjVec)
		{
			// Converting RGB
			if (checkList.bCh[0] || checkList.bCh[1] || checkList.bCh[2])
				pImgObjVec->at(0)->convertRgb();

			tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)3),
				[&](const tbb::blocked_range<size_t>& r) {
				for (size_t i = r.begin(); i != r.end(); ++i)
				{
					if (checkList.bCh[i])
					{
						pImgObjVec->at(i + 1)->scaledRgb4h();
					}
				}
			});
		}

		frameCount++;

		// Push the buffers to sync Queues
		m_syncQueueCircularizing.push(pImgObjVec);
	}
}

void ExportDlg::circularizing(CrossSectionCheckList checkList) // with longitudinal making
{
	// Range parameters
	int nTotalFrame = (int)m_pViewTab->m_vectorOctImage.size();
	int nTotalFrame4 = ROUND_UP_4S(nTotalFrame);
	int octScans = m_pViewTab->m_vectorOctImage.at(0).size(0);
	int octAlines = m_pViewTab->m_vectorOctImage.at(0).size(1);
	int n2Alines = int(octAlines / 2);

	// Colortable
	ColorTable temp_ctable;

	// Image objects for longitudinal image
	ImgObjVector *pImgObjVecLongi[3];
	if (checkList.bLongi)
	{
		for (int i = 0; i < 3; i++)
		{
			pImgObjVecLongi[i] = new ImgObjVector;
			for (int j = 0; j < n2Alines; j++)
			{
				ImageObject *pLongiImgObj = new ImageObject(nTotalFrame4, 2 * octScans, temp_ctable.m_colorTableVector.at(OCT_COLORTABLE));
				pImgObjVecLongi[i]->push_back(pLongiImgObj);
			}
		}
	}

	int frameCount = 0;
	while (frameCount < nTotalFrame)
	{
		// Get the buffer from the previous sync Queue
		ImgObjVector *pImgObjVec = m_syncQueueCircularizing.pop();
		ImgObjVector *pImgObjVecCirc = nullptr;

		if (pImgObjVec)
		{
			pImgObjVecCirc = new ImgObjVector;

			if (!checkList.bCh[0] && !checkList.bCh[1] && !checkList.bCh[2]) // Grayscale OCT
			{
				// ImageObject for circ writing
				ImageObject *pCircImgObj = new ImageObject(2 * octScans, 2 * octScans, temp_ctable.m_colorTableVector.at(OCT_COLORTABLE));

				// Buffer
				np::Uint8Array2 rect_temp(pImgObjVec->at(0)->qindeximg.bits(), octScans, octAlines);

				// Circularize
				if (checkList.bCirc)
					(*m_pViewTab->getCirc())(rect_temp, pCircImgObj->qindeximg.bits(), false, false);

				// Longitudinal
				if (checkList.bLongi)
				{
					tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)n2Alines),
						[&](const tbb::blocked_range<size_t>& r) {
						for (size_t i = r.begin(); i != r.end(); ++i)
						{
							ippiCopy_8u_C1R(&rect_temp(0, (int)i), 1, pImgObjVecLongi[0]->at((int)i)->qindeximg.bits() + frameCount, nTotalFrame4, { 1, octScans });
							ippiCopy_8u_C1R(&rect_temp(0, (int)i + n2Alines), 1, pImgObjVecLongi[0]->at((int)i)->qindeximg.bits() + nTotalFrame4 * octScans + frameCount, nTotalFrame4, { 1, octScans });
						}
					});
				}

				// Vector pushing back
				pImgObjVecCirc->push_back(pCircImgObj);
			}
			else // OCT-FLIm
			{
				// ImageObject for circ writing
				///if (!checkList.bMulti)

				int ring_thickness = (RING_THICKNESS * m_pConfigTemp->octRadius) / 1024;
				{
					for (int i = 0; i < 3; i++)
					{
						// ImageObject for circ writing
						ImageObject* pCircImgObj = new ImageObject(2 * octScans, 2 * octScans, temp_ctable.m_colorTableVector.at(OCT_COLORTABLE));

						// Buffer
						np::Uint8Array2 rect_temp(pImgObjVec->at(0)->qrgbimg.bits(), 3 * octScans, octAlines);

						if (checkList.bCh[i] && (checkList.bCirc || checkList.bLongi))
						{
							// Paste FLIM color ring to RGB rect image
							ippiCopy_8u_C3R(pImgObjVec->at(i + 1)->qrgbimg.bits(), 3 * ring_thickness,
								pImgObjVec->at(0)->qrgbimg.bits() + 3 * (octScans - ring_thickness), 3 * octScans, { ring_thickness, octAlines });

							// Circularize						
							(*m_pViewTab->getCirc())(rect_temp, pCircImgObj->qrgbimg.bits(), false, true);
						}

						// Longitudinal
						if (checkList.bLongi)
						{
							tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)n2Alines),
								[&](const tbb::blocked_range<size_t>& r) {
								for (size_t j = r.begin(); j != r.end(); ++j)
								{
									ippiCopy_8u_C3R(&rect_temp(0, (int)j), 3, pImgObjVecLongi[i]->at((int)j)->qrgbimg.bits() + 3 * frameCount, 3 * nTotalFrame4, { 1, octScans });
									ippiCopy_8u_C3R(&rect_temp(0, (int)j + n2Alines), 3, pImgObjVecLongi[i]->at((int)j)->qrgbimg.bits() + 3 * (nTotalFrame4 * octScans + frameCount), 3 * nTotalFrame4, { 1, octScans });
								}
							});
						}

						// Vector pushing back
						pImgObjVecCirc->push_back(pCircImgObj);
					}
				}
				///else
				{
					///int nCh = (checkList.bCh[0] ? 1 : 0) + (checkList.bCh[1] ? 1 : 0) + (checkList.bCh[2] ? 1 : 0);
					///int n = 0;

					/// // ImageObject for circ writing
					///pCircImgObj[0] = new ImageObject(2 * m_pVisTab->m_vectorOctImage.at(frameCount).size(0), 2 * m_pVisTab->m_vectorOctImage.at(frameCount).size(0),
					///	temp_ctable.m_colorTableVector.at(OCT_COLORTABLE));

					/// Buffer
					///np::Uint8Array2 rect_temp(pImgObjVec->at(0)->qrgbimg.bits(), 3 * pImgObjVec->at(0)->arr.size(0), pImgObjVec->at(0)->arr.size(1));

					/// Paste FLIM color ring to RGB rect image
					///for (int i = 0; i < 3; i++)
					///	if (checkList.bCh[i] && (checkList.bCirc || checkList.bLongi))
					///		memcpy(pImgObjVec->at(0)->qrgbimg.bits() + 3 * pImgObjVec->at(0)->arr.size(0) * (pImgObjVec->at(0)->arr.size(1) - (nCh - n++) * RING_THICKNESS),
					///			pImgObjVec->at(1 + 2 * i)->qrgbimg.bits(), pImgObjVec->at(1 + 2 * i)->qrgbimg.byteCount());				

					/// Circularize
					///(*m_pVisTab->m_pCirc)(rect_temp, pCircImgObj[0]->qrgbimg.bits(), "vertical", "rgb");

					/// Longitudinal
					///if (checkList.bLongi)
					///{
					///	IppiSize roi_longi = { 3, m_pVisTab->m_vectorOctImage.at(0).size(0) };
					///	int n2Alines = m_pVisTab->m_vectorOctImage.at(0).size(1) / 2;

					///	tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)n2Alines),
					///		[&](const tbb::blocked_range<size_t>& r) {
					///		for (size_t j = r.begin(); j != r.end(); ++j)
					///		{
					///			ippiCopy_8u_C1R(&rect_temp(3 * (int)j, 0), sizeof(uint8_t) * 2 * 3 * n2Alines,
					///				pImgObjVecLongi[0]->at((int)j)->qrgbimg.bits() + 3 * frameCount, sizeof(uint8_t) * 3 * nTotalFrame4, roi_longi);
					///			ippiCopy_8u_C1R(&rect_temp(3 * ((int)j + n2Alines), 0), sizeof(uint8_t) * 2 * 3 * n2Alines,
					///				pImgObjVecLongi[0]->at((int)j)->qrgbimg.bits() + m_pVisTab->m_vectorOctImage.at(0).size(0) * 3 * nTotalFrame4 + 3 * frameCount, sizeof(uint8_t) * 3 * nTotalFrame4, roi_longi);
					///		}
					///	});
					///}

					/// Vector pushing back
					///pImgObjVecCirc->push_back(pCircImgObj[0]);
				}
			}
			
			// Delete ImageObjects
			for (int i = 0; i < 4; i++)
				delete pImgObjVec->at(i);
			delete pImgObjVec;
		}

		frameCount++;

		// Push the buffers to sync Queues
		m_syncQueueCircWriting.push(pImgObjVecCirc);
	}

	// Write longtiduinal images
	if (checkList.bLongi)
	{
		int start = m_pLineEdit_RangeStart->text().toInt();
		int end = m_pLineEdit_RangeEnd->text().toInt();

		if (!checkList.bCh[0] && !checkList.bCh[1] && !checkList.bCh[2])
		{
#ifndef NEXT_GEN_SYSTEM
			QString longiPath = m_exportPath + QString("/longi_image[%1 %2]_gray[%3 %4]/").arg(start).arg(end).arg(m_pConfigTemp->octGrayRange.min).arg(m_pConfigTemp->octGrayRange.max);
#else
			QString longiPath = m_exportPath + QString("/longi_image[%1 %2]_dB[%3 %4]/").arg(start).arg(end).arg(m_pConfig->axsunDbRange.min).arg(m_pConfig->axsunDbRange.max);
#endif
			QDir().mkdir(longiPath);

			int alineCount = 0;
			while (alineCount < m_pViewTab->m_vectorOctImage.at(0).size(1) / 2)
			{
				// Write longi images
				ippiMirror_8u_C1IR(pImgObjVecLongi[0]->at(alineCount)->qindeximg.bits(), sizeof(uint8_t) * nTotalFrame4, 
								   { nTotalFrame4, m_pViewTab->m_vectorOctImage.at(0).size(0) }, ippAxsHorizontal);
				if (!checkList.bLongiResize)
					pImgObjVecLongi[0]->at(alineCount)->qindeximg.copy(start - 1, 0, end - start + 1, 2 * m_pViewTab->m_vectorOctImage.at(0).size(0)).
					save(longiPath + QString("longi_%1_%2.bmp").arg("pullback").arg(alineCount + 1, 4, 10, (QChar)'0'), "bmp");
				else
					pImgObjVecLongi[0]->at(alineCount)->qindeximg.copy(start - 1, 0, end - start + 1, 2 * m_pViewTab->m_vectorOctImage.at(0).size(0)).
					scaled(checkList.nLongiWidth, checkList.nLongiHeight, Qt::IgnoreAspectRatio, m_defaultTransformation).
					save(longiPath + QString("longi_%1_%2.bmp").arg("pullback").arg(alineCount + 1, 4, 10, (QChar)'0'), "bmp");

				emit savedSingleFrame(m_nSavedFrames++);

				// Delete ImageObjects
				delete pImgObjVecLongi[0]->at(alineCount);
				delete pImgObjVecLongi[1]->at(alineCount);
				delete pImgObjVecLongi[2]->at(alineCount++);
			}
		}
		else
		{
			QString longiPath[3];
			//if (!checkList.bMulti)
			{
				for (int i = 0; i < 3; i++)
				{
					if (checkList.bCh[i])
					{		
#ifndef NEXT_GEN_SYSTEM
						longiPath[i] = m_exportPath + QString("/longi_image[%1 %2]_gray[%3 %4]_ch%5_i[%6 %7]_t[%8 %9]/")
							.arg(start).arg(end).arg(m_pConfigTemp->octGrayRange.min).arg(m_pConfigTemp->octGrayRange.max)
							.arg(i + 1).arg(m_pConfig->flimIntensityRange[i].min, 2, 'f', 1).arg(m_pConfig->flimIntensityRange[i].max, 2, 'f', 1)
							.arg(m_pConfig->flimLifetimeRange[i].min, 2, 'f', 1).arg(m_pConfig->flimLifetimeRange[i].max, 2, 'f', 1);
#else
						longiPath[i] = m_exportPath + QString("/longi_image[%1 %2]_dB[%3 %4]_ch%5_i[%6 %7]_t[%8 %9]/")
							.arg(start).arg(end).arg(m_pConfig->axsunDbRange.min).arg(m_pConfig->axsunDbRange.max)
							.arg(i + 1).arg(m_pConfig->flimIntensityRange[i].min, 2, 'f', 1).arg(m_pConfig->flimIntensityRange[i].max, 2, 'f', 1)
							.arg(m_pConfig->flimLifetimeRange[i].min, 2, 'f', 1).arg(m_pConfig->flimLifetimeRange[i].max, 2, 'f', 1);
#endif

						if (checkList.bLongi) QDir().mkdir(longiPath[i]);
					}
				}
			}
			//else
			{
				//if (!m_pVisTab->getIntensityRatioMode())
				//	longiPath[0] = m_pProcessingTab->m_path + QString("/longi_merged[%1 %2]_gray[%3 %4]_ch%5%6%7_i[%8 %9][%10 %11][%12 %13]_t[%14 %15][%16 %17][%18 19]/")
				//		.arg(start).arg(end).arg(m_pConfig->octGrayRange.min).arg(m_pConfig->octGrayRange.max)
				//		.arg(checkList.bCh[0] ? "1" : "").arg(checkList.bCh[1] ? "2" : "").arg(checkList.bCh[2] ? "3" : "")
				//		.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimIntensityRange[0].min, 'f', 1) : "")
				//		.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimIntensityRange[0].max, 'f', 1) : "")
				//		.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimIntensityRange[1].min, 'f', 1) : "")
				//		.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimIntensityRange[1].max, 'f', 1) : "")
				//		.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimIntensityRange[2].min, 'f', 1) : "")
				//		.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimIntensityRange[2].max, 'f', 1) : "")
				//		.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimLifetimeRange[0].min, 'f', 1) : "")
				//		.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimLifetimeRange[0].max, 'f', 1) : "")
				//		.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimLifetimeRange[1].min, 'f', 1) : "")
				//		.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimLifetimeRange[1].max, 'f', 1) : "")
				//		.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimLifetimeRange[2].min, 'f', 1) : "")
				//		.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimLifetimeRange[2].max, 'f', 1) : "");
				//else
				//	longiPath[0] = m_pProcessingTab->m_path + QString("/longi_merged[%1 %2]_gray[%3 %4]_ch%5%6/%7%8/%9%10_ir[%11 %12][%13 %14][%15 %16]_t[%17 %18][%19 %20][%21 22]/")
				//	.arg(start).arg(end).arg(m_pConfig->octGrayRange.min).arg(m_pConfig->octGrayRange.max)
				//	.arg(checkList.bCh[0] ? "1" : "").arg(checkList.bCh[0] ? QString::number(ratio_index[0][m_pConfig->flimIntensityRatioRefIdx[0]]) : "")
				//	.arg(checkList.bCh[1] ? "2" : "").arg(checkList.bCh[1] ? QString::number(ratio_index[1][m_pConfig->flimIntensityRatioRefIdx[1]]) : "")
				//	.arg(checkList.bCh[2] ? "3" : "").arg(checkList.bCh[2] ? QString::number(ratio_index[2][m_pConfig->flimIntensityRatioRefIdx[2]]) : "")
				//	.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimIntensityRatioRange[0][m_pConfig->flimIntensityRatioRefIdx[0]].min, 'f', 1) : "")
				//	.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimIntensityRatioRange[0][m_pConfig->flimIntensityRatioRefIdx[0]].max, 'f', 1) : "")
				//	.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimIntensityRatioRange[1][m_pConfig->flimIntensityRatioRefIdx[1]].min, 'f', 1) : "")
				//	.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimIntensityRatioRange[1][m_pConfig->flimIntensityRatioRefIdx[1]].max, 'f', 1) : "")
				//	.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimIntensityRatioRange[2][m_pConfig->flimIntensityRatioRefIdx[2]].min, 'f', 1) : "")
				//	.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimIntensityRatioRange[2][m_pConfig->flimIntensityRatioRefIdx[2]].max, 'f', 1) : "")
				//	.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimLifetimeRange[0].min, 'f', 1) : "")
				//	.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimLifetimeRange[0].max, 'f', 1) : "")
				//	.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimLifetimeRange[1].min, 'f', 1) : "")
				//	.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimLifetimeRange[1].max, 'f', 1) : "")
				//	.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimLifetimeRange[2].min, 'f', 1) : "")
				//	.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimLifetimeRange[2].max, 'f', 1) : "");
				//if (checkList.bLongi) QDir().mkdir(longiPath[0]);
			}

			int alineCount = 0;
			while (alineCount < n2Alines)
			{
				if (checkList.bLongi)
				{
					//if (!checkList.bMulti)
					{
						for (int i = 0; i < 3; i++)
						{
							if (checkList.bCh[i])
							{
								// Write longi images
								ippiMirror_8u_C1IR(pImgObjVecLongi[i]->at(alineCount)->qrgbimg.bits(), sizeof(uint8_t) * 3 * nTotalFrame4, { 3 * nTotalFrame4, octScans }, ippAxsHorizontal);
								if (!checkList.bLongiResize)
									pImgObjVecLongi[i]->at(alineCount)->qrgbimg.copy(start - 1, 0, end - start + 1, 2 * octScans).
									save(longiPath[i] + QString("longi_%1_%2.bmp").arg("pullback").arg(alineCount + 1, 4, 10, (QChar)'0'), "bmp");
								else
									pImgObjVecLongi[i]->at(alineCount)->qrgbimg.copy(start - 1, 0, end - start + 1, 2 * octScans).
									scaled(checkList.nLongiWidth, checkList.nLongiHeight, Qt::IgnoreAspectRatio, m_defaultTransformation).
									save(longiPath[i] + QString("longi_%1_%2.bmp").arg("pullback").arg(alineCount + 1, 4, 10, (QChar)'0'), "bmp");
							}
						}
					}
					//else
					//{
					//	// Write longi images
					//	ippiMirror_8u_C1IR(pImgObjVecLongi[0]->at(alineCount)->qrgbimg.bits(), sizeof(uint8_t) * 3 * nTotalFrame4, { 3 * nTotalFrame4, m_pVisTab->m_vectorOctImage.at(0).size(0) }, ippAxsHorizontal);
					//	if (!checkList.bLongiResize)
					//		pImgObjVecLongi[0]->at(alineCount)->qrgbimg.copy(start - 1, 0, end - start + 1, 2 * m_pVisTab->m_vectorOctImage.at(0).size(0)).
					//		save(longiPath[0] + QString("longi_%1_%2.bmp").arg(folderName).arg(alineCount + 1, 4, 10, (QChar)'0'), "bmp");
					//	else
					//		pImgObjVecLongi[0]->at(alineCount)->qrgbimg.copy(start - 1, 0, end - start + 1, 2 * m_pVisTab->m_vectorOctImage.at(0).size(0)).
					//		scaled(checkList.nLongiWidth, checkList.nLongiHeight, Qt::IgnoreAspectRatio, m_defaultTransformation).
					//		save(longiPath[0] + QString("longi_%1_%2.bmp").arg(folderName).arg(alineCount + 1, 4, 10, (QChar)'0'), "bmp");
					//}

					emit savedSingleFrame(m_nSavedFrames++);
				}

				// Delete ImageObjects
				delete pImgObjVecLongi[0]->at(alineCount);
				delete pImgObjVecLongi[1]->at(alineCount);
				delete pImgObjVecLongi[2]->at(alineCount++);
			}
		}	

		for (int i = 0; i < 3; i++)
			delete pImgObjVecLongi[i];
	}
}

void ExportDlg::circWriting(CrossSectionCheckList checkList)
{
	// Range parameters
	int nTotalFrame = (int)m_pViewTab->m_vectorOctImage.size();

	if (!checkList.bCh[0] && !checkList.bCh[1] && !checkList.bCh[2]) // Grayscale OCT
	{
#ifndef NEXT_GEN_SYSTEM
		QString circPath = m_exportPath + QString("/circ_image_gray[%1 %2]/").arg(m_pConfigTemp->octGrayRange.min).arg(m_pConfigTemp->octGrayRange.max);
#else
		QString circPath = m_exportPath + QString("/circ_image_gray[%1 %2]/").arg(m_pConfig->axsunDbRange.min).arg(m_pConfig->axsunDbRange.max);
#endif
		if (checkList.bCirc) QDir().mkdir(circPath);

		int frameCount = 0;
		while (frameCount < nTotalFrame)
		{
			// Get the buffer from the previous sync Queue
			ImgObjVector *pImgObjVecCirc = m_syncQueueCircWriting.pop();

			if (pImgObjVecCirc)
			{
				// Write circ images
				if (checkList.bCirc)
				{
					if (!checkList.bCircResize)
						pImgObjVecCirc->at(0)->qindeximg.save(circPath + QString("circ_%1_%2.bmp").arg("pullback").arg(frameCount + 1, 3, 10, (QChar)'0'), "bmp");
					else
						pImgObjVecCirc->at(0)->qindeximg.scaled(checkList.nCircDiameter, checkList.nCircDiameter, Qt::IgnoreAspectRatio, m_defaultTransformation).
						save(circPath + QString("circ_%1_%2.bmp").arg("pullback").arg(frameCount + 1, 3, 10, (QChar)'0'), "bmp");
				}
				
				// Delete ImageObjects
				delete pImgObjVecCirc->at(0);
				delete pImgObjVecCirc;

			}

			emit savedSingleFrame(m_nSavedFrames++);
			frameCount++;
		}
	}
	else // OCT-FLIM
	{
		QString circPath[3];
		///if (!checkList.bMulti)
		{
			for (int i = 0; i < 3; i++)
			{
				if (checkList.bCh[i])
				{					
#ifndef NEXT_GEN_SYSTEM
					circPath[i] = m_exportPath + QString("/circ_image_gray[%1 %2]_ch%3_i[%4 %5]_t[%6 %7]/")
					.arg(m_pConfigTemp->octGrayRange.min).arg(m_pConfigTemp->octGrayRange.max)
					.arg(i + 1).arg(m_pConfig->flimIntensityRange[i].min, 2, 'f', 1).arg(m_pConfig->flimIntensityRange[i].max, 2, 'f', 1)
					.arg(m_pConfig->flimLifetimeRange[i].min, 2, 'f', 1).arg(m_pConfig->flimLifetimeRange[i].max, 2, 'f', 1);			
#else
					circPath[i] = m_exportPath + QString("/circ_image_dB[%1 %2]_ch%3_i[%4 %5]_t[%6 %7]/")
						.arg(m_pConfig->axsunDbRange.min).arg(m_pConfig->axsunDbRange.max)
						.arg(i + 1).arg(m_pConfig->flimIntensityRange[i].min, 2, 'f', 1).arg(m_pConfig->flimIntensityRange[i].max, 2, 'f', 1)
						.arg(m_pConfig->flimLifetimeRange[i].min, 2, 'f', 1).arg(m_pConfig->flimLifetimeRange[i].max, 2, 'f', 1);
#endif
					
					if (checkList.bCirc) QDir().mkdir(circPath[i]);
				}
			}
		}
		///else
		///{
		///	if (!m_pVisTab->getIntensityRatioMode())
		///		circPath[0] = m_pProcessingTab->m_path + QString("/circ_merged_gray[%1 %2]_ch%3%4%5_i[%6 %7][%8 %9][%10 %11]_t[%12 %13][%14 %15][%16 %17]/")
		///		.arg(m_pConfig->octGrayRange.min).arg(m_pConfig->octGrayRange.max)
		///		.arg(checkList.bCh[0] ? "1" : "").arg(checkList.bCh[1] ? "2" : "").arg(checkList.bCh[2] ? "3" : "")
		///		.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimIntensityRange[0].min, 'f', 1) : "")
		///		.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimIntensityRange[0].max, 'f', 1) : "")
		///		.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimIntensityRange[1].min, 'f', 1) : "")
		///		.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimIntensityRange[1].max, 'f', 1) : "")
		///		.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimIntensityRange[2].min, 'f', 1) : "")
		///		.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimIntensityRange[2].max, 'f', 1) : "")
		///		.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimLifetimeRange[0].min, 'f', 1) : "")
		///		.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimLifetimeRange[0].max, 'f', 1) : "")
		///		.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimLifetimeRange[1].min, 'f', 1) : "")
		///		.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimLifetimeRange[1].max, 'f', 1) : "")
		///		.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimLifetimeRange[2].min, 'f', 1) : "")
		///		.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimLifetimeRange[2].max, 'f', 1) : "");
		///	else
		///		circPath[0] = m_pProcessingTab->m_path + QString("/circ_merged_gray[%1 %2]_ch%3%4/%5%6/%7%8_ir[%9 %10][%11 %12][%13 %14]_t[%15 %16][%17 %18][%19 %20]/")
		///		.arg(m_pConfig->octGrayRange.min).arg(m_pConfig->octGrayRange.max)
		///		.arg(checkList.bCh[0] ? "1" : "").arg(checkList.bCh[0] ? QString::number(ratio_index[0][m_pConfig->flimIntensityRatioRefIdx[0]]) : "")
		///		.arg(checkList.bCh[1] ? "2" : "").arg(checkList.bCh[1] ? QString::number(ratio_index[1][m_pConfig->flimIntensityRatioRefIdx[1]]) : "")
		///		.arg(checkList.bCh[2] ? "3" : "").arg(checkList.bCh[2] ? QString::number(ratio_index[2][m_pConfig->flimIntensityRatioRefIdx[2]]) : "")
		///		.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimIntensityRatioRange[0][m_pConfig->flimIntensityRatioRefIdx[0]].min, 'f', 1) : "")
		///		.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimIntensityRatioRange[0][m_pConfig->flimIntensityRatioRefIdx[0]].max, 'f', 1) : "")
		///		.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimIntensityRatioRange[1][m_pConfig->flimIntensityRatioRefIdx[1]].min, 'f', 1) : "")
		///		.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimIntensityRatioRange[1][m_pConfig->flimIntensityRatioRefIdx[1]].max, 'f', 1) : "")
		///		.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimIntensityRatioRange[2][m_pConfig->flimIntensityRatioRefIdx[2]].min, 'f', 1) : "")
		///		.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimIntensityRatioRange[2][m_pConfig->flimIntensityRatioRefIdx[2]].max, 'f', 1) : "")
		///		.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimLifetimeRange[0].min, 'f', 1) : "")
		///		.arg(checkList.bCh[0] ? QString::number(m_pConfig->flimLifetimeRange[0].max, 'f', 1) : "")
		///		.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimLifetimeRange[1].min, 'f', 1) : "")
		///		.arg(checkList.bCh[1] ? QString::number(m_pConfig->flimLifetimeRange[1].max, 'f', 1) : "")
		///		.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimLifetimeRange[2].min, 'f', 1) : "")
		///		.arg(checkList.bCh[2] ? QString::number(m_pConfig->flimLifetimeRange[2].max, 'f', 1) : "");

		int frameCount = 0;
		while (frameCount < nTotalFrame)
		{
			// Get the buffer from the previous sync Queue
			ImgObjVector *pImgObjVecCirc = m_syncQueueCircWriting.pop();

			if (pImgObjVecCirc)
			{
				// Write circ images
				if (checkList.bCirc)
				{
					///if (!checkList.bMulti)
					{
						for (int i = 0; i < 3; i++)
						{
							if (checkList.bCh[i])
							{
								if (!checkList.bCircResize)
									pImgObjVecCirc->at(i)->qrgbimg.save(circPath[i] + QString("circ_%1_%2.bmp").arg("pullback").arg(frameCount + 1, 3, 10, (QChar)'0'), "bmp");
								else
									pImgObjVecCirc->at(i)->qrgbimg.scaled(checkList.nCircDiameter, checkList.nCircDiameter, Qt::IgnoreAspectRatio, m_defaultTransformation).
									save(circPath[i] + QString("circ_%1_%2.bmp").arg("pullback").arg(frameCount + 1, 3, 10, (QChar)'0'), "bmp");
							}
						}
					}
					///else
					///{
					///	// Write circ images
					///	if (!checkList.bCircResize)
					///		pImgObjVecCirc->at(0)->qrgbimg.save(circPath[0] + QString("circ_%1_%2.bmp").arg(folderName).arg(frameCount + 1, 3, 10, (QChar)'0'), "bmp");
					///	else
					///		pImgObjVecCirc->at(0)->qrgbimg.scaled(checkList.nCircDiameter, checkList.nCircDiameter, Qt::IgnoreAspectRatio, m_defaultTransformation).
					///		save(circPath[0] + QString("circ_%1_%2.bmp").arg(folderName).arg(frameCount + 1, 3, 10, (QChar)'0'), "bmp");
					///}
				}
				
				// Delete ImageObjects
				///if (!checkList.bMulti)
				{
					for (int i = 0; i < 3; i++)
						delete pImgObjVecCirc->at(i);
				}
				///else
				///	delete pImgObjVecCirc->at(0);
				delete pImgObjVecCirc;

			}

			emit savedSingleFrame(m_nSavedFrames++);
			frameCount++;
		}
	}

	emit savedSingleFrame(m_nSavedFrames++);
}
