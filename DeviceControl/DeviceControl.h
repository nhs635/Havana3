#ifndef DEVICECONTROL_H
#define DEVICECONTROL_H

#include <QObject>


class Configuration;

class PullbackMotor;
class RotaryMotor;

class FreqDivider;
class PmtGainControl;
class ElforlightLaser;

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
	inline PullbackMotor* getPullbackMotor() const { return m_pPullbackMotor; }
    inline RotaryMotor* getRotatyMotor() const { return m_pRotaryMotor; }
    inline FreqDivider* getFlimFreqDivider() const { return m_pFlimFreqDivider; }
    inline FreqDivider* getAxsunFreqDivider() const { return m_pAxsunFreqDivider; }
    inline PmtGainControl* getPmtGainControl() const { return m_pPmtGainControl; }
    inline ElforlightLaser* getElforlightLaser() const { return m_pElforlightLaser; }
    inline AxsunControl* getAxsunControl() const { return m_pAxsunControl; }

	inline bool isFlimSystemInitialized() const {
        return (m_pFlimFreqDivider != nullptr) && (m_pElforlightLaser != nullptr); //  && (m_pAxsunFreqDivider != nullptr)
	}
    inline bool isOctSystemInitialized() const { return (m_pAxsunControl != nullptr); }
	
public: 
    // Helical Scanning Control
	bool connectPullbackMotor(bool);
	void moveAbsolute();
    void pullback() { moveAbsolute(); }
    void setTargetSpeed(float);
    void changePullbackLength(float);
	void home();
    void stop();

	bool connectRotaryMotor(bool);
	void rotate(bool);
    void changeRotaryRpm(int);

    // FLIm System Control Initialization
//    void initializeFlimSystem(bool);
//	bool isDaqBoardConnected();

    // FLIm-Axsun Synchronization Control
//	void startFlimAsynchronization(bool);
    bool startSynchronization(bool);

    // FLIm PMT Gain Control
    bool applyPmtGainVoltage(bool);
    void changePmtGainVoltage(float);

    // FLIm Laser Power Control
    bool connectFlimLaser(bool);
	void adjustLaserPower(int);
    void sendLaserCommand(char*);

	// Axsun OCT Control
    bool connectAxsunControl(bool);
	void setLightSource(bool);
	void setLiveImaging(bool);
	void setClockDelay(double);
	void setVDLLength(double);
	void setVDLHome();
    void adjustDecibelRange(double, double);
    void requestOctStatus();
//	void setVDLWidgets(bool);
	
signals:
	void transferAxsunArray(int);

// Variables ////////////////////////////////////////////
private:
    // Configuration object
    Configuration* m_pConfig;

    // Pullback & Rotary Motor Control
	PullbackMotor* m_pPullbackMotor;
	RotaryMotor* m_pRotaryMotor;

	// NI DAQ class for PMT Gain Control & FLIm Synchronization Control
	FreqDivider* m_pFlimFreqDivider;
	FreqDivider* m_pAxsunFreqDivider;
	PmtGainControl* m_pPmtGainControl;

	// Elforlight Laser Control
	ElforlightLaser* m_pElforlightLaser;

    // Axsun OCT Control
    AxsunControl* m_pAxsunControl;	
};

#endif // DEVICECONTROL_H
