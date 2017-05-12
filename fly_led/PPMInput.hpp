/*
 * PPMInput.h
 *
 *  Created on: 10 февр. 2017 г.
 *      Author: icool
 */

#pragma once

#ifndef PPMINPUT_H_
#define PPMINPUT_H_

#include <Arduino.h>
#include <stdlib.h>
#include <Filters.h>

#ifdef eclipse
#include <Servo/src/Servo.h>
#else
#include <Servo.h>
#endif


template <uint8_t C_PIN> class PPMInput
{
public:
	PPMInput (float frequency = 1.0,
			int delta = 2,
			int centrePPM = 1510
		  ):
		_delta(delta), _ppm_time_save(0), _currentsignal(MIN_PULSE_WIDTH), _centrePPM(centrePPM), _Kline(400), _change(false)
	{
		_filterOneLowpass = new FilterOnePole(LOWPASS, frequency, 0);
		s_this = this;
	}

	~PPMInput()
	{
		free(_filterOneLowpass);
	}

	// Initialization
	void setup()
	{
	      pinMode(C_PIN, INPUT_PULLUP);
	      attachInterrupt(C_PIN, pin_change_isr, CHANGE);
	}

	// -1.0 до 1.0 значение
	float read_pct(void)
	{
		float cur_ppm = (float)readJitter();
		return (cur_ppm - (float)_centrePPM)/((float)_Kline);
	}

	// -100 до 100 значение
	int iread_pct(void)	{ return (int)((float)read_pct()*100.0); }

	int readMicroseconds(void) const
	{
		int i = (int)_filterOneLowpass->output();
		if (i < MIN_PULSE_WIDTH) i = MIN_PULSE_WIDTH;
		if (i > MAX_PULSE_WIDTH) i = MAX_PULSE_WIDTH;
		return i;
	}

	int readJitter(void)
	{
		int i =  readMicroseconds();
		_change = abs(_currentsignal - i) > _delta;
		if (_change)
		{
			_currentsignal = i;
		}
		return _currentsignal;
	}

	bool IsChange(void) const { return _change; }
	bool IsChange(int signal_ms) const {
		return (abs(signal_ms - readMicroseconds()) > _delta);
	}

	void setCentrePPM(const int centrePPM) { _centrePPM = centrePPM; 	}
	void setKpctLine(const int Kline) { _Kline = Kline; }

private:
	static PPMInput* s_this; 	// for the ISR

	uint32_t _ppm_time_save;
	FilterOnePole* _filterOneLowpass;

	int 	_centrePPM;
	int		_Kline;
	int 	_delta;
	int		_currentsignal;
	bool	_change;

	static void pin_change_isr(void)
	{
		int d = 0;
		for (int j = 0; j < 3; ++j)
		{
			d |= digitalRead(C_PIN);
		}

		if (d)
	    {
	      // PPM ___/---
	    	s_this->_ppm_time_save = micros();
	    } else {
	      // PPM ---\___
	      uint32_t ppm_time = micros() - s_this->_ppm_time_save;

	      // update the one pole lowpass filter, and statistics
	      s_this->_filterOneLowpass->input( ppm_time );
	    }
	}
};

template <uint8_t C_PIN>
PPMInput<C_PIN> * PPMInput<C_PIN>::s_this;


#endif /* PPMINPUT_H_ */
