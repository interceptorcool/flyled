/*
 * model_strip.h
 *
 *  Created on: 1 мар. 2017 г.
 *      Author: icool
 */

#pragma once

#ifndef FLY_LED_MODEL_STRIP_HPP_
#define FLY_LED_MODEL_STRIP_HPP_

#include <stdint.h>
#include <string.h>
#include <vector>
#include <functional>
#include <algorithm>

#include <FS.h>
#include <NeoPixelBus.h>
#include <NeoPixelBrightnessBus.h>
#include <NeoPixelAnimator.h>


#include "board_pin.h"

#ifdef eclipse
#include </home/icool/Arduino/libraries/ArduinoJson-master/ArduinoJson.h>
#else
#include <ArduinoJson.h>
#endif


//#define DEBUG_FLY
#define DEBUG_FLY_PORT Serial

#ifdef DEBUG_FLY
#ifdef DEBUG_FLY_PORT
#define DEBUG_FLY(...) DEBUG_FLY_PORT.printf("DBG_FLY: "); DEBUG_FLY_PORT.printf( __VA_ARGS__ )
#endif
#endif

#ifndef DEBUG_FLY
#define DEBUG_FLY(...)
#endif

class FlyColorLeds;

class TFlyConf
{
	friend	class FlyColorLeds;

public:
	const char *filename = "/config.json";

private:
	JsonObject*			_jsonRootptr;
	DynamicJsonBuffer 	_jsonBuffer;

public:
	TFlyConf()
	{
		_jsonRootptr = nullptr;

	}

	void setup()
	{
		if (SPIFFS.exists(filename))
		{
			File JSONFile = SPIFFS.open(filename, "r");
			String rd_json = JSONFile.readString();

			//Serial.print(rd_json);

			JSONFile.close();

			JsonObject& root = _jsonBuffer.parseObject(rd_json);

			if (root.success())
			{
				_jsonRootptr = &root;
				//DEBUG_FLY("jsonRoot success\n");
				Serial.printf("jsonRoot success, buffer size %d\n", _jsonBuffer.size());
			} else {
				_jsonRootptr = nullptr;
				//DEBUG_FLY("jsonRoot error\n");
				Serial.print("jsonRoot error\n");
			}
		} else {
			//DEBUG_FLY("File %s not found\n", filename);
			Serial.printf("File %s not found\n", filename);
		}
	}

	bool ready() const { return _jsonRootptr != nullptr; }

	JsonObject& getRoot() const { return *_jsonRootptr; }

};

// =====================
// =====================

struct stEffect {
	const char*	name;
	uint16_t	countAnimations;
};

enum en_effectNum_t {
	cEffectGlow = 0,
	cEffectBlink,
	cEffectFade,
	cEffectRand
};

const struct stEffect effectName[] = {
		{ .name = "GLOW", .countAnimations = 0 },
		{ .name = "BLINK", .countAnimations = 1 },
		{ .name = "FADE", .countAnimations = 2 },
		{ .name = "RAND", .countAnimations = 2 }
};

#define SIZEOF_STEFFECT()	(sizeof(effectName)/sizeof(struct stEffect))


static const char* strSTRIPLEDPIXELCOUNT = "STRIPLEDPIXELCOUNT";
static const char* LEDCONFIGGROUP 	= "LEDGROUP";
static const char* LEDCOLORGROUP 	= "LEDCOLOR";
static const char* DELAYNAME 		= "DELAY_MS";
static const char* EFFECTCONFIG		= "CONFIG";
static const char* BRIGHTNESS		= "BRIGHT";
static const char* BATTARY 			= "BATTARY";
static const char* BATTARYTYPE 		= "TYPE";
static const char* LED0				= "LED0";
static const char* CRITICAL			= "CRITICAL";
static const char* WARNING			= "WARNING";
static const char* ReceiverInputs 	= "RECEIVERINPUTS";
static const char* InputsConfig 	= "INPUTS";
static const char* RCH1 			= "RCH1%";
static const char* RCH2 			= "RCH2%";
static const char* RCH1RCH2 		= "RCH1+2%";
static const char* StrAction 		= "ACTION";
static const char* StrActionStep 	= "ACTIONSTEP";
static const char* MissedAction 	= "MISSED_ACTION";
static const char* strOutputsCFG 	= "OUTPUTS";
static const char* INITOUT			= "INITOUT";
static const char* SETMAXIMUMBRIGTH = "SETMAXIMUMBRIGTH";

