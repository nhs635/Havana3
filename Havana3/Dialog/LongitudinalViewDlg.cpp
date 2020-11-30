
#include "LongitudinalViewDlg.h"

#include <Havana3/MainWindow.h>
#include <Havana3/QResultTab.h>
#include <Havana3/QProcessingTab.h>
#include <Havana3/QViewTab.h>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>


LongitudinalViewDlg::LongitudinalViewDlg(QWidget *parent) :
    QDialog(parent),
	m_pImgObjOctLongiImage(nullptr), m_pImgObjIntensity(nullptr), m_pImgObjLifetime(nullptr), m_pMedfilt(nullptr)
{
    // Set default size & frame
    setMinimumSize(700, 300);
    setWindowFlags(Qt::Tool);
	setWindowTitle("Longitudinal View");

    // Set main window objects
    m_pViewTab = (QViewTab*)parent;
    m_pProcessingTab = m_pViewTab->getResultTab()->getProcessingTab();
    m_pConfig = m_pViewTab->getResultTab()->getMainWnd()->m_pConfiguration;
	
	// Create image view buffers
	ColorTable temp_ctable;	
	m_pImgObjOctLongiImage = new ImageObject(((m_pProcessingTab->getConfigTemp()->frames + 3) >> 2) << 2, 2 * m_pProcessingTab->getConfigTemp()->octScans, temp_ctable.m_colorTableVector.at(OCT_COLORTABLE));
	
	m_pImgObjIntensity = new ImageObject(((m_pProcessingTab->getConfigTemp()->frames + 3) >> 2) << 2, 2 * RING_THICKNESS, temp_ctable.m_colorTableVector.at(INTENSITY_COLORTABLE));
	m_pImgObjLifetime = new ImageObject(((m_pProcessingTab->getConfigTemp()->frames + 3) >> 2) << 2, 2 * RING_THICKNESS, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE));
	m_pImgObjIntensityWeightedLifetime = new ImageObject(((m_pProcessingTab->getConfigTemp()->frames + 3) >> 2) << 2, 2 * RING_THICKNESS, temp_ctable.m_colorTableVector.at(LIFETIME_COLORTABLE));

	// Create medfilt
	m_pMedfilt = new medfilt(((m_pProcessingTab->getConfigTemp()->frames + 3) >> 2) << 2, 2 * m_pProcessingTab->getConfigTemp()->octScans, 3, 3); // median filtering kernel


	// Create widgets
	m_pImageView_LongitudinalView = new QImageView(ColorTable::colortable(OCT_COLORTABLE), m_pProcessingTab->getConfigTemp()->frames, 2 * m_pProcessingTab->getConfigTemp()->octScans, true);
	m_pImageView_LongitudinalView->setMinimumWidth(600);
	m_pImageView_LongitudinalView->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    m_pImageView_LongitudinalView->setVLineChangeCallback([&](int frame) { m_pViewTab->setCurrentFrame(frame); });
    m_pImageView_LongitudinalView->setVerticalLine(1, m_pViewTab->getCurrentFrame());
	m_pImageView_LongitudinalView->getRender()->update();

	m_pLabel_CurrentAline = new QLabel(this);
	m_pLabel_CurrentAline->setFixedWidth(250);
	QString str; str.sprintf("Current A-line : (%4d, %4d) / %4d (%3.2f deg)  ", 
		1, m_pProcessingTab->getConfigTemp()->octAlines / 2 + 1, m_pProcessingTab->getConfigTemp()->octAlines, 0);
	m_pLabel_CurrentAline->setText(str);

	m_pSlider_CurrentAline = new QSlider(this);
	m_pSlider_CurrentAline->setOrientation(Qt::Horizontal);
	m_pSlider_CurrentAline->setRange(0, m_pProcessingTab->getConfigTemp()->octAlines / 2 - 1);
	m_pSlider_CurrentAline->setValue(0);
	
	// Create layout
	QGridLayout *pGridLayout = new QGridLayout;
	pGridLayout->setSpacing(3);

	pGridLayout->addWidget(m_pImageView_LongitudinalView, 0, 0, 1, 2);
	pGridLayout->addWidget(m_pLabel_CurrentAline, 1, 0);
	pGridLayout->addWidget(m_pSlider_CurrentAline, 1, 1);

	// Set layout
	this->setLayout(pGridLayout);

	// Connect
	connect(m_pSlider_CurrentAline, SIGNAL(valueChanged(int)), this, SLOT(drawLongitudinalImage(int)));
	connect(this, SIGNAL(paintLongiImage(uint8_t*)), m_pImageView_LongitudinalView, SLOT(drawRgbImage(uint8_t*)));
}

