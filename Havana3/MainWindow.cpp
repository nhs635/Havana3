
#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <Havana3/Configuration.h>

#include <Havana3/QStreamTab.h>
#include <Havana3/QResultTab.h>
#include <Havana3/QOperationTab.h>
#include <Havana3/QDeviceControlTab.h>

#include <Havana3/Dialog/FlimCalibDlg.h>

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/FLImProcess/FLImProcess.h>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Initialize user interface
    QString windowTitle("Havana3 [%1] v%2");
    this->setWindowTitle(windowTitle.arg("Clinical FLIm-OCT").arg(VERSION));

    // Create configuration object
    m_pConfiguration = new Configuration;
    m_pConfiguration->getConfigFile("Havana3.ini");

    m_pConfiguration->flimScans = FLIM_SCANS;
    m_pConfiguration->flimAlines = FLIM_ALINES;
    m_pConfiguration->octScans = OCT_SCANS;
    m_pConfiguration->octAlines = OCT_ALINES;

    // Set timer for renew configuration
    m_pTimer = new QTimer(this);
    m_pTimer->start(5 * 60 * 1000); // renew per 5 min
	m_pTimerDiagnostic = new QTimer(this);
	m_pTimerDiagnostic->start(2000);
	m_pTimerSync = new QTimer(this);
	m_pTimerSync->start(1000); // renew per 1 sec

    // Create tabs objects
    m_pStreamTab = new QStreamTab(this);
    m_pResultTab = new QResultTab(this);

    // Create group boxes and tab widgets
    m_pTabWidget = new QTabWidget(this);
    m_pTabWidget->addTab(m_pStreamTab, tr("Real-Time Data Streaming"));
    m_pTabWidget->addTab(m_pResultTab, tr("Image Post-Processing"));
	
    // Create status bar
	m_pStatusLabel_FlimDiagnostic = new QLabel(QString::fromLocal8Bit("[FLIm Laser] Diode Current: %1A (%2A) / Diode Temp.: %3¢ªC (%4¢ªC) / Chipset Temp.: %5¢ªC (%6¢ªC)")
		.arg(0.0, 1, 'f', 1).arg(0.0, 1, 'f', 1).arg(0.0, 2, 'f', 1).arg(0.0, 2, 'f', 1).arg(0.0, 2, 'f', 1).arg(0.0, 2, 'f', 1), this);
	m_pStatusLabel_OctDiagnostic = new QLabel(QString::fromLocal8Bit("[Axsun OCT] Source Current: %1A / TEC Temp.: %2¢ªC / Board Temp.: %3¢ªC")
		.arg(0.00, 0, 'f', 2).arg(10.0, 2, 'f', 1).arg(10.0, 2, 'f', 1));
    m_pStatusLabel_ImagePos = new QLabel(QString("(%1, %2)").arg(0000, 4).arg(0000, 4), this);
	
	size_t fp_bfn = m_pStreamTab->getFlimProcessingBufferQueueSize();
	size_t fv_bfn = m_pStreamTab->getFlimVisualizationBufferQueueSize();
	size_t ov_bfn = m_pStreamTab->getOctVisualizationBufferQueueSize();
	m_pStatusLabel_SyncStatus = new QLabel(QString("[Sync] FP#: %1 / FV#: %2 / OV#: %3").arg(fp_bfn, 3).arg(fv_bfn, 3).arg(ov_bfn, 3), this);

	m_pStatusLabel_FlimDiagnostic->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	m_pStatusLabel_OctDiagnostic->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_pStatusLabel_ImagePos->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	m_pStatusLabel_SyncStatus->setFrameStyle(QFrame::Panel | QFrame::Sunken);

    // then add the widget to the status bar
    statusBar()->addPermanentWidget(m_pStatusLabel_FlimDiagnostic, 42);
	statusBar()->addPermanentWidget(m_pStatusLabel_OctDiagnostic, 32);
    statusBar()->addPermanentWidget(m_pStatusLabel_ImagePos, 11);
    statusBar()->addPermanentWidget(m_pStatusLabel_SyncStatus, 15);
	statusBar()->setSizeGripEnabled(false);

    // Set layout
    m_pGridLayout = new QGridLayout;
    m_pGridLayout->addWidget(m_pTabWidget, 0, 0);

    ui->centralWidget->setLayout(m_pGridLayout);

    this->setMinimumSize(1280, 994);

    // Connect signal and slot
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(onTimer()));
	connect(m_pTimerDiagnostic, SIGNAL(timeout()), this, SLOT(onTimerDiagnostic()));
	connect(m_pTimerSync, SIGNAL(timeout()), this, SLOT(onTimerSync()));
    connect(m_pTabWidget, SIGNAL(currentChanged(int)), this, SLOT(changedTab(int)));
}

