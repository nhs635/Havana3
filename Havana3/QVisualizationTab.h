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

#include <iostream>
#include <vector>

class QStreamTab;
class QResultTab;

class QImageView;


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
    inline QGroupBox* getVisualizationWidgetsBox() const { return m_pGroupBox_VisualizationWidgets; }
	inline QGroupBox* getEnFaceWidgetsBox() const { return m_pGroupBox_EnFaceWidgets; }
	inline QImageView* getRectImageView() const { return m_pImageView_RectImage; }
	inline QImageView* getCircImageView() const { return m_pImageView_CircImage; }
	inline int getCurrentFrame() const { return m_pSlider_SelectFrame->value(); }
    inline int getFlimSyncAdjust() const { return m_pSlider_FlimSyncAdjust->value(); }
	inline bool getIntensityRatioMode() const { return m_pCheckBox_IntensityRatio->isChecked(); }

public:
	void setWidgetsValue();

private:
	void createNavigationTab();
    void createFlimVisualizationOptionTab(bool is_streaming);
    void createOctVisualizationOptionTab(bool is_streaming);
	void createEnFaceMapTab();

public:
	void setBuffers(Configuration* pConfig);

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
	void enableHsvEnhancingMode(bool);
    void changeEmissionChannel(int);
    void changeLifetimeColorTable(int);
	void setFlimInterFrameSync(const QString &);
    void setFlimSyncAdjust(int);
    void adjustFlimContrast();

    void changeVisImage(bool);
    void changeOctColorTable(int);
	void adjustOctGrayContrast();

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

private:
	// Image visualization buffers
	ImageObject *m_pImgObjRectImage;
	ImageObject *m_pImgObjCircImage;
	ImageObject *m_pImgObjIntensity;
	ImageObject *m_pImgObjLifetime;

	np::Uint8Array2 m_visOctProjection;
	ImageObject *m_pImgObjIntensityMap;
	ImageObject *m_pImgObjLifetimeMap;
	ImageObject *m_pImgObjHsvEnhancedMap;

public:
	circularize* m_pCirc;
	medfilt* m_pMedfilt;	
	medfilt* m_pMedfiltIntensityMap;
	medfilt* m_pMedfiltLifetimeMap;

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

    QLabel *m_pLabel_LifetimeColorTable;
    QComboBox *m_pComboBox_LifetimeColorTable;

	QCheckBox *m_pCheckBox_IntensityRatio;
	QComboBox *m_pComboBox_IntensityRef;

	QCheckBox *m_pCheckBox_HsvEnhancedMap;

	QLabel *m_pLabel_FlimInterFrameSync;
	QLineEdit *m_pLineEdit_FlimInterFrameSync;
    QLabel *m_pLabel_FlimSyncAdjust;
    QSlider *m_pSlider_FlimSyncAdjust;

    QLabel *m_pLabel_NormIntensity;
    QLabel *m_pLabel_Lifetime;
    QLineEdit *m_pLineEdit_IntensityMax;
    QLineEdit *m_pLineEdit_IntensityMin;
    QLineEdit *m_pLineEdit_LifetimeMax;
    QLineEdit *m_pLineEdit_LifetimeMin;
    QImageView *m_pImageView_IntensityColorbar;
    QImageView *m_pImageView_LifetimeColorbar;

    // OCT visualization option widgets
    QGroupBox *m_pGroupBox_OctVisualization;

    QCheckBox *m_pCheckBox_CircularizeImage;

    QLabel *m_pLabel_OctColorTable;
    QComboBox *m_pComboBox_OctColorTable;

    QLineEdit *m_pLineEdit_OctGrayMax;
    QLineEdit *m_pLineEdit_OctGrayMin;
    QImageView *m_pImageView_OctGrayColorbar;

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