LongitudinalViewDlg::~LongitudinalViewDlg()
{
	if (m_pImgObjOctLongiImage) delete m_pImgObjOctLongiImage;
	if (m_pImgObjIntensity) delete m_pImgObjIntensity;
	if (m_pImgObjLifetime) delete m_pImgObjLifetime;
	if (m_pImgObjIntensityWeightedLifetime) delete m_pImgObjIntensityWeightedLifetime;
	if (m_pMedfilt) delete m_pMedfilt;
}

void LongitudinalViewDlg::keyPressEvent(QKeyEvent *e)
{
	if (e->key() != Qt::Key_Escape)
		QDialog::keyPressEvent(e);
}


void LongitudinalViewDlg::setWidgets(bool enabled)
{
	m_pImageView_LongitudinalView->setEnabled(enabled);
	m_pLabel_CurrentAline->setEnabled(enabled);
	m_pSlider_CurrentAline->setEnabled(enabled);
}


void LongitudinalViewDlg::drawLongitudinalImage(int aline)
{	
	// Specified A line
    int aline0 = aline; // + m_pViewTab->getCurrentRotation();
	int aline1 = aline0 % (m_pProcessingTab->getConfigTemp()->octAlines / 2);

	// Make longitudinal - OCT
	IppiSize roi_longi = { m_pImgObjOctLongiImage->getHeight(), m_pImgObjOctLongiImage->getWidth() };
	
	np::FloatArray2 longi_temp(roi_longi.width, roi_longi.height);
	np::Uint8Array2 scale_temp(roi_longi.width, roi_longi.height);
	tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)(m_pProcessingTab->getConfigTemp()->frames)),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
		{
            memcpy(&scale_temp(0, (int)i), &m_pViewTab->m_vectorOctImage.at((int)i)(0, aline1), sizeof(uint8_t) * m_pProcessingTab->getConfigTemp()->octScans);
			memcpy(&scale_temp(m_pProcessingTab->getConfigTemp()->octScans, (int)i),
                &m_pViewTab->m_vectorOctImage.at((int)i)(0, m_pProcessingTab->getConfigTemp()->octAlines / 2 + aline1), sizeof(uint8_t) * m_pProcessingTab->getConfigTemp()->octScans);
			ippsFlip_8u_I(&scale_temp(0, (int)i), m_pProcessingTab->getConfigTemp()->octScans);
		}
	});

	ippsConvert_8u32f(scale_temp.raw_ptr(), longi_temp.raw_ptr(), scale_temp.length());
	ippiScale_32f8u_C1R(longi_temp.raw_ptr(), roi_longi.width * sizeof(float),
		scale_temp.raw_ptr(), roi_longi.width * sizeof(uint8_t), roi_longi, m_pConfig->octGrayRange.min, m_pConfig->octGrayRange.max);
	if (aline0 > (m_pProcessingTab->getConfigTemp()->octAlines / 2))
		ippiMirror_8u_C1IR(scale_temp.raw_ptr(), sizeof(uint8_t) * roi_longi.width, roi_longi, ippAxsVertical);
	ippiTranspose_8u_C1R(scale_temp.raw_ptr(), roi_longi.width * sizeof(uint8_t), m_pImgObjOctLongiImage->arr.raw_ptr(), roi_longi.height * sizeof(uint8_t), roi_longi);
	(*m_pMedfilt)(m_pImgObjOctLongiImage->arr.raw_ptr());
	
	m_pImgObjOctLongiImage->convertRgb();

	// Make longitudinal - FLIM
	tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)(m_pProcessingTab->getConfigTemp()->frames)),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
		{
            m_pImgObjIntensity->arr((int)i, 0) = m_pViewTab->m_pImgObjIntensityMap->arr(aline / 4, (int)(m_pProcessingTab->getConfigTemp()->frames - i - 1));
            m_pImgObjIntensity->arr((int)i, RING_THICKNESS) = m_pViewTab->m_pImgObjIntensityMap->arr(m_pProcessingTab->getConfigTemp()->flimAlines / 2 + aline / 4, (int)(m_pProcessingTab->getConfigTemp()->frames - i - 1));
            m_pImgObjLifetime->arr((int)i, 0) = m_pViewTab->m_pImgObjLifetimeMap->arr(aline / 4, (int)(m_pProcessingTab->getConfigTemp()->frames - i - 1));
            m_pImgObjLifetime->arr((int)i, RING_THICKNESS) = m_pViewTab->m_pImgObjLifetimeMap->arr(m_pProcessingTab->getConfigTemp()->flimAlines / 2 + aline / 4, (int)(m_pProcessingTab->getConfigTemp()->frames - i - 1));
		}
	});
	tbb::parallel_for(tbb::blocked_range<size_t>(1, (size_t)(RING_THICKNESS)),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
		{
			memcpy(&m_pImgObjIntensity->arr(0, (int)i), &m_pImgObjIntensity->arr(0, 0), sizeof(uint8_t) * m_pImgObjIntensity->arr.size(0));
			memcpy(&m_pImgObjIntensity->arr(0, (int)(i + RING_THICKNESS)), &m_pImgObjIntensity->arr(0, RING_THICKNESS), sizeof(uint8_t) * m_pImgObjIntensity->arr.size(0));
			memcpy(&m_pImgObjLifetime->arr(0, (int)i), &m_pImgObjLifetime->arr(0, 0), sizeof(uint8_t) * m_pImgObjLifetime->arr.size(0));
			memcpy(&m_pImgObjLifetime->arr(0, (int)(i + RING_THICKNESS)), &m_pImgObjLifetime->arr(0, RING_THICKNESS), sizeof(uint8_t) * m_pImgObjLifetime->arr.size(0));
		}
	});
	
