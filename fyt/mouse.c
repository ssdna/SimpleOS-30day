#include "bootpack.h"

void enable_mouse(struct MOUSE_DEC *mdec)
{
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	mdec->phase = 0;
	return;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data)
{
	if(mdec->phase == 0) {
		/* 等待鼠标的0xfa的状态 */
		if(data == 0xfa) {
			mdec->phase = 1;
		}
		return 0;
	}
	if (mdec->phase == 1) {
		// 解决鼠标断线问题，检查第一字节的数据
		if((data & 0xc8) == 0x08) {
			mdec->buf[0] = data;
			mdec->phase = 2;
		}
		return 0;
	}
	if (mdec->phase == 2) {
		mdec->buf[1] = data;
		mdec->phase = 3;
		return 0;
	}
	if (mdec->phase == 3) {
		mdec->buf[2] = data;
		mdec->phase = 1;

		// decode
		mdec->btn 	= mdec->buf[0] & 0x07;
		mdec->x 	= mdec->buf[1];
		mdec->y 	= mdec->buf[2];
		if((mdec->buf[0] & 0x10) != 0) {
			mdec->x |= 0xffffff00;
		}
		if((mdec->buf[0] & 0x20) != 0) {
			mdec->y |= 0xffffff00;
		}
		mdec->y 	= -mdec->y; 			/*y方向与画面方向相反*/
		return 1;
	}
	return -1;
}