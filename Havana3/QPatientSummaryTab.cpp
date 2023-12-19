
#include "QPatientSummaryTab.h"

#include <QFileInfo>
#include <QSqlQuery>
#include <QSqlError>

#include <Havana3/MainWindow.h>
#include <Havana3/Configuration.h>
#include <Havana3/HvnSqlDataBase.h>
#include <Havana3/QStreamTab.h>
#include <Havana3/QResultTab.h>
#include <Havana3/Dialog/AddPatientDlg.h>
#include <Havana3/Dialog/SettingDlg.h>

#include <DeviceControl/DeviceControl.h>
#include <DeviceControl/FaulhaberMotor/PullbackMotor.h>

#include <iostream>
#include <thread>


QPatientSummaryTab::QPatientSummaryTab(QString patient_id, QWidget *parent) :
    QDialog(parent), m_pDeviceControl(nullptr), m_pEditPatientDlg(nullptr), m_pSettingDlg(nullptr)
{
    // Set title
    setWindowTitle("Patient Summary");

	// Set main window objects
    m_pMainWnd = dynamic_cast<MainWindow*>(parent);
    m_pConfig = m_pMainWnd->m_pConfiguration;
    m_pHvnSqlDataBase = m_pMainWnd->m_pHvnSqlDataBase;
	
	m_patientInfo.patientId = patient_id;

    // Create widgets for patient summary view
    createPatientSummaryViewWidgets();

    // Set window layout
    QHBoxLayout* pHBoxLayout = new QHBoxLayout;
    pHBoxLayout->setSpacing(2);

    pHBoxLayout->addWidget(m_pGroupBox_PatientSummary);

    this->setLayout(pHBoxLayout);
}

QPatientSummaryTab::~QPatientSummaryTab()
{
	m_pTableWidget_RecordInformation->clear();
	if (m_pDeviceControl)
	{
		delete m_pDeviceControl;
		m_pDeviceControl = nullptr;
	}
}

void QPatientSummaryTab::keyPressEvent(QKeyEvent *e)
{
	if (e->key() != Qt::Key_Escape)
		QDialog::keyPressEvent(e);
	else
	{
		m_pTableWidget_RecordInformation->clearSelection();
		m_pTableWidget_RecordInformation->clearFocus();
	}
}


void QPatientSummaryTab::createPatientSummaryViewWidgets()
{
    // Create widgets for patient summary view
    m_pGroupBox_PatientSummary = new QGroupBox(this);
    m_pGroupBox_PatientSummary->setFixedSize(1200, 850);
    m_pGroupBox_PatientSummary->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
    m_pGroupBox_PatientSummary->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_pLabel_PatientSummary = new QLabel(this);
    m_pLabel_PatientSummary->setText(" Patient Summary");
    m_pLabel_PatientSummary->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    m_pLabel_PatientSummary->setStyleSheet("QLabel{font-size:20pt; font-weight:bold}");

    m_pLabel_PatientInformation = new QLabel(this);
    loadPatientInformation();
    m_pLabel_PatientInformation->setStyleSheet("QLabel{font-size:12pt}");
	
	m_pPushButton_NewRecord = new QPushButton(this);
	m_pPushButton_NewRecord->setText("  New Record");
	m_pPushButton_NewRecord->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
	m_pPushButton_NewRecord->setStyleSheet("QPushButton{font-size:12pt; font-weight:bold}");
	m_pPushButton_NewRecord->setFixedSize(180, 35);

	m_pToggleButton_CatheterConnect = new QPushButton(this);
	m_pToggleButton_CatheterConnect->setText("  Catheter Connect");
	m_pToggleButton_CatheterConnect->setCheckable(true);
	m_pToggleButton_CatheterConnect->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
	m_pToggleButton_CatheterConnect->setFixedSize(150, 25);

	m_pToggleButton_ShowHiddenData = new QPushButton(this);
	m_pToggleButton_ShowHiddenData->setText("  Show Hidden Data");
	m_pToggleButton_ShowHiddenData->setCheckable(true);
	m_pToggleButton_ShowHiddenData->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
	m_pToggleButton_ShowHiddenData->setFixedSize(150, 25);

	m_pPushButton_Import = new QPushButton(this);
	m_pPushButton_Import->setText("  Import");
	m_pPushButton_Import->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
	m_pPushButton_Import->setFixedSize(120, 25);

	m_pToggleButton_Export = new QPushButton(this);
	m_pToggleButton_Export->setCheckable(true);
	m_pToggleButton_Export->setText("  Export");
	m_pToggleButton_Export->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
	m_pToggleButton_Export->setFixedSize(120, 25);
	
    m_pPushButton_EditPatient = new QPushButton(this);
    m_pPushButton_EditPatient->setText("  Edit Patient");
    m_pPushButton_EditPatient->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));
    m_pPushButton_EditPatient->setFixedSize(120, 25);

	m_pPushButton_Setting = new QPushButton(this);
	m_pPushButton_Setting->setText("  Setting");
	m_pPushButton_Setting->setIcon(style()->standardIcon(QStyle::SP_FileDialogInfoView));
	m_pPushButton_Setting->setFixedSize(120, 25);

    createPatientSummaryTable();

    // Set layout: patient selection view
    QVBoxLayout *pVBoxLayout_PatientSummary = new QVBoxLayout;
	pVBoxLayout_PatientSummary->setSpacing(8);

	QHBoxLayout *pHBoxLayout_Title = new QHBoxLayout;
	pHBoxLayout_Title->setSpacing(8);
	pHBoxLayout_Title->addWidget(m_pLabel_PatientSummary);
	pHBoxLayout_Title->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout_Title->addWidget(m_pToggleButton_ShowHiddenData, 0, Qt::AlignBottom);
	pHBoxLayout_Title->addWidget(m_pToggleButton_CatheterConnect, 0, Qt::AlignBottom);
	pHBoxLayout_Title->addWidget(m_pPushButton_NewRecord);

	QHBoxLayout *pHBoxLayout_Info = new QHBoxLayout;
	pHBoxLayout_Info->setSpacing(8);
	pHBoxLayout_Info->addWidget(m_pLabel_PatientInformation);
	pHBoxLayout_Info->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout_Info->addWidget(m_pPushButton_Import);
	pHBoxLayout_Info->addWidget(m_pToggleButton_Export);	
	pHBoxLayout_Info->addWidget(m_pPushButton_EditPatient);
	pHBoxLayout_Info->addWidget(m_pPushButton_Setting);

	pVBoxLayout_PatientSummary->addItem(pHBoxLayout_Title);
	pVBoxLayout_PatientSummary->addItem(pHBoxLayout_Info);
	pVBoxLayout_PatientSummary->addWidget(m_pTableWidget_RecordInformation);

    m_pGroupBox_PatientSummary->setLayout(pVBoxLayout_PatientSummary);


    // Connect signal and slot
	connect(m_pPushButton_NewRecord, SIGNAL(clicked(bool)), this, SLOT(newRecord()));
	connect(m_pToggleButton_CatheterConnect, SIGNAL(toggled(bool)), this, SLOT(catheterConnect(bool)));
	connect(m_pToggleButton_ShowHiddenData, SIGNAL(toggled(bool)), this, SLOT(showHiddenData(bool)));
	connect(m_pPushButton_Import, SIGNAL(clicked(bool)), this, SLOT(import()));
	connect(m_pToggleButton_Export, SIGNAL(toggled(bool)), this, SLOT(exportRawData(bool)));	
    connect(m_pPushButton_EditPatient, SIGNAL(clicked(bool)), this, SLOT(editPatient()));
	connect(m_pPushButton_Setting, SIGNAL(clicked(bool)), this, SLOT(createSettingDlg()));    
}

