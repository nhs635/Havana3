#ifndef DATAACQUISITION_H
#define DATAACQUISITION_H

#include <QObject>

#include <Common/array.h>
#include <Common/callback.h>


class Configuration;

class SignatecDAQ;
class FLImProcess;
class AxsunCapture;

class DataAcquisition : public QObject
{
	Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit DataAcquisition(Configuration*);
    virtual ~DataAcquisition();

// Methods //////////////////////////////////////////////
public: 
	inline SignatecDAQ* getDigitizer() const { return m_pDaq; }
    inline FLImProcess* getFLIm() const { return m_pFLIm; }
	inline AxsunCapture* getAxsunCapture() const { return m_pAxsunCapture; }
	inline bool getAcquisitionState() { return m_bAcquisitionState; }
	inline bool getPauseState() { return m_bIsPaused; }

public:
    bool InitializeAcquistion();
    bool StartAcquisition();
    void StopAcquisition(bool suc_stop);

public: 
    void GetBootTimeBufCfg(int idx, int& buffer_size);
    void SetBootTimeBufCfg(int idx, int buffer_size);
    void SetDcOffset(int offset);
	
public:
    void ConnectDaqAcquiredFlimData(const std::function<void(int, const np::Array<uint16_t, 2>&)> &slot);
    void ConnectDaqStopFlimData(const std::function<void(void)> &slot);
    void ConnectDaqSendStatusMessage(const std::function<void(const char*, bool)> &slot);

	void ConnectAxsunAcquiredOctData(const std::function<void(uint32_t, const np::Uint8Array2&)> &slot);
	void ConnectAxsunStopOctData(const std::function<void(void)> &slot);
	void ConnectAxsunSendStatusMessage(const std::function<void(const char*, bool)> &slot);
	
// Variables ////////////////////////////////////////////
private: 
    // Configuration object
    Configuration* m_pConfig;

    // Acquisition state flag
    bool m_bAcquisitionState;
	bool m_bIsPaused;

    // Object related to data acquisition
    SignatecDAQ* m_pDaq;
    FLImProcess* m_pFLIm;
    AxsunCapture* m_pAxsunCapture;
};

#endif // DATAACQUISITION_H
