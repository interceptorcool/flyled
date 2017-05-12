/*
 * FlyBattaryMonitor.cpp
 *
 *  Created on: 15 мар. 2017 г.
 *      Author: icool
 */

#include "FlyBattaryMonitor.hpp"
#include "Arduino.h"



const float FlyBattaryMonitor::MaxBattaryVoltage = 17.0;

const FlyBattaryMonitor::st_battaryLimits FlyBattaryMonitor::battyLimits[] = {
		{ 3.3, 3.6, 4.2  },	// LiPo
		{ 2.8, 3.2, 3.65 },	// LiFe
		{ 1.1, 1.2, 1.4 },	// NiMH
		{ 1.0, 1.1, 1.35 }	// NiCd
};


FlyBattaryMonitor::FlyBattaryMonitor(TFlyConf& fc, FlyColorLeds& fl):
		_leds(fl),
		_conf(fc),
		_saveAnalorRead(0),
		_readDelayms(0),
		_current_alarm(_current_alarm_t::None),
		_time_alarm_ms(0)
{
	_st_analogReads.delay = _st_analogReads.count = _st_analogReads.value = _st_analogReads.ready = 0;
	_st_analogReads.result =  0.0;
	_i_battaryCellCount = 0;
	_i_batType = 0;
	_Action = nullptr;
}

int FlyBattaryMonitor::_MeasStep()
{
	if (_st_analogReads.count < _readCount)
	{
		_st_analogReads.count++;
		_st_analogReads.value += analogRead(A0);
	} else {
		_st_analogReads.count = 0;
		_st_analogReads.result = float(_st_analogReads.value) / float(_readCount) / 1023.0 * 13.0;
		_st_analogReads.value = 0;
		_st_analogReads.ready = true;
	}
	return _st_analogReads.count;
}

void FlyBattaryMonitor::setup()
{
	if (!_conf.ready() || !_leds.ready()) return;

	// определяем кол-во подключенных банок
	_st_analogReads.count = 0;
	while(_MeasStep())
		yield();
	_st_analogReads.ready = false;

	_i_batType = _conf.getRoot()[BATTARY][BATTARYTYPE];
	if (_i_batType < 0 || _i_batType > sizeof(battyLimits)/sizeof(st_battaryLimits)) _i_batType = 0;

	uint curMAXCell = MaxBattaryVoltage / battyLimits[_i_batType].MaxCellVoltage;

	for (_i_battaryCellCount = 1; _i_battaryCellCount < curMAXCell; ++_i_battaryCellCount)
	{
		if (getBattaryVoltage() < battyLimits[_i_batType].MaxCellVoltage * _i_battaryCellCount + 0.5)
			break;
	}
	_readDelayms = _conf.getRoot()[BATTARY][DELAYNAME];
	if (!_readDelayms) _readDelayms = 500;
	_time_alarm_ms = _readDelayms + 2;


	DEBUG_FLY_BAT("Battary type %d, cell %d curVoltage:", _i_batType, _i_battaryCellCount);
	#ifdef DEBUG_FLY_BAT
		Serial.println(_st_analogReads.result);
		Serial.println();
	#endif

}

void FlyBattaryMonitor::Update()
{
	if (!_conf.ready() || !_leds.ready()) return;
	// чтение баареи
	if (_saveAnalorRead  < millis())
	{
		while(_MeasStep())
			yield();

		_saveAnalorRead = millis() + _readAnalogMS;


		_current_alarm_t t_alarm = _current_alarm_t::None;

		if (getBattaryVoltage() < battyLimits[_i_batType].CriticalCellVoltage * _i_battaryCellCount)
		{
			// set led0 color critical
			t_alarm = _current_alarm_t::Critical;

		} else if (getBattaryVoltage() < battyLimits[_i_batType].WarningCellVoltage * _i_battaryCellCount)
			{
				// set led0 color warning
				t_alarm = _current_alarm_t::Warning;

			} else {
				//  пока все нормально
				if (_Action != nullptr || _time_alarm_ms > _readDelayms)
				{
					//  пока все нормально
					_time_alarm_ms = _readDelayms;
					const char* crtCol = _conf.getRoot()[BATTARY][LED0][2];
					_leds.SetLED0PixelColor(_leds.getColor(crtCol, "#00FF00"));
					_Action = nullptr;
					_current_alarm = _current_alarm_t::None;
				}
				return;
			}

		if (_current_alarm != t_alarm)
		{
			_time_alarm_ms = _readDelayms;
			_current_alarm = t_alarm;
		} else {

			if (_time_alarm_ms)
			{
				//_time_alarm_ms = _readDelayms;

				//DEBUG_FLY_BAT("BS: "); Serial.println(getBattaryVoltage());

				JsonObject& battary = _conf.getRoot()[BATTARY];

				switch(_current_alarm)
				{
				case _current_alarm_t::Critical:
				{
					// set led0 color critical
					const char* crtCol = battary[LED0][0];
					_leds.SetLED0PixelColor(_leds.getColor(crtCol, "#FF0000"));
					_Action = battary[CRITICAL];
					DEBUG_FLY_BAT("BS: %s ", CRITICAL);
					#ifdef DEBUG_FLY_BAT
						Serial.println(_st_analogReads.result);
						Serial.println();
					#endif
					break;
				} // case _current_alarm_t::Critical:
				case _current_alarm_t::Warning:
				{
					// set led0 color warning
					const char* crtCol = battary[LED0][1];
					_leds.SetLED0PixelColor(_leds.getColor(crtCol, "#0000FF"));
					_Action = battary[WARNING];
					DEBUG_FLY_BAT("BS: %s ", WARNING);
					#ifdef DEBUG_FLY_BAT
						Serial.println(_st_analogReads.result);
						Serial.println();
					#endif
					break;
				} // case _current_alarm_t::Warning:
				default:
					;
				}
				if (_Action != nullptr && strlen(_Action) == 0) _Action = nullptr;
			} else {
				_time_alarm_ms--;
			}
		}
	}
}
