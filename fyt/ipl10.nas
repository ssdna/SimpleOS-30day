; hello-os
; TAB=4

CYLS	EQU		10

	ORG	0x7c00

;
	JMP	entry
	DB	0x90
	DB	"HELLOIPL"
	DW	512
	DB	1
	DW	1
	DB	2
	DW	224
	DW	2880
	DB	0xf0
	DW	9
	DW	18
	DW	2
	DD	0
	DD	2880
	DB	0,0,0x29
	DD	0xffffffff
	DB	"HELLO-OS   "
	DB	"FAT12   "
	RESB	18

;

entry:
	MOV	AX,0
	MOV	SS,AX
	MOV	SP,0x7c00
	MOV	DS,AX

	MOV	AX,0x0820
	MOV	ES,AX
	MOV	CH,0 		;柱面0
	MOV	DH,0 		;磁头0
	MOV	CL,2 		;从2扇区开始读盘，1扇区是启动区。
;相当于1扇区是从0x008000开始，按照磁盘文件存储格式，
;文件名写在0x002600，文件内容（实际操作系统的代码）写在0x004200
;故，这里需要在读盘（加载入内存）完毕之后跳转到内存中的相关位置，
;即0x008000+0x004200=0x00c200

readloop:
	MOV	SI,0 		;记录失败次数

retry:
	MOV	AH,0x02 	;AH=0x02读入磁盘
	MOV	AL,1 		;1个扇区
	MOV	BX,0 		
	MOV	DL,0x00 	;A驱动器
	INT	0x13 		;调用磁盘BIOS
	JNC	next		;读盘没出错的话跳到fin
	ADD	SI,1
	CMP	SI,5
	JAE	error		;读盘出错重试5次，还出错跳到error
	MOV	AH,0x00
	MOV	DL,0x00 	;A驱动器
	INT	0x13 		;重置驱动器
	JMP	retry

next:
	MOV	AX,ES 		;内存地址后移0x200
	ADD	AX,0x0020
	MOV	ES,AX 		;因为没有 ADD ES,0x020,所以这里要绕个弯
	ADD	CL,1 		;扇区号+1
	CMP	CL,18 		;
	JBE	readloop 	;如果CL<=18，跳转至readloop
	MOV	CL,1
	ADD	DH,1
	CMP	DH,2
	JB	readloop 	;如果DH<2，跳转至readloop
	MOV	DH,0
	ADD	CH,1
	CMP	CH,CYLS
	JB	readloop 	;如果CH<CYLS，跳转至readloop

;读盘完毕之后，跳转到0x00c200
	MOV	[0x0ff0],CH 	;保存CYLS值
	JMP	0xc200

fin:
	HLT
	JMP	fin

error:
	MOV	SI,msg

;输出循环
putloop:
	MOV	AL,[SI]
	ADD	SI,1
	CMP	AL,0
	JE	fin
	MOV	AH,0x0e
	MOV	BX,12
	INT	0x10
	JMP	putloop


;显示消息
msg:
	DB	0x0a, 0x0a
	DB	"load errors"
	DB	0x0a
	DB	0x0a
	DB	"-----------------------------"
	DB	0x0a
	DB	"~~ fyt, I love you ~~"
	DB	0x0a
	DB	"----By:NaOH"
	DB	0x0a
	DB	0

	RESB	0x7dfe-$
	DB	0x55, 0xaa


