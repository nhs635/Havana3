#ifndef PULLBACK_MOTOR_H
#define PULLBACK_MOTOR_H

#include "FaulhaberMotor.h"

#define GEAR_RATIO 339.6715627996165 //334.224


class PullbackMotor : public FaulhaberMotor
{
public:
	explicit PullbackMotor();
	virtual ~PullbackMotor();
};

#endif
