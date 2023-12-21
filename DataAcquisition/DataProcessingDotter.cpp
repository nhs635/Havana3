
#include "DataProcessingDotter.h"

#include <Havana3/MainWindow.h>
#include <Havana3/QResultTab.h>
#include <Havana3/QViewTab.h>

#include <DataAcquisition/FLImProcess/FLImProcess.h>
#include <DataAcquisition/OCTProcess/OCTProcess.h>

#include <ippcore.h>
#include <ippvm.h>

#include <iostream>
#include <thread>


DataProcessingDotter::DataProcessingDotter(QWidget *parent)
    : m_pConfigTemp(nullptr), m_pFLIm(nullptr)
{
	// Set main window objects    
    m_pResultTab = dynamic_cast<QResultTab*>(parent);
	m_pConfig = m_pResultTab->getMainWnd()->m_pConfiguration;

	// Set send status message object (message & error handling function object)
	SendStatusMessage += [&](const char* msg, bool is_error) {
		QString qmsg = QString::fromUtf8(msg);
		if (is_error)
		{
			m_pConfig->writeToLog(QString("[DataProc ERROR] %1").arg(qmsg));
 			QMessageBox MsgBox(QMessageBox::Critical, "Error", qmsg);
			MsgBox.exec();
		}
		else
		{
			m_pConfig->writeToLog(QString("[DataProc] %1").arg(qmsg));
			qDebug() << qmsg;
		}
	};
}

DataProcessingDotter::~DataProcessingDotter()
{
	if (m_pConfigTemp)
	{	
		m_pConfigTemp->setConfigFile(m_iniName);
		delete m_pConfigTemp;
	}
	if (m_pFLIm) delete m_pFLIm;
}


void DataProcessingDotter::startProcessing(QString fileName, int frame)
{
	// Get path to read	
	if (fileName != "")
	{
		std::thread t1([&, fileName, frame]() {

			std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
			
			{
				// Read Ini File & Initialization ///////////////////////////////////////////////////////////
				QString fileTitle = fileName.split(".").at(0);
				QString flimName = fileTitle + ".flim";
				QString rpdName = fileTitle + ".rpd";
				m_iniName = fileTitle + "/pullback.ini";

				if (m_pConfigTemp) delete m_pConfigTemp;
				m_pConfigTemp = new Configuration;

				getFlimInfo(flimName, m_pConfigTemp);
				getOctInfo(rpdName, m_pConfigTemp);

				m_pConfigTemp->is_dotter = true;
				if ((m_pConfigTemp->quantitationRange.max == 0) && (m_pConfigTemp->quantitationRange.min == 0))
				{
					m_pConfigTemp->quantitationRange.max = m_pConfigTemp->frames - 1;
					m_pConfigTemp->quantitationRange.min = 0;
				}

				QFileInfo check_file(m_iniName);
				if (check_file.exists())
					m_pConfigTemp->getConfigFile(m_iniName);

				m_resFolder = fileTitle;				
				QDir().mkdir(m_resFolder);

				char msg[256];
				sprintf(msg, "Start record review processing... (Total nFrame: %d)", m_pConfigTemp->frames);
				SendStatusMessage(msg, false);

				// Set Widgets //////////////////////////////////////////////////////////////////////////////
				m_pResultTab->getViewTab()->setWidgets(m_pConfigTemp);

				// Set Buffers & Objects ////////////////////////////////////////////////////////////////////
				m_pResultTab->getViewTab()->setBuffers(m_pConfigTemp);
				m_pResultTab->getViewTab()->setObjects(m_pConfigTemp);

				m_syncFlimProcessing.allocate_queue_buffer(m_pConfigTemp->flimScans, m_pConfigTemp->flimAlines, PROCESSING_BUFFER_SIZE);

				// Open OCT & FLIm raw file /////////////////////////////////////////////////////////////////
				QFile flim_file(flimName), oct_file(rpdName);
				if ((false == flim_file.open(QFile::ReadOnly)) || (false == oct_file.open(QFile::ReadOnly)))
				{
					SendStatusMessage("Invalid file path! Cannot review the data.", true);
					emit abortedProcessing();
				}
				else
				{
					flim_file.seek(88); oct_file.seek(63);
				}
				
				// Set FLIm Object //////////////////////////////////////////////////////////////////////////
				if (m_pFLIm) delete m_pFLIm;
				m_pFLIm = new FLImProcess;
				m_pFLIm->setParameters(m_pConfigTemp);
				m_pFLIm->_resize(np::Uint16Array2(m_pConfigTemp->flimScans, m_pConfigTemp->flimAlines), m_pFLIm->_params, true);				
				
				// Load FLIm raw data ///////////////////////////////////////////////////////////////////////
				std::thread load_flim_data([&]() { loadingFlimRawData(&flim_file, m_pConfigTemp); });

				// Load OCT raw data ////////////////////////////////////////////////////////////////////////
				std::thread load_oct_data([&]() { loadingOctRawData(&oct_file, m_pConfigTemp); });

				// FLIm Process /////////////////////////////////////////////////////////////////////////////
				std::thread flim_proc([&]() { flimProcessing(m_pFLIm, m_pConfigTemp); });

				// Wait for threads end /////////////////////////////////////////////////////////////////////
				load_flim_data.join();
				load_oct_data.join();
				flim_proc.join();

				// Delete threading sync buffers ////////////////////////////////////////////////////////////				
				m_syncFlimProcessing.deallocate_queue_buffer();

				// Close OCT & FLIm raw file ////////////////////////////////////////////////////////////////
				flim_file.close();
				oct_file.close();

				// Generate en face maps ////////////////////////////////////////////////////////////////////
				getOctProjection(m_pResultTab->getViewTab()->m_vectorOctImage, m_pResultTab->getViewTab()->m_octProjection);

				// Visualization ////////////////////////////////////////////////////////////////////////////
				m_pResultTab->getViewTab()->invalidate();
				if (m_pConfig->autoVibCorrectionMode)
					m_pResultTab->getVibCorrectionButton()->setChecked(true);
				m_pResultTab->getViewTab()->loadPickFrames(m_pResultTab->getViewTab()->m_vectorPickFrames);
				if (frame == -1)
					m_pResultTab->getViewTab()->getPlayButton()->setChecked(true);
				else
					m_pResultTab->getViewTab()->setCurrentFrame(frame - 1);

				//m_pResultTab->getViewTab()->lumenDetection();				
			}

			emit finishedProcessing(true);

            std::chrono::duration<double> elapsed = std::chrono::system_clock::now() - start;

			char msg[256];
			sprintf(msg, "Finished record review processing... (elapsed time: %.2f sec)", elapsed.count());
			SendStatusMessage(msg, false);
		});

		t1.detach();
    }
}


