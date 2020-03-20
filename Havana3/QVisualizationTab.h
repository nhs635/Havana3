#ifndef QVISUALIZATIONTAB_H
#define QVISUALIZATIONTAB_H

#include <QDialog>
#include <QtWidgets>
#include <QtCore>

#include <Havana3/Configuration.h>

#include <Common/circularize.h>
#include <Common/medfilt.h>
#include <Common/ImageObject.h>
#include <Common/basic_functions.h>
#include <Common/ann.h>

#include <iostream>
#include <vector>

class QStreamTab;
class QResultTab;

class QImageView;

class PulseReviewDlg;
class LongitudinalViewDlg;


class QVisualizationTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit QVisualizationTab(bool is_streaming = true, QWidget *parent = nullptr);
    virtual ~QVisualizationTab();

// Methods //////////////////////////////////////////////
public:
    inline QVBoxLayout* getLayout() const { return m_pVBoxLayout; }
	inline QStreamTab* getStreamTab() const { return m_pStreamTab; }
	inline QResultTab* getResultTab() const { return m_pResultTab; }
    inline QGroupBox* getVisualizationWidgetsBox() const { return m_pGroupBox_VisualizationWidgets; }
	inline QGroupBox* getEnFaceWidgetsBox() const { return m_pGroupBox_EnFaceWidgets; }
	inline QImageView* getRectImageView() const { return m_pImageView_RectImage; }
	inline QImageView* getCircImageView() const { return m_pImageView_CircImage; }
	inline PulseReviewDlg* getPulseReviewDlg() const { return m_pPulseReviewDlg; }
	inline LongitudinalViewDlg* getLongitudinalViewDlg() const { return m_pLongitudinalViewDlg; }
	inline void setCurrentFrame(int frame) { m_pSlider_SelectFrame->setValue(frame); }
    inline int getCurrentFrame() const { return m_pSlider_SelectFrame->value(); }
	inline int getCurrentRotation() const { return m_pScrollBar_Rotation->value(); }
	inline bool getIntensityRatioMode() const { return m_pCheckBox_IntensityRatio->isChecked(); }
	inline bool isIntensityWeighted() const { return m_pCheckBox_IntensityWeightedLifetimeMap->isChecked(); }
	inline ImageObject* getImgObjIntensityWeightedLifetimeMap() const { return m_pImgObjIntensityWeightedLifetimeMap; }
	inline bool isClassification() const { return m_pCheckBox_Classification->isChecked(); }

public:
	void setWidgetsValue();

private:
	void createNavigationTab();
    void createFlimVisualizationOptionTab(bool is_streaming);
    void createOctVisualizationOptionTab(bool is_streaming);
	void createEnFaceMapTab();

public:
	void setBuffers(Configuration* pConfig);
	void setOctDecibelContrastWidgets(bool enabled);
	void setOuterSheathLines(bool setting);

public slots:
	void setWidgetsEnabled(bool enabled, Configuration* pConfig);
	void visualizeEnFaceMap(bool scaling = true);
	void visualizeImage(uint8_t* oct_im, float* intensity, float* lifetime);
	void visualizeImage(int frame);

private slots:
	void constructRgbImage(ImageObject*, ImageObject*, ImageObject*, ImageObject*);

	void measureDistance(bool);
	
	void enableIntensityRatioMode(bool);
	void changeIntensityRatioRef(int);
	void enableIntensityWeightingMode(bool);
    void changeEmissionChannel(int);
    void adjustFlimContrast();
	void createPulseReviewDlg();
	void deletePulseReviewDlg();
	void enableClassification(bool);

    void changeViewMode(int);
	void adjustDecibelRange();
	void adjustOctGrayContrast();
	void createLongitudinalViewDlg();
	void deleteLongitudinalViewDlg();
	void rotateImage(int);

