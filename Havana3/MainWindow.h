#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets>
#include <QtCore>

class Configuration;

class QStreamTab;
class QResultTab;

namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

// Methods //////////////////////////////////////////////
protected:
    void closeEvent(QCloseEvent *e);

private slots:
    void onTimer();
	void onTimerSync();
    void changedTab(int);

// Variables ////////////////////////////////////////////
public:
    Configuration* m_pConfiguration;

private:
    QTimer *m_pTimer, *m_pTimerSync;

private:
    Ui::MainWindow *ui;

    // Layout
    QGridLayout *m_pGridLayout;

public:
    // Tab objects
    QTabWidget *m_pTabWidget;
    QStreamTab *m_pStreamTab;
    QResultTab *m_pResultTab;

    // Status bar
    QLabel *m_pStatusLabel_ImagePos;
	QLabel *m_pStatusLabel_SyncStatus;
};

#endif // MAINWINDOW_H