MainWindow::~MainWindow()
{
	m_pTimer->stop();
	m_pTimerDiagnostic->stop();
	m_pTimerSync->stop();

    if (m_pConfiguration)
    {
        m_pConfiguration->setConfigFile("Havana3.ini");
        delete m_pConfiguration;
    }

    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *e)
{
	if (m_pStreamTab->getOperationTab()->getAcquisitionButton()->isChecked())
		m_pStreamTab->getOperationTab()->setAcquisitionButton(false);

    e->accept();
}

void MainWindow::updateFlimLaserState(double* state)
{
	m_pStatusLabel_FlimDiagnostic->setText(QString::fromLocal8Bit("[FLIm Laser] Diode Current: %1A (%2A) / Diode Temp.: %3¢ªC (%4¢ªC) / Chipset Temp.: %5¢ªC (%6¢ªC)")
		.arg(state[1], 0, 'f', 1).arg(state[0], 0, 'f', 1).arg(state[3], 0, 'f', 1).arg(state[2], 0, 'f', 1).arg(state[5], 0, 'f', 1).arg(state[4], 0, 'f', 1));
}


void MainWindow::onTimer()
{
    // Current Time should be added here.
    QDate date = QDate::currentDate();
    QTime time = QTime::currentTime();
    QString current = QString("%1-%2-%3 %4-%5-%6")
        .arg(date.year()).arg(date.month(), 2, 10, (QChar)'0').arg(date.day(), 2, 10, (QChar)'0')
        .arg(time.hour(), 2, 10, (QChar)'0').arg(time.minute(), 2, 10, (QChar)'0').arg(time.second(), 2, 10, (QChar)'0');
	
	m_pConfiguration->msgHandle(QString("[%1] Configuration data has been renewed!").arg(current).toLocal8Bit().data());

	m_pStreamTab->getOperationTab()->getDataAcq()->getFLIm()->saveMaskData();
    m_pConfiguration->setConfigFile("Havana3.ini");
}

void MainWindow::onTimerDiagnostic()
{
	m_pStreamTab->getDeviceControlTab()->sendLaserCommand((char*)"?");
	///m_pStreamTab->getDeviceControlTab()->requestOctStatus();
}

void MainWindow::onTimerSync()
{
	size_t fp_bfn = m_pStreamTab->getFlimProcessingBufferQueueSize();
	size_t fv_bfn = m_pStreamTab->getFlimVisualizationBufferQueueSize();
	size_t ov_bfn = m_pStreamTab->getOctVisualizationBufferQueueSize();
	m_pStatusLabel_SyncStatus->setText(QString("[Sync] FP#: %1 / FV#: %2 / OV#: %3").arg(fp_bfn, 3).arg(fv_bfn, 3).arg(ov_bfn, 3));

	if ((fp_bfn < PROCESSING_BUFFER_SIZE - 5) || (fv_bfn < PROCESSING_BUFFER_SIZE - 5) || (ov_bfn < PROCESSING_BUFFER_SIZE - 5))
	{
		emit m_pStreamTab->sendStatusMessage("Synchronization failed. Please re-start the acquisition.", true);
		m_pStreamTab->getOperationTab()->setAcquisitionButton(false);
	}
}

void MainWindow::changedTab(int index)
{
	m_pResultTab->changeTab(index == 1);
	m_pStreamTab->changeTab(index == 0);
}
