/*
 * shower.cpp
 *
 * Created: 05.07.2020 16:45:29
 * Author : Sysvamp
 */ 


#define F_CPU 1000000UL
#define uchar unsigned char
#define false 0
#define true !false
#define BitIsSet(reg, bit)  ((reg & (1<<(bit))) != 0)
#define ClearBit(reg, bit)	(reg) &= (~(1<<(bit)))
#define SetBit(reg, bit)	(reg) |= (1<<(bit))

#define HOURS_BEFORE 8		// За сколько часов, до включения нагрева, наливать воду? (авто режим)
#define DELAY_FILLING 25	// Задержка выключения налива воды после срабатывания датчика (0 до 254 сек)
#define DELAY_PUMP 240		// Задержка выключения перемешивателя после срабатывания датчика (0 до 254 мин)

#define CTRL_HEAT 0
#define CTRL_FILL 1
#define CTRL_PUMP 2
#define CTRL_DDR DDRD
#define CTRL_PORT PORTD

#define LED_HEAT 0
#define LED_FILL 1
#define LED_PUMP 2
#define LED_FILL_AND_HEAT 3
#define LED_AUTO 4
#define LED_PORT PORTC
#define LED_DDR DDRC

#define WATER_SENSOR 4
#define WATER_SENSOR_PIN PIND

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "class_RTOS.h"
#include "class_BUTTONS.h"

typedef struct {
	uint8_t H;
	uint8_t M;
	uint8_t S;
} TIME_STRUCT;

static struct {
	unsigned FULL_TANK: 1;
	unsigned HEAT: 1;
	unsigned FILLING: 1;
	unsigned PUMP: 1;
	unsigned FILL_AND_HEAT: 1;
	unsigned AUTO: 1;
} flags;

uchar blinks;
TIME_STRUCT TIME;
uchar EEMEM E_HEAT_FLAG = false;

void reset_time(void)
{
	TIME.H = 0;
	TIME.M = 0;
	TIME.S = 0;
}

static void init_timer2(void) 
{
	ASSR = (1<<AS2);					// Включаем асинхронный режим
	TCNT2 = 0;
	TCCR2 = (1<<CS22) | (1<<CS20);		// Предделитель на 128 на 32768 даст 256 тиков в секунду
	//TCCR2 = (1<<CS20);				// Для отладки (без предделителя)
	while ( ASSR & (1<<TCN2UB | 1<<OCR2UB | 1<<TCR2UB) ) {	// ждем пока есть хоть один флаговый бит
		_delay_ms(100);
	}
	TIMSK |= (1<<TOIE2);			// Разрешаем прерывание по переполнению
}

static void BLINK(void)
{
	LED_PORT ^= blinks;
	RTOS::SetTimerTask(BLINK,500);
}

static void init_blink(void)
{
	LED_PORT = 0xff;
	while(LED_PORT) {
		_delay_ms(400);
		LED_PORT = LED_PORT << 1;
	}
}

static void save_state(void) 
{
	eeprom_write_byte(&E_HEAT_FLAG, flags.HEAT);	
}

static void HEAT_ON(void) 
{
	if (!flags.FULL_TANK) return;
	
	SetBit(CTRL_PORT,CTRL_HEAT);
	ClearBit(LED_PORT,LED_HEAT);
	if (!flags.HEAT) {
		RTOS::SetTask(save_state);
	}
	flags.HEAT = true;

}

static void HEAT_OFF(void) 
{
	ClearBit(CTRL_PORT,CTRL_HEAT);
	SetBit(LED_PORT,LED_HEAT);
	if (flags.HEAT) {
		RTOS::SetTask(save_state);
	}
	flags.HEAT = false;
}

static void FILL_TIMER(void);
static void FILL_START(void) 
{
	if ( flags.FULL_TANK || flags.FILLING ) return;
	
	SetBit(CTRL_PORT,CTRL_FILL);
	ClearBit(LED_PORT,LED_FILL);
	SetBit(blinks,LED_FILL);
	flags.FILLING = true;
	RTOS::SetTask(FILL_TIMER);
}

