#include "stm32f10x.h"
#include "FONT.H"
#include "tetris.h"
#include "IERG3810_LED.h"
#include "IERG3810_Clock.h"
#include "stm32f10x_it.h"
#include "IERG3810_global.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define DS0_ON (GPIO_ResetBits(GPIOB, GPIO_Pin_5))
#define DS0_OFF (GPIO_SetBits(GPIOB, GPIO_Pin_5))
#define DS1_ON (GPIO_ResetBits(GPIOE, GPIO_Pin_5))
#define DS1_OFF (GPIO_SetBits(GPIOE, GPIO_Pin_5))

//Define the color
#define Black	0x0000;
#define White	0xFFFF;
#define LGray	0xBDF7;
#define DGray	0x7BEF;
#define Red 	0xF800;
#define	Yellow 	0xFFE0;
#define	Orange 	0xFBE0;
#define	Brown 	0x79E0;
#define	Green 	0x7E0;
#define	Cyan 	0x7FF;
#define	Blue 	0x1F;
#define	Pink 	0xF81F;

int KeyUpPress = 0;
double Speed = 1000000;
int CountChangeColor = 0;
int CountScore = 0;
int PS2count = 0;
int timeout = 0;
u8 LastRecv = 0x00;
int CountRow = 0;
u8 HeartBeat = 0;

int Playground[24][12];		//Other blocks in the board
int CurrentBoard[24][12];	//Current moving block
int MasterBoard[24][12];	//All the blocks will be copy to here
int Next[4][4];				//For Displaying the next block at the right side

void IERG3810_TFTLCD_SetParameter();
void IERG3810_TFTLCD_WrReg(u16);
void IERG3810_TFTLCD_WrData(u16);

//Game Structure
struct tetris
{
	int game;		//block number
	int tw;
	int th;
	int level;
	int gameover;	//when it is gameover will set to 1
	int score;
	//Block Structure
	 struct Block
	 {
	 	int color;
	 	int w;
	 	int h;
	 	int blockshape[4][4];
	 }block;
	int x;
	int y;
};

struct Block blocks[] = 
{
	//block[0-6]
	//There are total 7 types of block
	//int BlockI[4][4] = 
	//{1,4,
	{		1,
	 		1,4,
		{
		 	{1,0,0,0},
			{1,0,0,0},
			{1,0,0,0},
			{1,0,0,0}
		}
	},

	//int BlockL[4][4]	= 
	//{2,3,
	{		2,
	 		2,3,
		{
		 	{1,1,0,0},
			{1,0,0,0},
			{1,0,0,0},
			{0,0,0,0}
		}
	},

	//int BlockJ[4][4] = 
	//{2,3,
	{		3,
	 		2,3,
		{
		 	{1,1,0,0},
			{0,1,0,0},
			{0,1,0,0},
			{0,0,0,0}
		}
	},

	//int BlockZ[4][4] = 
	//{3,2,
	{		4,
	 		3,2,
	 	{
	 		{1,1,0,0},
			{0,1,1,0},
			{0,0,0,0},
			{0,0,0,0}
		}
	},

	//int BlockS[4][4] = 
	//{3,2,
	{		5,
	 		3,2,
	 	{
	 		{0,1,1,0},
	 		{1,1,0,0},
	 		{0,0,0,0},
	 		{0,0,0,0}
	 	}
	 },

	//int BlockO[4][4] = 
	//{2,2,										
	{		6,
	 		2,2,
	 	{
	 		{1,1,0,0},
			{1,1,0,0},
			{0,0,0,0},
			{0,0,0,0}
		}
	},

	//int BlockT[4][4] = 
	//{3,2,
	{		7,
	 		3,2,
	 	{
	 		{1,1,1,0},
	 		{0,1,0,0},
	 		{0,0,0,0},
	 		{0,0,0,0}
	 	}
	},

};	

struct tetris t;
struct Block currentBlock;
struct Block nextBlock;
struct Block newBlock;

//--------------------------------Copy from lab----------------------------
void Delay(u32 count)
{
	u32 i;
	for (i = 0; i < count; i++);
}

void IERG3810_TIM3_Init(u16 arr, u16 psc){
	RCC->APB1ENR|=1<<1;
	TIM3->ARR=arr;
	TIM3->PSC=psc;
	TIM3->DIER|=1<<0;
	TIM3->CR1|=0x01;
	NVIC->IP[29]=0x45;
	NVIC->ISER[0]|=(1<<29);
}

void TIM3_IRQHandler(void){
	if(TIM3->SR&1<<0)
	{
		GPIOB->ODR^=1<<5;

	}
	TIM3->SR&=~(1<<0);

}

void IERG3810_clock_tree1_init(void){
	u8 PLL=7;
	unsigned char temp=0;
	RCC->CFGR &= 0xF8FF0000;
	RCC->CR &= 0xFEF6FFFF;
	RCC->CR |=0x00010000;
	while(!(RCC->CR>>17));
	RCC->CFGR=0x00000400;
	RCC->CFGR |=PLL<<18;
	RCC->CFGR |=1<<16;
	FLASH->ACR |=0x32;
	RCC->CR |=0x01000000;
	while(!(RCC->CR>>25));
	RCC->CFGR |=0x00000002;
	while(temp!=0x02){
		temp = RCC->CFGR>>2;
		temp &= 0x03;
	}
}

void IERG3810_USART3_init(u32 pclkl, u32 bound){
	float temp;
	u16 mantissa;
	u16 fraction;
	temp = (float)(pclkl*1000000)/(bound*16);
	mantissa = temp;
	fraction = (temp - mantissa)*16;
	mantissa<<=4;
	mantissa+=fraction;
	RCC->APB2ENR |= 1<<2;
	RCC->APB1ENR |= 1<<17;
	GPIOA->CRL &= 0xFFFF00FF;
	GPIOA->CRL |= 0x00008B00;
	RCC->APB1RSTR |= 1<<17;
	RCC->APB1RSTR &=~(1<<17);
	USART2->BRR = mantissa;
	USART2->CR1 |=0x2008;	
}

void IERG3810_JOYSTICK_Init(void){
	
	GPIOC->CRH &= 0xFFFFFF0F;
	GPIOC->CRH |= 0x00000080;
	GPIOC->BRR = 1<<9;
	GPIOC->BSRR = 1<<9;
	
	GPIOC->CRH &= 0xFFF0FFFF;
	GPIOC->CRH |= 0x00030000;
	GPIOC->BRR = 1<<12;
	GPIOC->BSRR = 1<<12;
	
	RCC->APB2ENR |=1<<4;
	GPIOC->CRH &= 0xFFFFFFF0;
	GPIOC->CRH |= 0x00000003;
	GPIOC->BRR = 1<<8;
	GPIOC->BSRR = 1<<8;
}

