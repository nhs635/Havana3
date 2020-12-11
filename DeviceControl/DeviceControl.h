#ifndef DEVICECONTROL_H
#define DEVICECONTROL_H

#include <QObject>

#include <Common/callback.h>


class Configuration;

class PullbackMotor;
class RotaryMotor;

class PmtGainControl;
class ElforlightLaser;

class FreqDivider;

class AxsunControl;

class DeviceControl : public QObject
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit DeviceControl(Configuration*);
    virtual ~DeviceControl();

// Methods //////////////////////////////////////////////
public:
	void setAllDeviceOff();

public:
	inline RotaryMotor* getRotatyMotor() const { return m_pRotaryMotor; }
	inline PullbackMotor* getPullbackMotor() const { return m_pPullbackMotor; }
    inline PmtGainControl* getPmtGainControl() const { return m_pPmtGainControl; }
    inline ElforlightLaser* getElforlightLaser() const { return m_pElforlightLaser; }
	inline FreqDivider* getFlimFreqDivider() const { return m_pFlimFreqDivider; }
	inline FreqDivider* getAxsunFreqDivider() const { return m_pAxsunFreqDivider; }
    inline AxsunControl* getAxsunControl() const { return m_pAxsunControl; }

	inline bool isFlimSystemInitialized() const {
		return (m_pFlimFreqDivider != nullptr) && (m_pElforlightLaser != nullptr) && (m_pAxsunFreqDivider != nullptr);
	}
    inline bool isOctSystemInitialized() const { return (m_pAxsunControl != nullptr); }
	
public: 
    // Helical Scanning Control
	bool connectRotaryMotor(bool);
	void rotate(bool);
	void changeRotaryRpm(int);

	bool connectPullbackMotor(bool);
	void moveAbsolute();
    void pullback() { moveAbsolute(); }
    void setTargetSpeed(float);
    void changePullbackLength(float);
	void home();
    void stop();
	
    // FLIm PMT Gain Control
    bool applyPmtGainVoltage(bool);
    void changePmtGainVoltage(float);

    // FLIm Laser Power Control
    bool connectFlimLaser(bool);
	void adjustLaserPower(int);
    void sendLaserCommand(char*);
	
	// FLIm System Control Initialization
	bool startSynchronization(bool enabled, bool async = false);

	// Axsun OCT Control
    bool connectAxsunControl(bool);
	void setLightSource(bool);
	void setLiveImaging(bool);
	void setClockDelay(double);
	void setVDLLength(double);
	void setVDLHome();
    void adjustDecibelRange(double, double);
    void requestOctStatus();
	
//signals:
//	void transferAxsunArray(int);

// Variables ////////////////////////////////////////////
private:
    // Configuration object
    Configuration* m_pConfig;

    // Pullback & Rotary Motor Control
	RotaryMotor* m_pRotaryMotor;
	PullbackMotor* m_pPullbackMotor;

	// NI DAQ class for PMT Gain Control 
	PmtGainControl* m_pPmtGainControl;

	// Elforlight Laser Control
	ElforlightLaser* m_pElforlightLaser;

	// FLIm Synchronization Control
	FreqDivider* m_pFlimFreqDivider;
	FreqDivider* m_pAxsunFreqDivider;

    // Axsun OCT Control
    AxsunControl* m_pAxsunControl;	

	// Status message handling callback
	callback2<const char*, bool> SendStatusMessage;
};

#endif // DEVICECONTROL_H
