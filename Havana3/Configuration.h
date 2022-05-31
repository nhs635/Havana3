#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#define VERSION						"2.1.3.6"

#define POWER_2(x)					(1 << x)
#define NEAR_2_POWER(x)				(int)(1 << (int)ceil(log2(x)))
#define ROUND_UP_4S(x)				((x + 3) >> 2) << 2

////////////////////// Software Setup ///////////////////////
#define DEVELOPER_MODE
///#define NEXT_GEN_SYSTEM
///#define ENABLE_FPGA_FFT
#define ENABLE_DATABASE_ENCRYPTION
//#define AXSUN_ENABLE
//#define NI_ENABLE

//////////////////////// Size Setup /////////////////////////
#define FLIM_SCANS                  512
#define FLIM_ALINES                 256

#ifndef NEXT_GEN_SYSTEM
#define OCT_SCANS                   1024  
#else
#define OCT_SCANS					1280
#define OCT_SCANS_FFT				NEAR_2_POWER(OCT_SCANS)
#endif
#define OCT_ALINES                  FLIM_ALINES * 4

//////////////////////// System Setup ///////////////////////
#define ROTARY_MOTOR_COM_PORT		"COM4"
#define PULLBACK_MOTOR_COM_PORT		"COM7"

#define FLIM_LASER_COM_PORT			"COM5"

#define PMT_GAIN_AO_PORT			"Dev1/ao1"  

#define FLIM_LASER_SOURCE_TERMINAL	"/Dev1/PFI2"
#define FLIM_LASER_COUNTER_CHANNEL	"Dev1/ctr1"

#ifndef NEXT_GEN_SYSTEM
#define AXSUN_SOURCE_TERMINAL		"/Dev1/PFI3"
#define AXSUN_COUNTER_CHANNEL		"Dev1/ctr0"
#else
#define FLIM_DAQ_SOURCE_TERMINAL	"/Dev1/PFI11"
#define FLIM_DAQ_COUNTER_CHANNEL	"Dev1/ctr1"
#endif

#define ROTATION_100FPS				5872
#define CLOCK_DELAY					10

//////////////// Thread & Buffer Processing /////////////////
#define PROCESSING_BUFFER_SIZE		80

#ifdef _DEBUG
#define WRITING_BUFFER_SIZE			500
#else
#define WRITING_BUFFER_SIZE	        2000
#endif

///////////////////// FLIm Processing ///////////////////////
#define FLIM_CH_START_5				45
#define GAUSSIAN_FILTER_WIDTH		235 /// 200 // 200 vs 230
#define GAUSSIAN_FILTER_STD			56.5 ///60 // 48 // 48 vs 55 // 48 => 80MHz
#define FLIM_SPLINE_FACTOR			20
#define INTENSITY_THRES				0.001f

/////////////////////// Visualization ///////////////////////
#define RING_THICKNESS				160

#define LONGI_WIDTH					1180
#define LONGI_HEIGHT				243

#define OCT_COLORTABLE              2 // sepia
#define INTENSITY_COLORTABLE		0 // gray
#define INTENSITY_PROP_COLORTABLE	6 // fire
#define INTENSITY_RATIO_COLORTABLE	12 // bwr
#define LIFETIME_COLORTABLE         13 // hsv2 ==> Viewer/QImageView.cpp
#define IVUS_COLORTABLE				0 // gray
#define INFLAMMATION_COLORTABLE		5 // hot
#define COMPOSITION_COLORTABLE		14 // compo

#define RF_N_TREES					100
#define RF_N_FEATURES				9
#define RF_N_CATS					5
#define RF_INFL_DATA_NAME			"infl_forest.csv"
#define RF_INFL_MODEL_NAME			"infl_forest.xml"
#define RF_COMPO_DATA_NAME			"compo_forest.csv"
#define RF_COMPO_MODEL_NAME			"compo_forest.xml"

#define INTER_FRAME_SYNC			1 // Frames adjustment
#define INTRA_FRAME_SYNC			1010 // A-lines adjustment