void QPatientSummaryTab::createPatientSummaryTable()
{
    m_pTableWidget_RecordInformation = new QTableWidget(this);
	m_pTableWidget_RecordInformation->clearContents();
	m_pTableWidget_RecordInformation->setRowCount(0);
    m_pTableWidget_RecordInformation->setColumnCount(8);

    QStringList colLabels;
    colLabels << "Title" << "Preview" << "Datetime Created" << "Vessel" << "Procedure" << "Comments" << "Review" << "Delete";
    m_pTableWidget_RecordInformation->setHorizontalHeaderLabels(colLabels);

    // Insert data in the cells
    loadRecordDatabase();

    // Cell items properties
    m_pTableWidget_RecordInformation->setAlternatingRowColors(true);
    m_pTableWidget_RecordInformation->setSelectionMode(QAbstractItemView::SingleSelection);  /////////////////////// Focus ������ Deselect �ǵ��� �ϱ�
    m_pTableWidget_RecordInformation->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pTableWidget_RecordInformation->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pTableWidget_RecordInformation->setTextElideMode(Qt::ElideRight);

    // Table properties
    m_pTableWidget_RecordInformation->setShowGrid(true);
    m_pTableWidget_RecordInformation->setGridStyle(Qt::DotLine);
    m_pTableWidget_RecordInformation->setSortingEnabled(true);
    m_pTableWidget_RecordInformation->setCornerButtonEnabled(true);

    // Header properties
    m_pTableWidget_RecordInformation->verticalHeader()->setDefaultSectionSize(150);
	m_pTableWidget_RecordInformation->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);
    m_pTableWidget_RecordInformation->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_pTableWidget_RecordInformation->horizontalHeader()->setMinimumSectionSize(100);
    m_pTableWidget_RecordInformation->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    m_pTableWidget_RecordInformation->setColumnWidth(0, 100);
    m_pTableWidget_RecordInformation->setColumnWidth(1, 150);
    m_pTableWidget_RecordInformation->setColumnWidth(2, 150);
    m_pTableWidget_RecordInformation->setColumnWidth(3, 120);
    m_pTableWidget_RecordInformation->setColumnWidth(4, 120);
    m_pTableWidget_RecordInformation->setColumnWidth(6, 60);
    m_pTableWidget_RecordInformation->setColumnWidth(7, 60);
	
    // Connect signal and slot
	connect(m_pTableWidget_RecordInformation, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(editContents(int, int)));
	connect(this, SIGNAL(requestDelete(QString)), this, SLOT(deleteRecordData(QString)));
}


