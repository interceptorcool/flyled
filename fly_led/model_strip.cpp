/*
 * model_strip.cpp
 *
 *  Created on: 3 мар. 2017 г.
 *      Author: icool
 */

#include "model_strip.hpp"
#include <string.h>



/*
 * ========================================================================================
 */

const char* FlyColorLeds::StartAction(const char* key_Action, const char* key_AlarmAction)
{
	// если эффект уже наш
	if (_strip == nullptr) return nullptr;
	if (!_conf.ready()) return nullptr;
	if (key_Action == nullptr && key_AlarmAction == nullptr) return nullptr;


	std::pair<uint32_t, uint32_t> isEffectAction = std::make_pair(0,0);
	std::pair<uint32_t, uint32_t> isEffectAlarm = std::make_pair(0,0);
	if (key_Action != nullptr)
	{
		JsonObject& action = _conf.getRoot()[key_Action];;
		if (action == JsonObject::invalid())
		{
			// если нет такого объекта то выходим
			return key_Action;
		}

		// вычисляем величину Animation
		isEffectAction = _isEffects(action);
		if (!isEffectAction.first) return key_Action; // не наши эффекты
	}

	if (key_AlarmAction != nullptr)
	{
		JsonObject& alarmAction =  _conf.getRoot()[key_AlarmAction];
		if (alarmAction != JsonObject::invalid())
		{

			isEffectAlarm = _isEffects(alarmAction);
			if (!isEffectAlarm.first) return key_Action; // не наши эффекты
		}
	}

	// проверям отображается ли эффекты
	// TODO возможно при null не правильно отвечает ?
	bool isDrawAction = (_lastAction == key_Action);
	bool isDrawAlarmAction = (_lastAlarmAction == key_AlarmAction);

	if (!isDrawAction)
	{
		// основной эффект не отображается
		_lastAction = key_Action;
	}

	if (!isDrawAlarmAction)
	{
		// аларм эффект не отображается
		_lastAlarmAction = key_AlarmAction;
	}

	if (isDrawAction && isDrawAlarmAction)	return nullptr;	// оба эффекта отображены



	uint16 countAnimation;
	countAnimation = isEffectAction.second;
	countAnimation += isEffectAlarm.second;


	if (_animation != nullptr)
	{
		_animation->StopAll();
		delete _animation;
		_animation = nullptr;
	}

	DEBUG_FLY("FlyColorLeds::StartAction( %s ) %s = cA: %d \n", _lastAction, _lastAlarmAction, countAnimation);

	_countUseAnimation = 0;
	if (countAnimation)
	{
		_animation = new NeoPixelAnimator(countAnimation, NEO_MILLISECONDS);
	} else {
		_animation = nullptr;
	}

	if (isEffectAction.first)
	{
		// создаем основные эффекты
		JsonObject& action = _conf.getRoot()[_lastAction];
		_Effect_Selection(action, _lastAction);
	}
	if (isEffectAlarm.first)
	{
			// алармовые
			JsonObject& alarmAction =  _conf.getRoot()[_lastAlarmAction];
			_Effect_Selection(alarmAction, _lastAlarmAction);
	}

	return nullptr;
}
/*
 * возвращает количество "наших" эффектов если они есть
 * и число анимаций
 */
std::pair<uint32_t, uint32_t> FlyColorLeds::_isEffects(JsonObject& action)
{
	uint32_t countAnimation = 0;
	uint32_t isEffectAction = 0;
	for (auto e: action)
	{
		for (int i = 0; i < SIZEOF_STEFFECT(); ++i)
		{
			if (strstr(e.key, effectName[i].name) != NULL)
			{
				countAnimation += effectName[i].countAnimations;
				isEffectAction++;
			}
			//DEBUG_FLY("F: %s == %s\n", e.key, effectName[i].name);
		}
	}
	return std::make_pair(isEffectAction, countAnimation);
}