#define RENEWAL_COUNT				8 
#define REDUCED_COUNT				4
#define PIXEL_RESOLUTION			5.0757 // micrometers
#define OUTER_SHEATH_POSITION		114 // (int)((150 * 1.45 + 180 * 1 + 150 * 1.33) / PIXEL_RESOLUTION) // 0.034 inch OD





template <typename T>
struct ContrastRange
{
	T min = 0;
	T max = 0;
};


#include <QString>
#include <QStringList>
#include <QSettings>
#include <QDateTime>

#include <Common/callback.h>


class Configuration
{
public:
	explicit Configuration() : dbPath(""), ivusPath("")
	{
		memset(flimDelayOffset0, 0, sizeof(float) * 3);
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
#ifdef NEXT_GEN_SYSTEM
		octScansFFT = NEAR_2_POWER(octScans);
#endif
        octAlines = settings.value("octAlines").toInt();
#ifndef NEXT_GEN_SYSTEM
        octFrameSize = octScans * octAlines;
#else
		octFrameSize = octScansFFT / 2 * octAlines;
#endif

        // FLIm processing
		flimBg = settings.value("flimBg").toFloat();
		flimWidthFactor = settings.value("flimWidthFactor").toFloat();
		for (int i = 0; i < 4; i++)
		{
			flimChStartInd[i] = settings.value(QString("flimChStartInd_%1").arg(i)).toInt();
			if (i != 0)
				flimDelayOffset[i - 1] = settings.value(QString("flimDelayOffset_%1").arg(i)).toFloat();
		}

		// FLIM classification
		///clfAnnXNode = settings.value("clfAnnXNode").toInt();
		///clfAnnHNode = settings.value("clfAnnHNode").toInt();
		///clfAnnYNode = settings.value("clfAnnYNode").toInt();
		
        // Visualization
        flimEmissionChannel = settings.value("flimEmissionChannel").toInt();
		flimParameterMode = settings.value("flimParameterMode").toInt();
		for (int i = 0; i < 3; i++)
		{
			flimIntensityRange[i].max = settings.value(QString("flimIntensityRangeMax_Ch%1").arg(i + 1)).toFloat();
			flimIntensityRange[i].min = settings.value(QString("flimIntensityRangeMin_Ch%1").arg(i + 1)).toFloat();
			flimLifetimeRange[i].max = settings.value(QString("flimLifetimeRangeMax_Ch%1").arg(i + 1)).toFloat();
			flimLifetimeRange[i].min = settings.value(QString("flimLifetimeRangeMin_Ch%1").arg(i + 1)).toFloat();
			flimIntensityPropRange[i].max = settings.value(QString("flimIntensityPropRangeMax_Ch%1").arg(i + 1)).toFloat();
			flimIntensityPropRange[i].min = settings.value(QString("flimIntensityPropRangeMin_Ch%1").arg(i + 1)).toFloat();
			flimIntensityRatioRange[i].max = settings.value(QString("flimIntensityRatioRangeMax_Ch%1_%2").arg(i + 1).arg((i == 0) ? 3 : i)).toFloat();
			flimIntensityRatioRange[i].min = settings.value(QString("flimIntensityRatioRangeMin_Ch%1_%2").arg(i + 1).arg((i == 0) ? 3 : i)).toFloat();

			float temp = settings.value(QString("flimIntensityComp_%1").arg(i + 1)).toFloat();
			flimIntensityComp[i] = (temp == 0.0f) ? 1.0f : temp;			
        }
#ifndef NEXT_GEN_SYSTEM
		octGrayRange.max = settings.value("octGrayRangeMax").toInt();
		octGrayRange.min = settings.value("octGrayRangeMin").toInt();
#endif
		rfInflammationRange.max = settings.value("rfInflammationRangeMax").toFloat();
		rfInflammationRange.min = settings.value("rfInflammationRangeMin").toFloat();
		rotatedAlines = settings.value("rotatedAlines").toInt();
		innerOffsetLength = settings.value("innerOffsetLength").toInt();
		verticalMirroring = settings.value("verticalMirroring").toBool();
		
		// Additional synchronization parameters
		intraFrameSync = settings.value("intraFrameSync").toInt();
		interFrameSync = settings.value("interFrameSync").toInt();

		// Device control
		rotaryRpm = settings.value("rotaryRpm").toInt();
		pullbackSpeed = settings.value("pullbackSpeed").toFloat();
		pullbackLength = settings.value("pullbackLength").toFloat();	
		pullbackFlag = settings.value("pullbackFlag").toBool();
		pmtGainVoltage = settings.value("pmtGainVoltage").toFloat(); 
#ifndef NEXT_GEN_SYSTEM
        px14DcOffset = settings.value("px14DcOffset").toInt();
#else
		flimLaserPower = settings.value("flimLaserPower").toInt();
#endif
		axsunVDLLength = settings.value("axsunVDLLength").toFloat();
		axsunDispComp_a2 = settings.value("axsunDispComp_a2").toFloat();
		axsunDispComp_a3 = settings.value("axsunDispComp_a3").toFloat();
		axsunDbRange.max = settings.value("axsunDbRangeMax").toFloat();
		axsunDbRange.min = settings.value("axsunDbRangeMin").toFloat();

        // Database
        dbPath = settings.value("dbPath").toString();
		ivusPath = settings.value("ivusPath").toString();;

		settings.endGroup();

		writeToLog(QString("Get configuration data: %1").arg(inipath));
	}

