
//*****************************************************************************
//
// Application Name     - int_sw
// Application Overview - The objective of this application is to demonstrate
//                          GPIO interrupts using SW2 and SW3.
//                          NOTE: the switches are not debounced!
//
//*****************************************************************************

//****************************************************************************
//
//! \addtogroup int_sw
//! @{
//
//****************************************************************************

// Standard includes
#include <stdio.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "hw_apps_rcm.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "prcm.h"
#include "gpio.h"
#include "utils.h"
#include "timer.h"
#include "systick.h"
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

// Common interface includes
#include "uart_if.h"
#include "timer_if.h"
#include "pin_mux_config.h"


//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
extern void (* const g_pfnVectors[])(void);

volatile unsigned long time;
int interruptCounter;
int pulseCounter;
char prevRead, currRead, *buffer;
unsigned long _time;
unsigned long _readTime;
unsigned long timeInterval[35] = {};
unsigned long timeOfInterrupt[35] = {};
unsigned int bitSequence[35] = {};
int readIndex;
int buttons[12][35] = {
                       {0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,1,1,1,1,0,1,1,0,0,0,0,0,1,0,0,1,1,1,1,0}, // 0
                       {0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0}, // 1
                       {0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0}, // 2
                       {0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,1,1,1,0,1,0,0,0,0,0,0,1,0,1,1,1,1,1,1,0}, // 3
                       {0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,1,1,1,0,0,1,0,0,0,0,0,1,1,0,1,1,1,1,1,0}, // 4
                       {0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,1,1,1,1,0,1,0,0,0,0,0,0,1,0,1,1,1,1,1,0}, // 5
                       {0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,1,1,1,0,1,1,0,0,0,0,0,1,0,0,1,1,1,1,1,0}, // 6
                       {0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,1,1,1,0,0,0,1,0,0,0,0,1,1,1,0,1,1,1,1,0}, // 7
                       {0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,1,1,1,1,0,0,1,0,0,0,0,0,1,1,0,1,1,1,1,0}, // 8
                       {0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,1,1,1,0,1,0,1,0,0,0,0,1,0,1,0,1,1,1,1,0}, // 9
                       {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, // DELETE
                       {0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,0} // MUTE
};
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************
// an example of how you can use structs to organize your pin settings for easier maintenance
typedef struct PinSetting {
    unsigned long port;
    unsigned int pin;
} PinSetting;
static PinSetting gpioin = { .port = GPIOA0_BASE, .pin = 0x40 };

//*****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES                           
//*****************************************************************************

static void BoardInit(void);
//*****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS                         
//*****************************************************************************

void
TimerRefIntHandler(void)
{
    //
    // Clear the timer interrupt.
    //
    unsigned long ulInts;
    ulInts = TimerIntStatus(TIMERA0_BASE, true);
    TimerIntClear(TIMERA0_BASE, ulInts);
    _time++;
}

void
ReadTimeIntHandler(void)
{
    //
    // Clear the timer interrupt.
    //
    unsigned long ulInts;
    ulInts = TimerIntStatus(TIMERA0_BASE, true);
    TimerIntClear(TIMERA0_BASE, ulInts);
    _readTime++;
}

int compareTwoSequence(int sequence1[35], int sequence2[35]) {
    int i;
    for (i = 0; i < 35; i++) {
        if (sequence1[i] != sequence2[i])
            return 0;
    }
    return 1;
}

char compareBitPatterns() {
    int row;
    int col;
    for (row = 0; row < 12; row++) {
        if (compareTwoSequence(bitSequence, buttons[row])) {
            if (row == 10)
                return 'D';
            else if (row == 11)
                return 'M';
            else
                return row + '0';
        }
    }
    return 'F';
}

static void GPIOA2IntHandler(void)
{
    unsigned long ulStatus;
    ulStatus = MAP_GPIOIntStatus (gpioin.port, false);
    MAP_GPIOIntClear(gpioin.port, ulStatus);        // clear interrupts on GPIOA2
    // time = TimerValueGet(TIMERA0_BASE, TIMER_A);
    // TimerValueSet(TIMERA0_BASE, TIMER_A, 0);
    time = _time;
    _time = 0;
    int _interrupt = interruptCounter;
    if(interruptCounter < 35)
    {
        bitSequence[interruptCounter] = decode(time);
        timeOfInterrupt[interruptCounter] = time;
        interruptCounter++;
    }
    if(interruptCounter == 35)
    {
        // Report("H: %d\n\r", interruptCounter);
        // Report("%d \n\r", _time);
        // GPIOIntDisable(gpioin.port, gpioin.pin);
        // Don't tick till we read and decode the bits
        TimerDisable(TIMERA0_BASE, TIMER_A);
        // Stop taking ticks too
        // printArr();
        // interruptCounter = 0;
        // initializeArr();
        // GPIOIntEnable(gpioin.port, gpioin.pin);
    }
}

//static void GPIOA2IntHandler(void) {
//    unsigned long ulStatus;
//    ulStatus = MAP_GPIOIntStatus (gpioin.port, true);
//    MAP_GPIOIntClear(gpioin.port, ulStatus);        // clear interrupts on GPIOA2
//    time = _time;
//    _time = 0;
//
//    interruptCounter++;
//    Report("%d, %d\n\r", interruptCounter, time);
//
//}
//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************

static void
BoardInit(void) {
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
    
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

static void timerInit()
{
    Timer_IF_Init(PRCM_TIMERA0, TIMERA0_BASE, TIMER_CFG_PERIODIC, TIMER_A, 255);
    Timer_IF_IntSetup(TIMERA0_BASE, TIMER_A, TimerRefIntHandler);
    TimerLoadSet(TIMERA0_BASE, TIMER_A, 10000);
    TimerEnable(TIMERA0_BASE, TIMER_A);
}

static void readTimeInit()
{
    Timer_IF_Init(PRCM_TIMERA0, TIMERA0_BASE, TIMER_CFG_PERIODIC, TIMER_B, 255);
    Timer_IF_IntSetup(TIMERA0_BASE, TIMER_B, ReadTimeIntHandler);
    TimerLoadSet(TIMERA0_BASE, TIMER_B, 1);
    TimerEnable(TIMERA0_BASE, TIMER_B);
}

initializeArr()
{
    int i;
    for(i = 0; i < 35; i++)
    {
        timeInterval[i] = 0;
        timeOfInterrupt[i] = 0;
        bitSequence[i] = 0;
    }
}

printArr() {
    int i;
    for(i = 0; i < 35; i++)
    {
        // Report("Time interval : %lu, Interrupt: %d\n\r", (int) timeInterval[i], i);
        Report("T: %d, I: %d\n\r", timeOfInterrupt[i], i);
        // Report("%d", bitSequence[i]);
    }
    Report("\n\r");
    for(i = 0; i < 35; i++)
    {
        // Report("Time interval : %d, Interrupt: %d\n\r", timeInterval[i], i);
        // Report("Time of Interrupt: %d, Interrupt: %d\n\r", timeOfInterrupt[i], i);
        Report("%d", bitSequence[i]);
    }
    Report("\n\r");
}

static void printBuffer()
{
    int i;
    Report("\n\r");
    for(i = 0; i < readIndex; i++)
    {
        Report("%c", buffer[i]);
    }
    Report("\n\r");
}

static int decode(unsigned long time){

    if(time > 0 && time < 10){
        return 0;
    }
    else if(time > 10 && time < 20){
        return 1;
    }
    else if(time > 1000)
    {
        interruptCounter = 0;
    }
    return 0;
}

//***************************************************************************
//
//! Main function
//!
//! \param none
//! 
//!
//! \return None.
//
//****************************************************************************

int main() {
    unsigned long ulStatus;
    BoardInit();
    
    PinMuxConfig();
    
    // UART config

    // SPI config

    // Adafruit init

    InitTerm();

    ClearTerm();
    //
    // Initialize Timer
    //
    timerInit();
    readTimeInit();
    //
    // Register the interrupt handlers
    //
    MAP_GPIOIntRegister(gpioin.port, GPIOA2IntHandler);
    //
    // Configure rising edge interrupts on PIN 61
    //
    MAP_GPIOIntTypeSet(gpioin.port, gpioin.pin, GPIO_RISING_EDGE);
    ulStatus = MAP_GPIOIntStatus (gpioin.port, false);
    MAP_GPIOIntClear(gpioin.port, ulStatus);            // clear interrupts on GPIOA2

    // Initialize global variables
    interruptCounter = 0;
    pulseCounter = 0;
    initializeArr();
    _time = 0;
    _readTime = 0;
    readIndex = 0;
    buffer = (char *) malloc(sizeof(char) * (64*64));
    // Initialize local  variables
    int fPressed = 0;

    MAP_GPIOIntEnable(gpioin.port, gpioin.pin);

    Message("\t\t****************************************************\n\r");
    Message("\t\t\                      LAB 3                        \n\r");
    Message("\t\t ***************************************************\n\r");
    Message("\n\n\n\r");

    // setting timer value to 0
    TimerValueSet(TIMERA0_BASE, TIMER_A, 0);
    TimerValueSet(TIMERA1_BASE, TIMER_B, 0);

    while (1) {
        // logic for matching the pattern and displaying character on OLED
        // 35 RISING HIGH INTERRUPTS
        if(interruptCounter == 35)
        {
            MAP_GPIOIntDisable(gpioin.port, gpioin.pin);
            // TimerDisable(TIMERA0_BASE, TIMER_A);
            // Report("M: %d\n\r", interruptCounter);
            // Report("%d \n\r", _time);
            // Report("\n Time intervals for RISING_EDGE\n\r");
            // Report("%d \n\r", _time);
            // _time = 0;
            // TimerLoadSet(TIMERA0_BASE, TIMER_A, 10000);
            // printArr();
            currRead =  compareBitPatterns();
            // buffer[readIndex] = currRead;
            readIndex++;
            Report("%d\n\r",_readTime);
            Report("%c\n\r", currRead);
            Report("%d\n\r",_readTime);
            if(currRead == 'M')
            {
                printBuffer();
                // Send message to the other board
            }
            else if(currRead == 'F' || currRead == 'D' && fPressed == 0)
            {
                // Pressing the delete button
                buffer[readIndex] = buffer[readIndex-1];
                readIndex --;
            }
            else if(currRead == '2')
            {
                switch(fPressed)
                {
                case 0:
                    buffer[readIndex] = 'a';
                    fPressed++;
                    break;
                case 1:
                    buffer[readIndex] = 'b';
                    fPressed++;
                    break;
                case 2:
                    buffer[readIndex] = 'c';
                    fPressed = 0;
                    break;
                }
            }
            // Report("%c\n\r", compareBitPatterns());
            // _readTime = 0;

            interruptCounter = 0;
            initializeArr();
            MAP_GPIOIntEnable(gpioin.port, gpioin.pin);
            // TimerLoadSet(TIMERA0_BASE, TIMER_A, 10000);
            TimerEnable(TIMERA0_BASE, TIMER_A);
        }
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
