
#include "QPatientSummaryTab.h"

#include <QSqlQuery>
#include <QSqlError>

#include <Havana3/MainWindow.h>
#include <Havana3/Configuration.h>
#include <Havana3/HvnSqlDataBase.h>
#include <Havana3/QResultTab.h>
#include <Havana3/Dialog/AddPatientDlg.h>


QPatientSummaryTab::QPatientSummaryTab(QString patient_id, QWidget *parent) :
    QDialog(parent), m_patientId(patient_id), m_pEditPatientDlg(nullptr)
{
    // Set title
    setWindowTitle("Patient Summary");

	// Set main window objects
    m_pMainWnd = dynamic_cast<MainWindow*>(parent);
    m_pConfig = m_pMainWnd->m_pConfiguration;
    m_pHvnSqlDataBase = m_pMainWnd->m_pHvnSqlDataBase;

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
}

void QPatientSummaryTab::keyPressEvent(QKeyEvent *e)
{
	if (e->key() != Qt::Key_Escape)
		QDialog::keyPressEvent(e);
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

    m_pPushButton_EditPatient = new QPushButton(this);
    m_pPushButton_EditPatient->setText("  Edit Patient");
    m_pPushButton_EditPatient->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));
    m_pPushButton_EditPatient->setFixedSize(130, 25);

    createPatientSummaryTable();

    // Set layout: patient selection view
    QGridLayout *pGridLayout_PatientSummary = new QGridLayout;
    pGridLayout_PatientSummary->setSpacing(8);

    pGridLayout_PatientSummary->addWidget(m_pLabel_PatientSummary, 0, 0);
    pGridLayout_PatientSummary->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 1);
    pGridLayout_PatientSummary->addWidget(m_pPushButton_NewRecord, 0, 2, 1, 2);

    pGridLayout_PatientSummary->addWidget(m_pLabel_PatientInformation, 1, 0);
    pGridLayout_PatientSummary->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 0, 1, 3);
    pGridLayout_PatientSummary->addWidget(m_pPushButton_EditPatient, 1, 3);

    pGridLayout_PatientSummary->addWidget(m_pTableWidget_RecordInformation, 2, 0, 1, 4);

    m_pGroupBox_PatientSummary->setLayout(pGridLayout_PatientSummary);


    // Connect signal and slot
    connect(m_pPushButton_EditPatient, SIGNAL(clicked(bool)), this, SLOT(editPatient()));
    connect(m_pPushButton_NewRecord, SIGNAL(clicked(bool)), this, SLOT(newRecord()));
}

void QPatientSummaryTab::createPatientSummaryTable()
{
    m_pTableWidget_RecordInformation = new QTableWidget(this);
    m_pTableWidget_RecordInformation->setRowCount(0);
    m_pTableWidget_RecordInformation->setColumnCount(8);

    QStringList colLabels;
    colLabels << "Title" << "Preview" << "Datetime Created" << "Vessel" << "Procedure" << "Comments" << "Review" << "Delete";
    m_pTableWidget_RecordInformation->setHorizontalHeaderLabels(colLabels);

    // Insert data in the cells
    loadRecordDatabase();

    // Cell items properties
    m_pTableWidget_RecordInformation->setAlternatingRowColors(true);
    m_pTableWidget_RecordInformation->setSelectionMode(QAbstractItemView::SingleSelection);  // Focus 잃으면 Deselect 되도록 하기
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
	connect(this, SIGNAL(requestDelete(QString)), this, SLOT(deleteRecordData(QString)));
    connect(m_pTableWidget_RecordInformation, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(modifyRecordComment(int, int)));
//    connect(m_pTableWidget_RecordInformation, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(finishedModifyRecordComment(QTableWidgetItem*)));
}


void QPatientSummaryTab::newRecord()
{
    emit requestNewRecord(m_patientId);
}

void QPatientSummaryTab::editPatient()
{
    if (m_pEditPatientDlg == nullptr)
    {
        m_pEditPatientDlg = new AddPatientDlg(this, m_patientId);
        connect(m_pEditPatientDlg, SIGNAL(finished(int)), this, SLOT(deleteEditPatientDlg()));
        m_pEditPatientDlg->setModal(true);
        m_pEditPatientDlg->exec();
    }
}

void QPatientSummaryTab::deleteEditPatientDlg()
{
    m_pEditPatientDlg = nullptr;
}


void QPatientSummaryTab::modifyRecordComment(int row, int col)
{
	// Double Click하여 Comment 수정할 수 있도록 하기..

    //if (col == 5)
    //{
    //    m_pTableWidget_RecordInformation->setEditTriggers(QAbstractItemView::AllEditTriggers);

    //    auto item = m_pTableWidget_RecordInformation->item(row, col);
    //    item->setFlags(item->flags() | Qt::ItemIsEditable);

    //    if (item->text() == "Double click to add...")
    //    {
    //        item->setText("");
    //        item->setTextColor(QColor(255, 255, 255));

    //        QFont font;
    //        font.setItalic(false);
    //        item->setFont(font);
    //    }
    //}
}

