
#include "IvusViewerDlg.h"

#include <QSqlQuery>

#include <Havana3/MainWindow.h>
#include <Havana3/Configuration.h>
#include <Havana3/QResultTab.h>
#include <Havana3/QViewTab.h>

#include <DataAcquisition/DataProcessing.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <ippi.h>


cv::Mat ImageRotateInner(const cv::Mat src, double degree
	, cv::Point2f base = cv::Point2f(std::numeric_limits<float>::infinity())) {
	if (base.x == std::numeric_limits<float>::infinity()) {
		base.x = src.cols / 2;
		base.y = src.rows / 2;
	}
	cv::Mat dst = src.clone();
	cv::Mat rot = cv::getRotationMatrix2D(base, degree, 1.0);
	cv::warpAffine(src, dst, rot, src.size());
	return std::move(dst);
}


IvusViewerDlg::IvusViewerDlg(QWidget *parent) :
    QDialog(parent), m_pConfigTemp(nullptr), m_pResultTab(nullptr), _running(false)
{
    // Set default size & frame
    setFixedSize(500, 563);
    setWindowFlags(Qt::Tool);
    setWindowTitle("IVUS Viewer");

    // Set main window objects
	m_pResultTab = dynamic_cast<QResultTab*>(parent);
	m_pConfig = m_pResultTab->getMainWnd()->m_pConfiguration;
	m_pConfigTemp = m_pResultTab->getDataProcessing()->getConfigTemp();

	// Create layout
	m_pVBoxLayout = new QVBoxLayout;
	m_pVBoxLayout->setSpacing(5);

	createWidgets();

    // Set layout
    m_pGroupBox_IvusViewer = new QGroupBox(this);
	m_pGroupBox_IvusViewer->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	m_pGroupBox_IvusViewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	m_pGroupBox_IvusViewer->setLayout(m_pVBoxLayout);

	QVBoxLayout *pVBoxLayout = new QVBoxLayout;
	pVBoxLayout->setSpacing(2);
	pVBoxLayout->addWidget(m_pGroupBox_IvusViewer);

	this->setLayout(pVBoxLayout);

	// First execution
	loadIvusData(true);

	if (QApplication::desktop()->screen()->rect().width() != 1280)
		this->move(m_pResultTab->getMainWnd()->x() - 500,
			m_pResultTab->getMainWnd()->y() + m_pResultTab->getMainWnd()->height() - 563);
}

IvusViewerDlg::~IvusViewerDlg()
{
	disconnect(this, SIGNAL(playingDone(void)), 0, 0);
	if (playing.joinable())
	{
		_running = false;
		playing.join();
	}
}

void IvusViewerDlg::keyPressEvent(QKeyEvent *e)
{
	if (e->key() == Qt::Key_BracketLeft)
		m_pSlider_SelectFrame->setValue(m_pSlider_SelectFrame->value() - 1);
	else if (e->key() == Qt::Key_BracketRight)
		m_pSlider_SelectFrame->setValue(m_pSlider_SelectFrame->value() + 1);
	else if (e->key() == Qt::Key_Backslash)
		m_pToggleButton_Play->setChecked(!m_pToggleButton_Play->isChecked());
}


