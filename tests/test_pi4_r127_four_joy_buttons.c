/* R127 USB face-button regression test using production Pi4 HID descriptor
 * parser and input bridge. Mocks only the AGA port sink, not HID decoding.
 * Run on host; no USB hardware needed. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "frf_native_usb_input.h"
static uint16_t joydat;
static uint32_t pad_buttons;
static int primary_fire;
static unsigned fire_updates,extra_updates;
void frf_vcia_keyboard_raw(uint8_t v){(void)v;}
void aga_virtual_mouse_update(int dx,int dy,unsigned buttons)
{(void)dx;(void)dy;(void)buttons;}
void aga_virtual_joy_set(int port,uint16_t joy,int fire)
{
 if(port!=1){fprintf(stderr,"bad joy port %d\n",port);exit(2);}
 joydat=joy;primary_fire=fire;
 ++fire_updates;
}
void aga_virtual_pad_buttons(int port,uint32_t buttons)
{
 if(port!=1){fprintf(stderr,"bad pad port %d\n",port);exit(3);}
 pad_buttons=buttons;
 ++extra_updates;
}
static void must(int condition,const char *why)
{
 if(!condition){fprintf(stderr,"FAIL %s\n",why);exit(4);}
}
static void check_button(unsigned input)
{
 frf_native_usb_pad_update_axes(0,0,0,0,input,8);
 must(primary_fire==1,"face button activates CIA fire");
 must(pad_buttons==input,"extra buttons retain independent state");
 must(joydat==0,"press does not move joystick");
 if(input==2u)must((pad_buttons&2u)!=0,"button2 POTGOR14");
 if(input==4u)must((pad_buttons&4u)!=0,"button3 POTGOR12");
}
int main(void)
{
 for(unsigned button=0;button<4;button++)check_button(1u<<button);
 frf_native_usb_pad_update_axes(0,0,0,0,0u,8);
 must(!primary_fire && !pad_buttons,"button release clears fire and extra pins");
 frf_native_usb_pad_update_axes(0,0,0,0,0x0fu,8);
 must(primary_fire && (pad_buttons&0x0fu)==0x0fu,"combined four-button report");
 frf_native_usb_pad_release();
 must(!primary_fire && !pad_buttons && !joydat,"disconnect clears latched inputs");
 /* Other high buttons do not falsely trigger primary fire. */
 frf_native_usb_pad_update_axes(0,0,0,0,0x10u,8);
 must(!primary_fire && pad_buttons==0x10u,"non-face button separate");
 /* Direction handling remains unchanged. */
 frf_native_usb_pad_update_axes(-127,0,0,0,0,8);
 must((joydat&0x0200u)!=0 && !primary_fire,"left direction JOY1DAT");

 /* Generic HID gamepad report descriptor with four face buttons,
  * four bits of padding and X/Y 8-bit axes. */
 const uint8_t descriptor[]={
   0x05,0x01,0x09,0x05,0xA1,0x01,
   0x05,0x09,0x19,0x01,0x29,0x04,
   0x15,0x00,0x25,0x01,0x75,0x01,0x95,0x04,0x81,0x02,
   0x75,0x04,0x95,0x01,0x81,0x03,
   0x05,0x01,0x09,0x30,0x09,0x31,
   0x15,0x00,0x26,0xFF,0x00,0x75,0x08,0x95,0x02,0x81,0x02,0xC0
 };
 frf_native_usb_pad_set_identity_for(0u,0x1234u,0x5678u);
 must(frf_native_usb_pad_set_descriptor_for(0u,descriptor,sizeof(descriptor)),
      "parse generic four-button HID descriptor");
 for(unsigned b=0;b<4;b++) {
   const uint8_t report[3]={(uint8_t)(1u<<b),127u,127u};
   must(frf_native_usb_pad_report_for(0u,report,sizeof(report)),
        "accept generic 3-byte HID");
   must(primary_fire,"generic mapped face button");
   must((pad_buttons&(1u<<b))!=0,"generic preserved exact button bit");
 }
 const uint8_t released[3]={0u,127u,127u};
 must(frf_native_usb_pad_report_for(0u,released,sizeof(released)),
      "generic all released");
 must(!primary_fire,"generic release clears fire");

 /* Pi3-compatible 2563:0576 10-byte report ID1. */
 frf_native_usb_pad_set_identity_for(0u,0x2563u,0x0576u);
 must(frf_native_usb_pad_enable_shanwan_for(0u),"ShanWan selected");
 for(unsigned b=0;b<4;b++) {
   uint8_t r[10]={1u,(uint8_t)(1u<<b),0u,8u,
                  128u,128u,128u,128u,0u,0u};
   must(frf_native_usb_pad_report_for(0u,r,sizeof(r)),"ShanWan report valid");
   must(primary_fire,"ShanWan face button reaches primary fire");
   must((pad_buttons&(1u<<b))!=0,"ShanWan extra button state");
 }
 must(fire_updates>=15u && extra_updates==fire_updates,
      "each pad update reaches primary and POTGOR sinks");
 printf("PASS R127: four face buttons CIA fire, extra POTGOR, disconnect, generic HID and ShanWan reports (%u updates)\n",
        fire_updates);
 return 0;
}
