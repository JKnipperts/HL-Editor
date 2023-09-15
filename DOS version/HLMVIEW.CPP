 //History Line 1914-1918 Map Viewer
 //by Jan Knipperts (DragonSphere)
 //DOS Version - v1.0

 #include <stdio.h>
 #include <conio.h>
 #include <alloc.h>
 #include <dos.h>
 #include <string.h>


typedef unsigned char byte;
typedef unsigned int  word;
typedef unsigned long  dword;


byte far*	Backbuffer;
int         vpx;
int         vpy;
int         key;
char*		Partlib_filename;
char*		Partdat_filename;
char*		Unitlib_filename;
char*		Unitdat_filename;
char*		Map_filename;
char*		Pal_filename;

char	        CONFIG[12] = "HLMVIEW.CFG";
char		UNITDAT[9] = "UNIT.DAT";
char		UNITLIB[9] = "UNIT.LIB";
char		PARTDAT[9] = "PARTS.DAT";
char		PARTLIB[9] = "PARTS.LIB";
char		PAL[6]     = "00.PAL";
char		MAP0[6]	   = "00.FIN";
char		LIB_DIR[4] = "LIB\\";
char		MAP_DIR[4] = "MAP\\";




const int SCREEN_HEIGHT = 200;
const int SCREEN_WIDTH = 320;
const int IO_BUFFER_SIZE = 4096;
      int VP_WIDTH = 19;
      int VP_HEIGHT = 7;

byte far *VGA = (byte far *)0xA0000000L;    /* pointer to VGA memory */





/* changes video mode with given low byte*/
void set_mode(byte mode)
{
    union REGS regs;
    regs.h.ah = 0x00;   /* set high byte of register */
    regs.h.al = mode;       /* set low byte of register */
    int86(0x10, &regs, &regs);
}



#include "Lib.h"
#include "fin.h"


void set_palette()
{
 int i;
 outp(0x3C8,0);
 for (i = 0;i < 256*3; i++)
 {
   outp(0x3C9,pal[i] / 4); //div 4 for 6-Bit VGA register
 }
}



void Release_Buffers()
{
    if (Map.data != NULL) farfree(Map.data);
    if (Partlib.data != NULL) farfree(Partlib.data);
    if (Unitlib.data != NULL) farfree(Unitlib.data);
    if (Backbuffer != NULL) farfree(Backbuffer);
    if (TPWM.unpacked_data != NULL) farfree(TPWM.unpacked_data);
    if (pal != NULL) free(pal);
	free(Partlib_filename);
	free(Unitlib_filename);
	free(Partdat_filename);
	free(Unitdat_filename);
	free(Pal_filename);
	free(Map_filename);

    return;
}




void Screen_update()
{
	 _fmemset((byte far*)Backbuffer, 0, 0xFFFF);
	 Draw_Map_Section(vpx,vpy);
	_fmemcpy(VGA,Backbuffer,0xFFFF);
}