void FlyColorLeds::_Effect_Selection(JsonObject& action, const char* key_Action)
{
	if (action == JsonObject::invalid()) return;

	// устанавливаем эффекты
	for (auto a: action)
	{
		if (strstr(a.key, BRIGHTNESS) != NULL)
		{
			// устанавливаем яркость (если есть) - для элемента
			int bright_pct = a.value.as<int>();
			if (bright_pct)
			{
				SetBrightness(bright_pct);
				//DEBUG_FLY("B: %s == %d\n", a.key, bright_pct);
			}
		}
		if (strstr(a.key, effectName[cEffectGlow].name) != NULL) _Effect_glow(a.value.asObject());
		if (strstr(a.key, effectName[cEffectBlink].name) != NULL) _Effect_blink(key_Action, a.key);
		if (strstr(a.key, effectName[cEffectFade].name) != NULL) _Effect_fade(key_Action, a.key);
		if (strstr(a.key, effectName[cEffectRand].name) != NULL) _Effect_random(key_Action, a.key);
	}
}

//////////////// Glow effect

void FlyColorLeds::_Effect_glow(JsonObject& glow)
{
	if (glow == JsonObject::invalid()) return;

	for (auto objects: glow)
	{
		// get color
		SetGroupPixelColor(objects.key, getColor(objects.value.asString()));
	}
	_redraw = true;
	//Show();
}

//////////////// Blink effect

void FlyColorLeds::_Effect_blink(const char* key_Action, const char* key_Effect)
{
	if (_animation == nullptr) return;

	// читаем показание задержек для этой секции

	JsonObject& root = _conf.getRoot();
	JsonObject& blink = root[key_Action][key_Effect];

	if (blink == JsonObject::invalid()
		&& blink[DELAYNAME].asArray() == JsonArray::invalid())
	{
		DEBUG_FLY("JSON not Object \"%s\"\n", key_Effect);
		return;
	}

	// настраиваем анимационные эффекты
	// включение
	//DEBUG_FLY("FlyBlinkEffect 0x%08x = 0x%08x \n", (uint)this, uint(&_conf)); delay(200);
	FlyBlinkEffect *be = new FlyBlinkEffect(*this, _conf, key_Action, key_Effect);
	if (be == nullptr) return;
	if (be->GetDelay())
	{
		//DEBUG_FLY("+ %d - cA: %d \n", be->GetDelay(), _countUseAnimation);
		_animation->StartAnimation(_countUseAnimation++, be->GetDelay(), be);
	} else {
		delete be;
	}

}

//////////////// Fade effect

void FlyColorLeds::_Effect_fade(const char* key_Action, const char* key_Effect)
{
	if (_animation == nullptr) return;

	// читаем показание задержек для этой секции
	JsonObject& fade = _conf.getRoot()[key_Action][key_Effect];

	if (fade == JsonObject::invalid()
		&& fade[DELAYNAME].asArray() == JsonArray::invalid())
	{
		DEBUG_FLY("JSON not Object \"%s\"\n", key_Effect);
		return;
	}

	//DEBUG_FLY("_Effect_fade %s %s\n", key_Action, key_Effect);

	// настраиваем анимационные эффекты
	// включение
	FlyFadeEffect *fe = new FlyFadeEffect(*this, _conf, key_Action, key_Effect);
	_animation->StartAnimation(_countUseAnimation++, fade[DELAYNAME][0].as<uint16_t>(), fe);
	FlyFadeEffectMove *fm = new FlyFadeEffectMove(*this, _conf, key_Action, key_Effect);
	_animation->StartAnimation(_countUseAnimation++, fade[DELAYNAME][1].as<uint16_t>(), fm);
}

//////////////// Random effect

void FlyColorLeds::_Effect_random(const char* key_Action, const char* key_Effect)
{
	if (_animation == nullptr) return;

	// читаем показание задержек для этой секции
	JsonObject& fade = _conf.getRoot()[key_Action][key_Effect];

	if (fade == JsonObject::invalid()
		&& fade[DELAYNAME].asArray() == JsonArray::invalid())
	{
		DEBUG_FLY("JSON not Object \"%s\"\n", key_Effect);
		return;
	}

	// настраиваем анимационные эффекты
	// включение
	FlyFadeEffect *fe = new FlyFadeRandom(*this, _conf, key_Action, key_Effect);
	_animation->StartAnimation(_countUseAnimation++, fade[DELAYNAME][0].as<uint16_t>(), fe);
	FlyFadeEffectMove *fm = new FlyFadeEffectMove(*this, _conf, key_Action, key_Effect);
	_animation->StartAnimation(_countUseAnimation++, fade[DELAYNAME][1].as<uint16_t>(), fm);
}


/*
 * ========================================================================================
 */