//    if (!m_pViewTab->isIntensityWeighted())
	{
//        if (!m_pViewTab->isClassification())
		{
			m_pImgObjIntensity->convertNonScaledRgb();
			m_pImgObjLifetime->convertNonScaledRgb();

			memcpy(m_pImgObjOctLongiImage->qrgbimg.bits(), m_pImgObjLifetime->qrgbimg.bits(), m_pImgObjLifetime->qrgbimg.byteCount() / 2);
			memcpy(m_pImgObjOctLongiImage->qrgbimg.bits() + 3 * m_pImgObjOctLongiImage->arr.size(0) * RING_THICKNESS, m_pImgObjIntensity->qrgbimg.bits(), m_pImgObjIntensity->qrgbimg.byteCount() / 2);

			memcpy(m_pImgObjOctLongiImage->qrgbimg.bits() + 3 * m_pImgObjOctLongiImage->arr.size(0) * (m_pImgObjOctLongiImage->arr.size(1) - 2 * RING_THICKNESS),
				m_pImgObjIntensity->qrgbimg.bits() + m_pImgObjIntensity->qrgbimg.byteCount() / 2, m_pImgObjIntensity->qrgbimg.byteCount() / 2);
			memcpy(m_pImgObjOctLongiImage->qrgbimg.bits() + 3 * m_pImgObjOctLongiImage->arr.size(0) * (m_pImgObjOctLongiImage->arr.size(1) - 1 * RING_THICKNESS),
				m_pImgObjLifetime->qrgbimg.bits() + m_pImgObjLifetime->qrgbimg.byteCount() / 2, m_pImgObjLifetime->qrgbimg.byteCount() / 2);
		}
//		else
		{
			// Classification map
			//ColorTable temp_ctable;
			//ImageObject tempImgObj(m_pImgObjIntensityWeightedLifetime->getWidth(), m_pImgObjIntensityWeightedLifetime->getHeight(), temp_ctable.m_colorTableVector.at(ColorTable::clf));

			tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)(m_pProcessingTab->getConfigTemp()->frames)),
				[&](const tbb::blocked_range<size_t>& r) {
				for (size_t i = r.begin(); i != r.end(); ++i)
				{
                    //tempImgObj.arr((int)i, 0) = m_pViewTab->m_pAnn->GetClfMapPtr()(aline / 4, (int)(m_pProcessingTab->getConfigTemp()->frames - i - 1));
                    //tempImgObj.arr((int)i, RING_THICKNESS) = m_pViewTab->m_pAnn->GetClfMapPtr()(m_pProcessingTab->getConfigTemp()->flimAlines / 2 + aline / 4, (int)(m_pProcessingTab->getConfigTemp()->frames - i - 1));
				}
			});
			tbb::parallel_for(tbb::blocked_range<size_t>(1, (size_t)(RING_THICKNESS)),
				[&](const tbb::blocked_range<size_t>& r) {
				for (size_t i = r.begin(); i != r.end(); ++i)
				{
					//memcpy(&tempImgObj.arr(0, (int)i), &tempImgObj.arr(0, 0), sizeof(uint8_t) * tempImgObj.arr.size(0));
					//memcpy(&tempImgObj.arr(0, (int)(i + RING_THICKNESS)), &tempImgObj.arr(0, RING_THICKNESS), sizeof(uint8_t) * tempImgObj.arr.size(0));
				}
			});

			//memcpy(tempImgObj.qindeximg.bits(), m_pAnn->GetClfMapPtr(), tempImgObj.qindeximg.byteCount());
			//tempImgObj.convertRgb();

			//memcpy(m_pImgObjIntensityWeightedLifetime->qrgbimg.bits(), tempImgObj.qrgbimg.bits(), tempImgObj.qrgbimg.byteCount());

			memcpy(m_pImgObjOctLongiImage->qrgbimg.bits(), m_pImgObjIntensityWeightedLifetime->qrgbimg.bits(), m_pImgObjIntensityWeightedLifetime->qrgbimg.byteCount() / 2);
			memcpy(m_pImgObjOctLongiImage->qrgbimg.bits() + 3 * m_pImgObjOctLongiImage->arr.size(0) * (m_pImgObjOctLongiImage->arr.size(1) - 1 * RING_THICKNESS),
				m_pImgObjIntensityWeightedLifetime->qrgbimg.bits() + m_pImgObjIntensityWeightedLifetime->qrgbimg.byteCount() / 2, m_pImgObjIntensityWeightedLifetime->qrgbimg.byteCount() / 2);
		}
	}