int Read_cfg()
{
	FILE*	 cfg;
	size_t  IO_result;
	char*	GameDir;
	int     GameDir_length;

       cfg = fopen(CONFIG,"r");
       if (cfg == NULL) return -1;

       fseek(cfg,0,SEEK_END);
       GameDir_length = ftell(cfg);
       rewind(cfg);

       GameDir = (char*) malloc(GameDir_length);

       if (GameDir == NULL)
       {
	fclose(cfg);
	return -3;
       }


       IO_result = fread(GameDir,1,GameDir_length,cfg);
       if (IO_result != GameDir_length)
       {
	free(GameDir);
	fclose(cfg);
	return -2;
       }
       fclose(cfg);

       Partlib_filename = (char*) malloc(GameDir_length+13);
       if (Partlib_filename == NULL) return -3;
       Partdat_filename = (char*) malloc(GameDir_length+13);
       if (Partdat_filename == NULL) return -3;
       Unitlib_filename = (char*) malloc(GameDir_length+13);
       if (Partlib_filename == NULL) return -3;
       Unitdat_filename = (char*) malloc(GameDir_length+13);
       if (Unitdat_filename == NULL) return -3;
       Pal_filename = (char*) malloc(GameDir_length+6);
       if (Pal_filename == NULL) return -3;
       Map_filename = (char*) malloc(GameDir_length+10);
       if (Pal_filename == NULL) return -3;


       memcpy(Partlib_filename,GameDir,GameDir_length);
       memcpy(Partlib_filename+GameDir_length,LIB_DIR,4);
       memcpy(Partlib_filename+GameDir_length+4,PARTLIB,9);
       Partlib_filename[GameDir_length+13] = (char) 0;

       memcpy(Partdat_filename,GameDir,GameDir_length);
       memcpy(Partdat_filename+GameDir_length,LIB_DIR,4);
       memcpy(Partdat_filename+GameDir_length+4,PARTDAT,9);
       Partdat_filename[GameDir_length+13] = (char) 0;

       memcpy(Unitlib_filename,GameDir,GameDir_length);
       memcpy(Unitlib_filename+GameDir_length,LIB_DIR,4);
       memcpy(Unitlib_filename+GameDir_length+4,UNITLIB,9);
       Unitlib_filename[GameDir_length+13] = (char) 0;

       memcpy(Unitdat_filename,GameDir,GameDir_length);
       memcpy(Unitdat_filename+GameDir_length,LIB_DIR,4);
       memcpy(Unitdat_filename+GameDir_length+4,UNITDAT,9);
       Unitdat_filename[GameDir_length+13] = (char) 0;

       memcpy(Pal_filename,GameDir,GameDir_length);
       memcpy(Pal_filename+GameDir_length,PAL,6);
       Pal_filename[GameDir_length+6] = (char) 0;

       memcpy(Map_filename,GameDir,GameDir_length);
       memcpy(Map_filename+GameDir_length,MAP_DIR,4);
       memcpy(Map_filename+GameDir_length+4,MAP0,6);

       Map_filename[GameDir_length+10] = (char) 0;



       free(GameDir);

       return 0;
}











 int main(int argc, char**argv)
 {
  clrscr();

	if (Read_cfg() != 0)
	{
		printf("\n");
		printf(" Error loading configuration file! \n");
		return -1;
	}

    printf("Loading ");
    printf(Partlib_filename);
    printf(", ");
    printf(Partdat_filename);
    printf("...");

	if (Load_Part_files(Partlib_filename,
			    Partdat_filename) != 0)
	{
		printf("\n");
		printf(" Error loading terrain tile graphics! \n");

		Release_Buffers();
		return -1;
	}
     printf("OK \n");

    printf("Loading ");
    printf(Unitlib_filename);
    printf(", ");
    printf(Unitdat_filename);
    printf("...");
	if (Load_Unit_files(Unitlib_filename,
			    Unitdat_filename) != 0)
	{
		printf("\n");
		printf(" Error loading unit tile graphics! \n");
		Release_Buffers();
		return -1;
	}
    printf("OK \n");

    printf("Loading ");
    printf(Pal_filename);
    printf("...");

	if (Load_Palette(Pal_filename) != 0)
	{
		printf("\n");
		printf(" Error loading color palette! \n");
		Release_Buffers();
		return -1;
	}

    printf("OK \n");

	if (argc > 1)
	{
	 if (((argv[1][0] >= 48) && (argv[1][0] <= 57)) &&
	     ((argv[1][1] >= 48) && (argv[1][1] <= 57)))
	 {
	  Map_filename[strlen(Map_filename)-6]  = argv[1][0];
	  Map_filename[strlen(Map_filename)-5]  = argv[1][1];
	 }
	}

    printf("Loading ");
    printf(Map_filename);
    printf("...");


	if (Load_Mapdata(Map_filename) != 0)
	{
		printf("\n");
		printf(" Error loading Map! \n");
		Release_Buffers();
		return -1;
	}
    printf("OK \n");


	Backbuffer = (byte far*) farmalloc(0xFFFF);
	if (Backbuffer == NULL)
	{
		printf(" Not enough memory! \n");
		Release_Buffers();
		return -1; //Reserve memory buffer for backbuffer failed
	}
	_fmemset((byte far*)Backbuffer, 0, 0xFFFF);  //Clear backbuffer;


	set_mode(0x13);
	set_palette();

	vpx = 0;
	vpy = 0;
	key = 0;
	Screen_update();

       while (key != (char) 27)
       {

	key = getch();
	if (!key) key = getch();

	if (key == (char) 9)   //Tab
	{

	 if (Partlib_filename[strlen(Partlib_filename)-5] == (char) 'S')
	   Partlib_filename[strlen(Partlib_filename)-5] = (char) 'W';
	 else
	   Partlib_filename[strlen(Partlib_filename)-5] = (char) 'S';

	 if (Partlib.data != NULL) farfree(Partlib.data);

	 if (Load_Part_files(Partlib_filename,
			    Partdat_filename) != 0)
	{
		set_mode(0x03);
		printf("\n");
		printf(" Error loading terrain tile graphics! \n");

		Release_Buffers();
		return -1;
	}

	 Screen_update();
	}



	if (key == (char) 72)
	{
	 if (vpy > 0) vpy--;
	 Screen_update();
	}
	if (key == (char) 80)
	{
	 if ((vpy+VP_HEIGHT) < (Map.y-1)) vpy++;
	 Screen_update();
	}
	if (key == (char) 75)
	{
	 if (vpx > 0) vpx--;
	 Screen_update();
	}

	if (key == (char) 77)
	{
	 if ((vpx+VP_WIDTH) < (Map.x-1)) vpx++;
	 Screen_update();
	}

	}

	set_mode(0x03);

	Release_Buffers();
 return 0;
}