void QPatientSummaryTab::finishedModifyRecordComment(QTableWidgetItem* item)
{
    qDebug() << "here";
//    m_pTableWidget_RecordInformation->set
//    m_pTableWidget_RecordInformation->setEditTriggers(QAbstractItemView::NoEditTriggers);

//    item->setFlags(item->flags() & ~Qt::ItemIsEditable);

//    if (item->text() == "")
//    {
//        item->setText("Double click to add...");
//        item->setTextColor(QColor(128, 128, 128));

//        QFont font;
//        font.setItalic(true);
//        item->setFont(font);
//    }
}

void QPatientSummaryTab::deleteRecordData(QString record_id)
{
	QMessageBox MsgBox(QMessageBox::Question, "Confirm delete", "Do you want to remove the recorded data?");
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

	QString command = QString("SELECT * FROM records WHERE id=%1").arg(record_id);

	int current_row = -1;
	m_pHvnSqlDataBase->queryDatabase(command, [&](QSqlQuery& _sqlQuery) {

		while (_sqlQuery.next())
		{
			QString datetime = _sqlQuery.value(3).toString();

			for (int i = 0; i < m_pTableWidget_RecordInformation->rowCount(); i++)
			{
				if (m_pTableWidget_RecordInformation->item(i, 2)->text() == datetime)
				{
					current_row = i;
					break;
				}
			}
		}
	});

	m_pHvnSqlDataBase->queryDatabase(QString("DELETE FROM records WHERE id='%1'").arg(record_id));
	m_pTableWidget_RecordInformation->removeRow(current_row);
}


void QPatientSummaryTab::loadPatientInformation()
{
    QString command = QString("SELECT * FROM patients WHERE patient_id=%1").arg(m_patientId);

    m_pHvnSqlDataBase->queryDatabase(command, [&](QSqlQuery& _sqlQuery) {
        while (_sqlQuery.next())
        {
			QString patient_name = _sqlQuery.value(1).toString() + ", " + _sqlQuery.value(0).toString();
            m_pLabel_PatientInformation->setText(QString("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<b>%1</b>"
                                                         "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<b>ID:</b> %2"
                                                         "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<b>DOB:</b> %3"
                                                         "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<b>Gender:</b> %4").
                                                         arg(patient_name).
                                                         arg(_sqlQuery.value(3).toString()).
                                                         arg(_sqlQuery.value(4).toString()).
                                                         arg(m_pHvnSqlDataBase->getGender(_sqlQuery.value(5).toInt())));			
			QString title = QString("Patient Summary: %1").arg(patient_name);
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
        }
    });
}

void QPatientSummaryTab::loadRecordDatabase()
{
    m_pTableWidget_RecordInformation->setRowCount(0);
    m_pTableWidget_RecordInformation->setSortingEnabled(false);

    QString command = QString("SELECT * FROM records WHERE patient_id=%1").arg(m_patientId);

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

            QWidget *pWidget_Review = new QWidget;
            QHBoxLayout *pHBoxLayout_Review = new QHBoxLayout;
            pHBoxLayout_Review->setSpacing(0);
            pHBoxLayout_Review->setAlignment(Qt::AlignCenter);
            pHBoxLayout_Review->addWidget(pPushButton_Review);
            pWidget_Review->setLayout(pHBoxLayout_Review);

            QPushButton *pPushButton_Delete = new QPushButton;
            pPushButton_Delete->setFixedSize(40, 30);
            pPushButton_Delete->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));

            QWidget *pWidget_Delete = new QWidget;
            QHBoxLayout *pHBoxLayout_Delete = new QHBoxLayout;
            pHBoxLayout_Delete->setSpacing(0);
            pHBoxLayout_Delete->setAlignment(Qt::AlignCenter);
            pHBoxLayout_Delete->addWidget(pPushButton_Delete);
            pWidget_Delete->setLayout(pHBoxLayout_Delete);

            QByteArray previewByteArray = _sqlQuery.value(4).toByteArray();
            QPixmap previewImage = QPixmap();
            previewImage.loadFromData(previewByteArray);

            pTitleItem->setText(_sqlQuery.value(7).toString()); pTitleItem->setTextAlignment(Qt::AlignCenter);
            pPreviewItem->setData(Qt::DecorationRole, previewImage.scaled(144, 144));
            pDateTimeItem->setText(_sqlQuery.value(3).toString()); pDateTimeItem->setTextAlignment(Qt::AlignCenter);
            pVesselItem->setText(m_pHvnSqlDataBase->getVessel(_sqlQuery.value(12).toInt())); pVesselItem->setTextAlignment(Qt::AlignCenter);
            pProcedureItem->setText(m_pHvnSqlDataBase->getProcedure(_sqlQuery.value(11).toInt())); pProcedureItem->setTextAlignment(Qt::AlignCenter);
            if (!(_sqlQuery.value(8).toString() == ""))
                pCommentItem->setText(_sqlQuery.value(8).toString());
            else
            {
                pCommentItem->setText("Double click to add...");
                pCommentItem->setTextColor(QColor(128, 128, 128));

                QFont font;
                font.setItalic(true);
                pCommentItem->setFont(font);
            }

            QString record_id = _sqlQuery.value(0).toString();
            connect(pPushButton_Review, &QPushButton::clicked, [&, record_id]() { emit requestReview(record_id); });
			connect(pPushButton_Delete, &QPushButton::clicked, [&, record_id]() { emit requestDelete(record_id); });

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
    });

    m_pTableWidget_RecordInformation->setSortingEnabled(true);
}
