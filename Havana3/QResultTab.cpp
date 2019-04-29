
#include "QResultTab.h"

#include <Havana3/MainWindow.h>
#include <Havana3/QProcessingTab.h>
#include <Havana3/QVisualizationTab.h>

#include <Havana3/Dialog/SaveResultDlg.h>
#include <Havana3/Dialog/PulseReviewDlg.h>
#include <Havana3/Dialog/LongitudinalViewDlg.h>


QResultTab::QResultTab(QWidget *parent) :
	QDialog(parent)
{
	// Set main window objects
	m_pMainWnd = dynamic_cast<MainWindow*>(parent);
	m_pConfig = m_pMainWnd->m_pConfiguration;

	// Create post-processing objects
	m_pProcessingTab = new QProcessingTab(this);
	m_pVisualizationTab = new QVisualizationTab(false, this);

	// Create group boxes for streaming objects
	m_pGroupBox_ProcessingTab = new QGroupBox();
	m_pGroupBox_ProcessingTab->setLayout(m_pProcessingTab->getLayout());
	m_pGroupBox_ProcessingTab->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	m_pGroupBox_ProcessingTab->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

	m_pGroupBox_VisualizationTab = new QGroupBox();
	m_pGroupBox_VisualizationTab->setLayout(m_pVisualizationTab->getLayout());
	m_pGroupBox_VisualizationTab->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	m_pGroupBox_VisualizationTab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);


	// Create layout
	QGridLayout* pGridLayout = new QGridLayout;
	pGridLayout->setSpacing(2);

	// Set layout    
	pGridLayout->addWidget(m_pGroupBox_VisualizationTab, 0, 0, 3, 1);
	pGridLayout->addWidget(m_pGroupBox_ProcessingTab, 0, 1);
	pGridLayout->addWidget(m_pVisualizationTab->getVisualizationWidgetsBox(), 1, 1);
	pGridLayout->addWidget(m_pVisualizationTab->getEnFaceWidgetsBox(), 2, 1);

	this->setLayout(pGridLayout);
}

QResultTab::~QResultTab()
{
}


void QResultTab::keyPressEvent(QKeyEvent *e)
{
	if (e->key() != Qt::Key_Escape)
		QDialog::keyPressEvent(e);
}


void QResultTab::changeTab(bool status)
{
	if (status) // stream -> result
		m_pVisualizationTab->setWidgetsValue();
	else // result -> stream
		m_pProcessingTab->setWidgetsStatus();

	if (m_pProcessingTab->getSaveResultDlg()) m_pProcessingTab->getSaveResultDlg()->close();
	if (m_pVisualizationTab->getPulseReviewDlg()) m_pVisualizationTab->getPulseReviewDlg()->close();
	if (m_pVisualizationTab->getLongitudinalViewDlg()) m_pVisualizationTab->getLongitudinalViewDlg()->close();
}
