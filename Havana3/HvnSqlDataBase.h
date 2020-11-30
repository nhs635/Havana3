#ifndef HVNSQLDATABASE_H
#define HVNSQLDATABASE_H

#include <QSqlDatabase>

#include <Havana3/Configuration.h>


enum GENDER { MALE, FEMALE };
enum VESSEL { NOT_SELECTED_VESSEL = 0, RCA_PROX, RCA_MID, RCA_DISTAL, PDA, LEFT_MAIN, LAD_PROX, LAD_MID, LAD_DISTAL, DIAGONAL_1, DIAGONAL_2, LCX_PROX, LCX_OM1, LCX_MID, LCX_OM2, LCX_DISTAL, OTHER_VESSEL };
enum PROCEDURE { NOT_SELECTED_PROCEDURE = 0, PRE_PCI, POST_PCI, FOLLOW_UP, OTHER_PROCEDURE };

class HvnSqlDataBase : public QObject
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit HvnSqlDataBase(QObject *parent = nullptr);
    virtual ~HvnSqlDataBase();
	
// Methods //////////////////////////////////////////////
public:
    void openDatabase();
    void closeDatabase();
    void initializeDatabase();
    bool queryDatabase(const QString &, std::function<void(QSqlQuery &)> const &DidQuery = [](QSqlQuery &){}, bool db_opened = false);

public:
    QString getGender(int);
    QString getVessel(int);
    QString getProcedure(int);

// Variables ////////////////////////////////////////////
private:
    Configuration *m_pConfig;
    QSqlDatabase m_sqlDataBase;
};

#endif // HVNSQLDATABASE_H
