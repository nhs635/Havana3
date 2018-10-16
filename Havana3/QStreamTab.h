#ifndef QSTREAMTAB_H
#define QSTREAMTAB_H

#include <QDialog>
#include <QtWidgets>
#include <QtCore>

#include <Havana3/Configuration.h>

#include <Common/array.h>
#include <Common/SyncObject.h>

class MainWindow;
class QOperationTab;
class QDeviceControlTab;
class QVisualizationTab;

class ThreadManager;
class FLImProcess;


class QStreamTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit QStreamTab(QWidget *parent = nullptr);
	virtual ~QStreamTab();
	
// Methods //////////////////////////////////////////////
protected:
	void keyPressEvent(QKeyEvent *);

public:
	inline MainWindow* getMainWnd() const { return m_pMainWnd; }
    inline QOperationTab* getOperationTab() const { return m_pOperationTab; }
    inline QDeviceControlTab* getDeviceControlTab() const { return m_pDeviceControlTab; }

	inline size_t getFlimProcessingBufferQueueSize() const { return m_syncFlimProcessing.queue_buffer.size(); }
	inline size_t getFlimVisualizationBufferQueueSize() const { return m_syncFlimVisualization.queue_buffer.size(); }
	inline size_t getOctVisualizationBufferQueueSize() const { return m_syncOctVisualization.queue_buffer.size(); }

	//inline auto getFlimProcessingSync() { return &m_syncFlimProcessing; }
	//inline auto getFlimVisualizationSync() { return &m_syncFlimVisualization; }
	//inline auto getOctVisualizationSync() { return &m_syncOctVisualization; }

public:
    void changeTab(bool status);

private:		
// Set thread callback objects
    void setFlimAcquisitionCallback();
	void setFlimProcessingCallback();
	void setOctProcessingCallback();
    void setVisualizationCallback();

private slots:
    void processMessage(QString, bool);

signals:
    void sendStatusMessage(QString, bool);

// Variables ////////////////////////////////////////////
private:
    MainWindow* m_pMainWnd;
    Configuration* m_pConfig;

    QOperationTab* m_pOperationTab;
    QDeviceControlTab* m_pDeviceControlTab;
    QVisualizationTab* m_pVisualizationTab;

	// Message Window
	QListWidget *m_pListWidget_MsgWnd;

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
    // Layout
    QHBoxLayout *m_pHBoxLayout;

    // Group Boxes
    QGroupBox *m_pGroupBox_OperationTab;
    QGroupBox *m_pGroupBox_DeviceControlTab;
    QGroupBox *m_pGroupBox_VisualizationTab;
};

#endif // QSTREAMTAB_H
