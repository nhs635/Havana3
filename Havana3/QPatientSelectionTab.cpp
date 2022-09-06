
#include "QPatientSelectionTab.h"

#include <QSqlQuery>

#include <Havana3/MainWindow.h>
#include <Havana3/Configuration.h>
#include <Havana3/QPatientSummaryTab.h>
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
	else
	{
		m_pTableWidget_PatientInformation->clearSelection();
		m_pTableWidget_PatientInformation->clearFocus();
	}
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

	m_pPushButton_SearchData = new QPushButton(this);
	m_pPushButton_SearchData->setText("  Search Data");
	m_pPushButton_SearchData->setIcon(style()->standardIcon(QStyle::SP_FileDialogContentsView));
	m_pPushButton_SearchData->setFixedSize(130, 25);

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
	m_pLineEdit_DatabaseLocation->setReadOnly(true);
    m_pLineEdit_DatabaseLocation->setFixedWidth(450);

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
	pHBoxLayout_DatabaseLocation->addWidget(m_pPushButton_SearchData);
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
	connect(m_pPushButton_SearchData, SIGNAL(clicked(bool)), this, SLOT(searchData()));
    connect(m_pLineEdit_DatabaseLocation, SIGNAL(textChanged(const QString &)), this, SLOT(editDatabaseLocation(const QString &)));
    connect(m_pPushButton_DatabaseLocation, SIGNAL(clicked(bool)), this, SLOT(findDatabaseLocation()));
}

void QPatientSelectionTab::createPatientSelectionTable()
{
    m_pTableWidget_PatientInformation = new QTableWidget(this);
	m_pTableWidget_PatientInformation->clearContents();
	m_pTableWidget_PatientInformation->setRowCount(0);
    m_pTableWidget_PatientInformation->setColumnCount(7);

    QStringList colLabels;
    colLabels << "Patient Name" << "Patient ID" << "Gender" << "Date of Birth (Age)" << "Last Case (Total)" << "Physician" << "Keywords";
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
	m_pTableWidget_PatientInformation->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);
    m_pTableWidget_PatientInformation->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	m_pTableWidget_PatientInformation->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    m_pTableWidget_PatientInformation->horizontalHeader()->setMinimumSectionSize(60);
    m_pTableWidget_PatientInformation->horizontalHeader()->setStretchLastSection(true);
    m_pTableWidget_PatientInformation->setColumnWidth(0, 190);
    m_pTableWidget_PatientInformation->setColumnWidth(1, 90);
    m_pTableWidget_PatientInformation->setColumnWidth(2, 60);
    m_pTableWidget_PatientInformation->setColumnWidth(3, 140);
    m_pTableWidget_PatientInformation->setColumnWidth(4, 140);
    m_pTableWidget_PatientInformation->setColumnWidth(5, 140);
}


void QPatientSelectionTab::addPatient()
{
    if (m_pAddPatientDlg == nullptr)
    {
        m_pAddPatientDlg = new AddPatientDlg(this);
        connect(m_pAddPatientDlg, SIGNAL(finished(int)), this, SLOT(deleteAddPatientDlg()));
		m_pAddPatientDlg->setAttribute(Qt::WA_DeleteOnClose, true);
        m_pAddPatientDlg->setModal(true);
        m_pAddPatientDlg->exec();
    }
}

void QPatientSelectionTab::deleteAddPatientDlg()
{
	m_pAddPatientDlg->deleteLater();
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
	QString patient_id = QString("%1").arg(m_pTableWidget_PatientInformation->item(current_row, 1)->text().toInt());
	QString patient_name = m_pTableWidget_PatientInformation->item(current_row, 0)->text();

	foreach(QDialog* pTabView, m_pMainWnd->getVectorTabViews())
	{
		if (pTabView->windowTitle().contains(patient_name))
			m_pMainWnd->removeTabView(pTabView);
	}

    m_pHvnSqlDataBase->queryDatabase(QString("DELETE FROM patients WHERE patient_id='%1'").arg(patient_id));
    m_pTableWidget_PatientInformation->removeRow(current_row);

	m_pConfig->writeToLog(QString("Patient info removed: %1 (ID: %2)").arg(patient_name).arg(patient_id));
}

