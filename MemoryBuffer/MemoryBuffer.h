#ifndef MEMORYBUFFER_H
#define MEMORYBUFFER_H

#include <QObject>
#include <QString>

#include <Havana3/Configuration.h>
#include <Havana3/QPatientSummaryTab.h>

#include <iostream>
#include <queue>

#include <Common/SyncObject.h>
#include <Common/callback.h>

class MainWindow;
class Configuration;
class HvnSqlDataBase;
class QStreamTab;

class MemoryBuffer : public QObject
{
	Q_OBJECT

public:
    explicit MemoryBuffer(QObject *parent = nullptr);
    virtual ~MemoryBuffer();

public:
	// Memory allocation function (buffer for writing)
    void allocateWritingBuffer();
	void disallocateWritingBuffer();

    // Data recording (transfer streaming data to writing buffer)
    bool startRecording();
    void stopRecording();

    // Data saving (save wrote data to hard disk)
    void startSaving(RecordInfo &);

	/// Circulation
	///void circulation(int nFramesToCirc);

	/// Buffer operation
	///uint16_t* pop_front();
	///void push_back(uint16_t* buffer);

private: // writing threading operation
	void write();

signals:
	void wroteSingleFrame(int);
	void finishedBufferAllocation();
	void errorWhileWriting();
	void finishedWritingThread();

private:
	Configuration* m_pConfig;
	HvnSqlDataBase* m_pHvnSqlDataBase;
	QStreamTab* m_pStreamTab;

public:
	bool m_bIsAllocatedWritingBuffer;
	bool m_bIsRecording;
	bool m_bIsSaved;
	int m_nRecordedFrames;

public:
	callback<void> DidPullback;
	callback2<const char*, bool> SendStatusMessage;

public:
	SyncObject<uint16_t> m_syncFlimBuffering;
#ifndef NEXT_GEN_SYSTEM
	SyncObject<uint8_t> m_syncOctBuffering;
#else
	SyncObject<float> m_syncOctBuffering;
#endif

private:
    std::queue<uint16_t*> m_queueWritingFlimBuffer; // writing buffer
#ifndef NEXT_GEN_SYSTEM
	std::queue<uint8_t*> m_queueWritingOctBuffer; // writing buffer
#else
	std::queue<float*> m_queueWritingOctBuffer; // writing buffer
#endif
	QString m_fileName;
};

#endif // MEMORYBUFFER_H
