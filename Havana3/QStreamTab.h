#ifndef QSTREAMTAB_H
#define QSTREAMTAB_H

#include <QtWidgets>
#include <QtCore>

#include <Common/SyncObject.h>


class MainWindow;
class Configuration;
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
    inline QString getPatientId() { return m_patientId; }
    inline DataAcquisition* getDataAcquisition() const { return m_pDataAcquisition; }
    inline DeviceControl* getDeviceControl() const { return m_pDeviceControl; }
    inline QViewTab* getViewTab() const { return m_pViewTab; }

	inline size_t getFlimProcessingBufferQueueSize() const { return m_syncFlimProcessing.queue_buffer.size(); }
	inline size_t getFlimVisualizationBufferQueueSize() const { return m_syncFlimVisualization.queue_buffer.size(); }
	inline size_t getOctVisualizationBufferQueueSize() const { return m_syncOctVisualization.queue_buffer.size(); }

	//inline auto getFlimProcessingSync() { return &m_syncFlimProcessing; }
	//inline auto getFlimVisualizationSync() { return &m_syncFlimVisualization; }
	//inline auto getOctVisualizationSync() { return &m_syncOctVisualization; }

private:
    void createLiveStreamingViewWidgets();

public:    
    void changeTab(bool status);
    bool enableDataAcquisition(bool);
    bool enableMemoryBuffer(bool);
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
//    void processMessage(QString, bool);

//signals:
//    void sendStatusMessage(QString, bool);

// Variables ////////////////////////////////////////////
private:
    MainWindow* m_pMainWnd;
    Configuration* m_pConfig;
    HvnSqlDataBase* m_pHvnSqlDataBase;

    DataAcquisition* m_pDataAcquisition;
    MemoryBuffer* m_pMemoryBuffer;
    DeviceControl* m_pDeviceControl;

private:
    QString m_patientId;

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

    // Setting dialog
    SettingDlg *m_pSettingDlg;
};

#endif // QSTREAMTAB_H