FlyBlinkEffect::FlyBlinkEffect(FlyColorLeds& flyLeds, TFlyConf& fc, const char* jsonKeyAction, const char* jsonKeyEffect):
	_draw(false),
	_keyAction(jsonKeyAction),
	_keyEffect(jsonKeyEffect),
	_conf(fc),
	_pleds(flyLeds)
{
	// проверенно, что массив при инициализации
	JsonArray& json_delay = _conf.getRoot()[_keyAction][_keyEffect][DELAYNAME];

	_delayAll = 0;
	_maxProgressStep = json_delay.size();
	_progressStep = 0;
	_currentProgressEnd = 0.0;
	for (auto jd: json_delay)
	{
		_delayAll += jd.as<uint16_t>();
	}
	if (!_delayAll) _delayAll = 200.0;
}

float FlyBlinkEffect::_calcProgress(uint16_t progressStep)
{
	if (progressStep < _maxProgressStep)
	{
		uint16_t delay = _conf.getRoot()[_keyAction][_keyEffect][DELAYNAME][progressStep].as<uint16_t>();
		return float(delay)/float(_delayAll);
	} else {
		return -1.0;
	}

}

void FlyBlinkEffect::animUpdate(AnimationParam& param)
{
	if (_keyAction == nullptr || _keyEffect == nullptr) return;

//	Serial.printf("*** animUpdate: 0x%08x : %d = ", uint32_t(this), param.state);
//	Serial.println(_currentProgressEnd);
//	delay(200);


	switch (param.state)
	{
	case AnimationState::AnimationState_Started:
		{
			_draw = true;
			_progressStep = 0;
			_currentProgressEnd = _calcProgress(0);
			break;
		}
	case AnimationState::AnimationState_Progress:
		{
			if (param.progress >= _currentProgressEnd)
			{
				_progressStep++;
				if (_progressStep < _maxProgressStep)
				{
					_currentProgressEnd += _calcProgress(_progressStep);
					_draw = true;
				} else {
					_currentProgressEnd = 1.0;
				}
			}
			break;
		}
	case AnimationState::AnimationState_Completed:
		{
			param.state = AnimationState::AnimationState_GoToRestart;
			//DEBUG_FLY("====== FlyBlinkEffect: %d Complite\n", param.index);
			break;
		}
	}

	if (_draw)
	{
		JsonObject& effectConfig = _conf.getRoot()[_keyAction][_keyEffect];

		//DEBUG_FLY("Blink 0x%08x : %s, %s, %d\n", uint32_t(this), _keyAction, _keyEffect, _progressStep);

		for (auto elemet: effectConfig)
		{
			JsonArray& aColor = elemet.value.asArray();

			// пропускаем временную задержку
			if (strstr(elemet.key,  DELAYNAME) == NULL && aColor != JsonObject::invalid())
			{
				const char* jsoncolor = aColor[_progressStep % aColor.size()].asString();
				//DEBUG_FLY("Blink : %s, %s\n", elemet.key, jsoncolor);

				_pleds.SetGroupPixelColor(elemet.key, _pleds.getColor(jsoncolor));
			}
		}
		_draw = false; // не перерисовывать до изменений
	}
}


/*
 * =======================================================
 */
FlyFadeEffect::FlyFadeEffect(FlyColorLeds& flyLeds, TFlyConf& fc, const char* jsonKeyAction, const char* jsonKeyEffect):
	_keyAction(jsonKeyAction),
	_keyEffect(jsonKeyEffect),
	_pleds(flyLeds),
	_flyConfig(fc),
	_currentDir(1)
{

}


void FlyFadeEffect::animUpdate(AnimationParam& param)
{
	if (_keyAction == nullptr || _keyEffect == nullptr) return;

	JsonObject& effectConfig = _flyConfig.getRoot()[_keyAction][_keyEffect];
	int dir = effectConfig[EFFECTCONFIG][0].as<int>();

	if (param.state == AnimationState::AnimationState_Started)
	{
		if (dir == 0)
		{
			_currentDir *= -1;
		} else {
			_currentDir = dir;
		}
	}

	if (param.state == AnimationState::AnimationState_Progress)
	{
		for (auto elemet: effectConfig)
		{
			if (strstr(elemet.key,  DELAYNAME) != NULL
				|| strstr(elemet.key,  EFFECTCONFIG) != NULL) continue;

			size_t jsonLedsInGroup = _pleds.GroupSize(elemet.key);
			if (jsonLedsInGroup == 0) continue;

	//		DEBUG_FLY_PORT.printf("F0 %s, %d\n", elemet.key, jsonLedsInGroup);
	//		delay(200);

			uint16_t pixel;
			if (_currentDir > 0)
			{
				pixel = param.progress * (float)jsonLedsInGroup;
			} else if (_currentDir < 0)
			{
				pixel = (1.0f - param.progress) * (float)jsonLedsInGroup;
			}

			_pleds.SetGroupPixelColor(elemet.key, _pleds.getColor(elemet.value.asString()), pixel);
		}
	}

	if (param.state == AnimationState::AnimationState_Completed)
	{
		param.state = AnimationState::AnimationState_GoToRestart;
	}

}

