
#include "QPatientSelectionTab.h"

#include <QSqlQuery>

#include <Havana3/MainWindow.h>
#include <Havana3/Configuration.h>
#include <Havana3/HvnSqlDataBase.h>
#include <Havana3/Dialog/AddPatientDlg.h>


QPatientSelectionTab::QPatientSelectionTab(QWidget *parent) :
    QDialog(parent), m_pAddPatientDlg(nullptr)
{
    // Set title
    setWindowTitle("Patient Selection");

	// Set main window objects
    m_pMainWnd = dynamic_cast<MainWindow*>(parent);
    m_pConfig = m_pMainWnd->m_pConfiguration;
    m_pHvnSqlDataBase = m_pMainWnd->m_pHvnSqlDataBase;

    // Create widgets for patient selection view
    createPatientSelectionViewWidgets();

    // Set window layout
    QHBoxLayout* pHBoxLayout = new QHBoxLayout;
    pHBoxLayout->setSpacing(2);

    pHBoxLayout->addWidget(m_pGroupBox_PatientSelection);

    this->setLayout(pHBoxLayout);
}

QPatientSelectionTab::~QPatientSelectionTab()
{
}

void QPatientSelectionTab::keyPressEvent(QKeyEvent *e)
{
	if (e->key() != Qt::Key_Escape)
        QDialog::keyPressEvent(e);
}


void QPatientSelectionTab::createPatientSelectionViewWidgets()
{
    // Create widgets for patient selection view
    m_pGroupBox_PatientSelection = new QGroupBox(this);
    m_pGroupBox_PatientSelection->setFixedSize(1200, 550);
    m_pGroupBox_PatientSelection->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
    m_pGroupBox_PatientSelection->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_pLabel_PatientSelection = new QLabel(this);
    m_pLabel_PatientSelection->setText(" Patient Selection");
    m_pLabel_PatientSelection->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    m_pLabel_PatientSelection->setStyleSheet("QLabel{font-size:20pt; font-weight:bold}");

    m_pPushButton_AddPatient = new QPushButton(this);
    m_pPushButton_AddPatient->setText("  Add Patient");
    m_pPushButton_AddPatient->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    m_pPushButton_AddPatient->setFixedSize(130, 25);

    m_pPushButton_RemovePatient = new QPushButton(this);
    m_pPushButton_RemovePatient->setText("  Remove Patient");
    m_pPushButton_RemovePatient->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    m_pPushButton_RemovePatient->setFixedSize(130, 25);

    m_pLabel_DatabaseLocation = new QLabel(this);
    m_pLabel_DatabaseLocation->setText("  Database Location ");

    m_pLineEdit_DatabaseLocation = new QLineEdit(this);
    m_pLineEdit_DatabaseLocation->setText(m_pConfig->dbPath);
    m_pLineEdit_DatabaseLocation->setFixedWidth(600);

    m_pPushButton_DatabaseLocation = new QPushButton(this);
    m_pPushButton_DatabaseLocation->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    m_pPushButton_DatabaseLocation->setFixedSize(40, 25);

    createPatientSelectionTable();

    // Set layout: patient selection view
    QGridLayout *pGridLayout_PatientSelection = new QGridLayout;
    pGridLayout_PatientSelection->setSpacing(8);

    QHBoxLayout *pHBoxLayout_DatabaseLocation = new QHBoxLayout;
    pHBoxLayout_DatabaseLocation->setAlignment(Qt::AlignRight);
    pHBoxLayout_DatabaseLocation->setSpacing(2);
    pHBoxLayout_DatabaseLocation->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
    pHBoxLayout_DatabaseLocation->addWidget(m_pLabel_DatabaseLocation);
    pHBoxLayout_DatabaseLocation->addWidget(m_pLineEdit_DatabaseLocation);
    pHBoxLayout_DatabaseLocation->addWidget(m_pPushButton_DatabaseLocation);

    pGridLayout_PatientSelection->addWidget(m_pLabel_PatientSelection, 0, 0, 1, 3);
    pGridLayout_PatientSelection->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 0);
    pGridLayout_PatientSelection->addWidget(m_pPushButton_AddPatient, 1, 1);
    pGridLayout_PatientSelection->addWidget(m_pPushButton_RemovePatient, 1, 2);

    pGridLayout_PatientSelection->addWidget(m_pTableWidget_PatientInformation, 2, 0, 1, 3);
    pGridLayout_PatientSelection->addItem(pHBoxLayout_DatabaseLocation, 3, 0, 1, 3);

    m_pGroupBox_PatientSelection->setLayout(pGridLayout_PatientSelection);


    // Connect signal and slot
    connect(m_pPushButton_AddPatient, SIGNAL(clicked(bool)), this, SLOT(addPatient()));
    connect(m_pPushButton_RemovePatient, SIGNAL(clicked(bool)), this, SLOT(removePatient()));
    connect(m_pLineEdit_DatabaseLocation, SIGNAL(textChanged(const QString &)), this, SLOT(editDatabaseLocation(const QString &)));
    connect(m_pPushButton_DatabaseLocation, SIGNAL(clicked(bool)), this, SLOT(findDatabaseLocation()));
}

