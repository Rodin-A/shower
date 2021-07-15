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
		TPTR GoToTask; 		// ��������� ��������
		unsigned int Time;	// �������� � ��
	};
protected:
private:
	volatile static TPTR _TaskQueue[];	// ������� ����������
	volatile static Timer _MainTimer[];	// ������� ��������

//functions
public:
	static void Init (void);
	static void Run (void);
	// ������� ��������� ������ � �������. ������������ �������� - ��������� �� �������
	static void SetTask(TPTR TS);
	//������� ��������� ������ �� �������. ������������ ��������� - ��������� �� �������,
	// ����� �������� � ����� ���������� �������.
	static void SetTimerTask(TPTR TS, unsigned int NewTime);
	//��������� �����. �������� �� ������� ������ � ���������� �� ����������.
	static void TaskManager(void);
	//������ �������� ����. ������ ���������� �� ���������� ��� � 1��.
	static void TimerService(void);
protected:
private:
	//������ ��������� - ������� ����.
	static void Idle(void) {};


}; //RTOS

#endif //__CLASS_RTOS_H__