void QPatientSelectionTab::searchData()
{
	QDialog *pDialog = new QDialog(this);
	{
		QTextEdit *pTextEdit_Data = new QTextEdit(this);
		pTextEdit_Data->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		pTextEdit_Data->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		pTextEdit_Data->setAcceptRichText(false);
		
		QPushButton *pPushButton_Find = new QPushButton(this);
		pPushButton_Find->setText("Find");

		QGridLayout *pGridLayout = new QGridLayout;
		pGridLayout->addWidget(pTextEdit_Data, 0, 0, 1, 2);
		pGridLayout->addWidget(pPushButton_Find, 1, 1, Qt::AlignRight);
		pDialog->setLayout(pGridLayout);

		connect(pPushButton_Find, &QPushButton::clicked, [&, pTextEdit_Data]() {

			// Find databse
			QString text = pTextEdit_Data->toPlainText();
			
			QString pt_name = "", pt_id = "", acq_date = "", record_id = "";
			int frame = -1;

			// Find patient name, id, acquisition date
			if (text != "")
			{
				QStringList pb_data = text.split("]");				
				if (pb_data.size() > 0)
				{
					pt_name = pb_data[0];
					pt_name = pt_name.replace("[", "");
					qDebug() << pt_name;

					acq_date = pb_data[1];
					acq_date = acq_date.right(acq_date.length() - 1);
					acq_date = acq_date.left(19);
					qDebug() << acq_date;
				}

				QStringList frame_data = text.split("/");				
				if (frame_data.size() > 0)
				{
					QStringList frame_data1 = frame_data[0].split(":");
					if (frame_data1.size() > 0)
					{
						frame = frame_data1.back().toInt();
						qDebug() << frame;
					}
				}
			}

			// Find patient id & load his/her imaging database
			if (pt_name != "")
			{
				m_pHvnSqlDataBase->queryDatabase("SELECT * FROM patients", [&](QSqlQuery& _sqlQuery) {
					while (_sqlQuery.next())
					{
						QString _pt_name = _sqlQuery.value(1).toString() + ", " + _sqlQuery.value(0).toString();
						if (_pt_name == pt_name)
						{
							pt_id = QString("%1").arg(_sqlQuery.value(3).toString().toInt(), 8, 10, QChar('0'));
							break;
						}
					}
				});

				for (int i = 0; i < m_pTableWidget_PatientInformation->rowCount(); i++)
					if (m_pTableWidget_PatientInformation->item(i, 1)->text() == pt_id)
						emit m_pTableWidget_PatientInformation->cellDoubleClicked(i, 0);
								
				// Load the specified pullback
				if (acq_date != "")
				{
					QString command = QString("SELECT * FROM records WHERE patient_id=%1").arg(pt_id);
					m_pHvnSqlDataBase->queryDatabase(command, [&](QSqlQuery& _sqlQuery) {
						while (_sqlQuery.next())
						{
							QString comment = _sqlQuery.value(8).toString();
							if (!comment.contains("[HIDDEN]"))
							{
								QString _acq_date = _sqlQuery.value(3).toString();
								if (_acq_date == acq_date)
									record_id = _sqlQuery.value(0).toString();
							}
						}
					});

					foreach(QDialog* pTabView, m_pMainWnd->getVectorTabViews())
					{
						if (pTabView->windowTitle().contains("Summary"))
						{
							QString _pt_id = QString("%1").arg(((QPatientSummaryTab*)pTabView)->getPatientInfo().patientId.toInt(), 8, 10, QChar('0'));
							if (_pt_id == pt_id)
								emit((QPatientSummaryTab*)pTabView)->requestReview(record_id, frame);
						}
					}
				}
			}
			
			pDialog->close();
		});
	}
	pDialog->setWindowTitle("Search Data");
	pDialog->setFixedSize(300, 120);
	pDialog->setModal(true);
	pDialog->exec();
}

void QPatientSelectionTab::editDatabaseLocation(const QString &dbPath)
{
    m_pConfig->dbPath = dbPath;
}

void QPatientSelectionTab::findDatabaseLocation()
{
	QString dbPath = QFileDialog::getExistingDirectory(nullptr, "Select Database...", QDir::homePath(),
                                                       QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog);
	QString prevDbPath = m_pConfig->dbPath;
    if (dbPath != "")
    {
        m_pLineEdit_DatabaseLocation->setText(dbPath);

		m_pHvnSqlDataBase->closeDatabase();
		if (m_pHvnSqlDataBase->openDatabase(m_pHvnSqlDataBase->getUserName(), m_pHvnSqlDataBase->getPassword()))
			loadPatientDatabase();
		else
		{
			m_pLineEdit_DatabaseLocation->setText(prevDbPath);
			if (m_pHvnSqlDataBase->openDatabase(m_pHvnSqlDataBase->getUserName(), m_pHvnSqlDataBase->getPassword()))
				loadPatientDatabase();
		}
    }
}


