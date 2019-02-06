/*! \mainpage SIR firmware documentation
 *
 *  \section intro Introduction
 *  A collection of HTML-files has been generated using the documentation in the sourcefiles to
 *  allow the developer to browse through the technical documentation of this project.
 *  \par
 *  \note these HTML files are automatically generated (using DoxyGen) and all modifications in the
 *  documentation should be done via the sourcefiles.
 */

/*! \file
 *  COPYRIGHT (C) STREAMIT BV 2010
 *  \date 19 december 2003
 */
 
 
 

#define LOG_MODULE  LOG_MAIN_MODULE
#define USE_JTAG 0

/*--------------------------------------------------------------------------*/
/*  Include files                                                           */
/*--------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>

#include <sys/thread.h>
#include <sys/timer.h>
#include <sys/version.h>
#include <dev/irqreg.h>

#include "system.h"
#include "portio.h"
#include "display.h"
#include "remcon.h"
#include "keyboard.h"
#include "led.h"
#include "log.h"
#include "uart0driver.h"
#include "mmc.h"
#include "watchdog.h"
#include "flash.h"
#include "spidrv.h"

#include <time.h>
//#include "rtc.h"
#include "x1205.h"


/*-------------------------------------------------------------------------*/
/* global variable definitions                                             */
/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/* local variable definitions                                              */
/*-------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------*/
/* local routines (prototyping)                                            */
/*-------------------------------------------------------------------------*/
static void SysMainBeatInterrupt(void*);
static void SysControlMainBeat(u_char);

/*-------------------------------------------------------------------------*/
/* Stack check variables placed in .noinit section                         */
/*-------------------------------------------------------------------------*/

/*!
 * \addtogroup System
 */

/*@{*/


/*-------------------------------------------------------------------------*/
/*                         start of code                                   */
/*-------------------------------------------------------------------------*/


/* อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ */
/*!
 * \brief ISR MainBeat Timer Interrupt (Timer 2 for Mega128, Timer 0 for Mega256).
 *
 * This routine is automatically called during system
 * initialization.
 *
 * resolution of this Timer ISR is 4,448 msecs
 *
 * \param *p not used (might be used to pass parms from the ISR)
 */
/* อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ */
static void SysMainBeatInterrupt(void *p)
{

    /*
     *  scan for valid keys AND check if a MMCard is inserted or removed
     */
    KbScan();
    CardCheckCard();
}


/* อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ */
/*!
 * \brief Initialise Digital IO
 *  init inputs to '0', outputs to '1' (DDRxn='0' or '1')
 *
 *  Pull-ups are enabled when the pin is set to input (DDRxn='0') and then a '1'
 *  is written to the pin (PORTxn='1')
 */
/* อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ */
void SysInitIO(void)
{
    /*
     *  Port B:     VS1011, MMC CS/WP, SPI
     *  output:     all, except b3 (SPI Master In)
     *  input:      SPI Master In
     *  pull-up:    none
     */
    outp(0xF7, DDRB);

    /*
     *  Port C:     Address bus
     */

    /*
     *  Port D:     LCD_data, Keypad Col 2 & Col 3, SDA & SCL (TWI)
     *  output:     Keyboard colums 2 & 3
     *  input:      LCD_data, SDA, SCL (TWI)
     *  pull-up:    LCD_data, SDA & SCL
     */
    outp(0x0C, DDRD);
    outp((inp(PORTD) & 0x0C) | 0xF3, PORTD);

    /*
     *  Port E:     CS Flash, VS1011 (DREQ), RTL8019, LCD BL/Enable, IR, USB Rx/Tx
     *  output:     CS Flash, LCD BL/Enable, USB Tx
     *  input:      VS1011 (DREQ), RTL8019, IR
     *  pull-up:    USB Rx
     */
    outp(0x8E, DDRE);
    outp((inp(PORTE) & 0x8E) | 0x01, PORTE);

    /*
     *  Port F:     Keyboard_Rows, JTAG-connector, LED, LCD RS/RW, MCC-detect
     *  output:     LCD RS/RW, LED
     *  input:      Keyboard_Rows, MCC-detect
     *  pull-up:    Keyboard_Rows, MCC-detect
     *  note:       Key row 0 & 1 are shared with JTAG TCK/TMS. Cannot be used concurrent
     */
#ifndef USE_JTAG
    sbi(JTAG_REG, JTD); // disable JTAG interface to be able to use all key-rows
    sbi(JTAG_REG, JTD); // do it 2 times - according to requirements ATMEGA128 datasheet: see page 256

	
#endif //USE_JTAG

	cbi(OCDR, IDRD);
	cbi(OCDR, IDRD);


    outp(0x0E, DDRF);
    outp((inp(PORTF) & 0x0E) | 0xF1, PORTF);

    /*
     *  Port G:     Keyboard_cols, Bus_control
     *  output:     Keyboard_cols
     *  input:      Bus Control (internal control)
     *  pull-up:    none
     */
    outp(0x18, DDRG);
}

/* อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ */
/*!
 * \brief Starts or stops the 4.44 msec mainbeat of the system
 * \param OnOff indicates if the mainbeat needs to start or to stop
 */
/* อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ */
static void SysControlMainBeat(u_char OnOff)
{
    int nError = 0;

    if (OnOff==ON)
    {
        nError = NutRegisterIrqHandler(&OVERFLOW_SIGNAL, SysMainBeatInterrupt, NULL);
        if (nError == 0)
        {
            init_8_bit_timer();
        }
    }
    else
    {
        // disable overflow interrupt
        disable_8_bit_timer_ovfl_int();
    }
}

