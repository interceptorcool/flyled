/*
 * led_io.hpp
 *
 *  Created on: 15 мар. 2017 г.
 *      Author: icool
 */

#ifndef FLY_LED_LED_IO_HPP_
#define FLY_LED_LED_IO_HPP_

#include <vector>
#include <functional>
#include <algorithm>

#include "model_strip.hpp"
#include "PPMInput.hpp"

#ifdef eclipse
#include </home/icool/Arduino/libraries/ArduinoJson-master/ArduinoJson.h>
#else
#include <ArduinoJson.h>
#endif

/*
 * ========================================================================
 * PPM Inputs
 * Receiver Inputs
 */

extern PPMInput<FLYLED_RCV_CH1_PIN> ppm_input_ch1;
extern PPMInput<FLYLED_RCV_CH2_PIN> ppm_input_ch2;

class FlyReceiverInputs
{
private:
	//FlyReceiverInputs(const FlyReceiverInputs&);
	//void operator = (const FlyReceiverInputs&);
public:

	FlyReceiverInputs(TFlyConf& fc):
		_flyConfig(fc),
		_ResponseSensitivity_ms(1000), _saveResponseSensitivity_ms(0),
		_AllActionMissed(0), _AllActionMissed_active(0), _saveMissedSensitivity_ms(0),
		_ch1_input_enable(0), _ch2_input_enable(0)
	{
	}

	void setup(void)
	{
		if (!_flyConfig.ready()) return;
		JsonArray& RSConfig = _flyConfig.getRoot()[InputsConfig];
		uint cppm = RSConfig[0];
		if (cppm > MIN_PULSE_WIDTH && cppm < MAX_PULSE_WIDTH)
		{
			ppm_input_ch1.setCentrePPM(cppm);
			ppm_input_ch2.setCentrePPM(cppm);
		}
		uint rs = RSConfig[1];
		if (rs > 30) _ResponseSensitivity_ms = rs;

		_Actions.clear();
		_SignalLastAction.clear();

		DEBUG_FLY("cppm: %d, rs: %d\n", cppm, _ResponseSensitivity_ms);

		_ch1_input_enable = RSConfig[2];
		_ch2_input_enable = RSConfig[3];
	}

	void Update();


	const char* getAction(const char* ca)
	{
		if (ca != nullptr) return ca;
		if (_Actions.empty()) return nullptr;
		const char* ret = _Actions[0];
		_Actions.erase(_Actions.begin());
		//DEBUG_FLY("_Action.size( %d )\n", _Actions.size());
		return ret;
	}

private:
	TFlyConf&	_flyConfig;
	uint		_ResponseSensitivity_ms;
	uint		_saveResponseSensitivity_ms;
	uint		_saveMissedSensitivity_ms;
	int	 		_AllActionMissed, _AllActionMissed_active;
	uint8_t		_ch1_input_enable;
	uint8_t		_ch2_input_enable;

	typedef std::pair<const char*, int>	TKeyStep;
	std::vector<TKeyStep>		_ActionStep;
	std::vector<const char*>	_Actions;
	std::vector<std::pair<const char*, int>>		_SignalLastAction;

	// Добавление всех действий из списка
	void _AddAction(JsonArray& actions);

	// добавление одного действия из списка "по кругу"
	// key - название реакции на внешний канал
	// actions - набор действий
	void _AddActionStep(const char* key, JsonArray& actions);

};

/*
 * ========================================================================
 * OutPuts Signals
 */
class FlyOutputs
{
//private:
//	FlyOutputs(const FlyOutputs&);
//	void operator = (const FlyOutputs&);
public:
	static const uint8_t u8OutPins[];
	static const uint8_t u8CountOutPin = 4;


	FlyOutputs(TFlyConf& fc):
		_flyConfig(fc),
		_currentAction(nullptr)
	{
	}

	void setup(void)
	{
		if (!_flyConfig.ready()) return;
		JsonArray& CFG = _flyConfig.getRoot()[strOutputsCFG];
		JsonArray& INIT = _flyConfig.getRoot()[INITOUT];

		_ServoPorts.clear();
		_DigidsPorts.clear();
		if (CFG == JsonArray::invalid())
		{
			// все каналы = 0 - ничего не выводят
			return;
		}
		for (uint i = 0; i < u8CountOutPin; i++)
		{
			uint8_t pin_cfg = CFG[i];
			if (pin_cfg == 1)
			{
				// настройка вывода как Digital
				_DigidsPorts.push_back(i);
				pinMode(u8OutPins[i], OUTPUT);
				digitalWrite(u8OutPins[i], INIT[i]);
			}
			if (pin_cfg == 2)
			{
				// настройка вывода как PPM
				Servo srv;
				srv.attach(u8OutPins[i]);
				srv.writeMicroseconds(DEFAULT_PULSE_WIDTH);
				TServo add(i, srv);
				_ServoPorts.push_back(add);
			}
		}
		DEBUG_FLY("FlyOutputs d: %d, S: %d\n", _DigidsPorts.size(), _ServoPorts.size());
	}

	const char* StartAction(const char *);

private:
	TFlyConf&	_flyConfig;

	typedef std::pair<uint8_t, Servo>	TServo;
	std::vector<TServo>			_ServoPorts;
	std::vector<uint8_t>		_DigidsPorts;
	const char*					_currentAction;
};


#endif /* FLY_LED_LED_IO_HPP_ */
