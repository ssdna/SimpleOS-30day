#include "bootpack.h"
#include <stdio.h>

extern struct FIFO8 keyfifo, mousefifo;


void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char s[40], mcursor[256], keybuf[32], mousebuf[128];
	int mx, my, i;
	struct MOUSE_DEC mdec;

	init_gdtidt();
	init_pic();
	io_sti(); 			/* STI执行后，IF（中断许可标志位）变为1，CPU接收中断 */

	io_out8(PIC0_IMR, 0xf9); 		/*11111001*/
	io_out8(PIC1_IMR, 0xef); 		/*11101111*/

	fifo8_init(&keyfifo, 32, keybuf);
	fifo8_init(&mousefifo, 128, mousebuf);

	init_keyboard();

	init_palette();
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);

	putfonts8_asc(binfo->vram, binfo->scrnx, 108, 108, COL8_00FF00, "fyt, I love you~");
	sprintf(s, "scrnx = %d, scrny = %d", binfo->scrnx, binfo->scrny);

	//mouse
	mx = (binfo->scrnx -16) / 2;
	my = (binfo->scrny -28 -16) /2;
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf(s, "(%d,%d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 16, 100, COL8_FF0000, s);

	enable_mouse(&mdec);

	while(1)
	{
		io_cli();
		if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
			io_stihlt();
		} else {
			if(fifo8_status(&keyfifo) != 0) {
				i = fifo8_get(&keyfifo);
				io_sti();
				sprintf(s, "%02x", i);
				boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 32, 15, 47);
				putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
			} else if(fifo8_status(&mousefifo) != 0) {
				i = fifo8_get(&mousefifo);
				io_sti();
				if (mouse_decode(&mdec, i) != 0) {
					/* 鼠标的3个字节都齐了，显示出来 */
					sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					if((mdec.btn & 0x01) != 0) {
						s[1] = 'L';
					}
					if((mdec.btn & 0x02) != 0) {
						s[3] = 'R';
					}
					if((mdec.btn & 0x04) != 0) {
						s[2] = 'C';
					}
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 32, 32+15*8-1, 47);
					putfonts8_asc(binfo->vram, binfo->scrnx, 32, 32, COL8_FFFFFF, s);
					/* 鼠标的移动 */
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx+15, my+15); 	/*隐藏鼠标*/
					mx += mdec.x;
					my += mdec.y;
					if (mx < 0) {
						mx = 0;
					}
					if (my <0) {
						my = 0;
					}
					if (mx > binfo->scrnx - 16) {
						mx = binfo->scrnx - 16;
					}
					if (my > binfo->scrny - 16) {
						my = binfo->scrny - 16;
					}
					sprintf(s, "(%3d, %3d)", mx, my);
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15); 			/*隐藏坐标*/
					putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);				/*显示坐标*/
					putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);		/*描绘鼠标*/
				}
			}
		}
	}
}

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