	void setConfigFile(QString inipath)
	{
		QSettings settings(inipath, QSettings::IniFormat);
		settings.beginGroup("configuration");

		// Image size
		settings.setValue("flimScans", flimScans);
		settings.setValue("flimAlines", flimAlines);
		settings.setValue("octScans", octScans);
#ifdef NEXT_GEN_SYSTEM
		settings.setValue("octScansFFT", octScansFFT);
#endif
		settings.setValue("octAlines", octAlines);

		// FLIm processing
		settings.setValue("flimBg", QString::number(flimBg, 'f', 2));
		settings.setValue("flimWidthFactor", QString::number(flimWidthFactor, 'f', 2));
		for (int i = 0; i < 4; i++)
		{
			settings.setValue(QString("flimChStartInd_%1").arg(i), flimChStartInd[i]);
			if (i != 0)
			{
				settings.setValue(QString("flimDelayOffset_%1").arg(i), QString::number(flimDelayOffset[i - 1], 'f', 3));
				settings.setValue(QString("flimDelayOffset0_%1").arg(i), QString::number(flimDelayOffset0[i - 1], 'f', 3));
			}
		}

		// Visualization
        settings.setValue("flimEmissionChannel", flimEmissionChannel);
		settings.setValue("flimParameterMode", flimParameterMode);
		for (int i = 0; i < 3; i++)
		{
			settings.setValue(QString("flimIntensityRangeMax_Ch%1").arg(i + 1), QString::number(flimIntensityRange[i].max, 'f', 1));
			settings.setValue(QString("flimIntensityRangeMin_Ch%1").arg(i + 1), QString::number(flimIntensityRange[i].min, 'f', 1));
			settings.setValue(QString("flimLifetimeRangeMax_Ch%1").arg(i + 1), QString::number(flimLifetimeRange[i].max, 'f', 1));
			settings.setValue(QString("flimLifetimeRangeMin_Ch%1").arg(i + 1), QString::number(flimLifetimeRange[i].min, 'f', 1));
			settings.setValue(QString("flimIntensityPropRangeMax_Ch%1").arg(i + 1), QString::number(flimIntensityPropRange[i].max, 'f', 1));
			settings.setValue(QString("flimIntensityPropRangeMin_Ch%1").arg(i + 1), QString::number(flimIntensityPropRange[i].min, 'f', 1));
			settings.setValue(QString("flimIntensityRatioRangeMax_Ch%1_%2").arg(i + 1).arg((i == 0) ? 3 : i), QString::number(flimIntensityRatioRange[i].max, 'f', 1));
			settings.setValue(QString("flimIntensityRatioRangeMin_Ch%1_%2").arg(i + 1).arg((i == 0) ? 3 : i), QString::number(flimIntensityRatioRange[i].min, 'f', 1));
		}
#ifndef NEXT_GEN_SYSTEM
		settings.setValue("octGrayRangeMax", octGrayRange.max);
		settings.setValue("octGrayRangeMin", octGrayRange.min);
#endif
		settings.setValue("rfInflammationRangeMax", QString::number(rfInflammationRange.max, 'f', 1));
		settings.setValue("rfInflammationRangeMin", QString::number(rfInflammationRange.min, 'f', 2));
		settings.setValue("rotatedAlines", rotatedAlines);
		settings.setValue("innerOffsetLength", innerOffsetLength);
		settings.setValue("verticalMirroring", verticalMirroring);

		// Additional synchronization parameters
		settings.setValue("intraFrameSync", intraFrameSync);
		settings.setValue("interFrameSync", interFrameSync);

		// Device control
		settings.setValue("rotaryRpm", rotaryRpm);
		settings.setValue("pullbackSpeed", QString::number(pullbackSpeed, 'f', 2));
		settings.setValue("pullbackLength", QString::number(pullbackLength, 'f', 2));
		settings.setValue("pullbackFlag", pullbackFlag);
		settings.setValue("laserPowerLevel", laserPowerLevel);
		settings.setValue("pmtGainVoltage", QString::number(pmtGainVoltage, 'f', 3));
#ifndef NEXT_GEN_SYSTEM
        settings.setValue("px14DcOffset", px14DcOffset);
#else
		settings.setValue("flimLaserPower", flimLaserPower);
#endif
		settings.setValue("axsunVDLLength", QString::number(axsunVDLLength, 'f', 2));
		settings.setValue("axsunDispComp_a2", QString::number(axsunDispComp_a2, 'f', 1));
		settings.setValue("axsunDispComp_a3", QString::number(axsunDispComp_a3, 'f', 1));
		settings.setValue("axsunDbRangeMax", QString::number(axsunDbRange.max, 'f', 1));
		settings.setValue("axsunDbRangeMin", QString::number(axsunDbRange.min, 'f', 1));

        // Database
        settings.setValue("dbPath", dbPath);
		settings.setValue("ivusPath", ivusPath);

		// Current Time
		QString datetime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
		settings.setValue("time", datetime);

		settings.endGroup();

		writeToLog(QString("Set configuration data: %1").arg(inipath));
	}

