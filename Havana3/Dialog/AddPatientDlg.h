#ifndef ADDPATIENTDLG_H
#define ADDPATIENTDLG_H

#include <QtWidgets>
#include <QtCore>


class Configuration;
class HvnSqlDataBase;
class QPatientSelectionTab;
class QPatientSummaryTab;

class AddPatientDlg : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit AddPatientDlg(QWidget *parent = 0, const QString& patient_id = "");
    virtual ~AddPatientDlg();
		
// Methods //////////////////////////////////////////////
private:
	void keyPressEvent(QKeyEvent *);

private slots:
    void changePatientId(const QString &);
    void changeFirstName(const QString &);
    void changeLastName(const QString &);

    void add();
    void edit();

private:
    void loadPatientInformation();

// Variables ////////////////////////////////////////////
private:
	Configuration* m_pConfig;
    HvnSqlDataBase* m_pHvnSqlDataBase;
    QPatientSelectionTab* m_pPatientSelectionTab;
    QPatientSummaryTab* m_pPatientSummaryTab;

private:
    QString m_patientId;

private:
    QGroupBox *m_pGroupBox_AddPatient;

    QLabel *m_pLabel_PatientId;
    QLabel *m_pLabel_FirstName;
    QLabel *m_pLabel_LastName;
    QLabel *m_pLabel_DateOfBirth;
    QLabel *m_pLabel_Gender;
    QLabel *m_pLabel_PhysicianName;
    QLabel *m_pLabel_AccessionNumber;
    QLabel *m_pLabel_Keywords;

    QLineEdit *m_pLineEdit_PatientId;
    QLineEdit *m_pLineEdit_FirstName;
    QLineEdit *m_pLineEdit_LastName;
    QDateEdit *m_pDateEdit_DateOfBirth;
    QRadioButton *m_pRadioButton_Male;
    QRadioButton *m_pRadioButton_Female;
    QButtonGroup *m_pButtonGroup_Gender;
    QLineEdit *m_pLineEdit_PhysicianName;
    QLineEdit *m_pLineEdit_AccessionNumber;
    QLineEdit *m_pLineEdit_Keywords;

    QPushButton *m_pPushButton_Ok;
    QPushButton *m_pPushButton_Cancel;
};

#endif // ADDPATIENTDLG_H
