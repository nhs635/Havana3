
#include "PulseReviewTab.h"

#include <Havana3/MainWindow.h>
#include <Havana3/QResultTab.h>
#include <Havana3/QViewTab.h>

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/DataProcessing.h>
#include <DataAcquisition/FLImProcess/FLImProcess.h>


PulseReviewTab::PulseReviewTab(QWidget *parent) :
    QDialog(parent), m_pConfigTemp(nullptr), m_pResultTab(nullptr), m_pViewTab(nullptr), m_pFLIm(nullptr)
{
	// Set configuration objects
	m_pResultTab = dynamic_cast<QResultTab*>(parent);
	m_pViewTab = m_pResultTab->getViewTab();
	m_pConfigTemp = m_pResultTab->getDataProcessing()->getConfigTemp();
	m_pFLIm = m_pViewTab->getResultTab()->getDataProcessing()->getFLImProcess();

	// Create layout
	m_pGridLayout = new QGridLayout;
	m_pGridLayout->setSpacing(15);
	setWindowTitle("Pulse Review");

	// Create widgets
	m_pScope_PulseView = new QScope({ 0, (double)m_pFLIm->_resize.crop_src.size(0) }, 
									{ -m_pFLIm->_params.bg, POWER_2(16) - m_pFLIm->_params.bg },
									2, 2, 1, 1.2 / (double)POWER_2(16), 0, 0, "", " V", false);
	m_pScope_PulseView->setFixedWidth(420);
	m_pScope_PulseView->setMinimumHeight(200);
	m_pScope_PulseView->getRender()->setGrid(8, 32, 1, true);
	m_pScope_PulseView->setDcLine(m_pFLIm->_params.bg);
	
	m_pLabel_CurrentAline = new QLabel(this);
	QString str; str.sprintf("Current A-line : %4d / %4d (%3.2f deg)  ", 4 * (0 + 1), m_pFLIm->_resize.ny * 4,
        360.0 * (double)(4 * 0) / (double)m_pConfigTemp->octAlines);
	m_pLabel_CurrentAline->setText(str);
	m_pLabel_CurrentAline->setFixedWidth(250);
	
	m_pSlider_CurrentAline = new QSlider(this);
	m_pSlider_CurrentAline->setOrientation(Qt::Horizontal);
	m_pSlider_CurrentAline->setRange(0, m_pFLIm->_resize.ny - 1);
	m_pSlider_CurrentAline->setValue(0);

	m_pLabel_PulseType = new QLabel(this);
	m_pLabel_PulseType->setText("Pulse Type");

	m_pComboBox_PulseType = new QComboBox(this);
	m_pComboBox_PulseType->addItem("Crop");
	m_pComboBox_PulseType->addItem("Mask");

	// Set layout	
	QGridLayout *pGridLayout = new QGridLayout;
	pGridLayout->setSpacing(2);

	pGridLayout->addWidget(m_pScope_PulseView, 0, 0, 1, 3);

	pGridLayout->addWidget(m_pLabel_PulseType, 1, 1);
	pGridLayout->addWidget(m_pComboBox_PulseType, 1, 2);
	pGridLayout->addWidget(m_pLabel_CurrentAline, 2, 0);
	pGridLayout->addWidget(m_pSlider_CurrentAline, 2, 1, 1, 2);
	
	m_pGridLayout->addItem(pGridLayout, 0, 0);

	m_pGroupBox_PulseReview = new QGroupBox(this);
	m_pGroupBox_PulseReview->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	m_pGroupBox_PulseReview->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	m_pGroupBox_PulseReview->setLayout(m_pGridLayout);

	// Connect
	connect(m_pSlider_CurrentAline, SIGNAL(valueChanged(int)), this, SLOT(drawPulse(int)));
	connect(m_pComboBox_PulseType, SIGNAL(currentIndexChanged(int)), this, SLOT(changeType()));

	drawPulse(int(m_pViewTab->getCurrentAline() / 4));
}

PulseReviewTab::~PulseReviewTab()
{
}


void PulseReviewTab::drawPulse(int aline)
{
    auto pVector = (m_pComboBox_PulseType->currentIndex() == 0) ? &m_pViewTab->m_vectorPulseCrop : &m_pViewTab->m_vectorPulseMask;
	
	QString str; str.sprintf("Current A-line : %4d / %4d (%3.2f deg)  ", 4 * (aline + 1), m_pFLIm->_resize.ny * 4,
        360.0 * (double)(4 * aline) / (double)m_pConfigTemp->octAlines);
	m_pLabel_CurrentAline->setText(str);

    m_pScope_PulseView->drawData(&pVector->at(m_pViewTab->getCurrentFrame())(0, aline));
	
//    if (m_pViewTab->getRectImageView()->isVisible())
//	{
//        m_pViewTab->getRectImageView()->setVerticalLine(1, 4 * aline);
//        m_pViewTab->getRectImageView()->getRender()->update();
//	}
//	else
	{
        m_pViewTab->getCircImageView()->setVerticalLine(1, 4 * aline);
        m_pViewTab->getCircImageView()->getRender()->update();
	}
}

void PulseReviewTab::changeType()
{
	drawPulse(m_pSlider_CurrentAline->value());
}