static void FILL_STOP(void) 
{
	ClearBit(CTRL_PORT,CTRL_FILL);
	ClearBit(blinks,LED_FILL);
	flags.FILLING = false;
	if (flags.AUTO) {
		ClearBit(LED_PORT,LED_AUTO);
		ClearBit(blinks,LED_AUTO);
	}
}

static void FILL_TIMER(void)
{
	static uchar timer;
	if (!flags.FILLING) {
		timer = 0;
		return;
	}
	RTOS::SetTimerTask(FILL_TIMER, 1000);
	if (!flags.FULL_TANK) return;
	timer++;
	if (timer > DELAY_FILLING) {
		timer = 0;
		RTOS::SetTask(FILL_STOP);
	}
}

static void PUMP_TIMER(void);
static void PUMP_ON(void) 
{
	SetBit(CTRL_PORT,CTRL_PUMP);
	ClearBit(LED_PORT,LED_PUMP);
	flags.PUMP = true;
	RTOS::SetTask(PUMP_TIMER);
}

static void PUMP_OFF(void) 
{
	ClearBit(CTRL_PORT,CTRL_PUMP);
	SetBit(LED_PORT,LED_PUMP);
	flags.PUMP = false;
}

static void PUMP_TIMER(void)
{
	static uchar timer;
	if (!flags.PUMP) {
		timer = 0;
		return;
	}
	RTOS::SetTimerTask(PUMP_TIMER, 60000);
	if (flags.FULL_TANK) {
		timer = 0;
		return;
	}
	timer++;
	if (timer > DELAY_PUMP) {
		timer = 0;
		RTOS::SetTask(PUMP_OFF);
	}
}

static void FILL_AND_HEAT_TIMER(void);
static void FILL_AND_HEAT_START(void) 
{
	SetBit(blinks,LED_FILL_AND_HEAT);
	flags.FILL_AND_HEAT = true;
	RTOS::SetTask(FILL_START);
	RTOS::SetTask(FILL_AND_HEAT_TIMER);
}

static void FILL_AND_HEAT_STOP(void) 
{
	SetBit(LED_PORT,LED_FILL_AND_HEAT);
	ClearBit(blinks,LED_FILL_AND_HEAT);
	flags.FILL_AND_HEAT = false;
	RTOS::SetTask(FILL_STOP);
}

static void FILL_AND_HEAT_TIMER(void) 
{
	if (!flags.FILL_AND_HEAT) return;
	RTOS::SetTimerTask(FILL_AND_HEAT_TIMER,100);
	if (flags.FULL_TANK && !flags.FILLING) {
		RTOS::SetTask(FILL_AND_HEAT_STOP);
		RTOS::SetTask(HEAT_ON);
		RTOS::SetTask(PUMP_ON);
		return;
	}
	if (!flags.FILLING) RTOS::SetTask(FILL_AND_HEAT_STOP);
}

static void AUTO_ON(void) 
{
	reset_time();
	ClearBit(LED_PORT,LED_AUTO);
	flags.AUTO = true;	
}

static void AUTO_OFF(void) 
{
	SetBit(LED_PORT,LED_AUTO);
	ClearBit(blinks,LED_AUTO);
	flags.AUTO = false;
}

static void AUTO_EVENT(void)
{
	if (flags.FULL_TANK && !flags.FILLING) {
		ClearBit(blinks,LED_AUTO);
		ClearBit(LED_PORT,LED_AUTO);
		RTOS::SetTask(HEAT_ON);
		RTOS::SetTask(PUMP_ON);
		return;
	}
	if (!flags.AUTO) {
		RTOS::SetTask(FILL_STOP);
		return;		
	}
	if (!flags.FILLING) return;
	RTOS::SetTimerTask(AUTO_EVENT,100);
}

