#include "bootpack.h"

struct TIMERCTL timerctl;

void init_pit(void)
{
	int i;
	io_out8(PIT_CTRL, 0x34);
	/*11932 = 0x9c2e*/
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
	timerctl.count = 0;
	for (i = 0; i < MAX_TIMER; i++)
	{
		timerctl.timer[i].flags = 0;
	}
	return;
}


// void settimer(unsigned int timeout, struct FIFO8 *fifo, unsigned char data)
// {
// 	int eflags;
// 	eflags = io_load_eflags();
// 	io_cli();
// 	timerctl.timeout = timeout;
// 	timerctl.fifo = fifo;
// 	timerctl.data = data;
// 	io_store_eflags(eflags);
// 	return;
// }

struct TIMER *timer_alloc(void)
{
	int i;
	for (i = 0; i < MAX_TIMER; i++)
	{
		if(timerctl.timer[i].flags == 0)
		{
			timerctl.timer[i].flags = TIMER_FLAGS_ALLOC;
			return &timerctl.timer[i];
		}
	}
	return 0; 			/*没找到*/
}

void timer_free(struct TIMER *timer)
{
	timer->flags = 0;
	return;
}

void timer_init(struct TIMER *timer, struct FIFO8 *fifo, unsigned char data)
{
	timer->fifo = fifo;
	timer->data = data;
	return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout)
{
	timer->timeout = timeout;
	timer->flags = TIMER_FLAGS_USING;
	return;
}


void inthandler20(int *esp)
{
	int i;
	io_out8(PIC0_OCW2, 0x60); 			/*把IRQ-00信号接收结束的信息通知给PIC*/
	timerctl.count++;
	for (i = 0; i < MAX_TIMER; i++)
	{
		if (timerctl.timer[i].flags == TIMER_FLAGS_USING)
		{
			timerctl.timer[i].timeout--;
			if (timerctl.timer[i].timeout == 0)
			{
				timerctl.timer[i].flags = TIMER_FLAGS_ALLOC;
				fifo8_put(timerctl.timer[i].fifo, timerctl.timer[i].data);
			}
		}
	}
	return;
}