void QPatientSummaryTab::catheterConnect(bool toggled)
{
	if (toggled)
	{
		if (m_pMainWnd->getStreamTab())
			m_pDeviceControl = m_pMainWnd->getStreamTab()->getDeviceControl();
		else if (!m_pDeviceControl)
			m_pDeviceControl = new DeviceControl(m_pConfig);

		if (m_pDeviceControl->connectPullbackMotor(true))
		{
			m_pPushButton_NewRecord->setEnabled(false);
			m_pPushButton_Setting->setEnabled(false);
			m_pToggleButton_CatheterConnect->setStyleSheet("QPushButton { background-color:#ff0000; }");	

			m_pDeviceControl->getPullbackMotor()->DidRotateEnd.clear();
			if (!m_pConfig->pullbackFlag)
			{
				m_pConfig->pullbackFlag = true;
				m_pConfig->setConfigFile("Havana3.ini");
				m_pDeviceControl->moveAbsolute();
			}
		}
		else
		{
			m_pToggleButton_CatheterConnect->setChecked(false);
		}		
	}
	else
	{
		if (m_pDeviceControl->getPullbackMotor())
		{
			m_pDeviceControl->getPullbackMotor()->DidRotateEnd.clear();
			m_pDeviceControl->getPullbackMotor()->DidRotateEnd += [&](int timeout) {
				if (timeout)
				{
					m_pConfig->pullbackFlag = false;
					m_pConfig->setConfigFile("Havana3.ini");
				}
			};
			m_pDeviceControl->home();
			std::this_thread::sleep_for(std::chrono::milliseconds(6000));

			m_pDeviceControl->connectPullbackMotor(false);
		}

		m_pPushButton_NewRecord->setEnabled(true);
		m_pPushButton_Setting->setEnabled(true);
		m_pToggleButton_CatheterConnect->setStyleSheet("QPushButton { background-color:#353535; }");

		if (m_pMainWnd->getStreamTab())
			m_pDeviceControl = nullptr;
	}
}

void QPatientSummaryTab::newRecord()
{
    emit requestNewRecord(m_patientInfo.patientId);

	m_pConfig->writeToLog(QString("New record requested: %1 (ID: %2)").arg(m_patientInfo.patientName).arg(m_patientInfo.patientId));
}

void QPatientSummaryTab::showHiddenData(bool toggled)
{
	if (toggled)
	{
		m_pToggleButton_ShowHiddenData->setStyleSheet("QPushButton { background-color:#ff0000; }");
	}
	else
	{
		m_pToggleButton_ShowHiddenData->setStyleSheet("QPushButton { background-color:#353535; }");
	}

	loadRecordDatabase();
}

void QPatientSummaryTab::exportRawData(bool toggled)
{
	if (toggled)
	{
		// Set widgets 
		m_pToggleButton_ShowHiddenData->setChecked(true);
		m_pToggleButton_ShowHiddenData->setDisabled(true);
		m_pToggleButton_CatheterConnect->setDisabled(true);
		m_pPushButton_NewRecord->setDisabled(true);				
		m_pPushButton_Import->setDisabled(true);
		m_pPushButton_EditPatient->setDisabled(true);
		m_pPushButton_Setting->setDisabled(true);

		// Set table widgets
		int n_row = m_pTableWidget_RecordInformation->rowCount();

		for (int i = 0; i < n_row; i++)
		{
			// Create export checkbox
			QCheckBox *pCheckBox_Export = new QCheckBox;

			QWidget *pWidget_Export = new QWidget;
			QHBoxLayout *pHBoxLayout_Export = new QHBoxLayout;
			pHBoxLayout_Export->setSpacing(0);
			pHBoxLayout_Export->setAlignment(Qt::AlignCenter);
			pHBoxLayout_Export->addWidget(pCheckBox_Export);
			pWidget_Export->setLayout(pHBoxLayout_Export);

			// Substitute
			m_pTableWidget_RecordInformation->horizontalHeaderItem(0)->setText("Export");
			delete m_pTableWidget_RecordInformation->item(i, 0);
			m_pTableWidget_RecordInformation->setCellWidget(i, 0, pWidget_Export);
						
			// Default state
			if (m_pTableWidget_RecordInformation->cellWidget(i, 6)->isEnabled())				
				if (!m_pTableWidget_RecordInformation->item(i, 5)->text().contains("[HIDDEN]"))
					pCheckBox_Export->setChecked(true);
			else
				pCheckBox_Export->setDisabled(true);

			// Disable review button
			m_pTableWidget_RecordInformation->cellWidget(i, 6)->setDisabled(true);
			m_pTableWidget_RecordInformation->cellWidget(i, 7)->setDisabled(true);
		}
	}
	else
	{
		// Get check state
		int n_row = m_pTableWidget_RecordInformation->rowCount();

		QStringList export_date;
		for (int i = 0; i < n_row; i++)
		{
			QCheckBox *pCheckBox_Export = (QCheckBox *)m_pTableWidget_RecordInformation->cellWidget(i, 0)->layout()->itemAt(0)->widget();
			
			if (pCheckBox_Export->isChecked())
			{
				QString date = m_pTableWidget_RecordInformation->item(i, 2)->text();
				date = date.replace(' ', '_');
				date = date.replace(':', '-');
				export_date.append(date);
			}
		}

		if (!export_date.empty())
		{
			// Set directory
			QString export_path = QFileDialog::getExistingDirectory(nullptr, "Select Export Path...", QDir::homePath(),
				QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog);
			if (export_path != "")
			{
				// Make record path
				QDir rec_dir(export_path + "/record");
				if (!rec_dir.exists()) {
					rec_dir.mkdir(export_path + "/record");
				}

				// Copy database file
				QFile::copy(m_pConfig->dbPath + "/db.sqlite", export_path + "/db.sqlite");

				// Get export data
				QString command = QString("SELECT * FROM records WHERE patient_id=%1").arg(m_patientInfo.patientId);

				m_pHvnSqlDataBase->queryDatabase(command, [&](QSqlQuery& _sqlQuery) {

					QMessageBox msg_box(QMessageBox::NoIcon, "Export Raw Data...", "", QMessageBox::NoButton, this);
					msg_box.setStandardButtons(0);
					msg_box.setWindowModality(Qt::WindowModal);
					msg_box.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
					msg_box.move(QApplication::desktop()->screen()->rect().center() - msg_box.rect().center());
					msg_box.setFixedSize(msg_box.width(), msg_box.height());
					msg_box.show();

					while (_sqlQuery.next())
					{
						// Get relative paths
						QString filepath = _sqlQuery.value(9).toString();
						int idx = filepath.indexOf("record");

						QString rel_path = filepath.remove(0, idx - 1);
						QString fullpath = m_pConfig->dbPath + rel_path;

						QFileInfo check_file(fullpath);
						if ((check_file.exists() && check_file.isFile()))
						{
							// Get patient info & date info
							QStringList folders = rel_path.split('/');
							QString patient = folders.at(folders.size() - 3);
							QString date = folders.at(folders.size() - 2);
							int idx = export_date.indexOf(date);

							if (idx != -1)
							{
								// Make patient path
								QDir patient_dir(export_path + "/record/" + patient);
								if (!patient_dir.exists()) {
									patient_dir.mkdir(export_path + "/record/" + patient);
								}

								// Make date path
								QDir date_dir(export_path + "/record/" + patient + "/" + date);
								if (!date_dir.exists()) {
									date_dir.mkdir(export_path + "/record/" + patient + "/" + date);
								}

								// Source and destination
								QStringList src_folders = fullpath.split('/');
								src_folders.pop_back();
								QString src_path = src_folders.join('/');
								
								QStringList dst_folders = (export_path + rel_path).split('/');
								dst_folders.pop_back();
								QString dst_path = dst_folders.join('/');
								
								// Get entry list
								QDir dir(src_path);
								QStringList entrylist = dir.entryList();
								entrylist.pop_front();
								entrylist.pop_front();
								
								// Copy data
								foreach(const QString& entry, entrylist)
									QFile::copy(src_path + "/" + entry, dst_path + "/" + entry);
							}
						}
					}

					msg_box.close();
					QDesktopServices::openUrl(QUrl("file:///" + export_path));
				});
			}
		}

		// Load record database again
		m_pTableWidget_RecordInformation->horizontalHeaderItem(0)->setText("Title");
		loadRecordDatabase();

		// Set widgets 
		m_pToggleButton_ShowHiddenData->setChecked(false);
		m_pToggleButton_ShowHiddenData->setEnabled(true);
		m_pToggleButton_CatheterConnect->setEnabled(true);
		m_pPushButton_NewRecord->setEnabled(true);
		m_pPushButton_Import->setEnabled(true);
		m_pPushButton_EditPatient->setEnabled(true);
		m_pPushButton_Setting->setEnabled(true);
	}
}

