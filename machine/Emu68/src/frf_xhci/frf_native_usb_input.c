#include "frf_native_usb_input.h"
#include <stdint.h>
extern void frf_vcia_keyboard_raw(uint8_t raw);
extern void aga_virtual_mouse_update(int dx, int dy, unsigned buttons);
extern void aga_virtual_joy_set(int port, uint16_t joydat, int fire);
extern void aga_virtual_pad_buttons(int port,uint32_t buttons);
/* Trace actual decoded USB HID button bits across ARM->Amiga boundary.
 * These values are diagnostics only, no extra input mapping. */
volatile uint32_t frf_r127_legacy_decoded_buttons;
volatile uint32_t frf_r127_legacy_native_buttons;
volatile uint32_t frf_r127_legacy_input_updates;

volatile uint32_t frf_native_usb_key_reports;
volatile uint32_t frf_native_usb_key_events;
volatile uint32_t frf_native_usb_mouse_reports;
volatile uint32_t frf_native_usb_pad_reports;
/* Pi4 ARM host-only hard-reset shortcut: LShift + either Ctrl + F11.
 * CPU1's keyboard parser sets the flag, CPU1 USB worker executes
 * aga_reboot(0) outside the HID report callback. */
volatile uint32_t frf_native_usb_arm_reset_pending;
volatile uint32_t frf_native_usb_arm_reset_count;
static uint8_t arm_reset_chord_latched;
static uint8_t last_mod,last_keys[6];
static uint8_t hid_to_amiga(uint8_t h)
{
    if (h >= 4u && h <= 0x1du) {
        static const uint8_t alpha[26] = {
            0x20,0x35,0x33,0x22,0x12,0x23,0x24,0x25,0x17,0x26,0x27,0x28,0x37,
            0x36,0x18,0x19,0x10,0x13,0x21,0x14,0x16,0x34,0x11,0x32,0x15,0x31 };
        return alpha[h-4u];
    }
    if (h >= 0x1eu && h <= 0x26u) return (uint8_t)(h-0x1du);
    switch (h) {
    case 0x27: return 0x0a; case 0x28: return 0x44; case 0x29: return 0x45;
    case 0x2a: return 0x41; case 0x2b: return 0x42; case 0x2c: return 0x40;
    case 0x2d: return 0x0b; case 0x2e: return 0x0c; case 0x2f: return 0x1a;
    case 0x30: return 0x1b; case 0x31: return 0x0d; case 0x32: return 0x30;
    case 0x33: return 0x29; case 0x34: return 0x2a; case 0x35: return 0x00;
    case 0x36: return 0x38; case 0x37: return 0x39; case 0x38: return 0x3a;
    case 0x39: return 0x62;
    case 0x3a: return 0x50; case 0x3b: return 0x51; case 0x3c: return 0x52;
    case 0x3d: return 0x53; case 0x3e: return 0x54; case 0x3f: return 0x55;
    case 0x40: return 0x56; case 0x41: return 0x57; case 0x42: return 0x58;
    case 0x43: return 0x59;
    case 0x49: return 0x5f; case 0x4c: return 0x46; case 0x4f: return 0x4e;
    case 0x50: return 0x4f; case 0x51: return 0x4d; case 0x52: return 0x4c;
    case 0x54: return 0x5c; case 0x55: return 0x5d; case 0x56: return 0x4a;
    case 0x57: return 0x5e; case 0x58: return 0x43;
    case 0x59: return 0x1d; case 0x5a: return 0x1e; case 0x5b: return 0x1f;
    case 0x5c: return 0x2d; case 0x5d: return 0x2e; case 0x5e: return 0x2f;
    case 0x5f: return 0x3d; case 0x60: return 0x3e; case 0x61: return 0x3f;
    case 0x62: return 0x0f; case 0x63: return 0x3c;
    default: return 0xffu;
    }
}
static int key_in(const uint8_t keys[6],uint8_t key)
{
    for(unsigned i=0;i<6u;i++) if(keys[i]==key) return 1;
    return 0;
}
static void emit_key(uint8_t raw,int release)
{
    if(raw==0xffu) return;
    frf_vcia_keyboard_raw((uint8_t)(raw|(release?0x80u:0u)));
    __atomic_add_fetch(&frf_native_usb_key_events,1u,__ATOMIC_RELAXED);
}
void frf_native_usb_keyboard_report(const uint8_t report[8])
{
    if (!report) return;
    const uint8_t mod=report[0],*keys=&report[2];
    /* HID rollover/error usages must not release all held keys. */
    for(unsigned i=0;i<6u;i++) if(keys[i]>=1u && keys[i]<=3u) return;
    /* USB HID boot keyboard: F11 usage 0x44, modifier bit1=LShift,
       bits0/4=left/right Ctrl. The left shift requirement is intentional.
       Detect held chord on any report, even if a modifier was pressed
       after F11. Trigger only once until released. The shortcut belongs
       to the ARM host, never an Amiga raw-key or soft reset. */
    const int arm_chord=(mod&0x02u) && (mod&0x11u) &&
                        key_in(keys,0x44u);
    if(arm_chord && !arm_reset_chord_latched) {
        arm_reset_chord_latched=1u;
        __atomic_add_fetch(&frf_native_usb_arm_reset_count,1u,__ATOMIC_RELAXED);
        __atomic_store_n(&frf_native_usb_arm_reset_pending,1u,__ATOMIC_RELEASE);
    } else if(!arm_chord) {
        arm_reset_chord_latched=0u;
    }
    const uint8_t mods[8]={0x63,0x60,0x64,0x66,0x63,0x61,0x65,0x67};
    uint8_t changed=mod^last_mod;
    for(unsigned b=0;b<8u;b++) if(changed & (1u<<b))
        emit_key(mods[b],!(mod & (1u<<b)));
    for(unsigned i=0;i<6u;i++) {
        uint8_t k=last_keys[i];
        if(k<4u || key_in(keys,k) || k==0x53u || k==0x47u) continue;
        emit_key(hid_to_amiga(k),1);
    }
    for(unsigned i=0;i<6u;i++) {
        uint8_t k=keys[i];
        if(k<4u || key_in(last_keys,k) || k==0x53u || k==0x47u) continue;
        emit_key(hid_to_amiga(k),0);
    }
    last_mod=mod;
    for(unsigned i=0;i<6u;i++) last_keys[i]=keys[i];
    __atomic_add_fetch(&frf_native_usb_key_reports,1u,__ATOMIC_RELAXED);
}
/* HID report-mode mouse (USB interface protocol 2). Boot protocol is
   deliberately kept for devices without a usable wheel-bearing report.
   All bit offsets refer to the full report, including a report ID when
   present. Handles nested Pointer collections, constant padding,
   button usages 1..3, signed X/Y and signed wheel up to 16 bits. */