//	else
	{
		// intensity-weight lifetime map
		ColorTable temp_ctable;
		ImageObject tempImgObj(m_pImgObjIntensityWeightedLifetime->getWidth(), m_pImgObjIntensityWeightedLifetime->getHeight(), temp_ctable.m_colorTableVector.at(ColorTable::gray));

		m_pImgObjLifetime->convertRgb();
		memcpy(tempImgObj.qindeximg.bits(), m_pImgObjIntensity->arr.raw_ptr(), tempImgObj.qindeximg.byteCount());
		tempImgObj.convertRgb();

		ippsMul_8u_Sfs(m_pImgObjLifetime->qrgbimg.bits(), tempImgObj.qrgbimg.bits(), m_pImgObjIntensityWeightedLifetime->qrgbimg.bits(), tempImgObj.qrgbimg.byteCount(), 8);
		
		memcpy(m_pImgObjOctLongiImage->qrgbimg.bits(), m_pImgObjIntensityWeightedLifetime->qrgbimg.bits(), m_pImgObjIntensityWeightedLifetime->qrgbimg.byteCount() / 2);
		memcpy(m_pImgObjOctLongiImage->qrgbimg.bits() + 3 * m_pImgObjOctLongiImage->arr.size(0) * (m_pImgObjOctLongiImage->arr.size(1) - 1 * RING_THICKNESS),
			m_pImgObjIntensityWeightedLifetime->qrgbimg.bits() + m_pImgObjIntensityWeightedLifetime->qrgbimg.byteCount() / 2, m_pImgObjIntensityWeightedLifetime->qrgbimg.byteCount() / 2);
	}

	emit paintLongiImage(m_pImgObjOctLongiImage->qrgbimg.bits());
	
	// Widgets updates
	QString str; str.sprintf("Current A-line : (%4d, %4d) / %4d (%3.2f deg)  ",
		aline + 1, aline + m_pProcessingTab->getConfigTemp()->octAlines / 2 + 1, m_pProcessingTab->getConfigTemp()->octAlines,
		360.0 * (double)aline / (double)m_pProcessingTab->getConfigTemp()->octAlines);
	m_pLabel_CurrentAline->setText(str);

//    if (m_pViewTab->getRectImageView()->isVisible())
//	{
//        m_pViewTab->getRectImageView()->setVerticalLine(1, aline);
//        m_pViewTab->getRectImageView()->getRender()->update();
//	}
//	else
	{
        m_pViewTab->getCircImageView()->setVerticalLine(1, aline);
        m_pViewTab->getCircImageView()->getRender()->update();
	}
}