//////////////// Blink effect

class FlyBlinkEffect: public AnimUpdate
{
private:
	bool			_draw;
	uint8_t			_maxProgressStep;
	uint8_t			_progressStep;
	uint16_t		_delayAll;
	float			_currentProgressEnd;
	const char* 	_keyAction;
	const char* 	_keyEffect;
	FlyColorLeds&	_pleds;
	TFlyConf&		_conf;

public:
	FlyBlinkEffect(FlyColorLeds& flyLeds, TFlyConf& fc, const char* jsonKeyAction, const char* jsonKeyEffect);

	virtual ~FlyBlinkEffect() {};

	virtual void animUpdate(AnimationParam& param);

	uint16_t GetDelay() const { return _delayAll; };

private:
//	FlyBlinkEffect(const FlyBlinkEffect&);
//	void operator = (const FlyBlinkEffect&);

	float _calcProgress(uint16_t progressStep);
};

/////////////////////////
class FlyFadeEffect: public AnimUpdate
{
protected:
	const char* 	_keyAction;
	const char* 	_keyEffect;
	FlyColorLeds&	_pleds;
	int				_currentDir;
	TFlyConf&		_flyConfig;

public:
	FlyFadeEffect(FlyColorLeds& flyLeds, TFlyConf& fc, const char* jsonKeyAction, const char* jsonKeyEffect);

	virtual ~FlyFadeEffect() {};
	virtual void animUpdate(AnimationParam& param);

private:
	FlyFadeEffect(const FlyFadeEffect&);
	void operator = (const FlyFadeEffect&);
};

/////////////////////////
class FlyFadeEffectMove: public FlyFadeEffect
{
	using FlyFadeEffect::FlyFadeEffect;
private:
	float	_delay;
public:
	virtual void animUpdate(AnimationParam& param);
};

/////////////////////////
class FlyFadeRandom: public FlyFadeEffect
{
	using FlyFadeEffect::FlyFadeEffect;
public:
	virtual void animUpdate(AnimationParam& param);
};


/////////////////////////
#if 0
class FlyFlashEffect: public FlyFadeEffect
{
	using FlyFadeEffect::FlyFadeEffect;
public:
	virtual void animUpdate(AnimationParam& param);
};
#endif
/*
 * ============================================================
 */

class FlyColorLeds
{
private:
//	FlyColorLeds(const FlyColorLeds&);
//	void operator = (const FlyColorLeds&);

public:
	typedef NeoPixelBrightnessBus<NeoGrbFeature, Neo800KbpsMethod> TFlyStrip;
	//typedef NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> TFlyStrip;
	typedef std::vector<uint16_t> TLedsArray;
	typedef std::function<RgbColor(RgbColor currentColor)>	MoveGroupChangeColor;

private:
	TFlyStrip* 			_strip;
	NeoPixelAnimator*	_animation;
	uint16_t			_countUseAnimation;
	const char*			_lastAction;
	const char*			_lastAlarmAction;
	bool				_redraw;
	float				_curGradient;
	TFlyConf&			_conf;

public:
	FlyColorLeds(TFlyConf& fc):
		_conf(fc),
		_strip(nullptr),
		_animation(nullptr),
		_countUseAnimation(0),
		_lastAction(nullptr),
		_lastAlarmAction(nullptr),
		_redraw(false),
		_curGradient(100)
	{
	}