struct R144MouseField {
    unsigned bit,size,valid;
    int32_t logical_min;
};
struct R144MouseLayout {
    unsigned valid,id,has_id;
    struct R144MouseField button[3],x,y,wheel;
};
static struct R144MouseLayout r144_mouse[8];
static uint32_t r144_bits(const uint8_t *p,unsigned len,unsigned bit,unsigned n)
{
    if(!p || !n || n>16u || (uint64_t)bit+n>(uint64_t)len*8u)return 0u;
    uint32_t v=0u;
    for(unsigned i=0u;i<n;i++)
        if(p[(bit+i)>>3u]&(1u<<((bit+i)&7u)))v|=1u<<i;
    return v;
}
static int32_t r144_sign(uint32_t x,unsigned bits)
{
    if(!bits || bits>32u)return 0;
    if(bits==32u)return (int32_t)x;
    const uint32_t sign=1u<<(bits-1u),mask=(1u<<bits)-1u;
    x&=mask;
    return (int32_t)((x^sign)-sign);
}
static int32_t r144_value(const struct R144MouseField *f,
                          const uint8_t *p,unsigned len)
{
    uint32_t v=r144_bits(p,len,f->bit,f->size);
    return f->logical_min<0?r144_sign(v,f->size):(int32_t)v;
}
int frf_native_usb_mouse_set_descriptor_for(unsigned index,const uint8_t *desc,unsigned len)
{
    if(index>=8u)return 0;
    r144_mouse[index]=(struct R144MouseLayout){0};
    if(!desc || len<12u || len>512u)return 0;
    struct R144MouseLayout m={0};
    uint16_t offsets[256]={0};
    uint32_t page=0u,rsize=0u,rcount=0u,rid=0u;
    int32_t logical_min=0,logical_max=0;
    unsigned usages[32]={0},nusage=0u,umin=0u,umax=0u;
    unsigned depth=0u,active_depth=0u;
    /* HID Global Push/Pop is permitted; fail closed on stack overflow. */
    struct {uint32_t page,size,count,id;int32_t min,max;} globals[4];
    unsigned top=0u;
    for(unsigned at=0;at<len;){
        unsigned prefix=desc[at++];
        if(prefix==0xfeu){
            if(at+2u>len)return 0;
            unsigned skip=desc[at];at+=2u;
            if(skip>len-at)return 0;
            at+=skip;continue;
        }
        unsigned size=prefix&3u;if(size==3u)size=4u;
        if(size>len-at)return 0;
        uint32_t val=0u;
        for(unsigned k=0;k<size;k++)val|=(uint32_t)desc[at+k]<<(8u*k);
        at+=size;
        unsigned type=(prefix>>2u)&3u,tag=prefix>>4u;
        if(type==1u){
            switch(tag){
            case 0:page=val;break;
            case 1:logical_min=r144_sign(val,size*8u);break;
            case 2:logical_max=logical_min<0?r144_sign(val,size*8u):(int32_t)val;break;
            case 7:rsize=val;break;
            case 8:rid=val&255u;break;
            case 9:rcount=val;break;
            case 10:
                if(top>=4u)return 0;
                globals[top].page=page;globals[top].size=rsize;
                globals[top].count=rcount;globals[top].id=rid;
                globals[top].min=logical_min;globals[top].max=logical_max;
                ++top;break;
            case 11:
                if(!top)return 0;
                --top;page=globals[top].page;rsize=globals[top].size;
                rcount=globals[top].count;rid=globals[top].id;
                logical_min=globals[top].min;logical_max=globals[top].max;
                break;
            default:break;
            }
        } else if(type==2u){
            if(tag==0u){if(nusage<32u)usages[nusage++]=val;}
            if(tag==1u)umin=val;
            if(tag==2u)umax=val;
        } else if(type==0u){
            if(tag==10u){
                unsigned usage=nusage?usages[0]:umin;
                ++depth;
                if(!active_depth && page==1u && usage==2u && val==1u)
                    active_depth=depth; /* Mouse application */
            }else if(tag==12u){
                if(active_depth==depth)active_depth=0u;
                if(depth)--depth;
            }else if(tag==8u){
                if(!rsize || rsize>32u || rcount>128u ||
                   (uint64_t)rsize*rcount>512u)return 0;
                unsigned base=offsets[rid];
                if(rid && base==0u)base=8u;
                if((uint64_t)base+(uint64_t)rsize*rcount>512u)return 0;
                offsets[rid]=(uint16_t)(base+rsize*rcount);
                if(active_depth && !(val&1u) && (val&2u)){
                    for(unsigned k=0;k<rcount;k++){
                        unsigned usage=k<nusage?usages[k]:
                                 (umin && umin+k<=umax?umin+k:0u);
                        struct R144MouseField *f=0;
                        if(page==9u && usage>=1u && usage<=3u)
                            f=&m.button[usage-1u];
                        if(page==1u && usage==0x30u)f=&m.x;
                        if(page==1u && usage==0x31u)f=&m.y;
                        if(page==1u && usage==0x38u)f=&m.wheel;
                        if(f && !f->valid && rsize<=16u &&
                           (!m.valid || m.id==rid)){
                            f->bit=base+k*rsize;
                            f->size=rsize;
                            f->logical_min=logical_min;
                            f->valid=1u;
                            m.id=rid;
                            m.has_id=rid!=0u;
                            m.valid=1u;
                        }
                    }
                }
            }
            nusage=umin=umax=0u; /* Local HID usages expire at main item */
        }
    }
    if(!m.valid || !m.x.valid || !m.y.valid || !m.wheel.valid ||
       !m.button[0].valid || m.x.logical_min>=0 ||
       m.y.logical_min>=0 || m.wheel.logical_min>=0)
        return 0;
    r144_mouse[index]=m;
    return 1;
}
int frf_native_usb_mouse_decode_for(unsigned index,const uint8_t *bytes,unsigned len,
                                    uint8_t normalized[4])
{
    if(index>=8u || !bytes || !len || !normalized ||
       !r144_mouse[index].valid)return 0;
    const struct R144MouseLayout *m=&r144_mouse[index];
    if(m->has_id && bytes[0]!=m->id)return 0;
    const struct R144MouseField *fields[6]={
        &m->button[0],&m->button[1],&m->button[2],&m->x,&m->y,&m->wheel
    };
    for(unsigned k=0;k<6u;k++){
        const struct R144MouseField *f=fields[k];
        if(f->valid && ((uint64_t)f->bit+f->size>(uint64_t)len*8u))
            return 0;
    }
    unsigned buttons=0u;
    for(unsigned k=0;k<3u;k++)
        if(m->button[k].valid && r144_value(&m->button[k],bytes,len))
            buttons|=1u<<k;
    int32_t x=r144_value(&m->x,bytes,len);
    int32_t y=r144_value(&m->y,bytes,len);
    int32_t wheel=r144_value(&m->wheel,bytes,len);
    if(x>127)x=127;
    if(x< -127)x=-127;
    if(y>127)y=127;
    if(y< -127)y=-127;
    if(wheel>127)wheel=127;
    if(wheel< -127)wheel=-127;
    normalized[0]=(uint8_t)buttons;
    normalized[1]=(uint8_t)(int8_t)x;
    normalized[2]=(uint8_t)(int8_t)y;
    normalized[3]=(uint8_t)(int8_t)wheel;
    return 1;
}
/* R144 NewMouse compatible CIA raw-key bridge. The NewMouse wheel
   assignments are 0x7A (up) and 0x7B (down); each detent is a
   make+break pair. These enter the existing CPU1->CPU3 virtual CIA
   keyboard queue, not the mouse button bits or a Poseidon handler. */