void IERG3810_key2_ExtiInit(void)
{
	//KEY2 at PE2, EXTI-2, IRQ#8
	RCC->APB2ENR |= 1 << 6; //Refer to lab-1
	GPIOE->CRL &= 0XFFFFF0FF; //Refer to lab-1
	GPIOE->CRL |= 0X00000800; //Refer to lab-1
	GPIOE->ODR |= 1 << 2; //Refer to lab-1
	RCC->APB2ENR |= 0x01; //(RM008-page119)
	AFIO->EXTICR[0] &= 0xFFFFF0FF; //(RM008, page-185)
	AFIO->EXTICR[0] |= 0x00000400; //(RM008, page-185)
	EXTI->IMR |= 1<<2; //(RM008, page-202)
	EXTI->FTSR |= 1<<2; //(RM008, page-203)
	//EXTI->|=1<<2; //(RM0008, page-203)
	
	NVIC->IP[8] = 0x65; //set priority of this interrupt. (D10337E page 8-4, 8-16)
	NVIC->ISER[0] |= (1<<8); //(D10337E page 8-3)
}

void IERG3810_keyUP_ExtiInit(void) //lab 4.2.2
{
	RCC->APB2ENR |= 1 << 2;
	GPIOA->CRL &= 0xFFFFFFF0;
	GPIOA->CRL |= 0x00000008;
	GPIOA->ODR |= 1 << 2;
	RCC->APB2ENR |= 0x01;
	AFIO->EXTICR[0] &= 0xFFFFFFF0;
	AFIO->EXTICR[0] |= 0x00000000;
	EXTI->IMR |= 1<<0;
	EXTI->FTSR |= 1<<0;
	//NVIC->IP[6] = 0x75;
	//NVIC->IP[6] = 0x95; //for lab 4.3.1
	//NVIC->IP[6] = 0x35; //for lab 4.4.1
	NVIC->IP[6] = 0x35; //for lab 4.3.8
	NVIC->ISER[0] |= (1<<6);
}

void IERG3810_key0_ExtiInit(void)
{
	//KEY0 at PE4, EXTI-4, IRQ#
	RCC->APB2ENR |= 1 << 6; //Refer to lab-1
	GPIOE->CRL &= 0XFFFF0FFF; //Refer to lab-1
	GPIOE->CRL |= 0X00008000; //Refer to lab-1
	GPIOE->ODR |= 1 << 2; //Refer to lab-1
	RCC->APB2ENR |= 0x01; //(RM008-page119)
	AFIO->EXTICR[0] &= 0xFFFFF0FF; //(RM008, page-185)
	AFIO->EXTICR[0] |= 0x00000400; //(RM008, page-185)
	EXTI->IMR |= 1<<4; //(RM008, page-202)
	EXTI->FTSR |= 1<<4; //(RM008, page-203)
	//EXTI->|=1<<2; //(RM0008, page-203)
	
	NVIC->IP[8] = 0x65; //set priority of this interrupt. (D10337E page 8-4, 8-16)
	NVIC->ISER[0] |= (1<<8); //(D10337E page 8-3)
}

void IERG3810_PS2key_ExtiInit(void) // lab 4.5
{
  RCC->APB2ENR |= 1<<4;     
  GPIOC->CRH &= 0xFFFF00FF;
  GPIOC->CRH |= 0x00008800;
  RCC->APB2ENR |= 0x01;
  AFIO->EXTICR[2] &= 0xFFFF0FFF; 
  AFIO->EXTICR[2] |= 0x00002000;
  EXTI->IMR |= 1<<11;       
  EXTI->FTSR |= 1<<11;       
  NVIC->IP[40] = 0x00;         
  NVIC->ISER[1] |= (1<<8);     
}

void IERG3810_NVIC_SetPriorityGroup(u8 prigroup)
{
	//Set PRIGROUP AIRCR[10:8]
	u32 temp, temp1;
	temp1 = prigroup & 0x00000007; //only concern 3 bits
	temp1 <<= 8; // 'Why?'
	temp = SCB->AIRCR; //ARMDDI0337E page 8-22
	temp &= 0x0000F8FF; //ARMDDI0337E page 8-22
	temp |= 0x05FA0000; //ARMDDI0337E page 8-22
	temp |= temp1;
	SCB->AIRCR = temp;
}

typedef struct
{
		u16 LCD_REG;
		u16 LCD_RAM;
} LCD_TypeDef;

#define LCD_BASE 	((u32)(0x6C000000 | 0x000007FE))
#define LCD 		((LCD_TypeDef *) LCD_BASE)

void EXTI2_IRQHandler(void) //key 2
{
	DS0_ON;
	Delay(1000000);
	DS0_OFF;
	Delay(1000000);
	
	KeyUpPress = 1;
	
	EXTI->PR = 1<<2; //Clear this exception pending bit
}

void EXTI0_IRQHandler(void) //key 1
{
	DS1_ON;
	Delay(1000000);
	DS1_OFF;
	Delay(1000000);
	
	KeyUpPress = 1;
	
	EXTI->PR = 1<<0;
}

void (*PS2KEY_RECEIVE_FUNC)(u8);

void EXTI15_10_IRQHandler(void) // lab 4.5
{
/* int ps2count;
u8 Recv = 0x00;
u8 bit;
if(EXTI->PR & (1<<11))
Delay(10);
EXTI->PR = 1<<11;
*/
    static u8 recv = 0x00;
    static int bitcount = -1;
    u8 bit;
    if (EXTI->PR & (1 << 11))   // Handle EXTI11
    {
        bit = (GPIOC->IDR & (1 << 10))? 1 : 0;    // Sample PC10
        if (bitcount == -1) {       // Start bit = 0
            if (!bit) {
                recv = 0x00;
                bitcount = 0;
            }
        }
        else if (bitcount <= 7) {   // Received <bitcount>th bit
            recv |= bit << bitcount;
            ++bitcount;
        }
        else if (bitcount == 8) {   // Parity bit
						(*PS2KEY_RECEIVE_FUNC)(recv);
            bitcount = -1;
        }
        //else bitcount = -1; // End bit, back to start again
    }
Delay(100);
    EXTI->PR |= 1 << 11;    // Clear EXTI11 pending bit.  cf. RM0008 p204
}

void PS2key_Recv(u8 Recv) // Lab 4.5
{
	//DS0 lights up as pressed down "0"
  if (Recv == 0x16 && LastRecv != 0xF0) DS0_ON; // according to the figure 4-14
  if (Recv == 0x16 && LastRecv == 0xF0) DS0_OFF; // according to the figure 4-14
    
  //DS1 lights up as pressed down "1"
  if (Recv == 0x1E && LastRecv != 0xF0) DS1_ON; // according to the figure 4-14
  if (Recv == 0x1E && LastRecv == 0xF0) DS1_OFF; // according to the figure 4-14
	
	//DS1 lights up as pressed down "1"
  if (Recv == 0x69 && LastRecv != 0xF0)
		{
			DS1_ON;		//Keypad 1
			Left();
			USART2->DR=0x4C; //"L"
			Delay(50000);
		}
	
  if (Recv == 0x69 && LastRecv == 0xF0)
		{
			DS1_OFF;	//Keypad 1
		}
    
if (Recv == 0x72 && LastRecv != 0xF0)
		{
			DS0_ON; // Keypad 2
			Down();
			USART2->DR=0x44; //"D"
			Delay(50000);
		}
   if (Recv == 0x72 && LastRecv == 0xF0)
		 {
		 	DS0_OFF;	//Keypad 2
		 }

  //DS1 lights up as pressed down "3"
  if (Recv == 0x7A && LastRecv != 0xF0)
		{
			DS1_ON; // Keypad 3
			Right();
			USART2->DR=0x52; //"R"
			Delay(50000);
		}
   if (Recv == 0x7A && LastRecv == 0xF0)
		 {
		 	DS1_OFF;	//Keypad 3
		 }
		

	
	 //DS1 lights up as pressed down "5"
  if (Recv == 0x73 && LastRecv != 0xF0)
		{	
			DS0_ON;// Keypad 5
			DS1_ON;
			Rotate();
		}
  if (Recv == 0x73 && LastRecv == 0xF0) 
		{
			DS0_OFF; // Keypad 5
			DS1_OFF;
		}
		
	LastRecv = Recv;
}

