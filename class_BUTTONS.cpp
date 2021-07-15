/*
 * class_BUTTONS.cpp
 *
 * Created: 06.07.2020 16:10:38
 *  Author: Rodin
 */ 

#include "class_BUTTONS.h"

Buttons::enum_buttons Buttons::but_pressed;

void Buttons::Init(void) 
{
	DDR_BUTTON &= ~(MASK_BUTTONS);
	PORT_BUTTON &= ~(MASK_BUTTONS);
}

void Buttons::check(void)
{
	static uint8_t comp = 0;
	enum_buttons temp = none;
	but_pressed = none;
	
	//последовательный опрос выводов мк
	if ( PIN_BUTTON & (1<<HEAT_PIN) )
		temp = HEAT;
	else if ( PIN_BUTTON & (1<<FILL_PIN) )
		temp = FILL;
	else if ( PIN_BUTTON & (1<<PUMP_PIN) )
		temp = PUMP;
	else if ( PIN_BUTTON & (1<<FILL_AND_HEAT_PIN) )
		temp = F_AND_H;
	else if ( PIN_BUTTON & (1<<AUTO_PIN) )
		temp = AUTO;
	else {
		temp = none;
	}
	
	if (temp != none) {
		comp++;
		if (comp == THRESHOLD) {
			but_pressed = temp;
			return;
		}				
	} else {
		comp = 0;
	}
}