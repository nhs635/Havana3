#ifndef QDEVICECONTROLTAB_H
#define QDEVICECONTROLTAB_H

#include <QDialog>
#include <QtWidgets>
#include <QtCore>

#include <Havana3/Configuration.h>

class QStreamTab;

class ZaberStage;
class FaulhaberMotor;

class FreqDivider;
class PmtGainControl;

class ElforlightLaser;
class FlimCalibDlg;

class AxsunControl;

class QImageView;

class QMySpinBox : public QDoubleSpinBox
{
public:
    explicit QMySpinBox(QWidget *parent = nullptr) : QDoubleSpinBox(parent)
    {
        lineEdit()->setReadOnly(true);
    }
    virtual ~QMySpinBox() {}
};


class QDeviceControlTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit QDeviceControlTab(QWidget *parent = nullptr);
    virtual ~QDeviceControlTab();

// Methods //////////////////////////////////////////////
protected:
	void closeEvent(QCloseEvent*);

public: ////////////////////////////////////////////////////////////////////////////////////////////////
	void setControlsStatus();

public: ////////////////////////////////////////////////////////////////////////////////////////////////
    inline QVBoxLayout* getLayout() const { return m_pVBoxLayout; }
    inline QStreamTab* getStreamTab() const { return m_pStreamTab; }
	inline ZaberStage* getZaberStage() const { return m_pZaberStage; }
	inline bool isFlimSystemInitialized() const {
        return (m_pFlimFreqDivider != nullptr) && (m_pElforlightLaser != nullptr); //  && (m_pAxsunFreqDivider != nullptr)
	}
	inline bool isOctSystemInitialized() const { return (m_pAxsunControl != nullptr); }
    inline FlimCalibDlg* getFlimCalibDlg() const { return m_pFlimCalibDlg; }   

private: ////////////////////////////////////////////////////////////////////////////////////////////////
	void createHelicalScanningControl();
    void createFlimSystemControl();
    void createAxsunOctSystemControl();

public: ////////////////////////////////////////////////////////////////////////////////////////////////
	// Helical Scanning Control
	void setHelicalScanningControl(bool);
	bool isZaberStageEnabled() { return m_pZaberStage != nullptr; }
	void pullback() { moveAbsolute(); }
	bool isFaulhaberMotorEnabled() { return m_pFaulhaberMotor != nullptr; }
	void stopMotor() { rotate(false); }

	// FLIm Control
	void setFlimControl(bool);
	void sendLaserCommand(char*);

	// Axsun OCT Control
	void setAxsunControl(bool);
	void adjustDecibelRange();
	void requestOctStatus();
	
private slots: /////////////////////////////////////////////////////////////////////////////////////////
    // Helical Scanning Control
	void initializeHelicalScanning(bool);

	bool connectZaberStage(bool);
	void moveAbsolute();
	void setTargetSpeed(const QString &);
	void changeZaberPullbackLength(const QString &);
	void home();
	void stop();

	bool connectFaulhaberMotor(bool);
	void rotate(bool);
	void changeFaulhaberRpm(const QString &);

    // FLIm System Control Initialization
    void initializeFlimSystem(bool);
	bool isDaqBoardConnected();

    // FLIm Laser Synchronization Control
	void startFlimAsynchronization(bool);
    void startFlimSynchronization(bool);

    // FLIm PMT Gain Control
    void applyPmtGainVoltage(bool);
	void changePmtGainVoltage(const QString &);

    // FLIm Laser Power Control
    bool connectFlimLaser(bool);
	void adjustLaserPower(int);

    // FLIm Calibration Control
    void createFlimCalibDlg();
    void deleteFlimCalibDlg();

	// Axsun OCT Control
	void connectAxsunControl(bool);
	void setLightSource(bool);
	void setLiveImaging(bool);
	void setClockDelay(double);
	void setVDLLength(double);
	void setVDLHome();
	void setVDLWidgets(bool);
	
signals:
	void transferAxsunArray(int);

// Variables ////////////////////////////////////////////
private: ////////////////////////////////////////////////////////////////////////////////////////////////
    // Zaber Stage Control
	ZaberStage* m_pZaberStage;

	// Faulhaber Motor Control
	FaulhaberMotor* m_pFaulhaberMotor;

	// NI DAQ class for PMT Gain Control & FLIm Synchronization Control
	FreqDivider* m_pFlimFreqDivider;
	FreqDivider* m_pAxsunFreqDivider;
	PmtGainControl* m_pPmtGainControl;

	// Elforlight Laser Control
	ElforlightLaser* m_pElforlightLaser;

    // Axsun OCT Control
    AxsunControl* m_pAxsunControl;
	
private: ////////////////////////////////////////////////////////////////////////////////////////////////
    QStreamTab* m_pStreamTab;
    Configuration* m_pConfig;

    // Layout
    QVBoxLayout *m_pVBoxLayout;
	QGroupBox *m_pGroupBox_FlimControl;
	QGroupBox *m_pGroupBox_AxsunOctControl;
	QGroupBox *m_pGroupBox_HelicalScanningControl;
	
	// Widgets for Zaber pullback stage control
	QLabel *m_pLabel_PullbackSpeed;
	QLineEdit *m_pLineEdit_PullbackSpeed;
	QLabel *m_pLabel_PullbackSpeedUnit;

	QLabel *m_pLabel_PullbackLength;
	QLineEdit *m_pLineEdit_PullbackLength;
	QLabel *m_pLabel_PullbackLengthUnit;

	QPushButton *m_pPushButton_Pullback;
	QPushButton *m_pPushButton_Home;
	QPushButton *m_pPushButton_PullbackStop;

	// Widgets for Faulhaber rotation motor control
	QLabel *m_pLabel_RotationSpeed;
	QLineEdit *m_pLineEdit_RPM;
	QLabel *m_pLabel_RPM;
	QPushButton *m_pToggleButton_Rotate;
	
    // Widgets for FLIm control // Laser sync control
	QLabel *m_pLabel_AsynchronizedPulsedLaser;
	QPushButton *m_pToggleButton_AsynchronizedPulsedLaser;
	QLabel *m_pLabel_SynchronizedPulsedLaser;
	QPushButton *m_pToggleButton_SynchronizedPulsedLaser;

    // Widgets for FLIm control	// Gain control
    QLabel *m_pLabel_PmtGainControl;
	QLineEdit *m_pLineEdit_PmtGainVoltage;
    QLabel *m_pLabel_PmtGainVoltage;
    QPushButton *m_pToggleButton_PmtGainVoltage;

    // Widgets for FLIm control // Laser power control
	QLabel *m_pLabel_FlimLaserPowerControl;
	QSpinBox *m_pSpinBox_FlimLaserPowerControl;

    // Widgets for FLIm control // Calibration
    QPushButton *m_pPushButton_FlimCalibration;
    FlimCalibDlg *m_pFlimCalibDlg;

    // Widgets for Axsun OCT control
	QLabel *m_pLabel_LightSource;
    QPushButton *m_pToggleButton_LightSource;

	QLabel *m_pLabel_LiveImaging;
	QPushButton *m_pToggleButton_LiveImaging;
	
    QLabel *m_pLabel_VDLLength;
    QMySpinBox *m_pSpinBox_VDLLength;
	QPushButton *m_pPushButton_VDLHome;
};

#endif // QDEVICECONTROLTAB_H
