//Port Cpc-em (Tom Walker) emulator to TTGO VGA32 by ackerman:
// DSK 44 tracks 11 sectors
// VGA 400x300, 320x200
// Mode 464, 664, 6128
// Modo 128K (inestable)
// Low video ram mode
// Mode 8 colors, 64 colors
// Audio AY8912 fabgl library 0.9.0 (not include)
// VGA library bitluni 0.3.3 (include)
// gbConfig options configuration compile

#include "Emulator/Keyboard/PS2Kbd.h"
#include <Arduino.h>
//Para ahorrar memoria
//JJ #include <esp_bt.h>

#include "gbConfig.h"

#ifdef use_lib_sound_ay8912
 #include "fabgl.h" //Para fabgl
 #include "fabutils.h" //Para fabgl
#endif 

#include "CPCEM.h"
#include "MartianVGA.h"
#include "def/Font.h"
#include "def/hardware.h"
#include "driver/timer.h"
#include "soc/timer_group_struct.h"

#include "CRTC.h"
#include "FDC.h"
#include "PSG.h"
#include "Z80.h"
#include "gb_globals.h"
#include "osd.h"

#ifdef COLOR_3B
 #ifdef use_lib_vga_low_memory
  VGA3BitI vga; 
 #else
  VGA3Bit vga;
 #endif 
#else
 #ifdef use_lib_vga_low_memory
  VGA6BitI vga;
 #else
  VGA6Bit vga;
 #endif
#endif

unsigned char gb_run_emulacion = 1; //Ejecuta la emulacion
unsigned char gb_current_ms_poll_sound = gb_ms_sound;
unsigned char gb_current_ms_poll_keyboard = gb_ms_keyboard;
unsigned char gb_current_frame_crt_skip= gb_frame_crt_skip; //No salta frames
unsigned char gb_current_delay_emulate_ms= gb_delay_emulate_ms;
unsigned char gb_sdl_blit=0;
unsigned char gb_screen_xOffset=0;
static unsigned long gb_time_ini_espera;

//volatile unsigned char keymap[256];
//volatile unsigned char oldKeymap[256];
//unsigned char *ramArray[8];
static unsigned long gb_currentTime;
static unsigned long gb_keyboardTime;
unsigned char *ram;
#ifdef use_lib_mem_blocks
 unsigned char *ramArray[2]; //2 bloques de 64 KB
#endif
//unsigned char gb_sdl_current_frame=0;
#ifdef use_lib_ultrafast_vga
 unsigned char ** ptrVGA;
#endif

//JJ unsigned char gb_sdl_blit=0;//solo necesito en pc
unsigned char model;
//JJ int soundavail=1; //no necesito
//JJ char discname[260]="";//no lo necesito
//JJ int quit; //no lo necesito
int soundon;
int spdc=0;

#ifdef use_lib_sound_ay8912
 SoundGenerator soundGenerator;
 SineWaveformGenerator gb_sineArray[3];
 unsigned char gbVolMixer_before[3]={0,0,0};
 unsigned short int gbFrecMixer_before[3]={0,0,0};
 unsigned char gbVolMixer_now[3]={0,0,0};
 unsigned short int gbFrecMixer_now[3]={0,0,0}; 
 unsigned char gb_silence_all_channels=1; 
 static unsigned long gb_sdl_time_sound_before;
  
 void sound_cycleFabgl(void);
 void jj_mixpsg(void);
#endif