void QPatientSelectionTab::createPatientSelectionTable()
{
    m_pTableWidget_PatientInformation = new QTableWidget(this);
    m_pTableWidget_PatientInformation->setRowCount(0);
    m_pTableWidget_PatientInformation->setColumnCount(7);

    QStringList colLabels;
    colLabels << "Patient Name" << "Patient ID" << "Gender" << "Date of Birth" << "Last Case (Total)" << "Physician" << "Keywords";
    m_pTableWidget_PatientInformation->setHorizontalHeaderLabels(colLabels);    

    // Cell items properties
    m_pTableWidget_PatientInformation->setAlternatingRowColors(true);   // Focus 잃으면 Deselect 되도록 하기
    m_pTableWidget_PatientInformation->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pTableWidget_PatientInformation->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pTableWidget_PatientInformation->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pTableWidget_PatientInformation->setTextElideMode(Qt::ElideRight);

    // Table properties
    m_pTableWidget_PatientInformation->setShowGrid(true);
    m_pTableWidget_PatientInformation->setGridStyle(Qt::DotLine);
    m_pTableWidget_PatientInformation->setSortingEnabled(true);
    m_pTableWidget_PatientInformation->setCornerButtonEnabled(true);

    // Header properties
    m_pTableWidget_PatientInformation->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_pTableWidget_PatientInformation->horizontalHeader()->setMinimumSectionSize(100);
    m_pTableWidget_PatientInformation->horizontalHeader()->setStretchLastSection(true);
    m_pTableWidget_PatientInformation->setColumnWidth(0, 200);
    m_pTableWidget_PatientInformation->setColumnWidth(1, 100);
    m_pTableWidget_PatientInformation->setColumnWidth(2, 100);
    m_pTableWidget_PatientInformation->setColumnWidth(3, 120);
    m_pTableWidget_PatientInformation->setColumnWidth(4, 150);
    m_pTableWidget_PatientInformation->setColumnWidth(5, 150);
}


void QPatientSelectionTab::addPatient()
{
    if (m_pAddPatientDlg == nullptr)
    {
        m_pAddPatientDlg = new AddPatientDlg(this);
        connect(m_pAddPatientDlg, SIGNAL(finished(int)), this, SLOT(deleteAddPatientDlg()));
        m_pAddPatientDlg->setModal(true);
        m_pAddPatientDlg->exec();
    }
}

void QPatientSelectionTab::deleteAddPatientDlg()
{
    m_pAddPatientDlg = nullptr;
}

void QPatientSelectionTab::removePatient()
{
    QMessageBox MsgBox(QMessageBox::Question, "Confirm remove", "Do you want to remove the patient information?");
    MsgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    MsgBox.setDefaultButton(QMessageBox::No);

    switch (MsgBox.exec())
    {
    case QMessageBox::Yes:
        break;
    case QMessageBox::No:
        return;
    default:
        return;
    }

    int current_row = m_pTableWidget_PatientInformation->currentRow();
    m_pHvnSqlDataBase->queryDatabase(QString("DELETE FROM patients WHERE patient_id='%1'")
                                     .arg(m_pTableWidget_PatientInformation->item(current_row, 1)->text()));
    m_pTableWidget_PatientInformation->removeRow(current_row);
}