void IERG3810_TFTLCD_Init(void) //set FSMC
{
	RCC->AHBENR|=1<<8;			//FSMC
	RCC->APB2ENR|=1<<3;			//PORTB
	RCC->APB2ENR|=1<<5;			//PORTD
	RCC->APB2ENR|=1<<6;			//PORTE
	RCC->APB2ENR|=1<<8;			//PORTG
	GPIOB->CRL&=0XFFFFFFF0;	//PB0
	GPIOB->CRL|=0X00000003;

	//PORTD
	GPIOD->CRH&=0X00FFF000;
	GPIOD->CRH|=0XBB000BBB;
	GPIOD->CRL&=0XFF00FF00;
	GPIOD->CRL|=0X00BB00BB;
	//PORTE
	GPIOE->CRH&=0X00000000;
	GPIOE->CRH|=0XBBBBBBBB;
	GPIOE->CRL&=0X0FFFFFFF;
	GPIOE->CRL|=0XB0000000;
	//PORTG12
	GPIOG->CRH&=0XFFF0FFFF;
	GPIOG->CRH|=0X000B0000;
	GPIOG->CRL&=0XFFFFFFF0;	//PG0->RS
	GPIOG->CRL|=0X0000000B;

	// LCD uses FSMC Bank 4 memory bank
	// Use mode A
	FSMC_Bank1->BTCR[6]=0X00000000;			//TSMC_BCR4(reset)
	FSMC_Bank1->BTCR[7]=0X00000000;			//TSMC_BTR4(reset)
	FSMC_Bank1E->BWTR[6]=0X00000000;		//TSMC_BWTR4(reset)
	FSMC_Bank1->BTCR[6]|=1<<12;		//FSMC_BCR4 -> WREN
	FSMC_Bank1->BTCR[6]|=1<<14;		//FSMC_BCR4 -> EXTMOD
	FSMC_Bank1->BTCR[6]|=1<<4;		//FSMC_BCR4 -> MWID
	FSMC_Bank1->BTCR[7]|=0<<28;		//FSMC_BTR4 -> ACCMOD
	FSMC_Bank1->BTCR[7]|=1<<0;		//FSMC_BTR4 -> ADDSET
	FSMC_Bank1->BTCR[7]|=0xF<<8;	//FSMC_BTR4 -> DATAST
	FSMC_Bank1E->BWTR[6]|=0<<28;	//FSMC_BWTR4-> ACCMOD
	FSMC_Bank1E->BWTR[6]|=0<<0;		//FSMC_BWTR4-> ADDSET
	FSMC_Bank1E->BWTR[6]|=3<<8;		//FSMC_BWTR4-> DATASET
	FSMC_Bank1->BTCR[6]|=1<<0;		//FSMC_BTR4 -> FACCEN
	IERG3810_TFTLCD_SetParameter(); // special setting for LCD module
	GPIOB->BSRR |= 1<<0;					//LCD_LIGHT_ON;
}

//1-4 
void IERG3810_TFTLCD_SetParameter(void)
{
	IERG3810_TFTLCD_WrReg(0X01);	//Software reset
	IERG3810_TFTLCD_WrReg(0X11);	//Exit_sleep_mode

	IERG3810_TFTLCD_WrReg(0X3A);	//Set_pixel_format
	IERG3810_TFTLCD_WrData(0X55);	//65536 colors

	IERG3810_TFTLCD_WrReg(0X29);	//Display on

	IERG3810_TFTLCD_WrReg(0X36);	//Memory Access Control (section 8.2.29, page 127)
	IERG3810_TFTLCD_WrData(0XCA);
}

//1-5
void IERG3810_TFTLCD_WrReg(u16 regval)
{
	LCD->LCD_REG = regval;
}

//1-6
void IERG3810_TFTLCD_WrData(u16 data)
{
	LCD->LCD_RAM = data;
}

//1-8
void IERG3810_TFTLCD_FillRectangle(u16 color, u16 start_x, u16 start_y, u16 length_x, u16 length_y)
{
	u32 index = 0;
	IERG3810_TFTLCD_WrReg(0x2A);
		IERG3810_TFTLCD_WrData(start_x>>8);
		IERG3810_TFTLCD_WrData(start_x & 0xFF);
		IERG3810_TFTLCD_WrData((length_x + start_x - 1)>>8);
		IERG3810_TFTLCD_WrData((length_x + start_x - 1) & 0xFF);
	IERG3810_TFTLCD_WrReg(0x2B);
		IERG3810_TFTLCD_WrData(start_y>>8);
		IERG3810_TFTLCD_WrData(start_y & 0xFF);
		IERG3810_TFTLCD_WrData((length_y + start_y - 1)>>8);
		IERG3810_TFTLCD_WrData((length_y + start_y - 1) & 0xFF);
	IERG3810_TFTLCD_WrReg(0x2C);
	for(index = 0; index < length_x * length_y; index++)
	{
	IERG3810_TFTLCD_WrData(color);
	}
}


//ShowChar
void IERG3810_TFTLCD_ShowChar(u16 x, u16 y, u8 ascii, u16 color, u16 bgcolor)
{
	u8 i,j;
	u8 index;
	u8 height=16,length=8;
	if(ascii<32||ascii>127) return;
	ascii-= 32;
	IERG3810_TFTLCD_WrReg(0x2A);
		IERG3810_TFTLCD_WrData(x>>8);
		IERG3810_TFTLCD_WrData(x&0xFF);
		IERG3810_TFTLCD_WrData((length+x-1)>>8);
		IERG3810_TFTLCD_WrData((length+x-1)&0xFF);
	IERG3810_TFTLCD_WrReg(0x2B);
		IERG3810_TFTLCD_WrData(y>>8);
		IERG3810_TFTLCD_WrData(y&0xFF);
		IERG3810_TFTLCD_WrData((height+y-1)>>8);
		IERG3810_TFTLCD_WrData((height+y-1)&0xFF);
		IERG3810_TFTLCD_WrData((30+y)>>8);
		IERG3810_TFTLCD_WrData((30+y)&0xFF);
		IERG3810_TFTLCD_WrReg(0x2C); //LCD_writeRAM_Prepare();

	for(j=0;j<height/8;j++){
		for(i=0;i<height/2;i++){
			for(index=0;index<length;index++){
				if ((asc2_1608[ascii][index*2+1-j]>>i)&0x01)
					IERG3810_TFTLCD_WrData(color);
				else
					IERG3810_TFTLCD_WrData(bgcolor);
			}
		}
	}
}

//--------------------------------Copy from lab----------------------------
void CleanArray()
{
	int i, j;

	//Clean the boards
	for(i=0;i<24;i++)
		for(j=0;j<12;j++)
		{
			Playground[i][j] = 0;
			CurrentBoard[i][j] = 0;
			MasterBoard[i][j] = 0;
		}
}

