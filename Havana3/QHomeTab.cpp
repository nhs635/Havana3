
#include "QHomeTab.h"

#include <Havana3/MainWindow.h>
#include <Havana3/Configuration.h>
#include <Havana3/HvnSqlDataBase.h>

#include <iostream>
#include <thread>
#include <chrono>


QHomeTab::QHomeTab(QWidget *parent) :
    QDialog(parent)
{
    // Set title
    setWindowTitle("Home");

	// Set main window objects
    m_pMainWnd = dynamic_cast<MainWindow*>(parent);
    m_pConfig = m_pMainWnd->m_pConfiguration;
	m_pHvnSqlDataBase = m_pMainWnd->m_pHvnSqlDataBase;

    // Create widgets for sign-in view
    createSignInViewWidgets();

    // Set window layout
    QHBoxLayout* pHBoxLayout = new QHBoxLayout;
    pHBoxLayout->setSpacing(2);

    pHBoxLayout->addWidget(m_pGroupBox_SignIn);

    this->setLayout(pHBoxLayout);
}

QHomeTab::~QHomeTab()
{
}

void QHomeTab::keyPressEvent(QKeyEvent *e)
{
	if (e->key() != Qt::Key_Escape)
		QDialog::keyPressEvent(e);
    if (e->key() == Qt::Key_Enter)
        signIn();
}


void QHomeTab::createSignInViewWidgets()
{
    // Create widgets for sign-in view
    m_pGroupBox_SignIn = new QGroupBox(this);
    m_pGroupBox_SignIn->setFixedSize(330, 220);
    m_pGroupBox_SignIn->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
    m_pGroupBox_SignIn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    m_pLabel_Title = new QLabel(this);
    m_pLabel_Title->setText("Clinical OCT-FLIm");
    m_pLabel_Title->setAlignment(Qt::AlignCenter);
    m_pLabel_Title->setStyleSheet("QLabel{font-size:25pt; font-weight:bold}");

    m_pLabel_Version = new QLabel(this);
    m_pLabel_Version->setText(QString("Havana3 ver%2").arg(VERSION));
    m_pLabel_Version->setAlignment(Qt::AlignCenter);
    m_pLabel_Version->setStyleSheet("QLabel{font-size:12pt;}");

    m_pLabel_UserName = new QLabel(this);
    m_pLabel_UserName->setText("Username");
    m_pLabel_UserName->setStyleSheet("QLabel{font-weight:bold}");

    m_pLineEdit_UserName = new QLineEdit(this);
    m_pLineEdit_UserName->setText("admin");
    m_pLineEdit_UserName->setFixedWidth(300);

    m_pLabel_Password = new QLabel(this);
    m_pLabel_Password->setText("Password");
    m_pLabel_Password->setStyleSheet("QLabel{font-weight:bold}");

    m_pLineEdit_Password = new QLineEdit(this);
    m_pLineEdit_Password->setText("bop4328");
    m_pLineEdit_Password->setEchoMode(QLineEdit::Password);
    m_pLineEdit_Password->setFixedWidth(300);

    m_pPushButton_SignIn = new QPushButton(this);
    m_pPushButton_SignIn->setText("SIGN IN");
    m_pPushButton_SignIn->setFixedSize(300, 25);


    // Set layout: sign-in view
    QVBoxLayout *pVBoxLayout_Home = new QVBoxLayout;
    pVBoxLayout_Home->setAlignment(Qt::AlignCenter);
    pVBoxLayout_Home->setSpacing(4);

    pVBoxLayout_Home->addWidget(m_pLabel_Title);
    pVBoxLayout_Home->addWidget(m_pLabel_Version);
    pVBoxLayout_Home->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
    pVBoxLayout_Home->addWidget(m_pLabel_UserName);
    pVBoxLayout_Home->addWidget(m_pLineEdit_UserName);
    pVBoxLayout_Home->addWidget(m_pLabel_Password);
    pVBoxLayout_Home->addWidget(m_pLineEdit_Password);
    pVBoxLayout_Home->addWidget(m_pPushButton_SignIn);

    m_pGroupBox_SignIn->setLayout(pVBoxLayout_Home);


    // Connect signal and slot
    connect(m_pPushButton_SignIn, SIGNAL(clicked(bool)), this, SLOT(signIn()));
}


void QHomeTab::signIn()
{
    static int n_trial = 0;
    static bool too_many_attempts = false;

	QString typed_user = m_pLineEdit_UserName->text();
    QString typed_pswd = m_pLineEdit_Password->text();
    if (!too_many_attempts && (n_trial++ < 5))
    {
        if (m_pHvnSqlDataBase->openDatabase(typed_user, typed_pswd))
        {
            n_trial = 0;
            m_pGroupBox_SignIn->setVisible(false);        
			m_pConfig->writeToLog(QString("Successfully signed in. (username: %1)").arg(typed_user));
			
            emit signedIn();
        }
        else
        {
			m_pConfig->writeToLog("Permission error: Authentication failed. Please try again.");
            QMessageBox MsgBox(QMessageBox::Critical, "Permission error", "Authentication failed. Please try again.");
            MsgBox.exec();
        }
    }
    else
    {
		m_pConfig->writeToLog("Permission error: Too many wrong login attempts. Please try again after a few miniutes.");
        QMessageBox MsgBox(QMessageBox::Critical, "Permission error", "Too many wrong login attempts. Please try again after a few miniutes.");
        MsgBox.exec();

        if (!too_many_attempts)
        {
            std::thread login_lock([&](){
                too_many_attempts = true;
                std::this_thread::sleep_for(std::chrono::minutes(3));
                too_many_attempts = false;
                n_trial = 0;
            });
            login_lock.detach();
        }
    }
}
