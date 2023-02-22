//C72B - try command
//A9B0
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "FDC.h"
#include "gbGlobals.h"
#include <iostream>
#include "SD.h"

#define SDCARD_CS 13
#define SDCARD_MOSI 12
#define SDCARD_MISO 2
#define SDCARD_CLK 14
static SPIClass customSPI;

//unsigned char gb_select_dsk_disk=0;
//int gb_ptrBeginDataDisc[42][11]; //Donde empieza el disco
int startreal;
unsigned char discon;
int endread;
int fdcint=0;
unsigned char fdcstatus=0x80;
int params=0;
int readparams=0;
unsigned char paramdat[16]; //Escrive disco revisar
unsigned char command;
unsigned char st0,st1,st2,st3;
int fdctrack;
int starttrack,startsector,endsector;
int posinsector;
unsigned char reading=0;
int disctracks; 
int discsects[40]; //esta en rom
unsigned char discid[42][11][4];
Tdiscdat *discdat;
int max_list_dsk=0;
char * gb_list_dsk_title[MAX_DSKS];
unsigned char * gb_list_dsk_sects[MAX_DSKS];
unsigned char * gb_list_dsk_discid[MAX_DSKS];
unsigned char * gb_list_dsk_discdat[MAX_DSKS];

#define listItemsTotal 100 //maximum number of file names to store
#define listItemMaxChar 100 //maximum number of characters per file name, including null terminator
char tempString[listItemMaxChar];//temporary string of the max number of characters per file name

using namespace std;

void getDskList() {

  customSPI.begin(SDCARD_CLK, SDCARD_MISO, SDCARD_MOSI, SDCARD_CS);
  SD.begin(SDCARD_CS, customSPI, 4000000, "/sd");

  File root = SD.open("/dsk");

  while (true) {
    File entry = root.openNextFile();
    if (!entry) {break;} // no more files
    else {
      sprintf(tempString, "%s", entry.name());//save file name to temporary string with null terminator at the end
      gb_list_dsk_title[max_list_dsk] = (char *)malloc(listItemMaxChar);//assign enough memory for 100 chars to current list item pointer
      sprintf(gb_list_dsk_title[max_list_dsk],"%s",tempString);
      max_list_dsk++; //increment counter of files
      entry.close();
    }
  }
  return; 
}


FILE *loaddskFromSD(char *nombre) {

  //customSPI.begin(SDCARD_CLK, SDCARD_MISO, SDCARD_MOSI, SDCARD_CS);
  //SD.begin(SDCARD_CS, customSPI, 4000000, "/sd");
  return fopen(nombre, "rb");

//file.close();
//SD.end();
//customSPI.end();
}

void loaddsk2Flash(unsigned char id)
{

        int numsect;
        int c,d;
        char *head, fileName[max_list_dsk];
        unsigned char *dskhead,*trkhead;

        sprintf(fileName, "/sd/dsk/%s", gb_list_dsk_title[id]);
        printf("Vamos a cargar %s\n", fileName);           
        head=(char *)ps_malloc(40 * sizeof(char));
        dskhead=(unsigned char *)ps_malloc(256 * sizeof(unsigned char));
        trkhead=(unsigned char *)ps_malloc(256 * sizeof(unsigned char));
        discdat=(Tdiscdats *)ps_malloc(sizeof(Tdiscdats));
        
        FILE *f=loaddskFromSD(fileName);
        if (!f) {
          printf("No se ha cargado el disco\n");
          return;
        }
        fread(dskhead,256,1,f);        
        for (c=0;c<40;c++) head[c]=0;
        for (c=0;c<0x21;c++) head[c]=dskhead[c];
        
        disctracks=dskhead[0x30];
        //printf("%i tracks\n",dskhead[0x30]);
        
        for (d=0;d<disctracks;d++) {
                fread(trkhead,256,1,f);
                //JJ while (strncmp(trkhead,"Track-Info",10) && !feof(f))
                while (strncmp((const char *)trkhead,"Track-Info",10) && !feof(f)) {
                      fread(trkhead,256,1,f);
                }
                //printf("Track %i ftell %05X : ",d,ftell(f)-256);

                if (feof(f)) {
                        fclose(f);
                        return;
                }
                
                discsects[d]=numsect=trkhead[0x15];
                //printf("%i sectors\n",numsect);
                for (c=0;c<numsect;c++) {
                        discid[d][c][0]=trkhead[0x18+(c<<3)];
                        discid[d][c][1]=trkhead[0x19+(c<<3)];
                        discid[d][c][2]=trkhead[0x1A+(c<<3)];
                        discid[d][c][3]=trkhead[0x1B+(c<<3)];
                        //printf("%i %i %i %i  ",discid[d][c][0],discid[d][c][1],discid[d][c][2],discid[d][c][3]);
                        fread(discdat->datos[d][(discid[d][c][2]-1)&15],512,1,f);                        
                }
                printf("\n");
        }
        //printf("DSK pos %i\n",ftell(f));
        fclose(f);
}



