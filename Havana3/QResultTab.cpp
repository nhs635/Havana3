
#include "QResultTab.h"

#include <QSqlQuery>

#include <Havana3/MainWindow.h>
#include <Havana3/Configuration.h>
#include <Havana3/HvnSqlDataBase.h>
#include <Havana3/QViewTab.h>

#include <Havana3/Dialog/SettingDlg.h>
//#include <Havana3/Dialog/ExportDlg.h>

#include <DataAcquisition/DataProcessing.h>


QResultTab::QResultTab(QString record_id, QWidget *parent) :
    QDialog(parent), m_pSettingDlg(nullptr), m_pExportDlg(nullptr)
{
	// Set main window objects
	m_pMainWnd = dynamic_cast<MainWindow*>(parent);
	m_pConfig = m_pMainWnd->m_pConfiguration;    
    m_pHvnSqlDataBase = m_pMainWnd->m_pHvnSqlDataBase;
    m_recordInfo.recordId = record_id;

    // Create widgets for result review
    createResultReviewWidgets();

    // Create post-processing objects
    m_pDataProcessing = new DataProcessing(this);

    // Create window layout
    QHBoxLayout* pHBoxLayout = new QHBoxLayout;
    pHBoxLayout->setSpacing(2);

    pHBoxLayout->addWidget(m_pGroupBox_ResultReview);
	
    // Initialize & start result review
    readRecordData();  // 이거 fail 나는 경우는 어떡해?

	// Set window layout
	this->setLayout(pHBoxLayout);
}

QResultTab::~QResultTab()
{
	delete m_pDataProcessing;
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
    m_pGroupBox_ResultReview->setFixedHeight(780);
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

    m_pPushButton_Setting = new QPushButton(this);
    m_pPushButton_Setting->setText("  Setting");
    m_pPushButton_Setting->setIcon(style()->standardIcon(QStyle::SP_FileDialogInfoView));
    m_pPushButton_Setting->setFixedSize(100, 25);

    m_pPushButton_Export = new QPushButton(this);
    m_pPushButton_Export->setText("  Export");
    m_pPushButton_Export->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_pPushButton_Export->setFixedSize(100, 25);

    // Set layout: result review
    QVBoxLayout *pVBoxLayout_ResultReview = new QVBoxLayout;
    pVBoxLayout_ResultReview->setSpacing(0);
    pVBoxLayout_ResultReview->setAlignment(Qt::AlignCenter);

    QGridLayout *pGridLayout_RecordInformation = new QGridLayout;
    pGridLayout_RecordInformation->setSpacing(2);
    pGridLayout_RecordInformation->addWidget(m_pLabel_RecordInformation, 0, 0, 1, 2);
    pGridLayout_RecordInformation->addWidget(m_pComboBox_Vessel, 1, 0);
    pGridLayout_RecordInformation->addWidget(m_pComboBox_Procedure, 1, 1);

    QGridLayout *pGridLayout_Buttons = new QGridLayout;
    pGridLayout_Buttons->setSpacing(5);
    pGridLayout_Buttons->addWidget(new QLabel(this), 0, 0, 1, 2);
    pGridLayout_Buttons->addWidget(m_pPushButton_Export, 1, 0);
    pGridLayout_Buttons->addWidget(m_pPushButton_Setting, 1, 1);

    QHBoxLayout *pHBoxLayout_Title = new QHBoxLayout;
    pHBoxLayout_Title->setSpacing(5);
    pHBoxLayout_Title->setAlignment(m_pPushButton_Export, Qt::AlignBottom);// (Qt::AlignBottom);
    pHBoxLayout_Title->addWidget(m_pLabel_PatientInformation);
    pHBoxLayout_Title->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
    pHBoxLayout_Title->addItem(pGridLayout_RecordInformation);
    pHBoxLayout_Title->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
    pHBoxLayout_Title->addItem(pGridLayout_Buttons);

    pVBoxLayout_ResultReview->addItem(pHBoxLayout_Title);
    pVBoxLayout_ResultReview->addWidget(m_pViewTab->getViewWidget());
    pVBoxLayout_ResultReview->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

    m_pGroupBox_ResultReview->setLayout(pVBoxLayout_ResultReview);


    // Connect signal and slot
	connect(this, SIGNAL(getCapture(QByteArray &)), m_pViewTab, SLOT(getCapture(QByteArray &)));
    connect(m_pComboBox_Vessel, SIGNAL(currentIndexChanged(int)), this, SLOT(changeVesselInfo(int)));
    connect(m_pComboBox_Procedure, SIGNAL(currentIndexChanged(int)), this, SLOT(changeProcedureInfo(int)));
    connect(m_pPushButton_Setting, SIGNAL(clicked(bool)), this, SLOT(createSettingDlg()));
    connect(m_pPushButton_Export, SIGNAL(clicked(bool)), this, SLOT(createExportDlg()));
}


void QResultTab::readRecordData()
{
	// Start read and process the reviewing file
	m_pDataProcessing->startProcessing(m_recordInfo.filename);

	// Make progress dialog
	QProgressDialog progress("Processing...", "Cancel", 0, 100, this);
	connect(m_pDataProcessing, SIGNAL(processedSingleFrame(int)), &progress, SLOT(setValue(int)));
	connect(m_pDataProcessing, &DataProcessing::abortedProcessing, [&]() { progress.setValue(100); /* tab 종료 조건 */ });
	///connect(&progress, &QProgressDialog::canceled, [&]() { m_pDataProcessing->m_bAbort = true; });
	progress.setWindowTitle("Review");
	progress.setCancelButton(0);
	progress.setWindowModality(Qt::WindowModal);
	progress.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
	progress.move((m_pMainWnd->width() - progress.width()) / 2, (m_pMainWnd->height() - progress.height()) / 2);
	progress.setFixedSize(progress.width(), progress.height());
	progress.exec();
	
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
	///}

	m_pConfig->writeToLog(QString("Record reviewing: %1 (ID: %2): %3 : record id: %4")
		.arg(m_recordInfo.patientName).arg(m_recordInfo.patientId).arg(m_recordInfo.date).arg(m_recordInfo.recordId));
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
			m_recordInfo.filename = _sqlQuery.value(9).toString();

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

			m_pLabel_PatientInformation->setText(QString("<b>%1</b>"
				"&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<b>ID:</b> %2").
				arg(m_recordInfo.patientName).
				arg(m_recordInfo.patientId));
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


void QResultTab::createSettingDlg()
{
    if (m_pSettingDlg == nullptr)
    {
        m_pSettingDlg = new SettingDlg(this);
        connect(m_pSettingDlg, SIGNAL(finished(int)), this, SLOT(deleteSettingDlg()));
		m_pSettingDlg->setModal(true);
		m_pSettingDlg->exec();
    }
}

void QResultTab::deleteSettingDlg()
{
	m_pSettingDlg->deleteLater();
    m_pSettingDlg = nullptr;
}

void QResultTab::createExportDlg()
{
	// EXPORT DIALOG 기능 구현!!!
//    if (m_pExportDlg == nullptr)
//    {
//        m_pExportDlg = new SettingDlg(false, this);
//        connect(m_pExportDlg, SIGNAL(finished(int)), this, SLOT(deleteExportDlg()));
//        m_pExportDlg->show();
//    }
//    m_pExportDlg->raise();
//    m_pExportDlg->activateWindow();
}

void QResultTab::deleteExportDlg()
{
//    m_pExportDlg->deleteLater();
//    m_pExportDlg = nullptr;
}



