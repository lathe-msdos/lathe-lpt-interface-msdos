#include <conio.h>
#include <bios.h>
#include <stdio.h>
#include <dos.h>
#include <string.h>

#define LPT1 0x378
#define DATA_READY 0x100
#define SETTINGS (0x80 | 0x02)
#define NEMA17_1 4
#define NEMA17_2 4
#define N_STATI 7
#define M_STATI 7

unsigned int nema17_1[4] = {0x0A, 0x06, 0x05, 0x09};
unsigned int nema17_2[4] = {0xA0, 0x60, 0x50, 0x90};
unsigned int stepper1[N_STATI] = {0x01, 0x03, 0x06, 0x04, 0x0C, 0x08, 0x09};
unsigned int stepper2[M_STATI] = {0x10, 0x30, 0x60, 0x40, 0xC0, 0x80, 0x90};
unsigned int ultimostato = 0x01;
int MOD = 7;

unsigned int get_stato(int indice_motorino) {
    if (indice_motorino == 1) return ultimostato & 0x0F;
    if (indice_motorino == 2) return ultimostato & 0xF0;
    return 0;
}

void move(int indice_motorino, int direzione) {
    clrscr();
    textbackground(233);
    textcolor(3);
    int indice = 0;
    unsigned int *stepper;
    unsigned int stato = get_stato(indice_motorino);

    if (indice_motorino == 1 && MOD == 7) stepper = stepper1;
    else if (indice_motorino == 2 && MOD == 7) stepper = stepper2;
    else if (indice_motorino == 1 && MOD == 4) stepper = nema17_1;
    else if (indice_motorino == 2 && MOD == 4) stepper = nema17_2;

    for (int i = 0; i < N_STATI; i++) {
        if (stato == stepper[i]) {
            indice = (direzione == 1) ? (i + 1) % MOD : (i - 1 + MOD) % MOD;
            printf("################\n");
            printf("# i (STATUS REACHED) = %d       #\n", i);
            printf("# index (related the engine state) = %d  #\n", indice);
            printf("################\n");
            break;
        }
    }

    unsigned int mask = (indice_motorino == 1) ? 0x0F : 0xF0;
    unsigned int newvalue = stepper[indice];
    unsigned int value = (stato & ~mask) | (newvalue & mask);

    printf("\n value = 0x%X \n", value);
    outportb(LPT1, value);
    ultimostato = value;
    delay(5);
    clrscr();
}

void move_k_times(int indice_motorino, int direzione, int k) {
    clrscr();
    textbackground(233);
    textcolor(3);
    for (int x = 0; x < k; x++) {
        while (inportb(LPT1 + 1) == 0xF8) {
            printf("\n### EMERGENCY BLOCK ON###\n");
            delay(100);
            clrscr();
        }
        move(indice_motorino, direzione);
        printf("Steps-done = %d, Stpr = %d, direction = %d\n", x + 1, indice_motorino, direzione);
    }
}

void scanner() {
    clrscr();
    textbackground(233);
    textcolor(3);
    printf("\n ### If Leds are Blinking means that connection is fine! Press any key to stop the scanning... ###\n");
    printf("\n ### This check will overheat motor, stop it asap ###\n");
    while (!kbhit()) {
        outportb(LPT1, 127);
        delay(10);
        outportb(LPT1, 0);
        delay(10);
    }
}

void main() {
    clrscr();
    textbackground(233);
    textcolor(3);
    printf("\n Welcome to STPMTR-XY v1.2, type help to show the command list\n\n");
    printf("\n STEPPER MODEL >>> %s <<<\n\n", (MOD == 4) ? "NEMA" : "28BYJ-48");

    while (1) {
        int stepper, times, direzione;
        char command[15];

        printf("Command line: ");
        scanf("%s", command);

        if (strcmp(command, "help") == 0) {
            printf("\n#### STPMTR-XY v1.2 Commands ####\n");
            printf("scanner - test connection with LED blinking\n");
            printf("move - move a stepper motor forward/backward\n");
            printf("read - read LPT status pin\n");
            printf("exit - exit to MS-DOS\n");
            printf("feedback - last motor state\n");
            printf("setup - switch stepper model\n");
        } else if (strcmp(command, "scanner") == 0) {
            scanner();
        } else if (strcmp(command, "move") == 0) {
            printf("Which stepper X=1 or Y=2: ");
            scanf("%d", &stepper);
            if (stepper > 2) {
                printf("Out of range.\n");
                continue;
            }
            printf("\n Direction Clockwise=1, Counter=-1: ");
            scanf("%d", &direzione);
            printf("\n How many k-times (steps): ");
            scanf("%d", &times);
            move_k_times(stepper, direzione, times);
        } else if (strcmp(command, "read") == 0) {
            unsigned int status = inportb(LPT1 + 1);
            printf("LPT1+1 = 0x%X\n", status);
            if (status == 0x78) printf("Status: AVAILABLE\n");
            else if (status == 0xF8) printf("Status: EMERGENCY BLOCK ON\n");
            else printf("Status: UNKNOWN\n");
        } else if (strcmp(command, "setup") == 0) {
            int val;
            printf("\n | NEMA = 4 | 28BYJ-48 = 7 | : ");
            scanf("%d", &val);
            MOD = (val == 4 || val == 7) ? val : MOD;
            printf("\n New stepper model: %s\n", (MOD == 4) ? "NEMA" : "28BYJ-48");
        } else if (strcmp(command, "feedback") == 0) {
            printf("Last state = 0x%X\n", ultimostato);
        } else if (strcmp(command, "exit") == 0) {
            return;
        } else {
            printf("\n ### Type 'help' to see the command list. ###\n\n");
        }

        textbackground(233);
        textcolor(3);
    }
}
