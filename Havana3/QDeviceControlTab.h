#ifndef QDEVICECONTROLTAB_H
#define QDEVICECONTROLTAB_H

#include <QDialog>
#include <QtWidgets>
#include <QtCore>

#include <Havana3/Configuration.h>

class QStreamTab;

#ifndef NI_ENABLE
class ArduinoMCU;
#else
class FreqDivider;
class PmtGainControl;
#endif

class ElforlightLaser;
class FlimCalibDlg;

class AxsunControl;

class ZaberStage;
class FaulhaberMotor;

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
    inline FlimCalibDlg* getFlimCalibDlg() const { return m_pFlimCalibDlg; }    
    inline QCheckBox* getFlimControl() const { return m_pCheckBox_FlimControl; }
    inline QCheckBox* getPX14DigitizerControl() const { return m_pCheckBox_PX14DigitizerControl; }
	inline QCheckBox* getAxsunControl() const { return m_pCheckBox_AxsunOctControl; }
	inline QPushButton* getAxsunImagingControl() const { return m_pToggleButton_LiveImaging; }
	///inline QPushButton* getAxsunBgSetControl() const { return m_pPushButton_SetBackground; }
	///inline QPushButton* getAxsunBgResetControl() const { return m_pPushButton_ResetBackground; }
	inline QImageView* getOctDbColorbar() const { return m_pImageView_Colorbar; }
    inline ZaberStage* getZaberStage() const { return m_pZaberStage; }

private: ////////////////////////////////////////////////////////////////////////////////////////////////
    void createFlimSynchronizationControl();
    void createPmtGainControl();
	void createFlimLaserPowerControl();
    void createFLimCalibControl();
    void createAxsunOctControl();
    void createZaberStageControl();
    void createFaulhaberMotorControl();

public: ////////////////////////////////////////////////////////////////////////////////////////////////
    // Zaber Stage Control
    bool isZaberStageEnabled() { return m_pCheckBox_ZaberStageControl->isChecked(); }
    void pullback() { moveAbsolute(); }

    // Faulhaber Motor Control
	bool isFaulhaberMotorEnabled() { return m_pCheckBox_FaulhaberMotorControl->isChecked(); }
	void stopMotor() { m_pToggleButton_Rotate->setChecked(false); }

private slots: /////////////////////////////////////////////////////////////////////////////////////////
    // FLIm Control Initialization
    void connectFlimControl(bool);

    // FLIm Laser Synchronization Control
    void startFlimSynchronization(bool);
	void setFlimSyncAdjust(int);

    // FLIm PMT Gain Control
    void applyPmtGainVoltage(bool);
#ifndef NI_ENABLE
    void changePmtGainVoltage(double);
#else
	void changePmtGainVoltage(const QString &);
#endif

    // FLIm Laser Power Control
    void connectFlimLaser(bool);
	void adjustLaserPower(int);

    // FLIm Calibration Control
    void connectPX14Digitizer(bool);
    void setPX14DcOffset(int);
    void createFlimCalibDlg();
    void deleteFlimCalibDlg();

	// Axsun OCT Control
	void connectAxsunControl(bool);
	void setLightSource(bool);
	void setLiveImaging(bool);
	void setBackground();
	void resetBackground();
	void loadBackground();
	void setDispComp();
	void setClockDelay(double);
	void setVDLLength(double);
	void setVDLHome();
	void adjustDecibelRange();

    // Zaber Stage Control
    void connectZaberStage(bool);
    void moveAbsolute();
    void setTargetSpeed(const QString &);
    void changeZaberPullbackLength(const QString &);
    void home();
    void stop();

	// Faulhaber Motor Control
    void connectFaulhaberMotor(bool);
	void rotate(bool);
	void changeFaulhaberRpm(const QString &);
	
signals:
	void transferAxsunArray(int);

// Variables ////////////////////////////////////////////
private: ////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef NI_ENABLE
    // Arduino MCU for PMT Gain Control & FLIm Synchronization Control
    ArduinoMCU* m_pArduinoMCU;
