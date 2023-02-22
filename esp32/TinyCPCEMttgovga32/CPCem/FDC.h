#ifndef _FDC_H
 #define _FDC_H
 
 void getDskList(void);
 void loaddsk2Flash(unsigned char id);
 unsigned char readfdc(unsigned short addr);
 void writefdc(unsigned short addr, unsigned char val);
 
#endif
