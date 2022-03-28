#ifndef QPATIENTSUMMARY_H
#define QPATIENTSUMMARY_H

#include <QtWidgets>
#include <QtCore>


class MainWindow;
class Configuration;
class HvnSqlDataBase;
class AddPatientDlg;
class SettingDlg;
class DeviceControl;

struct RecordInfo
{
	QString recordId = "-1";
	QString patientId;
	QString patientName;
	QString	title = "pullback";
	QString date;
	QByteArray preview;
	int vessel = 0;
	int procedure = 0;
	QString filename0;
	QString filename;
	QString comment = "";
};

class QPatientSummaryTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit QPatientSummaryTab(QString patient_id, QWidget *parent = nullptr);
    virtual ~QPatientSummaryTab();
	
// Methods //////////////////////////////////////////////
protected:
	void keyPressEvent(QKeyEvent *);
	    
public:
    inline MainWindow* getMainWnd() const { return m_pMainWnd; }
    inline RecordInfo getPatientInfo() { return m_patientInfo; }
	inline bool getCatheterConnectionMode() { return m_pToggleButton_CatheterConnect->isChecked(); }
    inline QTableWidget* getTableWidgetRecordInformation() const { return m_pTableWidget_RecordInformation; }

private:
    void createPatientSummaryViewWidgets();
    void createPatientSummaryTable();

private slots:
	void catheterConnect(bool);
    void newRecord();
	void exportRawData(bool);
	void showHiddenData(bool);
	void import();
    void editPatient();
    void deleteEditPatientDlg();
	void createSettingDlg();
	void deleteSettingDlg();
	void editContents(int, int);
	void deleteRecordData(const QString &);

public:
    void loadPatientInformation();
    void loadRecordDatabase();

signals:
    void requestNewRecord(const QString &);
    void requestReview(const QString &);
	void requestDelete(const QString &);
///    void sendStatusMessage(QString, bool);

// Variables ////////////////////////////////////////////
private:
    MainWindow* m_pMainWnd;
    Configuration* m_pConfig;
    HvnSqlDataBase* m_pHvnSqlDataBase;
	DeviceControl* m_pDeviceControl;
	
private:
    RecordInfo m_patientInfo;
	
private:
    // Widgets for patient summary view
    QGroupBox *m_pGroupBox_PatientSummary;

    QLabel *m_pLabel_PatientSummary;
    QLabel *m_pLabel_PatientInformation;

	QPushButton *m_pToggleButton_CatheterConnect;
    QPushButton *m_pPushButton_NewRecord;
	QPushButton *m_pToggleButton_ShowHiddenData;

	QPushButton *m_pToggleButton_Export;
	QPushButton *m_pPushButton_Import;
    QPushButton *m_pPushButton_EditPatient;
	QPushButton *m_pPushButton_Setting;

    QTableWidget *m_pTableWidget_RecordInformation;

    // Dialogs
    AddPatientDlg *m_pEditPatientDlg;
	SettingDlg *m_pSettingDlg;
};

#endif // QPATIENTSUMMARYTAB_H
