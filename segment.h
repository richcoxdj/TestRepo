/* 
 * File:   segment.h
 * Author: Rich
 *
 * Created on September 10, 2013, 5:58 PM
 */
/******************************************************************************/
/* User Level #define Macros                                                  */
/******************************************************************************/

// Debounce and auto repeat for keys
#define KEYCOUNT      50              // Debounce counter to report a key press
#define KEYCOUNT_REP  KEYCOUNT+800   // Autorepeat

#define SEG7_0  0
#define SEG7_1  1
#define SEG7_2  2
#define SEG7_3  3
#define SEG7_4  4
#define SEG7_5  5
#define SEG7_6  6
#define SEG7_7  7
#define SEG7_8  8
#define SEG7_9  9
#define SEG7_A  10
#define SEG7_B  11
#define SEG7_C  12
#define SEG7_D  13
#define SEG7_E  14
#define SEG7_F  15
#define SEG7_c  16
#define SEG7_G  17
#define SEG7_H  18
#define SEG7_I  19
#define SEG7_i  20
#define SEG7_J  21
#define SEG7_L  22
#define SEG7_N  23
#define SEG7_n  24
#define SEG7_O  25
#define SEG7_P  26
#define SEG7_R  27
#define SEG7_S  28
#define SEG7_T  29
#define SEG7_U  30
#define SEG7_u  31
#define SEG7_SP  32
#define SEG7_UNDERLINE  33
#define SEG7_DASH  34
#define SEG7_OVERLINE  35
#define SEG7_EQUAL  36
#define SEG7_3BARS  37
#define SEG7_END 99
#define SEG7_DP 0x80

/******************************************************************************/
/* User Function Prototypes                                                   */
/******************************************************************************/

extern uint16_t DMX_3, DMX_2, DMX_1;

void OutputSeg(uint8_t value, uint8_t point);
uint8_t UpdateDisplay(void);
uint8_t get_key(uint8_t pb);
void menu(void);
void DisableStartUp(void);
uint8_t GetMenuPage(void);
uint8_t GetSubPage(void);

