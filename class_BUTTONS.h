/*
 * class_BUTTONS.h
 *
 * Created: 06.07.2020 16:10:19
 *  Author: Rodin
 */ 


#ifndef CLASS_BUTTONS_H_
#define CLASS_BUTTONS_H_

#include <avr/io.h>

//порт, к которому подключены кнопки
#define PORT_BUTTON		PORTB
#define PIN_BUTTON		PINB
#define DDR_BUTTON		DDRB

//номера выводов, к которым подключены кнопки
#define HEAT_PIN			0
#define FILL_PIN			1
#define PUMP_PIN			2
#define FILL_AND_HEAT_PIN	3
#define AUTO_PIN			4
#define MASK_BUTTONS		(1<<HEAT_PIN)|(1<<FILL_PIN)|(1<<PUMP_PIN)|(1<<FILL_AND_HEAT_PIN)|(1<<AUTO_PIN)

//сколько циклов опроса кнопка должна удерживаться
#define THRESHOLD	3

class Buttons
{
public:
	enum enum_buttons{ none, HEAT, FILL, PUMP, F_AND_H, AUTO };
	static enum_buttons but_pressed;
	static void Init(void);
	static void check(void);
};

#endif /* CLASS_BUTTONS_H_ */