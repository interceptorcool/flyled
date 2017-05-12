/*
 * FlyBattaryMonitor.hpp
 *
 *  Created on: 15 мар. 2017 г.
 *      Author: icool
 */

#ifndef FLY_LED_FLYBATTARYMONITOR_HPP_
#define FLY_LED_FLYBATTARYMONITOR_HPP_

#include <vector>
#include <functional>
#include <algorithm>

#include "Arduino.h"
#include "model_strip.hpp"
#include "Filters.h"

//#define DEBUG_FLY_BAT
#define DEBUG_FLY_BAT_PORT Serial

#ifdef DEBUG_FLY_BAT
#ifdef DEBUG_FLY_BAT_PORT
#define DEBUG_FLY_BAT(...) DEBUG_FLY_BAT_PORT.printf("DBG_BTM: "); DEBUG_FLY_BAT_PORT.printf( __VA_ARGS__ )
#endif
#endif

#ifndef DEBUG_FLY_BAT
#define DEBUG_FLY_BAT(...)
#endif


// TODO : Vin = Vbat / 13
//		: Vbat = 13 * (ADC / 1023)

class FlyBattaryMonitor
{
public:
	struct st_battaryLimits {
		float CriticalCellVoltage;
		float WarningCellVoltage;
		float MaxCellVoltage;
	};

	static const st_battaryLimits	battyLimits[];

	static const float 	MaxBattaryVoltage;

	FlyBattaryMonitor(TFlyConf&, FlyColorLeds&);

	void setup();

	bool	isReady()  { bool r = _st_analogReads.ready; _st_analogReads.ready = false; return r; }

	inline float getBattaryVoltage() const { return _st_analogReads.result; }

	void Update();

	inline const char* getAction() const { return _Action; }

private:
	int	_i_battaryCellCount;
	int	_i_batType;

	FlyColorLeds&	_leds;
	TFlyConf&		_conf;

	uint32_t		_readDelayms;
	static const int			_readCount = 4;

	static const int			_readAnalogMS = 100;
	uint32_t					_saveAnalorRead;

	struct st_analog_read_{
		float result;
		int value;
		int delay;
		int count;
		bool ready;
	};
	st_analog_read_				_st_analogReads;

	enum class _current_alarm_t
	{
		None	= 0,
		Critical,
		Warning
	};

	_current_alarm_t		_current_alarm;
	uint32_t				_time_alarm_ms;

	const char*				_Action;

	int		_MeasStep(void);
};

#endif /* FLY_LED_FLYBATTARYMONITOR_HPP_ */