void IvusViewerDlg::createWidgets()
{
	// Create widgets for IVUS viewer layout
	QGridLayout *pGridLayout_IvusViewer = new QGridLayout;
	pGridLayout_IvusViewer->setSpacing(2);

	// Create widgets
	m_pImageView_Ivus = new QImageView(ColorTable::colortable(IVUS_COLORTABLE), IVUS_IMG_SIZE, IVUS_IMG_SIZE, false, this);
	m_pImageView_Ivus->setMinimumSize(450, 450);
	m_pImageView_Ivus->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
	m_pImageView_Ivus->setSquare(true);
	m_pImageView_Ivus->getRender()->m_bCanBeMagnified = true;

	// Create push buttons for exploring frames
	m_pPushButton_Load = new QPushButton(this);
	m_pPushButton_Load->setText("Load IVUS Data");
	m_pPushButton_Load->setFixedWidth(150);

	m_pPushButton_Pick = new QPushButton(this);
	m_pPushButton_Pick->setText("Pick as Matched Frame");
	m_pPushButton_Pick->setFixedWidth(150);
	m_pPushButton_Pick->setDisabled(true);

	m_pPushButton_Decrement = new QPushButton(this);
	m_pPushButton_Decrement->setIcon(style()->standardIcon(QStyle::SP_MediaSeekBackward));
	m_pPushButton_Decrement->setFixedSize(40, 25);
	m_pPushButton_Decrement->setDisabled(true);

	m_pToggleButton_Play = new QPushButton(this);
	m_pToggleButton_Play->setCheckable(true);
	m_pToggleButton_Play->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
	m_pToggleButton_Play->setFixedSize(40, 25);
	m_pToggleButton_Play->setDisabled(true);

	m_pPushButton_Increment = new QPushButton(this);
	m_pPushButton_Increment->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));
	m_pPushButton_Increment->setFixedSize(40, 25);
	m_pPushButton_Increment->setDisabled(true);

	m_pLabel_SelectFrame = new QLabel(this);
	QString str; str.sprintf("   Frames : %4d / %4d", 1, 1000);
	m_pLabel_SelectFrame->setText(str);
	m_pLabel_SelectFrame->setFixedWidth(150);
	m_pLabel_SelectFrame->setDisabled(true);

	m_pSlider_SelectFrame = new QSlider(this);
	m_pSlider_SelectFrame->setOrientation(Qt::Horizontal);
	m_pSlider_SelectFrame->setRange(0, 1000 - 1);
	m_pSlider_SelectFrame->setValue(0);	
	m_pSlider_SelectFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_pSlider_SelectFrame->setDisabled(true);

	m_pLabel_Rotation = new QLabel(this);
	str.sprintf("   Rotation : %3d / %3d", 0, 360);
	m_pLabel_Rotation->setText(str);
	m_pLabel_Rotation->setFixedWidth(150);
	m_pLabel_Rotation->setDisabled(true);

	m_pSlider_Rotation = new QSlider(this);
	m_pSlider_Rotation->setOrientation(Qt::Horizontal);
	m_pSlider_Rotation->setRange(0, 360 - 1);
	m_pSlider_Rotation->setValue(0);
	m_pSlider_Rotation->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_pSlider_Rotation->setDisabled(true);

	// Set layout	
	QHBoxLayout *pHBoxLayout = new QHBoxLayout;
	pHBoxLayout->setSpacing(2);

	pHBoxLayout->addWidget(m_pPushButton_Load);
	pHBoxLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout->addWidget(m_pPushButton_Pick);

	pGridLayout_IvusViewer->addItem(pHBoxLayout, 0, 0, 1, 5);
	pGridLayout_IvusViewer->addWidget(m_pImageView_Ivus, 1, 0, 1, 5, Qt::AlignCenter);

	pGridLayout_IvusViewer->addWidget(m_pPushButton_Decrement, 2, 0);
	pGridLayout_IvusViewer->addWidget(m_pToggleButton_Play, 2, 1);
	pGridLayout_IvusViewer->addWidget(m_pPushButton_Increment, 2, 2);
	pGridLayout_IvusViewer->addWidget(m_pLabel_SelectFrame, 2, 3);
	pGridLayout_IvusViewer->addWidget(m_pSlider_SelectFrame, 2, 4);

	pGridLayout_IvusViewer->addWidget(m_pLabel_Rotation, 3, 3);
	pGridLayout_IvusViewer->addWidget(m_pSlider_Rotation, 3, 4);

	m_pVBoxLayout->addItem(pGridLayout_IvusViewer);

	// Connect
	connect(this, SIGNAL(paintIvusImage(uint8_t*)), m_pImageView_Ivus, SLOT(drawRgbImage(uint8_t*)));

	connect(m_pPushButton_Load, SIGNAL(clicked(bool)), this, SLOT(loadIvusData(bool)));
	connect(m_pPushButton_Pick, SIGNAL(clicked(bool)), this, SLOT(pickAsMatchedFrame()));

	connect(m_pToggleButton_Play, SIGNAL(toggled(bool)), this, SLOT(play(bool)));
	connect(this, &IvusViewerDlg::playingDone, [&]() { m_pToggleButton_Play->setChecked(false); });
	connect(m_pPushButton_Increment, &QPushButton::clicked, [&]() { m_pSlider_SelectFrame->setValue(m_pSlider_SelectFrame->value() + 1); });
	connect(m_pPushButton_Decrement, &QPushButton::clicked, [&]() { m_pSlider_SelectFrame->setValue(m_pSlider_SelectFrame->value() - 1); });

	connect(m_pSlider_SelectFrame, SIGNAL(valueChanged(int)), this, SLOT(visualizeImage(int)));
	connect(m_pSlider_Rotation, SIGNAL(valueChanged(int)), this, SLOT(imageRotated(int)));
}


