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
	bool connectRotaryMotor(bool);	
	void changeRotaryRpm(int);
	void rotateStop();
	
	bool connectPullbackMotor(bool);
	void setPullbackMode(int);
	void moveAbsolute();
	void home();
	void stop();
	void setPullbackWidgets(bool);
	
    // FLIm Laser Synchronization Control
	void startFlimAsynchronization(bool);
    void startFlimSynchronization(bool);

    // FLIm PMT Gain Control
    void applyPmtGainVoltage(bool);
	void changePmtGainVoltage(const QString &);

    // FLIm Laser Power Control
    bool connectFlimLaser(bool);
	void adjustLaserPower(int);
	void changeFlimLaserPower(const QString &);
	void applyFlimLaserPower();

	// Axsun OCT Control
	void connectAxsunControl(bool);
	void setLightSource(bool);
	void setLiveImaging(bool);
	void setBackground();
	void resetBackground();
	void setDispersionCompensation();
	void setClockDelay(double);
	void setVDLLength(double);
	void setVDLHome();
	void setVDLWidgets(bool);
	
signals:
	void stopPullback(bool);
	void showCurrentUVPower(int);

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
	QPushButton *m_pToggleButton_RotaryConnect;

	QLabel *m_pLabel_RotationSpeed;
	QSpinBox *m_pSpinBox_RPM;
	QLabel *m_pLabel_RPM;
	QPushButton *m_pPushButton_RotateStop;

    // Faulhaber pullback stage control widgets
	QLabel *m_pLabel_PullbackConnect;
	QPushButton *m_pToggleButton_PullbackConnect;

	QLabel *m_pLabel_PullbackMode;
	QRadioButton *m_pRadioButton_20mms50mm;
	QRadioButton *m_pRadioButton_10mms30mm;
	QRadioButton *m_pRadioButton_10mms40mm;
	QButtonGroup *m_pButtonGroup_PullbackMode;

	QLabel *m_pLabel_PullbackOperation;
	QPushButton *m_pPushButton_Pullback;
	QPushButton *m_pPushButton_Home;
	QPushButton *m_pPushButton_PullbackStop;
	
    // FLIm control - Laser sync control widgets
	QLabel *m_pLabel_AsynchronizedPulsedLaser;
	QPushButton *m_pToggleButton_AsynchronizedPulsedLaser;
	QLabel *m_pLabel_SynchronizedPulsedLaser;
	QPushButton *m_pToggleButton_SynchronizedPulsedLaser;

    // FLIm control	- PMT Gain control widgets
    QLabel *m_pLabel_PmtGainControl;
	QLineEdit *m_pLineEdit_PmtGainVoltage;
    QLabel *m_pLabel_PmtGainVoltage;
    QPushButton *m_pToggleButton_PmtGainVoltage;

    // FLIm control - Laser power control widgets
	QLabel *m_pLabel_FlimLaserConnect;
	QPushButton *m_pToggleButton_FlimLaserConnect;

	QLabel *m_pLabel_FlimLaserPowerControl;
	QLabel *m_pLabel_FlimLaserPowerLevel;
	QSpinBox *m_pSpinBox_FlimLaserPowerControl;
	QLineEdit *m_pLineEdit_FlimLaserPowerControl;
	QPushButton *m_pPushButton_FlimLaserPowerControl;

    // Axsun OCT control widgets
	QLabel *m_pLabel_AxsunOctConnect;
	QPushButton *m_pToggleButton_AxsunOctConnect;

	QLabel *m_pLabel_LightSource;
    QPushButton *m_pToggleButton_LightSource;

	QLabel *m_pLabel_LiveImaging;
	QPushButton *m_pToggleButton_LiveImaging;

	QLabel *m_pLabel_BackgroundSubtraction;
	QPushButton *m_pPushButton_BgSet;
	QPushButton *m_pPushButton_BgReset;

	QLabel *m_pLabel_DispersionCompensation;
	QLineEdit *m_pLineEdit_DispComp_a2;
	QLineEdit *m_pLineEdit_DispComp_a3;
	QPushButton *m_pPushButton_Compensate;
	
    QLabel *m_pLabel_VDLLength;
    QMySpinBox *m_pSpinBox_VDLLength;
	QPushButton *m_pPushButton_VDLHome;
};

#endif // DEVICEOPTIONTAB_H
