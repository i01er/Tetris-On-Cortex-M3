#ifndef __tetris_h
#define __tetris_h

//tetris.h
void GameOver(); //Done
void Initialize(); //Done
void create_block(); //Done?
void LoopIntro(); //Done

void Fall();
int isBottom(); //Done
int isFull();
void CleanRow();
int isTop();
void Stay();

int Reset(); //Done

void Levelup();

void Current();

void int2str(int i, char*s); //Done
void CleanArray(); //Done
void CleanCurrent(); //Done
void Delay(u32 count);

void Control(u8 Recv);
void Left();
void Right();
void Down();
void Rotate();

//Display Function
void Copy2Master();
void DisplayLogo(); //Done
void DisplayBG();
void DisplayGame(); //Done
void DisplayCurrentBlock(); //Done

void DisplayIntro(); //Done
void DisplayNextBlock(); //Done
void DisplayPlayground();//Done

//GameOver
void Win();  //Done
void Lose(); //Done

void IERG3810_TFTLCD_SetParameter();
void IERG3810_TFTLCD_WrReg(u16);
void IERG3810_TFTLCD_WrData(u16);

#endif