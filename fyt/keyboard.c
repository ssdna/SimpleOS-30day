#include "bootpack.h"
/*KeyBoard Controller*/

struct FIFO32 *keyfifo;
int keydata0;

void wait_KBC_sendready(void)
{
	for (;;)
	{
		// 如果从设备号0x0064处获得的数据中倒数第二位为0，表示已准备好
		if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREDEAY) == 0) {
			break;
		}
	}
}

void init_keyboard(struct FIFO32 *fifo, int data0)
{
	/*将fifo缓冲区中的信息保存到全局变量里*/
	keyfifo = fifo;
	keydata0 = data0;
	/*键盘控制器的初始化*/
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
	return;
}

// 键盘中断
void inthandler21(int *exp)
{
	int data;
	io_out8(PIC0_OCW2, 0x61); 		/* 通知PIC: "IRQ-01已受理完毕" */
	data = io_in8(PORT_KEYDAT);

	fifo32_put(keyfifo, data + keydata0);

	return;
}