
#include "FLImProcess.h"

#include <iostream>
#include <chrono>


FLImProcess::FLImProcess()
{
}

FLImProcess::~FLImProcess()
{
}


void FLImProcess::operator() (FloatArray2& intensity, FloatArray2& mean_delay, FloatArray2& lifetime, Uint16Array2& pulse)
{
	//std::chrono::system_clock::time_point StartTime = std::chrono::system_clock::now();

    // 1. Crop and resize pulse data
    _resize(pulse, _params);
	
	//std::chrono::system_clock::time_point EndTime1 = std::chrono::system_clock::now();
	//std::chrono::microseconds micro = std::chrono::duration_cast<std::chrono::microseconds>(EndTime1 - StartTime);
	//printf("[Resize] %d microsec\n", micro.count());

    // 2. Get intensity
    _intensity(_resize);
    memcpy(intensity, _intensity.intensity, sizeof(float) * _intensity.intensity.length());

	//std::chrono::system_clock::time_point EndTime2 = std::chrono::system_clock::now();
	//micro = std::chrono::duration_cast<std::chrono::microseconds>(EndTime2 - EndTime1);
	//printf("[Intensity] %d microsec\n", micro.count());

    // 3. Get lifetime
    _lifetime(_resize, _params, intensity);
    memcpy(mean_delay, _lifetime.mean_delay, sizeof(float) * _lifetime.mean_delay.length());
	memcpy(lifetime, _lifetime.lifetime, sizeof(float) * _lifetime.lifetime.length());

	//std::chrono::system_clock::time_point EndTime3 = std::chrono::system_clock::now();
	//micro = std::chrono::duration_cast<std::chrono::microseconds>(EndTime3 - EndTime2);
	//printf("[Lifetime] %d microsec\n", micro.count());
}

//void FLImProcess::getResize(Uint16Array2& pulse)
//{
//	//std::chrono::system_clock::time_point StartTime = std::chrono::system_clock::now();
//
//	//// 1. Crop and resize pulse data
//	//_resize(pulse, _params);
//
//	//std::chrono::system_clock::time_point EndTime1 = std::chrono::system_clock::now();
//	//std::chrono::microseconds micro = std::chrono::duration_cast<std::chrono::microseconds>(EndTime1 - StartTime);
//	////printf("[Resize] %d microsec\n", micro.count());
//}
//
//void FLImProcess::getIntensity(FloatArray2& intensity)
//{
//	//std::chrono::system_clock::time_point StartTime = std::chrono::system_clock::now();
//
//	//// 2. Get intensity
//	//_intensity(_resize);
//	//memcpy(intensity, _intensity.intensity, sizeof(float) * _intensity.intensity.length());
//
//	//std::chrono::system_clock::time_point EndTime = std::chrono::system_clock::now();
//	//std::chrono::microseconds micro = std::chrono::duration_cast<std::chrono::microseconds>(EndTime - StartTime);
//	////printf("[Intensity] %d microsec\n", micro.count());
//}
//
//void FLImProcess::getLifetime(FloatArray2& mean_delay, FloatArray2& lifetime)
//{
//	//std::chrono::system_clock::time_point StartTime = std::chrono::system_clock::now();
//
//	//// 3. Get lifetime
//	//_lifetime(_resize, _params, _intensity.intensity);
//	//memcpy(mean_delay, _lifetime.mean_delay, sizeof(float) * _lifetime.mean_delay.length());
//	//memcpy(lifetime, _lifetime.lifetime, sizeof(float) * _lifetime.lifetime.length());
//
//	//std::chrono::system_clock::time_point EndTime = std::chrono::system_clock::now();
//	//std::chrono::microseconds micro = std::chrono::duration_cast<std::chrono::microseconds>(EndTime - StartTime);
//	////printf("[Lifetime] %d microsec\n", micro.count());
//}




void FLImProcess::setParameters(Configuration* pConfig)
{
    _params.bg = pConfig->flimBg;

    _params.samp_intv = 1000.0f / (float)PX14_ADC_RATE;
    _params.width_factor = 2.0f;

    for (int i = 0; i < 4; i++)
    {
        _params.ch_start_ind[i] = pConfig->flimChStartInd[i];
        if (i != 0)
             _params.delay_offset[i - 1] = pConfig->flimDelayOffset[i - 1];
    }
    _params.ch_start_ind[4] = _params.ch_start_ind[3] + FLIM_CH_START_5;
}

void FLImProcess::saveMaskData(QString maskpath)
{
    qint64 sizeRead;

    // create file (flim mask)
    QFile maskFile(maskpath);
    if (false != maskFile.open(QIODevice::WriteOnly))
    {
        sizeRead = maskFile.write(reinterpret_cast<char*>(_resize.pMask), sizeof(float) * _resize.nx);
        maskFile.close();
    }
}

void FLImProcess::loadMaskData(QString maskpath)
{
    qint64 sizeRead;

    // create file (flim mask)
    QFile maskFile(maskpath);
    if (false != maskFile.open(QIODevice::ReadOnly))
    {
        sizeRead = sizeRead = maskFile.read(reinterpret_cast<char*>(_resize.pMask), sizeof(float) * _resize.nx);
        maskFile.close();

        int start_count = 0, end_count = 0;
        for (int i = 0; i < _resize.nx - 1; i++)
        {
            if (_resize.pMask[i + 1] - _resize.pMask[i] == -1)
            {
                start_count++;
                if (start_count < 5)
                        _resize.start_ind[start_count - 1] = i - 1;
            }
            if (_resize.pMask[i + 1] - _resize.pMask[i] == 1)
            {
                end_count++;
                if (end_count < 5)
                        _resize.end_ind[end_count - 1] = i;
            }
        }

		SendStatusMessage("[FLIm AF Mask]");

		for (int i = 0; i < 4; i++)
		{
			char msg[256];
			sprintf(msg, "mask %d: [%d %d]", i + 1, _resize.start_ind[i], _resize.end_ind[i]);
			SendStatusMessage(msg);
		}

		if ((start_count == 4) && (end_count == 4))
			SendStatusMessage("Proper mask is selected!!");
        else
			SendStatusMessage("Improper mask: please modify the mask!");
    }
}
