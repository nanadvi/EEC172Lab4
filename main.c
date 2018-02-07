
//*****************************************************************************
//
// Application Name     - int_sw
// Application Overview - The objective of this application is to demonstrate
//							GPIO interrupts using SW2 and SW3.
//							NOTE: the switches are not debounced!
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

#include "pin_mux_config.h"


//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
extern void (* const g_pfnVectors[])(void);

volatile unsigned long SW2_intcount;
volatile unsigned long SW3_intcount;
volatile unsigned char SW2_intflag;
volatile unsigned char SW3_intflag;
volatile unsigned long time;
int interruptCounter;

unsigned long timeInterval[18] = {};
unsigned long timeOfInterrupt[18] = {};
unsigned int bitSequence[18] = {};
int buttons[12][18] = {
                       {},
                       {},
                       {},
                       {},
                       {},

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

static void GPIOA2IntHandler(void) {
    interruptCounter++;
	unsigned long ulStatus;
    ulStatus = MAP_GPIOIntStatus (gpioin.port, true);
    MAP_GPIOIntClear(gpioin.port, ulStatus);		// clear interrupts on GPIOA2
    time = TimerValueGet(TIMERA0_BASE, TIMER_A);
    // Report("Interrupt: %d, Time of Interrupt: %d\n\r", interruptCounter, time);

    if(interruptCounter <= 18)
    {
        timeOfInterrupt[interruptCounter] = time;
        // Report("%lu\n\r", timeOfInterrupt[interruptCounter-1]);
        unsigned long read = time - timeOfInterrupt[interruptCounter-1];
        timeInterval[interruptCounter] = read;
        // Get the bit sequence
        if(read > 30000 && read< 55000){
            bitSequence[interruptCounter] = 0;
        }
        else if(read > 55000 && read< 80000){
            bitSequence[interruptCounter] = 1;
        }
        else {
            // Start of new burst
            bitSequence[interruptCounter] = 0;
        }
    }

}
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

static void
GPIOIntInit(){

}

initializeArr()
{
    int i;
    for(i = 0; i < 18; i++)
    {
        timeInterval[i] = 0;
        timeOfInterrupt[i] = 0;
        bitSequence[i] = 0;
    }
}

printArr()
{
    int i;
    for(i = 0; i < 18; i++)
    {
        Report("Time interval : %lu, Interrupt: %d\n\r", (int) timeInterval[i], i);
        // Report("Time of Interrupt: %d, Interrupt: %d\n\r", timeOfInterrupt[i], i);
        // Report("%d", bitSequence[i]);
    }
    Report("\n\r");
    for(i = 0; i < 18; i++)
    {
        // Report("Time interval : %d, Interrupt: %d\n\r", timeInterval[i], i);
        // Report("Time of Interrupt: %d, Interrupt: %d\n\r", timeOfInterrupt[i], i);
        Report("%d", bitSequence[i]);
    }
    Report("\n\r");
}

//****************************************************************************
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

    // Configuring the timer

    //
    // Initialize Timer
    //
    Timer_IF_Init(PRCM_TIMERA0, TIMERA0_BASE, TIMER_CFG_PERIODIC_UP, TIMER_A, 255);

    Timer_IF_Start(TIMERA0_BASE, TIMER_A, 40000000000);
    //
    // Register the interrupt handlers
    //
    MAP_GPIOIntRegister(gpioin.port, GPIOA2IntHandler);

    //
    // Configure rising edge interrupts on SW2 and SW3
    //
    MAP_GPIOIntTypeSet(gpioin.port, gpioin.pin, GPIO_FALLING_EDGE);	// pin 61

    ulStatus = MAP_GPIOIntStatus (gpioin.port, false);
    MAP_GPIOIntClear(gpioin.port, ulStatus);			// clear interrupts on GPIOA2

    // Initialize global variables
    interruptCounter = 0;
    initializeArr();

    // Enable SW2 and SW3 interrupts
    // MAP_GPIOIntEnable(GPIOA1_BASE, 0x20);
    MAP_GPIOIntEnable(gpioin.port, gpioin.pin);


    Message("\t\t****************************************************\n\r");
    Message("\t\t\                      LAB 3                        \n\r");
    Message("\t\t ***************************************************\n\r");
    Message("\n\n\n\r");

    // setting timer value to 0
    TimerValueSet(TIMERA0_BASE, TIMER_A, 0);

    while (1) {
        // logic for matching the pattern and displaying character on OLED

        // We read 64 time intervals
        if(interruptCounter == 18)
        {
            Report("\n Time intervals for RISING_EDGE\n\r");
            printArr();
            interruptCounter = 0;
            initializeArr();
            TimerValueSet(TIMERA0_BASE, TIMER_A, 0);

        }
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
