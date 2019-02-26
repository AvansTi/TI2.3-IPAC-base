#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "system.h"
#include "flash.h"
#include "userdata.h"

#include <fs/fs.h>
#include <dev/urom.h>
#include <fs/uromfs.h>
#include <io.h>

#define NOK -1

char buffer[127]; 

// Init flash 
int initUserData(void)
{
    return At45dbInit();
}

// Show a single page in persistent data storage (for debugging purposes)
void showPage(u_long pgn)
{
	unsigned char *pc = (unsigned char *) malloc(264);
	int idx;
	
	At45dbPageRead(pgn, (unsigned char *)pc, 264);
	for(idx = 0; idx < 264; idx++)
	{
		if( 0 == (idx % 32) )
			printf("\n");
		
		if( isalpha(pc[idx]) || isdigit(pc[idx]) )
		{
			printf("%c  ", pc[idx]);
		}
		else
		{
			printf("%02X ", pc[idx]);
		}
	}
	printf("\n");
	free(pc);
}

// Save user data to persistent data storage (data flash),
// currently to a single hardcoded page (0x04)!
// TODO: If you need more than one page, modify this code!
int savePersistent(USERDATA_STRUCT *src, int size)
{
	int result = NOK;
	
	unsigned char *storage = (unsigned char *) malloc(sizeof(unsigned char) * size);
	if( storage != NULL )
	{
		memcpy( (unsigned char *)storage, src, size );
		At45dbPageWrite(0x04, (unsigned char *)storage, size);

		result = OK;
	}
	free(storage);
	return result;
}

// Restore user data from persistent data storage (data flash)
// currently from a single hardcoded page (0x04)!
// TODO: If you need more than one page, modify this code!
int openPersistent(USERDATA_STRUCT *src, int size)
{
	int result = NOK;

	unsigned char *storage = (unsigned char *) malloc(sizeof(unsigned char) * size);
	if( storage != NULL )
	{
		At45dbPageRead(0x04, (unsigned char *)storage, size);
		memcpy( (USERDATA_STRUCT *) src, (unsigned char *)storage, size );

		result = OK;
	}
	free(storage);
	return result;
}

/*
void testRomFs(void)
{
	FILE *fp;
	
	// Connect to UROM
	if( NutRegisterDevice(&devUrom, 0, 0 ))
	{	
		printf("NutRegisterDevice(&devUrom, 0, 0 ) error");
	}
	else
	{
		// Ter illustratie, lees urom
		fp = fopen("UROM:index.htm", "r");
		while(!feof(fp))
		{
			fgets(buffer, sizeof(buffer), fp);
			puts(buffer);
		}
		fclose(fp);
	}
}
*/
