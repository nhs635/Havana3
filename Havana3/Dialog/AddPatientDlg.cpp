
#include "AddPatientDlg.h"

#include <QSqlQuery>

#include <Havana3/MainWindow.h>
#include <Havana3/Configuration.h>
#include <Havana3/HvnSqlDataBase.h>
#include <Havana3/QPatientSelectionTab.h>
#include <Havana3/QPatientSummaryTab.h>


AddPatientDlg::AddPatientDlg(QWidget *parent, const QString& patient_id) :
    QDialog(parent), m_pPatientSelectionTab(nullptr), m_pPatientSummaryTab(nullptr), m_patientId(patient_id)
{
    // Set default size & frame
    setFixedSize(320, 310);
    setWindowFlags(Qt::Tool);
    setWindowTitle((m_patientId == "") ? "Add Patient Information" : "Edit Patient Information");

    // Set main window objects
    if (m_patientId == "")
    {
        m_pPatientSelectionTab = dynamic_cast<QPatientSelectionTab*>(parent);
        m_pConfig = m_pPatientSelectionTab->getMainWnd()->m_pConfiguration;
        m_pHvnSqlDataBase = m_pPatientSelectionTab->getMainWnd()->m_pHvnSqlDataBase;
    }
    else
    {
        m_pPatientSummaryTab = dynamic_cast<QPatientSummaryTab*>(parent);
        m_pConfig = m_pPatientSummaryTab->getMainWnd()->m_pConfiguration;
        m_pHvnSqlDataBase = m_pPatientSummaryTab->getMainWnd()->m_pHvnSqlDataBase;
    }

    // Create widgets for adding patients information
    m_pGroupBox_AddPatient = new QGroupBox(this);
    m_pGroupBox_AddPatient->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
    m_pGroupBox_AddPatient->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    m_pLabel_PatientId = new QLabel(this);
    m_pLabel_PatientId->setText(QString("<font color=\"white\"><b>Patient Id</b></font>"
                                        "<font color=\"%1\"><font size=2> (required)</font></font>").arg(m_patientId == "" ? "red" : "green"));

    m_pLabel_FirstName = new QLabel(this);
    m_pLabel_FirstName->setText(QString("<font color=\"white\"><b>First Name</b></font>"
                                        "<font color=\"%1\"><font size=2> (required)</font></font>").arg(m_patientId == "" ? "red" : "green"));

    m_pLabel_LastName = new QLabel(this);
    m_pLabel_LastName->setText(QString("<font color=\"white\"><b>Last Name</b></font>"
                                       "<font color=\"%1\"><font size=2> (required)</font></font>").arg(m_patientId == "" ? "red" : "green"));

    m_pLabel_DateOfBirth = new QLabel(this);
    m_pLabel_DateOfBirth->setText("Date of Birth");
    m_pLabel_DateOfBirth->setStyleSheet("QLabel{font-weight:bold}");

    m_pLabel_Gender = new QLabel(this);
    m_pLabel_Gender->setText("Gender");
    m_pLabel_Gender->setStyleSheet("QLabel{font-weight:bold}");

    m_pLabel_PhysicianName = new QLabel(this);
    m_pLabel_PhysicianName->setText("Physician Name");
    m_pLabel_PhysicianName->setStyleSheet("QLabel{font-weight:bold}");

    m_pLabel_AccessionNumber = new QLabel(this);
    m_pLabel_AccessionNumber->setText("Accession Number  ");
    m_pLabel_AccessionNumber->setStyleSheet("QLabel{font-weight:bold}");

    m_pLabel_Keywords = new QLabel(this);
    m_pLabel_Keywords->setText("Keywords");
    m_pLabel_Keywords->setStyleSheet("QLabel{font-weight:bold}");

    m_pLineEdit_PatientId = new QLineEdit(this);
    m_pLineEdit_PatientId->setValidator(new QIntValidator(0, 9999999, this));
    m_pLineEdit_PatientId->setFixedWidth(150);

    m_pLineEdit_FirstName = new QLineEdit(this);
    m_pLineEdit_FirstName->setFixedWidth(150);

    m_pLineEdit_LastName = new QLineEdit(this);
    m_pLineEdit_LastName->setFixedWidth(150);

    m_pDateEdit_DateOfBirth = new QDateEdit(this);
    m_pDateEdit_DateOfBirth->setFixedSize(150, 25);
    m_pDateEdit_DateOfBirth->setCalendarPopup(true);

    m_pRadioButton_Male = new QRadioButton(this);
    m_pRadioButton_Male->setText("Male");
    m_pRadioButton_Male->setFixedHeight(25);

    m_pRadioButton_Female = new QRadioButton(this);
    m_pRadioButton_Female->setText("Female");
    m_pRadioButton_Female->setFixedHeight(25);

    m_pButtonGroup_Gender = new QButtonGroup(this);
    m_pButtonGroup_Gender->addButton(m_pRadioButton_Male, 0);
    m_pButtonGroup_Gender->addButton(m_pRadioButton_Female, 1);
    m_pRadioButton_Male->setChecked(true);

    m_pLineEdit_PhysicianName = new QLineEdit(this);
    m_pLineEdit_PhysicianName->setFixedWidth(150);

    m_pLineEdit_AccessionNumber = new QLineEdit(this);
    m_pLineEdit_AccessionNumber->setFixedWidth(150);

    m_pLineEdit_Keywords = new QLineEdit(this);
    m_pLineEdit_Keywords->setFixedWidth(150);

    m_pPushButton_Ok = new QPushButton(this);
    m_pPushButton_Ok->setText("OK");
    m_pPushButton_Ok->setDisabled(m_patientId == "");

    m_pPushButton_Cancel = new QPushButton(this);
    m_pPushButton_Cancel->setText("Cancel");


	// Set layout
    QGridLayout *pGridLayout = new QGridLayout;
    pGridLayout->setAlignment(Qt::AlignCenter);
    pGridLayout->setSpacing(5);

    pGridLayout->addWidget(m_pLabel_PatientId, 0, 0);
    pGridLayout->addWidget(m_pLineEdit_PatientId, 0, 1, 1, 2);

    pGridLayout->addWidget(m_pLabel_FirstName, 1, 0);
    pGridLayout->addWidget(m_pLineEdit_FirstName, 1, 1, 1, 2);

    pGridLayout->addWidget(m_pLabel_LastName, 2, 0);
    pGridLayout->addWidget(m_pLineEdit_LastName, 2, 1, 1, 2);

    pGridLayout->addWidget(m_pLabel_DateOfBirth, 3, 0);
    pGridLayout->addWidget(m_pDateEdit_DateOfBirth, 3, 1, 1, 2);

    pGridLayout->addWidget(m_pLabel_Gender, 4, 0);
    pGridLayout->addWidget(m_pRadioButton_Male, 4, 1);
    pGridLayout->addWidget(m_pRadioButton_Female, 4, 2);

    pGridLayout->addWidget(m_pLabel_PhysicianName, 5, 0);
    pGridLayout->addWidget(m_pLineEdit_PhysicianName, 5, 1, 1, 2);

    pGridLayout->addWidget(m_pLabel_AccessionNumber, 6, 0);
    pGridLayout->addWidget(m_pLineEdit_AccessionNumber, 6, 1, 1, 2);

    pGridLayout->addWidget(m_pLabel_Keywords, 7, 0);
    pGridLayout->addWidget(m_pLineEdit_Keywords, 7, 1, 1, 2);

    QHBoxLayout* pHBoxLayout_Buttons = new QHBoxLayout;
    pHBoxLayout_Buttons->setSpacing(3);
    pHBoxLayout_Buttons->setAlignment(Qt::AlignRight);

    pHBoxLayout_Buttons->addWidget(m_pPushButton_Ok);
    pHBoxLayout_Buttons->addWidget(m_pPushButton_Cancel);

    m_pGroupBox_AddPatient->setLayout(pGridLayout);


    QVBoxLayout* pVBoxLayout = new QVBoxLayout;
    pVBoxLayout->setSpacing(5);

    pVBoxLayout->addWidget(new QLabel("Enter patient information.", this));
    pVBoxLayout->addWidget(m_pGroupBox_AddPatient);
    pVBoxLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
    pVBoxLayout->addItem(pHBoxLayout_Buttons);

    this->setLayout(pVBoxLayout);

    // Connect
    connect(m_pLineEdit_PatientId, SIGNAL(textEdited(const QString &)), this, SLOT(changePatientId(const QString &)));
    connect(m_pLineEdit_FirstName, SIGNAL(textEdited(const QString &)), this, SLOT(changeFirstName(const QString &)));
    connect(m_pLineEdit_LastName, SIGNAL(textEdited(const QString &)), this, SLOT(changeLastName(const QString &)));
    if (m_pPatientSelectionTab)
        connect(m_pPushButton_Ok, SIGNAL(clicked(bool)), this, SLOT(add()));
    else
        connect(m_pPushButton_Ok, SIGNAL(clicked(bool)), this, SLOT(edit()));
    connect(m_pPushButton_Cancel, SIGNAL(clicked(bool)), this, SLOT(accept()));

    // Initialization
    if (m_pPatientSummaryTab) loadPatientInformation();
}

