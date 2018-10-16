
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
    QLabel *pStatusLabel_Temp1 = new QLabel(this); // System Message?
    m_pStatusLabel_ImagePos = new QLabel(QString("(%1, %2)").arg(0000, 4).arg(0000, 4), this);
	
	size_t fp_bfn = m_pStreamTab->getFlimProcessingBufferQueueSize();
	size_t fv_bfn = m_pStreamTab->getFlimVisualizationBufferQueueSize();
	size_t ov_bfn = m_pStreamTab->getOctVisualizationBufferQueueSize();
	m_pStatusLabel_SyncStatus = new QLabel(QString("FP bufn: %1 / FV bufn: %2 / OV bufn: %3").arg(fp_bfn, 3).arg(fv_bfn, 3).arg(ov_bfn, 3), this);

    pStatusLabel_Temp1->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_pStatusLabel_ImagePos->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	m_pStatusLabel_SyncStatus->setFrameStyle(QFrame::Panel | QFrame::Sunken);

    // then add the widget to the status bar
    statusBar()->addPermanentWidget(pStatusLabel_Temp1, 6);
    statusBar()->addPermanentWidget(m_pStatusLabel_ImagePos, 1);
    statusBar()->addPermanentWidget(m_pStatusLabel_SyncStatus, 2);

    // Set layout
    m_pGridLayout = new QGridLayout;
    m_pGridLayout->addWidget(m_pTabWidget, 0, 0);

    ui->centralWidget->setLayout(m_pGridLayout);

    this->setFixedSize(1280, 994);

    // Connect signal and slot
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(onTimer()));
	connect(m_pTimerSync, SIGNAL(timeout()), this, SLOT(onTimerSync()));
    connect(m_pTabWidget, SIGNAL(currentChanged(int)), this, SLOT(changedTab(int)));
}

MainWindow::~MainWindow()
{
	m_pTimer->stop();

    if (m_pConfiguration)
    {
        m_pConfiguration->setConfigFile("Havana3.ini");
        delete m_pConfiguration;
    }

    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *e)
{
	if (m_pStreamTab->getOperationTab()->isAcquisitionButtonToggled())
		m_pStreamTab->getOperationTab()->setAcquisitionButton(false);

    e->accept();
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

void MainWindow::onTimerSync()
{
	size_t fp_bfn = m_pStreamTab->getFlimProcessingBufferQueueSize();
	size_t fv_bfn = m_pStreamTab->getFlimVisualizationBufferQueueSize();
	size_t ov_bfn = m_pStreamTab->getOctVisualizationBufferQueueSize();
	m_pStatusLabel_SyncStatus->setText(QString("FP bufn: %1 / FV bufn: %2 / OV bufn: %3").arg(fp_bfn, 3).arg(fv_bfn, 3).arg(ov_bfn, 3));

	//size_t fp_exec = m_pStreamTab->getFlimProcessingSync()->get_sync_queue_size();
	//size_t fv_exec = m_pStreamTab->getFlimVisualizationSync()->get_sync_queue_size();
	//size_t ov_exec = m_pStreamTab->getOctVisualizationSync()->get_sync_queue_size();
	//printf("%d %d %d\n", fp_exec, fv_exec, ov_exec);
}

void MainWindow::changedTab(int index)
{
	m_pResultTab->changeTab(index == 1);
	m_pStreamTab->changeTab(index == 0);
}