void QPatientSummaryTab::import()
{
	QStringList fileNames = QFileDialog::getOpenFileNames(nullptr, "Import external OCT-FLIm data", "", "OCT-FLIm raw data (*.data *.xml)", nullptr, QFileDialog::DontUseNativeDialog);
	
	// Get path to read	
	if (fileNames.size() != 0)
	{
		foreach(QString fileName, fileNames)
		{
			// Set record information
			RecordInfo record_info;

			record_info.patientId = m_patientInfo.patientId;
			record_info.patientName = m_patientInfo.patientName;
			record_info.filename = fileName;
			record_info.date = QFileInfo(record_info.filename).lastModified().toString("yyyy-MM-dd hh:mm:ss");

			if (fileName.split(".")[1] == "xml")
			{
				QFile file(fileName);
				if (file.open(QFile::ReadOnly))
				{
					QString rpdname = record_info.filename;
					rpdname.replace(".xml", ".rpd");
					record_info.date = QFileInfo(rpdname).lastModified().toString("yyyy-MM-dd hh:mm:ss");

					QTextStream stream(&file);
					QStringList info = stream.readLine().split(" ");

					QString procedure = info.at(4).split("=").at(1); procedure.remove("\"");
					record_info.procedure = procedure.toInt();
					QString vessel = info.at(8).split("=").at(1); vessel.remove("\"");
					record_info.vessel = vessel.toInt();

					record_info.is_dotter = true;

					file.close();
				}
			}

			// Add to database
			QString command = QString("INSERT INTO records(patient_id, datetime_taken, title, filename, procedure_id, vessel_id) "
				"VALUES('%1', '%2', '%3', '%4', %5, %6)").arg(record_info.patientId).arg(record_info.date)
				.arg(record_info.title).arg(record_info.filename).arg(record_info.procedure).arg(record_info.vessel);
			m_pHvnSqlDataBase->queryDatabase(command);
			
			m_pConfig->writeToLog(QString("Exisiting record imported: %1 (ID: %2): %3")
				.arg(m_patientInfo.patientName).arg(m_patientInfo.patientId).arg(fileName));
		}

		loadRecordDatabase();

		QMessageBox MsgBox(QMessageBox::Information, "Import", "Successfully imported!");
		MsgBox.exec();
	}
}

void QPatientSummaryTab::editPatient()
{
    if (m_pEditPatientDlg == nullptr)
    {
        m_pEditPatientDlg = new AddPatientDlg(this, m_patientInfo.patientId);
        connect(m_pEditPatientDlg, SIGNAL(finished(int)), this, SLOT(deleteEditPatientDlg()));
		m_pEditPatientDlg->setAttribute(Qt::WA_DeleteOnClose, true);
        m_pEditPatientDlg->setModal(true);
        m_pEditPatientDlg->exec();
    }
}

void QPatientSummaryTab::deleteEditPatientDlg()
{
	m_pEditPatientDlg->deleteLater();
    m_pEditPatientDlg = nullptr;
}

