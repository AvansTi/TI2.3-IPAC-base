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
#define USE_JTAG 1

/*--------------------------------------------------------------------------*/
/*  Include files                                                           */
/*--------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>

#include <sys/thread.h>
#include <sys/timer.h>
#include <sys/version.h>
#include <dev/irqreg.h>

#include <arpa/inet.h> /* [LiHa] For inet_addr() */
#include <pro/sntp.h> /* [LiHa] For NTP */

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
#include "shoutcast.h"

#include <time.h>
#include "rtc.h"

#include "userdata.h"

/* Some dummy user data for testing the persistent data storage */
static USERDATA_STRUCT streams[] = {
	/* First data set: */
	// {
	// "Loungemain Jazz-Chill-Funk",
	// "http://80.101.35.49",
	// 8000,
	// 1010
	// },
	// {
	// "METAL ONLY - www.metal-only.de",
	// "http://62.138.229.187",
	// 6666,
	// 2020
	// }
	
	/* Alternative data set (uncomment one or the other): */
	{
		"Radio 1: nieuws elk uur",
		"http://radio1.stream/",
		1234,
		2020
	},
	{
		"XFM, hitradio",
		"http://212.121.111.99/uberstream/",
		4321,
		4040
	}
};


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

static void showUserData(USERDATA_STRUCT *streams, int len);

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
	//int x = 0;
	
	/* 
	 * Kroeske: time struct uit nut/os time.h (http://www.ethernut.de/api/time_8h-source.html)
	 *
	 */
	tm gmt;
	/*
	 * Kroeske: Ook kan 'struct _tm gmt' Zie bovenstaande link
	 */
	
	/* [LiHa] NTP server */
	time_t ntp_time;
	tm *ntp_datetime;
	uint32_t timeserver = 0;
	
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

	/*
	 * Kroeske: sources in rtc.c en rtc.h
	 */
    X12Init();
    if (X12RtcGetClock(&gmt) == 0)
    {
		LogMsg_P(LOG_INFO, PSTR("RTC time [%02d:%02d:%02d]"), gmt.tm_hour, gmt.tm_min, gmt.tm_sec );
    }


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
	
	LcdBackLight(LCD_BACKLIGHT_ON);
	char message[] = "Streaming";
	char *pmsg = message;
	while (*pmsg != 0) {
		LcdChar(*pmsg);
		pmsg += 1;
	}
	
	/* Init network adapter */
	if( OK != initInet() )
	{
		LogMsg_P(LOG_ERR, PSTR("initInet() = NOK, NO network!"));
	}

	/* Retrieve NTP time */
	/* [LiHa] Bron: http://www.ethernut.de/nutwiki/index.php/Network_Time_Protocol */
	printf("\nRetrieving time from pool.ntp.org...");
	_timezone = -1 * 60 * 60; /* GMT -1 hour */
	timeserver = inet_addr("91.148.192.49"); /* [LiHa] IP address may change frequently (it's a pool afterall) */
	if (NutSNTPGetTime(&timeserver, &ntp_time) != 0) {
		printf("Failed to retrieve time\n");
		} else {
		ntp_datetime = localtime(&ntp_time);
		printf("NTP time is: %02d:%02d:%02d\n", ntp_datetime->tm_hour, ntp_datetime->tm_min, ntp_datetime->tm_sec);
	}

	/* Show the user data that we're going to save to data flash */
	printf("\nUser data content: \n");
	showUserData(streams, sizeof(streams)/sizeof(streams[0]));
	/* Show what the data flash page for the user data currently contains */
	printf("\nFlash page 0x04 content: \n");
	showPage(0x04); // Attention, hardcoded page number
	
	/* Now save the user data to data flash */
	if( OK == savePersistent( (USERDATA_STRUCT *)streams, sizeof(streams)) )
	{
		printf("\nFlash page 0x04 content: \n");
		NutSleep(100);
		showPage(0x04); // Attention, hardcoded page number
		NutSleep(100);
		
		/* Trash userdata */
		strcpy(streams[0].desc,"trash - trash - trash");
		streams[1].volume = 0x0102;
		printf("\nTrashed user data: \n");
		showUserData(streams, sizeof(streams)/sizeof(streams[0]));

		/* Restore user data from data flash */
		openPersistent( (USERDATA_STRUCT *)streams, sizeof(streams) );
	}
	else
	{
		printf("Error saving persistent data");
	}
	/* Show restored user data retrieved from data flash */
	printf("\nRestored user data: \n");
	showUserData(streams, sizeof(streams)/sizeof(streams[0]));

	if( OK == connectToStream() )
	{
		playStream();
	}
	
    for (;;)
    {
        NutSleep(100);
		if( !((t++)%15) )
		{
			//LogMsg_P(LOG_INFO, PSTR("Yes!, I'm alive ... [%d]"),t);
			
			LedControl(LED_TOGGLE);
		}
		
        WatchDogRestart();
    }
	
	stopStream();

    return(0);      // never reached, but 'main()' returns a non-void, so.....
}


/* Local utility function for testing the persistent data storage */
static void showUserData(USERDATA_STRUCT *streams, int len)
{
	int idx = 0 ;

	for( idx = 0; idx < len; idx++ )
	{
		printf("\nstreams[%d].desc = %s\n", idx, streams[idx].desc);
		printf("streams[%d].url = %s\n", idx, streams[idx].url);
		printf("streams[%d].port = %d\n", idx, streams[idx].port);
		printf("streams[%d].volume = %d\n\n", idx, streams[idx].volume);
	}
}


/* ---------- end of module ------------------------------------------------ */

/*@}*/
