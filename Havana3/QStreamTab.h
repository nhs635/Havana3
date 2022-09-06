#ifndef QSTREAMTAB_H
#define QSTREAMTAB_H

#include <QtWidgets>
#include <QtCore>

#include <Havana3/Configuration.h>
#include <Havana3/QPatientSummaryTab.h>

#include <Common/SyncObject.h>

#include <iostream>
#include <mutex>


class MainWindow;
class HvnSqlDataBase;

class QScope;
class QViewTab;
class SettingDlg;

class DataAcquisition;
class MemoryBuffer;
class DeviceControl;

class ThreadManager;

class QStreamTab : public QDialog
{	
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit QStreamTab(QString patient_id = "", QWidget *parent = nullptr);
	virtual ~QStreamTab();
	
// Methods //////////////////////////////////////////////
protected:
	void closeEvent(QCloseEvent *);
	void keyPressEvent(QKeyEvent *);

public:
	inline MainWindow* getMainWnd() const { return m_pMainWnd; }
	inline RecordInfo getRecordInfo() { return m_recordInfo; }
    inline DataAcquisition* getDataAcquisition() const { return m_pDataAcquisition; }
    inline DeviceControl* getDeviceControl() const { return m_pDeviceControl; }
    inline QViewTab* getViewTab() const { return m_pViewTab; }
	inline QScrollBar* getCalibScrollBar() const { return m_pScrollBar_CatheterCalibration; }
	inline SettingDlg* getSettingDlg() const { return m_pSettingDlg; }
#ifdef DEVELOPER_MODE
	inline QScope* getAlineScope() const { return m_pScope_Alines; }
#endif
	inline size_t getFlimProcessingBufferQueueSize() const { return m_syncFlimProcessing.queue_buffer.size(); }
	inline size_t getOctProcessingBufferQueueSize() const { return m_syncOctProcessing.queue_buffer.size(); }
	inline size_t getFlimVisualizationBufferQueueSize() const { return m_syncFlimVisualization.queue_buffer.size(); }
	inline size_t getOctVisualizationBufferQueueSize() const { return m_syncOctVisualization.queue_buffer.size(); }
	
private:
    void createLiveStreamingViewWidgets();

public:
	void changePatient(QString patient_id);

public:    
	void startLiveImaging(bool);

private:
	bool enableMemoryBuffer(bool);
    bool enableDataAcquisition(bool);
    bool enableDeviceControl(bool);

private:		
// Set thread callback objects
    void setFlimAcquisitionCallback();
	void setOctAcquisitionCallback();
	void setFlimProcessingCallback();
	void setOctProcessingCallback();
    void setVisualizationCallback();

private slots:
    void createSettingDlg();
    void deleteSettingDlg();
	void resetCatheterCalibration();
    void scrollCatheterCalibration(int);
	void scrollInnerOffsetLength(int);
    
public slots:
	void enableRotation(bool);

private slots:
    void startPullback(bool);
#ifdef DEVELOPER_MODE
	void onTimerSyncMonitor();
	void onTimerLaserMonitor();
#endif

signals:
	void deviceInitialized();
	void deviceTerminate();
	void pullbackFinished(bool);
	void getCapture(QByteArray &);
	void requestReview(const QString &, int frame = -1);
#ifdef DEVELOPER_MODE
	void setStreamingSyncStatusLabel(const QString &);
	void setLaserStatusLabel(const QString &);
	void plotAline(const float*);
#endif

// Variables ////////////////////////////////////////////
private:
    MainWindow* m_pMainWnd;
    Configuration* m_pConfig;
    HvnSqlDataBase* m_pHvnSqlDataBase;

    DataAcquisition* m_pDataAcquisition;
    MemoryBuffer* m_pMemoryBuffer;
    DeviceControl* m_pDeviceControl;

private:
	RecordInfo m_recordInfo;

private:
	QTimer *m_pCaptureTimer;
#ifdef DEVELOPER_MODE
	QTimer *m_pSyncMonitorTimer;
	QTimer *m_pLaserMonitorTimer;
#endif

private:
	std::mutex m_mtxInit;

public:
	// Thread manager objects
	ThreadManager* m_pThreadFlimProcess;
	ThreadManager* m_pThreadOctProcess;
	ThreadManager* m_pThreadVisualization;

	// Thread synchronization objects
	SyncObject<uint16_t> m_syncFlimProcessing;
#ifndef NEXT_GEN_SYSTEM
	SyncObject<uint8_t> m_syncOctProcessing;
#else
	SyncObject<float> m_syncOctProcessing;
#endif
	SyncObject<float> m_syncFlimVisualization;
	SyncObject<uint8_t> m_syncOctVisualization;

private:
    // Live streaming view control widget
    QGroupBox *m_pGroupBox_LiveStreaming;

    QLabel *m_pLabel_PatientInformation;
    QPushButton *m_pPushButton_Setting;

    QViewTab* m_pViewTab;

    QLabel *m_pLabel_CatheterCalibration;
	QPushButton *m_pPushButton_CatheterCalibrationReset;
    QScrollBar *m_pScrollBar_CatheterCalibration;
	QScrollBar *m_pScrollBar_InnerOffset;
	
    QPushButton *m_pToggleButton_EnableRotation;
    QPushButton *m_pToggleButton_StartPullback;

#ifdef DEVELOPER_MODE
	QScope *m_pScope_Alines;
	QLabel *m_pLabel_StreamingSyncStatus;
	QLabel *m_pLabel_LaserStatus;
#endif

    // Setting dialog
    SettingDlg *m_pSettingDlg;
};

#endif // QSTREAMTAB_H