void QPatientSummaryTab::createSettingDlg()
{
	if (m_pSettingDlg == nullptr)
	{
		m_pSettingDlg = new SettingDlg(this);
		connect(m_pSettingDlg, SIGNAL(finished(int)), this, SLOT(deleteSettingDlg()));
		m_pSettingDlg->setAttribute(Qt::WA_DeleteOnClose, true);
		m_pSettingDlg->setModal(true);
		m_pSettingDlg->exec();
	}
}

void QPatientSummaryTab::deleteSettingDlg()
{
	m_pSettingDlg->deleteLater();
	m_pSettingDlg = nullptr;
}

void QPatientSummaryTab::editContents(int row, int column)
{
	if (!m_pToggleButton_Export->isChecked())
	{
		if (column == 5)
		{
			QDialog *pDialog = new QDialog(this);
			{
				QTextEdit *pTextEdit_Comment = new QTextEdit(this);
				pTextEdit_Comment->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
				pTextEdit_Comment->setAlignment(Qt::AlignLeft | Qt::AlignTop);
				pTextEdit_Comment->setPlainText(m_pTableWidget_RecordInformation->item(row, column)->text());
				connect(pTextEdit_Comment, &QTextEdit::textChanged, [&, pTextEdit_Comment]() { m_pTableWidget_RecordInformation->item(row, column)->setText(pTextEdit_Comment->toPlainText()); });
				connect(pDialog, &QDialog::finished, [&, pTextEdit_Comment]() {
					QString comment = pTextEdit_Comment->toPlainText();
					QString command = QString("UPDATE records SET comment='%1' WHERE id=%2").arg(comment).arg(m_pTableWidget_RecordInformation->item(row, 0)->toolTip());
					m_pHvnSqlDataBase->queryDatabase(command);
					m_pConfig->writeToLog(QString("Record comment updated: %1 (ID: %2): %3 : record id: %4")
						.arg(m_patientInfo.patientName).arg(m_patientInfo.patientId).arg(m_pTableWidget_RecordInformation->item(row, 2)->text()).arg(m_pTableWidget_RecordInformation->item(row, 0)->toolTip()));
					if (command.contains("[HIDDEN]"))
						loadRecordDatabase();
				});

				QVBoxLayout *pVBoxLayout = new QVBoxLayout;
				pVBoxLayout->addWidget(pTextEdit_Comment);
				pDialog->setLayout(pVBoxLayout);
			}
			pDialog->setWindowTitle("Comments");
			pDialog->setFixedSize(350, 200);
			pDialog->setModal(true);
			pDialog->exec();
		}
		else if (column == 0)
		{
			QDialog *pDialog = new QDialog(this);
			{
				QTextEdit *pTextEdit_Title = new QTextEdit(this);
				pTextEdit_Title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
				pTextEdit_Title->setAlignment(Qt::AlignLeft | Qt::AlignTop);
				pTextEdit_Title->setPlainText(m_pTableWidget_RecordInformation->item(row, column)->text());
				connect(pTextEdit_Title, &QTextEdit::textChanged, [&, pTextEdit_Title]() { 					
					QString title = pTextEdit_Title->toPlainText();
					//if (title.contains(QString::fromLocal8Bit("")))  // ★
					//	m_pTableWidget_RecordInformation->item(row, column)->setTextColor(QColor(255, 0, 0));
					//else if (title.contains(QString::fromLocal8Bit("")))  // ☆
					//	m_pTableWidget_RecordInformation->item(row, column)->setTextColor(QColor(0, 255, 255));
					//else
						m_pTableWidget_RecordInformation->item(row, column)->setTextColor(QColor(255, 255, 255));
					m_pTableWidget_RecordInformation->item(row, column)->setText(pTextEdit_Title->toPlainText()); 
				});
				connect(pDialog, &QDialog::finished, [&, pTextEdit_Title]() {
					QString comment = pTextEdit_Title->toPlainText();
					QString command = QString("UPDATE records SET title='%1' WHERE id=%2").arg(comment).arg(m_pTableWidget_RecordInformation->item(row, 0)->toolTip());
					m_pHvnSqlDataBase->queryDatabase(command);
					m_pConfig->writeToLog(QString("Record title updated: %1 (ID: %2): %3 : record id: %4")
						.arg(m_patientInfo.patientName).arg(m_patientInfo.patientId).arg(m_pTableWidget_RecordInformation->item(row, 2)->text()).arg(m_pTableWidget_RecordInformation->item(row, 0)->toolTip()));
				});

				QVBoxLayout *pVBoxLayout = new QVBoxLayout;
				pVBoxLayout->addWidget(pTextEdit_Title);
				pDialog->setLayout(pVBoxLayout);
			}
			pDialog->setWindowTitle("Title");
			pDialog->setFixedSize(200, 100);
			pDialog->setModal(true);
			pDialog->exec();
		}
		else if (column == 3)
		{
			QDialog *pDialog = new QDialog(this);
			{
				QComboBox *pComboBox_Vessel = new QComboBox(this);
				pComboBox_Vessel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
				QString cur_vessel = m_pTableWidget_RecordInformation->item(row, column)->text();
				for (int i = 0; i <= 16; i++)
				{
					pComboBox_Vessel->insertItem(i, m_pHvnSqlDataBase->getVessel(i));
					if (m_pHvnSqlDataBase->getVessel(i) == cur_vessel)
						pComboBox_Vessel->setCurrentIndex(i);
				}
				connect(pDialog, &QDialog::finished, [&, pComboBox_Vessel]() {
					int vessel = pComboBox_Vessel->currentIndex();
					m_pTableWidget_RecordInformation->item(row, column)->setText(m_pHvnSqlDataBase->getVessel(vessel));
					QString command = QString("UPDATE records SET vessel_id=%1 WHERE id=%2").arg(vessel).arg(m_pTableWidget_RecordInformation->item(row, 0)->toolTip());
					m_pHvnSqlDataBase->queryDatabase(command);
				});

				QVBoxLayout *pVBoxLayout = new QVBoxLayout;
				pVBoxLayout->addWidget(pComboBox_Vessel);
				pDialog->setLayout(pVBoxLayout);
			}
			pDialog->setWindowTitle("Vessel");
			pDialog->setFixedSize(180, 80);
			pDialog->setModal(true);
			pDialog->exec();
		}
		else if (column == 4)
		{
			QDialog *pDialog = new QDialog(this);
			{
				QComboBox *pComboBox_Procedure = new QComboBox(this);
				pComboBox_Procedure->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
				QString cur_procedure = m_pTableWidget_RecordInformation->item(row, column)->text();
				for (int i = 0; i <= 4; i++)
				{
					pComboBox_Procedure->insertItem(i, m_pHvnSqlDataBase->getProcedure(i));
					if (m_pHvnSqlDataBase->getProcedure(i) == cur_procedure)
						pComboBox_Procedure->setCurrentIndex(i);
				}
				connect(pDialog, &QDialog::finished, [&, pComboBox_Procedure]() {
					int procedure = pComboBox_Procedure->currentIndex();
					m_pTableWidget_RecordInformation->item(row, column)->setText(m_pHvnSqlDataBase->getProcedure(procedure));
					QString command = QString("UPDATE records SET procedure_id=%1 WHERE id=%2").arg(procedure).arg(m_pTableWidget_RecordInformation->item(row, 0)->toolTip());
					m_pHvnSqlDataBase->queryDatabase(command);
				});

				QVBoxLayout *pVBoxLayout = new QVBoxLayout;
				pVBoxLayout->addWidget(pComboBox_Procedure);
				pDialog->setLayout(pVBoxLayout);
			}
			pDialog->setWindowTitle("Procedure");
			pDialog->setFixedSize(180, 80);
			pDialog->setModal(true);
			pDialog->exec();
		}
		else if (column == 2)
		{
			QString recordId = m_pTableWidget_RecordInformation->item(row, 0)->toolTip();
			QString command = QString("SELECT * FROM records WHERE id=%1").arg(recordId);
			m_pHvnSqlDataBase->queryDatabase(command, [&](QSqlQuery& _sqlQuery) {
				while (_sqlQuery.next())
				{
					QString filename0 = _sqlQuery.value(9).toString();
					int idx = filename0.indexOf("record");
					QString filename = m_pConfig->dbPath + filename0.remove(0, idx - 1);

					QClipboard *pClipBoard = QApplication::clipboard();
					for (int i = filename.size() - 1; i >= 0; i--)
					{
						int slash_pos = i;
						if (filename.at(i) == '/')
						{
							filename = filename.left(slash_pos);
							break;
						}
					}
					pClipBoard->setText(filename);

					QMessageBox MsgBox(QMessageBox::Information, "Path Copy", QString("Data path was copied to the clipboard: \n%1").arg(filename));
					MsgBox.exec();
				}
			});
		}
	}
}

