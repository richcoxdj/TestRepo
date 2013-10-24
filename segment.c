/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

#if defined(__XC)
#include <xc.h>         /* XC8 General Include File */
#elif defined(HI_TECH_C)
#include <htc.h>        /* HiTech General Include File */
#elif defined(__18CXX)
#include <p18cxxx.h>    /* C18 General Include File */
#endif

#if defined(__XC) || defined(HI_TECH_C)

#include <stdint.h>         /* For uint8_t definition */
#include <stdbool.h>        /* For true/false definition */

#endif

#include "system.h"
#include "segment.h"
#include "dmx.h"
#include "ledstrobe.h"

extern uint8_t firmware[3];
extern uint16_t DMX_ADDR;
extern uint8_t function;
extern uint8_t pattern;
extern uint8_t testchannel;

uint8_t temp_function;
uint8_t temp_pattern;

uint8_t MenuPage;
uint8_t SubPage;
uint8_t InStartUp = 0;

uint8_t Display[20];

uint8_t DigitCount = 0;
uint8_t RefeshDisplay = 1;
uint16_t nexttick;

/******************************************************************************/
/* 7 Segment Functions                                                        */
/******************************************************************************/

/*   Patten Table for 7 segment display.
 *   Segments A->D are on port A while
 *   Segments E->G are on port B.
 *
 *          AAA
 *         F   B
 *         F   B
 *          GGG
 *         E   C
 *         E   C
 *          DDD
 *               P
 */

const uint8_t pat7seg[38] = {
    //PABCDEFG
    0b01111110, // 0    0
    0b00110000, // 1    1
    0b01101101, // 2    2
    0b01111001, // 3    3
    0b00110011, // 4    4
    0b01011011, // 5    5
    0b01011111, // 6    6
    0b01110000, // 7    7
    0b01111111, // 8    8
    0b01110011, // 9    9
    0b01110111, // A    10
    0b00011111, // B    11
    0b01001110, // C    12
    0b00111101, // D    13
    0b01001111, // E    14
    0b01000111, // F    15
    0b00001101, // c    16
    0b01011111, // G    17
    0b00110111, // H    18
    0b00110000, // I    19
    0b00010000, // i    20
    0b00111100, // J    21
    0b00001110, // L    22
    0b01110110, // N    23
    0b00010101, // n    24
    0b00011101, // o    25
    0b01100111, // P    26
    0b00000101, // r    27
    0b01011011, // S    28
    0b00001111, // t    29
    0b00111110, // U    30
    0b00011100, // u    31
    0b00000000, // SPACE 32
    0b00001000, // _    33
    0b00000001, // -    34
    0b01000000, // ~    35
    0b00001001, // =    36
    0b01001001 //  =~   37

};

void OutputSeg(uint8_t value, uint8_t point) {
    LATA = (PORTA & 0xF0) + (pat7seg[value]& 0x0F);
    if (point) {
        LATB = (PORTB & 0xF0) + ((pat7seg[value]& 0xF0) >> 4) + 0x08;
    } else {
        LATB = (PORTB & 0xF0) + ((pat7seg[value]& 0xF0) >> 4);
    }
}

uint8_t UpdateDisplay(void) {
    // Update Display and read input Push Buttons
    uint8_t pb = 0;
    uint8_t x;
    uint16_t tick;

    x = DigitCount;

    if (RefeshDisplay) {
        x = 0;
        nexttick = dmx_tick_count();
        nexttick += 900;
        DigitCount = 0;
    }

    OutputSeg(Display[x] & 0x7F, Display[x] & 0x80);
    LATG = 0b00011000;
    __delay_us(70);
    pb |= (PORTAbits.RA4 << 3);
    LATG = 0b00010000;
    x++;

    OutputSeg(Display[x] & 0x7F, Display[x] & 0x80);
    LATG = 0b00010100;
    __delay_us(70);
    pb |= (PORTAbits.RA4 << 2);
    LATG = 0b00010000;
    x++;

    OutputSeg(Display[x] & 0x7F, Display[x] & 0x80);
    LATG = 0b00010010;
    __delay_us(70);
    pb |= (PORTAbits.RA4 << 1);
    LATG = 0b00010000;
    x++;

    OutputSeg(Display[x] & 0x7F, Display[x] & 0x80);
    LATG = 0b00010001;
    __delay_us(70);
    pb |= PORTAbits.RA4;
    LATG = 0b00010000;
    x++;

    DigitCount = x - 4;
    tick = dmx_tick_count();

    if (Display[x] != SEG7_END) {
        if (nexttick < tick) {
            nexttick = dmx_tick_count();
            nexttick += 250;
            DigitCount = x - 3;
        }
    } else {
        if (nexttick + 750 < tick) {
            nexttick = dmx_tick_count();
            nexttick += 900;
            DigitCount = 0;

        }

    }
    RefeshDisplay = 0;

    return (~pb & 0x0F);
}