	~FlyColorLeds()
	{
		StopCurrentAction();
		if (_strip != nullptr)
			delete _strip;
	}

	bool ready() const { return _strip != nullptr; };
	TFlyStrip& strip() { return *_strip; };

	void setup()
	{
		if (_conf.ready())
		{
			JsonObject& root = _conf.getRoot();
			uint32_t StripLedPixelCount = root[strSTRIPLEDPIXELCOUNT];
			// длинна есть
			if (StripLedPixelCount < 1)
			{
				StripLedPixelCount = 1;
			} else
			{
				StripLedPixelCount++;
			}
			_strip = new TFlyStrip(StripLedPixelCount);
			_strip->Begin();
			_strip->ClearTo(HsbColor(0,0,0));

			uint8_t MaxBrigth = root[SETMAXIMUMBRIGTH];
			if (!MaxBrigth || MaxBrigth == 255)
			{
				SetMAXBrightness(254);
			} else {
				SetMAXBrightness(MaxBrigth);
			}
			_redraw = true;

			DEBUG_FLY("Led Strip init, pixels - %d\n", StripLedPixelCount);
		} else {
			DEBUG_FLY("Not section %s on JSON Config\n", strSTRIPLEDPIXELCOUNT);
		}
	}

	void SetBrightness(uint8_t br_pct)
	{
		_curGradient = (float(br_pct) / 100.0);
		if (_curGradient > 1.0) _curGradient = 1.0;
		if (_curGradient < 0.0) _curGradient = 0.0;
		//DEBUG_FLY_PORT.print("SetBr"); DEBUG_FLY_PORT.println(_curGradient);
	}
	void SetMAXBrightness(uint8_t br)
	{
		_strip->SetBrightness(br);
	}

	void Show(bool Forced = false)
	{
		if (_strip == nullptr) return;
		if (!_redraw && !Forced) return;
		_strip->Show();
		_redraw = false;
	}

	size_t GroupSize(const char* key_LedConfigGroup)
	{
		if (key_LedConfigGroup == nullptr) return 0;

		TLedsArray ledsArray;

		if (_getLeds(key_LedConfigGroup, ledsArray)) return 0;

		return ledsArray.size();
	}

	void SetLED0PixelColor(RgbColor color)
	{
		if (_strip == nullptr) return;
		_strip->SetPixelColor(0, color, 1.0);
		_redraw = true;
	}

	// выводим цвет в группу светодиодов
	void SetGroupPixelColor(const char* key_LedConfigGroup, RgbColor color, int pixel = -1)
	{
		if (_strip == nullptr && key_LedConfigGroup == nullptr) return;

		TLedsArray ledsArray;

		if (_getLeds(key_LedConfigGroup, ledsArray)) return;

		// TODO яркость на пикселе

		if (pixel >= 0 && pixel < ledsArray.size())
		{
			_strip->SetPixelColor(ledsArray[pixel], color, _curGradient);
		} else {
			for (auto led: ledsArray)
			{
				_strip->SetPixelColor(led, color, _curGradient);
			}
		}
		_redraw = true;
	}

	void MoveGroupColor(const char* key_LedConfigGroup, MoveGroupChangeColor ChangeFnc)
	{
		if (_strip == nullptr && key_LedConfigGroup == nullptr) return;

		TLedsArray ledsArray;

		if (_getLeds(key_LedConfigGroup, ledsArray)) return;

		RgbColor cc;
		for (auto led: ledsArray)
		{
			cc = _strip->GetPixelColor(led);
			cc = ChangeFnc(cc);
			_strip->SetPixelColor(led, cc, _curGradient);
		}
		_redraw = true;
	}

	// работа по выбору эффектов
	const char* StartAction(const char* key_Action, const char* key_AlarmAction);


	void StopCurrentAction()
	{
		if (_animation != nullptr)
		{
			_animation->StopAll();
			delete _animation;
			_animation = nullptr;
		}
		_lastAction = nullptr;
		_lastAlarmAction = nullptr;
	}

