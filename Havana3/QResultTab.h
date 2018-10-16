#ifndef QRESULTTAB_H
#define QRESULTTAB_H

#include <QDialog>
#include <QtWidgets>
#include <QtCore>

#include <Havana3/Configuration.h>

class MainWindow;
class QProcessingTab;
class QVisualizationTab;


class QResultTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit QResultTab(QWidget *parent = nullptr);
	virtual ~QResultTab();

// Methods //////////////////////////////////////////////
protected:
	void keyPressEvent(QKeyEvent *);

public:
	inline MainWindow* getMainWnd() const { return m_pMainWnd; }
	inline QProcessingTab* getProcessingTab() const { return m_pProcessingTab; }
	inline QVisualizationTab* getVisualizationTab() const { return m_pVisualizationTab; }

public:
	void changeTab(bool status);


// Variables ////////////////////////////////////////////
private: // main pointer
	MainWindow* m_pMainWnd;
	Configuration* m_pConfig;

	QProcessingTab* m_pProcessingTab;
	QVisualizationTab* m_pVisualizationTab;
		
private:
    // Layout
    QHBoxLayout *m_pHBoxLayout;

	// Group Boxes
	QGroupBox *m_pGroupBox_ProcessingTab;
	QGroupBox *m_pGroupBox_VisualizationTab;
};

#endif // QRESULTTAB_H