#ifdef use_lib_sound_ay8912
 void SilenceAllChannels()
 {  
  for (unsigned char i=0;i<3;i++)  
  {
   gb_sineArray[i].setFrequency(0);
   gb_sineArray[i].setVolume(0);
   gbVolMixer_before[i] = gbVolMixer_now[i] = 0;
   gbFrecMixer_before[i] = gbFrecMixer_now[i] = 0;
  }  
 }

 inline void sound_cycleFabgl()
 {  
  //AY8912
  for (unsigned char i=0;i<3;i++)
  {
   if (gbVolMixer_now[i] != gbVolMixer_before[i])
   {
    gb_sineArray[i].setVolume((gbVolMixer_now[i]<<2));
    gbVolMixer_before[i] = gbVolMixer_now[i];
   }
   if (gbFrecMixer_now[i] != gbFrecMixer_before[i])
   {
    gb_sineArray[i].setFrequency(gbFrecMixer_now[i]);
    gbFrecMixer_before[i] = gbFrecMixer_now[i];
   }
  }
 }

 inline void jj_mixpsg()
 {
  int auxFrec;
  gbVolMixer_now[0] = psgregs[8]&15;//((psgregs[8]&15)>4)?16:0;
  gbVolMixer_now[1] = psgregs[9]&15;//((psgregs[9]&15)>4)?16:0;
  gbVolMixer_now[2] = psgregs[10]&15;//((psgregs[10]&15)>4)?16:0;
  if (gbVolMixer_now[0] == 0) gbFrecMixer_now[0] = 0;
  else{
   gbVolMixer_now[0]=15;
   auxFrec =((psgregs[0]|((psgregs[1]<<8)&0xF00)))+1;
   auxFrec = (auxFrec>0)?(62500/auxFrec):0;
   if (auxFrec>15000) auxFrec=15000;
   gbFrecMixer_now[0] = auxFrec;
  }
  if (gbVolMixer_now[1] == 0) gbFrecMixer_now[1] = 0;
  else{
   gbVolMixer_now[1]=15; 
   auxFrec = ((psgregs[2]|((psgregs[3]<<8)&0xF00)))+1;  
   auxFrec = (auxFrec>0)?(62500/auxFrec):0;
   if (auxFrec>15000) auxFrec=15000;   
   gbFrecMixer_now[1] = auxFrec;
  }
  if (gbVolMixer_now[2] == 0) gbFrecMixer_now[2] = 0;
  else{
   gbVolMixer_now[2]=15;
   auxFrec = ((psgregs[4]|((psgregs[5]<<8)&0xF00)))+1;
   auxFrec = (auxFrec>0)?(62500/auxFrec):0;
   if (auxFrec>15000) auxFrec=15000;   
   gbFrecMixer_now[2] = auxFrec;
  }
 } 
#endif 