	void UpdateAnimations()
	{
		if (!_conf.ready()) return;
		if (_animation != nullptr && _animation->IsAnimating())
		{
			_animation->UpdateAnimations();
		}
	}

	RgbColor getColor(const char* key_LedColorGroup_or_Color, const char* defautColor = "#000000")
	{
		const char* htmlcolorstr = key_LedColorGroup_or_Color;

		if (htmlcolorstr != nullptr)
		{
			if (strchr(htmlcolorstr, '#') == NULL)
			{
				JsonObject& root = _conf.getRoot();
				htmlcolorstr = root[LEDCOLORGROUP][key_LedColorGroup_or_Color];
			}
		}
		if (htmlcolorstr == nullptr)
		{
			// если такого цвета нет то не применяем
			htmlcolorstr = defautColor;
		}

		//DEBUG_FLY("Color: k: %s, c: %s\n", key_LedColorGroup_or_Color, htmlcolorstr);

		HtmlColor htmlcolor;
		htmlcolor.Parse<HtmlColorNames>(htmlcolorstr);
		return RgbColor(htmlcolor);
	}

private:
	// LedArrays

	int _getLeds(const char* key_LedConfigGroup, TLedsArray& ledArray)
	{
		if (key_LedConfigGroup == nullptr || _strip == nullptr) return -1;

		int r = _AnalyzeLeds(key_LedConfigGroup, ledArray);
		if (r > 0)
		{
			// нашли где-то all
			ledArray.clear();
			for (uint16_t i = 1; i < _strip->PixelCount(); ++i)
			{
				ledArray.push_back(i);
			}
			return 0;
		}
		return r;
	}

	int _AnalyzeLeds(const char* key_LedConfigGroup, TLedsArray &ledsArray)
	{
		if (!_conf.ready()) return -1;
		if (key_LedConfigGroup == nullptr) return -1;

		JsonObject& root = _conf.getRoot();
		JsonObject& LedConfigGroup = root[LEDCONFIGGROUP];

		// проверяем наличие
		if (LedConfigGroup == JsonObject::invalid())
		{
			DEBUG_FLY("JSON not Object \"LedConfigGroup\"\n");
			return -1;
		}
		JsonArray& CurrentLedArray = LedConfigGroup[key_LedConfigGroup];

		if (LedConfigGroup == JsonArray::invalid())
		{
			DEBUG_FLY("JSON not Object \"%s\"\n", key_LedConfigGroup);
			return -1;
		}

		int ret = 0;
		for (auto led: CurrentLedArray)
		{
			if (led.is<char*>())
			{
				if (!strcmp(led.asString(), "all"))
				{
					return 1;
				}
				String s(led.asString());
				uint pm = s.indexOf('-');
				uint begin = atoi(s.substring(0, pm).c_str());
				uint end =   atoi(s.substring(pm+1, s.length()).c_str());
				if (begin > 0 && end > 0)
				{
					int dir = 1;
					if (begin > end) dir = -1;

					for (int led = begin; led != end + dir; led += dir)
					{
						ledsArray.push_back(led);
					}
				} else {
					ret = _AnalyzeLeds(led.asString(), ledsArray);
					if (ret) break;
				}
			} else if (led.is<uint32_t>()) {
				uint16_t c = led.as<uint16_t>();
				if (c >= 1 && c < _strip->PixelCount())
				{
					ledsArray.push_back(c);
				}
			}
		}
		return ret;
	}

	//////////////// effect
	void _Effect_Selection(JsonObject&, const char*);
	void _Effect_glow(JsonObject& glow);
	void _Effect_blink(const char* key_Action, const char* key_Effect);
	void _Effect_fade(const char* key_Action, const char* key_Effect);
	void _Effect_random(const char* key_Action, const char* key_Effect);


	std::pair<uint32_t, uint32_t> _isEffects(JsonObject&);

};



#endif /* FLY_LED_MODEL_STRIP_HPP_ */