void DataProcessingDotter::getFlimInfo(QString flimName, Configuration* pConfig)
{
	QFile file(flimName);
	if (false == file.open(QFile::ReadOnly))
	{
		SendStatusMessage("Invalid FLIm file! Cannot review the data.", true);
		emit abortedProcessing();
	}
	else
	{
		uint16_t pulse_len, pulse_per_frame, frame_count;
		uint16_t irf_level, bg_level;
		np::Uint16Array channel_rois(8); memset(channel_rois, 0, sizeof(uint16_t) * 8);
		np::DoubleArray channel_offsets(3); memset(channel_offsets, 0, sizeof(double) * 3);

		// Read FLIm-relevant parameters as big-endian machine format
		file.seek(6);
		file.read(reinterpret_cast<char*>(&pulse_len), sizeof(uint16_t)); pulse_len = qToBigEndian(pulse_len);
		file.read(reinterpret_cast<char*>(&pulse_per_frame), sizeof(uint16_t)); pulse_per_frame = qToBigEndian(pulse_per_frame);
		file.read(reinterpret_cast<char*>(&frame_count), sizeof(uint16_t)); frame_count = qToBigEndian(frame_count);
				
		file.seek(28);
		file.read(reinterpret_cast<char*>(&irf_level), sizeof(uint16_t)); irf_level = qToBigEndian(irf_level);
		file.read(reinterpret_cast<char*>(&bg_level), sizeof(uint16_t)); bg_level = qToBigEndian(bg_level);
				
		file.seek(44);
		file.read(reinterpret_cast<char*>(channel_rois.raw_ptr()), sizeof(uint16_t) * 8);
		for (int i = 0; i < 8; i++)
			channel_rois(i) = qToBigEndian(channel_rois(i));

		file.seek(62);		
		file.read(reinterpret_cast<char*>(channel_offsets.raw_ptr()), sizeof(double) * 3);
		for (int i = 0; i < 3; i++)
			channel_offsets(i) = qToBigEndian(channel_offsets(i));

		// Specify to configuration object
		pConfig->flimScans = (int)pulse_len;
		pConfig->flimAlines = (int)pulse_per_frame;
		pConfig->flimFrameSize = pConfig->flimScans * pConfig->flimAlines;
		pConfig->frames = (int)frame_count;

		pConfig->flimIrfLevel = (float)irf_level;
		pConfig->flimBg = (float)bg_level;

		for (int i = 0; i < 8; i++)
			pConfig->flimChStartIndD[i] = (int)channel_rois[i];
		for (int i = 0; i < 3; i++)
			pConfig->flimDelayOffset[i] = (float)channel_offsets[i];
	}
}

