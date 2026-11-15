#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "frf_native_usb_input.h"
extern volatile uint32_t frf_r127_legacy_decoded_buttons;
extern volatile uint32_t frf_r127_legacy_native_buttons;
extern volatile uint32_t frf_r127_legacy_input_updates;
static uint16_t joydat;
static unsigned fire,pad,fire_count,pad_count;
static void check(int v,const char *reason) {
 if(!v){fprintf(stderr,"FAIL legacy R127: %s\n",reason);exit(4);}
}
void frf_vcia_keyboard_raw(uint8_t data){(void)data;}
void aga_virtual_mouse_update(int x,int y,unsigned b){(void)x;(void)y;(void)b;}
void aga_virtual_joy_set(int port,uint16_t jd,int f){
 check(port==1,"native Amiga joystick port2");
 joydat=jd;fire=(unsigned)f;++fire_count;
}
void aga_virtual_pad_buttons(int port,uint32_t b){
 check(port==1,"native Amiga POTGOR port2");
 pad=(unsigned)b;++pad_count;
}
static void state(unsigned physical,unsigned mapped) {
 check(frf_r127_legacy_decoded_buttons==physical,"raw HID mask retained for diagnostics");
 check(frf_r127_legacy_native_buttons==mapped,"three-button mask only");
 check(fire==((mapped&1u)!=0u),"CIA B1 only, not ORed with others");
 check((pad&7u)==mapped,"POTGOR driver got native bits");
 check((pad&~7u)==0u,"fourth USB button ignored by Amiga port");
}
int main(void) {
 const unsigned cases[]={0u,1u,2u,4u,8u,3u,5u,6u,7u,15u};
 const unsigned masks[]={0u,1u,2u,4u,0u,3u,5u,6u,7u,7u};
 for(unsigned i=0;i<sizeof(cases)/sizeof(cases[0]);i++){
   frf_native_usb_pad_update_axes(0,0,0,0,cases[i],8);
   state(cases[i],masks[i]);
 }
 frf_native_usb_pad_release();
 state(0u,0u);
 check(!fire && !pad && !joydat,"wireless disconnect releases all three");
 /* Generic HID descriptor: four physical face buttons. */
 const uint8_t desc[]={
   0x05,0x01,0x09,0x05,0xA1,0x01,
   0x05,0x09,0x19,0x01,0x29,0x04,
   0x15,0x00,0x25,0x01,0x75,0x01,0x95,0x04,0x81,0x02,
   0x75,0x04,0x95,0x01,0x81,0x03,
   0x05,0x01,0x09,0x30,0x09,0x31,
   0x15,0x00,0x26,0xFF,0x00,0x75,0x08,0x95,0x02,0x81,0x02,0xC0
 };
 frf_native_usb_pad_set_identity_for(0u,0x046du,0xc216u);
 check(frf_native_usb_pad_set_descriptor_for(0u,desc,sizeof(desc)),"generic HID descriptor");
 for(unsigned b=0;b<4;b++){
   const uint8_t report[]={ (uint8_t)(1u<<b),127u,127u };
   check(frf_native_usb_pad_report_for(0u,report,sizeof(report)),"generic report accepted");
   state(1u<<b,b<3u?(1u<<b):0u);
 }
 const uint8_t allup[]={0u,127u,127u};
 check(frf_native_usb_pad_report_for(0u,allup,sizeof(allup)),"generic release");
 state(0u,0u);
 /* 2563:0576 ShanWan Pi3 report ID1 with button byte b1. */
 frf_native_usb_pad_set_identity_for(0u,0x2563u,0x0576u);
 check(frf_native_usb_pad_enable_shanwan_for(0u),"ShanWan selected");
 for(unsigned b=0;b<4;b++){
   uint8_t report[10]={1u,(uint8_t)(1u<<b),0,8,128,128,128,128,0,0};
   check(frf_native_usb_pad_report_for(0u,report,sizeof(report)),"ShanWan ID1 accepted");
   state(1u<<b,b<3u?(1u<<b):0u);
 }
 /* Switch Pro input report 0x30: physical bits 08 04 02 01.
  * 0x01 is the FOURTH button and must not be sent to B1. */
 frf_native_usb_pad_set_identity_for(0u,0x057eu,0x2009u);
 check(frf_native_usb_pad_enable_switchpro_for(0u),"Switch Pro selected");
 for(unsigned b=0;b<4;b++){
   uint8_t report[12]={0x30u,0u,0u,(uint8_t)(0x08u>>b),0u,0u,0u,8u,128u,0u,8u,128u};
   check(frf_native_usb_pad_report_for(0u,report,sizeof(report)),"Switch Pro 0x30 accepted");
   state(b<3u?(1u<<b):0u,b<3u?(1u<<b):0u);
 }
 check(fire_count==pad_count && fire_count>=22,"both Amiga sinks updated on every report");
 printf("PASS Pi4 legacy R127: B1 CIA, B2 POTGOR14, B3 POTGOR12, fourth ignored, generic HID/ShanWan/Switch Pro, disconnect (%u sink updates)\n",fire_count);
 return 0;
}
