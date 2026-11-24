/* R146 Pi4 wheel direction correction with NewMouse CIA bridge host regressions. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "frf_native_usb_input.h"

static uint8_t keyq[256];
static unsigned keycount, mousecount, last_buttons, last_fire;
static int last_x,last_y;
static uint16_t last_joy;
void frf_vcia_keyboard_raw(uint8_t raw) {
    if(keycount>=sizeof keyq){fprintf(stderr,"keyboard queue test overflow\n");exit(2);}
    keyq[keycount++]=raw;
}
void aga_virtual_mouse_update(int dx,int dy,unsigned buttons) {
    last_x=dx;last_y=dy;last_buttons=buttons;mousecount++;
}
void aga_virtual_joy_set(int port,uint16_t joydat,int fire) {
    (void)port;last_joy=joydat;last_fire=(unsigned)fire;
}
void aga_virtual_pad_buttons(int port,uint32_t buttons) {(void)port;(void)buttons;}
static void must(int ok,const char *why){
    if(!ok){fprintf(stderr,"FAIL R144: %s\n",why);exit(2);}
}
/* Standard HID wheel mouse: three variable buttons + five padding bits +
   three signed relative 8-bit fields X,Y,Wheel. No boot/report-ID ambiguity. */
static const uint8_t mouse4[]={
    0x05,0x01,0x09,0x02,0xa1,0x01,
    0x09,0x01,0xa1,0x00,
    0x05,0x09,0x19,0x01,0x29,0x03,
    0x15,0x00,0x25,0x01,
    0x95,0x03,0x75,0x01,0x81,0x02,
    0x95,0x01,0x75,0x05,0x81,0x03,
    0x05,0x01,0x09,0x30,0x09,0x31,0x09,0x38,
    0x15,0x81,0x25,0x7f,
    0x75,0x08,0x95,0x03,0x81,0x06,
    0xc0,0xc0
};
static const uint8_t mouse3[]={
    0x05,0x01,0x09,0x02,0xa1,0x01,
    0x09,0x01,0xa1,0x00,
    0x05,0x09,0x19,0x01,0x29,0x03,
    0x15,0x00,0x25,0x01,
    0x95,0x03,0x75,0x01,0x81,0x02,
    0x95,0x01,0x75,0x05,0x81,0x03,
    0x05,0x01,0x09,0x30,0x09,0x31,
    0x15,0x81,0x25,0x7f,
    0x75,0x08,0x95,0x02,0x81,0x06,
    0xc0,0xc0
};
static void send(unsigned idx,const uint8_t *wire,unsigned wire_len,int x,int y,
                 unsigned buttons,int wheel,unsigned keytotal){
    uint8_t normal[4]={0};
    must(frf_native_usb_mouse_decode_for(idx,wire,wire_len,normal),
         "descriptor mouse decode");
    must((unsigned)normal[0]==buttons && (int8_t)normal[1]==x &&
         (int8_t)normal[2]==y && (int8_t)normal[3]==wheel,
         "X/Y/buttons/wheel normalized correctly");
    frf_native_usb_mouse_report(normal,4);
    must(last_x==x && last_y==y && last_buttons==buttons,
         "normal mouse pointer/buttons unchanged");
    must(keycount==keytotal,"wheel raw event count");
}
int main(void){
    must(frf_native_usb_mouse_set_descriptor_for(1,mouse4,sizeof mouse4),
         "no-ID 4-byte report descriptor");
    uint8_t r[]={5,0x02,0xfe,1}; /* L+middle, dx2, dy-2, wheel up1 */
    send(1,r,sizeof r,2,-2,5,1,2);
    must(keyq[0]==0x7b && keyq[1]==0xfb,"HID positive now maps NewMouse down 0x7B / release FB");
    r[0]=2;r[1]=0xff;r[2]=3;r[3]=0xfe; /* right, dx-1,dy3,wheel down2 */
    send(1,r,sizeof r,-1,3,2,-2,6);
    must(keyq[2]==0x7a && keyq[3]==0xfa &&
         keyq[4]==0x7a && keyq[5]==0xfa,
         "HID negative now maps NewMouse up 0x7A / release FA for both detents");
    r[0]=0;r[1]=0;r[2]=0;r[3]=0;
    send(1,r,sizeof r,0,0,0,0,6);
    must(frf_r144_wheel_up==2 && frf_r144_wheel_down==1 &&
         frf_r144_wheel_reports==2,"wheel telemetry");
    /* Explicit report ID 7: button offset starts at bit8; wheel bit32. */
    uint8_t id_mouse[sizeof mouse4+2];
    memcpy(id_mouse,mouse4,sizeof mouse4);
    id_mouse[10]=0x85;id_mouse[11]=0x07;
    /* Add ID after mouse application+Pointer collection. */
    uint8_t with_id[sizeof mouse4+2];
    memcpy(with_id,mouse4,10);
    with_id[10]=0x85;with_id[11]=0x07;
    memcpy(with_id+12,mouse4+10,sizeof mouse4-10);
    must(frf_native_usb_mouse_set_descriptor_for(2,with_id,sizeof with_id),
         "report ID mouse descriptor");
    uint8_t report_id[]={7,4,0xfb,9,0x01};
    send(2,report_id,sizeof report_id,-5,9,4,1,8);
    must(keyq[6]==0x7b && keyq[7]==0xfb,
         "report ID wheel uses correct extra byte offset");
    report_id[0]=6;
    uint8_t bogus[4];
    must(!frf_native_usb_mouse_decode_for(2,report_id,sizeof report_id,bogus),
         "ignore unrelated composite report ID");
    must(!frf_native_usb_mouse_decode_for(2,report_id,3,bogus),
         "ignore short report");
    /* No wheel descriptor must refuse report-mode switch. */
    must(!frf_native_usb_mouse_set_descriptor_for(3,mouse3,sizeof mouse3),
         "ordinary boot mouse has no wheel; remain in boot protocol");
    uint8_t boot[]={1,1,0xff};
    frf_native_usb_mouse_report(boot,3);
    must(last_buttons==1 && last_x==1 && last_y==-1 && keycount==8,
         "legacy 3-byte mouse still works without wheel injection");
    uint8_t extended[]={4,0,0,0xff};
    frf_native_usb_mouse_report(extended,4);
    must(last_buttons==4 && keyq[8]==0x7a && keyq[9]==0xfa,
         "four-byte boot extended wheel and middle button");
    /* Quiescence/move without wheel must emit no keys. */
    frf_native_usb_mouse_report((uint8_t[]){0,8,9,0},4);
    must(keycount==10 && last_x==8 && last_y==9,
         "wheel zero does not flood or alter pointer");
    /* Saturate a malformed high-resolution report without flooding CIA. */
    frf_native_usb_mouse_report((uint8_t[]){0,0,0,100},4);
    must(keycount==26 && frf_r144_wheel_down==10,
         "8-detent per-report bound");
    must(mousecount==8, "mouse reports counted, wheel independent");
    puts("PASS R146: reversed HID wheel sign, NewMouse up/down make-break, report ID and boot 3B/4B mice, pointer, buttons, 8-detent bound");
    return 0;
}
