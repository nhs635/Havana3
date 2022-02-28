#ifndef VIEWOPTIONTAB_H
#define VIEWOPTIONTAB_H

#include <QtWidgets>
#include <QtCore>


class Configuration;
class QPatientSummaryTab;
class QStreamTab;
class QResultTab;
class QViewTab;
class QImageView;

enum VisualizationMode
{
	_LIFETIME_ = 0,
	_INTENSITY_RATIO_ = 1
};


class ViewOptionTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit ViewOptionTab(QWidget *parent = nullptr);
    virtual ~ViewOptionTab();

// Methods //////////////////////////////////////////////
public:
	inline QPatientSummaryTab* getPatientSummaryTab() const { return m_pPatientSummaryTab; }
	inline QStreamTab* getStreamTab() const { return m_pStreamTab; }
	inline QResultTab* getResultTab() const { return m_pResultTab; }
    inline QViewTab* getViewTab() const { return m_pViewTab; }
    inline QGroupBox* getLayoutBox() const { return m_pGroupBox_ViewOption; }

	inline int getCurrentRotation() const { return m_pScrollBar_Rotation->value(); }
	//inline bool getIntensityRatioMode() const { return m_pCheckBox_IntensityRatio->isChecked(); }	
	inline bool isClassification() const { return m_pCheckBox_Classification->isChecked(); }
	
private:
    void createFlimVisualizationOptionTab();
    void createOctVisualizationOptionTab();
	void createSyncVisualizationOptionTab();

private slots:
    // FLIm visualization option control
	void changeEmissionChannel(int);
	void setVisualizationMode(int);
	void enableClassification(bool);
    void adjustFlimContrast();

	void rotateImage(int);
	void verticalMirriong(bool);
	void adjustDecibelRange();
	void adjustOctGrayContrast();

	void setIntraFrameSync(int);
	void setInterFrameSync(int);

// Variables ////////////////////////////////////////////
private:
    Configuration* m_pConfig;
	Configuration* m_pConfigTemp;
	QPatientSummaryTab* m_pPatientSummaryTab;
    QStreamTab* m_pStreamTab;
    QResultTab* m_pResultTab;
    QViewTab* m_pViewTab;

private:
    // Visualization option widgets
	QGroupBox *m_pGroupBox_ViewOption;
    QVBoxLayout *m_pVBoxLayout_ViewOption;

    // FLIm visualization option widgets
    QLabel *m_pLabel_EmissionChannel;
    QComboBox *m_pComboBox_EmissionChannel;

	QLabel *m_pLabel_VisualizationMode;
	QRadioButton *m_pRadioButton_Lifetime;
	QRadioButton *m_pRadioButton_IntensityRatio;
	QButtonGroup *m_pButtonGroup_Visualization;

	QCheckBox *m_pCheckBox_Classification;;
	
    QLabel *m_pLabel_NormIntensity;
    QLabel *m_pLabel_Lifetime;
	QLabel *m_pLabel_IntensityRatio;
    QLineEdit *m_pLineEdit_IntensityMax;
    QLineEdit *m_pLineEdit_IntensityMin;
    QLineEdit *m_pLineEdit_LifetimeMax;
    QLineEdit *m_pLineEdit_LifetimeMin;
	QLineEdit *m_pLineEdit_IntensityRatioMax;
	QLineEdit *m_pLineEdit_IntensityRatioMin;
    QImageView *m_pImageView_IntensityColorbar;
    QImageView *m_pImageView_LifetimeColorbar;
	QImageView *m_pImageView_IntensityRatioColorbar;

    // OCT visualization option widgets
	QLabel *m_pLabel_Rotation;
	QScrollBar *m_pScrollBar_Rotation;

	QCheckBox *m_pCheckBox_VerticalMirroring;

	QLabel *m_pLabel_OctContrast;
	QLineEdit *m_pLineEdit_DecibelMax;
	QLineEdit *m_pLineEdit_DecibelMin;
    QLineEdit *m_pLineEdit_OctGrayMax;
    QLineEdit *m_pLineEdit_OctGrayMin;
    QImageView *m_pImageView_OctColorbar;

	// Manual synchronization
	QLabel *m_pLabel_IntraFrameSync;
	QScrollBar *m_pScrollBar_IntraFrameSync;
	QLabel *m_pLabel_InterFrameSync;
	QScrollBar *m_pScrollBar_InterFrameSync;
};

#endif // VIEWOPTIONTAB_H
