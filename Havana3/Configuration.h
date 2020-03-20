#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#define VERSION						"1.2.2"

#define POWER_2(x)					(1 << x)
#define NEAR_2_POWER(x)				(int)(1 << (int)ceil(log2(x)))

//////////////////////// FLIm Setup /////////////////////////
#define PX14_ADC_RATE               400 // MHz
#define PX14_VOLT_RANGE             1.2 // Vpp
#define PX14_BOOTBUF_IDX            0

#define FLIM_SCANS                  512
#define FLIM_ALINES                 256

#define ELFORLIGHT_PORT				"COM14"

//#define NI_ENABLE 

#ifdef NI_ENABLE
#define NI_PMT_GAIN_CHANNEL		    "Dev2/ao1" // 13
#define NI_FLIM_TRIG_CHANNEL		"Dev2/ctr1" // PFI5 (7) // 100k/4 Hz
#define NI_FLIM_TRIG_SOURCE			"/Dev2/PFI2"
#define NI_AXSUN_TRIG_CHANNEL		"Dev2/ctr0" // PFI4 (6) // 100k/1024 Hz
#define NI_AXSUN_TRIG_SOURCE		"/Dev2/PFI3"
#endif

///////////////////////// OCT Setup /////////////////////////
#define OCT_SCANS                   1024 // The number of Samples in a single OCT A-line
#define OCT_ALINES                  FLIM_ALINES * 4

#define CLOCK_DELAY					10

//#define VERTICAL_MIRRORING

///#define OCT_DEFAULT_BACKGROUND      "bg.bin"

/////////////////// Pullback Device Setup ///////////////////
#define ZABER_PORT					"\\\\.\\COM15"
#define ZABER_MAX_MICRO_RESOLUTION  64 // BENCHTOP_MODE ? 128 : 64;
#define ZABER_MICRO_RESOLUTION		64
#define ZABER_CONVERSION_FACTOR		1.6384 //1.0 / 9.375 // BENCHTOP_MODE ? 1.0 / 9.375 : 1.6384;
#define ZABER_MICRO_STEPSIZE		0.09921875 //  micro-meter /// 0.49609375 // 
#define ZABER_HOME_OFFSET			0 // mm

#define FAULHABER_PORT				"COM13"
#define FAULHABER_POSITIVE_ROTATION false


//////////////// Thread & Buffer Processing /////////////////
#define PROCESSING_BUFFER_SIZE		80

#ifdef _DEBUG
#define WRITING_BUFFER_SIZE			100
#else
#define WRITING_BUFFER_SIZE	        2000
#endif

///////////////////// FLIm Processing ///////////////////////
#define FLIM_CH_START_5				30
#define GAUSSIAN_FILTER_WIDTH		200 // 200 vs 230
#define GAUSSIAN_FILTER_STD			48 // 48 vs 55 // 48 => 80MHz
#define FLIM_SPLINE_FACTOR			20
#define INTENSITY_THRES				0.001f

/////////////////////// Visualization ///////////////////////
#define RING_THICKNESS				100

#define OCT_COLORTABLE              0 // gray
#define INTENSITY_COLORTABLE		6 // fire
#define LIFETIME_COLORTABLE         16 // hsv1 ==> Viewer/QImageView.cpp

#define INTER_FRAME_SYNC			10//9  // Frames
#define INTRA_FRAME_SYNC			0//30 // A-lines

#define RENEWAL_COUNT				10
#define PIXEL_RESOLUTION			5.7 // micrometer
#define OUTER_SHEATH_POSITION		120 // (int)((150 * 1.45 + 180 * 1 + 150 * 1.33) / PIXEL_RESOLUTION)





template <typename T>
struct Range
{
	T min = 0;
	T max = 0;
};

#include <QString>
#include <QSettings>
#include <QDateTime>

#include <Common/callback.h>


static int ratio_index[3][2] = { { 2, 3 }, { 1, 3 }, { 1, 2 } };

class Configuration
{
public:
    explicit Configuration() 
	{
		flimBg[0] = 0.0f;;
		flimBg[1] = 0.0f;;
		//memset(flimBg, 0, sizeof(float) * 2);
	}