void DataProcessingDotter::getOctInfo(QString rpdName, Configuration* pConfig)
{
	QFile file(rpdName);
	if (false == file.open(QFile::ReadOnly))
	{
		SendStatusMessage("Invalid rpd file! Cannot review the data.", true);
		emit abortedProcessing();
	}
	else
	{
		QByteArray arr;

		uint16_t aline_len, aline_per_frame;
		uint16_t calib_offset;

		// Read FLIm-relevant parameters as big-endian machine format
		file.seek(6);
		file.read(reinterpret_cast<char*>(&aline_len), sizeof(uint16_t)); aline_len = qToBigEndian(aline_len);
		file.read(reinterpret_cast<char*>(&aline_per_frame), sizeof(uint16_t)); aline_per_frame = qToBigEndian(aline_per_frame);

		file.seek(36);
		file.read(reinterpret_cast<char*>(&calib_offset), sizeof(uint16_t)); calib_offset = qToBigEndian(calib_offset);
		
		// Specify to configuration object
		pConfig->octScans = (int)aline_len;
		pConfig->octRadius = DOTTER_OCT_RADIUS;
		pConfig->octAlines = (int)aline_per_frame;
		pConfig->octFrameSize = pConfig->octScans * pConfig->octAlines;

		pConfig->innerOffsetLength = (int)calib_offset;
	}
}


void DataProcessingDotter::loadingFlimRawData(QFile* pFile, Configuration* pConfig)
{
	int frameCount = 0;
	while (frameCount < pConfig->frames)
	{
		// Get buffers from threading queues
		uint16_t* pulse_data = nullptr;
		do
		{
			std::unique_lock<std::mutex> lock(m_syncFlimProcessing.mtx);
			if (!m_syncFlimProcessing.queue_buffer.empty())
			{
				pulse_data = m_syncFlimProcessing.queue_buffer.front();
				m_syncFlimProcessing.queue_buffer.pop();
			}

			if (pulse_data)
			{
				// Read data from the external data
				pFile->read(reinterpret_cast<char *>(pulse_data), sizeof(uint16_t) * pConfig->flimFrameSize);
				frameCount++;
				
				// Push the buffers to sync Queues
				m_syncFlimProcessing.Queue_sync.push(pulse_data);
			}

		} while (pulse_data == nullptr);
	}
}

void DataProcessingDotter::loadingOctRawData(QFile* pFile, Configuration* pConfig)
{
	QViewTab* pVisTab = m_pResultTab->getViewTab();

	int frameCount = 0;
	while (frameCount < pConfig->frames)
	{
		// Read data from the external data
		IppiSize roi_oct0 = { pConfig->octScans, pConfig->octAlines };
		IppiSize roi_octR = { pConfig->octRadius, pConfig->octAlines };
		np::Uint8Array2 frame_data(roi_oct0.width, roi_oct0.height);
		pFile->read(reinterpret_cast<char *>(frame_data.raw_ptr()), sizeof(uint8_t) * pConfig->octFrameSize);
		
		if (pConfig->verticalMirroring)
			ippiMirror_8u_C1IR(frame_data, roi_oct0.width, roi_oct0, ippAxsVertical);

		ippiCopy_8u_C1R(frame_data + pConfig->innerOffsetLength, roi_oct0.width,  ///  - pConfig->interFrameSync
			pVisTab->m_vectorOctImage.at(frameCount).raw_ptr(), roi_octR.width,  /// - pConfig->interFrameSync
			{ roi_octR.width, roi_octR.height });
						
		frameCount++;
	}
}

