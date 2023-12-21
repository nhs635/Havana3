#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#define VERSION						"2.1.6.0"

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

#define RAW_SUBSAMPLING				8 // for raw data acquisition

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
#define DOTTER_OCT_RADIUS			1300
#define RING_THICKNESS				160

#define LONGI_WIDTH					1180
#define LONGI_HEIGHT				243

#define OCT_COLORTABLE              2 // sepia
#define INTENSITY_COLORTABLE		0 // gray
#define INTENSITY_PROP_COLORTABLE	6 // fire
#define INTENSITY_RATIO_COLORTABLE	12 // bwr
#define LIFETIME_COLORTABLE         13 // hsv2 ==> Viewer/QImageView.cpp
#define IVUS_COLORTABLE				0 // gray
#define COMPOSITION_COLORTABLE		14 // compo

#define ML_N_FEATURES				9
#define ML_N_CATS					7
#define ML_COMPO_DATASET_NAME		"compo_dataset.csv"

#define ML_NORMAL_COLOR				0x85bb65 // 0x84c06f
#define ML_FIBROUS_COLOR			0x649254
#define ML_LOOSE_FIBROUS_COLOR		0x4b9cd3  // 0xd5d52b  // 
#define ML_CALCIFICATION_COLOR		0xffffff
#define ML_MACROPHAGE_COLOR			0xff2323 // ff355e // FF5A5F // 0xff4748
#define ML_LIPID_MAC_COLOR			0xffc30b // 0xff7518 // 0x860005 //0x780005
#define ML_SHEATH_COLOR				0x000000

#define RF_N_TREES					100
#define RF_COMPO_MODEL_NAME			"compo_forest.xml"
#define SVM_COMPO_MODEL_NAME		"compo_svm"

#define REFLECTION_DISTANCE			35
#define REFLECTION_LEVEL			0.10f

///#define INTER_FRAME_SYNC			0 // Frames adjustment
///#define INTRA_FRAME_SYNC			0 // A-lines adjustment
#define FLIM_DELAY_SYNC				1015

#define RENEWAL_COUNT				8 
#define REDUCED_COUNT				4

#define PIXEL_RESOLUTION_SAMSUNG	5.0757 // micrometers
#define PIXEL_RESOLUTION_DOTTER		4.9425 // micrometers
#define OUTER_SHEATH_POSITION		515.7475 // (int)((150 * 1.45 + 230.75 * 1 + 50.75 * 1.33) / PIXEL_RESOLUTION) // 0.034 inch OD