void SDL_keys_poll()
{

   cpckeys[9][7] = cpckeys[2][0]= (keymap[0x12]==0 && keymap[0x45]==0)?0:1;//delete
   cpckeys[2][1] = keymap[0x54]; //case SDLK_OPENBRACE: 
   cpckeys[2][2]= keymap[0x5a]; //return
   cpckeys[2][3] = keymap[0x5B]; //case SDLK_CLOSEBRACE:   
   //case SDLK_LEFT: cpckeys[2][4]= 0; break;
   cpckeys[2][5]= keymap[0x59];//rshift
   cpckeys[2][6]= keymap[0x5D];; //BACKSLASH barra izquierda
   cpckeys[2][7]= keymap[0x14];//rcontrol   
                          
   //case SDLK_F2: cpckeys[3][0]= 0; break;
   cpckeys[3][1]= keymap[0x4E]; //MINUS -
   //case SDLK_TILDE:
   cpckeys[3][3]= keymap[0x4d]; //p
   //case SDLK_COLON: cpckeys[3][4]= 0; break; //:   
   cpckeys[3][5] = keymap[0x52]; //case SDLK_QUOTE: cpckeys[3][5]= 0; break; //'
   //case SDLK_SLASH: cpckeys[3][6]= 0; break;   
   cpckeys[3][7]= keymap[0x49]; //teclado stop .


   cpckeys[4][0]= keymap[0x45]; //0
   cpckeys[4][1]= keymap[0x46]; //9
   cpckeys[4][2]= keymap[0x44]; //o
   cpckeys[4][3]= keymap[0x43]; //i
   cpckeys[4][4]= keymap[0x4b]; //l
   cpckeys[4][5]= keymap[0x42]; //k
   cpckeys[4][6]= keymap[0x3a]; //m
   cpckeys[4][7]= keymap[0x41]; //,

   cpckeys[5][0]= keymap[0x3e];//8
   cpckeys[5][1]= keymap[0x3d];//7
   cpckeys[5][2]= keymap[0x3c];//u
   cpckeys[5][3]= keymap[0x35];//y
   cpckeys[5][4]= keymap[0x33];//h
   cpckeys[5][5]= keymap[0x3b];//j
   cpckeys[5][6]= keymap[0x31];//n
   cpckeys[5][7]= keymap[0x29];//space

   cpckeys[6][0]= keymap[0x36];//6
   cpckeys[6][1]= keymap[0x2e];//5
   cpckeys[6][2]= keymap[0x2d];//r
   cpckeys[6][3]= keymap[0x2c];//t
   cpckeys[6][4]= keymap[0x34];//g
   cpckeys[6][5]= keymap[0x2b];//f
   cpckeys[6][6]= keymap[0x32];//b
   cpckeys[6][7]= keymap[0x2a];//v

   cpckeys[7][0]= keymap[0x25];//4
   cpckeys[7][1]= keymap[0x26];//3
   cpckeys[7][2]= keymap[0x24];//e
   cpckeys[7][3]= keymap[0x1d];//w
   cpckeys[7][4]= keymap[0x1b];//s
   cpckeys[7][5]= keymap[0x23];//d
   cpckeys[7][6]= keymap[0x21];//c
   cpckeys[7][7]= keymap[0x22];//x 

   cpckeys[8][0]= keymap[0x16];//1
   cpckeys[8][1]= keymap[0x1e];//2
   cpckeys[8][2]= keymap[0x76];//ESCAPE
   cpckeys[8][3]= keymap[0x15];//q
   cpckeys[8][4]= keymap[0x0D];//tab
   cpckeys[8][5]= keymap[0x1c];//a
   cpckeys[8][6]= keymap[0x58];//capslock
   cpckeys[8][7]= keymap[0x1a];//z

   cpckeys[9][0]= keymap[KEY_CURSOR_UP];  //UP
   cpckeys[9][1]= keymap[KEY_CURSOR_DOWN]; //down
   cpckeys[9][2]= keymap[KEY_CURSOR_LEFT]; //left
   cpckeys[9][3]= keymap[KEY_CURSOR_RIGHT]; //right
   //case SDLK_INSERT: cpckeys[9][4]= 0; break;
   //
   //
   cpckeys[9][7]= keymap[KEY_BACKSPACE]; //Backspace Borrar KEY_BACKSPACE
}



