#ifndef VIEWOPTIONTAB_H
#define VIEWOPTIONTAB_H

#include <QtWidgets>
#include <QtCore>


class Configuration;
class QStreamTab;
class QResultTab;
class QViewTab;
class QImageView;

class ViewOptionTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit ViewOptionTab(bool is_streaming = true, QWidget *parent = nullptr);
    virtual ~ViewOptionTab();

// Methods //////////////////////////////////////////////
public:    
	inline QStreamTab* getStreamTab() const { return m_pStreamTab; }
	inline QResultTab* getResultTab() const { return m_pResultTab; }
    inline QViewTab* getViewTab() const { return m_pViewTab; }
    inline QGroupBox* getLayoutBox() const { return m_pGroupBox_ViewOption; }

	inline int getCurrentRotation() const { return m_pScrollBar_Rotation->value(); }
	inline bool getIntensityRatioMode() const { return m_pCheckBox_IntensityRatio->isChecked(); }
	inline bool isIntensityWeighted() const { return m_pCheckBox_IntensityWeightedLifetimeMap->isChecked(); }
	inline bool isClassification() const { return m_pCheckBox_Classification->isChecked(); }

public:
	void setWidgetsValue();

private:
    void createFlimVisualizationOptionTab(bool is_streaming);
    void createOctVisualizationOptionTab(bool is_streaming);

private slots:
    // FLIm visualization option control
	void enableIntensityRatioMode(bool);
	void changeIntensityRatioRef(int);
	void enableIntensityWeightingMode(bool);
    void changeEmissionChannel(int);
    void adjustFlimContrast();
//	void createPulseReviewDlg();
//	void deletePulseReviewDlg();
	void enableClassification(bool);

    void changeViewMode(int);
	void adjustDecibelRange();
	void adjustOctGrayContrast();
//	void createLongitudinalViewDlg();
//	void deleteLongitudinalViewDlg();
	void rotateImage(int);

// Variables ////////////////////////////////////////////
private:
    Configuration* m_pConfig;
    QStreamTab* m_pStreamTab;
    QResultTab* m_pResultTab;
    QViewTab* m_pViewTab;

private:
    // Visualization option widgets
    QGroupBox *m_pGroupBox_ViewOption;

    // FLIm visualization option widgets
    QGroupBox *m_pGroupBox_FlimVisualization;

    QLabel *m_pLabel_EmissionChannel;
    QComboBox *m_pComboBox_EmissionChannel;

	QCheckBox *m_pCheckBox_IntensityRatio;
	QComboBox *m_pComboBox_IntensityRef;

	QCheckBox *m_pCheckBox_IntensityWeightedLifetimeMap;

    QLabel *m_pLabel_NormIntensity;
    QLabel *m_pLabel_Lifetime;
    QLineEdit *m_pLineEdit_IntensityMax;
    QLineEdit *m_pLineEdit_IntensityMin;
    QLineEdit *m_pLineEdit_LifetimeMax;
    QLineEdit *m_pLineEdit_LifetimeMin;
    QImageView *m_pImageView_IntensityColorbar;
    QImageView *m_pImageView_LifetimeColorbar;
	
    QPushButton *m_pPushButton_PulseReview;

	QCheckBox *m_pCheckBox_Classification;;

    // OCT visualization option widgets
    QGroupBox *m_pGroupBox_OctVisualization;

	QRadioButton *m_pRadioButton_CircularView;
	QRadioButton *m_pRadioButton_RectangularView;
	QButtonGroup *m_pButtonGroup_ViewMode;

    QLineEdit *m_pLineEdit_OctGrayMax;
    QLineEdit *m_pLineEdit_OctGrayMin;
    QImageView *m_pImageView_OctGrayColorbar;

	QLabel *m_pLabel_Rotation;
	QScrollBar *m_pScrollBar_Rotation;
	
	QLabel *m_pLabel_DecibelRange;
	QLineEdit *m_pLineEdit_DecibelMax;
	QLineEdit *m_pLineEdit_DecibelMin;
	QImageView *m_pImageView_Colorbar;

};

#endif // VIEWOPTIONTAB_H
