#ifndef QVIEWTAB_H
#define QVIEWTAB_H

#include <QtWidgets>
#include <QtCore>

#include <Havana3/Configuration.h>

#include <Havana3/Viewer/QImageView.h>

#include <Common/array.h>
#include <Common/circularize.h>
#include <Common/medfilt.h>
#include <Common/ImageObject.h>
#include <Common/basic_functions.h>
#include <Common/random_forest.h>

#include <iostream>
#include <vector>
#include <thread>


class QStreamTab;
class QResultTab;
class QImageView;

class QViewTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit QViewTab(bool is_streaming = true, QWidget *parent = nullptr);
    virtual ~QViewTab();

// Methods //////////////////////////////////////////////
public:    
    inline QWidget* getViewWidget() const { return m_pViewWidget; }
	inline QStreamTab* getStreamTab() const { return m_pStreamTab; }
	inline QResultTab* getResultTab() const { return m_pResultTab; }
	inline QImageView* getCircImageView() const { return m_pImageView_CircImage; }
	inline QWidget* getVisWidget(int i) const { return m_pWidget[i]; }
    inline QImageView* getEnFaceImageView() const { return m_pImageView_EnFace; }
    inline QImageView* getLongiImageView() const { return m_pImageView_Longi; }
	inline QImageView* getIvusImageView() const { return m_pImageView_Ivus; }
	inline circularize* getCirc() const { return m_pCirc; }
	inline medfilt* getMedfiltRect() const { return m_pMedfiltRect; }
	inline medfilt* getMedfiltIntensityMap() const { return m_pMedfiltIntensityMap; }
	inline medfilt* getMedfiltLifetimeMap() const { return m_pMedfiltLifetimeMap; }
	inline medfilt* getMedfiltLongi() const { return m_pMedfiltLongi; }
	inline RandomForest* getForestInfl() const { return m_pForestInfl; }
	inline RandomForest* getForestPlqCompo() const { return m_pForestPlqCompo; }
	inline QPushButton* getPlayButton() const { return m_pToggleButton_Play; }
    inline QSlider* getSliderSelectFrame() const { return m_pSlider_SelectFrame; }
	inline void setVisualizationMode(int mode) { if (!mode) m_pRadioButton_FLImParameters->setChecked(true); 
	else m_pRadioButton_RFPrediction->setChecked(true); changeVisualizationMode(mode);	}
	inline bool getVisualizationMode() { return m_pRadioButton_RFPrediction->isChecked(); }
	inline void setFLImParameters(int ch) { m_pComboBox_FLImParameters->setCurrentIndex(ch); }
	inline int getFLImparameters() { return m_pComboBox_FLImParameters->currentIndex(); }
	inline void setRFPrediction(int mode) { return m_pComboBox_RFPrediction->setCurrentIndex(mode); }
	inline int getRFPrediction() { return m_pComboBox_RFPrediction->currentIndex(); }
	inline void setCurrentFrame(int frame) { m_pSlider_SelectFrame->setValue(frame); }
    inline int getCurrentFrame() { return m_pSlider_SelectFrame->value(); }
	inline int getCurrentAline() { return m_pImageView_CircImage->getRender()->m_pVLineInd[0]; }
	
private:
    void createViewTabWidgets(bool);

public:
	void invalidate();

private:
    void setStreamingBuffersObjects();

public:
	void setBuffers(Configuration* pConfig);
	void setObjects(Configuration* pConfig);
    void setWidgets(Configuration* pConfig);
	void setCircImageViewClickedMouseCallback(const std::function<void(void)> &slot);
	void setEnFaceImageViewClickedMouseCallback(const std::function<void(void)> &slot);
	void setLongiImageViewClickedMouseCallback(const std::function<void(void)> &slot);

public slots:
	void visualizeEnFaceMap(bool scaling = true);
	void visualizeImage(uint8_t* oct_im, float* intensity, float* lifetime);
	void visualizeImage(int frame);
    void visualizeLongiImage(int aline);

private slots:
    void constructCircImage();
	void play(bool);
	void measureDistance(bool);
	void measureArea(bool);
	void changeVisualizationMode(int);
	void changeEmissionChannel(int);
	void changeRFPrediction(int);

public:
	void scaleFLImEnFaceMap(ImageObject* pImgObjIntensityMap, ImageObject* pImgObjLifetimeMap, 
		ImageObject* pImgObjIntensityPropMap, ImageObject* pImgObjIntensityRatioMap, 
		ImageObject* pImgObjInflammationMap, ImageObject* pImgObjPlaqueCompositionMap,
		int ch, int mode);
	void circShift(np::Uint8Array2& image, int shift);
	void setAxialOffset(np::Uint8Array2& image, int offset);

