#ifndef PULSEREVIEWTAB_H
#define PULSEREVIEWTAB_H

#include <QtWidgets>
#include <QtCore>

#include <Havana3/Viewer/QScope.h>
#include <Havana3/Dialog/FlimCalibTab.h>

#include <iostream>
#include <vector>

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
	inline QGroupBox* getLayoutBox() const { return m_pGroupBox_PulseReviewTab; }
	inline int getCurrentAline() const { return m_pSlider_CurrentAline->value(); }
	inline void setCurrentAline(int aline) { m_pSlider_CurrentAline->setValue(aline); }

private:
	void createPulseView();
	void createHistogram();
	
public slots :
	void showWindow(bool);
	void showMeanDelay(bool);
	void drawPulse(int);
	void changePulseType();
	void restoreOffsetValues();
	void updateDelayOffset();
	void addRoi(bool);
	void modifyRoi(bool);
	void deleteRoi();
	void set();
	void cancel();
	void selectRow(int, int, int, int);
	void changePlaqueType(int);

public:
	void saveRois();
	void loadRois();

// Variables ////////////////////////////////////////////
private:	
	Configuration* m_pConfig;
	Configuration* m_pConfigTemp;
	QResultTab* m_pResultTab;
    QViewTab* m_pViewTab;
	FLImProcess* m_pFLIm;

public:
	int m_start, m_end;
	std::vector<QStringList> m_vectorRois;
	int m_totalRois;
	bool m_bFromTable;

private:
	Histogram* m_pHistogramIntensity;
	Histogram* m_pHistogramLifetime;

private:
	// Layout
	QVBoxLayout *m_pVBoxLayout;
	QGroupBox *m_pGroupBox_PulseReviewTab;

	// Widgets for pulse view
	QScope *m_pScope_PulseView;
	
	QCheckBox *m_pCheckBox_ShowWindow;
	QCheckBox *m_pCheckBox_ShowMeanDelay;

	QLabel *m_pLabel_CurrentAline;
	QSlider *m_pSlider_CurrentAline;
	QLabel *m_pLabel_PulseType;
	QComboBox *m_pComboBox_PulseType;
	
	QLabel *m_pLabel_DelayTimeOffset;
	QLineEdit *m_pLineEdit_DelayTimeOffset[3];
	QLabel *m_pLabel_NanoSec;
	QPushButton *m_pPushButton_Update;

	QTableWidget *m_pTableWidget_CurrentResult;

	// Widgets for arc ROI labeling
	QPushButton *m_pToggleButton_AddRoi;
	QPushButton *m_pToggleButton_ModifyRoi;
	QPushButton *m_pPushButton_DeleteRoi;
	QPushButton *m_pPushButton_Set;
	QPushButton *m_pPushButton_Cancel;

	QLabel *m_pLabel_PlaqueType;
	QComboBox *m_pComboBox_PlaqueType;

	QLabel *m_pLabel_Comments;
	QLineEdit *m_pLineEdit_Comments;

	QTableWidget *m_pTableWidget_RoiList;

	// Histogram widgets
	QLabel *m_pLabel_FluIntensity;
	QRenderArea3 *m_pRenderArea_FluIntensity;
	QImageView *m_pColorbar_FluIntensity;
	QLabel *m_pLabel_FluIntensityMin;
	QLabel *m_pLabel_FluIntensityMax;
	QLabel *m_pLabel_FluIntensityVal[3];

	QLabel *m_pLabel_FluLifetime;
	QRenderArea3 *m_pRenderArea_FluLifetime;
	QImageView *m_pColorbar_FluLifetime;
	QLabel *m_pLabel_FluLifetimeMin;
	QLabel *m_pLabel_FluLifetimeMax;
	QLabel *m_pLabel_FluLifetimeVal[3];
};

#endif // PULSEREVIEWTAB_H
