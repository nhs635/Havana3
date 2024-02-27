#ifndef VIEWOPTIONTAB_H
#define VIEWOPTIONTAB_H

#include <QtWidgets>
#include <QtCore>

#include <Havana3/Configuration.h>


class Configuration;
class QPatientSummaryTab;
class QStreamTab;
class QResultTab;
class QViewTab;
class QImageView;

enum FlimColormapType
{
	_HSV_ = 0,
	_TCT_NEW_ = 1
};

enum VisualizationMode
{
	_FLIM_PARAMETERS_ = 0,
	_ML_PREDICTION_ = 1
};

enum FLImParameters
{
	_LIFETIME_ = 0,
	_INTENSITY_PROP_ = 1,
	_INTENSITY_RATIO_ = 2,
	_NONE_ = 3
};

enum MLPrediction
{
	_RF_COMPO_ = 0,
	_SVM_SOFTMAX_ = 1,
	_SVM_LOGISTICS_ = 2	
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
	inline QRadioButton* getRadioButtonMLPrediction() const { return m_pRadioButton_MLPrediction; }
	inline QComboBox* getEmissionChannelComboBox() const { return m_pComboBox_EmissionChannel; }
	inline QRadioButton* getRadioButtonLifetime() const { return m_pRadioButton_Lifetime; }
	inline QRadioButton* getRadioButtonIntensityProp() const { return m_pRadioButton_IntensityProp; }
	inline QRadioButton* getRadioButtonIntensityRatio() const { return m_pRadioButton_IntensityRatio; }	
	inline QRadioButton* getRadioButtonRandomForest() const { return m_pRadioButton_RandomForest; }
	inline QRadioButton* getRadioButtonSVMSoftmax() const { return m_pRadioButton_SVMSoftmax; }
	inline QRadioButton* getRadioButtonSVMLogistics() const { return m_pRadioButton_SVMLogistics; }

	inline QLabel* getLabelFlimDelaySync() const { return m_pLabel_FlimDelaySync; }
	inline QScrollBar* getScrollBarFlimDelaySync() const { return m_pScrollBar_FlimDelaySync; }
	
	inline int getCurrentCircOffset() const { return m_pScrollBar_CircOffset->value(); }
	inline int getCurrentRotation() const { return m_pScrollBar_Rotation->value(); }
	
private:
    void createFlimVisualizationOptionTab();
    void createOctVisualizationOptionTab();
	void createSyncVisualizationOptionTab();

public slots:
    // FLIm visualization option control
	void changeVisualizationMode(int);
	void changeEmissionChannel(int);
	void changeFlimColormapType(int);
	void changeFLImParameters(int);
	void changeMLPrediction(int);

private slots:
    void adjustFlimContrast();
	void changePlaqueCompositionShowingMode();
	void changeMergeCompositionMode();
	void changeLogisticsNormalizeMode();

	void setCircOffset(int);
	void rotateImage(int);
	void verticalMirroring(bool);
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
	QGroupBox *m_pGroupBox_FlimVisualization;

    // FLIm visualization option widgets
	QLabel *m_pLabel_VisualizationMode;
	QRadioButton *m_pRadioButton_FLImParameters;
	QRadioButton *m_pRadioButton_MLPrediction;
	QButtonGroup *m_pButtonGroup_VisualizationMode;
	
    QLabel *m_pLabel_EmissionChannel;
    QComboBox *m_pComboBox_EmissionChannel;
	QComboBox *m_pComboBox_FlimColormap;

	QLabel *m_pLabel_FLImParameters;
	QRadioButton *m_pRadioButton_Lifetime;
	QRadioButton *m_pRadioButton_IntensityProp;
	QRadioButton *m_pRadioButton_IntensityRatio;
	QRadioButton *m_pRadioButton_None;
	QButtonGroup *m_pButtonGroup_FLImParameters;

	QLabel *m_pLabel_MLPrediction;
	QRadioButton *m_pRadioButton_RandomForest;
	QRadioButton *m_pRadioButton_SVMSoftmax;
	QRadioButton *m_pRadioButton_SVMLogistics;
	QButtonGroup *m_pButtonGroup_MLPrediction;
	
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

	QLabel *m_pLabel_PlaqueComposition;
	QCheckBox *m_pCheckBox_PlaqueComposition[ML_N_CATS];
	QCheckBox *m_pCheckBox_NormalFibrousMerge;
	QCheckBox *m_pCheckBox_MacTcfaMerge;
	QCheckBox *m_pCheckBox_LogisticsNormalize;

    // OCT visualization option widgets
	QLabel *m_pLabel_CircOffset;
	QScrollBar *m_pScrollBar_CircOffset;

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
