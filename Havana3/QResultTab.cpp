
#include "QResultTab.h"

#include <QSqlQuery>

#include <Havana3/MainWindow.h>
#include <Havana3/Configuration.h>
#include <Havana3/HvnSqlDataBase.h>
#include <Havana3/QViewTab.h>

#include <Havana3/Dialog/SettingDlg.h>
#include <Havana3/Dialog/PulseReviewTab.h>
#include <Havana3/Dialog/ExportDlg.h>

#include <DataAcquisition/DataProcessing.h>


QResultTab::QResultTab(QString record_id, QWidget *parent) :
    QDialog(parent), m_bIsDataLoaded(false), m_pSettingDlg(nullptr), m_pExportDlg(nullptr)
{
	// Set main window objects
	m_pMainWnd = dynamic_cast<MainWindow*>(parent);
	m_pConfig = m_pMainWnd->m_pConfiguration;    
    m_pHvnSqlDataBase = m_pMainWnd->m_pHvnSqlDataBase;
    m_recordInfo.recordId = record_id;
	
	// Create post-processing objects
	m_pDataProcessing = new DataProcessing(this);

    // Create widgets for result review
    createResultReviewWidgets();
	
    // Create window layout
    QVBoxLayout* pVBoxLayout = new QVBoxLayout;
	pVBoxLayout->setSpacing(2);
	pVBoxLayout->setAlignment(Qt::AlignCenter);

	pVBoxLayout->addWidget(m_pGroupBox_ResultReview);
	pVBoxLayout->addWidget(m_pProgressBar, Qt::AlignCenter);

	// Set window layout
	this->setLayout(pVBoxLayout);
}

QResultTab::~QResultTab()
{
	delete m_pDataProcessing;
}


void QResultTab::closeEvent(QCloseEvent *e)
{
	if (m_pSettingDlg)
		m_pSettingDlg->close();

	e->accept();
}

void QResultTab::keyPressEvent(QKeyEvent *e)
{
	if (e->key() != Qt::Key_Escape)
        QDialog::keyPressEvent(e);
}


void QResultTab::createResultReviewWidgets()
{
    // Create widgets for live streaming view
    m_pGroupBox_ResultReview = new QGroupBox(this);
    m_pGroupBox_ResultReview->setMinimumWidth(1100);
#ifndef ALTERNATIVE_VIEW
    m_pGroupBox_ResultReview->setFixedHeight(780);
#else^
	m_pGroupBox_ResultReview->setMinimumHeight(780);
#endif
    m_pGroupBox_ResultReview->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
    m_pGroupBox_ResultReview->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    m_pLabel_PatientInformation = new QLabel(this);
    m_pLabel_RecordInformation = new QLabel(this);

    m_pComboBox_Vessel = new QComboBox(this);
    for (int i = 0; i <= 16; i++)
        m_pComboBox_Vessel->insertItem(i, m_pHvnSqlDataBase->getVessel(i));

    m_pComboBox_Procedure = new QComboBox(this);
    for (int i = 0; i <= 4; i++)
        m_pComboBox_Procedure->insertItem(i, m_pHvnSqlDataBase->getProcedure(i));

	loadRecordInfo();
	loadPatientInfo();

    m_pLabel_PatientInformation->setStyleSheet("QLabel{font-size:11pt}");
    m_pLabel_PatientInformation->setAlignment(Qt::AlignBottom);
    m_pLabel_RecordInformation->setAlignment(Qt::AlignCenter);

    m_pViewTab = new QViewTab(false, this);

	m_pPushButton_OpenFolder = new QPushButton(this);
	m_pPushButton_OpenFolder->setText("  Open Folder");
	m_pPushButton_OpenFolder->setIcon(style()->standardIcon(QStyle::SP_DirHomeIcon));
	m_pPushButton_OpenFolder->setFixedSize(125, 25);

	m_pPushButton_Comment = new QPushButton(this);
	m_pPushButton_Comment->setText("  Comment");
	m_pPushButton_Comment->setIcon(style()->standardIcon(QStyle::SP_FileDialogContentsView));
	m_pPushButton_Comment->setFixedSize(125, 25);

    m_pPushButton_Setting = new QPushButton(this);
    m_pPushButton_Setting->setText("  Setting");
    m_pPushButton_Setting->setIcon(style()->standardIcon(QStyle::SP_FileDialogInfoView));
    m_pPushButton_Setting->setFixedSize(125, 25);

    m_pPushButton_Export = new QPushButton(this);
    m_pPushButton_Export->setText("  Export");
    m_pPushButton_Export->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_pPushButton_Export->setFixedSize(125, 25);

    // Set layout: result review
    QVBoxLayout *pVBoxLayout_ResultReview = new QVBoxLayout;
    pVBoxLayout_ResultReview->setSpacing(0);
    pVBoxLayout_ResultReview->setAlignment(Qt::AlignCenter);

    QGridLayout *pGridLayout_RecordInformation = new QGridLayout;
    pGridLayout_RecordInformation->setSpacing(2);
#ifndef ALTERNATIVE_VIEW
    pGridLayout_RecordInformation->addWidget(m_pLabel_RecordInformation, 0, 0, 1, 2);
    pGridLayout_RecordInformation->addWidget(m_pComboBox_Vessel, 1, 0);
    pGridLayout_RecordInformation->addWidget(m_pComboBox_Procedure, 1, 1);
#else	
	pGridLayout_RecordInformation->setAlignment(Qt::AlignLeft);
	pGridLayout_RecordInformation->addWidget(m_pLabel_RecordInformation, 0, 0, 1, 2);
	pGridLayout_RecordInformation->addWidget(m_pComboBox_Vessel, 1, 0);
	pGridLayout_RecordInformation->addWidget(m_pComboBox_Procedure, 1, 1);
	pGridLayout_RecordInformation->addWidget(m_pLabel_PatientInformation, 2, 0, 1, 2);
	pGridLayout_RecordInformation->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 3, 0, 1, 2);