AddPatientDlg::~AddPatientDlg()
{
}

void AddPatientDlg::keyPressEvent(QKeyEvent *e)
{
    if (e->key() != Qt::Key_Escape)
        QDialog::keyPressEvent(e);
}


void AddPatientDlg::changePatientId(const QString &str)
{
    if (str == "")
    {
        m_pLabel_PatientId->setText("<font color=\"white\"><b>Patient Id</b></font>"
                                    "<font color=\"red\"><font size=2> (required)</font></font>");
        m_pPushButton_Ok->setDisabled(true);
    }
    else
    {
        m_pLabel_PatientId->setText("<font color=\"white\"><b>Patient Id</b></font>"
                                    "<font color=\"green\"><font size=2> (required)</font></font>");

        QString first_name = m_pLineEdit_FirstName->text();
        QString last_name = m_pLineEdit_LastName->text();
        m_pPushButton_Ok->setEnabled((first_name != "") && (last_name != ""));
    }
}

void AddPatientDlg::changeFirstName(const QString &str)
{
    if (str == "")
    {
        m_pLabel_FirstName->setText("<font color=\"white\"><b>First Name</b></font>"
                                    "<font color=\"red\"><font size=2> (required)</font></font>");
        m_pPushButton_Ok->setDisabled(true);
    }
    else
    {
        m_pLabel_FirstName->setText("<font color=\"white\"><b>First Name</b></font>"
                                    "<font color=\"green\"><font size=2> (required)</font></font>");

        QString patient_id = m_pLineEdit_PatientId->text();
        QString last_name = m_pLineEdit_LastName->text();
        m_pPushButton_Ok->setEnabled((patient_id != "") && (last_name != ""));
    }
}

