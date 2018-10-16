
#include "QOperationTab.h"

#include <Havana3/Configuration.h>
#include <Havana3/MainWindow.h>
#include <Havana3/QStreamTab.h>
#include <Havana3/QDeviceControlTab.h>

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/ThreadManager.h>
#include <MemoryBuffer/MemoryBuffer.h>

#include <iostream>
#include <thread>


QOperationTab::QOperationTab(QWidget *parent) :
    QDialog(parent)
{
	// Set main window objects
    m_pStreamTab = dynamic_cast<QStreamTab*>(parent);
    m_pConfig = m_pStreamTab->getMainWnd()->m_pConfiguration;

	// Create data acquisition object
    m_pDataAcquisition = new DataAcquisition(m_pConfig);

	// Create memory buffer object
    m_pMemoryBuffer = new MemoryBuffer(this);
	m_pMemoryBuffer->SendStatusMessage += [&](const char* msg, bool is_error) {
		if (is_error) setRecordingButton(false);
		QString qmsg = QString::fromUtf8(msg);
		emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
	};

    // Create widgets for acquisition / recording / saving operation
    m_pToggleButton_Acquisition = new QPushButton(this);
    m_pToggleButton_Acquisition->setCheckable(true);
    m_pToggleButton_Acquisition->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    m_pToggleButton_Acquisition->setFixedHeight(30);
    m_pToggleButton_Acquisition->setText("Start &Acquisition");

    m_pToggleButton_Recording = new QPushButton(this);
    m_pToggleButton_Recording->setCheckable(true);
    m_pToggleButton_Recording->setFixedHeight(30);
    m_pToggleButton_Recording->setText("Start &Recording");
	m_pToggleButton_Recording->setDisabled(true);

    m_pToggleButton_Saving = new QPushButton(this);
    m_pToggleButton_Saving->setCheckable(true);
    m_pToggleButton_Saving->setFixedHeight(30);
    m_pToggleButton_Saving->setText("&Save Recorded Data");
	m_pToggleButton_Saving->setDisabled(true);

    // Create a progress bar (general purpose?)
    m_pProgressBar = new QProgressBar(this);
    m_pProgressBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	m_pProgressBar->setFormat("");
    m_pProgressBar->setValue(0);

    // Set layout
    m_pVBoxLayout = new QVBoxLayout;
    m_pVBoxLayout->setSpacing(2);
	
    QHBoxLayout *pHBoxLayout = new QHBoxLayout;
    pHBoxLayout->setSpacing(2);

    pHBoxLayout->addWidget(m_pToggleButton_Acquisition);
    pHBoxLayout->addWidget(m_pToggleButton_Recording);
    pHBoxLayout->addWidget(m_pToggleButton_Saving);

    m_pVBoxLayout->addItem(pHBoxLayout);
    m_pVBoxLayout->addWidget(m_pProgressBar);
    m_pVBoxLayout->addStretch(1);

    setLayout(m_pVBoxLayout);


	// Connect signal and slot
    connect(m_pToggleButton_Acquisition, SIGNAL(toggled(bool)), this, SLOT(operateDataAcquisition(bool)));
	connect(m_pToggleButton_Recording, SIGNAL(toggled(bool)), this, SLOT(operateDataRecording(bool)));
    connect(m_pToggleButton_Saving, SIGNAL(toggled(bool)), this, SLOT(operateDataSaving(bool)));
    connect(m_pMemoryBuffer, SIGNAL(finishedBufferAllocation()), this, SLOT(setAcqRecEnable()));
	connect(m_pMemoryBuffer, SIGNAL(finishedWritingThread(bool)), this, SLOT(setSaveButtonDefault(bool)));
	connect(m_pMemoryBuffer, SIGNAL(wroteSingleFrame(int)), m_pProgressBar, SLOT(setValue(int)));
}

QOperationTab::~QOperationTab()
{
    if (m_pDataAcquisition) delete m_pDataAcquisition;
    if (m_pMemoryBuffer) delete m_pMemoryBuffer;
}


void QOperationTab::setWidgetsStatus()
{
	if (m_pToggleButton_Acquisition->isChecked()) m_pToggleButton_Acquisition->setChecked(false);
}


void QOperationTab::changedTab(bool change)
{	
	(void)change;
}

