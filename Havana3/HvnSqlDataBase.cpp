
#include "HvnSqlDataBase.h"

#include <Havana3/MainWindow.h>

#include <QSqlQuery>
#include <QSqlError>


HvnSqlDataBase::HvnSqlDataBase(QObject *parent) :
    QObject(parent)
{
    // Set configuration objects
    m_pConfig = dynamic_cast<MainWindow*>(parent)->m_pConfiguration;
}

HvnSqlDataBase::~HvnSqlDataBase()
{
}


void HvnSqlDataBase::openDatabase()
{    
    m_sqlDataBase = QSqlDatabase::addDatabase("QSQLITE", "DBConnection");
    m_sqlDataBase.setDatabaseName(m_pConfig->dbPath + "\\db.sqlite");
    if (!m_sqlDataBase.open())
    {
        QMessageBox MsgBox(QMessageBox::Critical, "Database error", "Failed to open: " + m_sqlDataBase.lastError().text());
        MsgBox.exec();
    }
}

void HvnSqlDataBase::closeDatabase()
{
    if (m_sqlDataBase.isOpen())
    {
        m_sqlDataBase.close();
        m_sqlDataBase = QSqlDatabase();
        m_sqlDataBase.removeDatabase("DBConnection");
    }
}

void HvnSqlDataBase::initializeDatabase()
{
    QFile sql_file("initialize_query.sql");
    if (sql_file.open(QIODevice::ReadOnly))
    {
        QString query_str(sql_file.readAll());
        QStringList query_str_list = query_str.split(';', QString::SkipEmptyParts);

        foreach (const QString& command, query_str_list)
            if (command.trimmed() != "")
                queryDatabase(command);
        sql_file.close();
    }
    else
    {
        return; // error
    }
}

bool HvnSqlDataBase::queryDatabase(const QString &command, std::function<void(QSqlQuery &)> const &DidQuery, bool db_opened)
{
    if (!db_opened) openDatabase();
    {
        QSqlQuery sqlQuery(m_sqlDataBase);
        if (!sqlQuery.exec(command))
        {
            QMessageBox MsgBox(QMessageBox::Critical, "Database error", "Failed to query: " + sqlQuery.lastError().text());
            MsgBox.exec();
            closeDatabase();
            return false;
        }

        // Do next thing
        DidQuery(sqlQuery);
    }
    if (!db_opened) closeDatabase();

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
