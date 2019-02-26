
///
#define LOG_MODULE  LOG_SHOUTCAST_MODULE

#include <sys/thread.h>
#include <sys/timer.h>

#include <dev/nicrtl.h>
#include <arpa/inet.h>
#include <sys/confnet.h>
#include <pro/dhcp.h>


#include "log.h"
#include "shoutcast.h"

#include <sys/socket.h>
#include "player.h"


#define ETH0_BASE	0xC300
#define ETH0_IRQ	5

//#define OK		1
//#define NOK		0
#define OK			0
#define NOK			-1

//#define RADIO_IPADDRESS	"104.28.7.54" // Swissgroove.ch
//#define RADIO_PORT		8100			// Swissgroove.ch
//#define RADIO_URL		"/"				// Swissgroove.ch
#define RADIO_IPADDRESS	"50.7.98.106"   // Radiowax.com
#define RADIO_PORT		8465			// Radiowax.com
#define RADIO_URL		"/"				// Radiowax.com


static char eth0IfName[9] = "eth0";
FILE *stream;

int initInet(void)
{
	uint8_t mac_addr[6] = { 0x00, 0x06, 0x98, 0x30, 0x02, 0x76 };
	
	int result = OK;

	// Registreer NIC device (located in nictrl.h)
	result = NutRegisterDevice(&DEV_ETHER, ETH0_BASE, ETH0_IRQ);
	LogMsg_P(LOG_DEBUG, PSTR("NutRegisterDevice() returned %d"), result);
	if(result)
	{
		LogMsg_P(LOG_ERR, PSTR("Error: >> NutRegisterDevice()"));
		result = NOK;
	}
	
	if( OK == result )
	{
		LogMsg_P(LOG_INFO, PSTR("Getting IP address (DHCP)"));
		if( NutDhcpIfConfig(eth0IfName, NULL, 10000) )
		{
			LogMsg_P(LOG_ERR, PSTR("Error: no stored MAC address, try hardcoded MAC"));
			if( NutDhcpIfConfig(eth0IfName, mac_addr, 0) )
			{
				LogMsg_P(LOG_ERR, PSTR("Error: >> NutDhcpIfConfig()"));
				result = NOK;
			}
		}
	}
	
	if( OK == result )
	{
		LogMsg_P(LOG_INFO, PSTR("Networking setup OK, new settings are:\n") );	

		LogMsg_P(LOG_INFO, PSTR("if_name: %s"), confnet.cd_name);	
		LogMsg_P(LOG_INFO, PSTR("ip-addr: %s"), inet_ntoa(confnet.cdn_ip_addr) );
		LogMsg_P(LOG_INFO, PSTR("ip-mask: %s"), inet_ntoa(confnet.cdn_ip_mask) );
		LogMsg_P(LOG_INFO, PSTR("gw     : %s"), inet_ntoa(confnet.cdn_gateway) );
	}
	
	NutSleep(1000);
	
	return result;
}

int connectToStream(void)
{
	int result = OK;
	char *data;
	
	TCPSOCKET *sock;
	
	sock = NutTcpCreateSocket();
	if( NutTcpConnect(	sock,
						//inet_addr("62.212.132.54"), 
						//8100) )
						inet_addr(RADIO_IPADDRESS),
						RADIO_PORT) )
	{
		LogMsg_P(LOG_ERR, PSTR("Error: >> NutTcpConnect()"));
		exit(1);
	}
	stream = _fdopen( (int) sock, "r+b" );
	
	fprintf(stream, "GET %s HTTP/1.0\r\n", RADIO_URL);
	//fprintf(stream, "Host: %s\r\n", "62.212.132.54");
	fprintf(stream, "Host: %s\r\n", inet_addr(RADIO_IPADDRESS));
	fprintf(stream, "User-Agent: Ethernut\r\n");
	fprintf(stream, "Accept: */*\r\n");
	fprintf(stream, "Icy-MetaData: 1\r\n");
	fprintf(stream, "Connection: close\r\n\r\n");
	fflush(stream);

	
	// Server stuurt nu HTTP header terug, catch in buffer
	data = (char *) malloc(512 * sizeof(char));
	
	while( fgets(data, 512, stream) )
	{
		if( 0 == *data )
			break;

		printf("%s", data);
	}
	
	free(data);
	
	return result;
}

int playStream(void)
{
	play(stream);
	
	return OK;
}

int stopStream(void)
{
	fclose(stream);	
	
	return OK;
}