void CleanCurrent()
{
	int i, j;

	for(i=0;i<24;i++)
		for(j=0;j<12;j++)
				CurrentBoard[i][j] = 0;
}

void CleanRow(int y)
{	
	int i, j;
	
	for(i=0;i<12;i++)
	{
		if(y == -1)break;
		Playground[y][i] = 0;
		MasterBoard[y][i] = 0;
		Delay(100000);
	}

	//Gravity
	for(i=22;i>y;i--)
		for(j=0;j<12;j++)
			MasterBoard[i][j] = MasterBoard[i+1][j];

	CountRow = 0;
	t.score += 20;
}

void Initialize()
{
	//Initial the Structure t's values
	t.game = -1;
	t.tw = 12;	//GameBoard wide
	t.th = 24;	//GameBoard height
	t.level = 0;
	t.gameover = 0;
	t.score = 0;
	t.x = 5;	//current x-axis
	t.y = 20;	//current y-axis
	CleanArray();
}

//Function for creating new block
//Very difficult to write
void create_block()
{
	int i, j;
	int r = rand() % 7; //Print random numbers from 0 to 6

	//struct Block newBlock;
	for(i=0;i<4;i++)
		for(j=0;j<4;j++)
			nextBlock.blockshape[i][j] = 0;
	
	newBlock = blocks[r];

	//if the game is new, copy a new block to become the current playing block
	if(t.game == -1)
	{
		// for(i=0;i<4;i++)
		// 	for(j=0;j<4;j++)
		// 		currentBlock->blockshape[i][j] = newBlock.blockshape[i][j];
		currentBlock = newBlock;
		r = rand() % 7;
		newBlock = blocks[r];
	}

	// for(i=0;i<4;i++)
	// 		for(j=0;j<4;j++)
	// 			currentBlock->blockshape[i][j] = newBlock.blockshape[i][j];
	nextBlock = newBlock;
	t.game++;
}

int isBottom()
{
	int i, j;
	for(i=0;i<currentBlock.w;i++)
		//if(t.y == 0 || (MasterBoard[t.y-1][t.x+i] == 1 && CurrentBoard[t.y][i] == 1))return 1;
		if(t.y == 0 || MasterBoard[t.y-1][t.x+i] == 1)return 1;
		//Checking if the block has reached the bottom of the board or
		//reached other blocks
	
	return 0;
}

int isTop()
{
	int i, j;

	for(i=0;i<12;i++)
		if(MasterBoard[22][i] == 1 || MasterBoard[23][i] == 1 || Playground[22][i] == 1 || Playground[23][i] == 1)return 1;

	for(i=0;i<4;i++)
		for(j=0;j<4;j++)
			if(Playground[20+i][5+j] == 1)return 1;

	return 0;
}

int isFull()
{
	int fcount = 0;
	int i, j;

	for(i=0;i<24;i++)
	{
		for(j=0;j<12;j++)
		{
			if(MasterBoard[i][j] == 1)fcount++;
			if(fcount == 12)
				{
					CountRow = i;
					return 1;
				}
		}
		fcount = 0;
	}

	return 0;
}

void Fall()
{
	Delay(Speed);
	t.y--;
}

void Stay()
{
	int i, j;

	for(i=0;i<24;i++)
		for(j=0;j<12;j++)
			if(CurrentBoard[i][j] == 1)Playground[i][j] = 1;

	Copy2Master();
	t.score += 10;
}

//integer to string
void int2str(int i, char *s)
{
	sprintf(s,"%d",i);
}


void Copy2Master()
{
	int i, j;

	for(i=0;i<24;i++)
		for(j=0;j<12;j++)
			if(Playground[i][j] == 1)MasterBoard[i][j] = 1;
			else MasterBoard[i][j] = 0;
}

void Levelup()
{
	if((t.score % 20)==0)
		{
			t.level++;
			Speed/=1.5;
		}
}

void DisplayPlayground()
{
	int i, j;

	//Displaying the Playground
	for(i=0;i<24;i++)
		for(j=0;j<12;j++)
		{
			if(Playground[i][j] == 1 || MasterBoard[i][j] == 1) IERG3810_TFTLCD_FillRectangle(0x7BEF,10+j*10,10+i*10,10,10);
			else if(CurrentBoard[i][j] == 0 && Playground[i][j] == 0)IERG3810_TFTLCD_FillRectangle(0x0000,10+j*10,10+i*10,10,10);
		}
}

void DisplayCurrentBlock()
{
	int i, j;
	u16 BlockColor;

	CleanCurrent();

	//Copying currentBlock to the CurrentBoard
	for(i=0;i<4;i++)
		for(j=0;j<4;j++)
		{
			CurrentBoard[t.y+i][t.x+j] = currentBlock.blockshape[i][j];
		}

	//Switching the color
	if(currentBlock.color == 1)BlockColor = Red;
	if(currentBlock.color == 2)BlockColor = Yellow;
	if(currentBlock.color == 3)BlockColor = Orange;
	if(currentBlock.color == 4)BlockColor = Pink;
	if(currentBlock.color == 5)BlockColor = Green;
	if(currentBlock.color == 6)BlockColor = Cyan;
	if(currentBlock.color == 7)BlockColor = Blue;

	//Displaying the CurrentBoard
	for(i=0;i<24;i++)
		for(j=0;j<12;j++)
		{
			if(CurrentBoard[i][j] == 1) IERG3810_TFTLCD_FillRectangle(BlockColor,10+j*10,10+i*10,10,10);
			else if(CurrentBoard[i][j] == 0 && Playground[i][j] == 0)IERG3810_TFTLCD_FillRectangle(0x0000,10+j*10,10+i*10,10,10);
		}

}

