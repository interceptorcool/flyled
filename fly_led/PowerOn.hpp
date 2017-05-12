/*
 * PowerOn.hpp
 *
 *  Created on: 15 мар. 2017 г.
 *      Author: icool
 */

#ifndef FLY_LED_DATA_POWERON_HPP_
#define FLY_LED_DATA_POWERON_HPP_


#include <vector>
#include <functional>
#include <algorithm>

#include "Arduino.h"
#include "model_strip.hpp"

class PowerOn
{
public:
	enum class status {
		Error = -1,
		Stop = 0,
		WaitForTransmitt = 1,
		Send = 2
	};



	static const char*	POWERON;
	static const char*	COUNTS;

	PowerOn(TFlyConf& fc):
			_conf(fc),
			_delay(0), _count(0), _stat(status::WaitForTransmitt), _currentAction(0), _save_millis(0), _size(0)
	{
	}

	void setup();

	const char* getIsolationAction(const char* ca);
	status		getIsolationStat() const { return _stat; }


private:
	TFlyConf&	_conf;
	uint		_delay;
	uint		_save_millis;
	int			_count;
	int			_size;
	size_t		_currentAction;
	status		_stat;
};

#endif /* FLY_LED_DATA_POWERON_HPP_ */