#else
	// NI DAQ class for PMT Gain Control & FLIm Synchronization Control
	FreqDivider* m_pFlimFreqDivider;
	FreqDivider* m_pAxsunFreqDivider;
	PmtGainControl* m_pPmtGainControl;
#endif

	// Elforlight Laser Control
	ElforlightLaser* m_pElforlightLaser;

    // Axsun OCT Control
    AxsunControl* m_pAxsunControl;

    // Zaber Stage Control
    ZaberStage* m_pZaberStage;

	// Faulhaber Motor Control
	FaulhaberMotor* m_pFaulhaberMotor;
	
private: ////////////////////////////////////////////////////////////////////////////////////////////////
    QStreamTab* m_pStreamTab;
    Configuration* m_pConfig;

    // Layout
    QVBoxLayout *m_pVBoxLayout;

	// FLIM Layout
	QVBoxLayout *m_pVBoxLayout_FlimControl;
	QGroupBox *m_pGroupBox_FlimControl;
    QCheckBox *m_pCheckBox_FlimControl;

    // Widgets for FLIm control // Laser sync control
    QPushButton *m_pToggleButton_FlimSynchronization;
	QLabel *m_pLabel_FlimSyncAdjust;
	QSlider *m_pSlider_FlimSyncAdjust;

    // Widgets for FLIm control	// Gain control
    QLabel *m_pLabel_PmtGainControl;
#ifndef NI_ENABLE
    QDoubleSpinBox *m_pSpinBox_PmtGainControl;
#else
	QLineEdit *m_pLineEdit_PmtGainVoltage;
#endif
    QLabel *m_pLabel_PmtGainVoltage;
    QPushButton *m_pToggleButton_PmtGainVoltage;

    // Widgets for FLIm control // Laser power control
    QCheckBox *m_pCheckBox_FlimLaserPowerControl;
	QSpinBox *m_pSpinBox_FlimLaserPowerControl;

    // Widgets for FLIm control // Calibration
    QCheckBox *m_pCheckBox_PX14DigitizerControl;
    QLabel *m_pLabel_PX14DcOffset;
    QSlider *m_pSlider_PX14DcOffset;
    QPushButton *m_pPushButton_FlimCalibration;
    FlimCalibDlg *m_pFlimCalibDlg;

    // Widgets for Axsun OCT control
    QCheckBox *m_pCheckBox_AxsunOctControl;

    QPushButton *m_pToggleButton_LightSource;
	QPushButton *m_pToggleButton_LiveImaging;

    QLabel *m_pLabel_Background;
    QPushButton *m_pPushButton_SetBackground;
    QPushButton *m_pPushButton_ResetBackground;
    QLabel *m_pLabel_DispCompCoef;
    QLineEdit *m_pLineEdit_a2;
    QLineEdit *m_pLineEdit_a3;
    QPushButton *m_pPushButton_DispComp;

    QLabel *m_pLabel_ClockDelay;
    QMySpinBox *m_pSpinBox_ClockDelay;

    QLabel *m_pLabel_VDLLength;
    QMySpinBox *m_pSpinBox_VDLLength;
	QPushButton *m_pPushButton_VDLHome;

    QLabel *m_pLabel_DecibelRange;
    QLineEdit *m_pLineEdit_DecibelMax;
    QLineEdit *m_pLineEdit_DecibelMin;
    QImageView *m_pImageView_Colorbar;

    // Widgets for Zaber stage control
    QCheckBox *m_pCheckBox_ZaberStageControl;
    QPushButton *m_pPushButton_SetTargetSpeed;
    QPushButton *m_pPushButton_MoveAbsolute;
    QPushButton *m_pPushButton_Home;
    QPushButton *m_pPushButton_Stop;
    QLineEdit *m_pLineEdit_TargetSpeed;
    QLineEdit *m_pLineEdit_TravelLength;
    QLabel *m_pLabel_TargetSpeed;
    QLabel *m_pLabel_TravelLength;

    // Widgets for Faulhaber motor control
    QCheckBox *m_pCheckBox_FaulhaberMotorControl;
    QPushButton *m_pToggleButton_Rotate;
    QLineEdit *m_pLineEdit_RPM;
    QLabel *m_pLabel_RPM;
};

#endif // QDEVICECONTROLTAB_H