void DataProcessingDotter::flimProcessing(FLImProcess* pFLIm, Configuration* pConfig)
{
    QViewTab* pViewTab = m_pResultTab->getViewTab();

	np::Array<float, 2> pp(pConfig->flimAlines, 4); // temp pulse power
	np::Array<float, 2> itn(pConfig->flimAlines, 4); // temp intensity
	np::Array<float, 2> md(pConfig->flimAlines, 4); // temp mean delay
	np::Array<float, 2> ltm(pConfig->flimAlines, 3); // temp lifetime
	
	///QFile file("pulse.data");
	///file.open(QIODevice::WriteOnly);

	int frameCount = 0;
	while (frameCount < pConfig->frames) ///  + pConfig->interFrameSync)
	{
		// Get the buffer from the previous sync Queue
		uint16_t* pulse_data = m_syncFlimProcessing.Queue_sync.pop();
		if (pulse_data != nullptr)
		{			
			// Pulse array definition
			//np::Uint16Array2 pulse_temp(pulse_data, pConfig->flimScans, pConfig->flimAlines);
			np::Uint16Array2 pulse(pulse_data, pConfig->flimScans, pConfig->flimAlines);

			//// Additional intra frame synchronization process by pulse circulating
			//memcpy(&pulse(0, 0), &pulse_temp(0, 0), sizeof(uint16_t) * pulse.size(0) * pulse.size(1));  /// pConfig->intraFrameSync // - pConfig->intraFrameSync
			///// memcpy(&pulse(0, pulse.size(1) - pConfig->intraFrameSync), &pulse_temp(0, 0), sizeof(uint16_t) * pulse.size(0) * pConfig->intraFrameSync);

			// FLIM Process
			(*pFLIm)(itn, md, ltm, pulse);

			// Copy for Pulse Review
			np::Array<float, 2> crop(pFLIm->_resize.nx, pFLIm->_resize.ny);
			np::Array<float, 2> bg_sub(pFLIm->_resize.nx, pFLIm->_resize.ny);
			np::Array<float, 2> mask(pFLIm->_resize.nx, pFLIm->_resize.ny);
			np::Array<float, 2> ext(pFLIm->_resize.nsite, pFLIm->_resize.ny);
			np::Array<float, 2> filt(pFLIm->_resize.nsite, pFLIm->_resize.ny);

			memcpy(crop, pFLIm->_resize.crop_src, crop.length() * sizeof(float));
			memcpy(bg_sub, pFLIm->_resize.bgsb_src, bg_sub.length() * sizeof(float));
			memcpy(mask, pFLIm->_resize.mask_src, mask.length() * sizeof(float));
			memcpy(ext, pFLIm->_resize.ext_src, ext.length() * sizeof(float));
			memcpy(filt, pFLIm->_resize.filt_src, filt.length() * sizeof(float));

			pViewTab->m_vectorPulseCrop.push_back(crop);
			pViewTab->m_vectorPulseBgSub.push_back(bg_sub);
			pViewTab->m_vectorPulseMask.push_back(mask);
			pViewTab->m_vectorPulseSpline.push_back(ext);
			pViewTab->m_vectorPulseFilter.push_back(filt);
				
			// Intensity compensation
			//for (int i = 0; i < 3; i++)
				//ippsDivC_32f_I(pConfig->flimIntensityComp[i], &itn(0, i + 1), pConfig->flimAlines);

			//QFile file("pulse.data");
			//file.open(QIODevice::WriteOnly);
			//file.write(reinterpret_cast<const char*>(pFLIm->_resize.mask_src.raw_ptr()), sizeof(float) * pFLIm->_resize.mask_src.length());

			// Copy for Intensity & Lifetime	
			memcpy(pp, pFLIm->_resize.pulse_power, sizeof(float) * pp.length());

			memcpy(&pViewTab->m_pulsepowerMap.at(0)(0, frameCount), &pp(0, 0), sizeof(float) * pConfig->flimAlines);
			memcpy(&pViewTab->m_pulsepowerMap.at(1)(0, frameCount), &pp(0, 1), sizeof(float) * pConfig->flimAlines);
			memcpy(&pViewTab->m_pulsepowerMap.at(2)(0, frameCount), &pp(0, 2), sizeof(float) * pConfig->flimAlines);
			memcpy(&pViewTab->m_pulsepowerMap.at(3)(0, frameCount), &pp(0, 3), sizeof(float) * pConfig->flimAlines);

            memcpy(&pViewTab->m_intensityMap.at(0)(0, frameCount), &itn(0, 1), sizeof(float) * pConfig->flimAlines);
            memcpy(&pViewTab->m_intensityMap.at(1)(0, frameCount), &itn(0, 2), sizeof(float) * pConfig->flimAlines);
            memcpy(&pViewTab->m_intensityMap.at(2)(0, frameCount), &itn(0, 3), sizeof(float) * pConfig->flimAlines);

			memcpy(&pViewTab->m_meandelayMap.at(0)(0, frameCount), &md(0, 0), sizeof(float) * pConfig->flimAlines);
			memcpy(&pViewTab->m_meandelayMap.at(1)(0, frameCount), &md(0, 1), sizeof(float) * pConfig->flimAlines);
			memcpy(&pViewTab->m_meandelayMap.at(2)(0, frameCount), &md(0, 2), sizeof(float) * pConfig->flimAlines);
			memcpy(&pViewTab->m_meandelayMap.at(3)(0, frameCount), &md(0, 3), sizeof(float) * pConfig->flimAlines);

            memcpy(&pViewTab->m_lifetimeMap.at(0)(0, frameCount), &ltm(0, 0), sizeof(float) * pConfig->flimAlines);
            memcpy(&pViewTab->m_lifetimeMap.at(1)(0, frameCount), &ltm(0, 1), sizeof(float) * pConfig->flimAlines);
            memcpy(&pViewTab->m_lifetimeMap.at(2)(0, frameCount), &ltm(0, 2), sizeof(float) * pConfig->flimAlines);
         
			emit processedSingleFrame(int(double(100 * frameCount++) / (double)pConfig->frames + 1));

			// Return (push) the buffer to the previous threading queue
			{
				std::unique_lock<std::mutex> lock(m_syncFlimProcessing.mtx);
				m_syncFlimProcessing.queue_buffer.push(pulse_data);
			}
		}
		else
		{
			printf("flimProcessing is halted.\n");
			break;
		}
	}

	// Normalized intensity & lifetime
	for (int i = 0; i < 3; i++)
	{
		FloatArray2 intensity_map1(3 * pViewTab->m_intensityMap.at(i).size(0), pViewTab->m_intensityMap.at(i).size(1));
		for (int j = 0; j < 3; j++)
			ippiCopy_32f_C1R(pViewTab->m_intensityMap.at(i), sizeof(float) * pViewTab->m_intensityMap.at(i).size(0),
				&intensity_map1(j * pViewTab->m_intensityMap.at(i).size(0), 0), sizeof(float) * intensity_map1.size(0),
				{ intensity_map1.size(0) / 3, intensity_map1.size(1) });
		(*pViewTab->getMedfiltIntensityMap())(intensity_map1);
		ippiCopy_32f_C1R(&intensity_map1(pViewTab->m_intensityMap.at(i).size(0), 0), sizeof(float) * intensity_map1.size(0),
			pViewTab->m_intensityMap.at(i), sizeof(float) * pViewTab->m_intensityMap.at(i).size(0),
			{ intensity_map1.size(0) / 3, intensity_map1.size(1) });

		FloatArray2 lifetime_map1(3 * pViewTab->m_lifetimeMap.at(i).size(0), pViewTab->m_lifetimeMap.at(i).size(1));
		for (int j = 0; j < 3; j++)
			ippiCopy_32f_C1R(pViewTab->m_lifetimeMap.at(i), sizeof(float) * pViewTab->m_lifetimeMap.at(i).size(0),
				&lifetime_map1(j * pViewTab->m_lifetimeMap.at(i).size(0), 0), sizeof(float) * lifetime_map1.size(0),
				{ lifetime_map1.size(0) / 3, lifetime_map1.size(1) });
		(*pViewTab->getMedfiltLifetimeMap())(lifetime_map1);
		ippiCopy_32f_C1R(&lifetime_map1(pViewTab->m_lifetimeMap.at(i).size(0), 0), sizeof(float) * lifetime_map1.size(0),
			pViewTab->m_lifetimeMap.at(i), sizeof(float) * pViewTab->m_lifetimeMap.at(i).size(0),
			{ lifetime_map1.size(0) / 3, lifetime_map1.size(1) });
	}
	
	// Calculate other FLIm parameters
	calculateFlimParameters();
}


