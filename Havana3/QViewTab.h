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
///#include <Common/ann.h>

#include <iostream>
#include <vector>


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
    inline QImageView* getEnFaceImageView() const { return m_pImageView_EnFace; }
    inline QImageView* getLongiImageView() const { return m_pImageView_Longi; }
	inline circularize* getCirc() const { return m_pCirc; }
	inline medfilt* getMedfiltRect() const { return m_pMedfiltRect; }
	inline medfilt* getMedfiltIntensityMap() const { return m_pMedfiltIntensityMap; }
	inline medfilt* getMedfiltLifetimeMap() const { return m_pMedfiltLifetimeMap; }
	inline medfilt* getMedfiltLongi() const { return m_pMedfiltLongi; }
    inline QSlider* getSliderSelectFrame() const { return m_pSlider_SelectFrame; }
	inline void setEmissionChannel(int ch) { m_pComboBox_ViewMode->setCurrentIndex(ch - 1); }
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
	void changeEmissionChannel(int);

public:
	void scaleFLImEnFaceMap(ImageObject* pImgObjIntensityMap, ImageObject* pImgObjLifetimeMap, int ch);
	void circShift(np::Uint8Array2& image, int shift);

public slots:
	void getCapture(QByteArray &);
	
signals:
	void drawImage(uint8_t*, float*, float*);
	void makeCirc(void);
	void paintCircImage(uint8_t*);
	void paintEnFaceMap(uint8_t*);
    void paintLongiImage(uint8_t*);

// Variables ////////////////////////////////////////////
private:
    Configuration* m_pConfig;
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

    // Image visualization buffers - longitudinal
    ImageObject *m_pImgObjLongiImage;

private:
	circularize* m_pCirc;
	medfilt* m_pMedfiltRect;	
	medfilt* m_pMedfiltIntensityMap;
	medfilt* m_pMedfiltLifetimeMap;
    medfilt* m_pMedfiltLongi;
	//ann* m_pAnn;
	
private:
    // Layout
    QWidget *m_pViewWidget;

    // Image viewer widgets
    QImageView *m_pImageView_CircImage;
    QImageView *m_pImageView_EnFace;
    QImageView *m_pImageView_Longi;
    QImageView *m_pImageView_ColorBar;
	
	// Navigation widgets
    QPushButton *m_pToggleButton_Play;
    QPushButton *m_pPushButton_Decrement;
    QPushButton *m_pPushButton_Increment;
	QSlider *m_pSlider_SelectFrame;

    // View option widgets
    QPushButton *m_pToggleButton_MeasureDistance;

    QComboBox *m_pComboBox_ViewMode;
    QLabel *m_pLabel_ViewMode;
};

#endif // QVIEWTAB_H
