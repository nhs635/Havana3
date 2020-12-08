#ifndef QSTREAMTAB_H
#define QSTREAMTAB_H

#include <QtWidgets>
#include <QtCore>

#include <Havana3/Configuration.h>
#include <Havana3/QPatientSummaryTab.h>

#include <Common/SyncObject.h>


class MainWindow;
class HvnSqlDataBase;

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
	void keyPressEvent(QKeyEvent *);

public:
	inline MainWindow* getMainWnd() const { return m_pMainWnd; }
	inline RecordInfo getRecordInfo() { return m_recordInfo; }
    inline DataAcquisition* getDataAcquisition() const { return m_pDataAcquisition; }
    inline DeviceControl* getDeviceControl() const { return m_pDeviceControl; }
    inline QViewTab* getViewTab() const { return m_pViewTab; }

	inline void setFirstImplemented(bool impl) { m_bFirstImplemented = impl; }
	inline bool getFirstImplemented() { return m_bFirstImplemented; }

	inline size_t getFlimProcessingBufferQueueSize() const { return m_syncFlimProcessing.queue_buffer.size(); }
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
	void setFlimProcessingCallback();
	void setOctProcessingCallback();
    void setVisualizationCallback();

private slots:
    void createSettingDlg();
    void deleteSettingDlg();
    void scrollCatheterCalibration(int);
    void enableRotation(bool);
    void startPullback(bool);
#ifdef DEVELOPER_MODE
	void onTimerSyncMonitor();
#endif

signals:
	void devInit();
	void getCapture(QByteArray &);
	void requestReview(const QString &);

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
	bool m_bFirstImplemented;
	QTimer *m_pCaptureTimer;
#ifdef DEVELOPER_MODE
	QTimer *m_pSyncMonitorTimer;
#endif

public:
	// Thread manager objects
	ThreadManager* m_pThreadFlimProcess;
	ThreadManager* m_pThreadVisualization;

private:
	// Thread synchronization objects
	SyncObject<uint16_t> m_syncFlimProcessing;
	SyncObject<float> m_syncFlimVisualization;
	SyncObject<uint8_t> m_syncOctVisualization;

private:
    // Live streaming view control widget
    QGroupBox *m_pGroupBox_LiveStreaming;

    QLabel *m_pLabel_PatientInformation;
    QPushButton *m_pPushButton_Setting;

    QViewTab* m_pViewTab;

    QLabel *m_pLabel_CatheterCalibration;
    QScrollBar *m_pScrollBar_CatheterCalibration;

    QPushButton *m_pToggleButton_EnableRotation;
    QPushButton *m_pToggleButton_StartPullback;

#ifdef DEVELOPER_MODE
	QLabel *m_pLabel_StreamingSyncStatus;
#endif

    // Setting dialog
    SettingDlg *m_pSettingDlg;
};

#endif // QSTREAMTAB_H
