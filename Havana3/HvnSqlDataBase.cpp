
#include "HvnSqlDataBase.h"

#include <Havana3/MainWindow.h>

#include <QSqlQuery>
#include <QSqlError>


HvnSqlDataBase::HvnSqlDataBase(QObject *parent) :
    QObject(parent)
{
    // Set configuration objects
	m_pMainWnd = dynamic_cast<MainWindow*>(parent);
    m_pConfig = m_pMainWnd->m_pConfiguration;

	// Set default database path & check database location is valid
	if (m_pConfig->dbPath == "")
	{
		m_pConfig->dbPath = QDir().homePath() + "/Documents/Havana3";
		QDir().mkdir(m_pConfig->dbPath);
	}
}

HvnSqlDataBase::~HvnSqlDataBase()
{
}


bool HvnSqlDataBase::openDatabase(const QString& username, const QString& password)
{    
	m_pMainWnd->setCursor(Qt::WaitCursor);
	{
		if (!m_sqlDataBase.isValid())
		{
			// Initialize sql database
			bool init_req = !QDir().exists(m_pConfig->dbPath + "/db.sqlite");

#ifndef ENABLE_DATABASE_ENCRYPTION
			m_sqlDataBase = QSqlDatabase::addDatabase("SQLITECIPHER", "DBConnection");
			m_sqlDataBase.setDatabaseName(m_pConfig->dbPath + "/db.sqlite");
			m_sqlDataBase.setUserName(username);
			m_sqlDataBase.setPassword(password);
			m_sqlDataBase.setConnectOptions("QSQLITE_USE_CIPHER=sqlcipher");
#else
			m_sqlDataBase = QSqlDatabase::addDatabase("QSQLITE", "DBConnection");
			m_sqlDataBase.setDatabaseName(m_pConfig->dbPath + "/db.sqlite");
#endif
			if (!m_sqlDataBase.open())
			{
				QMessageBox MsgBox(QMessageBox::Critical, "Database error", "Failed to open: " + m_sqlDataBase.lastError().text());
				MsgBox.exec();

				closeDatabase();
				return false;
			}
			else
			{
				if (init_req) initializeDatabase();

				m_username = username;
				m_password = password;
			}
		}
	}
	m_pMainWnd->setCursor(Qt::ArrowCursor);

	return true;
}

void HvnSqlDataBase::closeDatabase()
{
    if (m_sqlDataBase.isOpen())
        m_sqlDataBase.close();
    m_sqlDataBase = QSqlDatabase();
    m_sqlDataBase.removeDatabase("DBConnection");
}

void HvnSqlDataBase::initializeDatabase()
{
	QMessageBox MsgBox(QMessageBox::Information, "Database initialization", "There is no db.sqlite file in the specified path.\nDB initialization begins...");
	MsgBox.exec();

	MsgBox.setCursor(Qt::WaitCursor);
	{
		QFile sql_file("initialize_query.sql");
		if (sql_file.open(QIODevice::ReadOnly))
		{
			QString query_str(sql_file.readAll());
			QStringList query_str_list = query_str.split(';', QString::SkipEmptyParts);

			foreach(const QString& command, query_str_list)
				if (command.trimmed() != "")
					queryDatabase(command);
			sql_file.close();

			// Make directory for recorded data
			if (!QDir().exists(m_pConfig->dbPath + "/record"))
				QDir().mkdir(m_pConfig->dbPath + "/record");
		}
		else
		{
			/* 이니셜라이제이션 파일이 없다고 에러 나와야 함. */
			//return; // error
		}
	}
	m_pMainWnd->setCursor(Qt::ArrowCursor);
}

bool HvnSqlDataBase::queryDatabase(const QString &command, std::function<void(QSqlQuery &)> const &DidQuery, bool db_opened, const QByteArray & preview)
{
	if (!db_opened) m_sqlDataBase.open();
    {
		QSqlQuery sqlQuery(m_sqlDataBase);

		sqlQuery.prepare(command);
		if (preview.size() > 0)
			sqlQuery.bindValue(":preview", preview);

        if (!sqlQuery.exec())
        {
            QMessageBox MsgBox(QMessageBox::Critical, "Database error", "Failed to query: " + sqlQuery.lastError().text());
            MsgBox.exec();
			m_sqlDataBase.close();
            return false;
        }

        // Do next thing
        DidQuery(sqlQuery);
    }
	if (!db_opened) m_sqlDataBase.close(); 

    return true;
}


QString HvnSqlDataBase::getGender(int id)
{
    switch (id)
    {
    case MALE:
        return "M";
    case FEMALE:
        return "F";
    default:
        return "M";
    }
}

QString HvnSqlDataBase::getVessel(int id)
{
    switch (id)
    {
    case NOT_SELECTED_VESSEL:
        return "Not Selected";
    case RCA_PROX:
        return "RCA Prox";
    case RCA_MID:
        return "RCA Mid";
    case RCA_DISTAL:
        return "RCA Distal";
    case PDA:
        return "PDA";
    case LEFT_MAIN:
        return "Left Main";
    case LAD_PROX:
        return "LAD Prox";
    case LAD_MID:
        return "LAD Mid";
    case LAD_DISTAL:
        return "LAD Distal";
    case DIAGONAL_1:
        return "Diagonal 1";
    case DIAGONAL_2:
        return "Diagonal 2";
    case LCX_PROX:
        return "LCX Prox";
    case LCX_OM1:
        return "LCX OM1";
    case LCX_MID:
        return "LCX Mid";
    case LCX_OM2:
        return "LCX OM2";
    case LCX_DISTAL:
        return "LCX Distal";
    case OTHER_VESSEL:
        return "Other";
    default:
        return "Not Selected";
    }
}

QString HvnSqlDataBase::getProcedure(int id)
{
    switch (id)
    {
    case NOT_SELECTED_PROCEDURE:
        return "Not Selected";
    case PRE_PCI:
        return "Pre PCI";
    case POST_PCI:
        return "Post PCI";
    case FOLLOW_UP:
        return "Follow Up";
    case OTHER_PROCEDURE:
        return "Other";
    default:
        return "Not Selected";
    }
}
