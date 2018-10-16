
#include "ZaberStage.h"
#include <Havana3/Configuration.h>


ZaberStage::ZaberStage() :
	device_name(ZABER_PORT),
	max_micro_resolution(ZABER_MAX_MICRO_RESOLUTION),
	micro_resolution(ZABER_MICRO_RESOLUTION),
	conversion_factor(ZABER_CONVERSION_FACTOR),
	port(INVALID_HANDLE_VALUE),
	microstep_size(ZABER_MICRO_STEPSIZE),
	_running(false)
{
	memset(home, 0, sizeof(home));
	memset(stop, 0, sizeof(home));
	memset(target_speed, 0, sizeof(home));
	memset(move_absolute, 0, sizeof(home));
}


ZaberStage::~ZaberStage()
{
	if (t_wait.joinable())
	{
		_running = false;
		t_wait.join();
	}

	zb_disconnect(port);
}


void ZaberStage::SetCommandData(int cmd_data, uint8_t* msg)
{
	unsigned int power24 = 1 << (8 * 3);
	unsigned int power16 = 1 << (8 * 2);
	unsigned int power08 = 1 << (8 * 1);

	int c3, c4, c5, c6;

	c6 = cmd_data / power24;
	cmd_data = cmd_data - power24 * c6;
	c5 = cmd_data / power16;
	cmd_data = cmd_data - power16 * c5;
	c4 = cmd_data / power08;
	cmd_data = cmd_data - power08 * c4;
	c3 = cmd_data;

	msg[2] = c3;
	msg[3] = c4;
	msg[4] = c5;
	msg[5] = c6;
}


void ZaberStage::GetReplyData(uint8_t* msg, int &reply_val)
{
	unsigned int power24 = 1 << (8 * 3);
	unsigned int power16 = 1 << (8 * 2);
	unsigned int power08 = 1 << (8 * 1);

	int c3, c4, c5, c6;
	
	c3 = msg[2];
	c4 = msg[3];
	c5 = msg[4];
	c6 = msg[5];

	reply_val = power24 * c6 + power16 * c5 + power08 * c4 + c3;
	if (c6 > 127)
		reply_val = 0; // (unsigned long long)reply_val - (unsigned long long)(256 * 256 * 256 * 256);
}


bool ZaberStage::ConnectStage()
{
	if (port == INVALID_HANDLE_VALUE)
	{
		SendStatusMessage("ZABER: Connecting to Zaber stage...", false);

		if (zb_connect(&port, device_name) != Z_SUCCESS)
		{
			char msg[256];
			sprintf(msg, "ZABER: Could not connect to device %s.", device_name);
			SendStatusMessage(msg, true);
			return false;
		}
		home[0] = 1; stop[0] = 1; target_speed[0] = 1; move_absolute[0] = 1;
		home[1] = 1; stop[1] = 23; target_speed[1] = 42; move_absolute[1] = 20;

		SendStatusMessage("ZABER: Success to connect to Zaber stage...", false);
	}

	// Message receiving thread
	if (!(t_wait.joinable()))
	{
		t_wait = std::thread([&]() {

			DWORD nread;
			int i = 0; char c;

			_running = true;
			while (_running)
			{
				if (!_running)
					break;

				// Read message byte by byte
				ReadFile(port, &c, 1, &nread, NULL);
				
				// Writing message
				if ((nread != 0))
					received_msg[i++] = c;

				// Show message
				if (i == 6)
				{
					char msg[256];
					sprintf(msg, "ZABER: Received [%d %d %d %d %d %d].", received_msg[0], received_msg[1]
																	   , received_msg[2], received_msg[3]
																	   , received_msg[4], received_msg[5]);
					SendStatusMessage(msg, false);
					i = 0;

					if (received_msg[1] == 20)
					{
						SendStatusMessage("pullback finished.", false);
						DidMovedAbsolute();
					}
				}
			}
		});
	}

	// Set micro resolution
	uint8_t resol[6] = { 0, };
	resol[0] = 1; resol[1] = 37;

	SetCommandData(micro_resolution, resol);
	zb_send(port, resol);
	
	char msg[256];
	sprintf(msg, "ZABER: Set micro resolution step %d!!.", micro_resolution);
	SendStatusMessage(msg, false);

	return true;
}


void ZaberStage::DisconnectStage()
{
	if (port != INVALID_HANDLE_VALUE)
	{
		zb_disconnect(port);
		port = INVALID_HANDLE_VALUE;
		SendStatusMessage("ZABER: Success to disconnect to Zaber stage...", false);
	}
}


void ZaberStage::StopWaitThread()
{
	if (t_wait.joinable())
	{
		_running = false;
		t_wait.join();
	}
}


void ZaberStage::Home()
{
	zb_send(port, home);
	SendStatusMessage("ZABER: Go home!!", false);
}


void ZaberStage::Stop()
{
	zb_send(port, stop);
	SendStatusMessage("ZABER: Halted the operation intentionally.", false);
}


void ZaberStage::MoveAbsoulte(double position)
{	
	int cmd_abs_dist = (int)round(1000.0 * position / 
		(microstep_size * (double)max_micro_resolution / (double)micro_resolution) );
	SetCommandData(cmd_abs_dist, move_absolute);
	zb_send(port, move_absolute);

	char msg[256];
	sprintf(msg, "ZABER: Move absolute %d (%.1f mm)", cmd_abs_dist, position);
	SendStatusMessage(msg, false);
}


void ZaberStage::SetTargetSpeed(double speed)
{
	int cmd_speed = (int)round(1000.0 * speed * conversion_factor / 
		(microstep_size * (double)max_micro_resolution / (double)micro_resolution) );
	SetCommandData(cmd_speed, target_speed);
	zb_send(port, target_speed);

	char msg[256];
	sprintf(msg, "ZABER: Set target speed %d (%.1f mm/sec)\n", cmd_speed, speed);
	SendStatusMessage(msg, false);
}