void DisplayLogo(int x, int y)
{
	if(CountChangeColor%6==5)
	{
		//T
		IERG3810_TFTLCD_FillRectangle(0xFBE0,x,y+40,30,10);
		IERG3810_TFTLCD_FillRectangle(0xFBE0,x+10,y,10,40);
		
		//E
		IERG3810_TFTLCD_FillRectangle(0xFFE0,x+40,y,10,50);
		IERG3810_TFTLCD_FillRectangle(0xFFE0,x+50,y+40,20,10);
		IERG3810_TFTLCD_FillRectangle(0xFFE0,x+50,y+20,20,10);
		IERG3810_TFTLCD_FillRectangle(0xFFE0,x+50,y,20,10);
		
		//T
		IERG3810_TFTLCD_FillRectangle(0x7E0,x+80,y+40,30,10);
		IERG3810_TFTLCD_FillRectangle(0x7E0,x+90,y,10,40);
		
		//R
		IERG3810_TFTLCD_FillRectangle(0x1F,x+120,y,10,50);
		IERG3810_TFTLCD_FillRectangle(0x1F,x+130,y+10,10,20);
		IERG3810_TFTLCD_FillRectangle(0x1F,x+130,y+40,10,10);
		IERG3810_TFTLCD_FillRectangle(0x1F,x+140,y,10,10);
		IERG3810_TFTLCD_FillRectangle(0x1F,x+140,y+20,10,30);
		
		//I
		IERG3810_TFTLCD_FillRectangle(0xF81F,x+160,y,10,50);
		
		//S
		IERG3810_TFTLCD_FillRectangle(0xF800,x+180,y+40,30,10);
		IERG3810_TFTLCD_FillRectangle(0xF800,x+180,y+30,10,10);
		IERG3810_TFTLCD_FillRectangle(0xF800,x+180,y+20,30,10);
		IERG3810_TFTLCD_FillRectangle(0xF800,x+200,y+10,10,10);
		IERG3810_TFTLCD_FillRectangle(0xF800,x+180,y,30,10);
	}
		
	if(CountChangeColor%6==4)
	{
		//T
		IERG3810_TFTLCD_FillRectangle(0xFFE0,x,y+40,30,10);
		IERG3810_TFTLCD_FillRectangle(0xFFE0,x+10,y,10,40);
		
		//E
		IERG3810_TFTLCD_FillRectangle(0x7E0,x+40,y,10,50);
		IERG3810_TFTLCD_FillRectangle(0x7E0,x+50,y+40,20,10);
		IERG3810_TFTLCD_FillRectangle(0x7E0,x+50,y+20,20,10);
		IERG3810_TFTLCD_FillRectangle(0x7E0,x+50,y,20,10);
		
		//T
		IERG3810_TFTLCD_FillRectangle(0x1F,x+80,y+40,30,10);
		IERG3810_TFTLCD_FillRectangle(0x1F,x+90,y,10,40);
		
		//R
		IERG3810_TFTLCD_FillRectangle(0xF81F,x+120,y,10,50);
		IERG3810_TFTLCD_FillRectangle(0xF81F,x+130,y+10,10,20);
		IERG3810_TFTLCD_FillRectangle(0xF81F,x+130,y+40,10,10);
		IERG3810_TFTLCD_FillRectangle(0xF81F,x+140,y,10,10);
		IERG3810_TFTLCD_FillRectangle(0xF81F,x+140,y+20,10,30);
		
		//I
		IERG3810_TFTLCD_FillRectangle(0xF800,x+160,y,10,50);
		
		//S
		IERG3810_TFTLCD_FillRectangle(0xFBE0,x+180,y+40,30,10);
		IERG3810_TFTLCD_FillRectangle(0xFBE0,x+180,y+30,10,10);
		IERG3810_TFTLCD_FillRectangle(0xFBE0,x+180,y+20,30,10);
		IERG3810_TFTLCD_FillRectangle(0xFBE0,x+200,y+10,10,10);
		IERG3810_TFTLCD_FillRectangle(0xFBE0,x+180,y,30,10);
	}
	
	if(CountChangeColor%6==3)
	{
		//T
		IERG3810_TFTLCD_FillRectangle(0x7E0,x,y+40,30,10);
		IERG3810_TFTLCD_FillRectangle(0x7E0,x+10,y,10,40);
		
		//E
		IERG3810_TFTLCD_FillRectangle(0x1F,x+40,y,10,50);
		IERG3810_TFTLCD_FillRectangle(0x1F,x+50,y+40,20,10);
		IERG3810_TFTLCD_FillRectangle(0x1F,x+50,y+20,20,10);
		IERG3810_TFTLCD_FillRectangle(0x1F,x+50,y,20,10);
		
		//T
		IERG3810_TFTLCD_FillRectangle(0xF81F,x+80,y+40,30,10);
		IERG3810_TFTLCD_FillRectangle(0xF81F,x+90,y,10,40);
		
		//R
		IERG3810_TFTLCD_FillRectangle(0xF800,x+120,y,10,50);
		IERG3810_TFTLCD_FillRectangle(0xF800,x+130,y+10,10,20);
		IERG3810_TFTLCD_FillRectangle(0xF800,x+130,y+40,10,10);
		IERG3810_TFTLCD_FillRectangle(0xF800,x+140,y,10,10);
		IERG3810_TFTLCD_FillRectangle(0xF800,x+140,y+20,10,30);
		
		//I
		IERG3810_TFTLCD_FillRectangle(0xFBE0,x+160,y,10,50);
		
		//S
		IERG3810_TFTLCD_FillRectangle(0xFFE0,x+180,y+40,30,10);
		IERG3810_TFTLCD_FillRectangle(0xFFE0,x+180,y+30,10,10);
		IERG3810_TFTLCD_FillRectangle(0xFFE0,x+180,y+20,30,10);
		IERG3810_TFTLCD_FillRectangle(0xFFE0,x+200,y+10,10,10);
		IERG3810_TFTLCD_FillRectangle(0xFFE0,x+180,y,30,10);
	}
		
	if(CountChangeColor%6==2)
	{
		//T
		IERG3810_TFTLCD_FillRectangle(0x1F,x,y+40,30,10);
		IERG3810_TFTLCD_FillRectangle(0x1F,x+10,y,10,40);
		
		//E
		IERG3810_TFTLCD_FillRectangle(0xF81F,x+40,y,10,50);
		IERG3810_TFTLCD_FillRectangle(0xF81F,x+50,y+40,20,10);
		IERG3810_TFTLCD_FillRectangle(0xF81F,x+50,y+20,20,10);
		IERG3810_TFTLCD_FillRectangle(0xF81F,x+50,y,20,10);
		
		//T
		IERG3810_TFTLCD_FillRectangle(0xF800,x+80,y+40,30,10);
		IERG3810_TFTLCD_FillRectangle(0xF800,x+90,y,10,40);
		
		//R
		IERG3810_TFTLCD_FillRectangle(0xFBE0,x+120,y,10,50);
		IERG3810_TFTLCD_FillRectangle(0xFBE0,x+130,y+10,10,20);
		IERG3810_TFTLCD_FillRectangle(0xFBE0,x+130,y+40,10,10);
		IERG3810_TFTLCD_FillRectangle(0xFBE0,x+140,y,10,10);
		IERG3810_TFTLCD_FillRectangle(0xFBE0,x+140,y+20,10,30);
		
		//I
		IERG3810_TFTLCD_FillRectangle(0xFFE0,x+160,y,10,50);
		
		//S
		IERG3810_TFTLCD_FillRectangle(0x7E0,x+180,y+40,30,10);
		IERG3810_TFTLCD_FillRectangle(0x7E0,x+180,y+30,10,10);
		IERG3810_TFTLCD_FillRectangle(0x7E0,x+180,y+20,30,10);
		IERG3810_TFTLCD_FillRectangle(0x7E0,x+200,y+10,10,10);
		IERG3810_TFTLCD_FillRectangle(0x7E0,x+180,y,30,10);
	}
	
	if(CountChangeColor%6==1)
	{
		//T
		IERG3810_TFTLCD_FillRectangle(0xF81F,x,y+40,30,10);
		IERG3810_TFTLCD_FillRectangle(0xF81F,x+10,y,10,40);
		
		//E
		IERG3810_TFTLCD_FillRectangle(0xF800,x+40,y,10,50);
		IERG3810_TFTLCD_FillRectangle(0xF800,x+50,y+40,20,10);
		IERG3810_TFTLCD_FillRectangle(0xF800,x+50,y+20,20,10);
		IERG3810_TFTLCD_FillRectangle(0xF800,x+50,y,20,10);
		
		//T
		IERG3810_TFTLCD_FillRectangle(0xFBE0,x+80,y+40,30,10);
		IERG3810_TFTLCD_FillRectangle(0xFBE0,x+90,y,10,40);
		
		//R
		IERG3810_TFTLCD_FillRectangle(0xFFE0,x+120,y,10,50);
		IERG3810_TFTLCD_FillRectangle(0xFFE0,x+130,y+10,10,20);
		IERG3810_TFTLCD_FillRectangle(0xFFE0,x+130,y+40,10,10);
		IERG3810_TFTLCD_FillRectangle(0xFFE0,x+140,y,10,10);
		IERG3810_TFTLCD_FillRectangle(0xFFE0,x+140,y+20,10,30);
		
		//I
		IERG3810_TFTLCD_FillRectangle(0x7E0,x+160,y,10,50);
		
		//S
		IERG3810_TFTLCD_FillRectangle(0x1F,x+180,y+40,30,10);
		IERG3810_TFTLCD_FillRectangle(0x1F,x+180,y+30,10,10);
		IERG3810_TFTLCD_FillRectangle(0x1F,x+180,y+20,30,10);
		IERG3810_TFTLCD_FillRectangle(0x1F,x+200,y+10,10,10);
		IERG3810_TFTLCD_FillRectangle(0x1F,x+180,y,30,10);
	}
	
	if(CountChangeColor%6==0)
	{
		//T
		IERG3810_TFTLCD_FillRectangle(0xF800,x,y+40,30,10);
		IERG3810_TFTLCD_FillRectangle(0xF800,x+10,y,10,40);
		
		//E
		IERG3810_TFTLCD_FillRectangle(0xFBE0,x+40,y,10,50);
		IERG3810_TFTLCD_FillRectangle(0xFBE0,x+50,y+40,20,10);
		IERG3810_TFTLCD_FillRectangle(0xFBE0,x+50,y+20,20,10);
		IERG3810_TFTLCD_FillRectangle(0xFBE0,x+50,y,20,10);
		
		//T
		IERG3810_TFTLCD_FillRectangle(0xFFE0,x+80,y+40,30,10);
		IERG3810_TFTLCD_FillRectangle(0xFFE0,x+90,y,10,40);
		
		//R
		IERG3810_TFTLCD_FillRectangle(0x7E0,x+120,y,10,50);
		IERG3810_TFTLCD_FillRectangle(0x7E0,x+130,y+10,10,20);
		IERG3810_TFTLCD_FillRectangle(0x7E0,x+130,y+40,10,10);
		IERG3810_TFTLCD_FillRectangle(0x7E0,x+140,y,10,10);
		IERG3810_TFTLCD_FillRectangle(0x7E0,x+140,y+20,10,30);
		
		//I
		IERG3810_TFTLCD_FillRectangle(0x1F,x+160,y,10,50);
		
		//S
		IERG3810_TFTLCD_FillRectangle(0xF81F,x+180,y+40,30,10);
		IERG3810_TFTLCD_FillRectangle(0xF81F,x+180,y+30,10,10);
		IERG3810_TFTLCD_FillRectangle(0xF81F,x+180,y+20,30,10);
		IERG3810_TFTLCD_FillRectangle(0xF81F,x+200,y+10,10,10);
		IERG3810_TFTLCD_FillRectangle(0xF81F,x+180,y,30,10);
	}
	
	CountChangeColor++;
}