void QPatientSummaryTab::deleteRecordData(const QString& record_id)
{
	QMessageBox MsgBox(QMessageBox::Question, "Confirm delete", "Choose Remove to remove this data from Havana3.\nChoose Delete to permanently delete this data.");
	MsgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
	MsgBox.setButtonText(QMessageBox::Save, "&Remove");
	MsgBox.setButtonText(QMessageBox::Discard, "&Delete");
	MsgBox.setDefaultButton(QMessageBox::Cancel);
	int ret = MsgBox.exec();

	switch (ret)
	{
	case QMessageBox::Save:
		break;
	case QMessageBox::Discard:
		break;
	case QMessageBox::Cancel:
		return;
	default:
		return;
	}

	QString command = QString("SELECT * FROM records WHERE id=%1").arg(record_id);

	int current_row = -1;
	m_pHvnSqlDataBase->queryDatabase(command, [&, ret](QSqlQuery& _sqlQuery) {

		while (_sqlQuery.next())
		{
			QString datetime = _sqlQuery.value(3).toString();
			QString filename = _sqlQuery.value(9).toString();

			if (ret == QMessageBox::Discard)
			{
				QString filename0 = filename.replace("/pullback.data", "");
				QDir dir(filename0);
				dir.removeRecursively();
			}

			for (int i = 0; i < m_pTableWidget_RecordInformation->rowCount(); i++)
			{
				if (m_pTableWidget_RecordInformation->item(i, 2)->text() == datetime)
				{
					foreach(QDialog* pTabView, m_pMainWnd->getVectorTabViews())
					{
						if (pTabView->windowTitle().contains(datetime))
							m_pMainWnd->removeTabView(pTabView);
					}

					current_row = i;
					break;
				}
			}

			m_pConfig->writeToLog(QString("Record deleted: %1 (ID: %2): %3 : record id: %4")
				.arg(m_patientInfo.patientName).arg(m_patientInfo.patientId).arg(datetime).arg(record_id));
		}
	});

	m_pHvnSqlDataBase->queryDatabase(QString("DELETE FROM records WHERE id='%1'").arg(record_id));
	m_pTableWidget_RecordInformation->removeRow(current_row);
}


