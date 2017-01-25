#include "bootpack.h"
/*KeyBoard Controller*/
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

void init_keyboard(void)
{
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
	return;
}