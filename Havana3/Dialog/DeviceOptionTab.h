#ifndef DEVICEOPTIONTAB_H
#define DEVICEOPTIONTAB_H

#include <QtWidgets>
#include <QtCore>


class Configuration;
class QPatientSummaryTab;
class QStreamTab;
class QResultTab;
class DeviceControl;

class QMySpinBox : public QDoubleSpinBox
{
public:
    explicit QMySpinBox(QWidget *parent = nullptr) : QDoubleSpinBox(parent)
    {
        lineEdit()->setReadOnly(true);
    }
    virtual ~QMySpinBox() {}
};

class DeviceOptionTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit DeviceOptionTab(QWidget *parent = nullptr);
    virtual ~DeviceOptionTab();

// Methods //////////////////////////////////////////////
public:
	inline QPatientSummaryTab* getPatientSummaryTab() const { return m_pPatientSummaryTab; }
    inline QStreamTab* getStreamTab() const { return m_pStreamTab; }
    inline QResultTab* getResultTab() const { return m_pResultTab; }
    inline DeviceControl* getDeviceControl() const { return m_pDeviceControl; }
    inline QGroupBox* getLayoutBox() const { return m_pGroupBox_DeviceOption; }
	
private:
	void createHelicalScanningControl();
    void createFlimSystemControl();
    void createAxsunOctSystemControl();

private slots:
    // Helical Scanning Control
	void setRotaryComPort(int);
	bool connectRotaryMotor(bool);
	void rotate(bool);
	void changeRotaryRpm(const QString &);
	
	void setPullbackComPort(int);
	bool connectPullbackMotor(bool);
	void moveAbsolute();
	void setTargetSpeed(const QString &);
	void changePullbackLength(const QString &);
	void home();
	void stop();

    // Synchronization port setup
	void setFlimTriggerSource(const QString &);
	void setFlimTriggerChannel(const QString &);
	void setOctTriggerSource(const QString &);
	void setOctTriggerChannel(const QString &);

    // FLIm Laser Synchronization Control
	void startFlimAsynchronization(bool);
    void startFlimSynchronization(bool);

    // FLIm PMT Gain Control
	void setPmtGainPort(const QString &);
    void applyPmtGainVoltage(bool);
	void changePmtGainVoltage(const QString &);

    // FLIm Laser Power Control
	void setFlimLaserComPort(int);
    bool connectFlimLaser(bool);
	void adjustLaserPower(int);

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
private:
    Configuration* m_pConfig;
	QPatientSummaryTab* m_pPatientSummaryTab;
    QStreamTab* m_pStreamTab;
    QResultTab* m_pResultTab;
    DeviceControl* m_pDeviceControl;

private:
    // Device control option widgets
	QGroupBox *m_pGroupBox_DeviceOption;
	QVBoxLayout *m_pVBoxLayout_DeviceOption;

	// Faulhaber rotation motor control widgets
	QLabel *m_pLabel_RotaryConnect;
	QComboBox *m_pComboBox_RotaryConnect;
	QPushButton *m_pToggleButton_RotaryConnect;

	QLabel *m_pLabel_RotationSpeed;
	QLineEdit *m_pLineEdit_RPM;
	QLabel *m_pLabel_RPM;
	QPushButton *m_pToggleButton_Rotate;

    // Faulhaber pullback stage control widgets
	QLabel *m_pLabel_PullbackConnect;
	QComboBox *m_pComboBox_PullbackConnect;
	QPushButton *m_pToggleButton_PullbackConnect;

	QLabel *m_pLabel_PullbackSpeed;
	QLineEdit *m_pLineEdit_PullbackSpeed;
	QLabel *m_pLabel_PullbackSpeedUnit;

	QLabel *m_pLabel_PullbackLength;
	QLineEdit *m_pLineEdit_PullbackLength;
	QLabel *m_pLabel_PullbackLengthUnit;

	QPushButton *m_pPushButton_Pullback;
	QPushButton *m_pPushButton_Home;
	QPushButton *m_pPushButton_PullbackStop;
	
    // FLIm control - Laser sync control widgets
	QLabel *m_pLabel_FlimTriggerSource;
	QLineEdit *m_pLineEdit_FlimTriggerSource;
	QLabel *m_pLabel_FlimTriggerChannel;
	QLineEdit *m_pLineEdit_FlimTriggerChannel;
	QLabel *m_pLabel_OctTriggerSource;
	QLineEdit *m_pLineEdit_OctTriggerSource;
	QLabel *m_pLabel_OctTriggerChannel;
	QLineEdit *m_pLineEdit_OctTriggerChannel;

	QLabel *m_pLabel_AsynchronizedPulsedLaser;
	QPushButton *m_pToggleButton_AsynchronizedPulsedLaser;
	QLabel *m_pLabel_SynchronizedPulsedLaser;
	QPushButton *m_pToggleButton_SynchronizedPulsedLaser;

    // FLIm control	- PMT Gain control widgets
	QLabel *m_pLabel_PmtGainPort;
	QLineEdit *m_pLineEdit_PmtGainPort;

    QLabel *m_pLabel_PmtGainControl;
	QLineEdit *m_pLineEdit_PmtGainVoltage;
    QLabel *m_pLabel_PmtGainVoltage;
    QPushButton *m_pToggleButton_PmtGainVoltage;

    // FLIm control - Laser power control widgets
	QLabel *m_pLabel_FlimLaserConnect;
	QComboBox *m_pComboBox_FlimLaserConnect;
	QPushButton *m_pToggleButton_FlimLaserConnect;

	QLabel *m_pLabel_FlimLaserPowerControl;
	QSpinBox *m_pSpinBox_FlimLaserPowerControl;

    // Axsun OCT control widgets
	QLabel *m_pLabel_AxsunOctConnect;
	QPushButton *m_pToggleButton_AxsunOctConnect;

	QLabel *m_pLabel_LightSource;
    QPushButton *m_pToggleButton_LightSource;

	QLabel *m_pLabel_LiveImaging;
	QPushButton *m_pToggleButton_LiveImaging;
	
    QLabel *m_pLabel_VDLLength;
    QMySpinBox *m_pSpinBox_VDLLength;
	QPushButton *m_pPushButton_VDLHome;
};

#endif // DEVICEOPTIONTAB_H
