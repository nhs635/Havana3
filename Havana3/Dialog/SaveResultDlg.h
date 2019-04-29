#ifndef SAVERESULTDLG_H
#define SAVERESULTDLG_H

#include <QObject>
#include <QtWidgets>
#include <QtCore>

#include <Havana3/Configuration.h>

#include <iostream>
#include <vector>

#include <Common/array.h>
#include <Common/circularize.h>
#include <Common/medfilt.h>
#include <Common/Queue.h>
#include <Common/ImageObject.h>
#include <Common/basic_functions.h>

class QProcessingTab;
class QVisualizationTab;

using ImgObjVector = std::vector<ImageObject*>; 

struct CrossSectionCheckList
{
	bool bRect, bCirc, bLongi;
	bool bRectResize, bCircResize, bLongiResize;
	int nRectWidth, nRectHeight;
	int nCircDiameter;
	int nLongiWidth, nLongiHeight;
	bool bCh[3];
	bool bMulti;
};

struct EnFaceCheckList
{
	bool bRaw;
	bool bScaled;
	bool bCh[3];
	bool bOctProj;
};


class SaveResultDlg : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit SaveResultDlg(QWidget *parent = 0);
    virtual ~SaveResultDlg();
		
// Methods //////////////////////////////////////////////
private:
	void closeEvent(QCloseEvent *e);
	void keyPressEvent(QKeyEvent *);

private slots:
	void saveCrossSections();
	void saveEnFaceMaps();
	void setRange();
	void checkRectImage(bool);
	void checkCircImage(bool);
	void checkLongiImage(bool);
	void enableRectResize(bool);
	void enableCircResize(bool);
	void enableLongiResize(bool);
	void checkResizeValue();
	void checkChannels();
	void checkEnFaceOptions();
	void setWidgetsEnabled(bool);

signals:
	void setWidgets(bool);
	void savedSingleFrame(int);

private:
	void scaling(std::vector<np::Uint8Array2>& vectorOctImage,
		std::vector<np::Uint8Array2>& intensityMap, std::vector<np::Uint8Array2>& lifetimeMap,
		CrossSectionCheckList checkList);
	void converting(CrossSectionCheckList checkList);
	void rectWriting(CrossSectionCheckList checkList);
	void circularizing(CrossSectionCheckList checkList);
	void circWriting(CrossSectionCheckList checkList);

// Variables ////////////////////////////////////////////
private:
	Configuration* m_pConfig;
	QProcessingTab* m_pProcessingTab;
	QVisualizationTab* m_pVisTab;

private:
	int m_nSavedFrames;
	
public:
	bool m_bIsSaving;
	Qt::TransformationMode m_defaultTransformation;

private: // for threading operation
	Queue<ImgObjVector*> m_syncQueueConverting;
	Queue<ImgObjVector*> m_syncQueueRectWriting;
	Queue<ImgObjVector*> m_syncQueueLongitudinal;
	Queue<ImgObjVector*> m_syncQueueCircularizing;
	Queue<ImgObjVector*> m_syncQueueCircWriting;

private:
	// Save Cross-sections
	QPushButton* m_pPushButton_SaveCrossSections;
	QCheckBox* m_pCheckBox_RectImage;
	QCheckBox* m_pCheckBox_ResizeRectImage;
	QLineEdit* m_pLineEdit_RectWidth;
	QLineEdit* m_pLineEdit_RectHeight;
	QCheckBox* m_pCheckBox_CircImage;
	QCheckBox* m_pCheckBox_ResizeCircImage;
	QLineEdit* m_pLineEdit_CircDiameter;
	QCheckBox* m_pCheckBox_LongiImage;
	QCheckBox* m_pCheckBox_ResizeLongiImage;
	QLineEdit* m_pLineEdit_LongiWidth;
	QLineEdit* m_pLineEdit_LongiHeight;

	QCheckBox* m_pCheckBox_CrossSectionCh1;
	QCheckBox* m_pCheckBox_CrossSectionCh2;
	QCheckBox* m_pCheckBox_CrossSectionCh3;
	QCheckBox* m_pCheckBox_IntensityWeighting;

	// Set Range
	QLabel* m_pLabel_Range;
	QLineEdit* m_pLineEdit_RangeStart;
	QLineEdit* m_pLineEdit_RangeEnd;

	// Save En Face Maps
	QPushButton* m_pPushButton_SaveEnFaceMaps;
	QCheckBox* m_pCheckBox_RawData;
	QCheckBox* m_pCheckBox_ScaledImage;

	QCheckBox* m_pCheckBox_EnFaceCh1;
	QCheckBox* m_pCheckBox_EnFaceCh2;
	QCheckBox* m_pCheckBox_EnFaceCh3;
	QCheckBox* m_pCheckBox_OctMaxProjection;
};

#endif // SAVERESULTDLG_H