unsigned char readfdc(unsigned short addr)
{
        unsigned char aux_discdat[1024];
        int c;
        unsigned char temp;
//        printf("Read %04X  %04X\n",addr,pc);
        if (addr&1)
        {
                if (!readparams)
                {
                        //printf("Reading but no params - last command %02X\n",command);
                        exit(-1);
                }
                switch (command)
                {
                        case 0x04: //Sense drive status
                        readparams=0;
                        fdcstatus=0x80;
                        return st3;

                        case 0x06: //Read sectors
                        if (reading)
                        {

                                temp=discdat->datos[fdctrack][startsector-1][posinsector];                                
//                                printf("Read track %i sector %i pos %i\n",fdctrack,startreal,posinsector);
//                                printf("%c",temp);
                                posinsector++;
                                if (posinsector==512)
                                {
                                        if ((startsector&15)==(endsector&15))
                                        {
                                                reading=0;
                                                readparams=7;
//                                                output=1;
                                                fdcstatus=0xD0;
//                                                if (startsector==4) output=1;
//                                                printf("Done it %04X\n",pc);
                                                endread=1;
                                                fdcint=1;
                                                discon=0;
//                                                output=1;
//                                                dumpregs();
//                                                dumpram();
//                                                exit(-1);
                                        }
                                        else
                                        {
                                                posinsector=0;
                                                startsector++;
                                                //JJ if ((startsector&15)==(discsects[fdctrack]+1))
                                                if ((startsector&15)==(discsects[fdctrack]+1))
                                                {
                                                        if (command&0x80)
                                                           fdctrack++;
                                                        startsector=0xC1;
                                                }
                                                startreal=0;
                                                for (c=0;c<11;c++)
                                                {
                                                        if ((discid[starttrack][c][2]&15)==(startsector&15))
                                                        {
                                                                startreal=c;
                                                                break;
                                                        }
                                                }
                                        }
                                }
                                return temp;
                        }
                        readparams--;
                        switch (readparams)
                        {
                                case 6: st0=0x40; st1=0x80; st2=0; return st0;
                                case 5: return st1;
                                case 4: return st2;
                                case 3: return fdctrack;
                                case 2: return 0;
                                case 1: return startsector;
                                case 0: fdcstatus=0x80; return 2;
                        }
                        break;
                        case 0x86: /*Read sector fail*/
                        readparams--;
                        switch (readparams)
                        {
                                case 6: st0=0x40; st1=0x84; st2=0; return st0;
                                case 5: return st1;
                                case 4: return st2;
                                case 3: return fdctrack;
                                case 2: return 0;
                                case 1: return startsector;
                                case 0: fdcstatus=0x80; return 2;
                        }
                        break;

                        case 0x08: /*Sense interrupt state*/
                        readparams--;
                        if (readparams==1)
                           return st0;
                        fdcstatus=0x80;
                        return fdctrack;

                        case 0x0A: /*Read sector ID*/
                        readparams--;
                        switch (readparams)
                        {
                                case 6: return st0;
                                case 5: return st1;
                                case 4: return st2;
                                case 3: return discid[fdctrack][startsector][0];
                                case 2: return discid[fdctrack][startsector][1];
                                case 1: return discid[fdctrack][startsector][2];
                                case 0: fdcstatus=0x80; return discid[fdctrack][startsector][3];
                        }
                        break;

                        default:
                        //printf("Reading command %02X\n",command);
                        exit(-1);
                }
        }
        else
        {
//                if (reading)
//                   fdcstatus^=0x80;
                return fdcstatus;
        }
}

