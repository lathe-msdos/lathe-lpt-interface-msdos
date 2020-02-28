#include<conio.h>
#include<bios.h>
#include<stdio.h>
#include<dos.h>
#include<string.h>


#define LPT1      0x378

#define DATA_READY 0x100
#define SETTINGS ( 0x80 | 0x02 | 0x00 | 0x00)
#define NEMA17_1 4
#define NEMA17_2 4
#define N_STATI 7
#define M_STATI 7
//#define MOD 7
unsigned int nema17_1[4] = {0x0A,0x06, 0x05, 0x09};
unsigned int nema17_2[4] = {0xA0, 0x60, 0x50, 0x90};
int MOD = 7;
//clrscr();
//textbackground(233);
//textclor(3);
unsigned int stepper1[N_STATI]  = {0x01,0x03,0x06,0x04,0x0C,0x08,0x09};
unsigned int stepper2[M_STATI]  = {0x10,0x30,0x60,0x40,0xC0,0x80,0x90};
unsigned int ultimostato= 0x01;

unsigned int get_stato(int indice_motorino){
unsigned int feedback = ultimostato;


if (indice_motorino==1){
return feedback & 0x0F;
}
if (indice_motorino == 2){
  return feedback & 0xF0;
}
   return 0;
}

void move(int indice_motorino, int direzione){
clrscr();
textbackground(233);
textcolor(3);
int indice;
unsigned int *stepper;
unsigned int stato;
if(indice_motorino==1 && MOD==7){
  stepper = stepper1;
}
if ((indice_motorino==2)&& (MOD==7)){
    stepper = stepper2;
}
    if( (indice_motorino==1) && (MOD==4)){
      stepper = nema17_1;
    }
    if( (indice_motorino==2)&& (MOD==4)){
    stepper = nema17_2;
    }
stato = get_stato(indice_motorino);

for(int i=0; i<N_STATI; i++){
  if (stato==stepper[i]){
    if (direzione==1){
    indice = (i+1)%MOD;
  }
      if (direzione==-1){
    indice = ((i-1)%MOD);
    if(indice < 0){
    indice = MOD-1;
    }
      }
//printf("################");
printf("# index (related the engine state) =  %d  # direction = %d  \n",indice,direzione);
//printf("################");
     // printf("index =  %d  \n",indice);
  }
}

indice = indice%MOD;

//printf("\n index =  %d \n",indice);
unsigned int mask;
unsigned int newvalue = stepper[indice];
if(indice_motorino == 1){
  mask=0x0F;
}
if(indice_motorino == 2){
  mask=0xF0;
}
unsigned int value = (stato & ~mask) | (newvalue & mask);
printf("\n value =0x%X \n",value);
outportb(LPT1,value);
ultimostato=value;
delay(5);
clrscr();

}

void move_k_times(int indice_motorino, int direzione, int k ){
 clrscr();
 textbackground(233);
 textcolor(3);
  for(int x=0; x<k; x++){

  while(inportb(LPT1+1)==0xF8){
  textbackground(0);
  textcolor(4);
  printf("###########################\n");
  printf("### EMERGENCY BLOCK ON###\n");
  printf("###########################");
  printf("#\n Stepper_Selected = %d, direction = %d ",indice_motorino,direzione);
  printf("# ");
  printf("#\n States_done = %d , Max = %d ",x,k);
  delay(250);
  clrscr();
   }

move(indice_motorino,direzione);
  printf("Steps_done =  %d ,Stpr = %d, direction = %d    \n",k,indice_motorino,direzione);

  }




}

void scanner(){
clrscr();
textbackground(233);
textcolor(3);
printf("\n ### Leds Blinking! press any key to stop the scanning... ### \n");
  while(!kbhit()){

  outportb(LPT1,127);
  delay(150);
  outportb(LPT1,0);
  delay(150);

  }

}


