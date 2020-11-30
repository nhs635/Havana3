#ifndef QPATIENTSUMMARY_H
#define QPATIENTSUMMARY_H

#include <QtWidgets>
#include <QtCore>


class MainWindow;
class Configuration;
class HvnSqlDataBase;
class AddPatientDlg;

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
    inline QString getPatientId() { return m_patientId; }
    inline QTableWidget* getTableWidgetRecordInformation() const { return m_pTableWidget_RecordInformation; }

private:
    void createPatientSummaryViewWidgets();
    void createPatientSummaryTable();

private slots:
    void newRecord();
    void editPatient();
    void deleteEditPatientDlg();
    void modifyRecordComment(int, int);
    void finishedModifyRecordComment(QTableWidgetItem*);
	void deleteRecordData(QString);
//    void reviewRequested();

public:
    void loadPatientInformation();
    void loadRecordDatabase();

signals:
    void requestNewRecord(QString);
    void requestReview(QString);
	void requestDelete(QString);
//    void sendStatusMessage(QString, bool);

// Variables ////////////////////////////////////////////
private:
    MainWindow* m_pMainWnd;
    Configuration* m_pConfig;
    HvnSqlDataBase* m_pHvnSqlDataBase;
	
private:
    QString m_patientId;
	
private:
    // Widgets for patient summary view
    QGroupBox *m_pGroupBox_PatientSummary;

    QLabel *m_pLabel_PatientSummary;
    QLabel *m_pLabel_PatientInformation;

    QPushButton *m_pPushButton_NewRecord;
    QPushButton *m_pPushButton_EditPatient;

    QTableWidget *m_pTableWidget_RecordInformation;

    // Edit Patient Dialog
    AddPatientDlg *m_pEditPatientDlg;
};

#endif // QPATIENTSUMMARYTAB_H
