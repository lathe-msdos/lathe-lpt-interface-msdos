/*
 * STPMTR-XY v2.1  -  Lathe Stepper Controller
 * =============================================
 * Axes:  Motor 1 = Z (carriage / longitudinal feed)
 *        Motor 2 = X (cross slide / radial feed)
 * Port:  LPT1 (0x378)
 *        Low  nibble (bits 0-3) = Motor Z
 *        High nibble (bits 4-7) = Motor X
 *
 * MICROSTEPPING NOTE:
 *   Full (1/1) and Half (1/2) tables are pre-filled for NEMA17.
 *   Quarter (1/4), Eighth (1/8), Sixteenth (1/16) tables are
 *   intentionally left empty -- YOU must fill them in based on
 *   the actual driver board wiring (A4988, DRV8825, TMC2208...).
 *
 * BUG FIX vs v1.x:
 *   The original move() used (stato & ~mask) which wiped the other
 *   motor's bits. Fixed to (ultimostato & ~mask).
 */

#include <conio.h>
#include <bios.h>
#include <stdio.h>
#include <dos.h>
#include <string.h>

#define LPT1     0x378
#define VERSION  "2.1"

/* =========================================================
   STEP TABLES
   Each table encodes the coil activation sequence.
   Motor 1 = low nibble (0x0_), Motor 2 = high nibble (0x_0).
   ========================================================= */

/* --- NEMA17 Full step (1/1) --- */
#define NEMA_FULL_N  4
unsigned int nema_full_1[NEMA_FULL_N] = {0x0A, 0x06, 0x05, 0x09};
unsigned int nema_full_2[NEMA_FULL_N] = {0xA0, 0x60, 0x50, 0x90};

/* --- NEMA17 Half step (1/2) --- */
#define NEMA_HALF_N  8
unsigned int nema_half_1[NEMA_HALF_N] = {0x08,0x0A,0x02,0x06,0x04,0x05,0x01,0x09};
unsigned int nema_half_2[NEMA_HALF_N] = {0x80,0xA0,0x20,0x60,0x40,0x50,0x10,0x90};

