#ifndef PULSEREVIEWTAB_H
#define PULSEREVIEWTAB_H

#include <QtWidgets>
#include <QtCore>

#include <Havana3/Viewer/QScope.h>

class Configuration;
class QResultTab;
class QViewTab;
class FLImProcess;

class PulseReviewTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit PulseReviewTab(QWidget *parent = nullptr);
    virtual ~PulseReviewTab();

// Methods //////////////////////////////////////////////
public:
	inline QResultTab* getResultTab() const { return m_pResultTab; }
	inline QGroupBox* getLayoutBox() const { return m_pGroupBox_PulseReview; }
	inline int getCurrentAline() const { return m_pSlider_CurrentAline->value(); }
	inline void setCurrentAline(int aline) { m_pSlider_CurrentAline->setValue(aline); }
	
public slots :
	void drawPulse(int);
	void changeType();

signals:
	void plotRoiPulse(FLImProcess*, int);

// Variables ////////////////////////////////////////////
private:	
	Configuration* m_pConfigTemp;
	QResultTab* m_pResultTab;
    QViewTab* m_pViewTab;
	FLImProcess* m_pFLIm;

private:
	// Layout
	QGridLayout *m_pGridLayout;
	QGroupBox *m_pGroupBox_PulseReview;

	// Widgets for pulse view
	QScope *m_pScope_PulseView;
	
	QLabel *m_pLabel_CurrentAline;
	QSlider *m_pSlider_CurrentAline;
	QLabel *m_pLabel_PulseType;
	QComboBox *m_pComboBox_PulseType;
};

#endif // PULSEREVIEWTAB_H
