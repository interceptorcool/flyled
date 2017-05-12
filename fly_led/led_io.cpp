/*
 * led_io.cpp
 *
 *  Created on: 15 мар. 2017 г.
 *      Author: icool
 */

#include "led_io.hpp"

const uint8_t FlyOutputs::u8OutPins[] = { 0, 15, 14, 13 };

/*
 * ========================================================================================
 */
const char* FlyOutputs::StartAction(const char *key_Action)
{
	if (key_Action == nullptr) return nullptr;
	if (!_flyConfig.ready()) return nullptr;
	if (_currentAction != NULL && !strcmp(_currentAction, key_Action)) return nullptr;
	JsonArray& action = _flyConfig.getRoot()[key_Action];

	if (action == JsonArray::invalid())
	{
		// если нет такого объекта то выходим
		return key_Action;
	}

	// это наше действие - Output
	_currentAction = key_Action;

	uint8_t	port = action[0];
	int		act	 = action[1];
	// применяем
	if (port > 4 || act > 151 || act < -151) return nullptr;

	port -= 1; // от осмысленного к программе

	//DEBUG_FLY("FlyOutputs::StartAction( %s )\n", key_Action);

	// цыфровые порты
	if (!_DigidsPorts.empty())
	{
		for (auto dp: _DigidsPorts)
		{
			if (dp == port)
			{
				uint8_t o = LOW;
				if (act) o = HIGH;
				digitalWrite(u8OutPins[dp], o);
				//DEBUG_FLY("digitalWrite(%d,%d)\n", u8OutPins[dp], o);
			}
		}
	}
	// серво
	if (!_ServoPorts.empty())
	{
		for (auto sp: _ServoPorts)
		{
			if (sp.first  == port)
			{
				uint16_t pct_to_ms = uint16_t(float(act) * 4.0 + DEFAULT_PULSE_WIDTH);
				if (MIN_PULSE_WIDTH < pct_to_ms && pct_to_ms < MAX_PULSE_WIDTH)
				{
					sp.second.writeMicroseconds(pct_to_ms);
					//DEBUG_FLY("Servo(%d)\n", pct_to_ms);
				}
			}
		}
	}
	return nullptr;
}


/*
 * ========================================================================================
 */
