
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#define USB_HID_PROTOCOL_KEYBOARD 1u
#define USB_HID_PROTOCOL_MOUSE 2u
struct NativeHID { uint32_t valid,protocol,vid,pid; };
static struct NativeHID g_native_hid[8];
static uint32_t g_native_count,g_frf_msc_slot;
static volatile uint32_t frf_r130_pad_discovered,frf_r130_pad_vidpid;
static volatile uint32_t frf_r76_usb_mass_count;
static uint32_t posted_k,posted_m,posted_p,posted_disk,post_calls;
void frf_post_usb_devices(unsigned k,unsigned m,unsigned p,unsigned disk)
{
 posted_k=k;posted_m=m;posted_p=p;posted_disk=disk;++post_calls;
}
#define ASSERT(c) do{if(!(c)){fprintf(stderr,"FAIL line %u %s\n",__LINE__,#c);exit(3);}}while(0)
static void frf_r130_post_hid_discovered(void)
{
#if defined(FRF_BOOT_POST)
    extern void frf_post_usb_devices(unsigned,unsigned,unsigned,unsigned);
    unsigned kb=0u,mouse=0u,pad=0u;
    for(uint32_t i=0u;i<g_native_count;i++) {
        const struct NativeHID *h=&g_native_hid[i];
        if(!h->valid)continue;
        if(h->protocol==USB_HID_PROTOCOL_KEYBOARD)++kb;
        else if(h->protocol==USB_HID_PROTOCOL_MOUSE)++mouse;
        else if(h->protocol==0u) {
            ++pad;
            frf_r130_pad_vidpid=((h->vid&0xffffu)<<16)|(h->pid&0xffffu);
        }
    }
    __atomic_store_n(&frf_r130_pad_discovered,pad,__ATOMIC_RELEASE);
    extern volatile uint32_t frf_r76_usb_mass_count;
    frf_post_usb_devices(kb,mouse,pad,frf_r76_usb_mass_count);
#endif
}
int main(void) {
 frf_r130_post_hid_discovered();
 ASSERT(posted_k==0 && posted_m==0 && posted_p==0 && !posted_disk);
 ASSERT(post_calls==1);
 g_native_hid[0].valid=1u;g_native_hid[0].protocol=0u;g_native_count=1u;
 g_native_hid[0].vid=0x2563u;g_native_hid[0].pid=0x0576u;
 frf_r130_post_hid_discovered();
 ASSERT(posted_p==1 && posted_k==0 && posted_m==0);
 ASSERT(frf_r130_pad_discovered==1u && frf_r130_pad_vidpid==0x25630576u);
 g_native_hid[1].valid=1u;g_native_hid[1].protocol=1u;g_native_count=2u;
 frf_r130_post_hid_discovered();
 ASSERT(posted_p==1 && posted_k==1 && posted_m==0);
 g_native_hid[2].valid=1u;g_native_hid[2].protocol=2u;g_native_count=3u;
 g_frf_msc_slot=4u;
 frf_r76_usb_mass_count=1u;
 frf_r130_post_hid_discovered();
 ASSERT(posted_p==1 && posted_k==1 && posted_m==1 && posted_disk==1);
 g_native_hid[0].valid=0u;
 frf_r130_post_hid_discovered();
 ASSERT(posted_p==0 && posted_k==1 && posted_m==1 && posted_disk==1);
 ASSERT(frf_r130_pad_discovered==0u);
 ASSERT(post_calls==5);
 puts("PASS legacy R130: actual POST helper publishes gamepad as soon as found, then keyboard/mouse, filters invalid, MSC separate");
 return 0;
}