signals:
	void setWidgets(bool enabled, Configuration* pConfig);

	void drawImage(uint8_t*, float*, float*);
	void makeRgb(ImageObject*, ImageObject*, ImageObject*, ImageObject*);

	void paintOctProjection(uint8_t*);
	void paintIntensityMap(uint8_t*);
	void paintLifetimeMap(uint8_t*);

// Variables ////////////////////////////////////////////
private:
	QStreamTab* m_pStreamTab;
	QResultTab* m_pResultTab;
    Configuration* m_pConfig;

public:
	// Visualization buffers
	np::Uint8Array2 m_visImage;
	np::FloatArray2 m_visIntensity;
	np::FloatArray2 m_visMeanDelay;
	np::FloatArray2 m_visLifetime;

public: // for post processing
	std::vector<np::Uint8Array2> m_vectorOctImage;
	np::Uint8Array2 m_octProjection;
	std::vector<np::FloatArray2> m_intensityMap; // (256 x N) x 3
	std::vector<np::FloatArray2> m_lifetimeMap; // (256 x N) x 3
	std::vector<np::FloatArray2> m_vectorPulseCrop;
	std::vector<np::FloatArray2> m_vectorPulseMask;

public:
	// Image visualization buffers
	ImageObject *m_pImgObjRectImage;
	ImageObject *m_pImgObjCircImage;
	ImageObject *m_pImgObjIntensity;
	ImageObject *m_pImgObjLifetime;

	np::Uint8Array2 m_visOctProjection;
	ImageObject *m_pImgObjIntensityMap;
	ImageObject *m_pImgObjLifetimeMap;
	ImageObject *m_pImgObjIntensityWeightedLifetimeMap;

public:
	circularize* m_pCirc;
	medfilt* m_pMedfilt;	
	medfilt* m_pMedfiltIntensityMap;
	medfilt* m_pMedfiltLifetimeMap;
	ann* m_pAnn;

private:
    // Layout
    QVBoxLayout *m_pVBoxLayout;

    // Image viewer widgets
    QImageView *m_pImageView_RectImage;
    QImageView *m_pImageView_CircImage;

    // Visualization widgets groupbox
    QGroupBox *m_pGroupBox_VisualizationWidgets;

	// Navigation widgets
	QGroupBox *m_pGroupBox_Navigation;

	QSlider *m_pSlider_SelectFrame;
	QLabel *m_pLabel_SelectFrame;

	QPushButton* m_pToggleButton_MeasureDistance;

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
	PulseReviewDlg *m_pPulseReviewDlg;

	QCheckBox *m_pCheckBox_Classification;;

    // OCT visualization option widgets
    QGroupBox *m_pGroupBox_OctVisualization;

	QRadioButton *m_pRadioButton_CircularView;
	QRadioButton *m_pRadioButton_RectangularView;
	QButtonGroup *m_pButtonGroup_ViewMode;

    QLineEdit *m_pLineEdit_OctGrayMax;
    QLineEdit *m_pLineEdit_OctGrayMin;
    QImageView *m_pImageView_OctGrayColorbar;
	
	QPushButton *m_pPushButton_LongitudinalView;
	LongitudinalViewDlg *m_pLongitudinalViewDlg;

	QLabel *m_pLabel_Rotation;
	QScrollBar *m_pScrollBar_Rotation;
	
	QLabel *m_pLabel_DecibelRange;
	QLineEdit *m_pLineEdit_DecibelMax;
	QLineEdit *m_pLineEdit_DecibelMin;
	QImageView *m_pImageView_Colorbar;

	// En face widgets
	QGroupBox *m_pGroupBox_EnFaceWidgets;

	QLabel *m_pLabel_OctProjection;
	QLabel *m_pLabel_IntensityMap;
	QLabel *m_pLabel_LifetimeMap;

	QImageView *m_pImageView_OctProjection;
	QImageView *m_pImageView_IntensityMap;
	QImageView *m_pImageView_LifetimeMap;
};

#endif // QVISUALIZATIONTAB_H