void AddPatientDlg::changeLastName(const QString &str)
{
    if (str == "")
    {
        m_pLabel_LastName->setText("<font color=\"white\"><b>Last Name</b></font>"
                                   "<font color=\"red\"><font size=2> (required)</font></font>");
        m_pPushButton_Ok->setDisabled(true);
    }
    else
    {
        m_pLabel_LastName->setText("<font color=\"white\"><b>Last Name</b></font>"
                                   "<font color=\"green\"><font size=2> (required)</font></font>");

        QString patient_id = m_pLineEdit_PatientId->text();
        QString first_name = m_pLineEdit_FirstName->text();
        m_pPushButton_Ok->setEnabled((patient_id != "") && (first_name != ""));
    }
}


void AddPatientDlg::add()
{
    QString patient_id = m_pLineEdit_PatientId->text();
    QString first_name = m_pLineEdit_FirstName->text();
    QString last_name = m_pLineEdit_LastName->text();
    QString date_of_birth = m_pDateEdit_DateOfBirth->date().toString("yyyy-MM-dd");
    int gender = m_pButtonGroup_Gender->checkedId();
    QString physician_name = m_pLineEdit_PhysicianName->text();
    QString accession_number = m_pLineEdit_AccessionNumber->text();
    QString keywords = m_pLineEdit_Keywords->text();

    QString command = QString("INSERT INTO patients(first_name, last_name, patient_id, dob, gender, accession_number, notes, physician_name) "
                              "VALUES('%1', '%2', '%3', '%4', %5, '%6', '%7', '%8')").arg(first_name).arg(last_name).arg(patient_id)
                              .arg(date_of_birth).arg(gender).arg(accession_number).arg(keywords).arg(physician_name);

    if (m_pHvnSqlDataBase->queryDatabase(command))
    {
        m_pPatientSelectionTab->loadPatientDatabase();
        for (int i = 0; i < m_pPatientSelectionTab->getTableWidgetPatientInformation()->rowCount(); i++)
        {
            if (m_pPatientSelectionTab->getTableWidgetPatientInformation()->item(i, 1)->text() == patient_id)
            {
                m_pPatientSelectionTab->getTableWidgetPatientInformation()->selectRow(i);
                break;
            }
        }

        this->accept();
    }
}

