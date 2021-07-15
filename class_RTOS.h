/* 
* class_RTOS.h
*
* Created: 26.03.2020 14:05:32
* Author: Rodin
*/


#ifndef __CLASS_RTOS_H__
#define __CLASS_RTOS_H__

#include <avr/io.h>
#include <avr/interrupt.h>

#define STATUS_REG 			SREG
#define Interrupt_Flag		SREG_I
#define Disable_Interrupt	cli();
#define Enable_Interrupt	sei();

//Clock Config
#define F_CPU 1000000UL

//System Timer Config
#define Prescaler	  		8
#define	TimerDivider  		(F_CPU/Prescaler/1000)		// 1 mS

//RTOS Config
#define RTOS_ISR  			TIMER0_OVF_vect
#define	TaskQueueSize		20
#define MainTimerQueueSize	15

class RTOS
{
//variables
public:
	typedef void (*TPTR)(void);
	struct Timer
	{
		TPTR GoToTask; 		// Указатель перехода
		unsigned int Time;	// Выдержка в мс
	};
protected:
private:
	volatile static TPTR _TaskQueue[];	// очередь указателей
	volatile static Timer _MainTimer[];	// Очередь таймеров

//functions
public:
	static void Init (void);
	static void Run (void);
	// Функция установки задачи в очередь. Передаваемый параметр - указатель на функцию
	static void SetTask(TPTR TS);
	//Функция установки задачи по таймеру. Передаваемые параметры - указатель на функцию,
	// Время выдержки в тиках системного таймера.
	static void SetTimerTask(TPTR TS, unsigned int NewTime);
	//Диспетчер задач. Выбирает из очереди задачи и отправляет на выполнение.
	static void TaskManager(void);
	//Служба таймеров ядра. Должна вызываться из прерывания раз в 1мс.
	static void TimerService(void);
protected:
private:
	//Пустая процедура - простой ядра.
	static void Idle(void) {};


}; //RTOS

#endif //__CLASS_RTOS_H__
