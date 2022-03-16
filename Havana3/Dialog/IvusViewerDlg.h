#ifndef IVUSVIEWERDLG_H
#define IVUSVIEWERDLG_H

#include <QtWidgets>
#include <QtCore>

#include <Havana3/Viewer/QImageView.h>

#include <Common/array.h>

#include <iostream>
#include <vector>
#include <thread>

#define IVUS_IMG_SIZE 512

class Configuration;
class QResultTab;

class IvusViewerDlg : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit IvusViewerDlg(QWidget *parent = 0);
    virtual ~IvusViewerDlg();
		
// Methods //////////////////////////////////////////////
private:
	void keyPressEvent(QKeyEvent *);

private:
	void createWidgets();

private slots:
	void loadIvusData(bool);
	void pickAsMatchedFrame();
	void play(bool);
	void visualizeImage(int);
	void imageRotated(int);

public:
	void getIvusImage(int frame, int angle, np::Uint8Array2& dst);

signals:
	void paintIvusImage(uint8_t*);
	void playingDone();

// Variables ////////////////////////////////////////////
private:
	Configuration* m_pConfig;
	Configuration* m_pConfigTemp;
	QResultTab* m_pResultTab;
	
public:
	std::vector<np::Uint8Array2> m_vectorIvusImages;
	np::Uint8Array2 m_imageBuffer;
	std::vector<QStringList> m_vectorMatches;

private:
	std::thread playing;
	bool _running;

private:
	QVBoxLayout *m_pVBoxLayout;
    QGroupBox *m_pGroupBox_IvusViewer;
	
	QImageView *m_pImageView_Ivus;

	// Navigation widgets
	QPushButton *m_pPushButton_Load;
	QPushButton *m_pPushButton_Pick;

	QPushButton *m_pToggleButton_Play;
	QPushButton *m_pPushButton_Decrement;
	QPushButton *m_pPushButton_Increment;
	QLabel *m_pLabel_SelectFrame;
	QSlider *m_pSlider_SelectFrame;
	QLabel *m_pLabel_Rotation;
	QSlider *m_pSlider_Rotation;
};

#endif // IVUSVIEWERDLG_H