void setup()
{
 //DO NOT turn off peripherals to recover some memory
 //esp_bt_controller_deinit(); //Reduzco consumo RAM
 //esp_bt_controller_mem_release(ESP_BT_MODE_BTDM); //Reduzco consumo RAM
 #ifdef use_lib_mem_blocks
  //for (int i=0;i<2;i++)
  // ramArray[i] = (unsigned char *)calloc(0x10000,1);//64KB+16KB fix error
  ramArray[0] = (unsigned char *)calloc(0x10000,1);//64KB+16KB fix error
  ramArray[1] = (unsigned char *)calloc(0x10000,1);//64KB
  ram = ramArray[0]; 
 #else
  #ifdef use_lib_cheat_128k
   ram=(unsigned char *)malloc(0x1B000);  //truco 122880 bytes
   memset(ram,1,0x10000);
  #else
   ram=(unsigned char *)malloc(0x10000); 
   memset(ram,1,0x10000);
  #endif 
 #endif 

 #ifdef use_lib_log_serial
  Serial.begin(115200);         
  Serial.printf("HEAP BEGIN %d\n", ESP.getFreeHeap()); 
 #endif

 //ram=(unsigned char *)heap_caps_malloc(0x20000,MALLOC_CAP_8BIT);
 //for (int i=0;i<8;i++)
 // ramArray[i] = (unsigned char *)calloc(0x20000,1);
  
 //Serial.printf("Address RAM: %d\n",ram);


 #ifdef use_lib_log_serial
  Serial.printf("RAM %d\n", ESP.getFreeHeap()); 
 #endif

 #ifdef COLOR_3B
  #ifdef use_lib_400x300
   vga.init(vga.MODE400x300, RED_PIN_3B, GRE_PIN_3B, BLU_PIN_3B, HSYNC_PIN, VSYNC_PIN);       
  #else
   vga.init(vga.MODE320x200, RED_PIN_3B, GRE_PIN_3B, BLU_PIN_3B, HSYNC_PIN, VSYNC_PIN);    
  #endif
 #else
  const int redPins[] = {RED_PINS_6B};
  const int grePins[] = {GRE_PINS_6B};
  const int bluPins[] = {BLU_PINS_6B}; 
  #ifdef use_lib_400x300
   vga.init(vga.MODE400x300, redPins, grePins, bluPins, HSYNC_PIN, VSYNC_PIN);   
  #else
   vga.init(vga.MODE320x200, redPins, grePins, bluPins, HSYNC_PIN, VSYNC_PIN);
  #endif 
 #endif
 vga.setFont(Font6x8);
 vga.clear(BLACK); 
 #ifdef use_lib_ultrafast_vga
  #ifdef use_lib_400x300
   vga.fillRect(0,0,400,300,BLACK);
   vga.fillRect(0,0,400,300,BLACK);//fix mode fast video
  #else
   vga.fillRect(0,0,320,200,BLACK);
   vga.fillRect(0,0,320,200,BLACK);//fix mode fast video
  #endif 
  ptrVGA = vga.backBuffer; //Asignamos  puntero buffer
 #endif  

 #ifdef use_lib_log_serial
  Serial.printf("VGA %d\n", ESP.getFreeHeap()); 
 #endif

 kb_begin();

 //resolution=0;
 model=0;
 soundon=0;
 initz80();
 loadroms2FlashModel();
 resetz80();
 resetcrtc();
 loaddsk2Flash(0);

 gb_keyboardTime = gb_currentTime = millis();

 #ifdef use_lib_sound_ay8912
  gb_sdl_time_sound_before = gb_currentTime;
  for (unsigned char i=0;i<3;i++)
  {
   soundGenerator.attach(&gb_sineArray[i]);
   gb_sineArray[i].enable(true);
   gb_sineArray[i].setFrequency(0);
  }
  soundGenerator.play(true);
 #endif

 #ifdef use_lib_log_serial  
  Serial.printf("END SETUP %d\n", ESP.getFreeHeap()); 
 #endif
}




// VIDEO core 0 *************************************

//Video sin hilos
//void videoTaskNoThread()
//{
//}



// +-------------+
// | LOOP core 1 |
// +-------------+
//
void loop()
{
 gb_currentTime = millis();
 if ((gb_currentTime-gb_keyboardTime) >= gb_current_ms_poll_keyboard)
 { 
  gb_keyboardTime = gb_currentTime;
  SDL_keys_poll();
 }
 do_tinyOSD();

 if ((gb_current_delay_emulate_ms == 0) || (gb_run_emulacion == 1))
 {     
  cpuline=0;
  execz80();
 }

 if (gb_current_delay_emulate_ms != 0)
 {
  if (gb_sdl_blit == 1)
  {
   gb_sdl_blit= 0;
   gb_run_emulacion= 0;
   gb_time_ini_espera = millis();
  }
 }

 #ifdef use_lib_sound_ay8912
  if (gb_silence_all_channels == 1)
   SilenceAllChannels();
  else
  {
   gb_currentTime = millis();
   if ((gb_currentTime-gb_sdl_time_sound_before) >= gb_current_ms_poll_sound)
   {
    gb_sdl_time_sound_before= gb_currentTime;
    jj_mixpsg();
    sound_cycleFabgl();
   }
  }
 #endif   

 if (gb_current_delay_emulate_ms != 0)
 {
  gb_currentTime = millis();     
  if (gb_run_emulacion == 0)  
   if ((gb_currentTime - gb_time_ini_espera) >= gb_current_delay_emulate_ms)   
    gb_run_emulacion = 1;     
 }
 
 //videoTaskNoThread(); 
}
