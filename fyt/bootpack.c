#include "bootpack.h"
#include <stdio.h>


extern struct FIFO8 keyfifo, mousefifo;


void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char s[40], mcursor[256], keybuf[32], mousebuf[128];
	int mx, my, i;
	struct MOUSE_DEC mdec;
	// 内存测试
	unsigned int memtotal, count = 0;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	// sheet
	struct SHTCTL *shtctl;
	struct SHEET *sht_back, *sht_mouse, *sht_win;
	unsigned char *buf_back, buf_mouse[256], *buf_win;

	init_gdtidt();
	init_pic();
	io_sti(); 			/* STI执行后，IF（中断许可标志位）变为1，CPU接收中断 */

	fifo8_init(&keyfifo, 32, keybuf);
	fifo8_init(&mousefifo, 128, mousebuf);

	io_out8(PIC0_IMR, 0xf9); 		/*11111001*/
	io_out8(PIC1_IMR, 0xef); 		/*11101111*/

	init_keyboard();
	/*memory*/
	memtotal = memtest_sub(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000);
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

	init_palette();
	/*sheet*/
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	sht_back = sheet_alloc(shtctl);
	sht_mouse = sheet_alloc(shtctl);
	sht_win = sheet_alloc(shtctl);
	buf_back = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	buf_win = (unsigned char *) memman_alloc_4k(memman, 160 * 80);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); 	/*没有透明色*/
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99); 	/*透明色号99*/
	sheet_setbuf(sht_win, buf_win, 160, 80, -1);
	init_screen8(buf_back, binfo->scrnx, binfo->scrny);
	init_mouse_cursor8(buf_mouse, 99); 		/*背景色号99*/
	make_window8(buf_win, 160, 80, "fyt");
	// putfonts8_asc(buf_win, 160, 24, 28, COL8_00FF00, "fyt, I love you.");
	sheet_slide(sht_back, 0, 0);
	mx = (binfo->scrnx -16) / 2;
	my = (binfo->scrny -28 -16) /2;
	sheet_slide(sht_mouse, mx, my);
	sheet_slide(sht_win, 80, 72);
	sheet_updown(sht_back, 0);
	sheet_updown(sht_win, 1);
	sheet_updown(sht_mouse, 2);
	putfonts8_asc(sht_back, binfo->scrnx, 108, 168, COL8_00FF00, "fyt, I love you~");
	sprintf(s, "(%d,%d)", mx, my);
	putfonts8_asc(buf_back, binfo->scrnx, 16, 100, COL8_FF0000, s);
	sprintf(s, "Memory %dMB; free: %dKB.", memtotal/(1024*1024), memman_total(memman) / 1024);
	putfonts8_asc(buf_back, binfo->scrnx, 0, 64, COL8_FFFFFF, s);
	sheet_refresh(sht_back, 0, 0, binfo->scrnx, binfo->scrny);

/*

	putfonts8_asc(binfo->vram, binfo->scrnx, 108, 108, COL8_00FF00, "fyt, I love you~");

	memtotal = memtest_sub(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000);
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

	sprintf(s, "Memory %dMB; free: %dKB.", memtotal/(1024*1024), memman_total(memman) / 1024);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 64, COL8_FFFFFF, s);

	//mouse
	mx = (binfo->scrnx -16) / 2;
	my = (binfo->scrny -28 -16) /2;
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf(s, "(%d,%d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 16, 100, COL8_FF0000, s);
*/
	enable_mouse(&mdec);

	while(1)
	{
		count++;
		sprintf(s, "%010d", count);
		boxfill8(buf_win, 160, COL8_C6C6C6, 40, 28, 119, 43);
		putfonts8_asc(buf_win, 160, 40, 28, COL8_FFFF00, s);
		sheet_refresh(sht_win, 40, 28, 120, 44);

		io_cli();
		if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
			io_stihlt();
		} else {
			if(fifo8_status(&keyfifo) != 0) {
				i = fifo8_get(&keyfifo);
				io_sti();
				sprintf(s, "%02x", i);
				boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 32, 15, 47);
				putfonts8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
				sheet_refresh(sht_back, 0, 32, 16, 48);
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
					boxfill8(buf_back, binfo->scrnx, COL8_008484, 32, 32, 32+15*8-1, 47);
					putfonts8_asc(buf_back, binfo->scrnx, 32, 32, COL8_FFFFFF, s);
					sheet_refresh(sht_back, 32, 32, 32+15*8, 48);
					/* 鼠标的移动 */
					// boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx+15, my+15); 	/*隐藏鼠标*/
					mx += mdec.x;
					my += mdec.y;
					if (mx < 0) {
						mx = 0;
					}
					if (my <0) {
						my = 0;
					}
					if (mx > binfo->scrnx - 1) {
						mx = binfo->scrnx - 1;
					}
					if (my > binfo->scrny - 1) {
						my = binfo->scrny - 1;
					}
					sprintf(s, "(%3d, %3d)", mx, my);
					boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 0, 79, 15); 			/*隐藏坐标*/
					putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);				/*显示坐标*/
					// putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);		/*描绘鼠标*/
					sheet_refresh(sht_back, 0, 0, 80, 16);
					sheet_slide(sht_mouse, mx, my); 		/*包含sheet_refresh*/
				}
			}
		}
	}
}

