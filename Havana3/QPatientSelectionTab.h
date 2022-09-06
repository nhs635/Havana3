#ifndef QPATIENTSELECTIONTAB_H
#define QPATIENTSELECTIONTAB_H

#include <QtWidgets>
#include <QtCore>


class MainWindow;
class Configuration;
class HvnSqlDataBase;
class AddPatientDlg;

class QPatientSelectionTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit QPatientSelectionTab(QWidget *parent = nullptr);
    virtual ~QPatientSelectionTab();
	
// Methods //////////////////////////////////////////////
protected:
	void keyPressEvent(QKeyEvent *);
	    
public:
    inline MainWindow* getMainWnd() const { return m_pMainWnd; }
    inline QTableWidget* getTableWidgetPatientInformation() const { return m_pTableWidget_PatientInformation; }

private:
    void createPatientSelectionViewWidgets();
    void createPatientSelectionTable();

private slots:
    void addPatient();
    void deleteAddPatientDlg();
    void removePatient();
	void searchData();
    void editDatabaseLocation(const QString &);
    void findDatabaseLocation();
	
public:
    void loadPatientDatabase();

// Variables ////////////////////////////////////////////
private:
    MainWindow* m_pMainWnd;
    Configuration* m_pConfig;
    HvnSqlDataBase* m_pHvnSqlDataBase;
	
private:
	// Widgets for patient selection view
    QGroupBox *m_pGroupBox_PatientSelection;

    QLabel *m_pLabel_PatientSelection;

	QPushButton *m_pPushButton_SearchData;
    QPushButton *m_pPushButton_AddPatient;
    QPushButton *m_pPushButton_RemovePatient;

    QTableWidget *m_pTableWidget_PatientInformation;

    QLabel *m_pLabel_DatabaseLocation;
    QLineEdit *m_pLineEdit_DatabaseLocation;
    QPushButton *m_pPushButton_DatabaseLocation;

    // Add Patient Dialog
    AddPatientDlg *m_pAddPatientDlg;
};

#endif // QPATIENTSELECTIONTAB_H