void DisplayGame()
{
	char *id1, *id2, *id3, *id4, *id5;
	char lvl[10], sc[1024];
	int x;
	

	IERG3810_TFTLCD_FillRectangle(0xBDF7,0,310,240,10);
	IERG3810_TFTLCD_FillRectangle(0xBDF7,0,0,10,320);
	IERG3810_TFTLCD_FillRectangle(0xBDF7,130,0,10,250);
	IERG3810_TFTLCD_FillRectangle(0xBDF7,230,0,10,320);
	IERG3810_TFTLCD_FillRectangle(0xBDF7,0,0,240,10);
	IERG3810_TFTLCD_FillRectangle(0xBDF7,0,250,240,10);
	IERG3810_TFTLCD_FillRectangle(0x0000,180,180,10,40);

	DisplayLogo(10,260);

	//Display Level
	id1="Level";
	for(x=0;x<5;x++)
		IERG3810_TFTLCD_ShowChar((10*x+150),160,id1[x],0xF800,0x0000);
	//Display int Level
	int2str(t.level, lvl);
	for(x=0;x<2;x++)
		IERG3810_TFTLCD_ShowChar((10*x+150),140,lvl[x],0x7E0,0x0000);
	
	//Display Score
	id2="Score";
	for (x=0;x<5;x++)
		IERG3810_TFTLCD_ShowChar((10*x+150),110,id2[x],0xF800,0x0000);
	//Display int Score
	int2str(t.score, sc);
	for (x=0;x<4;x++)
		IERG3810_TFTLCD_ShowChar((10*x+150),90,sc[x],0x7E0,0x0000);
	
	//Display Next
	id3="Next";
	for (x=0;x<4;x++)
		IERG3810_TFTLCD_ShowChar((10*x+150),230,id3[x],0xF800,0x0000);
		
	//Display Reset Instruction
	id4="*Reset";
	for (x=0;x<6;x++)
		IERG3810_TFTLCD_ShowChar((10*x+150),30,id4[x],0x7FF,0x0000);
		
	id5="by KEY2*";
	for (x=0;x<8;x++)
		IERG3810_TFTLCD_ShowChar((10*x+150),15,id5[x],0x7FF,0x0000);
}

void DisplayNextBlock()
{
	int i, j;
	int m, n;

	for(i=0;i<4;i++)
		for(j=0;j<4;j++)
		{
			if(nextBlock.blockshape[i][j] == 1)Next[i][j] = 1;
			else Next[i][j] = 0;
		}

	//Displaying at the Next Block Space
	for(m=0;m<4;m++)
		for(n=0;n<4;n++)
		{
			//if(Next[m][n] == 1)IERG3810_TFTLCD_FillRectangle(0xFFFF,150+m*10,185+n*10,10,10);
			//next board color
			 if(Next[m][n] == 1){
			 	for(i=0;i<4;i++)
			 		for(j=0;j<4;j++){
			 			if(nextBlock.color == 1)
			 				IERG3810_TFTLCD_FillRectangle(0xF800,150+m*10,185+n*10,10,10);
			 			if(nextBlock.color == 2)
			 				IERG3810_TFTLCD_FillRectangle(0xFFE0,150+m*10,185+n*10,10,10);
			 			if(nextBlock.color == 3)
			 				IERG3810_TFTLCD_FillRectangle(0xFBE0,150+m*10,185+n*10,10,10);
			 			if(nextBlock.color == 4)
			 				IERG3810_TFTLCD_FillRectangle(0xF81F,150+m*10,185+n*10,10,10);
			 			if(nextBlock.color == 5)
			 				IERG3810_TFTLCD_FillRectangle(0x7E0,150+m*10,185+n*10,10,10);
			 			if(nextBlock.color == 6)
			 				IERG3810_TFTLCD_FillRectangle(0x7FF,150+m*10,185+n*10,10,10);
			 			if(nextBlock.color == 7)
			 				IERG3810_TFTLCD_FillRectangle(0x1F,150+m*10,185+n*10,10,10);
					}
			}
			else IERG3810_TFTLCD_FillRectangle(0x0000,150+m*10,185+n*10,10,10);
		}
}

