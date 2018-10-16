#ifndef _QSERIAL_COMM_H_
#define _QSERIAL_COMM_H_

#include <QObject>
#include <QtSerialPort/QSerialPort>

#include <iostream>
#include <thread>

#include <Common/callback.h>


class QSerialComm : public QObject
{
	Q_OBJECT

public:
	QSerialComm(QObject *parent = 0) :
		QObject(parent), m_pSerialPort(nullptr), m_bIsConnected(false)
	{
		m_pSerialPort = new QSerialPort;
		connect(m_pSerialPort, SIGNAL(readyRead()), this, SLOT(readSerialPort()));
	}

	~QSerialComm()
	{
		closeSerialPort();
		if (m_pSerialPort) delete m_pSerialPort;
	}

public:
	bool openSerialPort(QString portName, 
		QSerialPort::BaudRate baudRate = QSerialPort::Baud9600, 
		QSerialPort::DataBits dataBits = QSerialPort::Data8,
		QSerialPort::Parity parity = QSerialPort::NoParity, 
		QSerialPort::StopBits stopBits = QSerialPort::OneStop, 
		QSerialPort::FlowControl flowControl = QSerialPort::NoFlowControl)
	{
		m_pSerialPort->setPortName(portName);
		m_pSerialPort->setBaudRate(baudRate);
		m_pSerialPort->setDataBits( dataBits);
		m_pSerialPort->setParity(parity);
		m_pSerialPort->setStopBits(stopBits);
		m_pSerialPort->setFlowControl(flowControl);

		if (!m_pSerialPort->open(QIODevice::ReadWrite))
		{
			closeSerialPort();
			return false;
		}

		m_bIsConnected = true;
		
		return true;
	}

	void closeSerialPort()
	{
		if (m_pSerialPort->isOpen())
			m_pSerialPort->close();
		m_bIsConnected = false;
	}

	bool writeSerialPort(char* data)
	{
		if (m_pSerialPort->isOpen())
		{
			qint64 nWrote = m_pSerialPort->write(data);			
			if (nWrote != 0)
				return true;
		}
		return false;
	}

	void waitUntilResponse()
	{
		m_pSerialPort->waitForReadyRead(100);
	}
	
public slots:
	void readSerialPort()
	{
		qint64 nRead;
		char buffer[50];

		nRead = m_pSerialPort->read(buffer, 50);
		DidReadBuffer(buffer, nRead);
	}


public:
	callback2<char*, qint64> DidReadBuffer;
	bool m_bIsConnected;

private:
	QSerialPort* m_pSerialPort;
};

#endif // _SERIAL_COMM_H_