static void Check_Auto(void)
{
	if (!flags.AUTO) return;
	
	const uchar FILL_TIME_H = 24 - HOURS_BEFORE;
	 
	if ((TIME.H == FILL_TIME_H) && (!TIME.M)) {
		if (!flags.FULL_TANK) {
			SetBit(blinks,LED_AUTO);
			RTOS::SetTask(FILL_START);
		}
		RTOS::SetTask(PUMP_ON);
		return;
	}
	
	if ((!TIME.H) && (!TIME.M)) { // Всегда в 00:00
		SetBit(blinks,LED_AUTO);
		RTOS::SetTask(FILL_START);
		RTOS::SetTask(AUTO_EVENT);
	}
}

static void TICK(void)
{
	TIME.S++;
	if (TIME.S == 60) {
		TIME.M++;
		TIME.S = 0;
		RTOS::SetTask(Check_Auto);
		if (TIME.M == 60) {
			TIME.H++;
			TIME.M = 0;
			if (TIME.H == 24) {
				TIME.H = 0;
			}
		}
	}
}

static void Check_Water_Level(void) 
{
	RTOS::SetTimerTask(Check_Water_Level,300);
	if (BitIsSet(WATER_SENSOR_PIN,WATER_SENSOR)) {
		flags.FULL_TANK = true;
	} else {
		flags.FULL_TANK = false;
		RTOS::SetTask(HEAT_OFF);
	}
	
	if (!flags.FILLING) {
		if(flags.FULL_TANK) {
			ClearBit(LED_PORT,LED_FILL);	
		} else {
			SetBit(LED_PORT,LED_FILL);
		}
	}
}

static void CheckButtons(void) {
	RTOS::SetTimerTask(CheckButtons,75);
	Buttons::check();
	
	switch (Buttons::but_pressed)
	{
		case Buttons::none:
			return;
		case Buttons::HEAT:
			flags.HEAT ? RTOS::SetTask(HEAT_OFF) : RTOS::SetTask(HEAT_ON);
			break;
		case Buttons::FILL:
			flags.FILLING ? RTOS::SetTask(FILL_STOP) : RTOS::SetTask(FILL_START);
			break;
		case Buttons::PUMP:
			flags.PUMP ? RTOS::SetTask(PUMP_OFF) : RTOS::SetTask(PUMP_ON);
			break;
		case Buttons::F_AND_H:
			flags.FILL_AND_HEAT ? RTOS::SetTask(FILL_AND_HEAT_STOP) : RTOS::SetTask(FILL_AND_HEAT_START);
			break;
		case Buttons::AUTO:
			flags.AUTO ? RTOS::SetTask(AUTO_OFF) : RTOS::SetTask(AUTO_ON);
			break;
	}
	
}

static void restore_state(void)
{
	Check_Water_Level();
	if (flags.FULL_TANK) {
		if(eeprom_read_byte(&E_HEAT_FLAG)) {
			HEAT_ON();
			PUMP_ON();
		}
	} else {
		save_state();
	}
}

ISR(TIMER2_OVF_vect)
{
	RTOS::SetTask(TICK);
}

int main(void)
{
	LED_DDR |= (1<<LED_HEAT)|(1<<LED_FILL)|(1<<LED_PUMP)|(1<<LED_FILL_AND_HEAT)|(1<<LED_AUTO);
	init_blink();
	init_timer2();
	reset_time();
	LED_PORT = LED_DDR;
	CTRL_DDR |= (1<<CTRL_HEAT)|(1<<CTRL_FILL)|(1<<CTRL_PUMP);
	restore_state();
	
	RTOS::Init();
	RTOS::SetTask(Check_Water_Level);
	RTOS::SetTask(CheckButtons);
	RTOS::SetTask(BLINK);
	RTOS::Run();
    while (1)
    {
	    RTOS::TaskManager();
    }
}