void writefdc(unsigned short addr, unsigned char val)
{
        int c;
//        printf("Write %04X %02X  %04X\n",addr,val,pc);
        if (addr==0xFA7E)
        {
                //JJ motoron=val&1;//No necesito estado motor disco
                return;
        }
        if (addr&1)
        {
                if (params)
                {
                        paramdat[params-1]=val;
                        params--;
                        if (!params)
                        {
                                switch (command)
                                {
                                        case 0x03: /*Specify*/
//                                        printf("Specified %02X %02X\n",paramdat[1],paramdat[0]);
                                        fdcstatus=0x80;
                                        break;

                                        case 0x04: /*Sense drive status*/
                                        st3=0x60;
                                        if (!fdctrack) st3|=0x10;
                                        fdcstatus=0xD0;
                                        readparams=1;
                                        break;

                                        case 0x06: /*Read sectors*/
//                                        printf("Read sectors %02X %02X %02X %02X %02X %02X %02X %02X\n",paramdat[7],paramdat[6],paramdat[5],paramdat[4],paramdat[3],paramdat[2],paramdat[1],paramdat[0]);
                                        starttrack=paramdat[6];
                                        startsector=paramdat[4]&15;
                                        endsector=paramdat[2]&15;
                                        startreal=0;
/*                                        for (c=0;c<11;c++)
                                        {
                                                printf("Sector %i ID %02X\n",c,discid[starttrack][c][2]);
                                                if (discid[starttrack][c][2]==paramdat[4])
                                                {
                                                        startreal=c;
                                                        break;
                                                }
                                        }
                                        if (c==11)
                                        {
                                                printf("Sector %02X not found on track %02X\n",startsector,starttrack);
                                                command=0x86;
                                                reading=0;
                                                readparams=7;
                                                fdcstatus=0xD0;
//                                                exit(-1);
                                        }
                                        else
                                        {*/
//                                        printf("FDC read %02X %02X %i %i %i\n",paramdat[4],endsector,startreal,c,starttrack);
//                                        printf("FDC Read sector track %i start %i end %i\n",starttrack,startsector,endsector);
                                        posinsector=0;
                                        readparams=1;
                                        reading=1;
                                        fdcstatus=0xF0;
//                                        }
//                                        printf("Start - track %i sector %i\n",starttrack,startsector);
                                        break;

                                        case 0x07: /*Recalibrate*/
//                                        printf("Recalibrate %02X\n",paramdat[0]);
                                        fdcstatus=0x80;
                                        fdctrack=0;
                                        fdcint=1;
                                        break;

                                        case 0x0A: /*Read sector ID*/
                                        fdcstatus|=0x60;
                                        readparams=7;
                                        break;

                                        case 0x0F: /*Seek*/
//                                        printf("Seek %02X %02X\n",paramdat[1],paramdat[0]);
                                        fdcstatus=0x80;
                                        fdctrack=paramdat[0];
                                        fdcint=1;
//                                        output=1;
                                        break;

                                        default:
                                        //printf("Executing bad command %02X\n",command);
                                        exit(-1);
                                }
                        }
                }
                else
                {
                        command=val&0x1F;
                        switch (command)
                        {
                                case 0: case 0x1F: return; /*Invalid*/
                                case 0x03: /*Specify*/
                                params=2;
                                fdcstatus|=0x10;
                                break;

                                case 0x04: /*Sense drive status*/
                                params=1;
                                fdcstatus|=0x10;
                                break;

                                case 0x06: /*Read sectors*/
//                                if (output) exit(0);
                                params=8;
                                fdcstatus|=0x10;
                                discon=1;
                                break;

                                case 0x07: /*Recalibrate*/
                                params=1;
                                fdcstatus|=0x10;
                                break;

                                case 0x08: /*Sense interrupt state*/
                                st0=0x21;
                                if (!fdcint) st0|=0x80;
                                else         fdcint=0;
                                fdcstatus|=0xD0;
                                readparams=2;
                                break;

                                case 0x0A: /*Read sector ID*/
                                params=1;
                                fdcstatus|=0x10;
                                break;

                                case 0x0F: /*Seek*/
                                params=2;
                                fdcstatus|=0x10;
                                break;

                                default:
                                //printf("Starting bad command %02X\n",command);
                                exit(-1);
                        }
                }
        }
}