void QPatientSummaryTab::loadPatientInformation()
{
    QString command = QString("SELECT * FROM patients WHERE patient_id=%1").arg(m_patientInfo.patientId);

    m_pHvnSqlDataBase->queryDatabase(command, [&](QSqlQuery& _sqlQuery) {
        while (_sqlQuery.next())
        {
			QDate date_of_birth = QDate::fromString(_sqlQuery.value(4).toString(), "yyyy-MM-dd");
			QDate date_today = QDate::currentDate();
			int age = (int)((double)date_of_birth.daysTo(date_today) / 365.25);
			
			m_patientInfo.patientName = _sqlQuery.value(1).toString() + ", " + _sqlQuery.value(0).toString();
            m_pLabel_PatientInformation->setText(QString("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<b>%1</b>"
                                                         "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<b>ID:</b> %2"
                                                         "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<b>DOB:</b> %3"
                                                         "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<b>Gender:</b> %4").
                                                         arg(m_patientInfo.patientName).
                                                         arg(QString("%1").arg(_sqlQuery.value(3).toString().toInt(), 8, 10, QChar('0'))).
                                                         arg(QString("%1 (%2)").arg(_sqlQuery.value(4).toString()).arg(age)).
                                                         arg(m_pHvnSqlDataBase->getGender(_sqlQuery.value(5).toInt())));			
			QString title = QString("Patient Summary: %1").arg(m_patientInfo.patientName);
            setWindowTitle(title);

			// Set current tab title
			if (m_pMainWnd->getTabWidget()->currentWidget() == this)
				m_pMainWnd->getTabWidget()->setTabText(m_pMainWnd->getTabWidget()->currentIndex(), title);

			// Set relevant review titles
			int index = 0;
			foreach(QDialog* pTabView, m_pMainWnd->getVectorTabViews())
			{
				if (pTabView->windowTitle().contains("Review"))
				{
					((QResultTab*)pTabView)->loadPatientInfo();
					m_pMainWnd->getTabWidget()->setTabText(index, ((QResultTab*)pTabView)->windowTitle());
				}

				index++;
			}

			m_pConfig->writeToLog(QString("Patient info loaded: %1 (ID: %2) [QPatientSummaryTab]").arg(m_patientInfo.patientName).arg(m_patientInfo.patientId));
        }
    });
}

