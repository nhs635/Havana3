#ifndef PULSEREVIEWTAB_H
#define PULSEREVIEWTAB_H

#include <QObject>
#include <QtWidgets>
#include <QtCore>

#include <Havana3/Configuration.h>
#include <Havana3/Viewer/QScope.h>

#include <Common/array.h>
#include <Common/callback.h>

class QViewTab;
class FLImProcess;


class PulseReviewTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit PulseReviewTab(QWidget *parent = 0);
    virtual ~PulseReviewTab();

// Methods //////////////////////////////////////////////
private:
	void keyPressEvent(QKeyEvent *e);

public:
	inline int getCurrentAline() const { return m_pSlider_CurrentAline->value(); }
	inline void setCurrentAline(int aline) { m_pSlider_CurrentAline->setValue(aline); }
	
public slots : // widgets
	void drawPulse(int);
	void changeType();

signals:
	void plotRoiPulse(FLImProcess*, int);

// Variables ////////////////////////////////////////////
private:	
	Configuration* m_pConfig;
    QViewTab* m_pViewTab;
	FLImProcess* m_pFLIm;

private:
	// Widgets for pulse view
	QScope *m_pScope_PulseView;
	
	QLabel *m_pLabel_CurrentAline;
	QSlider *m_pSlider_CurrentAline;
	QLabel *m_pLabel_PulseType;
	QComboBox *m_pComboBox_PulseType;
};

#endif // PULSEREVIEWTAB_H