#ifndef NEXT_GEN_SYSTEM
void DataProcessingDotter::getOctProjection(std::vector<np::Uint8Array2>& vecImg, np::Uint8Array2& octProj, int offset)
#else
void DataProcessingDotter::getOctProjection(std::vector<np::FloatArray2>& vecImg, np::FloatArray2& octProj, int offset)
#endif
{
	int len = vecImg.at(0).size(1);
	int outer_sheath_position = OUTER_SHEATH_POSITION / m_pResultTab->getViewTab()->getCircImageView()->getRender()->m_dPixelResol;
	tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)vecImg.size()),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
		{
#ifndef NEXT_GEN_SYSTEM
			uint8_t maxVal, minVal;
#else
			Ipp32f maxVal, minVal;
#endif
			for (int j = 0; j < octProj.size(0); j++)
			{
#ifndef NEXT_GEN_SYSTEM
				ippsMinMax_8u(&vecImg.at((int)i)(offset + outer_sheath_position, j), len - outer_sheath_position, &minVal, &maxVal);
#else
				ippsMinMax_32f(&vecImg.at((int)i)(offset + outer_sheath_position, j), len - outer_sheath_position, &minVal, &maxVal);
#endif
				octProj(j, (int)i) = maxVal;
			}
		}
	});
}


