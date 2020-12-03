#ifndef QHOMETAB_H
#define QHOMETAB_H

#include <QtWidgets>
#include <QtCore>


class MainWindow;
class Configuration;
class HvnSqlDataBase;

class QHomeTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit QHomeTab(QWidget *parent = nullptr);
	virtual ~QHomeTab();
	
// Methods //////////////////////////////////////////////
protected:
	void keyPressEvent(QKeyEvent *);
	    
public:
    inline MainWindow* getMainWnd() const { return m_pMainWnd; }

private:
    void createSignInViewWidgets();

private slots:
    void signIn();

signals:
    void signedIn();

// Variables ////////////////////////////////////////////
private:
    MainWindow* m_pMainWnd;
    Configuration* m_pConfig;
	HvnSqlDataBase* m_pHvnSqlDataBase;

private:
	// Widgets for sign-in view
	QGroupBox *m_pGroupBox_SignIn;

	QLabel *m_pLabel_Title;
	QLabel *m_pLabel_Version;

	QLabel *m_pLabel_UserName;
	QLineEdit *m_pLineEdit_UserName;

	QLabel *m_pLabel_Password;
	QLineEdit *m_pLineEdit_Password;

    QPushButton *m_pPushButton_SignIn;
};

#endif // QHOMETAB_H