void QPatientSelectionTab::loadPatientDatabase()
{
	// Get patient database
	int currentRow = m_pTableWidget_PatientInformation->currentRow();

	m_pTableWidget_PatientInformation->clearContents();
	m_pTableWidget_PatientInformation->setRowCount(0);
    m_pTableWidget_PatientInformation->setSortingEnabled(false);

	QStringList dataset;

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

			QDate last_date(1, 1, 1); QTime last_time(0, 0, 0);  int total = 0;
            QString command = QString("SELECT * FROM records WHERE patient_id=%1").arg(_sqlQuery.value(3).toString());
            m_pHvnSqlDataBase->queryDatabase(command, [&, _sqlQuery](QSqlQuery& __sqlQuery) {
                while (__sqlQuery.next())
                {
					QString comment = __sqlQuery.value(8).toString();
					if (!comment.contains("[HIDDEN]"))
					{
						dataset << (QString("%1").arg(_sqlQuery.value(3).toString().toInt(), 8, 10, QChar('0'))) + " / " + (_sqlQuery.value(1).toString() + ", " + _sqlQuery.value(0).toString())
							+ " / " + __sqlQuery.value(3).toString();

						QTime time = QDateTime::fromString(__sqlQuery.value(3).toString(), "yyyy-MM-dd hh:mm:ss").time();
						QDate date = QDateTime::fromString(__sqlQuery.value(3).toString(), "yyyy-MM-dd hh:mm:ss").date();

						if (date > last_date)
							last_date = date;
						if (time > last_time)
							last_time = time;

						total++;
					}
                }
            }, true);

			QDate date_of_birth = QDate::fromString(_sqlQuery.value(4).toString(), "yyyy-MM-dd");
			QDate date_today = QDate::currentDate();
			int age = (int)((double)date_of_birth.daysTo(date_today) / 365.25);

            pNameItem->setText(_sqlQuery.value(1).toString() + ", " + _sqlQuery.value(0).toString());
            pIdItem->setText(QString("%1").arg(_sqlQuery.value(3).toString().toInt(), 8, 10, QChar('0')));
            pGenderItem->setText(m_pHvnSqlDataBase->getGender(_sqlQuery.value(5).toInt()));
            pDobItem->setText(QString("%1 (%2)").arg(_sqlQuery.value(4).toString()).arg(age));
            pLastCaseItem->setText((total != 0 ? last_date.toString("yyyy-MM-dd") : "N/A") + QString(" (%1)").arg(total));
			pLastCaseItem->setToolTip(last_date.toString("yyyy-MM-dd") + " " + last_time.toString("hh:mm:ss"));
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

			if (!QDir().exists(m_pConfig->dbPath + "/record/" + _sqlQuery.value(3).toString()))
				QDir().mkdir(m_pConfig->dbPath + "/record/" + _sqlQuery.value(3).toString());

            rowCount++;
        }

		m_pConfig->writeToLog(QString("Patient database loaded: %1").arg(m_pConfig->dbPath));
    });
	
	QFile file("dataset_paths.csv");
	if (file.open(QFile::WriteOnly))
	{
		QTextStream stream(&file);

		for (int i = 0; i < dataset.size(); i++)
		{
			QStringList _dataset = dataset.at(i).split("/");

			stream << _dataset.at(0) << "\t" // ID
				<< _dataset.at(1) << "\t" // Name
				<< _dataset.at(2) << "\n"; // Acq date
		}
		file.close();
	}


    m_pTableWidget_PatientInformation->setSortingEnabled(true);
	m_pTableWidget_PatientInformation->sortItems(6, Qt::DescendingOrder);
	QStringList numbers;
	for (int i = m_pTableWidget_PatientInformation->rowCount(); i > 0; i--)
		numbers << QString::number(i);
	m_pTableWidget_PatientInformation->setVerticalHeaderLabels(numbers);
	
	if (currentRow >= 0) m_pTableWidget_PatientInformation->selectRow(currentRow);
}
