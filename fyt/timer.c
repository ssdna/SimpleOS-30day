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
	timerctl.next = 0xffffffff;
	timerctl.useing = 0;
	for (i = 0; i < MAX_TIMER; i++)
	{
		timerctl.timers0[i].flags = 0;
	}
	return;
}

struct TIMER *timer_alloc(void)
{
	int i;
	for (i = 0; i < MAX_TIMER; i++)
	{
		if(timerctl.timers0[i].flags == 0)
		{
			timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
			return &timerctl.timers0[i];
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
	int e, i, j;
	timer->timeout = timeout + timerctl.count;
	timer->flags = TIMER_FLAGS_USING;
	e = io_load_eflags();
	io_cli();
	/*搜索注册位置*/
	for (i = 0; i < timerctl.useing; i++)	
	{
		if (timerctl.timers[i]->timeout >= timer->timeout)
		{
			break;
		}
	}
	/*i号之后的全部后移一位*/
	for (j = timerctl.useing; j > i; j--)
	{
		timerctl.timers[j] = timerctl.timers[j-1];
	}
	timerctl.useing++;
	/*插入到空位上*/
	timerctl.timers[i] = timer;
	timerctl.next = timerctl.timers[0]->timeout;
	io_store_eflags(e);
	return;
}


void inthandler20(int *esp)
{
	int i, j;
	io_out8(PIC0_OCW2, 0x60); 			/*把IRQ-00信号接收结束的信息通知给PIC*/
	timerctl.count++;
	if (timerctl.next > timerctl.count)
	{
		/*还不到下一个时刻*/
		return;
	}
	for (i = 0; i < timerctl.useing; i++)
	{
		/*timers的定时器都处于动作中，所以不确定flags*/
		if(timerctl.timers[i]->timeout > timerctl.count)
		{
			break;
		}
		/*超时*/
		timerctl.timers[i]->flags = TIMER_FLAGS_ALLOC;
		fifo8_put(timerctl.timers[i]->fifo, timerctl.timers[i]->data);
	}
	/*正好有i个定时器超时了。其余的进行移位*/
	timerctl.useing -= i;
	for (j = 0; j < timerctl.useing; j++)
	{
		timerctl.timers[j] = timerctl.timers[i + j];
	}
	if (timerctl.useing > 0)
	{
		timerctl.next = timerctl.timers[0]->timeout;
	} else {
		timerctl.next = 0xffffffff;
	}
	return;
}
