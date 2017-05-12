/*
 * board_pin.h
 *
 *  Created on: 21 февр. 2017 г.
 *      Author: icool
 */

#ifndef FLY_LED_BOARD_PIN_H_
#define FLY_LED_BOARD_PIN_H_

// ESP8266 - Arduino Stile configuration PIN

// Button connect to GND, changed boot mode use or not WiFi
#define FLYLED_BUTTON_PIN	(12)

// Receiver CH1 Input
#define FLYLED_RCV_CH1_PIN	(5)
#define FLYLED_RCV_CH2_PIN	(4)

// Servo/Digital out

#define FLYLED_OUT_CH1_PIN	(0)
#define FLYLED_OUT_CH1_PWR	(14)
#define FLYLED_OUT_CH2_PIN	(15)
#define FLYLED_OUT_CH2_PWR	(13)
#define FLYLED_OUT_PWR_OFF	(HIGH)
#define FLYLED_OUT_PWR_ON	(LOW)


// ESP-12 on board LED
#define FLYLED_STATUS_LED_PIN	(2)
#define FLYLED_STATUS_LED_OFF	(HIGH)
#define FLYLED_STATUS_LED_ON	(LOW)



#endif /* FLY_LED_BOARD_PIN_H_ */