// ID: 1000 * 0.026*25.4 = 660.4 ¥ìm
// OD : 1000 * 0.034*25.4 = 863.6 ¥ìm



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
	explicit Configuration() : dbPath(""), ivusPath(""), 
		octRadius(0), circOffset(0), reflectionRemoval(false), reflectionDistance(REFLECTION_DISTANCE), reflectionLevel(REFLECTION_LEVEL),
		mergeNorFib(false), mergeMacTcfa(true), normalizeLogistics(true), playInterval(100),
		rotatedAlines(0), verticalMirroring(false), intraFrameSync(0), interFrameSync(0), flimDelaySync(0), is_dotter(false)
	{
		memset(flimDelayOffset0, 0, sizeof(float) * 3);
		quantitationRange.min = -1;
		quantitationRange.max = -1;
		memset(showPlaqueComposition, 1, sizeof(bool) * ML_N_CATS);
		octGrayRange.min = 0;
		octGrayRange.max = 255;
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
		octRadius = settings.value("octRadius").toInt();
		if (octRadius == 0)
			octRadius = octScans;
        octAlines = settings.value("octAlines").toInt();
#ifndef NEXT_GEN_SYSTEM
        octFrameSize = octScans * octAlines;
#else
		octFrameSize = octScansFFT / 2 * octAlines;
#endif

        // FLIm processing
		flimBg = settings.value("flimBg").toFloat();
		flimIrfLevel = settings.value("flimIrfLevel").toFloat();
		flimWidthFactor = settings.value("flimWidthFactor").toFloat();
		for (int i = 0; i < 4; i++)
		{
			flimChStartInd[i] = settings.value(QString("flimChStartInd_%1").arg(i)).toInt();
			if (i != 0)
				flimDelayOffset[i - 1] = settings.value(QString("flimDelayOffset_%1").arg(i)).toFloat();
		}
		for (int i = 0; i < 8; i++)
			flimChStartIndD[i] = settings.value(QString("flimChStartIndD_%1").arg(i)).toInt();

		// FLIM classification
		///clfAnnXNode = settings.value("clfAnnXNode").toInt();
		///clfAnnHNode = settings.value("clfAnnHNode").toInt();
		///clfAnnYNode = settings.value("clfAnnYNode").toInt();
		
        // Visualization
        flimEmissionChannel = settings.value("flimEmissionChannel").toInt();
		flimParameterMode = settings.value("flimParameterMode").toInt();
		mlPredictionMode = settings.value("mlPredictionMode").toInt();
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
		circOffset = settings.value("circOffset").toInt();
		rotatedAlines = settings.value("rotatedAlines").toInt();
		innerOffsetLength = settings.value("innerOffsetLength").toInt();
		verticalMirroring = settings.value("verticalMirroring").toBool();
		reflectionRemoval = settings.value("reflectionRemoval").toBool();
		reflectionDistance = settings.value("reflectionDistance").toInt();
		reflectionLevel = settings.value("reflectionLevel").toFloat();
		autoVibCorrectionMode = settings.value("autoVibCorrectionMode").toBool();
		quantitationRange.max = settings.value("quantitationRangeMax").toInt();
		quantitationRange.min = settings.value("quantitationRangeMin").toInt();		
		//for (int i = 0; i < ML_N_CATS; i++)
		//	showPlaqueComposition[i] = settings.value(QString("showPlaqueComposition_%1").arg(i + 1)).toBool();
		//mergeNorFib = settings.value("mergeNorFib").toBool();
		//mergeMacTcfa = settings.value("mergeMacTcfa").toBool();
		//normalizeLogistics = settings.value("normalizeLogistics").toBool();
		playInterval = settings.value("playInterval").toInt();
		
		// Additional synchronization parameters
		intraFrameSync = settings.value("intraFrameSync").toInt();
		interFrameSync = settings.value("interFrameSync").toInt();
		flimDelaySync = settings.value("flimDelaySync").toInt();

		// Device control
		rotaryRpm = settings.value("rotaryRpm").toInt();
		pullbackSpeed = settings.value("pullbackSpeed").toFloat();
		pullbackLength = settings.value("pullbackLength").toFloat();	
		pullbackFlag = settings.value("pullbackFlag").toBool();
		autoHomeMode = settings.value("autoHomeMode").toBool();
		autoPullbackMode = settings.value("autoPullbackMode").toBool();
		autoPullbackTime = settings.value("autoPullbackTime").toInt();
		pmtGainVoltage = settings.value("pmtGainVoltage").toFloat(); 
#ifndef NEXT_GEN_SYSTEM
        px14DcOffset = settings.value("px14DcOffset").toInt();
#else
		flimLaserPower = settings.value("flimLaserPower").toInt();
#endif
		axsunPipelineMode = settings.value("axsunPipelineMode").toInt();
		axsunVDLLength = settings.value("axsunVDLLength").toFloat();
		axsunDispComp_a2 = settings.value("axsunDispComp_a2").toFloat();
		axsunDispComp_a3 = settings.value("axsunDispComp_a3").toFloat();
		axsunDbRange.max = settings.value("axsunDbRangeMax").toFloat();
		axsunDbRange.min = settings.value("axsunDbRangeMin").toFloat();

        // Database
        dbPath = settings.value("dbPath").toString();
		ivusPath = settings.value("ivusPath").toString();

		// System type
		is_dotter = settings.value("is_dotter").toBool();

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
		settings.setValue("octRadius", octRadius);
		settings.setValue("octAlines", octAlines);

		// FLIm processing
		settings.setValue("flimBg", QString::number(flimBg, 'f', 2));
		settings.setValue("flimIrfLevel", QString::number(flimIrfLevel, 'f', 2));
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
		for (int i = 0; i < 8; i++)
			settings.setValue(QString("flimChStartIndD_%1").arg(i), flimChStartIndD[i]);

		// Visualization
        settings.setValue("flimEmissionChannel", flimEmissionChannel);
		settings.setValue("flimParameterMode", flimParameterMode);
		settings.setValue("mlPredictionMode", mlPredictionMode);
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
		settings.setValue("circOffset", circOffset);
		settings.setValue("rotatedAlines", rotatedAlines);
		settings.setValue("innerOffsetLength", innerOffsetLength);
		settings.setValue("verticalMirroring", verticalMirroring);
		settings.setValue("reflectionRemoval", reflectionRemoval);
		settings.setValue("reflectionDistance", reflectionDistance);
		settings.setValue("reflectionLevel", QString::number(reflectionLevel, 'f', 2));
		settings.setValue("autoVibCorrectionMode", autoVibCorrectionMode);
		settings.setValue("quantitationRangeMax", quantitationRange.max);
		settings.setValue("quantitationRangeMin", quantitationRange.min);
		//for (int i = 0; i < ML_N_CATS; i++)
		//	settings.setValue(QString("showPlaqueComposition_%1").arg(i + 1), showPlaqueComposition[i]);
		//settings.setValue("mergeNorFib", mergeNorFib);
		//settings.setValue("mergeMacTcfa", mergeMacTcfa);
		//settings.setValue("normalizeLogistics", normalizeLogistics);
		settings.setValue("playInterval", playInterval);

		// Additional synchronization parameters
		settings.setValue("intraFrameSync", intraFrameSync);
		settings.setValue("interFrameSync", interFrameSync);
		settings.setValue("flimDelaySync", flimDelaySync);

		// Device control
		settings.setValue("rotaryRpm", rotaryRpm);
		settings.setValue("pullbackSpeed", QString::number(pullbackSpeed, 'f', 2));
		settings.setValue("pullbackLength", QString::number(pullbackLength, 'f', 2));
		settings.setValue("pullbackFlag", pullbackFlag);
		settings.setValue("autoHomeMode", autoHomeMode);
		settings.setValue("autoPullbackMode", autoPullbackMode);
		settings.setValue("autoPullbackTime", autoPullbackTime);
		settings.setValue("laserPowerLevel", laserPowerLevel);
		settings.setValue("pmtGainVoltage", QString::number(pmtGainVoltage, 'f', 3));
#ifndef NEXT_GEN_SYSTEM
        settings.setValue("px14DcOffset", px14DcOffset);
#else
		settings.setValue("flimLaserPower", flimLaserPower);
#endif
		settings.setValue("axsunPipelineMode", QString::number(axsunPipelineMode));
		settings.setValue("axsunVDLLength", QString::number(axsunVDLLength, 'f', 2));
		settings.setValue("axsunDispComp_a2", QString::number(axsunDispComp_a2, 'f', 1));
		settings.setValue("axsunDispComp_a3", QString::number(axsunDispComp_a3, 'f', 1));
		settings.setValue("axsunDbRangeMax", QString::number(axsunDbRange.max, 'f', 1));
		settings.setValue("axsunDbRangeMin", QString::number(axsunDbRange.min, 'f', 1));

        // Database
        settings.setValue("dbPath", dbPath);
		settings.setValue("ivusPath", ivusPath);

		// System type
		settings.setValue("is_dotter", is_dotter);

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
	int octRadius;
#ifdef NEXT_GEN_SYSTEM
	int octScansFFT;
#endif

    // FLIm processing
	float flimBg;
	float flimIrfLevel;
	float flimWidthFactor;
	int flimChStartInd[4];
	int flimChStartIndD[8];
    float flimDelayOffset[3];
	float flimDelayOffset0[3];

	// FLIm classification
	///int clfAnnXNode;
	///int clfAnnHNode;
	///int clfAnnYNode;
	
	// Visualization    
    int flimEmissionChannel;
	int flimParameterMode;
	int mlPredictionMode;
    ContrastRange<float> flimIntensityRange[3];
    ContrastRange<float> flimLifetimeRange[3];
	ContrastRange<float> flimIntensityPropRange[3];
    ContrastRange<float> flimIntensityRatioRange[3];
	float flimIntensityComp[3];
#ifndef NEXT_GEN_SYSTEM
    ContrastRange<int> octGrayRange;
#endif
	int circOffset;
	int rotatedAlines;
	int innerOffsetLength;
	bool verticalMirroring;
	bool reflectionRemoval;
	int reflectionDistance;
	float reflectionLevel;
	bool autoVibCorrectionMode;
	ContrastRange<int> quantitationRange;
	bool showPlaqueComposition[ML_N_CATS];
	bool mergeNorFib, mergeMacTcfa;
	bool normalizeLogistics;
	int playInterval;

	// Additional synchronization parameters
	int intraFrameSync;
	int interFrameSync;
	int flimDelaySync;

	// Device control
	int rotaryRpm;
	float pullbackSpeed;
	float pullbackLength;
	bool pullbackFlag;
	bool autoHomeMode;
	bool autoPullbackMode;
	int autoPullbackTime;
	float pmtGainVoltage;
	int laserPowerLevel;
#ifndef NEXT_GEN_SYSTEM
    int px14DcOffset;
#else
	int flimLaserPower;
#endif
	int axsunPipelineMode;
	float axsunVDLLength;
	float axsunDispComp_a2, axsunDispComp_a3;
    ContrastRange<float> axsunDbRange;	

    // Database
    QString dbPath;
	QString ivusPath;
	
	// System type
	bool is_dotter;

	// System log
	QStringList log;
	callback<const QString&> DidLogAdded;
};


#endif // CONFIGURATION_H
