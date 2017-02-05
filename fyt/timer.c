#include "bootpack.h"

struct TIMERCTL timerctl;

void init_pit(void)
{
	int i;
	struct TIMER *t;
	io_out8(PIT_CTRL, 0x34);
	/*11932 = 0x9c2e*/
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
	timerctl.count = 0;
	for (i = 0; i < MAX_TIMER; ++i)
	{
		timerctl.timers0[i].flags = 0; 		/*没有使用*/
	}
	t = timer_alloc(); 				/*引入哨兵*/
	t->timeout = 0xffffffff;
	t->flags = TIMER_FLAGS_USING;
	t->next = 0; 					/*哨兵插入末尾*/
	timerctl.t0 = t;
	timerctl.next = 0xffffffff; 	/*设置下一个超时时刻为哨兵*/
	
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

void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data)
{
	timer->fifo = fifo;
	timer->data = data;
	return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout)
{
	int e;
	struct TIMER *t, *s;

	timer->timeout = timeout + timerctl.count;
	timer->flags = TIMER_FLAGS_USING;
	e = io_load_eflags();
	io_cli();

	t = timerctl.t0;
	if (timer->timeout <= t->timeout)
	{
		/*插入最前面的情况下*/
		timerctl.t0 = timer;
		timer->next = t; 		/*下面是t*/
		timerctl.next = timer->timeout;
		io_store_eflags(e);
		return;
	}
	/*搜索插入位置*/
	for (;;)	
	{
		s = t;
		t = t->next;
		if (t == 0)
		{
			break; 		/*最后面*/
		}

		if (timer->timeout <= t->timeout)
		{
			/*插入到s和t之间时*/
			s->next = timer; 		/*s的下一个是timer*/
			timer->next = t; 		/*timer的下一个是t*/
			io_store_eflags(e);
			return;
		}
	}
	return;
}


void inthandler20(int *esp)
{
	int i;
	struct TIMER *timer;
	io_out8(PIC0_OCW2, 0x60);
	timerctl.count++;
	if (timerctl.next > timerctl.count)
	{
		/*还不到下一个时刻*/
		return;
	}
	timer = timerctl.t0;
	for (;;)
	{
		/*timers的定时器都处于动作中，所以不确定flags*/
		if(timer->timeout > timerctl.count)
		{
			break;
		}
		/*超时*/
		timer->flags = TIMER_FLAGS_ALLOC;
		fifo32_put(timer->fifo, timer->data);
		timer = timer->next; 		/*下一定时器的地址赋给timer*/
	}

	/*timerctl.next的设定*/
	timerctl.t0 = timer;
	timerctl.next = timerctl.t0->timeout;
	
	return;
}
