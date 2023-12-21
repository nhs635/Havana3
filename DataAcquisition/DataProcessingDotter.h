#ifndef DATAPROCESSINGDOTTER_H
#define DATAPROCESSINGDOTTER_H

#include <QObject>
#include <QFile>

#include <Havana3/Configuration.h>

#include <Common/array.h>
#include <Common/callback.h>
#include <Common/SyncObject.h>


class Configuration;
class QResultTab;

class FLImProcess;

class DataProcessingDotter : public QObject
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit DataProcessingDotter(QWidget *parent = nullptr);
    virtual ~DataProcessingDotter();

// Methods //////////////////////////////////////////////
public:
	inline Configuration* getConfigTemp() const { return m_pConfigTemp; }
    inline QResultTab* getResultTab() const { return m_pResultTab; }
    inline FLImProcess* getFLImProcess() const { return m_pFLIm; }
	inline QString getResFolder() const { return m_resFolder; }

public:
    void startProcessing(QString, int frame = -1);
	
private:
	void getFlimInfo(QString, Configuration*);
	void getOctInfo(QString, Configuration*);

private:
	void loadingFlimRawData(QFile*, Configuration*);
	void loadingOctRawData(QFile*, Configuration*);
	void flimProcessing(FLImProcess*, Configuration*);
	
public:
	void calculateFlimParameters();

private:
#ifndef NEXT_GEN_SYSTEM
	void getOctProjection(std::vector<np::Uint8Array2>& vecImg, np::Uint8Array2& octProj, int offset = 0);
#else
	void getOctProjection(std::vector<np::FloatArray2>& vecImg, np::FloatArray2& octProj, int offset = 0);
#endif

signals:
	void processedSingleFrame(int);
	void abortedProcessing();
	void finishedProcessing(bool);

// Variables ////////////////////////////////////////////
private:
	Configuration* m_pConfig;
	Configuration* m_pConfigTemp;
    QResultTab* m_pResultTab;
	FLImProcess* m_pFLIm;	
	
private:
    // for threading operation	
    SyncObject<uint16_t> m_syncFlimProcessing;

private:
	QString m_iniName;
	QString m_resFolder;

private:
	callback2<const char*, bool> SendStatusMessage;
};

#endif // DATAPROCESSING_H