#endif

    QGridLayout *pGridLayout_Buttons = new QGridLayout;
    pGridLayout_Buttons->setSpacing(5);
	pGridLayout_Buttons->addWidget(m_pPushButton_OpenFolder, 0, 0);
	pGridLayout_Buttons->addWidget(m_pPushButton_Comment, 0, 1);
    pGridLayout_Buttons->addWidget(m_pPushButton_Export, 1, 0);
    pGridLayout_Buttons->addWidget(m_pPushButton_Setting, 1, 1);
#ifdef ALTERNATIVE_VIEW
	pGridLayout_Buttons->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 2, 0, 1, 2);
#endif

#ifndef ALTERNATIVE_VIEW
    QHBoxLayout *pHBoxLayout_Title = new QHBoxLayout;
    pHBoxLayout_Title->setSpacing(5);
    pHBoxLayout_Title->setAlignment(m_pPushButton_Export, Qt::AlignBottom);
    pHBoxLayout_Title->addWidget(m_pLabel_PatientInformation);
    pHBoxLayout_Title->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
    pHBoxLayout_Title->addItem(pGridLayout_RecordInformation);
    pHBoxLayout_Title->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
    pHBoxLayout_Title->addItem(pGridLayout_Buttons);
#endif

#ifndef ALTERNATIVE_VIEW
    pVBoxLayout_ResultReview->addItem(pHBoxLayout_Title);
#endif
    pVBoxLayout_ResultReview->addWidget(m_pViewTab->getViewWidget());
#ifdef ALTERNATIVE_VIEW	
	m_pViewTab->getVisWidget(0)->setLayout(pGridLayout_RecordInformation);
	m_pViewTab->getVisWidget(2)->setLayout(pGridLayout_Buttons);