void QPatientSummaryTab::loadRecordDatabase()
{
	int currentRow = m_pTableWidget_RecordInformation->currentRow();

    m_pTableWidget_RecordInformation->clearContents();
	m_pTableWidget_RecordInformation->setRowCount(0);
    m_pTableWidget_RecordInformation->setSortingEnabled(false);

    QString command = QString("SELECT * FROM records WHERE patient_id=%1").arg(m_patientInfo.patientId);

	m_pHvnSqlDataBase->queryDatabase(command, [&](QSqlQuery& _sqlQuery) {

		int rowCount = 0;
		while (_sqlQuery.next())
		{
			m_pTableWidget_RecordInformation->insertRow(rowCount);

			QTableWidgetItem *pTitleItem = new QTableWidgetItem;
			QTableWidgetItem *pPreviewItem = new QTableWidgetItem;
			QTableWidgetItem *pDateTimeItem = new QTableWidgetItem;
			QTableWidgetItem *pVesselItem = new QTableWidgetItem;
			QTableWidgetItem *pProcedureItem = new QTableWidgetItem;
			QTableWidgetItem *pCommentItem = new QTableWidgetItem;

			QPushButton *pPushButton_Review = new QPushButton;
			pPushButton_Review->setFixedSize(40, 30);
			pPushButton_Review->setIcon(style()->standardIcon(QStyle::SP_DialogYesButton));

			QWidget *pWidget_Review = new QWidget; // (m_pTableWidget_RecordInformation);
			QHBoxLayout *pHBoxLayout_Review = new QHBoxLayout; //(m_pTableWidget_RecordInformation); // leak
			pHBoxLayout_Review->setSpacing(0);
			pHBoxLayout_Review->setAlignment(Qt::AlignCenter);
			pHBoxLayout_Review->addWidget(pPushButton_Review);
			pWidget_Review->setLayout(pHBoxLayout_Review);

			QPushButton *pPushButton_Delete = new QPushButton; //(m_pTableWidget_RecordInformation); // leak
			pPushButton_Delete->setFixedSize(40, 30);
			pPushButton_Delete->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));

			QWidget *pWidget_Delete = new QWidget; //(m_pTableWidget_RecordInformation); // leak
			QHBoxLayout *pHBoxLayout_Delete = new QHBoxLayout; //(m_pTableWidget_RecordInformation); // leak
			pHBoxLayout_Delete->setSpacing(0);
			pHBoxLayout_Delete->setAlignment(Qt::AlignCenter);
			pHBoxLayout_Delete->addWidget(pPushButton_Delete);
			pWidget_Delete->setLayout(pHBoxLayout_Delete);

			QString title = _sqlQuery.value(7).toString();
			//if (title.contains(QString::fromLocal8Bit("")))  // ★
			//	pTitleItem->setTextColor(QColor(255, 0, 0));
			//else if (title.contains(QString::fromLocal8Bit("")))  // ☆
			//	pTitleItem->setTextColor(QColor(0, 255, 255));

			QString procedure = m_pHvnSqlDataBase->getProcedure(_sqlQuery.value(11).toInt());
			if (procedure.contains("Follow Up"))
			{
				//pVesselItem->setTextColor(QColor(235, 235, 174));
				pProcedureItem->setTextColor(QColor(235, 235, 174));
				//pCommentItem->setTextColor(QColor(235, 235, 174));
			}

			QString comment = _sqlQuery.value(8).toString();
			if (comment.contains("[HIDDEN]"))
			{
				if (!m_pToggleButton_ShowHiddenData->isChecked())
				{
					m_pTableWidget_RecordInformation->removeRow(rowCount);
					continue;
				}
				else
				{
					pTitleItem->setTextColor(QColor(128, 128, 128));
					pDateTimeItem->setTextColor(QColor(128, 128, 128));
					pVesselItem->setTextColor(QColor(128, 128, 128));
					pProcedureItem->setTextColor(QColor(128, 128, 128));
					pCommentItem->setTextColor(QColor(128, 128, 128));
				}
			}

			QByteArray previewByteArray = _sqlQuery.value(4).toByteArray();
			if (previewByteArray.size() > 0)
			{
				QPixmap previewImage = QPixmap();
				previewImage.loadFromData(previewByteArray, "bmp", Qt::ColorOnly);
				if (comment.contains("[HIDDEN]"))
				{
					QImage previewImage0 = previewImage.toImage();
					QImage previewImage1 = previewImage0.convertToFormat(QImage::Format_Grayscale8);
					previewImage = QPixmap::fromImage(previewImage1);
				}
				pPreviewItem->setData(Qt::DecorationRole, previewImage);
			}
			else
			{
				pPreviewItem->setText("No Preview Image");
				pPreviewItem->setTextAlignment(Qt::AlignCenter);
				if (comment.contains("[HIDDEN]"))
					pPreviewItem->setTextColor(QColor(128, 128, 128));
				else
					pPreviewItem->setTextColor(QColor(192, 192, 192));
			}

            pTitleItem->setText(title); pTitleItem->setTextAlignment(Qt::AlignCenter);            
            pDateTimeItem->setText(_sqlQuery.value(3).toString()); pDateTimeItem->setTextAlignment(Qt::AlignCenter);
            pVesselItem->setText(m_pHvnSqlDataBase->getVessel(_sqlQuery.value(12).toInt())); pVesselItem->setTextAlignment(Qt::AlignCenter);
            pProcedureItem->setText(procedure); pProcedureItem->setTextAlignment(Qt::AlignCenter);            
            pCommentItem->setText(comment); pCommentItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter); 
						            			
            QString record_id = _sqlQuery.value(0).toString();
            connect(pPushButton_Review, &QPushButton::clicked, [&, record_id]() 
			{ 
				emit requestReview(record_id); 
				for (int i = 0; i < m_pTableWidget_RecordInformation->rowCount(); i++)
				{
					if (m_pTableWidget_RecordInformation->item(i, 0)->toolTip() == record_id)
						m_pTableWidget_RecordInformation->selectRow(i);
				}
			
			});
			connect(pPushButton_Delete, &QPushButton::clicked, [&, record_id]() { emit requestDelete(record_id); });

			pTitleItem->setToolTip(record_id);

			QString filepath0 = _sqlQuery.value(9).toString();
			QString ext = filepath0.split(".").at(1);
			
			int idx = filepath0.indexOf("record");
			QString filepath = m_pConfig->dbPath + filepath0.remove(0, idx - 1);

			if (ext == "data")
			{
				QFileInfo check_file(filepath);
				if (!(check_file.exists() && check_file.isFile()))
					pWidget_Review->setDisabled(true);

				QFileInfo check_ini(filepath.replace("pullback.data", "roi.csv"));
				if (check_ini.exists())
					pDateTimeItem->setTextColor(QColor(244, 177, 131));
			}
			else if (ext == "xml")
			{
				{
					QFileInfo check_file(filepath.replace(".xml", ".flim"));
					if (!(check_file.exists() && check_file.isFile()))
						pWidget_Review->setDisabled(true);
				}
				{
					QFileInfo check_file(filepath.replace(".flim", ".rpd"));
					if (!(check_file.exists() && check_file.isFile()))
						pWidget_Review->setDisabled(true);
				}
			}

            m_pTableWidget_RecordInformation->setItem(rowCount, 0, pTitleItem);
            m_pTableWidget_RecordInformation->setItem(rowCount, 1, pPreviewItem);
            m_pTableWidget_RecordInformation->setItem(rowCount, 2, pDateTimeItem);
            m_pTableWidget_RecordInformation->setItem(rowCount, 3, pVesselItem);
            m_pTableWidget_RecordInformation->setItem(rowCount, 4, pProcedureItem);
            m_pTableWidget_RecordInformation->setItem(rowCount, 5, pCommentItem);
            m_pTableWidget_RecordInformation->setCellWidget(rowCount, 6, pWidget_Review);
            m_pTableWidget_RecordInformation->setCellWidget(rowCount, 7, pWidget_Delete);

            rowCount++;
        }

		m_pConfig->writeToLog(QString("Record database loaded: %1 (ID: %2)").arg(m_patientInfo.patientName).arg(m_patientInfo.patientId));
    });

    m_pTableWidget_RecordInformation->setSortingEnabled(true);
	m_pTableWidget_RecordInformation->sortItems(2, Qt::DescendingOrder);
	QStringList numbers;
	for (int i = m_pTableWidget_RecordInformation->rowCount(); i > 0; i--)
		numbers << QString::number(i);
	m_pTableWidget_RecordInformation->setVerticalHeaderLabels(numbers);

	if (currentRow >= 0) m_pTableWidget_RecordInformation->selectRow(currentRow);
}