void DataProcessingDotter::calculateFlimParameters()
{
	QViewTab* pViewTab = m_pResultTab->getViewTab();

	// Intensity ratio
	for (int i = 0; i < 3; i++)
	{
		int ch_num = i;
		int ch_den = (i == 0) ? 2 : i - 1;

		FloatArray2 ratio_temp(pViewTab->m_intensityMap.at(i).size(0), pViewTab->m_intensityMap.at(i).size(1));
		ippsDiv_32f(pViewTab->m_intensityMap.at(ch_den), pViewTab->m_intensityMap.at(ch_num), ratio_temp.raw_ptr(), ratio_temp.length());
		ippsLog10_32f_A11(ratio_temp.raw_ptr(), pViewTab->m_intensityRatioMap.at(ch_num).raw_ptr(), pViewTab->m_intensityRatioMap.at(ch_num).length());
	}

	// Intensity proportion
	FloatArray2 sum(pViewTab->m_intensityMap.at(0).size(0), pViewTab->m_intensityMap.at(0).size(1));
	ippsAdd_32f(pViewTab->m_intensityMap.at(0), pViewTab->m_intensityMap.at(1), sum, sum.length());
	ippsAdd_32f_I(pViewTab->m_intensityMap.at(2), sum, sum.length());
	for (int i = 0; i < 3; i++)
		ippsDiv_32f(sum, pViewTab->m_intensityMap.at(i), pViewTab->m_intensityProportionMap.at(i), sum.length());

	// Random forest feature aggregation
	FloatArray2 features(pViewTab->m_lifetimeMap.at(0).length(), 9);
	for (int i = 0; i < 3; i++)
	{
		memcpy(&features(0, 0 + i), pViewTab->m_lifetimeMap.at(i).raw_ptr(), sizeof(float) * pViewTab->m_lifetimeMap.at(i).length());
		memcpy(&features(0, 3 + i), pViewTab->m_intensityRatioMap.at(i).raw_ptr(), sizeof(float) * pViewTab->m_intensityRatioMap.at(i).length());
		memcpy(&features(0, 6 + i), pViewTab->m_intensityProportionMap.at(i).raw_ptr(), sizeof(float) * pViewTab->m_intensityProportionMap.at(i).length());
	}
	ippiTranspose_32f_C1R(features, sizeof(float) * features.size(0), pViewTab->m_featVectors, sizeof(float) * pViewTab->m_featVectors.size(0), { features.size(0), features.size(1) });

	m_pResultTab->getViewTab()->m_bRePrediction = true;
}
