
#include "PullbackMotor.h"
#include <Havana3/Configuration.h>


PullbackMotor::PullbackMotor() 
{
	setPortName(PULLBACK_MOTOR_PORT, 2);
}


PullbackMotor::~PullbackMotor()
{
}