void QOperationTab::operateDataAcquisition(bool toggled)
{
    if (toggled) // Start Data Acquisition
    {
        if (m_pDataAcquisition->InitializeAcquistion())
        {
            // Start Thread Process
            m_pStreamTab->m_pThreadVisualization->startThreading();
            m_pStreamTab->m_pThreadFlimProcess->startThreading();

            // Start Data Acquisition
            if (m_pDataAcquisition->StartAcquisition())
            {
                m_pToggleButton_Acquisition->setText("Stop &Acquisition");

                m_pToggleButton_Acquisition->setDisabled(true);
                m_pToggleButton_Recording->setDisabled(true);

                std::thread allocate_writing_buffer([&]() {
                    m_pMemoryBuffer->allocateWritingBuffer();
                });
                allocate_writing_buffer.detach();

                // Set PX14 Control widgets
                m_pStreamTab->getDeviceControlTab()->getPX14DigitizerControl()->setChecked(true);

				/// Set Axsun Bg Control widgets
				///if (m_pStreamTab->getDeviceControlTab()->getAxsunControl()->isChecked()
				///	&& m_pStreamTab->getDeviceControlTab()->getAxsunImagingControl()->isChecked())
				///{
				///	//m_pStreamTab->getDeviceControlTab()->getAxsunBgSetControl()->setEnabled(false);
				///	//m_pStreamTab->getDeviceControlTab()->getAxsunBgResetControl()->setEnabled(false);
				///}

				/// Set synchronization
				///m_pStreamTab->getDeviceControlTab()->getFlimControl()->setChecked(true);

				/// Set Axsun OCT control
				///m_pStreamTab->getDeviceControlTab()->getAxsunControl()->setChecked(true);
            }
            else
                m_pToggleButton_Acquisition->setChecked(false); // When start acquisition is failed...
        }
        else
            m_pToggleButton_Acquisition->setChecked(false); // When initialization is failed...
    }
    else // Stop Data Acquisition
    {
		/// Set Axsun OCT control
		///if (m_pStreamTab->getDeviceControlTab()->getAxsunControl()->isChecked())
		///	m_pStreamTab->getDeviceControlTab()->getAxsunControl()->setChecked(false);

		/// Set synchronization
		///if (m_pStreamTab->getDeviceControlTab()->getFlimControl()->isChecked())
		///	m_pStreamTab->getDeviceControlTab()->getFlimControl()->setChecked(false);

        // Stop Thread Process
        m_pDataAcquisition->StopAcquisition();
        m_pStreamTab->m_pThreadFlimProcess->stopThreading();
        m_pStreamTab->m_pThreadVisualization->stopThreading();

        m_pToggleButton_Acquisition->setText("Start &Acquisition");
        m_pToggleButton_Recording->setDisabled(true);

        // Set PX14 Control widgets
        m_pStreamTab->getDeviceControlTab()->getPX14DigitizerControl()->setChecked(false);

		/// Set Axsun Bg Control widgets
		///if (m_pStreamTab->getDeviceControlTab()->getAxsunControl()->isChecked()
		///	&& m_pStreamTab->getDeviceControlTab()->getAxsunImagingControl()->isChecked())
		///{
		///	//m_pStreamTab->getDeviceControlTab()->getAxsunBgSetControl()->setEnabled(true);
		///	//m_pStreamTab->getDeviceControlTab()->getAxsunBgResetControl()->setEnabled(true);
		///}
    }
}

void QOperationTab::operateDataRecording(bool toggled)
{
    if (toggled) // Start Data Recording
    {
		if (m_pMemoryBuffer->startRecording())
		{
			m_pToggleButton_Recording->setText("Stop &Recording");
			m_pToggleButton_Acquisition->setDisabled(true);
			m_pToggleButton_Saving->setDisabled(true);
		}
		else
		{
			m_pToggleButton_Recording->setChecked(false);

			if (m_pMemoryBuffer->m_bIsSaved) // When select "discard"
				m_pToggleButton_Saving->setDisabled(true);
		}
    }
    else // Stop DataRecording
    {
		m_pMemoryBuffer->stopRecording();

		m_pToggleButton_Recording->setText("Start &Recording");
		m_pToggleButton_Acquisition->setEnabled(true);
		m_pToggleButton_Saving->setEnabled(m_pMemoryBuffer->m_nRecordedFrames != 0);

		if (m_pStreamTab->getDeviceControlTab()->isFaulhaberMotorEnabled())
			m_pStreamTab->getDeviceControlTab()->stopMotor();

		if (m_pMemoryBuffer->m_nRecordedFrames > 1)
			m_pProgressBar->setRange(0, m_pMemoryBuffer->m_nRecordedFrames - 1);
		else
			m_pProgressBar->setRange(0, 1);
		m_pProgressBar->setValue(0);

		m_pToggleButton_Saving->setChecked(true);
    }
}

void QOperationTab::operateDataSaving(bool toggled)
{
    if (toggled)
    {
		if (m_pMemoryBuffer->startSaving())
		{
			m_pToggleButton_Saving->setText("Saving...");
			m_pToggleButton_Recording->setDisabled(true);
			m_pToggleButton_Saving->setDisabled(true);
			m_pProgressBar->setFormat("Writing recorded data... %p%");
			//m_pStreamTab->getMainWnd()->m_pTabWidget->setCurrentIndex(1);
		}
		else
			m_pToggleButton_Saving->setChecked(false);
    }
}


void QOperationTab::setAcqRecEnable()
{
	m_pToggleButton_Acquisition->setEnabled(true); 
	m_pToggleButton_Recording->setEnabled(true);
}

void QOperationTab::setSaveButtonDefault(bool error)
{
	m_pProgressBar->setFormat("");
	m_pToggleButton_Saving->setText("&Save Recorded Data");
	m_pToggleButton_Saving->setChecked(false);
	m_pToggleButton_Saving->setDisabled(!error);
	if (m_pToggleButton_Acquisition->isChecked())
		m_pToggleButton_Recording->setEnabled(true);
}