#endif
    pVBoxLayout_ResultReview->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

    m_pGroupBox_ResultReview->setLayout(pVBoxLayout_ResultReview);
	m_pGroupBox_ResultReview->setVisible(false);
	
	// Progress bar
	m_pProgressBar = new QProgressBar(this);
	m_pProgressBar->setFormat("Processing... %p%");
	m_pProgressBar->setFixedWidth(450);
	m_pProgressBar->setValue(0);

    // Connect signal and slot
	connect(m_pDataProcessing, SIGNAL(processedSingleFrame(int)), m_pProgressBar, SLOT(setValue(int)));
	connect(m_pDataProcessing, &DataProcessing::abortedProcessing, [&]() { m_pMainWnd->getTabWidget()->tabCloseRequested(m_pMainWnd->getCurrentTabIndex()); });
	connect(m_pDataProcessing, SIGNAL(finishedProcessing(bool)), this, SLOT(setVisibleState(bool)));

	connect(this, SIGNAL(getCapture(QByteArray &)), m_pViewTab, SLOT(getCapture(QByteArray &)));
    connect(m_pComboBox_Vessel, SIGNAL(currentIndexChanged(int)), this, SLOT(changeVesselInfo(int)));
    connect(m_pComboBox_Procedure, SIGNAL(currentIndexChanged(int)), this, SLOT(changeProcedureInfo(int)));
	connect(m_pPushButton_OpenFolder, SIGNAL(clicked(bool)), this, SLOT(openContainingFolder()));
	connect(m_pPushButton_Comment, SIGNAL(clicked(bool)), this, SLOT(createCommentDlg()));
    connect(m_pPushButton_Setting, SIGNAL(clicked(bool)), this, SLOT(createSettingDlg()));
    connect(m_pPushButton_Export, SIGNAL(clicked(bool)), this, SLOT(createExportDlg()));
}


void QResultTab::readRecordData()
{
	if (!m_bIsDataLoaded)
	{
		/// Make progress dialog 
		///m_pProgressDlg = new QProgressDialog("Processing...", "Cancel", 0, 100, this);
		///m_pProgressDlg->setWindowTitle("Review");
		///m_pProgressDlg->setCancelButton(0);
		///m_pProgressDlg->setWindowModality(Qt::WindowModal);
		///m_pProgressDlg->setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
		///m_pProgressDlg->move((m_pMainWnd->width() - m_pProgressDlg->width()) / 2, (m_pMainWnd->height() - m_pProgressDlg->height()) / 2);
		///m_pProgressDlg->setFixedSize(m_pProgressDlg->width(), m_pProgressDlg->height());
		///connect(&progress, &QProgressDialog::canceled, [&]() { m_pDataProcessing->m_bAbort = true; });
		///m_pProgressDlg->show();

		// Start read and process the reviewing file
		m_pDataProcessing->startProcessing(m_recordInfo.filename);

		// Write to log...
		m_pConfig->writeToLog(QString("Record reviewing: %1 (ID: %2): %3: record id: %4")
			.arg(m_recordInfo.patientName).arg(m_recordInfo.patientId).arg(m_recordInfo.date).arg(m_recordInfo.recordId));

		/// // Abort
		///if (m_pDataProcessing->m_bAbort)
		///{
		///	std::thread tab_close([&]() {
		///		//while (1)
		///		//{
		///		//	int total = (int)m_pMainWnd->getVectorTabViews().size();
		///		//	int current = m_pMainWnd->getTabWidget()->currentIndex() + 1;
		///		//	if (total > current)
		///		//	{
		///		//		emit m_pMainWnd->getTabWidget()->tabCloseRequested(current);
		///		//		break;
		///		//	}
		///		//}
		///	});
		///	tab_close.detach();
		
		m_bIsDataLoaded = true;
	}
	else
		m_pViewTab->invalidate();

	QClipboard *pClipBoard = QApplication::clipboard();
	QString folderPath = m_recordInfo.filename;
	for (int i = folderPath.size() - 1; i >= 0; i--)
	{
		int slash_pos = i;
		if (folderPath.at(i) == '/')
		{
			folderPath = folderPath.left(slash_pos);
			break;
		}
	}
	pClipBoard->setText(folderPath);
}

void QResultTab::loadRecordInfo()
{
	QString command = QString("SELECT * FROM records WHERE id=%1").arg(m_recordInfo.recordId);
	m_pHvnSqlDataBase->queryDatabase(command, [&](QSqlQuery& _sqlQuery) {
		while (_sqlQuery.next())
		{
			m_recordInfo.patientId = _sqlQuery.value(1).toString();
			m_recordInfo.title = _sqlQuery.value(7).toString();
			m_recordInfo.date = _sqlQuery.value(3).toString();
			m_recordInfo.vessel = _sqlQuery.value(12).toInt();
			m_recordInfo.procedure = _sqlQuery.value(11).toInt();
			m_recordInfo.filename0 = _sqlQuery.value(9).toString();
			int idx = m_recordInfo.filename0.indexOf("record");
			m_recordInfo.filename = m_pConfig->dbPath + m_recordInfo.filename0.remove(0, idx - 1);;
			m_recordInfo.comment = _sqlQuery.value(8).toString();

			m_pLabel_RecordInformation->setText(QString("<b><font size=6>%1</font></b>"
				"&nbsp;&nbsp;&nbsp;<font size=4>%2</font>")
				.arg(m_recordInfo.title).arg(m_recordInfo.date));
			m_pComboBox_Vessel->setCurrentIndex(m_recordInfo.vessel);
			m_pComboBox_Procedure->setCurrentIndex(m_recordInfo.procedure);
		}

		m_pConfig->writeToLog(QString("Record info loaded: %1 (ID: %2): %3 : record id: %4")
			.arg(m_recordInfo.patientName).arg(m_recordInfo.patientId).arg(m_recordInfo.date).arg(m_recordInfo.recordId));
	});
}

