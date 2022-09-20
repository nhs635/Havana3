#ifndef DATAACQUISITION_H
#define DATAACQUISITION_H

#include <QObject>

#include <Common/array.h>
#include <Common/callback.h>


class Configuration;

class AxsunCapture;
class SignatecDAQ;
class AlazarDAQ;
class FLImProcess;
class OCTProcess;

class DataAcquisition : public QObject
{
	Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit DataAcquisition(Configuration*);
    virtual ~DataAcquisition();

// Methods //////////////////////////////////////////////
public:
	inline AxsunCapture* getAxsunCapture() const { return m_pAxsunCapture; }
	inline SignatecDAQ* getDigitizer() const { return m_pDaq; }
	inline AlazarDAQ* getOctDigitizer() const { return m_pDaqOct; }
	inline AlazarDAQ* getFlimDigitizer() const { return m_pDaqFlim; }
    inline FLImProcess* getFLIm() const { return m_pFLIm; }
	inline OCTProcess* getOCT() const { return m_pOCT; }
	inline bool getAcquisitionState() { return m_bAcquisitionState; }
	inline bool getPauseState() { return m_bIsPaused; }

public:
    bool InitializeAcquistion();
    bool StartAcquisition();
    void StopAcquisition();

public: 
    void GetBootTimeBufCfg(int idx, int& buffer_size);
    void SetBootTimeBufCfg(int idx, int buffer_size);
    void SetDcOffset(int offset);
	
public:
    void ConnectAcquiredFlimData(const std::function<void(int, const np::Array<uint16_t, 2>&)> &slot);
	void ConnectAcquiredFlimData1(const std::function<void(int, const void*)> &slot);
    void ConnectStopFlimData(const std::function<void(void)> &slot);
    void ConnectFlimSendStatusMessage(const std::function<void(const char*, bool)> &slot);

	void ConnectAcquiredOctData(const std::function<void(uint32_t, const np::Uint8Array2&)> &slot);
	void ConnectAcquiredOctBG(const std::function<void(uint32_t, const np::Uint8Array2&)> &slot);
	void ConnectAcquiredOctData1(const std::function<void(int, const void*)> &slot);
	void ConnectStopOctData(const std::function<void(void)> &slot);
	void ConnectOctSendStatusMessage(const std::function<void(const char*, bool)> &slot);
	
// Variables ////////////////////////////////////////////
private: 
    // Configuration object
    Configuration* m_pConfig;

    // Acquisition state flag
    bool m_bAcquisitionState;
	bool m_bIsPaused;

    // Object related to data acquisition
	AxsunCapture* m_pAxsunCapture;
    SignatecDAQ* m_pDaq;
	AlazarDAQ* m_pDaqOct;
	AlazarDAQ* m_pDaqFlim;
    FLImProcess* m_pFLIm;    
	OCTProcess* m_pOCT;
};

#endif // DATAACQUISITION_H
