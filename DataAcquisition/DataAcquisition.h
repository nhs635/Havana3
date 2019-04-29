#ifndef DATAACQUISITION_H
#define DATAACQUISITION_H

#include <QObject>

#include <Havana3/Configuration.h>

#include <Common/array.h>
#include <Common/callback.h>

class SignatecDAQ;
class FLImProcess;
class AxsunCapture;


class DataAcquisition : public QObject
{
	Q_OBJECT

public:
    explicit DataAcquisition(Configuration*);
    virtual ~DataAcquisition();

public:
	inline SignatecDAQ* getDigitizer() const { return m_pDaq; }
    inline FLImProcess* getFLIm() const { return m_pFLIm; }
	inline AxsunCapture* getAxsunCapture() const { return m_pAxsunCapture; }

public:
    bool InitializeAcquistion();
    bool StartAcquisition();
    void StopAcquisition();

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

private:
	Configuration* m_pConfig;

    SignatecDAQ* m_pDaq;
    FLImProcess* m_pFLIm;
    AxsunCapture* m_pAxsunCapture;
};

#endif // DATAACQUISITION_H