void QResultTab::loadPatientInfo()
{
	QString command = QString("SELECT * FROM patients WHERE patient_id=%1").arg(m_recordInfo.patientId);
	m_pHvnSqlDataBase->queryDatabase(command, [&](QSqlQuery& _sqlQuery) {
		while (_sqlQuery.next())
		{
			m_recordInfo.patientName = _sqlQuery.value(1).toString() + ", " + _sqlQuery.value(0).toString();

			QDate date_of_birth = QDate::fromString(_sqlQuery.value(4).toString(), "yyyy-MM-dd");
			QDate date_today = QDate::currentDate();
			int age = (int)((double)date_of_birth.daysTo(date_today) / 365.25);
			QString gender = m_pHvnSqlDataBase->getGender(_sqlQuery.value(5).toInt());

#ifndef ALTERNATIVE_VIEW
			m_pLabel_PatientInformation->setText(QString("<b>%1 (%2%3)</b>\n"
				"&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<b>ID:</b> %4").
#else
			m_pLabel_PatientInformation->setText(QString("<br><b>%1 (%2%3)</b>\n"
				"&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<br><br><b>ID:</b> %4").
#endif
				arg(m_recordInfo.patientName).arg(age).arg(gender).
				arg(QString("%1").arg(m_recordInfo.patientId.toInt(), 8, 10, QChar('0'))));
			setWindowTitle(QString("Review: %1: %2").arg(m_recordInfo.patientName).arg(m_recordInfo.date));
		}

		m_pConfig->writeToLog(QString("Patient info loaded: %1 (ID: %2) [QResultTab]").arg(m_recordInfo.patientName).arg(m_recordInfo.patientId));
	});
}

void QResultTab::updatePreviewImage()
{
	emit getCapture(m_recordInfo.preview);

	QString command = QString("UPDATE records SET preview=:preview WHERE id=%1").arg(m_recordInfo.recordId);
	m_pHvnSqlDataBase->queryDatabase(command, [&](QSqlQuery &) {}, false, m_recordInfo.preview);

	m_pConfig->writeToLog(QString("Record preview updated: %1 (ID: %2): %3 : record id: %4")
		.arg(m_recordInfo.patientName).arg(m_recordInfo.patientId).arg(m_recordInfo.date).arg(m_recordInfo.recordId));
}


void QResultTab::setVisibleState(bool enabled)
{
	m_pGroupBox_ResultReview->setVisible(enabled);
	m_pProgressBar->setVisible(!enabled);
}

void QResultTab::changeVesselInfo(int info)
{
    m_recordInfo.vessel = info;
    QString command = QString("UPDATE records SET vessel_id=%1 WHERE id=%2").arg(info).arg(m_recordInfo.recordId);
    m_pHvnSqlDataBase->queryDatabase(command);

	m_pConfig->writeToLog(QString("Record vessel updated: %1 (ID: %2): %3 : record id: %4")
		.arg(m_recordInfo.patientName).arg(m_recordInfo.patientId).arg(m_recordInfo.date).arg(m_recordInfo.recordId));
}

void QResultTab::changeProcedureInfo(int info)
{
    m_recordInfo.procedure = info;
    QString command = QString("UPDATE records SET procedure_id=%1 WHERE id=%2").arg(info).arg(m_recordInfo.recordId);
    m_pHvnSqlDataBase->queryDatabase(command);

	m_pConfig->writeToLog(QString("Record procedure updated: %1 (ID: %2): %3 : record id: %4")
		.arg(m_recordInfo.patientName).arg(m_recordInfo.patientId).arg(m_recordInfo.date).arg(m_recordInfo.recordId));
}

void QResultTab::openContainingFolder()
{
	QString folderPath = m_recordInfo.filename;
	for (int i = folderPath.size() - 1; i >= 0; i--)
	{
		int slash_pos = i;
		if (folderPath.at(i) == '/')
		{
			folderPath = folderPath.left(slash_pos);
			break;
		}
	}
	QDesktopServices::openUrl(QUrl("file:///" + folderPath));
}

void QResultTab::createCommentDlg()
{
	QDialog *pDialog = new QDialog(this);
	{		
		QTextEdit *pTextEdit_Comment = new QTextEdit(this);
		pTextEdit_Comment->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		pTextEdit_Comment->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		pTextEdit_Comment->setPlainText(m_recordInfo.comment);
		connect(pTextEdit_Comment, &QTextEdit::textChanged, [&, pTextEdit_Comment]() { m_recordInfo.comment = pTextEdit_Comment->toPlainText(); });
		connect(pDialog, SIGNAL(finished(int)), this, SLOT(updateComment()));

		QVBoxLayout *pVBoxLayout = new QVBoxLayout;
		pVBoxLayout->addWidget(pTextEdit_Comment);
		pDialog->setLayout(pVBoxLayout);
	}
	pDialog->setWindowTitle("Comment");
	pDialog->setFixedSize(300, 200);
	pDialog->setModal(true);
	pDialog->exec();
}

void QResultTab::updateComment()
{
	QString command = QString("UPDATE records SET comment='%1' WHERE id=%2").arg(m_recordInfo.comment).arg(m_recordInfo.recordId);
	m_pHvnSqlDataBase->queryDatabase(command);

	m_pConfig->writeToLog(QString("Record comment updated: %1 (ID: %2): %3 : record id: %4")
		.arg(m_recordInfo.patientName).arg(m_recordInfo.patientId).arg(m_recordInfo.date).arg(m_recordInfo.recordId));
}

void QResultTab::createSettingDlg()
{
	if (m_pSettingDlg == nullptr)
	{
		m_pSettingDlg = new SettingDlg(this);
		connect(m_pSettingDlg, SIGNAL(finished(int)), this, SLOT(deleteSettingDlg()));
		m_pSettingDlg->show(); // modal-less		
		
		m_pViewTab->setCircImageViewClickedMouseCallback([&]() { m_pSettingDlg->getPulseReviewTab()->setCurrentAline(int(m_pViewTab->getCurrentAline() / 4)); });
		m_pViewTab->setEnFaceImageViewClickedMouseCallback([&]() { m_pSettingDlg->getPulseReviewTab()->setCurrentAline(int(m_pViewTab->getCurrentAline() / 4)); });
		m_pViewTab->setLongiImageViewClickedMouseCallback([&]() { m_pSettingDlg->getPulseReviewTab()->setCurrentAline(int(m_pViewTab->getCurrentAline() / 4)); });

		///m_pSettingDlg->setModal(true);
		///m_pSettingDlg->exec();
	}
	m_pSettingDlg->raise();
	m_pSettingDlg->activateWindow();
}

void QResultTab::deleteSettingDlg()
{
	m_pViewTab->setCircImageViewClickedMouseCallback([&]() { });
	m_pViewTab->setEnFaceImageViewClickedMouseCallback([&]() { });
	m_pViewTab->setLongiImageViewClickedMouseCallback([&]() { });
	
	QImageView *pCircView = m_pViewTab->getCircImageView();
	pCircView->getRender()->m_bArcRoiSelect = false;
	pCircView->getRender()->m_bArcRoiShow = false;
	pCircView->getRender()->m_nClicked = 0;
	pCircView->getRender()->update();

	m_pSettingDlg->deleteLater();
    m_pSettingDlg = nullptr;
}

void QResultTab::createExportDlg()
{
    if (m_pExportDlg == nullptr)
    {
        m_pExportDlg = new ExportDlg(this);
        connect(m_pExportDlg, SIGNAL(finished(int)), this, SLOT(deleteExportDlg()));
		m_pExportDlg->setModal(true);
		m_pExportDlg->exec();
    }
}

void QResultTab::deleteExportDlg()
{
    m_pExportDlg->deleteLater();
    m_pExportDlg = nullptr;
}