void QPatientSelectionTab::editDatabaseLocation(const QString &dbPath)
{
    m_pConfig->dbPath = dbPath;
}

void QPatientSelectionTab::findDatabaseLocation()
{
    QString dbPath = QFileDialog::getExistingDirectory(nullptr, "Select Database...", m_pConfig->dbPath,
                                                       QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog);
    if (dbPath != "")
    {
        m_pLineEdit_DatabaseLocation->setText(dbPath);

        if (!QDir().exists(dbPath + "\\db.sqlite"))
            m_pHvnSqlDataBase->initializeDatabase();  // DataBase 로케이션을 My Document로 기본적으로 만들게 하기??? QDir::HomeDir 써서...

        loadPatientDatabase();
    }
	else
	{
		// else인 경우에 어떻게 처리할 지...
	}
}


void QPatientSelectionTab::loadPatientDatabase()
{
    m_pTableWidget_PatientInformation->setRowCount(0);
    m_pTableWidget_PatientInformation->setSortingEnabled(false);

    m_pHvnSqlDataBase->queryDatabase("SELECT * FROM patients", [&](QSqlQuery& _sqlQuery) {

        int rowCount = 0;
        while (_sqlQuery.next())
        {
            m_pTableWidget_PatientInformation->insertRow(rowCount);

            QTableWidgetItem *pNameItem = new QTableWidgetItem;
            QTableWidgetItem *pIdItem = new QTableWidgetItem;
            QTableWidgetItem *pGenderItem = new QTableWidgetItem;
            QTableWidgetItem *pDobItem = new QTableWidgetItem;
            QTableWidgetItem *pLastCaseItem = new QTableWidgetItem;
            QTableWidgetItem *pPhysicianItem = new QTableWidgetItem;
            QTableWidgetItem *pKeywordsItem = new QTableWidgetItem;

            QDate last_date(1, 1, 1); int total = 0;
            QString command = QString("SELECT * FROM records WHERE patient_id=%1").arg(_sqlQuery.value(3).toString());
            m_pHvnSqlDataBase->queryDatabase(command, [&](QSqlQuery& __sqlQuery) {
                while (__sqlQuery.next())
                {
                    QDate date = QDateTime::fromString(__sqlQuery.value(3).toString(), "yyyy-MM-dd hh:mm:ss").date();

                    if (date > last_date)
                        last_date = date;
                    total++;
                }
            }, true);

            pNameItem->setText(_sqlQuery.value(1).toString() + ", " + _sqlQuery.value(0).toString());
            pIdItem->setText(_sqlQuery.value(3).toString());
            pGenderItem->setText(m_pHvnSqlDataBase->getGender(_sqlQuery.value(5).toInt()));
            pDobItem->setText(_sqlQuery.value(4).toString());
            pLastCaseItem->setText((total != 0 ? last_date.toString("yyyy-MM-dd") : "N/A") + QString(" (%1)").arg(total));
            pPhysicianItem->setText(_sqlQuery.value(8).toString());
            pKeywordsItem->setText(_sqlQuery.value(7).toString());

            pIdItem->setTextAlignment(Qt::AlignCenter);
            pGenderItem->setTextAlignment(Qt::AlignCenter);
            pDobItem->setTextAlignment(Qt::AlignCenter);
            pLastCaseItem->setTextAlignment(Qt::AlignCenter);

            m_pTableWidget_PatientInformation->setItem(rowCount, 0, pNameItem);
            m_pTableWidget_PatientInformation->setItem(rowCount, 1, pIdItem);
            m_pTableWidget_PatientInformation->setItem(rowCount, 2, pGenderItem);
            m_pTableWidget_PatientInformation->setItem(rowCount, 3, pDobItem);
            m_pTableWidget_PatientInformation->setItem(rowCount, 4, pLastCaseItem);
            m_pTableWidget_PatientInformation->setItem(rowCount, 5, pPhysicianItem);
            m_pTableWidget_PatientInformation->setItem(rowCount, 6, pKeywordsItem);

            rowCount++;
        }
    });

    m_pTableWidget_PatientInformation->setSortingEnabled(true);
}