	~Configuration() {}

public:
    void getConfigFile(QString inipath)
	{
		QSettings settings(inipath, QSettings::IniFormat);
		settings.beginGroup("configuration");

        // Image size
        flimScans = settings.value("flimScans").toInt();
        flimAlines = settings.value("flimAlines").toInt();
        flimFrameSize = flimScans * flimAlines;
        octScans = settings.value("octScans").toInt();
        octAlines = settings.value("octAlines").toInt();
        octFrameSize = octScans * octAlines;

        // FLIm processing
		flimBg[0] = settings.value("flimBg0").toFloat();
		flimBg[1] = settings.value("flimBg").toFloat();
		flimWidthFactor = settings.value("flimWidthFactor").toFloat();
		for (int i = 0; i < 4; i++)
		{
			flimChStartInd[i] = settings.value(QString("flimChStartInd_%1").arg(i)).toInt();
			if (i != 0)
				flimDelayOffset[i - 1] = settings.value(QString("flimDelayOffset_%1").arg(i)).toFloat();
		}

		// FLIM classification
		clfAnnXNode = settings.value("clfAnnXNode").toInt();
		clfAnnHNode = settings.value("clfAnnHNode").toInt();
		clfAnnYNode = settings.value("clfAnnYNode").toInt();
		
        // Visualization
        flimEmissionChannel = settings.value("flimEmissionChannel").toInt();
		for (int i = 0; i < 3; i++)
		{
			flimIntensityRatioRefIdx[i] = settings.value(QString("flimIntensityRatioRefIdx_Ch%1").arg(i + 1)).toInt();

			flimIntensityRange[i].max = settings.value(QString("flimIntensityRangeMax_Ch%1").arg(i + 1)).toFloat();
			flimIntensityRange[i].min = settings.value(QString("flimIntensityRangeMin_Ch%1").arg(i + 1)).toFloat();
			flimLifetimeRange[i].max = settings.value(QString("flimLifetimeRangeMax_Ch%1").arg(i + 1)).toFloat();
			flimLifetimeRange[i].min = settings.value(QString("flimLifetimeRangeMin_Ch%1").arg(i + 1)).toFloat();

			for (int j = 0; j < 2; j++)
			{
				flimIntensityRatioRange[i][j].max = settings.value(QString("flimIntensityRatioRangeMax_Ch%1_%2").arg(i + 1).arg(ratio_index[i][j])).toFloat();
				flimIntensityRatioRange[i][j].min = settings.value(QString("flimIntensityRatioRangeMin_Ch%1_%2").arg(i + 1).arg(ratio_index[i][j])).toFloat();
			}

			float temp = settings.value(QString("flimIntensityComp_%1").arg(i + 1)).toFloat();
			flimIntensityComp[i] = (temp == 0.0f) ? 1.0f : temp;			
        }
		octGrayRange.max = settings.value("octGrayRangeMax").toInt();
		octGrayRange.min = settings.value("octGrayRangeMin").toInt();
		
		// Additional synchronization parameters
		intraFrameSync = settings.value("flimSyncAdjust").toInt();
		interFrameSync = settings.value("flimSyncInterFrame").toInt();

		// Device control
		pmtGainVoltage = settings.value("pmtGainVoltage").toFloat(); 
        px14DcOffset = settings.value("px14DcOffset").toInt();
		axsunVDLLength = settings.value("axsunVDLLength").toFloat();
		axsunDbRange.max = settings.value("axsunDbRangeMax").toFloat();
		axsunDbRange.min = settings.value("axsunDbRangeMin").toFloat();
		zaberPullbackSpeed = settings.value("zaberPullbackSpeed").toInt();
		zaberPullbackLength = settings.value("zaberPullbackLength").toInt();
		faulhaberRpm = settings.value("faulhaberRpm").toInt();

		settings.endGroup();
	}

