/* R143 Pi4 analogue vertical direction test, host-only; no USB hardware. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "frf_native_usb_input.h"
static uint16_t last_joy;
static unsigned last_buttons,last_port, last_fire,updates;
void frf_vcia_keyboard_raw(uint8_t raw){(void)raw;}
void aga_virtual_mouse_update(int dx,int dy,unsigned buttons){(void)dx;(void)dy;(void)buttons;}
void aga_virtual_joy_set(int port,uint16_t joydat,int fire){
    last_port=(unsigned)port; last_joy=joydat; last_fire=(unsigned)fire; ++updates;
}
void aga_virtual_pad_buttons(int port,uint32_t buttons){
    (void)port;last_buttons=buttons;
}
static void check(int yes,const char *msg){
    if(!yes){fprintf(stderr,"FAIL R143 %s: joy=%04x B=%u port=%u fire=%u updates=%u\n",
             msg,last_joy,last_buttons,last_port,last_fire,updates);exit(2);}
}
static void direction(const char *name,int what){
    if(what==1)check(last_joy==0x0100u,name);
    else if(what==2)check(last_joy==0x0001u,name);
    else check(last_joy==0u,name);
}
static void swpack(uint8_t *b,unsigned lx,unsigned ly,unsigned rx,unsigned ry){
    memset(b,0,12);b[0]=0x30u;
    b[6]=(uint8_t)lx;b[7]=(uint8_t)((lx>>8)&15u)|((uint8_t)(ly&15u)<<4);
    b[8]=(uint8_t)(ly>>4);
    b[9]=(uint8_t)rx;b[10]=(uint8_t)((rx>>8)&15u)|((uint8_t)(ry&15u)<<4);
    b[11]=(uint8_t)(ry>>4);
}
int main(void){
    static const uint8_t desc[]={
      0x05,0x01,0x09,0x05,0xa1,0x01, /* application gamepad */
      0x15,0x00,0x26,0xff,0x00,      /* logical 0..255 */
      0x75,0x08,0x95,0x05,            /* five eight-bit fields */
      0x09,0x30,0x09,0x31,0x09,0x33,0x09,0x34,0x09,0x39,
      0x81,0x02,0xc0 };
    frf_native_usb_pad_set_identity_for(0u,0x1111u,0x2222u);
    check(frf_native_usb_pad_set_descriptor_for(0u,desc,sizeof desc),"descriptor");
    uint8_t r[]={128,128,128,128,8};
    r[1]=255;check(frf_native_usb_pad_report_for(0u,r,sizeof r),"generic L up accepted");
    direction("generic L up",1);
    r[1]=0;frf_native_usb_pad_report_for(0u,r,sizeof r);direction("generic L down",2);
    r[1]=128;r[3]=255;
    frf_native_usb_pad_report_for(0u,r,sizeof r);direction("generic R up",1);
    r[3]=0;frf_native_usb_pad_report_for(0u,r,sizeof r);direction("generic R down",2);
    r[3]=128;r[4]=0;
    frf_native_usb_pad_report_for(0u,r,sizeof r);direction("HAT up unchanged",1);
    r[4]=4;
    frf_native_usb_pad_report_for(0u,r,sizeof r);direction("HAT down unchanged",2);
    r[4]=8;frf_native_usb_pad_report_for(0u,r,sizeof r);direction("centered",0);
    frf_native_usb_pad_set_identity_for(1u,0x2563u,0x0576u);
    check(frf_native_usb_pad_enable_shanwan_for(1u),"ShanWan enable");
    uint8_t s[]={1,0,0,8,128,255,128,128,0,0};
    check(frf_native_usb_pad_report_for(1u,s,sizeof s),"shanwan L accepted");
    direction("shanwan left up",1);
    s[5]=128;s[7]=255;frf_native_usb_pad_report_for(1u,s,sizeof s);
    direction("shanwan right up",1);
    s[7]=0;frf_native_usb_pad_report_for(1u,s,sizeof s);
    direction("shanwan right down",2);
    frf_native_usb_pad_set_identity_for(2u,0x057eu,0x2009u);
    check(frf_native_usb_pad_enable_switchpro_for(2u),"Switch enable");
    uint8_t sw[12];
    swpack(sw,2048,4095,2048,2048);
    check(frf_native_usb_pad_report_for(2u,sw,sizeof sw),"Switch 30 accepted");
    direction("Switch L up",1);
    swpack(sw,2048,2048,2048,4095);
    frf_native_usb_pad_report_for(2u,sw,sizeof sw);
    direction("Switch R up",1);
    swpack(sw,2048,0,2048,2048);
    frf_native_usb_pad_report_for(2u,sw,sizeof sw);
    direction("Switch L down",2);
    memset(sw,0,sizeof sw);sw[0]=0x3f;sw[3]=8u;
    sw[4]=0;sw[5]=0x80;sw[6]=0xff;sw[7]=0xff;
    check(frf_native_usb_pad_report_for(2u,sw,sizeof sw),"Switch 3f accepted");
    direction("Switch 3F L up",1);
    sw[6]=0;sw[7]=0;
    frf_native_usb_pad_report_for(2u,sw,sizeof sw);
    direction("Switch 3F L down",2);
    frf_native_usb_pad_update_axes(0,-127,0,0,3u,8);
    direction("JOY1DAT quadrature preserved",1);
    check(last_buttons==3u && last_fire==1u && last_port==1u,"buttons unchanged");
    puts("PASS R143: generic HID left/right Y, ShanWan dual Y, Switch Pro dual Y and 3F, hat up/down, buttons and JOY1DAT");
    return 0;
}
