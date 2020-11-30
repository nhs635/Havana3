#ifndef DATAPROCESSING_H
#define DATAPROCESSING_H

#include <QObject>
#include <QFile>

#include <Common/array.h>
#include <Common/SyncObject.h>


class Configuration;
class QResultTab;

class FLImProcess;

class DataProcessing : public QObject
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit DataProcessing(QWidget *parent = nullptr);
    virtual ~DataProcessing();

// Methods //////////////////////////////////////////////
public:
	inline Configuration* getConfigTemp() const { return m_pConfigTemp; }
    inline QResultTab* getResultTab() const { return m_pResultTab; }
    inline FLImProcess* getFLImProcess() const { return m_pFLIm; }

public:
    void startProcessing(QString);
	
private:
	void loadingRawData(QFile*, Configuration*);
	void deinterleaving(Configuration*);
	void flimProcessing(FLImProcess*, Configuration*);

private:
	void getOctProjection(std::vector<np::Uint8Array2>& vecImg, np::Uint8Array2& octProj, int offset = 0);

signals:
	void processedSingleFrame(int);

// Variables ////////////////////////////////////////////
private:
	Configuration* m_pConfigTemp;
    QResultTab* m_pResultTab;
	FLImProcess* m_pFLIm;

public:
	//QString m_path;
	bool m_bAbort;

private:
    // for threading operation
	SyncObject<uint8_t> m_syncDeinterleaving;
    SyncObject<uint16_t> m_syncFlimProcessing;
};

#endif // DATAPROCESSING_H