	void writeToLog(QString msg)
	{
		QString total_msg = QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss] ") + msg;
		log << total_msg;
		DidLogAdded(total_msg);
	}
	
public:
    // Image size
    int frames;
    int flimScans, flimAlines, flimFrameSize;
    int octScans, octAlines, octFrameSize;
#ifdef NEXT_GEN_SYSTEM
	int octScansFFT;
#endif

    // FLIm processing
	float flimBg;
	float flimWidthFactor;
	int flimChStartInd[4];
    float flimDelayOffset[3];
	float flimDelayOffset0[3];

	// FLIm classification
	///int clfAnnXNode;
	///int clfAnnHNode;
	///int clfAnnYNode;
	
	// Visualization    
    int flimEmissionChannel;
	int flimParameterMode;
    ContrastRange<float> flimIntensityRange[3];
    ContrastRange<float> flimLifetimeRange[3];
	ContrastRange<float> flimIntensityPropRange[3];
    ContrastRange<float> flimIntensityRatioRange[3];
	float flimIntensityComp[3];
#ifndef NEXT_GEN_SYSTEM
    ContrastRange<int> octGrayRange;
#endif
	ContrastRange<float> rfInflammationRange;
	int rotatedAlines;
	int innerOffsetLength;
	bool verticalMirroring;

	// Additional synchronization parameters
	int intraFrameSync;
	int interFrameSync;

	// Device control
	int rotaryRpm;
	float pullbackSpeed;
	float pullbackLength;
	bool pullbackFlag;
	float pmtGainVoltage;
	int laserPowerLevel;
#ifndef NEXT_GEN_SYSTEM
    int px14DcOffset;
#else
	int flimLaserPower;
#endif
	float axsunVDLLength;
	float axsunDispComp_a2, axsunDispComp_a3;
    ContrastRange<float> axsunDbRange;	

    // Database
    QString dbPath;
	QString ivusPath;
	
	// System log
	QStringList log;
	callback<const QString&> DidLogAdded;
};


#endif // CONFIGURATION_H
