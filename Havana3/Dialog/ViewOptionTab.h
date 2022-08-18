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
	_FLIM_PARAMETERS_ = 0,
	_RF_PREDICTION_ = 1
};

enum FLImParameters
{
	_LIFETIME_ = 0,
	_INTENSITY_PROP_ = 1,
	_INTENSITY_RATIO_ = 2,
	_NONE_ = 3
};

enum RFPrediction
{
	_PLAQUE_COMPOSITION_ = 0,
	_INFLAMMATION_ = 1
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

	inline QRadioButton* getRadioButtonFLImParameters() const { return m_pRadioButton_FLImParameters; }
	inline QRadioButton* getRadioButtonRFPrediction() const { return m_pRadioButton_RFPrediction; }
	inline QComboBox* getEmissionChannelComboBox() const { return m_pComboBox_EmissionChannel; }
	inline QRadioButton* getRadioButtonLifetime() const { return m_pRadioButton_Lifetime; }
	inline QRadioButton* getRadioButtonIntensityProp() const { return m_pRadioButton_IntensityProp; }
	inline QRadioButton* getRadioButtonIntensityRatio() const { return m_pRadioButton_IntensityRatio; }
	inline QRadioButton* getRadioButtonInflammation() const { return m_pRadioButton_Inflammation; }
	inline QRadioButton* getRadioButtonPlaqueComposition() const { return m_pRadioButton_PlaqueComposition; }

	inline QLabel* getLabelFlimDelaySync() const { return m_pLabel_FlimDelaySync; }
	inline QScrollBar* getScrollBarFlimDelaySync() const { return m_pScrollBar_FlimDelaySync; }
	
	inline int getCurrentRotation() const { return m_pScrollBar_Rotation->value(); }
	
private:
    void createFlimVisualizationOptionTab();
    void createOctVisualizationOptionTab();
	void createSyncVisualizationOptionTab();

public slots:
    // FLIm visualization option control
	void changeVisualizationMode(int);
	void changeEmissionChannel(int);
	void changeFLImParameters(int);
	void changeRFPrediction(int);

private slots:
    void adjustFlimContrast();
	void adjustInflammationContrast();

	void rotateImage(int);
	void verticalMirriong(bool);
	void reflectionRemoval(bool);
	void changeReflectionDistance(const QString &);
	void changeReflectionLevel(const QString &);
	void adjustDecibelRange();
	void adjustOctGrayContrast();

	void setIntraFrameSync(int);
	void setInterFrameSync(int);
	void setFlimDelaySync(int);

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
	QLabel *m_pLabel_VisualizationMode;
	QRadioButton *m_pRadioButton_FLImParameters;
	QRadioButton *m_pRadioButton_RFPrediction;
	QButtonGroup *m_pButtonGroup_VisualizationMode;
	
    QLabel *m_pLabel_EmissionChannel;
    QComboBox *m_pComboBox_EmissionChannel;

	QLabel *m_pLabel_FLImParameters;
	QRadioButton *m_pRadioButton_Lifetime;
	QRadioButton *m_pRadioButton_IntensityProp;
	QRadioButton *m_pRadioButton_IntensityRatio;
	QRadioButton *m_pRadioButton_None;
	QButtonGroup *m_pButtonGroup_FLImParameters;

	QLabel *m_pLabel_RFPrediction;
	QRadioButton *m_pRadioButton_Inflammation;
	QRadioButton *m_pRadioButton_PlaqueComposition;
	QButtonGroup *m_pButtonGroup_RFPrediction;
	
    QLabel *m_pLabel_NormIntensity;
    QLabel *m_pLabel_Lifetime;
	QLabel *m_pLabel_IntensityProp;
	QLabel *m_pLabel_IntensityRatio;
    QLineEdit *m_pLineEdit_IntensityMax;
    QLineEdit *m_pLineEdit_IntensityMin;
    QLineEdit *m_pLineEdit_LifetimeMax;
    QLineEdit *m_pLineEdit_LifetimeMin;
	QLineEdit *m_pLineEdit_IntensityPropMax;
	QLineEdit *m_pLineEdit_IntensityPropMin;
	QLineEdit *m_pLineEdit_IntensityRatioMax;
	QLineEdit *m_pLineEdit_IntensityRatioMin;
    QImageView *m_pImageView_IntensityColorbar;
    QImageView *m_pImageView_LifetimeColorbar;
	QImageView *m_pImageView_IntensityPropColorbar;
	QImageView *m_pImageView_IntensityRatioColorbar;

	QLabel *m_pLabel_Inflammation;
	QLineEdit *m_pLineEdit_InflammationMax;
	QLineEdit *m_pLineEdit_InflammationMin;
	QImageView *m_pImageView_InflammationColorbar;

	QLabel *m_pLabel_PlaqueComposition;
	QImageView *m_pImageView_PlaqueCompositionColorbar;

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

	QCheckBox *m_pCheckBox_ReflectionRemoval;
	QLineEdit *m_pLineEdit_ReflectionDistance;
	QLineEdit *m_pLineEdit_ReflectionLevel;

	// Manual synchronization
	QLabel *m_pLabel_IntraFrameSync;
	QScrollBar *m_pScrollBar_IntraFrameSync;
	QLabel *m_pLabel_InterFrameSync;
	QScrollBar *m_pScrollBar_InterFrameSync;
	QLabel *m_pLabel_FlimDelaySync;
	QScrollBar *m_pScrollBar_FlimDelaySync;
};

#endif // VIEWOPTIONTAB_H