void FlyReceiverInputs::Update()
{
	if (!_flyConfig.ready()) return;

	// проверяем состояние внутри запамненных позиций
	ppm_input_ch1.readJitter();
	ppm_input_ch2.readJitter();
	if ((_ch1_input_enable && !ppm_input_ch1.IsChange()) && (_ch2_input_enable && !ppm_input_ch2.IsChange()) && _saveMissedSensitivity_ms > millis())
	{
		return;
	}
	_saveMissedSensitivity_ms = millis() + _ResponseSensitivity_ms;

	//digitalWrite(FLYLED_STATUS_LED_PIN, !digitalRead(FLYLED_STATUS_LED_PIN));

	int ch1_pct = ppm_input_ch1.iread_pct();
	int ch2_pct = ppm_input_ch2.iread_pct();

	if (_saveResponseSensitivity_ms < millis())
	{
		_saveResponseSensitivity_ms = millis() + _ResponseSensitivity_ms;

		// очищаем проверки

		JsonObject& RSSection = _flyConfig.getRoot()[ReceiverInputs];

		_AllActionMissed = 1;

		for (auto cur_sig: RSSection)
		{
			//DEBUG_FLY_PORT.println();
			//DEBUG_FLY("%s\n", cur_sig.key);

			if (cur_sig.value.asObject() == JsonObject::invalid()) continue;

			int wrkAction = 0;
			int last_wrkAction = 0;

			//DEBUG_FLY_PORT.println("=====================================");

			std::vector<std::pair<const char*, int>>::iterator it_SignalLastAction;
			if (!_SignalLastAction.empty())
			{
				auto it_SignalLastAction = std::find_if(_SignalLastAction.begin(), _SignalLastAction.end(),
						[cur_sig](const std::pair<const char*, int> sig) { return sig.first ==  cur_sig.key; } );
				if (it_SignalLastAction != _SignalLastAction.end())
				{
					last_wrkAction = it_SignalLastAction->second;
					_SignalLastAction.erase(it_SignalLastAction);
					//DEBUG_FLY("%s - %d\n", cur_sig.key, last_wrkAction);
				}
			}
			for (auto param: cur_sig.value.asObject())
			{
				// внутри сигнала
				if (_ch1_input_enable && strstr(param.key, RCH1) != NULL)
				{
					// "RCH1%": [-100, -80],
					bool ex = true;
					for (int i = 0; i < 2; ++i)
					{
						const char* v = param.value.asArray()[i];
						// если буква не существует, то масив не числовой
						if (v == NULL) ex = false;
					}

					if (ex)
					{
						int begin = param.value.asArray()[0];
						int end = param.value.asArray()[1];

						// проверяем на вхождение в диапазон

						if (_min(begin, end) < ch1_pct && ch1_pct < _max(begin, end))
							wrkAction |= _BV(1);
						else
							last_wrkAction &= ~_BV(1);
						//DEBUG_FLY("RCH1 = b %d, e %d, a %d\n", begin, end, wrkAction);
					}
				}

				if (_ch2_input_enable && strstr(param.key, RCH2) != NULL)
				{
					// "RCH2%": [-100, -80],
					bool ex = true;
					for (int i = 0; i < 2; ++i)
					{
						const char* v = param.value.asArray()[i];
						// если буква не существует
						if (v == NULL) ex = false;
					}

					if (ex)
					{
						int begin = param.value.asArray()[0];
						int end = param.value.asArray()[1];

						// проверяем на вхождение в диапазон
						if (_min(begin, end) < ch2_pct && ch2_pct < _max(begin, end))
							wrkAction |= _BV(2);
						else
							last_wrkAction &= ~_BV(2);

						//DEBUG_FLY("RCH2 = b %d, e %d, a %d\n", begin, end, wrkAction);
					}
				}

				if (_ch1_input_enable && _ch2_input_enable && strstr(param.key, RCH1RCH2) != NULL)
				{
					// "RCH1%+RCH2%": [-80, -60, -100, -80],
					bool ex = true;
					for (int i = 0; i < 4; ++i)
					{
						const char* v = param.value.asArray()[i];
						// если буква не существует
						if (v == NULL) ex = false;
					}

					if (ex)
					{
						int begin1 = param.value.asArray()[0];
						int end1 = param.value.asArray()[1];
						int begin2 = param.value.asArray()[2];
						int end2 = param.value.asArray()[3];

						// проверяем на вхождение в диапазон
						if ((_min(begin1, end1) < ch1_pct && ch1_pct < _max(begin1, end1)) &&
							(_min(begin2, end2) < ch2_pct && ch2_pct < _max(begin2, end2)))
							wrkAction |= _BV(3);
						else
							last_wrkAction &= ~_BV(3);
						//DEBUG_FLY("RCH12 = b %d, e %d, b %d, e %d, a %d\n", begin1, end1, begin2, end2, wrkAction);
					}
				}
			} // for (auto param: signal.value.asObject()

/*
   			Serial.printf("PPM CH1 %d%% %d ms || PPM CH2  %d%% %d ms ||\tw: %d, l: %d\n",
    					ppm_input_ch1.iread_pct(),
						ppm_input_ch1.readMicroseconds(),
						ppm_input_ch2.iread_pct(),
						ppm_input_ch2.readMicroseconds(),
						wrkAction,
						last_wrkAction
				);
*/

   			if (wrkAction) _AllActionMissed = 0;
			// если в текущей секции стик сдвинулся (или зашел в диапазон)
			// то генерим событие и запоминаем позицию
			// если он остался в диапазоне то last_wrkAction не уменьшится
			if (wrkAction && (wrkAction != last_wrkAction))
			{
				// если попали в диапазон
				last_wrkAction = wrkAction;
				_AllActionMissed_active = 1;

				_AddAction(cur_sig.value.asObject()[StrAction]);
				_AddActionStep(cur_sig.key, cur_sig.value.asObject()[StrActionStep]);
			} // if (wrkAction)
			_SignalLastAction.push_back(std::make_pair(cur_sig.key, last_wrkAction));
		} // for (auto cur_sig: RSSection)
		if (_AllActionMissed  && _AllActionMissed_active)
		{
			_AddAction(RSSection[MissedAction]);
			_AllActionMissed_active = 0;
			//DEBUG_FLY("===MissedAction( %d )\n", _Actions.size());
		}

	}
}

// Добавление всех действий из списка
void FlyReceiverInputs::_AddAction(JsonArray& actions)
{
	if (actions != JsonArray::invalid())
	{
		// Actions
		for (auto action: actions)
		{
			_Actions.push_back(action.asString());
			//DEBUG_FLY("A: _Action+ %s\n", action.asString());
		}
	}
}

// добавление одного действия из списка "по кругу"
// key - название реакции на внешний канал
// actions - набор действий
void FlyReceiverInputs::_AddActionStep(const char* key, JsonArray& actions)
{
	// ActionsStep
	if (actions != JsonArray::invalid())
	{
		// ищем есть ли такой шаг
		int step = -1; // -1 - пусто, -2 - конец цепочки
		if (!_ActionStep.empty())
		{
			for (std::vector<TKeyStep>::iterator it = _ActionStep.begin(); it != _ActionStep.end(); ++it)
			//for (auto find: _ActionStep)
			{
				if (it->first == key)
				{
					// есть такой элемент
					//DEBUG_FLY("FA: %s, wA %d, S %d\n", it->first, it->wrkAction, it->currentStep);

					step = it->second + 1;
					if (step >= actions.size()) step = 0;
					it->second = step;
					break;
				}
			}
		}
		if (step < 0)
		{
			// добавляем
			_ActionStep.push_back(std::make_pair(key, 0));
			//DEBUG_FLY("AS: _ActionStep+ %s\n", cur_sig.key);
			step = 0;
		}
		// добавляем действие в список
		_Actions.push_back(actions[step].asString());
		//DEBUG_FLY("AS: _Action+ %s\n", actions[step].asString());
	}
}



