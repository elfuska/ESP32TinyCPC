//C72B - try command
//A9B0
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "FDC.h"
#include "gbGlobals.h"
#include "hardware.h"
#include <iostream>
#include "SD.h"

static SPIClass customSPI;

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
unsigned char discid[43][15][4];
Tdiscdat *discdat;
int max_list_dsk=0;
char * gb_list_dsk_title[MAX_DSKS];

#define listItemsTotal 100 
#define listItemMaxChar 50 

using namespace std;


void mountSD() {
  customSPI.begin(SDCARD_CLK, SDCARD_MISO, SDCARD_MOSI, SDCARD_CS);
  SD.begin(SDCARD_CS, customSPI, 4000000, "/sd");
  return; 
}


void getDskList() {

  File root = SD.open("/dsk");

  while (true) {
    File entry = root.openNextFile();
    if (!entry) {break;}
    else {
      gb_list_dsk_title[max_list_dsk] = (char *)malloc(listItemMaxChar);
      sprintf(gb_list_dsk_title[max_list_dsk],"%s",entry.name());
      max_list_dsk++; //increment counter of files
      entry.close();
    }
  }
  return; 
}


void loaddsk2Flash(unsigned char id)
{

        int numsect, sectorSize, sides, trackSize;
        int c,d;
        char *head, fileName[max_list_dsk];
        unsigned char *dskhead,*trkhead, *trackSizeTable;

        sprintf(fileName, "/sd/dsk/%s", gb_list_dsk_title[id]);
        
        head=(char *)ps_malloc(40 * sizeof(char));
        dskhead=(unsigned char *)ps_malloc(0x100 * sizeof(unsigned char));
        trkhead=(unsigned char *)ps_malloc(0x100 * sizeof(unsigned char));
        discdat=(Tdiscdats *)ps_malloc(sizeof(Tdiscdats));
        
        FILE *f=fopen(fileName, "rb");
        if (!f) {
          printf("No se ha cargado el disco\n");
          return;
        }
        fread(dskhead,0x100,1,f);        
        for (c=0;c<40;c++) head[c]=0;
        for (c=0;c<0x21;c++) head[c]=dskhead[c];
        
        if (strncmp((const char *)head,"MV - CPC",8)==0)  {
          printf("Cabecera MV - CPC\n");
          disctracks=dskhead[0x30];
          sides = dskhead[0x31]; // number of sides  
          printf("%i tracks, %i sides\n",disctracks, sides);
        
          trackSize = (dskhead[0x32] | (dskhead[0x33] << 8)) - 0x100;     
          printf("Track size: %i\n", trackSize);   

          for (d=0;d<disctracks;d++) {
            fread(trkhead,0x100,1,f);
              
            if (strncmp((const char *)trkhead,"Track-Info",10)!=0) {
              printf("Error en el formato del disco\n");
              fclose(f);                  
              return;
            }    

            sectorSize = 0x80<<trkhead[0x14];
            discsects[d]=numsect=trkhead[0x15]; 
            printf("%i sectors, sector size: %i\n",numsect, sectorSize);
            for (c=0;c<numsect;c++) {
                    discid[d][c][0]=trkhead[0x18+(c<<3)];
                    discid[d][c][1]=trkhead[0x19+(c<<3)];
                    discid[d][c][2]=trkhead[0x1A+(c<<3)];
                    discid[d][c][3]=trkhead[0x1B+(c<<3)];
                    printf("%i %i %i %i  ",discid[d][c][0],discid[d][c][1],discid[d][c][2],discid[d][c][3]);
                    fread(discdat->datos[d][(discid[d][c][2]-1)&15],sectorSize,1,f);                        
            }
            printf("\n");
          }

        } else if (strncmp((const char *)head,"EXTENDED",8)==0)  {
          printf("Cabecera EXTENDED\n");
          disctracks=dskhead[0x30];
          sides = dskhead[0x31] & 3; // number of sides
          trackSizeTable = dskhead+0x34; // pointer to track size table in DSK header
          printf("%i tracks, %i sides\n",disctracks, sides);
          //printf("%s\n", dskhead);
          for (d=0;d<disctracks;d++) {
                  fread(trkhead,0x100,1,f);

                  if (strncmp((const char *)trkhead,"Track-Info",10)!=0) {
                    printf("Error en el formato del disco\n");
                    fclose(f);                  
                    return;
                  }
                  
                  trackSize = (*trackSizeTable++ << 8); // track size in bytes
                  printf("Track size: %i\n", trackSize);
                  if (trackSize==0) break;           
                  discsects[d]=numsect=trkhead[0x15];
                  sectorSize=0x80<<(trkhead[0x14] & 7);
                  printf("%i sectors, sector size %i\n",numsect, sectorSize);
                  for (c=0;c<numsect;c++) {
                          discid[d][c][0]=trkhead[0x18+(c<<3)];
                          discid[d][c][1]=trkhead[0x19+(c<<3)];
                          discid[d][c][2]=trkhead[0x1A+(c<<3)];
                          discid[d][c][3]=trkhead[0x1B+(c<<3)];

                          unsigned int actualSectorSize=((int)trkhead[0x18+(c<<3)+6]) | (((int)trkhead[0x18+(c<<3)+7])<<8);
                          printf("%i %i %i %i  - actualSectorSize: %i ",discid[d][c][0],discid[d][c][1],discid[d][c][2],discid[d][c][3], actualSectorSize);
                          fread(discdat->datos[d][(discid[d][c][2]-1)&15],actualSectorSize,1,f);
                  }
                  printf("\n");
          }
        } //else
        printf("Leido\n");
        fclose(f);
}

unsigned char readfdc(unsigned short addr)
{
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
                                //printf("Read track %i sector %i pos %i\n",fdctrack,startreal,posinsector);
                                //printf("%c",temp);
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
                                                //FUSKendread=1;
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
                                                //FUSKstartreal=0;
                                                for (c=0;c<11;c++)
                                                {
                                                        if ((discid[starttrack][c][2]&15)==(startsector&15))
                                                        {
                                                                //FUSKstartreal=c;
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
                                        //FUSstartreal=0;
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