/*
 * Show current time in x1205 RTC
 */
static void ShowX1205DateTime()
{
	u_char buf[10]; // Buffer to hold data returned by x1205 RTC
	// Read from register 0x30 (RTC SC) 3 bytes: Seconds, Minutes, Hours
	// See X1205 datasheet Clock/Control Memory Map (page 10)
	x1205ReadNByte(0x30, buf, 8);
	LogMsg_P(LOG_DEBUG, PSTR("Time in x1205 RTC is [%02d:%02d:%02d]"),
		BCD2DEC(buf[2] & 0x7F), BCD2DEC(buf[1] & 0x7F), BCD2DEC(buf[0] & 0x7F));
	LogMsg_P(LOG_DEBUG, PSTR("Date in x1205 RTC is [%02d.%02d %02d-%02d-%02d]"),
		BCD2DEC(buf[7] & 0x7F), BCD2DEC(buf[6] & 0x7F),
		BCD2DEC(buf[5] & 0x7F), BCD2DEC(buf[4] & 0x7F), BCD2DEC(buf[3] & 0x7F));
}

/*
 * Set current time in x1205 RTC
 */
static void SetX1205Time(int Y2K, int DW, int YR, int MO, int DT, int HR, int MN, int SC)
{
	u_char buffer[10]; // Buffer to assemble data in for sending to x1205 RTC
	buffer[0] = DEC2BCD(SC); // Seconds
	buffer[1] = DEC2BCD(MN); // Minutes
	buffer[2] = DEC2BCD(HR); // Hours
	buffer[3] = DEC2BCD(DT); // Date
	buffer[4] = DEC2BCD(MO); // Month
	buffer[5] = DEC2BCD(YR); // Year (00 - 99)
	buffer[6] = DEC2BCD(DW); // Day of Week (0 - 6)
	buffer[7] = DEC2BCD(Y2K); // Year 2K
	// Send to register 0x30 (RTC SC) and beyond 8 bytes:
	// Seconds, Minutes, Hours, Date, Month, Year, Day of Week, Year 2K
	// See X1205 datasheet Clock/Control Memory Map (page 10)
	x1205WriteNBytes(0x30, buffer, 8);
	NutSleep(100);
}

/* อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ */
/*!
 * \brief Main entry of the SIR firmware
 *
 * All the initialisations before entering the for(;;) loop are done BEFORE
 * the first key is ever pressed. So when entering the Setup (POWER + VOLMIN) some
 * initialisatons need to be done again when leaving the Setup because new values
 * might be current now
 *
 * \return \b never returns
 */
/* อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ */
int main(void)
{
	int t = 0;
	int x = 0;
	
	/* 
	 * Kroeske: time struct uit nut/os time.h (http://www.ethernut.de/api/time_8h-source.html)
	 *
	 */
	tm gmt;
	/*
	 * Kroeske: Ook kan 'struct _tm gmt' Zie bovenstaande link
	 */
	
    /*
     *  First disable the watchdog
     */
    WatchDogDisable();

    NutDelay(100);

    SysInitIO();
	
	SPIinit();
    
	LedInit();
	
	LcdLowLevelInit();

    Uart0DriverInit();
    Uart0DriverStart();
	LogInit();
	LogMsg_P(LOG_INFO, PSTR("Hello World"));

    CardInit();

#if 0
	/*
	 * Kroeske: sources in rtc.c en rtc.h
	 */
    X12Init();
    if (X12RtcGetClock(&gmt) == 0)
    {
		LogMsg_P(LOG_INFO, PSTR("RTC time [%02d:%02d:%02d]"), gmt.tm_hour, gmt.tm_min, gmt.tm_sec );
    }
#endif

	// [LiHa] Experiment with our own x1205 read and write routines
	x1205Init();
	NutSleep(100);
	// [LiHa] Show the current time in the RTC
	ShowX1205DateTime();
	// [LiHa] Enable writing to the x1205 RTC, see datasheet
	x1205Enable();
	// [LiHa] Set time in the RTC: 2019-02-07 10:11:12 (let's say that Thursday = 3)
	SetX1205Time(20, 3, 19, 2, 7, 10, 11, 12);
	// [LiHa] Show current time again
	ShowX1205DateTime();


    if (At45dbInit()==AT45DB041B)
    {
        // ......
    }


    RcInit();
    
	KbInit();

    SysControlMainBeat(ON);             // enable 4.4 msecs hartbeat interrupt

    /*
     * Increase our priority so we can feed the watchdog.
     */
    NutThreadSetPriority(1);

	/* Enable global interrupts */
	sei();
	
    for (;;)
    {
        NutSleep(100);
		if( !((t++)%15) )
		{
			// [LiHa] Show the current time, should be updating
			ShowX1205DateTime();
			
			//LogMsg_P(LOG_INFO, PSTR("Yes!, I'm alive ... [%d]"),t);
			
			LedControl(LED_TOGGLE);
		
			if( x )
			{
				LcdBackLight(LCD_BACKLIGHT_ON);
				x = 0;
			}
			else
			{
				LcdBackLight(LCD_BACKLIGHT_OFF);
				x = 1;
			}
		}
		
        WatchDogRestart();
    }

    return(0);      // never reached, but 'main()' returns a non-void, so.....
}
/* ---------- end of module ------------------------------------------------ */

/*@}*/