/* --- NEMA17 Quarter step (1/4) --- USER MUST FILL IN --- */
#define NEMA_QUARTER_N  16
unsigned int nema_quarter_1[NEMA_QUARTER_N] = {
    /* TODO: fill with 1/4 microstepping values for your driver */
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
unsigned int nema_quarter_2[NEMA_QUARTER_N] = {
    /* TODO */
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/* --- NEMA17 Eighth step (1/8) --- USER MUST FILL IN --- */
#define NEMA_EIGHTH_N  32
unsigned int nema_eighth_1[NEMA_EIGHTH_N] = {
    /* TODO */
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
unsigned int nema_eighth_2[NEMA_EIGHTH_N] = {
    /* TODO */
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/* --- NEMA17 Sixteenth step (1/16) --- USER MUST FILL IN --- */
#define NEMA_16TH_N  64
unsigned int nema_16th_1[NEMA_16TH_N];   /* TODO: fill in */
unsigned int nema_16th_2[NEMA_16TH_N];   /* TODO: fill in */

/* --- 28BYJ-48 (7-step half-wave drive) --- */
#define BYJ_N  7
unsigned int byj_1[BYJ_N] = {0x01,0x03,0x06,0x04,0x0C,0x08,0x09};
unsigned int byj_2[BYJ_N] = {0x10,0x30,0x60,0x40,0xC0,0x80,0x90};

/* =========================================================
   GLOBAL STATE
   ========================================================= */
unsigned int ultimostato = 0x01;
int MOTOR_MODEL = 0;    /* 0 = NEMA17,  1 = 28BYJ-48 */
int MICROSTEP   = 1;    /* 1/2/4/8/16  (NEMA17 only) */
int STEP_DELAY  = 5;    /* ms between steps            */
long pos_z = 0;         /* Z axis position (steps)     */
long pos_x = 0;         /* X axis position (steps)     */

/* =========================================================
   HELPER: get step table for motor + current config
   ========================================================= */
void get_table(int motor, unsigned int **tbl, int *n) {
    if (MOTOR_MODEL == 1) {
        *tbl = (motor == 1) ? byj_1 : byj_2;
        *n   = BYJ_N;
        return;
    }
    switch (MICROSTEP) {
        case 2:
            *tbl = (motor == 1) ? nema_half_1    : nema_half_2;
            *n   = NEMA_HALF_N;    break;
        case 4:
            *tbl = (motor == 1) ? nema_quarter_1 : nema_quarter_2;
            *n   = NEMA_QUARTER_N; break;
        case 8:
            *tbl = (motor == 1) ? nema_eighth_1  : nema_eighth_2;
            *n   = NEMA_EIGHTH_N;  break;
        case 16:
            *tbl = (motor == 1) ? nema_16th_1    : nema_16th_2;
            *n   = NEMA_16TH_N;   break;
        case 1:
        default:
            *tbl = (motor == 1) ? nema_full_1    : nema_full_2;
            *n   = NEMA_FULL_N;    break;
    }
}

/* =========================================================
   HEADER (redrawn after each command)
   ========================================================= */
void print_header() {
    printf("\n");
    textbackground(0);
    textcolor(11);
    printf("============================================\n");
    printf("  STPMTR-XY v%s   |   Lathe Controller\n", VERSION);
    printf("============================================\n");
    textcolor(14);
    printf("  Model : %-10s  Microstep : 1/%d\n",
           MOTOR_MODEL ? "28BYJ-48" : "NEMA17", MICROSTEP);
    printf("  Delay : %-4d ms       State    : 0x%02X\n",
           STEP_DELAY, ultimostato);
    textcolor(10);
    printf("  Z (carriage)   = %ld steps\n", pos_z);
    printf("  X (cross slide)= %ld steps\n", pos_x);
    textcolor(11);
    printf("============================================\n\n");
    textcolor(7);
}

/* =========================================================
   EMERGENCY CHECK  (returns 1 if E-stop is active)
   De-energises coils immediately on trigger.
   ========================================================= */
int check_emergency() {
    if (inportb(LPT1 + 1) == 0xF8) {
        outportb(LPT1, 0);      /* de-energise all coils immediately */
        ultimostato = 0;
        textcolor(12);
        printf("\n  *** EMERGENCY STOP - COILS OFF ***\n");
        printf("  Release E-stop to resume.\n");
        textcolor(7);
        return 1;
    }
    return 0;
}

/* =========================================================
   MOVE (single step)
   ========================================================= */
void move_one(int motor, int dir) {
    unsigned int *tbl;
    int n, idx = 0;
    unsigned int mask = (motor == 1) ? 0x0F : 0xF0;
    unsigned int stato = ultimostato & mask;

    get_table(motor, &tbl, &n);

    for (int i = 0; i < n; i++) {
        if (stato == tbl[i]) {
            idx = (dir == 1) ? (i + 1) % n : (i - 1 + n) % n;
            break;
        }
    }

    /* FIX: preserve the OTHER motor's bits using ultimostato, not stato */
    unsigned int value = (ultimostato & ~mask) | (tbl[idx] & mask);
    outportb(LPT1, value);
    ultimostato = value;
    delay(STEP_DELAY);

    if (motor == 1) pos_z += dir;
    else            pos_x += dir;
}

/* =========================================================
   MOVE N STEPS  (with E-stop and keypress abort)
   ========================================================= */
void move_k(int motor, int dir, long k) {
    long done;
    print_header();
    textcolor(14);
    printf("Moving %s | %s | %ld steps\n",
           motor == 1 ? "Z carriage" : "X cross slide",
           dir == 1   ? "CW" : "CCW", k);
    printf("Press any key to abort\n\n");
    textcolor(7);

    for (done = 0; done < k; done++) {
        if (kbhit()) { getch(); break; }
        while (check_emergency()) { delay(200); }
        move_one(motor, dir);
        if (done % 20 == 0)
            printf("\rStep %ld / %ld   Z=%-8ld  X=%-8ld", done+1, k, pos_z, pos_x);
    }
    printf("\n\nDone (%ld steps).\n", done);
}

/* =========================================================
   SCANNER  (LED blink test)
   ========================================================= */
void scanner() {
    clrscr();
    textcolor(14);
    printf("\n### LED Blink Test ###\n");
    printf("If LEDs blink, LPT connection is OK.\n");
    printf("WARNING: keeps coils energised - stop ASAP!\n");
    printf("Press any key to stop...\n\n");
    while (!kbhit()) {
        outportb(LPT1, 0xFF); delay(120);
        outportb(LPT1, 0x00); delay(120);
    }
    getch();
    outportb(LPT1, 0);
}

/* =========================================================
   MAIN
   ========================================================= */
void main() {
    char cmd[20];

    print_header();
    printf("Type 'help' for command list.\n\n");

    while (1) {
        textcolor(11);
        printf("STPMTR> ");
        textcolor(7);
        scanf("%s", cmd);

        /* ---- HELP ---- */
        if (strcmp(cmd, "help") == 0) {
            textcolor(14);
            printf("\n=== Commands ===\n\n");
            printf("MOTION\n");
            printf("  move        Move Z or X axis (steps, direction)\n");
            printf("  goto        Move to absolute position (steps from home)\n");
            printf("  home        Reset position counters to Z=0, X=0\n");
            printf("  pos         Show current position\n");
            printf("POWER\n");
            printf("  coilsoff    De-energise all coils (motor free)\n");
            printf("  coilson     Re-energise coils (restore last state)\n");
            printf("DIAGNOSTICS\n");
            printf("  scanner     LED blink connection test\n");
            printf("  read        Read LPT status pin\n");
            printf("  feedback    Last motor state (hex)\n");
            printf("CONFIG\n");
            printf("  setup       Switch motor model (NEMA17 / 28BYJ-48)\n");
            printf("  microstep   Set microstepping 1/2/4/8/16 (NEMA17 only)\n");
            printf("  speed       Set step delay in ms\n");
            printf("  exit        Exit to MS-DOS\n\n");
            textcolor(7);
            printf("Press any key to continue...\n");
            getch();

        /* ---- MOVE ---- */
        } else if (strcmp(cmd, "move") == 0) {
            int motor, dir;
            long steps;
            printf("Axis  Z=1 (carriage)  X=2 (cross slide): ");
            scanf("%d", &motor);
            if (motor < 1 || motor > 2) { printf("Invalid axis.\n"); goto next; }
            printf("Direction  CW=1  CCW=-1: ");
            scanf("%d", &dir);
            if (dir != 1 && dir != -1) { printf("Invalid direction.\n"); goto next; }
            printf("Steps: ");
            scanf("%ld", &steps);
            move_k(motor, dir, steps);

        /* ---- GOTO (absolute position) ---- */
        } else if (strcmp(cmd, "goto") == 0) {
            int motor;
            long target, delta, dir;
            printf("Axis Z=1 X=2: ");
            scanf("%d", &motor);
            if (motor < 1 || motor > 2) { printf("Invalid.\n"); goto next; }
            printf("Target position (steps from home): ");
            scanf("%ld", &target);
            delta = target - (motor == 1 ? pos_z : pos_x);
            if (delta == 0) { printf("Already at target.\n"); goto next; }
            dir = (delta > 0) ? 1 : -1;
            if (delta < 0) delta = -delta;
            move_k(motor, (int)dir, delta);

        /* ---- HOME ---- */
        } else if (strcmp(cmd, "home") == 0) {
            pos_z = 0; pos_x = 0;
            printf("Position reset: Z=0, X=0\n");

        /* ---- COILS OFF ---- */
        } else if (strcmp(cmd, "coilsoff") == 0) {
            outportb(LPT1, 0);
            printf("Coils de-energised. Motor shaft is free.\n");

        /* ---- COILS ON ---- */
        } else if (strcmp(cmd, "coilson") == 0) {
            outportb(LPT1, ultimostato);
            printf("Coils re-energised (state=0x%02X).\n", ultimostato);

        /* ---- POS ---- */
        } else if (strcmp(cmd, "pos") == 0) {
            printf("Z (carriage)    = %ld steps\n", pos_z);
            printf("X (cross slide) = %ld steps\n", pos_x);

        /* ---- SCANNER ---- */
        } else if (strcmp(cmd, "scanner") == 0) {
            scanner();

        /* ---- READ ---- */
        } else if (strcmp(cmd, "read") == 0) {
            unsigned int s = inportb(LPT1 + 1);
            printf("LPT1+1 = 0x%X  ->  ", s);
            if      (s == 0x78) printf("AVAILABLE\n");
            else if (s == 0xF8) printf("EMERGENCY STOP ON\n");
            else                printf("UNKNOWN (0x%X)\n", s);

        /* ---- FEEDBACK ---- */
        } else if (strcmp(cmd, "feedback") == 0) {
            printf("State=0x%02X  Z=%ld  X=%ld\n", ultimostato, pos_z, pos_x);

        /* ---- SETUP ---- */
        } else if (strcmp(cmd, "setup") == 0) {
            printf("Motor model  NEMA17=0  28BYJ-48=1 : ");
            scanf("%d", &MOTOR_MODEL);
            if (MOTOR_MODEL != 0 && MOTOR_MODEL != 1) MOTOR_MODEL = 0;
            MICROSTEP = 1;
            printf("Model set: %s\n", MOTOR_MODEL ? "28BYJ-48" : "NEMA17");

        /* ---- MICROSTEP ---- */
        } else if (strcmp(cmd, "microstep") == 0) {
            if (MOTOR_MODEL == 1) {
                printf("28BYJ-48 does not support microstepping.\n");
            } else {
                int ms;
                printf("1=Full  2=Half  4=Quarter  8=Eighth  16=Sixteenth : ");
                scanf("%d", &ms);
                if (ms==1||ms==2||ms==4||ms==8||ms==16) {
                    MICROSTEP = ms;
                    printf("Microstepping set to 1/%d\n", ms);
                    if (ms >= 4)
                        printf("NOTE: fill in step table for 1/%d in source!\n", ms);
                } else {
                    printf("Invalid. Use 1/2/4/8/16.\n");
                }
            }

        /* ---- SPEED ---- */
        } else if (strcmp(cmd, "speed") == 0) {
            printf("Step delay ms (now %d ms): ", STEP_DELAY);
            scanf("%d", &STEP_DELAY);
            if (STEP_DELAY < 1) STEP_DELAY = 1;
            printf("Delay = %d ms\n", STEP_DELAY);

        /* ---- EXIT ---- */
        } else if (strcmp(cmd, "exit") == 0) {
            outportb(LPT1, 0);
            return;

        } else {
            printf("Unknown command. Type 'help'.\n");
        }

        next:
        printf("\n");
    }
}
