
#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <Havana3/Configuration.h>
#include <Havana3/HvnSqlDataBase.h>

#include <Havana3/QHomeTab.h>
#include <Havana3/QPatientSelectionTab.h>
#include <Havana3/QPatientSummaryTab.h>

#include <Havana3/QStreamTab.h>
#include <Havana3/QResultTab.h>

#include <DataAcquisition/DataAcquisition.h>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), m_pStreamTab(nullptr),
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

    // Create sql database object
    m_pHvnSqlDataBase = new HvnSqlDataBase(this);

	// Create group boxes and tab widgets
	m_pTabWidget = new QTabWidget(this);
	m_pTabWidget->setTabsClosable(true);
	m_pTabWidget->setDocumentMode(true);

    // Create tabs objects
    m_pHomeTab = new QHomeTab(this);
    addTabView(m_pHomeTab);

    connect(m_pHomeTab, SIGNAL(signedIn()), this, SLOT(makePatientSelectionTab()));
	

    // Set layout
    m_pGridLayout = new QGridLayout;
    m_pGridLayout->addWidget(m_pTabWidget, 0, 0);

    ui->centralWidget->setLayout(m_pGridLayout);

    this->setMinimumSize(1280, 994);

    // Connect signal and slot
    connect(m_pTabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabCurrentChanged(int)));
    connect(m_pTabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(tabCloseRequested(int)));
	
	// Set timer for renew configuration		(여러 가지 타이머 관련 셋팅이 필요하지 않을까요.)
//	m_pTimer = new QTimer(this);
//	m_pTimer->start(5 * 60 * 1000); // renew per 5 min
//	m_pTimerDiagnostic = new QTimer(this);
//	m_pTimerDiagnostic->start(2000);
//	m_pTimerSync = new QTimer(this);
//	m_pTimerSync->start(1000); // renew per 1 sec

//    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(onTimer()));
//	connect(m_pTimerDiagnostic, SIGNAL(timeout()), this, SLOT(onTimerDiagnostic()));
//	connect(m_pTimerSync, SIGNAL(timeout()), this, SLOT(onTimerSync()));
}

MainWindow::~MainWindow()
{
//	m_pTimer->stop();
//	m_pTimerDiagnostic->stop();
//	m_pTimerSync->stop();

    if (m_pConfiguration)
    {
        m_pConfiguration->setConfigFile("Havana3.ini");
        delete m_pConfiguration;
    }

    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *e)
{
	if (m_pStreamTab)
		if (m_pStreamTab->getDataAcquisition()->getAcquisitionState())
			m_pStreamTab->startLiveImaging(false);

    e->accept();
}


void MainWindow::addTabView(QDialog *pTabView)
{
    m_vectorTabViews.push_back(pTabView);

    int tabIndex = m_pTabWidget->addTab(pTabView, pTabView->windowTitle());
    m_pTabWidget->setCurrentIndex(tabIndex);

	m_pConfiguration->writeToLog(QString("Tab added: %1").arg(pTabView->windowTitle()));
}

void MainWindow::removeTabView(QDialog *pTabView)
{
	m_pConfiguration->writeToLog(QString("Tab removed: %1").arg(pTabView->windowTitle()));

	if (pTabView != m_pStreamTab)
		pTabView->deleteLater();

    m_pTabWidget->removeTab(m_pTabWidget->indexOf(pTabView));
	m_vectorTabViews.erase(std::find(m_vectorTabViews.begin(), m_vectorTabViews.end(), pTabView));

}


void MainWindow::tabCurrentChanged(int index)
{
	static int prev_index = 0;

	auto previousTab = m_vectorTabViews.at(prev_index);
    auto currentTab = m_vectorTabViews.at(index);

	if (previousTab == m_pStreamTab)
	{
		if (m_pStreamTab->getDataAcquisition()->getAcquisitionState())
			m_pStreamTab->startLiveImaging(false);
	}
	else if (previousTab->windowTitle().contains("Review"))
	{
		dynamic_cast<QResultTab*>(previousTab)->updatePreviewImage();
	}

	if (currentTab == m_pPatientSelectionTab)
	{
		m_pPatientSelectionTab->loadPatientDatabase();
	}
	else if (currentTab->windowTitle().contains("Summary"))
	{
		dynamic_cast<QPatientSummaryTab*>(currentTab)->loadRecordDatabase();
	}
	else if (currentTab == m_pStreamTab)
	{		
		m_pStreamTab->startLiveImaging(true);
	
		if (!m_pStreamTab->getDataAcquisition()->getAcquisitionState())
			emit m_pTabWidget->tabCloseRequested(index);
	}
	
	prev_index = index;

	m_pConfiguration->writeToLog(QString("Tab changed: %1 ==> %2").arg(previousTab->windowTitle()).arg(currentTab->windowTitle()));
}

