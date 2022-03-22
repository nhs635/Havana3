
#include "SettingDlg.h"

#include <Havana3/MainWindow.h>
#include <Havana3/QPatientSummaryTab.h>
#include <Havana3/QStreamTab.h>
#include <Havana3/QResultTab.h>
#include <Havana3/QViewTab.h>

#include <Havana3/Dialog/ViewOptionTab.h>
#include <Havana3/Dialog/DeviceOptionTab.h>
#include <Havana3/Dialog/FlimCalibTab.h>
#include <Havana3/Dialog/PulseReviewTab.h>

#include <DeviceControl/DeviceControl.h>


SettingDlg::SettingDlg(QWidget *parent) :
    QDialog(parent), m_pPatientSummaryTab(nullptr), m_pStreamTab(nullptr), m_pResultTab(nullptr), m_pViewTab(nullptr), 
	m_pViewOptionTab(nullptr), m_pDeviceOptionTab(nullptr), m_pFlimCalibTab(nullptr), m_pPulseReviewTab(nullptr)
{
    // Set default size & frame
    setFixedWidth(600);
	setMinimumHeight(500);
    setWindowFlags(Qt::Tool);
    setWindowTitle("Setting");

    // Set main window objects
	QString parent_name, parent_title = parent->windowTitle();
	if (parent_title.contains("Summary"))
	{
		parent_name = "Summary";
		m_pPatientSummaryTab = dynamic_cast<QPatientSummaryTab*>(parent);
		m_pConfig = m_pPatientSummaryTab->getMainWnd()->m_pConfiguration;

		m_pDeviceOptionTab = new DeviceOptionTab(parent);
	}
	else if (parent_title.contains("Streaming"))
    {
		parent_name = "Streaming";
        m_pStreamTab = dynamic_cast<QStreamTab*>(parent);
        m_pConfig = m_pStreamTab->getMainWnd()->m_pConfiguration;

		m_pViewOptionTab = new ViewOptionTab(parent);
		m_pDeviceOptionTab = new DeviceOptionTab(parent);
		m_pFlimCalibTab = new FlimCalibTab(this);
    }
    else if (parent_title.contains("Review"))
    {
		parent_name = "Review";
        m_pResultTab = (QResultTab*)parent;
        m_pConfig = m_pResultTab->getMainWnd()->m_pConfiguration;

		m_pViewOptionTab = new ViewOptionTab(parent);
		m_pDeviceOptionTab = new DeviceOptionTab(parent);
		m_pPulseReviewTab = new PulseReviewTab(parent);
    }
	
	// Create widgets
    m_pTabWidget_Setting = new QTabWidget(this);
    m_pTabWidget_Setting->setTabPosition(QTabWidget::West);
	m_pCusomTabstyle = new CustomTabStyle;
    m_pTabWidget_Setting->tabBar()->setStyle(m_pCusomTabstyle);
	m_pTabWidget_Setting->tabBar()->setFixedWidth(120);
    m_pTabWidget_Setting->tabBar()->setStyleSheet("QTabBar::tab { background-color: #2a2a2a; color: #808080 }"
                                                  "QTabBar::tab::selected { background-color: #424242; color: #ffffff }");

	// Visualization option tab
	if (parent_name != "Summary")
	{
		QScrollArea *pScrollArea_Visualization = new QScrollArea;
		pScrollArea_Visualization->setWidget(m_pViewOptionTab->getLayoutBox());
		m_pTabWidget_Setting->addTab(pScrollArea_Visualization, " Visualization ");
	}

	// System device tab
	QScrollArea *pScrollArea_SystemDevice = new QScrollArea;
	pScrollArea_SystemDevice->setWidget(m_pDeviceOptionTab->getLayoutBox());
	m_pTabWidget_Setting->addTab(pScrollArea_SystemDevice, " System Device ");
	
	// FLIm calibration tab
	if (parent_name == "Streaming")
	{
		QScrollArea *pScrollArea_FlimCalibration = new QScrollArea;
		pScrollArea_FlimCalibration->setWidget(m_pFlimCalibTab->getLayoutBox());
		m_pTabWidget_Setting->addTab(pScrollArea_FlimCalibration, "FLIm Calibration");
	}

	// Pulse review tab
	if (parent_name == "Review")
	{
		QScrollArea *pScrollArea_PulseReview = new QScrollArea;
		pScrollArea_PulseReview->setWidget(m_pPulseReviewTab->getLayoutBox());
		m_pTabWidget_Setting->addTab(pScrollArea_PulseReview, "    Analysis    ");
				
		if (QApplication::desktop()->screen()->rect().width() == 1280)
		{
			this->move(m_pResultTab->getMainWnd()->x() + m_pResultTab->getMainWnd()->width() - 600,
				m_pResultTab->getMainWnd()->y() + m_pResultTab->getMainWnd()->height() - 500);
		}
		else
		{
			setMinimumHeight(765);
			this->move(m_pResultTab->getMainWnd()->x() + m_pResultTab->getMainWnd()->width() - 25,
				m_pResultTab->getMainWnd()->y() + m_pResultTab->getMainWnd()->height() - 765);
		}
	}

	// Log tab
	m_pListWidget_Log = new QListWidget(this);
	m_pListWidget_Log->setStyleSheet("QListWidget { font: 8pt; }");
	for (int i = 0; i < m_pConfig->log.size(); i++)
		m_pListWidget_Log->addItem(m_pConfig->log.at(i));
	m_pListWidget_Log->setCurrentRow(m_pListWidget_Log->count() - 1);
	connect(this, &SettingDlg::addLog, [&](const QString& msg) { m_pListWidget_Log->addItem(msg); });

	m_pConfig->DidLogAdded += [&](const QString& msg) {
		emit addLog(msg);
		m_pListWidget_Log->setCurrentRow(m_pListWidget_Log->count() - 1);
	};

	m_pTabWidget_Setting->addTab(m_pListWidget_Log, "Log");
	   
	// Close button
    m_pPushButton_Close = new QPushButton(this);
	m_pPushButton_Close->setText("Close");
	

	// Create layout
    QVBoxLayout *pVBoxLayout = new QVBoxLayout;
    pVBoxLayout->setSpacing(5);

    QHBoxLayout* pHBoxLayout_Buttons = new QHBoxLayout;
    pHBoxLayout_Buttons->setSpacing(3);
    pHBoxLayout_Buttons->setAlignment(Qt::AlignRight);

    pHBoxLayout_Buttons->addWidget(m_pPushButton_Close);

    pVBoxLayout->addWidget(m_pTabWidget_Setting);
    pVBoxLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
    pVBoxLayout->addItem(pHBoxLayout_Buttons);

	// Set layout
    this->setLayout(pVBoxLayout);

    // Connect
	connect(m_pPushButton_Close, SIGNAL(clicked(bool)), this, SLOT(accept()));
}

SettingDlg::~SettingDlg()
{
	if (m_pDeviceOptionTab) /// && !m_pFlimCalibTab)
	{
		if (!m_pFlimCalibTab)
		{
			m_pDeviceOptionTab->getDeviceControl()->turnOffAllDevices();
			if (!m_pDeviceOptionTab->getStreamTab())
			{
				m_pDeviceOptionTab->getDeviceControl()->disconnectAllDevices();
				delete m_pDeviceOptionTab->getDeviceControl();
			}
		}
	}

	delete m_pCusomTabstyle;
	m_pConfig->DidLogAdded.clear();
}

void SettingDlg::keyPressEvent(QKeyEvent *e)
{
	if (e->key() != Qt::Key_Escape)
		QDialog::keyPressEvent(e);
}