	void setConfigFile(QString inipath)
	{
		QSettings settings(inipath, QSettings::IniFormat);
		settings.beginGroup("configuration");

		// Image size
		settings.setValue("flimScans", flimScans);
		settings.setValue("flimAlines", flimAlines);
		settings.setValue("octScans", octScans);
		settings.setValue("octAlines", octAlines);

		// FLIm processing
		settings.setValue("flimBg0", QString::number(flimBg[0], 'f', 2));
		settings.setValue("flimBg", QString::number(flimBg[1], 'f', 2));
		settings.setValue("flimWidthFactor", QString::number(flimWidthFactor, 'f', 2));
		for (int i = 0; i < 4; i++)
		{
			settings.setValue(QString("flimChStartInd_%1").arg(i), flimChStartInd[i]);
			if (i != 0)
				settings.setValue(QString("flimDelayOffset_%1").arg(i), QString::number(flimDelayOffset[i - 1], 'f', 3));
		}

		// Visualization
        settings.setValue("flimEmissionChannel", flimEmissionChannel);
		for (int i = 0; i < 3; i++)
		{
			settings.setValue(QString("flimIntensityRatioRefIdx_Ch%1").arg(i + 1), flimIntensityRatioRefIdx[i]);

			settings.setValue(QString("flimIntensityRangeMax_Ch%1").arg(i + 1), QString::number(flimIntensityRange[i].max, 'f', 1));
			settings.setValue(QString("flimIntensityRangeMin_Ch%1").arg(i + 1), QString::number(flimIntensityRange[i].min, 'f', 1));
			settings.setValue(QString("flimLifetimeRangeMax_Ch%1").arg(i + 1), QString::number(flimLifetimeRange[i].max, 'f', 1));
			settings.setValue(QString("flimLifetimeRangeMin_Ch%1").arg(i + 1), QString::number(flimLifetimeRange[i].min, 'f', 1));

			for (int j = 0; j < 2; j++)
			{
				settings.setValue(QString("flimIntensityRatioRangeMax_Ch%1_%2").arg(i + 1).arg(ratio_index[i][j]), QString::number(flimIntensityRatioRange[i][j].max, 'f', 1));
				settings.setValue(QString("flimIntensityRatioRangeMin_Ch%1_%2").arg(i + 1).arg(ratio_index[i][j]), QString::number(flimIntensityRatioRange[i][j].min, 'f', 1));
			}
		}
		settings.setValue("octGrayRangeMax", octGrayRange.max);
		settings.setValue("octGrayRangeMin", octGrayRange.min);

		// Device control
		settings.setValue("pmtGainVoltage", QString::number(pmtGainVoltage, 'f', 3));
        settings.setValue("px14DcOffset", px14DcOffset);
		settings.setValue("axsunVDLLength", QString::number(axsunVDLLength, 'f', 2));
		settings.setValue("axsunDbRangeMax", QString::number(axsunDbRange.max, 'f', 1));
		settings.setValue("axsunDbRangeMin", QString::number(axsunDbRange.min, 'f', 1));
		settings.setValue("zaberPullbackSpeed", zaberPullbackSpeed);
		settings.setValue("zaberPullbackLength", zaberPullbackLength);
		settings.setValue("faulhaberRpm", faulhaberRpm);

		// Current Time
		QDate date = QDate::currentDate();
		QTime time = QTime::currentTime();
		settings.setValue("time", QString("%1-%2-%3 %4-%5-%6")
			.arg(date.year()).arg(date.month(), 2, 10, (QChar)'0').arg(date.day(), 2, 10, (QChar)'0')
			.arg(time.hour(), 2, 10, (QChar)'0').arg(time.minute(), 2, 10, (QChar)'0').arg(time.second(), 2, 10, (QChar)'0'));

		settings.endGroup();
	}
	
public:
    // Image size
    int frames;
    int flimScans, flimAlines, flimFrameSize;
    int octScans, octAlines, octFrameSize;

    // FLIm processing
	float flimBg[2];
	float flimWidthFactor;
	int flimChStartInd[4];
    float flimDelayOffset[3];

	// FLIm classification
	int clfAnnXNode;
	int clfAnnHNode;
	int clfAnnYNode;
	
	// Visualization    
    int flimEmissionChannel;
    Range<float> flimIntensityRange[3];
    Range<float> flimLifetimeRange[3];
	int flimIntensityRatioRefIdx[3];
	Range<float> flimIntensityRatioRange[3][3];
	float flimIntensityComp[3];
	Range<int> octGrayRange;

	// Additional synchronization parameters
	int intraFrameSync;
	int interFrameSync;

	// Device control
	float pmtGainVoltage;
    int px14DcOffset;
	float axsunVDLLength;
	Range<float> axsunDbRange;
	int zaberPullbackSpeed;
	int zaberPullbackLength;
    int faulhaberRpm;
	
	// Message callback
	callback<const char*> msgHandle;
};


#endif // CONFIGURATION_H
