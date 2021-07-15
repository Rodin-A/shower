/* 
* class_RTOS.cpp
*
* Created: 26.03.2020 14:05:32
* Author: Rodin
*/


#include "class_RTOS.h"

volatile RTOS::TPTR RTOS::_TaskQueue[TaskQueueSize+1];
volatile RTOS::Timer RTOS::_MainTimer[MainTimerQueueSize+1];

//RTOS Interrupt
ISR(RTOS_ISR)
{
	TCNT0 = TimerDivider;
	RTOS::TimerService();
}


void RTOS::Run(void)
{
	TCCR0 = (1<<CS01);						// Freq = CK/8 - ���������� ����� � ������������
	TCNT0 = TimerDivider;					// ���������� ��������� �������� ���������
	//OCR2 = TimerDivider; 					// ���������� �������� � ������� ���������
	TIMSK |= (1<<TOIE0);					// ��������� ���������� RTOS - ������ ��
	Enable_Interrupt
}

void RTOS::Init(void)
{
	uint8_t	index;

	for(index=0;index!=TaskQueueSize+1;index++) {		// �� ��� ������� ���������� Idle
		_TaskQueue[index] = Idle;
	}

	for(index=0;index!=MainTimerQueueSize+1;index++) {	// �������� ��� �������.
		_MainTimer[index].GoToTask = Idle;
		_MainTimer[index].Time = 0;
	}
}

void RTOS::SetTask(TPTR TS)
{
	
	uint8_t	index = 0;
	uint8_t	nointerrupted = 0;

	if (STATUS_REG & (1<<Interrupt_Flag)) {  // ���� ���������� ���������, �� ��������� ��.
		Disable_Interrupt
		nointerrupted = 1;					// � ������ ����, ��� �� �� � ����������.
	}

	while(_TaskQueue[index]!=Idle) {		// ����������� ������� ����� �� ������� ��������� ������
											// � ��������� Idle - ����� �������.
		index++;
		if (index==TaskQueueSize+1) { 		// ���� ������� ����������� �� ������� �� ������ ��������
			if (nointerrupted) Enable_Interrupt 	// ���� �� �� � ����������, �� ��������� ����������
			return;									// ������ ������� ���������� ��� ������ - ������� �����������. ���� �����.
		}
	}
													// ���� ����� ��������� �����, ��
	_TaskQueue[index] = TS;							// ���������� � ������� ������
	if (nointerrupted) Enable_Interrupt				// � �������� ���������� ���� �� � ����������� ����������.
}

void RTOS::SetTimerTask(TPTR TS, unsigned int NewTime)
{
	uint8_t	index = 0;
	uint8_t nointerrupted = 0;

	if (STATUS_REG & (1<<Interrupt_Flag)) { 			// �������� ������� ����������, ���������� ������� ����
		Disable_Interrupt
		nointerrupted = 1;
	}


	for(index=0;index!=MainTimerQueueSize+1;++index) {	//����������� ������� ��������
		if(_MainTimer[index].GoToTask == TS) {			// ���� ��� ���� ������ � ����� �������
			_MainTimer[index].Time = NewTime;			// �������������� �� ��������
			if (nointerrupted) Enable_Interrupt			// ��������� ���������� ���� �� ���� ���������.
			return;										// �������. ������ ��� ��� �������� ��������. ���� �����
		}
	}
	

	for(index=0;index!=MainTimerQueueSize+1;++index) {	// ���� �� ������� ������� ������, �� ���� ����� ������
		if (_MainTimer[index].GoToTask == Idle) {
			_MainTimer[index].GoToTask = TS;		// ��������� ���� �������� ������
			_MainTimer[index].Time = NewTime;		// � ���� �������� �������
			if (nointerrupted) Enable_Interrupt		// ��������� ����������
			return;									// �����.
		}
		
	}												// ��� ����� ������� return c ����� ������ - ��� ��������� ��������
}

void RTOS::TaskManager(void)
{
	uint8_t	index=0;
	TPTR GoToTask = Idle;

	Disable_Interrupt				// ��������� ����������!!!
	GoToTask = _TaskQueue[0];		// ������� ������ �������� �� �������

	if (GoToTask==Idle) {			// ���� ��� �����
		Enable_Interrupt			// ��������� ����������
		(Idle)(); 					// ��������� �� ��������� ������� �����
	} else {
		for(index=0;index!=TaskQueueSize;index++) {		// � ��������� ������ �������� ��� �������
			_TaskQueue[index]=_TaskQueue[index+1];
		}
		
		_TaskQueue[TaskQueueSize]= Idle;				// � ��������� ������ ������ �������

		Enable_Interrupt			// ��������� ����������
		(GoToTask)();				// ��������� � ������
	}
}

void RTOS::TimerService(void)
{
	uint8_t index;

	for(index=0;index!=MainTimerQueueSize+1;index++) {		// ����������� ������� ��������
		if(_MainTimer[index].GoToTask == Idle) continue;	// ���� ����� �������� - ������� ��������� ��������

		if(_MainTimer[index].Time !=1) {					// ���� ������ �� ��������, �� ������� ��� ���.
			_MainTimer[index].Time--;						// ��������� ����� � ������ ���� �� �����.
		} else {
			SetTask(_MainTimer[index].GoToTask);			// ��������� �� ����? ������ � ������� ������
			_MainTimer[index].GoToTask = Idle;				// � � ������ ����� �������
		}
	}
}
