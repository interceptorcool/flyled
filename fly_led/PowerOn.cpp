/*
 * PowerOn.cpp
 *
 *  Created on: 15 мар. 2017 г.
 *      Author: icool
 */

#include "PowerOn.hpp"
#include "led_io.hpp"

const char*	PowerOn::POWERON = "POWERON";
const char*	PowerOn::COUNTS = "COUNTS";


void PowerOn::setup()
{
	if (!_conf.ready()) return;

	JsonObject& p = _conf.getRoot()[POWERON];
	_delay = p[DELAYNAME];
	_count = p[COUNTS];

	// if (!_delay) - то работаем по кнопке

	JsonArray& a = p[StrActionStep];
	if (a == JsonArray::invalid())
	{
		_stat = status::Error;
		DEBUG_FLY("PowerON: error in %s", StrActionStep);
	} else {
		_currentAction = 0;
		_stat = status::WaitForTransmitt;
		_size = a.size();
	}
	//DEBUG_FLY("PO_: c: %d d: %d s: %d z: %d\n", _count, _delay, _stat, _size);
}


const char* PowerOn::getIsolationAction(const char* ca)
{
	if (ca != nullptr)
	{
		return ca;
	}
	if (!_conf.ready() || _stat < status::WaitForTransmitt) return nullptr;

	_stat = status::WaitForTransmitt;
	const char *ret = nullptr;

	if (_save_millis < millis() || !_delay)
	{
		if (_delay) _save_millis = millis() + _delay;
		else
		{
			while (digitalRead(FLYLED_BUTTON_PIN) == HIGH)
				yield();
			while (digitalRead(FLYLED_BUTTON_PIN) == LOW)
				yield();
		}
		//DEBUG_FLY("PO2: count: %d\n", _count);
		if (_count == 0)
		{
			_stat = status::Stop;
			return nullptr;
		}
		_stat = status::Send;

		JsonArray& a = _conf.getRoot()[POWERON][StrActionStep];
		ret = a[_currentAction];
		_currentAction++;
		if (_currentAction >= _size)
		{
			_currentAction = 0;
			if (_count && _count > 0)
				_count--;
		}
		//DEBUG_FLY("PO3: %s, %d\n", ret, _currentAction);
	}

	return ret;
}
