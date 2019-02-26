#ifndef USERDATA_INC
#define USERDATA_INC

typedef struct
{
	char desc[35];
	char url[40];
	int port;
	int volume;
} USERDATA_STRUCT;

int initUserData(void);
int savePersistent(USERDATA_STRUCT *src, int size);
int openPersistent(USERDATA_STRUCT *src, int size);
void showPage(u_long pgn);

void testRomFs(void);

#endif