void GameOver()
{
	if(t.gameover == 1)
	{
		Lose();
	}
			
	if(t.level == 10)
	{
		Win();
	}
}

//Only Display the Intro Words
void DisplayIntro()
{
	//while(1)
	//{
		//struct tetris t;
		int x = 10, y = 260;
		char* GameName;
		char* StudentNameAndSID1;
		char* StudentNameAndSID2;
		char* PressKeyToStart;
		char* InstructionLine1;
		char* InstructionLine2;
		char* InstructionLine3;
		char* InstructionLine4;
		char* InstructionLine5;
		char* InstructionLine6;
		char* InstructionLine7;
	
		IERG3810_TFTLCD_FillRectangle(0x0000,0,10,240,300);
	
		IERG3810_TFTLCD_FillRectangle(0xBDF7,0,310,240,10); // x y hw vw
		IERG3810_TFTLCD_FillRectangle(0xBDF7,0,0,240,10);
		
		//T
		IERG3810_TFTLCD_FillRectangle(0xF800,x,y+40,30,10);
		IERG3810_TFTLCD_FillRectangle(0xF800,x+10,y,10,40);
		
		//E
		IERG3810_TFTLCD_FillRectangle(0xFBE0,x+40,y,10,50);
		IERG3810_TFTLCD_FillRectangle(0xFBE0,x+50,y+40,20,10);
		IERG3810_TFTLCD_FillRectangle(0xFBE0,x+50,y+20,20,10);
		IERG3810_TFTLCD_FillRectangle(0xFBE0,x+50,y,20,10);
		
		//T
		IERG3810_TFTLCD_FillRectangle(0xFFE0,x+80,y+40,30,10);
		IERG3810_TFTLCD_FillRectangle(0xFFE0,x+90,y,10,40);
		
		//R
		IERG3810_TFTLCD_FillRectangle(0x7E0,x+120,y,10,50);
		IERG3810_TFTLCD_FillRectangle(0x7E0,x+130,y+10,10,20);
		IERG3810_TFTLCD_FillRectangle(0x7E0,x+130,y+40,10,10);
		IERG3810_TFTLCD_FillRectangle(0x7E0,x+140,y,10,10);
		IERG3810_TFTLCD_FillRectangle(0x7E0,x+140,y+20,10,30);
		
		//I
		IERG3810_TFTLCD_FillRectangle(0x1F,x+160,y,10,50);
		
		//S
		IERG3810_TFTLCD_FillRectangle(0xF81F,x+180,y+40,30,10);
		IERG3810_TFTLCD_FillRectangle(0xF81F,x+180,y+30,10,10);
		IERG3810_TFTLCD_FillRectangle(0xF81F,x+180,y+20,30,10);
		IERG3810_TFTLCD_FillRectangle(0xF81F,x+200,y+10,10,10);
		IERG3810_TFTLCD_FillRectangle(0xF81F,x+180,y,30,10);		

		StudentNameAndSID1="TSUI MAN LONG 1155068080"; 	// 24 char
		for(x=0;x<24;x++)
		IERG3810_TFTLCD_ShowChar((10*x),220,StudentNameAndSID1[x],0x7E0,0x0000);

		StudentNameAndSID2="WAN KAM LEUNG 1155068082"; 	// 24 char
		for(x=0;x<24;x++)
		IERG3810_TFTLCD_ShowChar((10*x),200,StudentNameAndSID2[x],0x7E0,0x0000);
		
		PressKeyToStart="*Press KEY_UP to start*"; 			// 23 char
		for(x=0;x<23;x++)
		IERG3810_TFTLCD_ShowChar((10*x),170,PressKeyToStart[x],0xF800,0x0000);
		
		InstructionLine1="Tetris is constructed by"; 		// 24 char
		for(x=0;x<24;x++)
		IERG3810_TFTLCD_ShowChar((10*x),140,InstructionLine1[x],0xFFFF,0x0000);
		
		InstructionLine2="different blocks, which"; 		// 23 char
		for(x=0;x<23;x++)
		IERG3810_TFTLCD_ShowChar((10*x),120,InstructionLine2[x],0xFFFF,0x0000);
		
		InstructionLine3="is composed of four";					// 19 char
		for(x=0;x<19;x++)
		IERG3810_TFTLCD_ShowChar((10*x),100,InstructionLine3[x],0xFFFF,0x0000);
		
		InstructionLine4="square boxes. The game.";			// 22char
		for(x=0;x<22;x++)
		IERG3810_TFTLCD_ShowChar((10*x),80,InstructionLine4[x],0xFFFF,0x0000);
		
		InstructionLine5="player can get the marks";		// 24char
		for(x=0;x<24;x++)
		IERG3810_TFTLCD_ShowChar((10*x),60,InstructionLine5[x],0xFFFF,0x0000);
		
		InstructionLine6="when completing a row";				// 21char
		for(x=0;x<21;x++)
		IERG3810_TFTLCD_ShowChar((10*x),40,InstructionLine6[x],0xFFFF,0x0000);
		
		InstructionLine7="or more rows at a time.";			// 24char
		for(x=0;x<24;x++)
		IERG3810_TFTLCD_ShowChar((10*x),20,InstructionLine7[x],0xFFFF,0x0000);
	
		//if(KeyUpPress == 1){
		// 	KeyUpPress = 0;
		// 	IERG3810_TFTLCD_FillRectangle(0x0000,0,0,240,310);	
		// 	//Initialize();
		// 	break;
		 //}
	//}
}

//Holding in the Intro Page
void LoopIntro()
{
	int CountLoop = 0;
	Delay(100000);
	DisplayIntro();
	while(KeyUpPress == 0)
	{
		CountLoop++;
	}
	
	KeyUpPress = 0;
	IERG3810_TFTLCD_FillRectangle(0x0000,0,0,240,310);
}

void Win()
{
	IERG3810_TFTLCD_FillRectangle(0x0000,0,0,240,310);
	while(Reset() == 0)
	{
		int JoyCount=0;
		int Joykey=0>>9;
		IERG3810_TFTLCD_FillRectangle(0xBDF7,0,310,240,10);
		IERG3810_TFTLCD_FillRectangle(0xBDF7,0,0,10,310);
		IERG3810_TFTLCD_FillRectangle(0xBDF7,230,10,10,310);
		IERG3810_TFTLCD_FillRectangle(0xBDF7,10,0,240,10);	
						
		//W
		IERG3810_TFTLCD_FillRectangle(0xF800,10+40,160,10,50);
		IERG3810_TFTLCD_FillRectangle(0xF800,10+50,160,40,10);
		IERG3810_TFTLCD_FillRectangle(0xF800,10+60,160,10,50);	
		IERG3810_TFTLCD_FillRectangle(0xF800,10+80,160,10,50);

		//I
		IERG3810_TFTLCD_FillRectangle(0x1F,	120,160,10,50);

		//N
		IERG3810_TFTLCD_FillRectangle(0x7E0,110+40,160,10,50);
		IERG3810_TFTLCD_FillRectangle(0x7E0,110+50,200,20,10);
		IERG3810_TFTLCD_FillRectangle(0x7E0,130+50,160,20,10);
		IERG3810_TFTLCD_FillRectangle(0x7E0,110+60,160,10,50);	
		IERG3810_TFTLCD_FillRectangle(0x7E0,110+80,160,10,50);	

		DS1_ON;
		Delay(1000000);
		DS1_OFF;
		Delay(1000000);
		
		while(JoyCount<8){
			u8 Mydata;
			if(JoyCount==0){
				GPIOC->BSRR=1<<8;
				Delay(10);
				Mydata = (GPIOC->IDR & 1<<9)>>9;
				Joykey |= (Mydata << (7-JoyCount));
				GPIOC->BRR = 1<<8;
			}
			else {
				GPIOC->BSRR=1<<12;
				Delay(10);
				Mydata = (GPIOC->IDR & 1<<9)>>9;
				Joykey |= (Mydata << (7-JoyCount));								
				GPIOC->BRR = 1<<12;		
			}
			JoyCount++;
			Delay(10);						
		}
			
		if(Joykey==0xBF){ //B
			LoopIntro();
		}

	}	
}

