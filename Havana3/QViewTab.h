#ifndef QVIEWTAB_H
#define QVIEWTAB_H

#include <QtWidgets>
#include <QtCore>

#include <Havana3/Viewer/QImageView.h>

#include <Common/array.h>
#include <Common/circularize.h>
#include <Common/medfilt.h>
#include <Common/ImageObject.h>
#include <Common/basic_functions.h>
#include <Common/ann.h>

#include <iostream>
#include <vector>


class Configuration;
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
    inline QSlider* getSliderSelectFrame() const { return m_pSlider_SelectFrame; }
	inline void setCurrentFrame(int frame) { m_pSlider_SelectFrame->setValue(frame); }
    inline int getCurrentFrame() { return m_pSlider_SelectFrame->value(); }
	inline int getCurrentAline() { return m_pImageView_CircImage->getRender()->m_pVLineInd[0]; }
	
private:
    void createViewTabWidgets(bool);

private:
    void setStreamingBuffersObjects();

public:
	void setBuffers(Configuration* pConfig);
	void setObjects(Configuration* pConfig);
    void setWidgets(Configuration* pConfig);

public slots:
	void visualizeEnFaceMap(bool scaling = true);
	void visualizeImage(uint8_t* oct_im, float* intensity, float* lifetime);
	void visualizeImage(int frame);
    void visualizeLongiImage(int aline);

private slots:
    void constructRgbImage(ImageObject*, ImageObject*, ImageObject*, ImageObject*);
	void play(bool);
	void measureDistance(bool);
	void changeEmissionChannel(int);

private:
	void circShift(np::Uint8Array2& image, int shift);
	
signals:
	void drawImage(uint8_t*, float*, float*);
	void makeRgb(ImageObject*, ImageObject*, ImageObject*, ImageObject*);
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
	std::vector<np::Uint8Array2> m_vectorOctImage;
	np::Uint8Array2 m_octProjection;
	std::vector<np::FloatArray2> m_intensityMap; // (256 x N) x 3
	std::vector<np::FloatArray2> m_lifetimeMap; // (256 x N) x 3
	std::vector<np::FloatArray2> m_vectorPulseCrop;
	std::vector<np::FloatArray2> m_vectorPulseMask;

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
	ImageObject *m_pImgObjIntensityWeightedLifetimeMap;

    // Image visualization buffers - longitudinal
    ImageObject *m_pImgObjLongiImage;
    ImageObject *m_pImgObjLongiIntensity;
    ImageObject *m_pImgObjLongiLifetime;
    ImageObject *m_pImgObjLongiIntensityWeightedLifetime;

private:
	circularize* m_pCirc;
	medfilt* m_pMedfiltRect;	
	medfilt* m_pMedfiltIntensityMap;
	medfilt* m_pMedfiltLifetimeMap;
    medfilt* m_pMedfiltLongi;
	ann* m_pAnn;
	
private:
    // Layout
    QWidget *m_pViewWidget;

    // Image viewer widgets
    QImageView *m_pImageView_CircImage;
    QImageView *m_pImageView_EnFace;
    QImageView *m_pImageView_Longi;
    QImageView *m_pImageView_ColorBar;

	QLabel *m_pLabel_EnFace;
	QLabel *m_pLabel_Longi;
	QLabel *m_pLabel_ColorBar;

	// Navigation widgets
    QPushButton *m_pToggleButton_Play;
    QPushButton *m_pPushButton_Decrement;
    QPushButton *m_pPushButton_Increment;

	QSlider *m_pSlider_SelectFrame;
	QLabel *m_pLabel_SelectFrame;

    // View option widgets
    QPushButton *m_pToggleButton_MeasureDistance;

    QComboBox *m_pComboBox_ViewMode;
    QLabel *m_pLabel_ViewMode;
};

#endif // QVIEWTAB_H
