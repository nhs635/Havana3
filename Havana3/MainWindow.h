#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets>
#include <QtCore>

class Configuration;
class HvnSqlDataBase;

class QHomeTab;
class QPatientSelectionTab;
class QPatientSummaryTab;

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
    virtual ~MainWindow();

// Methods //////////////////////////////////////////////
protected:
    void closeEvent(QCloseEvent *e);

public:
	inline QTabWidget* getTabWidget() const { return m_pTabWidget; }
    inline QHomeTab* getHomeTab() const { return m_pHomeTab; }
    inline QPatientSelectionTab* getPatientSelectionTab() const { return m_pPatientSelectionTab; }
    inline QStreamTab* getStreamTab() const { return m_pStreamTab; }
    inline auto getVectorTabViews() { return m_vectorTabViews; }
	inline int getCurrentTabIndex() { return m_nCurTabIndex; }

public:
    void addTabView(QDialog *);
    void removeTabView(QDialog *);

private slots:
    void tabCurrentChanged(int);
    void tabCloseRequested(int);

    void makePatientSelectionTab();
    void makePatientSummaryTab(int);
    void makeStreamTab(const QString&);
    void makeResultTab(const QString&);

// Variables ////////////////////////////////////////////
public:
    Configuration* m_pConfiguration;
    HvnSqlDataBase* m_pHvnSqlDataBase;
	
private:
    Ui::MainWindow *ui;

    // Layout
    QGridLayout *m_pGridLayout;

private:
    // Tab objects
    QTabWidget *m_pTabWidget;
    std::vector<QDialog *> m_vectorTabViews;

	QHomeTab *m_pHomeTab;
    QPatientSelectionTab *m_pPatientSelectionTab;
    QStreamTab *m_pStreamTab;

	int m_nCurTabIndex;
};

#endif // MAINWINDOW_H