/*
 * =======================================================
 */


void FlyFadeEffectMove::animUpdate(AnimationParam& param)
{
	if (_keyAction == nullptr || _keyEffect == nullptr) return;

	if (param.state == AnimationState_Completed)
	{
		JsonObject& effectConfig = _flyConfig.getRoot()[_keyAction][_keyEffect];
		uint8_t DarkenDelta = effectConfig[EFFECTCONFIG].asArray()[1].as<uint8_t>();

		for (auto elemet: effectConfig)
		{
			if (strstr(elemet.key,  DELAYNAME) != NULL
				|| strstr(elemet.key,  EFFECTCONFIG) != NULL) continue;

				_pleds.MoveGroupColor(elemet.key, [DarkenDelta](RgbColor color) {
					color.Darken(DarkenDelta);
					return color;
				});
		}

		param.state = AnimationState::AnimationState_GoToRestart;
	}
}

/*
 * =======================================================
 */

void FlyFadeRandom::animUpdate(AnimationParam& param)
{
	if (_keyAction == nullptr || _keyEffect == nullptr) return;

	switch(param.state)
	{
	case AnimationState::AnimationState_Started:
	{
		JsonObject& effectConfig = _flyConfig.getRoot()[_keyAction][_keyEffect];
		int jsonLedsOnGroup = effectConfig[EFFECTCONFIG][0].as<int>();
		if (jsonLedsOnGroup < 1)
		{
			jsonLedsOnGroup = 1;
		}

		for (auto elemet: effectConfig)
		{
			if (strstr(elemet.key,  DELAYNAME) != NULL
				|| strstr(elemet.key,  EFFECTCONFIG) != NULL) continue;

			size_t jsonLedsInGroup = _pleds.GroupSize(elemet.key);
			if (jsonLedsInGroup == 0) continue;

			// зажигаем светодиоды в группе по рандому месту
			for (int j = 0; j < jsonLedsOnGroup; j++)
			{
				_pleds.SetGroupPixelColor(elemet.key, _pleds.getColor(elemet.value.asString()), random(jsonLedsInGroup));
			}
		}
		break;
	}
	case AnimationState::AnimationState_Completed:
		param.state = AnimationState::AnimationState_GoToRestart;
		break;
	}

}

/*
 * =======================================================
 */
#if 0
void FlyFlashEffect::animUpdate(AnimationParam& param)
{
	if (_keyAction == nullptr || _keyEffect == nullptr) return;

	switch(param.state)
	{
	case AnimationState::AnimationState_Started:
		_currentDir = 1;
		break;
	case AnimationState::AnimationState_Completed:
		param.state = AnimationState::AnimationState_GoToRestart;
		break;
	case AnimationState::AnimationState_Progress:
		JsonObject& effectConfig = _flyConfig.getRoot()[_keyAction][_keyEffect];

		if (param.progress > 0.5)
		{
			_currentDir = -1;
		}

		uint8_t DarkenDelta = effectConfig[EFFECTCONFIG].asArray()[1].as<uint8_t>();

		for (auto elemet: effectConfig)
		{
			if (strstr(elemet.key,  DELAYNAME) != NULL
				|| strstr(elemet.key,  EFFECTCONFIG) != NULL) continue;

				_pleds.MoveGroupColor(elemet.key, [DarkenDelta, _currentDir](RgbColor color) {
					if (_currentDir > 0)
					{
						color.Lighten(DarkenDelta);
					} else {
						color.Darken(DarkenDelta);
					}
					return color;
				});
		}
		break;
	}

}
#endif
