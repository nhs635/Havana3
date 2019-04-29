
#include "PulseReviewDlg.h"

#include <Havana3/MainWindow.h>
#include <Havana3/QResultTab.h>
#include <Havana3/QProcessingTab.h>
#include <Havana3/QVisualizationTab.h>

#include <Havana3/Viewer/QImageView.h>

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/FLImProcess/FLImProcess.h>


PulseReviewDlg::PulseReviewDlg(QWidget *parent) :
    QDialog(parent)
{
    // Set default size & frame
    setFixedSize(525, 250);
    setWindowFlags(Qt::Tool);
	setWindowTitle("Pulse Review");

    // Set main window objects
    m_pVisualizationTab = (QVisualizationTab*)parent;
	m_pConfig = m_pVisualizationTab->getResultTab()->getMainWnd()->m_pConfiguration;
	m_pFLIm = m_pVisualizationTab->getResultTab()->getProcessingTab()->getFLImProcess();

	// Create widgets
	m_pScope_PulseView = new QScope({ 0, (double)m_pFLIm->_resize.crop_src.size(0) }, 
		{ -(double)(int)m_pFLIm->_params.bg[1], (double)(int)(POWER_2(16) - m_pFLIm->_params.bg[1]), }, 2, 2, 1, 1, 0, 0, "", "", false);
	m_pScope_PulseView->setMinimumHeight(200);
	m_pScope_PulseView->getRender()->setGrid(8, 32, 1, true);
	
	m_pLabel_CurrentAline = new QLabel(this);
	QString str; str.sprintf("Current A-line : %4d / %4d (%3.2f deg)  ", 4 * (0 + 1), m_pFLIm->_resize.ny * 4,
		360.0 * (double)(4 * 0) / (double)m_pVisualizationTab->getResultTab()->getProcessingTab()->getConfigTemp()->octAlines);
	m_pLabel_CurrentAline->setText(str);
	
	m_pSlider_CurrentAline = new QSlider(this);
	m_pSlider_CurrentAline->setOrientation(Qt::Horizontal);
	m_pSlider_CurrentAline->setRange(0, m_pFLIm->_resize.ny - 1);
	m_pSlider_CurrentAline->setValue(0);

	m_pLabel_PulseType = new QLabel(this);
	m_pLabel_PulseType->setText("Pulse Type");

	m_pComboBox_PulseType = new QComboBox(this);
	m_pComboBox_PulseType->addItem("Crop");
	m_pComboBox_PulseType->addItem("Mask");

	// Create layout
	QGridLayout *pGridLayout = new QGridLayout;
	
	pGridLayout->addWidget(m_pScope_PulseView, 0, 0, 1, 4);

	pGridLayout->addWidget(m_pLabel_CurrentAline, 1, 0);
	pGridLayout->addWidget(m_pSlider_CurrentAline, 1, 1);
	pGridLayout->addWidget(m_pLabel_PulseType, 1, 2);
	pGridLayout->addWidget(m_pComboBox_PulseType, 1, 3);

	// Set layout
	this->setLayout(pGridLayout);

	// Connect
	connect(m_pSlider_CurrentAline, SIGNAL(valueChanged(int)), this, SLOT(drawPulse(int)));
	connect(m_pComboBox_PulseType, SIGNAL(currentIndexChanged(int)), this, SLOT(changeType()));
	drawPulse(0);
}

PulseReviewDlg::~PulseReviewDlg()
{
}

void PulseReviewDlg::keyPressEvent(QKeyEvent *e)
{
	if (e->key() != Qt::Key_Escape)
		QDialog::keyPressEvent(e);
}


void PulseReviewDlg::drawPulse(int aline)
{
	auto pVector = (m_pComboBox_PulseType->currentIndex() == 0) ? &m_pVisualizationTab->m_vectorPulseCrop : &m_pVisualizationTab->m_vectorPulseMask;
	
	QString str; str.sprintf("Current A-line : %4d / %4d (%3.2f deg)  ", 4 * (aline + 1), m_pFLIm->_resize.ny * 4, 
		360.0 * (double)(4 * aline) / (double)m_pVisualizationTab->getResultTab()->getProcessingTab()->getConfigTemp()->octAlines);
	m_pLabel_CurrentAline->setText(str);

	m_pScope_PulseView->drawData(&pVector->at(m_pVisualizationTab->getCurrentFrame())(0, aline));
	
	if (m_pVisualizationTab->getRectImageView()->isVisible())
	{
		m_pVisualizationTab->getRectImageView()->setVerticalLine(1, 4 * aline);
		m_pVisualizationTab->getRectImageView()->getRender()->update();
	}
	else
	{
		m_pVisualizationTab->getCircImageView()->setVerticalLine(1, 4 * aline);
		m_pVisualizationTab->getCircImageView()->getRender()->update();
	}
}

void PulseReviewDlg::changeType()
{
	drawPulse(m_pSlider_CurrentAline->value());
}