public slots:
	void getCapture(QByteArray &);
	
signals:
	void playingDone();
	void drawImage(uint8_t*, float*, float*);
	void makeCirc(void);
	void paintCircImage(uint8_t*);
	void paintEnFaceMap(uint8_t*);
    void paintLongiImage(uint8_t*);

// Variables ////////////////////////////////////////////
private:
    Configuration* m_pConfig;
	Configuration* m_pConfigTemp;
	QStreamTab* m_pStreamTab;
    QResultTab* m_pResultTab;

public:
	// Visualization buffers - only for streaming tab
	np::Uint8Array2 m_visImage;
	np::FloatArray2 m_visIntensity;
	np::FloatArray2 m_visMeanDelay;
	np::FloatArray2 m_visLifetime;

public: // for post processing
#ifndef NEXT_GEN_SYSTEM
	std::vector<np::Uint8Array2> m_vectorOctImage;
	np::Uint8Array2 m_octProjection;
#else
	std::vector<np::FloatArray2> m_vectorOctImage;
	np::FloatArray2 m_octProjection;
#endif
	std::vector<np::FloatArray2> m_intensityMap; // (256 x N) x 3
	std::vector<np::FloatArray2> m_meandelayMap; // (256 X N) x 4
	std::vector<np::FloatArray2> m_lifetimeMap; // (256 x N) x 3
	std::vector<np::FloatArray2> m_intensityProportionMap; // (256 x N) x 3
	std::vector<np::FloatArray2> m_intensityRatioMap; // (256 x N) x 3
	np::FloatArray2 m_featVectors;
	np::FloatArray2 m_inflammationMap;
	np::FloatArray2 m_plaqueCompositionMap;

	std::vector<np::FloatArray2> m_vectorPulseCrop;
	std::vector<np::FloatArray2> m_vectorPulseBgSub;
	std::vector<np::FloatArray2> m_vectorPulseMask;
	std::vector<np::FloatArray2> m_vectorPulseSpline;
	std::vector<np::FloatArray2> m_vectorPulseFilter;

private:
    // Image visualization buffers - cross-section
    ImageObject *m_pImgObjRectImage;
	ImageObject *m_pImgObjCircImage;
	ImageObject *m_pImgObjIntensity;
	ImageObject *m_pImgObjLifetime;

    // Image visualization buffers - en face
	ImageObject* m_pImgObjOctProjection;
	ImageObject *m_pImgObjIntensityMap;
	ImageObject *m_pImgObjLifetimeMap;
	ImageObject *m_pImgObjIntensityPropMap;
	ImageObject *m_pImgObjIntensityRatioMap;
	ImageObject *m_pImgObjInflammationMap;
	ImageObject *m_pImgObjPlaqueCompositionMap;

    // Image visualization buffers - longitudinal
    ImageObject *m_pImgObjLongiImage;

private:
	circularize* m_pCirc;
	medfilt* m_pMedfiltRect;	
	medfilt* m_pMedfiltIntensityMap;
	medfilt* m_pMedfiltLifetimeMap;
    medfilt* m_pMedfiltLongi;
	RandomForest* m_pForestInfl;
	RandomForest* m_pForestPlqCompo;
	
private:
	std::thread playing;
	bool _running;

private:
    // Layout
    QWidget *m_pViewWidget;

    // Image viewer widgets
    QImageView *m_pImageView_CircImage;
	QWidget *m_pWidget[4];
    QImageView *m_pImageView_EnFace;
    QImageView *m_pImageView_Longi;
	QImageView *m_pImageView_Ivus;
    QImageView *m_pImageView_ColorBar;
	
	// Navigation widgets
    QPushButton *m_pToggleButton_Play;
    QPushButton *m_pPushButton_Decrement;
    QPushButton *m_pPushButton_Increment;
	QSlider *m_pSlider_SelectFrame;

    // View option widgets
    QPushButton *m_pToggleButton_MeasureDistance;
	QPushButton *m_pToggleButton_MeasureArea;

    QComboBox *m_pComboBox_FLImParameters;
	QComboBox *m_pComboBox_RFPrediction;
    
	QButtonGroup *m_pButtonGroup_VisualizationMode;
	QRadioButton *m_pRadioButton_FLImParameters;
	QRadioButton *m_pRadioButton_RFPrediction;
};

#endif // QVIEWTAB_H