void main(){
clrscr();
textbackground(233);
textcolor(3);
clrscr();
textbackground(233);
textcolor(3);
printf("\n Welcome to LATHE-XY v1.2, type help to show the command's list\n \n");
    if (MOD == 4) {  printf("\n  \n STEPPER MODEL >>> NEMA <<<  [SELECTED] ");
     printf("\n STEPPER MODEL >>> 28BYJ-48 <<< []");}
      if (MOD == 7) {
      printf("`\n STEPPER MODEL >>> NEMA <<< []");
      printf("\n STEPPER MODEL >>> 28BYJ-48 <<< [SELECTED] \n \n");}

while(1){
int stepper,times,direzione;
char command[15];

printf("Command line: ");

scanf("%s",command);
if(strcmp(command,"help")==0){
//printf("\n###################################################");
//printf("#");
clrscr();
textbackground(233);
textcolor(3);
printf("\n#### Welcome to STPMTR-XY #####  v1.2 ###############\n");
printf("\n# scanner = check if the stepper are linked then blinking");
printf("\n# move = to let a stepper to move forward or back for k-steps\n The Stepper 1 is linked to the first 4 bit and the Stepper 2 is linked to the last 4 bit.");
printf("\n# read = check the state");
printf("\n# exit = to back to ms-dos");
printf("-- version = to check version and authors.\n");
printf("\n# feedback = to check the last state.");
printf("-- lpt1+2, lpt1+3 = check which address is synched.");
printf("\n# clearscreen = erase all the screen.");
printf(" -- nemainfo = info about nema passes.\n");
printf("\n setup = set the motor [ NEMA17 / 28BYJ-48 ].");
}
if (strcmp(command,"scanner")==0) {
scanner();
}
if(strcmp(command,"move")==0) {
  if(inportb(LPT1+1) != 0xF8){printf("\n EMERGENCY BLOCK OFF! That is not safe.\n \n ");}
  printf("Which stepper X=1 or Y=2: ");
  scanf("%d",&stepper);
  if(stepper !=1 && stepper !=2){
  printf("Out of Range.");
  printf("\n Debug Line 185 to add more steppers; \n");
  printf("Please, re launch the app again.");
  return; }
  printf("\n Direction Clock-Foreward = 1, Back = -1: ");
  scanf("%d",&direzione);
  printf("\n How many k-times");

  printf("\n NOTE: 1 Complete round is equal 1 complet step int the screw. For example [ 28BYJ-48] needs 2038 states to advance 1 pass in the screw. \n");
   if(inportb(LPT1+1) != 0xF8){printf("\n EMERGENCY BLOCK OFF! That is not safe.\n \n ");}
   if(inportb(LPT1+1) == 0xF8){printf("\n EMERGENCY BLOCK ON! That is safe :] \n \n ");}
  printf("\n k = ");
  scanf("%d",&times);
  move_k_times(stepper,direzione,times);


}
if(strcmp(command,"read")==0) {
  clrscr();
  printf("LPT1+1 = 0x%X\n ", inportb(LPT1+1));
  if (inportb(LPT1+1)==0x78){
  printf("Status: AVAILABLE\n");   }
  if (inportb(LPT1+1)==0xF8){
  printf("Status: EMERGENCY BLOCK ON\n");}
  if (inportb(LPT1+1)!=0xF8 && inportb(LPT1+1) != 0x78){
  printf("Status: UNKNOWN");}
  if(MOD == 4) {
  printf("The stepper setted up is = %d, NEMA",MOD);}
  if(MOD == 7) { printf("The stepper setted up is = %d,28BYJ",MOD);}
}

if(strcmp(command,"nemainfo")==0){
printf("\n NEMA 17 infos:\n");
delay(10);
printf("\n Set for Nema17\n");
     printf("\n Pin1  : Pin2 : Pin3 :   Resolution N-States x 1 step   \n");
    printf("\n  Low   : Low  : Low  :   1x1 \n");
    printf("\n  High  : Low  : Low  :   2x1 \n");
    printf("\n  Low   : High : Low  :   4x1   [SELECTED] \n");
    printf("\n  High  : High : Low  :   8x1 \n");
    printf("\n  High  : High : High :  16x1 \n");
    printf("\n  To change that selection, a modify in the source code is required\n");

}

    if (strcmp(command,"setup")==0) {

    printf("\n | NEMA = 4 | 28BYJ-48 = 7 | Nothing = another value |: ");
   int val=7;
   scanf("%d",&val);

   if (val == 4) {  printf("\n STEPPER MODEL >>> NEMA <<<  [SELECTED] ");
  printf("\n STEPPER MODEL >>> 28BYJ-48 <<< []");
  MOD=4;}
   if (val == 7) {
   printf("`\n STEPPER MODEL >>> NEMA <<< []");
   printf("\n STEPPER MODEL >>> 28BYJ-48 <<< [SELECTED] ");
   MOD=7;}

   else{ printf("\n Nothing changed \n"); }
   printf("\n MOD = %d",MOD);

    }




if (strcmp(command,"exit")==0) {
return;
}

if (strcmp(command,"feedback")==0) {
//clrscr();
printf("Last state = 0x%X ",ultimostato);
}


if (strcmp(command,"version")==0) {
//clrscr();
printf("\nSTPMTR12 - Step Motor 1 and 2  version 1.2 \nBy Alberto Ragagnin \n Thanks to Riccardo Ragagnin and Antonio Ragagnin \nfor the useful help given to do that project.\n\n ");
printf("\nLast update: February 2020\n\n");
}
if(strcmp(command,"lpt1+2")==0){
//clrscr();
printf("\n LPT1+2 is synched to : 0x%X  \n",inportb(LPT1+2));
}

if(strcmp(command,"lpt1+3")==0){
//clrscr();
printf("\n LPT1+3 is synched to : 0x%X\n",inportb(LPT1+3));


}

if(strcmp(command,"clearscreen")==0){
clrscr();
}
else{printf("\n ### Type 'help' to see the command's list. ###\n \n");}





textbackground(233);
textcolor(3);
}

}