void AddPatientDlg::edit()
{
    QString patient_id = m_pLineEdit_PatientId->text();
    QString first_name = m_pLineEdit_FirstName->text();
    QString last_name = m_pLineEdit_LastName->text();
    QString date_of_birth = m_pDateEdit_DateOfBirth->date().toString("yyyy-MM-dd");
    int gender = m_pButtonGroup_Gender->checkedId();
    QString physician_name = m_pLineEdit_PhysicianName->text();
    QString accession_number = m_pLineEdit_AccessionNumber->text();
    QString keywords = m_pLineEdit_Keywords->text();

    QString command = QString("UPDATE patients SET first_name='%1', last_name='%2', dob='%4', gender=%5, accession_number='%6', notes='%7', physician_name='%8' WHERE patient_id='%3'")
                              .arg(first_name).arg(last_name).arg(patient_id).arg(date_of_birth).arg(gender).arg(accession_number).arg(keywords).arg(physician_name);

    if (m_pHvnSqlDataBase->queryDatabase(command))
    {
        m_pPatientSummaryTab->loadPatientInformation();

        this->accept();
    }
}

void AddPatientDlg::loadPatientInformation()
{
    QString command = QString("SELECT * FROM patients WHERE patient_id=%1").arg(m_patientId);

    m_pHvnSqlDataBase->queryDatabase(command, [&](QSqlQuery& _sqlQuery) {

        while (_sqlQuery.next())
        {
            m_pLineEdit_PatientId->setText(_sqlQuery.value(3).toString());
            m_pLineEdit_FirstName->setText(_sqlQuery.value(0).toString());
            m_pLineEdit_LastName->setText(_sqlQuery.value(1).toString());
            m_pDateEdit_DateOfBirth->setDate(_sqlQuery.value(4).toDate());
            if (_sqlQuery.value(5).toInt() == MALE)
                m_pRadioButton_Male->setChecked(true);
            else
                m_pRadioButton_Female->setChecked(true);
            m_pLineEdit_PhysicianName->setText(_sqlQuery.value(8).toString());
            m_pLineEdit_AccessionNumber->setText(_sqlQuery.value(6).toString());
            m_pLineEdit_Keywords->setText(_sqlQuery.value(7).toString());

        }
    });

    m_pLineEdit_PatientId->setDisabled(true);
}

