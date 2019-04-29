#ifndef QOPERATIONTAB_H
#define QOPERATIONTAB_H

#include <QDialog>
#include <QtWidgets>
#include <QtCore>

class QStreamTab;
class Configuration;

class DataAcquisition;
class MemoryBuffer;


class QOperationTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit QOperationTab(QWidget *parent = nullptr);
    virtual ~QOperationTab();

// Methods //////////////////////////////////////////////
public:
	void setWidgetsStatus();

public:
    inline QVBoxLayout* getLayout() const { return m_pVBoxLayout; }
    inline QStreamTab* getStreamTab() const { return m_pStreamTab; }
    inline DataAcquisition* getDataAcq() const { return m_pDataAcquisition; }
	inline MemoryBuffer* getMemBuff() const { return m_pMemoryBuffer; }
	inline QProgressBar* getProgressBar() const { return m_pProgressBar; }
	inline QPushButton* getAcquisitionButton() const { return m_pToggleButton_Acquisition; }
	
public:
	void changedTab(bool change);

private slots:
	// Slots for widgets
	void operateDataAcquisition(bool toggled);
	void operateDataRecording(bool toggled);
    void operateDataSaving(bool toggled);

public slots :
	void setAcqRecEnable();
	void setSaveButtonDefault(bool error);
    void setAcquisitionButton(bool toggled) { m_pToggleButton_Acquisition->setChecked(toggled); }
	void setRecordingButton(bool toggled) { m_pToggleButton_Recording->setChecked(toggled); }

// Variables ////////////////////////////////////////////
private:	
    QStreamTab* m_pStreamTab;
    Configuration* m_pConfig;

public:
	// Data acquisition and memory operation object
    DataAcquisition* m_pDataAcquisition;
    MemoryBuffer* m_pMemoryBuffer;
	bool m_pAcquisitionState;

private:
	// Layout
	QVBoxLayout *m_pVBoxLayout;

	// Operation buttons
	QPushButton *m_pToggleButton_Acquisition;
	QPushButton *m_pToggleButton_Recording;
    QPushButton *m_pToggleButton_Saving;
	QProgressBar *m_pProgressBar;
};

#endif // QOPERATIONTAB_H
