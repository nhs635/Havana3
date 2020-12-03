#ifndef PULLBACK_MOTOR_H
#define PULLBACK_MOTOR_H

#include "FaulhaberMotor.h"

#define GEAR_RATIO 334.224


class PullbackMotor : public FaulhaberMotor
{
public:
	PullbackMotor();
	~PullbackMotor();
};

#endif
