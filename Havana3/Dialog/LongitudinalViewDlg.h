#ifndef LONGITUDINALVIEWDLG_H
#define LONGITUDINALVIEWDLG_H

#include <QObject>
#include <QtWidgets>
#include <QtCore>

#include <Havana3/Configuration.h>
#include <Havana3/Viewer/QImageView.h>

#include <Common/medfilt.h>
#include <Common/ImageObject.h>

class MainWindow;
class QProcessingTab;
class QViewTab;


class LongitudinalViewDlg : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit LongitudinalViewDlg(QWidget *parent = 0);
    virtual ~LongitudinalViewDlg();

// Methods //////////////////////////////////////////////
private:
	void keyPressEvent(QKeyEvent *e);    

public:
	inline QImageView* getImageView() const { return m_pImageView_LongitudinalView; }
	inline int getCurrentAline() const { return m_pSlider_CurrentAline->value(); }
	inline void setCurrentAline(int aline) { m_pSlider_CurrentAline->setValue(aline); }

public:
	void setWidgets(bool);

public slots : // widgets
	void drawLongitudinalImage(int);

signals:
	void paintLongiImage(uint8_t*);

// Variables ////////////////////////////////////////////
private:	
    MainWindow* m_pMainWnd;
	Configuration* m_pConfig;
	QProcessingTab* m_pProcessingTab;
    QViewTab* m_pViewTab;

public:
	ImageObject *m_pImgObjOctLongiImage; 
	ImageObject *m_pImgObjIntensity;
	ImageObject *m_pImgObjLifetime;
	ImageObject *m_pImgObjIntensityWeightedLifetime;
	
private:
	medfilt* m_pMedfilt;
	
private:
	QImageView *m_pImageView_LongitudinalView;
	QLabel *m_pLabel_CurrentAline;
	QSlider *m_pSlider_CurrentAline;
};

#endif // LONGITUDINALVIEWDLG_H