volatile uint32_t frf_r144_wheel_up,frf_r144_wheel_down;
volatile uint32_t frf_r144_wheel_last,frf_r144_wheel_reports;
static void frf_r144_emit_wheel(int wheel)
{
    if(!wheel)return;
    __atomic_store_n(&frf_r144_wheel_last,(uint32_t)(int32_t)wheel,__ATOMIC_RELAXED);
    __atomic_add_fetch(&frf_r144_wheel_reports,1u,__ATOMIC_RELAXED);
    /* Never monopolise the 64-byte CIA keyboard queue if a high
       resolution HID device reports many detents in one packet. */
    if(wheel>8)wheel=8;
    if(wheel< -8)wheel=-8;
    /* R146: hardware test on Pi4 showed the physical wheel direction
       reversed relative to the NewMouse/FreeWheel up/down semantics.
       Keep NewMouse keycodes intact, reverse ONLY HID wheel sign. */
    const uint8_t key=wheel>0?0x7bu:0x7au;
    const unsigned steps=wheel>0?(unsigned)wheel:(unsigned)-wheel;
    for(unsigned n=0;n<steps;n++){
        frf_vcia_keyboard_raw(key);
        frf_vcia_keyboard_raw((uint8_t)(key|0x80u));
    }
    if(wheel<0)
        __atomic_add_fetch(&frf_r144_wheel_up,steps,__ATOMIC_RELAXED);
    else
        __atomic_add_fetch(&frf_r144_wheel_down,steps,__ATOMIC_RELAXED);
}
void frf_native_usb_mouse_report(const uint8_t *r,unsigned length)
{
    if(!r || length<3u)return;
    /* Normalized mouse report: buttons, signed X, signed Y,
       optional signed vertical wheel in byte 3. Boot 3B still works. */
    const unsigned buttons=(unsigned)(r[0]&7u);
    aga_virtual_mouse_update((int8_t)r[1],(int8_t)r[2],buttons);
    __atomic_add_fetch(&frf_native_usb_mouse_reports,1u,__ATOMIC_RELAXED);
    if(length>=4u)frf_r144_emit_wheel((int8_t)r[3]);
}
void frf_native_usb_pad_release(void)
{
    /* Wireless disconnect: release held stick/fire without incrementing
       report counters or modifying unrelated keyboard/mouse input. */
    aga_virtual_joy_set(1,0u,0);
    aga_virtual_pad_buttons(1,0u);
    __atomic_store_n(&frf_r127_legacy_native_buttons,0u,__ATOMIC_RELEASE);
    __atomic_store_n(&frf_r127_legacy_decoded_buttons,0u,__ATOMIC_RELEASE);
}
void frf_native_usb_pad_update_axes(int x,int y,int x2,int y2,
                                    unsigned buttons,int hat)
{
    /* Match the working Pi3 v0.10.56 axis 0/1 plus D-pad axis 2/3:
       20% deflection threshold, OR with HID hat. */
    int l=x<-32 || x2<-32,rr=x>32 || x2>32;
    int u=y<-32 || y2<-32,d=y>32 || y2>32;
    if(hat>=0 && hat<8) {
        u|=(hat==0||hat==1||hat==7);
        rr|=(hat==1||hat==2||hat==3);
        d|=(hat==3||hat==4||hat==5);
        l|=(hat==5||hat==6||hat==7);
    }
    /* Exactly the working Pi3 JOY1DAT quadrature encoding. */
    uint16_t joy=0;
    if(l) joy|=0x0200u;
    if(rr)joy|=0x0002u;
    if(!!l != !!u) joy|=0x0100u;
    if(!!rr != !!d)joy|=0x0001u;
    /* Native Amiga joystick port 2 exposes exactly THREE buttons:
       B1 CIA-A PRA7 active low, B2 POTGOR bit14 active low,
       B3 POTGOR bit12 active low. Fourth and higher HID button bits
       are not silently collapsed to the first button.
       Preserve *unmodified* HID bits in diagnostics so we can tell
       decoder mismatch from actual POTGOR register behavior. */
    const unsigned amiga_buttons=buttons&7u;
    __atomic_store_n(&frf_r127_legacy_decoded_buttons,buttons,__ATOMIC_RELEASE);
    __atomic_store_n(&frf_r127_legacy_native_buttons,amiga_buttons,__ATOMIC_RELEASE);
    __atomic_add_fetch(&frf_r127_legacy_input_updates,1u,__ATOMIC_RELAXED);
    aga_virtual_joy_set(1,joy,(amiga_buttons&1u)!=0u);
    aga_virtual_pad_buttons(1,amiga_buttons);
    __atomic_add_fetch(&frf_native_usb_pad_reports,1u,__ATOMIC_RELAXED);
}
void frf_native_usb_pad_update(int x,int y,unsigned buttons,int hat)
{
    frf_native_usb_pad_update_axes(x,y,0,0,buttons,hat);
}