/******************************************************************************
 * Function: uint8_t get_key(void)
 *
 * Overview: Debounce and report what key is pressed
 *
 * Input:    none
 *
 * Output:   None
 *
 ******************************************************************************/
uint16_t SW1 = 0, SW2 = 0, SW3 = 0, SW4 = 0;

uint8_t get_key(uint8_t pb) {

    uint8_t retval = 0;

    if (pb & 0x01)SW1++;
    else SW1 = 0;
    if (pb & 0x02)SW2++;
    else SW2 = 0;
    if (pb & 0x04)SW3++;
    else SW3 = 0;
    if (pb & 0x08)SW4++;
    else SW4 = 0;

    // First press
    if (SW1 == KEYCOUNT)retval |= 0x01;
    if (SW2 == KEYCOUNT)retval |= 0x02;
    if (SW3 == KEYCOUNT)retval |= 0x04;
    if (SW4 == KEYCOUNT)retval |= 0x08;

    // Auto repeat
    if (SW1 > KEYCOUNT_REP) {
        SW1 = KEYCOUNT;
        retval |= 0x01;
    }
    if (SW2 > KEYCOUNT_REP) {
        SW2 = KEYCOUNT;
        retval |= 0x02;
    }
    if (SW3 > KEYCOUNT_REP) {
        SW3 = KEYCOUNT;
        retval |= 0x04;
    }
    if (SW4 > KEYCOUNT_REP) {
        SW4 = KEYCOUNT;
        retval |= 0x08;
    }

    return (retval);

}