void IvusViewerDlg::loadIvusData(bool status)
{	
	// Get relative file name first
	QString fileName;	
	if (status && m_pConfigTemp->ivusPath != "")
		fileName = m_pConfigTemp->ivusPath;
	else
	{
		fileName = QFileDialog::getOpenFileName(nullptr, "Load IVUS Data", m_pConfig->dbPath, "IVUS Movie (*.wmv)", nullptr, QFileDialog::DontUseNativeDialog);

		QStringList folders = fileName.split('/');
		int pos = 0;
		foreach(const QString &folder, folders) {			
			if (folder.contains("record"))
				break;
			pos++;
		}
		for (int i = 0; i < pos; i++)
			folders.pop_front();
	
		fileName = folders.join('/');
	}

	// Get path to read	
	if (fileName != "")
	{
		m_pConfigTemp->ivusPath = fileName;
		{
			cv::VideoCapture capture((m_pConfig->dbPath + "/" + m_pConfigTemp->ivusPath).toStdString());
			cv::Mat frame;

			if (capture.isOpened())
			{
				m_vectorIvusImages.clear();
				while (1)
				{
					capture >> frame;
					if (frame.empty())
						break;
					
					np::Uint8Array2 ivusImage(frame.cols, frame.rows);
					ippiCopy_8u_C3C1R(frame.data, 3 * frame.cols, ivusImage, frame.cols, { frame.cols, frame.rows });
					ippiMirror_8u_C1IR(ivusImage, frame.cols, { frame.cols, frame.rows }, ippAxsVertical);
					m_vectorIvusImages.push_back(ivusImage);
				} 
			}
			else
			{
				QMessageBox MsgBox(QMessageBox::Critical, "Read error", "Cannot open IVUS movie!");
				MsgBox.exec();

				return;
			}
		}

		// Load matching data
		m_vectorMatches.clear();

		QString match_path = m_pResultTab->getRecordInfo().filename;
		match_path.replace("pullback.data", "ivus_match.csv");

		QFile match_file(match_path);
		if (match_file.open(QFile::ReadOnly))
		{
			QTextStream stream(&match_file);

			stream.readLine();
			while (!stream.atEnd())
			{
				QStringList matches = stream.readLine().split('\t');
				if (matches.size() > 1)
					m_vectorMatches.push_back(matches);
			}
			match_file.close();
		}

		// Memorize IVUS file path
		m_pConfigTemp->setConfigFile(m_pResultTab->getDataProcessing()->getIniName());

		// Set widgets
		m_pImageView_Ivus->resetSize(m_vectorIvusImages.at(0).size(0), m_vectorIvusImages.at(0).size(1));

		m_pPushButton_Pick->setEnabled(true);
		m_pToggleButton_Play->setEnabled(true);
		m_pPushButton_Decrement->setEnabled(true);
		m_pPushButton_Increment->setEnabled(true);
		m_pLabel_SelectFrame->setEnabled(true);
		m_pSlider_SelectFrame->setEnabled(true);
		m_pLabel_Rotation->setEnabled(true);
		m_pSlider_Rotation->setEnabled(true);

		QString str; str.sprintf("   Frames : %4d / %4d", 1, m_vectorIvusImages.size());
		m_pLabel_SelectFrame->setText(str);
		m_pSlider_SelectFrame->setRange(0, m_vectorIvusImages.size() - 1);

		// Play IVUS movie
		m_pToggleButton_Play->setChecked(true);

		// Invalidate parent dialog
		m_pResultTab->getViewTab()->invalidate();
	}	
}