void Lose()
{
	IERG3810_TFTLCD_FillRectangle(0x0000,0,0,240,310);
	while(Reset() == 0)
	{
		int JoyCount=0;
		int Joykey=0>>9;
		IERG3810_TFTLCD_FillRectangle(0xBDF7,0,310,240,10);
		IERG3810_TFTLCD_FillRectangle(0xBDF7,0,0,10,310);
		IERG3810_TFTLCD_FillRectangle(0xBDF7,230,10,10,310);
		IERG3810_TFTLCD_FillRectangle(0xBDF7,10,0,240,10);	
		
		//L
		IERG3810_TFTLCD_FillRectangle(0xF800,5+40,160,10,50);
		IERG3810_TFTLCD_FillRectangle(0xF800,5+50,160,20,10);		

		//O
		IERG3810_TFTLCD_FillRectangle(0xF800,45+40,160,10,50);
		IERG3810_TFTLCD_FillRectangle(0xF800,45+60,160,10,50);
		IERG3810_TFTLCD_FillRectangle(0xF800,45+50,160,20,10);		
		IERG3810_TFTLCD_FillRectangle(0xF800,45+50,200,20,10);

		//S
		IERG3810_TFTLCD_FillRectangle(0xF800,85+40,180,10,30);
		IERG3810_TFTLCD_FillRectangle(0xF800,85+50,160+40,20,10);
		IERG3810_TFTLCD_FillRectangle(0xF800,85+60,160+10,10,10);
		IERG3810_TFTLCD_FillRectangle(0xF800,85+50,160+20,20,10);
		IERG3810_TFTLCD_FillRectangle(0xF800,85+40,160,30,10);		

		//E
		IERG3810_TFTLCD_FillRectangle(0xF800,125+40,160,10,50);
		IERG3810_TFTLCD_FillRectangle(0xF800,125+50,160+40,20,10);
		IERG3810_TFTLCD_FillRectangle(0xF800,125+50,160+20,20,10);
		IERG3810_TFTLCD_FillRectangle(0xF800,125+50,160,20,10);

		DS1_ON;
		Delay(1000000);
		DS1_OFF;
		Delay(1000000);

		while(JoyCount<8){
			u8 Mydata;
			if(JoyCount==0){
				GPIOC->BSRR=1<<8;
				Delay(10);
				Mydata = (GPIOC->IDR & 1<<9)>>9;
				Joykey |= (Mydata << (7-JoyCount));
				GPIOC->BRR = 1<<8;
			}
			else {
				GPIOC->BSRR=1<<12;
				Delay(10);
				Mydata = (GPIOC->IDR & 1<<9)>>9;
				Joykey |= (Mydata << (7-JoyCount));								
				GPIOC->BRR = 1<<12;		
			}
			JoyCount++;
			Delay(10);						
		}
			
		if(Joykey==0x7F){ //A
			LoopIntro();
		}
	}
}

int Reset()
{
	int i, j;
	if(KeyUpPress == 1)
		{
			IERG3810_TFTLCD_FillRectangle(0x0000,0,0,240,310);
			Speed = 1000000;
			Initialize();
			DisplayGame();
			KeyUpPress = 0;
			return 1;
		}

	return 0;
}

//The Control
void Right()
{
	if(t.x<10)t.x++;
	if((currentBlock.color == 7 || currentBlock.color == 4 || currentBlock.color == 5) && t.x>9)t.x-=1;
	//else if(t.x>10 && currentBlock.w == 3)t.x-=2;
	//else t.x-=1;
}

void Left()
{
	if(t.x>0)t.x--;
}

void Rotate()
{
	int i, j;
	int temp[4][4];
	int tmp = 0;
	int count = 0;

	for(i=0; i<4; i++)
   		for(j=0; j<4; j++)
        	temp[i][j] = currentBlock.blockshape[4-1-j][i];
	
	for(i=0; i<4; i++)
    	for(j=0; j<4; j++)
        	currentBlock.blockshape[i][j] = temp[i][j];

    tmp = currentBlock.w;
    currentBlock.w = currentBlock.h;
    currentBlock.h = tmp;

    // for(i=0;i<4;i++)
    // {
    // 	for(j=0;j<4;j++)
    // 	{
    // 		if(currentBlock.blockshape[i][j] == 1)
    // 			{
    // 				t.x -= j;
    // 				t.y -= i;
    // 				count = 1;
    // 				break;
    // 			}
    // 	}
    // 	if(count == 1)break;
    // }

    //t.x -= 2;
    //t.y -= 1;

}

void Down()
{
	if(t.y>0&&t.y<24)t.y--;
}

int main(void)
{
	PS2KEY_RECEIVE_FUNC = &PS2key_Recv;
	IERG3810_clock_tree_init();
	IERG3810_LED_Init();
	IERG3810_NVIC_SetPriorityGroup(5); //set PRIGROUP
	IERG3810_key2_ExtiInit(); //Init KEYUP as an interrupt input
	IERG3810_keyUP_ExtiInit(); //Init KEYUP as an interrupt input
	IERG3810_PS2key_ExtiInit();
	IERG3810_TIM3_Init(4999, 7199);
	IERG3810_clock_tree1_init();
	IERG3810_USART3_init(36,9600);
	IERG3810_JOYSTICK_Init();
	DS0_OFF;
	DS1_OFF;
	
	Delay(1000000);
	IERG3810_TFTLCD_Init();

	srand(HeartBeat);

	LoopIntro();

	Initialize();
	DisplayGame();

	//Game Core
	while(1)
	{
		DisplayGame();
		t.x = 5;
		t.y = 20;
		create_block();	//Mainly to create the next block
		DisplayNextBlock();
		
		while(1)	//the block will keep falling until it reachs the bottom
		{
			Fall();
			DisplayCurrentBlock();
			DisplayPlayground();
			if(isBottom() || Reset())break;
		}
		Stay();
		DisplayPlayground();
		Levelup();
		
		if(isFull())CleanRow(CountRow);
		//CleanRow(isFull());
		if(isTop())t.gameover = 1;

		GameOver();
		currentBlock = nextBlock;	//Let the next block come out to the board
		if(t.gameover == 1)break;
		
		
	}
}