void MainWindow::tabCloseRequested(int index)
{
    auto *pTabView = m_vectorTabViews.at(index);

    if (index != 0) // HomeTab & SelectionTab cannot be closed.
    {
		if (pTabView->windowTitle().contains("Summary"))
		{
			QMessageBox MsgBox(QMessageBox::Information, "Confirm close", "All relevant tabs will be closed.");
			MsgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
			MsgBox.setDefaultButton(QMessageBox::Cancel);

			switch (MsgBox.exec())
			{
			case QMessageBox::Ok:				
				break;
			case QMessageBox::Cancel:
				return;
			default:
				return;
			}

			int _index = 0;
			QString patient_name = ((QPatientSummaryTab*)pTabView)->getPatientInfo().patientName;
			foreach(QDialog* _pTabView, m_vectorTabViews)
			{				
				QString window_title = _pTabView->windowTitle();
				if (window_title.contains(patient_name) && !window_title.contains("Summary"))
					tabCloseRequested(_index--);
				_index++;
			}
			m_pTabWidget->setCurrentIndex(0);
		}
		else if (pTabView->windowTitle().contains("Streaming"))
		{
			int _index = 0;
			foreach(QDialog* _pTabView, m_vectorTabViews)
			{
				if (_pTabView->windowTitle().contains("Summary"))
				{
					if (_pTabView->windowTitle().contains(((QStreamTab*)pTabView)->getRecordInfo().patientName))
					{
						m_pTabWidget->setCurrentIndex(_index);
						break;
					}
				}
				_index++;
			}
		}
		else if (pTabView->windowTitle().contains("Review"))
		{
			int _index = 0;
			foreach(QDialog* _pTabView, m_vectorTabViews)
			{
				if (_pTabView->windowTitle().contains("Summary"))
				{
					if (_pTabView->windowTitle().contains(((QResultTab*)pTabView)->getRecordInfo().patientName))
					{
						m_pTabWidget->setCurrentIndex(_index);
						break;
					}
				}
				_index++;
			}
		}

		removeTabView(pTabView);
    }
}


void MainWindow::makePatientSelectionTab()
{
    m_pPatientSelectionTab = new QPatientSelectionTab(this);
    addTabView(m_pPatientSelectionTab);
    removeTabView(m_pHomeTab);

    connect(m_pPatientSelectionTab->getTableWidgetPatientInformation(), SIGNAL(cellDoubleClicked(int, int)), this, SLOT(makePatientSummaryTab(int)));
}

void MainWindow::makePatientSummaryTab(int row)
{
    QString patient_id = m_pPatientSelectionTab->getTableWidgetPatientInformation()->item(row, 1)->text();

    foreach (QDialog* pTabView, m_vectorTabViews)
    {
        if (pTabView->windowTitle().contains("Summary"))
        {
            if (((QPatientSummaryTab*)pTabView)->getPatientInfo().patientId == patient_id)
            {
                m_pTabWidget->setCurrentWidget(pTabView);
                return;
            }
        }
    }

    QPatientSummaryTab *pPatientSummaryTab = new QPatientSummaryTab(patient_id, this);
    addTabView(pPatientSummaryTab);

    connect(pPatientSummaryTab, SIGNAL(requestNewRecord(const QString &)), this, SLOT(makeStreamTab(const QString &)));
    connect(pPatientSummaryTab, SIGNAL(requestReview(const QString &)), this, SLOT(makeResultTab(const QString &)));
}

void MainWindow::makeStreamTab(const QString& patient_id)
{
	if (!m_pStreamTab)
	{
		m_pStreamTab = new QStreamTab(patient_id, this);
		addTabView(m_pStreamTab);

		connect(m_pStreamTab, SIGNAL(requestReview(const QString &)), this, SLOT(makeResultTab(const QString &)));
	}
	else
	{
		m_pStreamTab->changePatient(patient_id);
		addTabView(m_pStreamTab);
	}
}

void MainWindow::makeResultTab(const QString& record_id)
{
	foreach(QDialog* pTabView, m_vectorTabViews)
	{
		if (pTabView->windowTitle().contains("Review"))
		{
			if (((QResultTab*)pTabView)->getRecordInfo().recordId == record_id)
			{
				m_pTabWidget->setCurrentWidget(pTabView);
				return;
			}
		}
	}

    QResultTab *pResultTab = new QResultTab(record_id, this);
    addTabView(pResultTab);
}