void IvusViewerDlg::pickAsMatchedFrame()
{
	// Add matching data to vector
	int oct_frame = m_pResultTab->getViewTab()->getCurrentFrame() + 1;

	int k = 0;
	foreach(QStringList _matches, m_vectorMatches)
	{
		if (_matches.at(0).toInt() == oct_frame)
			break;
		k++;
	}

	QStringList matches;
	matches << QString::number(oct_frame) // OCT frame
		<< QString::number(m_pSlider_SelectFrame->value() + 1) // IVUS frame
		<< QString::number(m_pSlider_Rotation->value()); // IVUS rotation
	
	if (k == m_vectorMatches.size())
		m_vectorMatches.push_back(matches);
	else
		m_vectorMatches.at(k) = matches;

	// Save matching data
	QString match_path = m_pResultTab->getRecordInfo().filename;
	match_path.replace("pullback.data", "ivus_match.csv");

	QFile match_file(match_path);
	if (match_file.open(QFile::WriteOnly))
	{
		QTextStream stream(&match_file);
		stream << "OCT Frame" << "\t"
			<< "IVUS Frame" << "\t"
			<< "Rotation" << "\n";

		for (int i = 0; i < m_vectorMatches.size(); i++)
		{
			QStringList _matches = m_vectorMatches.at(i);

			stream << _matches.at(0) << "\t" // OCT frame
				<< _matches.at(1) << "\t" // IVUS frame
				<< _matches.at(2) << "\n"; // IVUS rotation
		}
		match_file.close();
	}

	// Invalidate parent dialog
	m_pResultTab->getViewTab()->invalidate();
}

void IvusViewerDlg::play(bool enabled)
{
	if (enabled)
	{
		m_pToggleButton_Play->setIcon(style()->standardIcon(QStyle::SP_MediaStop));

		int cur_frame = m_pSlider_SelectFrame->value();
		int end_frame = (int)m_vectorIvusImages.size();

		if (cur_frame + 1 == end_frame)
			cur_frame = 0;

		_running = true;
		playing = std::thread([&, cur_frame, end_frame]() {
			for (int i = cur_frame; i < end_frame; i++)
			{
				m_pSlider_SelectFrame->setValue(i);
				std::this_thread::sleep_for(std::chrono::milliseconds(33));

				if (!_running) break;
			}
			emit playingDone();
		});
	}
	else
	{
		if (playing.joinable())
		{
			_running = false;
			playing.join();
		}

		m_pToggleButton_Play->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
	}
}

void IvusViewerDlg::visualizeImage(int frame)
{
	if (m_vectorIvusImages.size() != 0)
	{
		// Draw IVUS image
		m_imageBuffer = np::Uint8Array2(m_vectorIvusImages.at(frame).size(1), m_vectorIvusImages.at(frame).size(0));
		getIvusImage(frame, m_pSlider_Rotation->value(), m_imageBuffer);
		emit paintIvusImage(m_imageBuffer);

		// Set widget
		QString str; str.sprintf("   Frames : %4d / %4d", frame + 1, m_vectorIvusImages.size());
		m_pLabel_SelectFrame->setText(str);		
	}
}

void IvusViewerDlg::imageRotated(int angle)
{
	visualizeImage(m_pSlider_SelectFrame->value());

	// Set widget
	QString str; str.sprintf("   Rotation : %3d / %3d", angle, 360);
	m_pLabel_Rotation->setText(str);
}


void IvusViewerDlg::getIvusImage(int frame, int angle, np::Uint8Array2& dst)
{
	cv::Mat inputMat = cv::Mat(m_vectorIvusImages.at(frame).size(1), m_vectorIvusImages.at(frame).size(0), CV_8UC1, m_vectorIvusImages.at(frame));
	cv::Mat outputMat = ImageRotateInner(inputMat, angle);
	memcpy(dst, outputMat.data, dst.length());	
}