void menu(void) {
    int key = 0;
    int data, d3, d2, d1;

    if (MenuPage == 4) {
        data = dmx_read_byte(testchannel);
    }
    if (MenuPage == 1) {
        data = DMX_ADDR;
    }
    if (MenuPage == 2) {
        data = function;
    }
    d1 = data % 10;
    data = data / 10;
    d2 = data % 10;
    data = data / 10;
    d3 = data % 10;

    if (InStartUp) {
        MenuPage = 0;
        SubPage = 4;
        UpdateDisplay();
    } else {
        key = get_key(UpdateDisplay());
    }
    switch (MenuPage) {
        case 0: //Main Menu
            if (SubPage == 0) { // Address = XXX
                Display[0] = SEG7_A;
                Display[1] = SEG7_D;
                Display[2] = SEG7_D;
                Display[3] = SEG7_R;
                Display[4] = SEG7_E;
                Display[5] = SEG7_S;
                Display[6] = SEG7_S;
                Display[7] = SEG7_SP;
                Display[8] = SEG7_EQUAL;
                Display[9] = SEG7_SP;
                Display[10] = DMX_3 / 100;
                Display[11] = DMX_2 / 10;
                Display[12] = DMX_1;
                Display[13] = SEG7_END;
            }

            if (SubPage == 1) { // Function
                Display[0] = SEG7_F;
                Display[1] = SEG7_u;
                Display[2] = SEG7_n;
                Display[3] = SEG7_c;
                Display[4] = SEG7_T;
                Display[5] = SEG7_i;
                Display[6] = SEG7_O;
                Display[7] = SEG7_n;
                Display[8] = SEG7_SP;
                Display[9] = SEG7_EQUAL;
                Display[10] = SEG7_SP;
                Display[11] = function / 10;
                Display[12] = function % 10;
                Display[13] = SEG7_END;

            }

            if (SubPage == 2) { // Test Pattern
                Display[0] = SEG7_T;
                Display[1] = SEG7_E;
                Display[2] = SEG7_S;
                Display[3] = SEG7_T;
                Display[4] = SEG7_SP;
                Display[5] = SEG7_P;
                Display[6] = SEG7_A;
                Display[7] = SEG7_T;
                Display[8] = SEG7_T;
                Display[9] = SEG7_E;
                Display[10] = SEG7_R;
                Display[11] = SEG7_n;
                Display[12] = SEG7_SP;
                Display[13] = SEG7_END;
            }

            if (SubPage == 3) { // DMX Value
                Display[0] = SEG7_C;
                Display[1] = SEG7_H;
                Display[2] = SEG7_A;
                Display[3] = SEG7_N;
                Display[4] = SEG7_N;
                Display[5] = SEG7_E;
                Display[6] = SEG7_L;
                Display[7] = SEG7_SP;
                Display[8] = SEG7_D;
                Display[9] = SEG7_A;
                Display[10] = SEG7_T;
                Display[11] = SEG7_A;
                Display[12] = SEG7_END;
            }

            if (SubPage == 4) { // Firmware
                Display[0] = SEG7_C;
                Display[1] = SEG7_O;
                Display[2] = SEG7_D;
                Display[3] = SEG7_E;
                Display[4] = SEG7_SP;
                Display[5] = SEG7_F;
                Display[6] = firmware[0] + SEG7_DP;
                Display[7] = firmware[1];
                Display[8] = firmware[2];
                Display[9] = SEG7_END;
            }
            break;
        case 1: // Set Address
            if (SubPage == 0) { // AXXX
                Display[0] = SEG7_A;
                Display[1] = d3;
                Display[2] = d2;
                Display[3] = d1;
                Display[4] = SEG7_END;
            }
            if (SubPage == 1) { // done
                Display[0] = SEG7_D;
                Display[1] = SEG7_O;
                Display[2] = SEG7_n;
                Display[3] = SEG7_E;
                Display[4] = SEG7_END;
            }
            break;
        case 2: // Set Function
            if (SubPage == 0) {
                Display[0] = SEG7_F;
                Display[1] = SEG7_SP;
                Display[2] = temp_function / 10;
                Display[3] = temp_function % 10;
                ;
                Display[4] = SEG7_END;
            }
            if (SubPage == 1) { // done
                Display[0] = SEG7_D;
                Display[1] = SEG7_O;
                Display[2] = SEG7_n;
                Display[3] = SEG7_E;
                Display[4] = SEG7_END;
            }
            break;
        case 3: // Set Test Pattern
            if (SubPage == 0) {
                Display[0] = SEG7_T;
                Display[1] = SEG7_P;
                Display[2] = temp_pattern / 10;
                Display[3] = temp_pattern % 10;
                Display[4] = SEG7_END;
            }
            if (SubPage == 1) {
                Display[0] = SEG7_R;
                Display[1] = SEG7_U;
                Display[2] = SEG7_N;
                Display[3] = SEG7_N;
                Display[4] = SEG7_I;
                Display[5] = SEG7_N;
                Display[6] = SEG7_G;
                Display[7] = SEG7_SP;
                Display[8] = SEG7_T;
                Display[9] = SEG7_P;
                Display[10] = pattern / 10;
                Display[11] = pattern % 10;
                Display[12] = SEG7_END;
            }
            break;
        case 4: // DMX Tester
            if (SubPage == 0) {
                Display[0] = SEG7_C;
                Display[1] = SEG7_H;
                Display[2] = testchannel / 10;
                Display[3] = testchannel % 10;
                Display[4] = SEG7_END;
            }
            if (SubPage == 1) {
                Display[0] = SEG7_D;
                Display[1] = d3;
                Display[2] = d2;
                Display[3] = d1;
                Display[4] = SEG7_END;
            }
            break;
    }
    switch (key) {
        default: // Nothing pressed
            break;

        case 0x08: // Left/Esc
            switch (MenuPage) {
                case 0: // Main Menu
                    if (SubPage > 0) {
                        SubPage = 0;
                        RefeshDisplay = 1;
                        break;
                    }
                    break;
                case 1: //DMX Address setup
                    if (SubPage == 0) {
                        DMX_ADDR = (DMX_3*100) + (DMX_2*10) + DMX_1;
                        MenuPage = 0;
                        SubPage = 0;
                        RefeshDisplay = 1;
                        break;
                    }
                    if (SubPage == 1) {
                        MenuPage = 0;
                        SubPage = 0;
                        RefeshDisplay = 1;
#asm
                        RESET;
#endasm
                    }
                    break;
                case 2: //Function setup
                    if (SubPage == 0) {
                        function = EEPROM_READ(3);
                        MenuPage = 0;
                        SubPage = 1;
                        RefeshDisplay = 1;
                        break;
                    }
                    if (SubPage == 1) {
                        MenuPage = 0;
                        SubPage = 1;
                        RefeshDisplay = 1;
                        break;
                    }
                    break;
                case 3: //Pattern set
                    if (SubPage == 0) {
                        pattern = EEPROM_READ(4);
                        MenuPage = 0;
                        SubPage = 2;
                        RefeshDisplay = 1;
                        break;
                    }
                    if (SubPage == 1) {
                        MenuPage = 3;
                        SubPage = 0;
                        RefeshDisplay = 1;
                    }
                    break;
                case 4: // DMX Tester
                    if (SubPage == 0) {
                        MenuPage = 0;
                        SubPage = 3;
                        RefeshDisplay = 1;
                        break;
                    }
                    if (SubPage == 1) {
                        MenuPage = 4;
                        SubPage = 0;
                        RefeshDisplay = 1;
                    }
                    break;
            }
            break;
        case 0x04: // Down
            switch (MenuPage) {
                case 0: // Main Menu
                    if (SubPage) {
                        SubPage--;
                        RefeshDisplay = 1;
                    }
                    break;

                case 1: // DMX Address setup
                    if (SubPage == 0) {
                        // Decrement - Limit appropriately
                        if (DMX_ADDR > 1)DMX_ADDR--;
                    }
                    break;
                case 2: // Function setup
                    if (SubPage == 0) {
                        // Decrement - Limit appropriately
                        if (temp_function > 0)temp_function--;
                        if (temp_function == 98)temp_function = 90;
                        if (temp_function == 89)temp_function = 30;
                        if (temp_function == 29)temp_function = 24;
                        if (temp_function == 19)temp_function = 11;
                        if (temp_function == 9)temp_function = 4;
                    }
                    break;
                case 3: // Test Pattern
                    if (SubPage == 0) {
                        if (temp_pattern)temp_pattern--;
                    }
                    break;
                case 4: // DMX Tester
                    if (SubPage == 0) {
                        if (testchannel > 1)testchannel--;
                    }
                    break;
            }
            break;
        case 0x02: // Up
            switch (MenuPage) {
                case 0: // Main Menu
                    if (SubPage < 4) {
                        SubPage++;
                        RefeshDisplay = 1;
                    }
                    break;
                case 1: // DMX Address setup
                    if (SubPage == 0) {
                        // Increment - Limit appropriately
                        if (DMX_ADDR < 512)DMX_ADDR++;
                    }
                    break;
                case 2: // Function setup
                    if (SubPage == 0) {
                        // Increment - Limit appropriately
                        if (temp_function < 99)temp_function++;
                        if (temp_function == 5)temp_function = 10;
                        if (temp_function == 12)temp_function = 20;
                        if (temp_function == 25)temp_function = 30;
                        if (temp_function == 31)temp_function = 90;
                        if (temp_function == 91)temp_function = 99;
                    }
                    break;
                case 3: // Test Pattern
                    if (SubPage == 0) {
                        if (temp_pattern < 10)temp_pattern++;
                    }
                    break;
                case 4:
                    if (SubPage == 0) {
                        if (testchannel < 64)testchannel++;
                    }
                    break;
            }
            break;
        case 0x01: // Right/Enter
            switch (MenuPage) {
                case 0: // Main Menu
                    if (SubPage < 4) {
                        MenuPage = SubPage + 1;
                        SubPage = 0;
                        temp_function = function;
                        temp_pattern = pattern;
                        RefeshDisplay = 1;
                    }
                    break;
                case 1: // DMX Address setup
                    if (SubPage == 0) {
                        SubPage = 1;
                        RefeshDisplay = 1;

                        DMX_1 = DMX_ADDR % 10;
                        DMX_ADDR = DMX_ADDR / 10;
                        DMX_2 = DMX_ADDR % 10;
                        DMX_ADDR = DMX_ADDR / 10;
                        DMX_3 = DMX_ADDR % 10;
                        EEPROM_WRITE(0, DMX_3); //*100;
                        EEPROM_WRITE(1, DMX_2); //*10;
                        EEPROM_WRITE(2, DMX_1); //*1
                        DMX_ADDR = (DMX_3*100) + (DMX_2*10) + DMX_1;
                        break;

                    }
                    if (SubPage == 1) {
                        MenuPage = 0;
                        SubPage = 0;
                        RefeshDisplay = 1;
                        key = 0;

#asm
                        RESET;
#endasm
                        break;
                    }
                    break;
                case 2: // Function setup
                    if (SubPage == 0) {
                        SubPage = 1;
                        RefeshDisplay = 1;
                        function = temp_function;
                        EEPROM_WRITE(3, function); //*100;
                        break;
                    }
                    if (SubPage == 1) {
                        MenuPage = 0;
                        SubPage = 1;
                        RefeshDisplay = 1;
                        break;
                    }
                    break;
                case 3: // Test Pattern
                    if (SubPage == 0) {
                        SubPage = 1;
                        RefeshDisplay = 1;
                        pattern = temp_pattern;
                        EEPROM_WRITE(4, pattern);
                        TestPattern_Init();
                        break;
                    }
                    break;
                case 4: // DMX Tester
                    if (SubPage == 0) {
                        SubPage = 1;
                        RefeshDisplay = 1;
                        break;
                    }
            }
            break;

    }
}

void DisableStartUp() {
    //Disables the Start Up delay
    InStartUp = 0;
    MenuPage = 0;
    SubPage = 0;
    RefeshDisplay = 1;
}

uint8_t GetMenuPage(void) {
    return MenuPage;
}

uint8_t GetSubPage(void) {
    return SubPage;
}