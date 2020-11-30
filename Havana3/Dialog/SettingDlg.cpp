
#include "SettingDlg.h"

#include <Havana3/MainWindow.h>
#include <Havana3/QStreamTab.h>
#include <Havana3/QResultTab.h>
#include <Havana3/QViewTab.h>

#include <Havana3/Dialog/ViewOptionTab.h>
#include <Havana3/Dialog/DeviceOptionTab.h>
#include <Havana3/Dialog/FlimCalibTab.h>
#include <Havana3/Dialog/PulseReviewTab.h>


SettingDlg::SettingDlg(bool is_streaming, QWidget *parent) :
    QDialog(parent)
{
    // Set default size & frame
    setFixedSize(650, 350);
    setWindowFlags(Qt::Tool);
    setWindowTitle("Setting");

    // Set main window objects
    if (is_streaming)
    {
        m_pStreamTab = dynamic_cast<QStreamTab*>(parent);
        m_pConfig = m_pStreamTab->getMainWnd()->m_pConfiguration;
    }
    else
    {
        m_pResultTab = (QResultTab*)parent;
        m_pConfig = m_pResultTab->getMainWnd()->m_pConfiguration;
    }

    // Create option tabs
    m_pViewOptionTab = new ViewOptionTab(is_streaming, this);
    m_pDeviceOptionTab = new DeviceOptionTab(is_streaming, this);
    if (is_streaming) m_pFlimCalibTab = new FlimCalibTab(this);

	// Create widgets
    m_pTabWidget_Setting = new QTabWidget(this);
    m_pTabWidget_Setting->setTabPosition(QTabWidget::West);
    m_pTabWidget_Setting->tabBar()->setStyle(new CustomTabStyle);
    m_pTabWidget_Setting->tabBar()->setStyleSheet("QTabBar::tab { background-color: #2a2a2a; color: #808080 }"
                                                  "QTabBar::tab::selected { background-color: #424242; color: #ffffff }");

    m_pTabWidget_Setting->addTab(m_pViewOptionTab->getLayoutBox(), "Visualization");
    m_pTabWidget_Setting->addTab(m_pDeviceOptionTab->getHelicalScanLayoutBox(), "Motor Control");
    m_pTabWidget_Setting->addTab(m_pDeviceOptionTab->getFlimSystemLayoutBox(), "FLIm System");
    m_pTabWidget_Setting->addTab(m_pDeviceOptionTab->getAxsunOCTSystemLayoutBox(), "Axsun OCT System");
    if (is_streaming) m_pTabWidget_Setting->addTab(m_pFlimCalibTab->getLayoutBox(), "FLIm Calibration");
    if (!is_streaming) m_pTabWidget_Setting->addTab(new QWidget(this), "FLIm Pulse Review");
    m_pTabWidget_Setting->addTab(new QWidget(this), "Log");

    m_pPushButton_Ok = new QPushButton(this);
    m_pPushButton_Ok->setText("OK");

    m_pPushButton_Cancel = new QPushButton(this);
    m_pPushButton_Cancel->setText("Cancel");

	// Create layout
    QVBoxLayout *pVBoxLayout = new QVBoxLayout;
    pVBoxLayout->setSpacing(5);

    QHBoxLayout* pHBoxLayout_Buttons = new QHBoxLayout;
    pHBoxLayout_Buttons->setSpacing(3);
    pHBoxLayout_Buttons->setAlignment(Qt::AlignRight);

    pHBoxLayout_Buttons->addWidget(m_pPushButton_Ok);
    pHBoxLayout_Buttons->addWidget(m_pPushButton_Cancel);

    pVBoxLayout->addWidget(m_pTabWidget_Setting);
    pVBoxLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
    pVBoxLayout->addItem(pHBoxLayout_Buttons);

	// Set layout
    this->setLayout(pVBoxLayout);

    // Connect

}

SettingDlg::~SettingDlg()
{
}

void SettingDlg::keyPressEvent(QKeyEvent *e)
{
	if (e->key() != Qt::Key_Escape)
		QDialog::keyPressEvent(e);
}

