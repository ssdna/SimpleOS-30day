#include "bootpack.h"
#include <stdio.h>


// extern struct FIFO8 keyfifo, mousefifo;
extern struct TIMERCTL timerctl;

void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);

static char keytable[0x54] = {
	0,0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
	'-', '^',  0 ,  0 , 'Q', 'W', 'E', 'R', 'T', 'Y', 'U',
	'I', 'O', 'P', '[', ']',  0 ,  0 , 'A', 'S', 'D', 'F', 
	'G', 'H', 'J', 'K', 'L', ';', '\'', 0 ,  0 , ']', 'Z', 
	'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/',  0 , '*', 
	 0 , ' ',  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 
	 0 ,  0 ,  0 ,  0 , '7', '8', '9', '-', '4', '5', '6', 
	'+', '1', '2', '3', '0', '.'
};

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char s[40];
	int mx, my, i;
	struct MOUSE_DEC mdec;
	// 内存测试
	unsigned int memtotal;
	int cnt = 0;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	// sheet
	struct SHTCTL *shtctl;
	struct SHEET *sht_back, *sht_mouse, *sht_win;
	unsigned char *buf_back, buf_mouse[256], *buf_win;
	// timer
	struct FIFO32 fifo;
	int fifobuf[128];
	struct TIMER *timer, *timer2, *timer3;
	char timerbuf[8];


	init_gdtidt();
	init_pic();
	io_sti(); 			/* STI执行后，IF（中断许可标志位）变为1，CPU接收中断 */

	init_pit();
	
	fifo32_init(&fifo, 128, fifobuf);
	init_keyboard(&fifo, 256);
	enable_mouse(&fifo, 512, &mdec);

	io_out8(PIC0_IMR, 0xf8); 		/*11111000 PIT和PIC1和键盘设置为许可*/
	io_out8(PIC1_IMR, 0xef); 		/*11101111 鼠标设置为许可*/

	timer = timer_alloc();
	timer_init(timer, &fifo, 10);
	timer_settime(timer, 1000);

	timer2 = timer_alloc();
	timer_init(timer2, &fifo, 3);
	timer_settime(timer2, 300);
	//
	timer3 = timer_alloc();
	timer_init(timer3, &fifo, 1);
	timer_settime(timer3, 50);

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

	// 按键显示字符 letter
	int cursor_x, cursor_c;
	make_textbox8(sht_win, 8, 46, 144, 16, COL8_FFFFFF);
	cursor_x = 8;
	cursor_c = COL8_FFFFFF;
	
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
	

	// 测试中断速度
	int isTest = 1;

	for(;;)
	{
		// boxfill8(buf_win,)
		// if (isTest == 1)
		{
			/* code */
			cnt++; 		/*测试定时器性能*/
		}
		// sprintf(s, "%010d", cnt);
		// sprintf(s, "%010d", timerctl.count);
		// putfonts8_asc_sht(sht_win, 40, 28, COL8_FFFFFF, COL8_C6C6C6, s, 10);

		io_cli();
		if (fifo32_status(&fifo) == 0) {
			// if (isTest == 1)
			// {
				io_sti();
			// } else {
				// io_stihlt();
			// }
		} else {
			i = fifo32_get(&fifo);
			io_sti();

			if(256 <= i && i <= 511) {
				/*键盘数据*/
				sprintf(s, "%02X", i - 256);
				putfonts8_asc_sht(sht_back, 0, 32, COL8_FFFFFF, COL8_008484, s, 2);
				if (i < 256 + 0x54)
				{
					if (keytable[i - 256] != 0 && cursor_x < 144)
					{
						s[0] = keytable[i - 256];
						s[1] = 0;
						putfonts8_asc_sht(sht_win, cursor_x, 46, COL8_000000, COL8_FFFFFF, s, 1);
						cursor_x += 8;
					}
				}
				if (i == 256 + 0x0e && cursor_x > 8)
				{
					/* 退格键 */
					putfonts8_asc_sht(sht_win, cursor_x, 46, COL8_000000, COL8_FFFFFF, " ", 1);
					cursor_x -= 8;
				}
				/*光标再显示*/
				boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 46, cursor_x + 7, 61);
				sheet_refresh(sht_win, cursor_x, 46, cursor_x + 8, 62);
			} else if(512 <= i && i <= 767) {
				/*鼠标数据*/
				if (mouse_decode(&mdec, i - 512) != 0) {
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
					putfonts8_asc_sht(sht_back, 32, 32, COL8_FFFFFF, COL8_008484, s, 15);
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
					putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
					sheet_slide(sht_mouse, mx, my); 		/*包含sheet_refresh*/
					if((mdec.btn & 0x01) != 0) {
						/*按下左键，移动sht_win*/
						sheet_slide(sht_win, mx - 80, my - 8);
					}
				}
			} else if(i == 10) {
				/*10秒定时器*/
				putfonts8_asc_sht(sht_back, 0, 80, COL8_FFFFFF, COL8_008484, "10 [sec]", 8);
				sprintf(s, "%010d", cnt);
				putfonts8_asc_sht(sht_win, 40, 28, COL8_FFFFFF, COL8_008484, s, 10);
				// isTest = 0;
			} else if(i == 3)
			{
				putfonts8_asc_sht(sht_back, 0, 80+1*16, COL8_FFFFFF, COL8_008484, "3 [sec]", 7);
				cnt = 0;
			} else if(i <= 1){
				/*光标用定时器*/
				if (i != 0)
				{
					timer_init(timer3, &fifo, 0);
					cursor_c = COL8_000000;
				} else {
					timer_init(timer3, &fifo, 1);
					cursor_c = COL8_FFFFFF;
				}
				timer_settime(timer3, 50);
				boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 46, cursor_x + 7, 61);
				sheet_refresh(sht_win, cursor_x, 46, cursor_x+8, 62);
			}
		}
	}
}

void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l)
{
	boxfill8(sht->buf, sht->bxsize, b, x, y, x+l*8-1, y + 15);
	putfonts8_asc(sht->buf, sht->bxsize, x, y, c, s);
	sheet_refresh(sht, x, y, x+l*8, y + 16);
	return;
}

void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c)
{
	int x1 = x0 + sx, y1 = y0 + sy;
	boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 2, y0 - 3, x1 + 1, y0 - 3);
	boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 3, y0 - 3, x0 - 3, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x0 - 3, y1 + 2, x1 + 1, y1 + 2);
	boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x1 + 2, y0 - 3, x1 + 2, y1 + 2);
	boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 1, y0 - 2, x1 + 0, y0 - 2);
	boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 2, y0 - 2, x1 - 2, y1 + 0);
	boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x0 - 2, y1 + 1, x1 + 0, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x1 + 1, y0 - 2, x1 + 1, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, c,  		 x0 - 1, y0 - 1, x1 + 0, y1 + 0);
	sheet_refresh(sht, x0, y0, sx, sy);
	return;
}

