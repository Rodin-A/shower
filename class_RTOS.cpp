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
	TCCR0 = (1<<CS01);						// Freq = CK/8 - Установить режим и предделитель
	TCNT0 = TimerDivider;					// Установить начальное значение счётчиков
	//OCR2 = TimerDivider; 					// Установить значение в регистр сравнения
	TIMSK |= (1<<TOIE0);					// Разрешаем прерывание RTOS - запуск ОС
	Enable_Interrupt
}

void RTOS::Init(void)
{
	uint8_t	index;

	for(index=0;index!=TaskQueueSize+1;index++) {		// Во все позиции записываем Idle
		_TaskQueue[index] = Idle;
	}

	for(index=0;index!=MainTimerQueueSize+1;index++) {	// Обнуляем все таймеры.
		_MainTimer[index].GoToTask = Idle;
		_MainTimer[index].Time = 0;
	}
}

void RTOS::SetTask(TPTR TS)
{
	
	uint8_t	index = 0;
	uint8_t	nointerrupted = 0;

	if (STATUS_REG & (1<<Interrupt_Flag)) {  // Если прерывания разрешены, то запрещаем их.
		Disable_Interrupt
		nointerrupted = 1;					// И ставим флаг, что мы не в прерывании.
	}

	while(_TaskQueue[index]!=Idle) {		// Прочесываем очередь задач на предмет свободной ячейки
											// с значением Idle - конец очереди.
		index++;
		if (index==TaskQueueSize+1) { 		// Если очередь переполнена то выходим не солоно хлебавши
			if (nointerrupted) Enable_Interrupt 	// Если мы не в прерывании, то разрешаем прерывания
			return;									// Раньше функция возвращала код ошибки - очередь переполнена. Пока убрал.
		}
	}
													// Если нашли свободное место, то
	_TaskQueue[index] = TS;							// Записываем в очередь задачу
	if (nointerrupted) Enable_Interrupt				// И включаем прерывания если не в обработчике прерывания.
}

void RTOS::SetTimerTask(TPTR TS, unsigned int NewTime)
{
	uint8_t	index = 0;
	uint8_t nointerrupted = 0;

	if (STATUS_REG & (1<<Interrupt_Flag)) { 			// Проверка запрета прерывания, аналогично функции выше
		Disable_Interrupt
		nointerrupted = 1;
	}


	for(index=0;index!=MainTimerQueueSize+1;++index) {	//Прочесываем очередь таймеров
		if(_MainTimer[index].GoToTask == TS) {			// Если уже есть запись с таким адресом
			_MainTimer[index].Time = NewTime;			// Перезаписываем ей выдержку
			if (nointerrupted) Enable_Interrupt			// Разрешаем прерывания если не были запрещены.
			return;										// Выходим. Раньше был код успешной операции. Пока убрал
		}
	}
	

	for(index=0;index!=MainTimerQueueSize+1;++index) {	// Если не находим похожий таймер, то ищем любой пустой
		if (_MainTimer[index].GoToTask == Idle) {
			_MainTimer[index].GoToTask = TS;		// Заполняем поле перехода задачи
			_MainTimer[index].Time = NewTime;		// И поле выдержки времени
			if (nointerrupted) Enable_Interrupt		// Разрешаем прерывания
			return;									// Выход.
		}
		
	}												// тут можно сделать return c кодом ошибки - нет свободных таймеров
}

void RTOS::TaskManager(void)
{
	uint8_t	index=0;
	TPTR GoToTask = Idle;

	Disable_Interrupt				// Запрещаем прерывания!!!
	GoToTask = _TaskQueue[0];		// Хватаем первое значение из очереди

	if (GoToTask==Idle) {			// Если там пусто
		Enable_Interrupt			// Разрешаем прерывания
		(Idle)(); 					// Переходим на обработку пустого цикла
	} else {
		for(index=0;index!=TaskQueueSize;index++) {		// В противном случае сдвигаем всю очередь
			_TaskQueue[index]=_TaskQueue[index+1];
		}
		
		_TaskQueue[TaskQueueSize]= Idle;				// В последнюю запись пихаем затычку

		Enable_Interrupt			// Разрешаем прерывания
		(GoToTask)();				// Переходим к задаче
	}
}

void RTOS::TimerService(void)
{
	uint8_t index;

	for(index=0;index!=MainTimerQueueSize+1;index++) {		// Прочесываем очередь таймеров
		if(_MainTimer[index].GoToTask == Idle) continue;	// Если нашли пустышку - щелкаем следующую итерацию

		if(_MainTimer[index].Time !=1) {					// Если таймер не выщелкал, то щелкаем еще раз.
			_MainTimer[index].Time--;						// Уменьшаем число в ячейке если не конец.
		} else {
			SetTask(_MainTimer[index].GoToTask);			// Дощелкали до нуля? Пихаем в очередь задачу
			_MainTimer[index].GoToTask = Idle;				// А в ячейку пишем затычку
		}
	}
}
