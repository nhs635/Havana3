#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#define VERSION						"0.0.0" ///"1.4.1"

#define POWER_2(x)					(1 << x)
#define NEAR_2_POWER(x)				(int)(1 << (int)ceil(log2(x)))
#define ROUND_UP_4S(x)				((x + 3) >> 2) << 2

////////////////////// Software Setup ///////////////////////
#define DEVELOPER_MODE
//#define NEXT_GEN_SYSTEM
//#define ENABLE_FPGA_FFT
#define ENABLE_DATABASE_ENCRYPTION
#define NI_ENABLE

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
#define PULLBACK_MOTOR_COM_PORT		"COM5"

#define FLIM_LASER_COM_PORT			"COM3"

#define PMT_GAIN_AO_PORT			"Dev1/ao1"  /// "Dev1/ao0"

#define FLIM_LASER_SOURCE_TERMINAL	"/Dev1/PFI2"  /// "/Dev1/PFI0"
#define FLIM_LASER_COUNTER_CHANNEL	"Dev1/ctr1"  /// "Dev1/ctr0"

#ifndef NEXT_GEN_SYSTEM
#define AXSUN_SOURCE_TERMINAL		"/Dev1/PFI3"
#define AXSUN_COUNTER_CHANNEL		"Dev1/ctr0"
#else
#define FLIM_DAQ_SOURCE_TERMINAL	"/Dev1/PFI11"
#define FLIM_DAQ_COUNTER_CHANNEL	"Dev1/ctr1"
#endif

#define CLOCK_DELAY					10
//#define PULLBACK_HOME_OFFSET		0 // mm
//#define ROTARY_POSITIVE_ROTATION	false

//////////////// Thread & Buffer Processing /////////////////
#define PROCESSING_BUFFER_SIZE		80

#ifdef _DEBUG
#define WRITING_BUFFER_SIZE			1000
#else
#define WRITING_BUFFER_SIZE	        2000
#endif

///////////////////// FLIm Processing ///////////////////////
#define FLIM_CH_START_5				45
#define GAUSSIAN_FILTER_WIDTH		250 // 200 // 200 vs 230
#define GAUSSIAN_FILTER_STD			60 // 48 // 48 vs 55 // 48 => 80MHz
#define FLIM_SPLINE_FACTOR			20
#define INTENSITY_THRES				0.001f

/////////////////////// Visualization ///////////////////////
#define RING_THICKNESS				160

#define OCT_COLORTABLE              0 // gray
#define INTENSITY_COLORTABLE		6 // fire
#define LIFETIME_COLORTABLE         14 // hsv1 ==> Viewer/QImageView.cpp

#define INTER_FRAME_SYNC			0 //10 //9  // Frames
#define INTRA_FRAME_SYNC			0 //30 // A-lines

#define RENEWAL_COUNT				10
#define PIXEL_RESOLUTION			5.7 // micrometer
#define OUTER_SHEATH_POSITION		120 // (int)((150 * 1.45 + 180 * 1 + 150 * 1.33) / PIXEL_RESOLUTION)





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


static int ratio_index[3][2] = { { 2, 3 }, { 1, 3 }, { 1, 2 } };

class Configuration
{
public:
	explicit Configuration() {}
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
#ifndef NEXT_GEN_SYSTEM
		octGrayRange.max = settings.value("octGrayRangeMax").toInt();
		octGrayRange.min = settings.value("octGrayRangeMin").toInt();
#endif
		rotatedAlines = settings.value("rotatedAlines").toInt();
		verticalMirroring = settings.value("verticalMirroring").toBool();
		
		// Additional synchronization parameters
		intraFrameSync = settings.value("flimSyncAdjust").toInt();
		interFrameSync = settings.value("flimSyncInterFrame").toInt();

		// Device control
		rotaryRpm = settings.value("rotaryRpm").toInt();
		pullbackSpeed = settings.value("pullbackSpeed").toFloat();
		pullbackLength = settings.value("pullbackLength").toFloat();		
		pmtGainVoltage = settings.value("pmtGainVoltage").toFloat(); 
#ifndef NEXT_GEN_SYSTEM
        px14DcOffset = settings.value("px14DcOffset").toInt();
#else
		flimLaserPower = settings.value("flimLaserPower").toInt();
#endif
		axsunVDLLength = settings.value("axsunVDLLength").toFloat();
		axsunDbRange.max = settings.value("axsunDbRangeMax").toFloat();
		axsunDbRange.min = settings.value("axsunDbRangeMin").toFloat();

        // Database
        dbPath = settings.value("dbPath").toString();

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
#ifndef NEXT_GEN_SYSTEM
		settings.setValue("octGrayRangeMax", octGrayRange.max);
		settings.setValue("octGrayRangeMin", octGrayRange.min);
#endif
		settings.setValue("rotatedAlines", rotatedAlines);
		settings.setValue("verticalMirroring", verticalMirroring);

		// Device control
		settings.setValue("rotaryRpm", rotaryRpm);
		settings.setValue("pullbackSpeed", QString::number(pullbackSpeed, 'f', 2));
		settings.setValue("pullbackLength", QString::number(pullbackLength, 'f', 2));
		settings.setValue("pmtGainVoltage", QString::number(pmtGainVoltage, 'f', 3));
#ifndef NEXT_GEN_SYSTEM
        settings.setValue("px14DcOffset", px14DcOffset);
#else
		settings.setValue("flimLaserPower", flimLaserPower);
#endif
		settings.setValue("axsunVDLLength", QString::number(axsunVDLLength, 'f', 2));
		settings.setValue("axsunDbRangeMax", QString::number(axsunDbRange.max, 'f', 1));
		settings.setValue("axsunDbRangeMin", QString::number(axsunDbRange.min, 'f', 1));

        // Database
        settings.setValue("dbPath", dbPath);

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

	// FLIm classification
	int clfAnnXNode;
	int clfAnnHNode;
	int clfAnnYNode;
	
	// Visualization    
    int flimEmissionChannel;
    ContrastRange<float> flimIntensityRange[3];
    ContrastRange<float> flimLifetimeRange[3];
	int flimIntensityRatioRefIdx[3];
    ContrastRange<float> flimIntensityRatioRange[3][3];
	float flimIntensityComp[3];
#ifndef NEXT_GEN_SYSTEM
    ContrastRange<int> octGrayRange;
#endif
	int rotatedAlines;
	bool verticalMirroring;

	// Additional synchronization parameters
	int intraFrameSync;
	int interFrameSync;

	// Device control
	int rotaryRpm;
	float pullbackSpeed;
	float pullbackLength;
	float pmtGainVoltage;
#ifndef NEXT_GEN_SYSTEM
    int px14DcOffset;
#else
	int flimLaserPower;
#endif
	float axsunVDLLength;
    ContrastRange<float> axsunDbRange;	

    // Database
    QString dbPath;
	
	// System log
	QStringList log;
	callback<const QString&> DidLogAdded;
};


#endif // CONFIGURATION_H
