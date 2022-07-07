#ifndef EXPORTDLG_H
#define EXPORTDLG_H

#include <QtWidgets>
#include <QtCore>

#include <iostream>
#include <vector>

#include <Havana3/Configuration.h>

#include <Common/array.h>
#include <Common/circularize.h>
#include <Common/medfilt.h>
#include <Common/Queue.h>
#include <Common/ImageObject.h>
#include <Common/basic_functions.h>

class Configuration;
class QResultTab;
class QViewTab;

using ImgObjVector = std::vector<ImageObject*>; 

struct CrossSectionCheckList
{
	bool bCirc, bLongi;
	bool bCircResize, bLongiResize;	
	int nCircDiameter;
	int nLongiWidth, nLongiHeight;
	bool bCh[3];
};

struct EnFaceCheckList
{
	bool bRaw;
	bool bScaled;
	bool bCh[3];
	bool bRfPred;
};


class ExportDlg : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit ExportDlg(QWidget *parent = 0);
    virtual ~ExportDlg();
		
// Methods //////////////////////////////////////////////
private:
	void closeEvent(QCloseEvent *e);
	void keyPressEvent(QKeyEvent *);

private slots:
	void exportResults();
	void findExportPath();
	void saveCrossSections();
	void saveEnFaceMaps();
	void setRange();
	void checkCircImage(bool);
	void checkLongiImage(bool);
	void enableCircResize(bool);
	void enableLongiResize(bool);
	void checkResizeValue();
	void checkEnFaceOptions();
	void setWidgetsEnabled(bool, int);

signals:
	void setWidgets(bool, int);
	void savedSingleFrame(int);

private:
#ifndef NEXT_GEN_SYSTEM
	void scaling(std::vector<np::Uint8Array2>& vectorOctImage, std::vector<ImageObject*>& vectorLifetimeMap, CrossSectionCheckList checkList);
#else
	void scaling(std::vector<np::FloatArray2>& vectorOctImage, std::vector<ImageObject*>& vectorLifetimeMap, CrossSectionCheckList checkList);
#endif
	void converting(CrossSectionCheckList checkList);
	void circularizing(CrossSectionCheckList checkList);
	void circWriting(CrossSectionCheckList checkList);

// Variables ////////////////////////////////////////////
private:
	Configuration* m_pConfig;
	Configuration* m_pConfigTemp;
	QResultTab* m_pResultTab;
	QViewTab* m_pViewTab;

private:
	int m_nSavedFrames;
	QString m_exportPath;
	
public:
	bool m_bIsSaving;
	Qt::TransformationMode m_defaultTransformation;

private: // for threading operation
	Queue<ImgObjVector*> m_syncQueueConverting;	
	Queue<ImgObjVector*> m_syncQueueLongitudinal;
	Queue<ImgObjVector*> m_syncQueueCircularizing;
	Queue<ImgObjVector*> m_syncQueueCircWriting;

private:
	// Export
	QPushButton* m_pPushButton_Export;
	QProgressBar* m_pProgressBar_Export;

	QLabel* m_pLabel_ExportPath;
	QLineEdit* m_pLineEdit_ExportPath;
	QPushButton* m_pPushButton_ExportPath;

	// Set Range
	QLabel* m_pLabel_Range;
	QLineEdit* m_pLineEdit_RangeStart;
	QLineEdit* m_pLineEdit_RangeEnd;

	// Save Cross-sections
	QGroupBox* m_pGroupBox_CrossSections;

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
	
	// Save En Face Chemogram
	QGroupBox* m_pGroupBox_EnFaceChemogram;

	QCheckBox* m_pCheckBox_RawData;
	QCheckBox* m_pCheckBox_ScaledImage;

	QCheckBox* m_pCheckBox_EnFaceCh1;
	QCheckBox* m_pCheckBox_EnFaceCh2;
	QCheckBox* m_pCheckBox_EnFaceCh3;
	QCheckBox* m_pCheckBox_RFPrediction;
};

#endif // EXPORTDLG_H
