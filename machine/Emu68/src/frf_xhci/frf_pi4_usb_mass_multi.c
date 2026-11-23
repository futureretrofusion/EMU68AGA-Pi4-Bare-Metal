/* SPDX-License-Identifier: GPL-2.0-or-later
 * R133 experimental Pi4 multi-BOT, multi-LUN, virtual FRUM v1 + v2.
 * ARM CPU1 exclusively owns the VL805 xHCI. No hotplug and no auto format.
 * Multi-device access is read-only except USB unit0, which is writable
 * ONLY after safely scanning its first 16 blocks for an Amiga RDB header.
 */
#include <stdint.h>
#include <support.h>
#include "frf_xhci_host.h"
#include "frf_usb_mass.h"

extern volatile struct frf_um_shared *frf_um_host;
extern volatile uint32_t frf_r76_usb_mass_count;
#define UMAX FRF_UM_MAX_UNITS
struct R133Unit {
    uint32_t host,lun,cap,attempts,dead,read_errors,bot_error;
    uint32_t change,flags,scan_done,reported_failure;
    uint64_t next_retry;
};
static struct R133Unit units[UMAX];
static uint32_t used_units,registered_hosts,ready_units,bot_tag=0x52313300u;
static uint64_t next_global_retry;
static uint32_t retry_cursor;
static uint8_t cbw[64] __attribute__((aligned(64)));
static uint8_t csw[64] __attribute__((aligned(64)));
static uint8_t payload[FRF_UM_MAX_SECTORS*FRF_UM_SECTOR_SIZE] __attribute__((aligned(64)));

static void zero(uint8_t *p,uint32_t n){while(n--)*p++=0u;}
static uint32_t be32(const uint8_t *p){
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
static uint32_t le32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void putle(uint8_t *p,uint32_t n){
    p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);p[2]=(uint8_t)(n>>16);p[3]=(uint8_t)(n>>24);
}
static struct frf_um_extension *extension(volatile struct frf_um_shared *s){
    return (struct frf_um_extension *)(uintptr_t)((uintptr_t)s+FRF_UM_EXT_OFFSET);
}
static void publish_ready(void){
    ready_units=0u;
    for(uint32_t i=0;i<used_units;i++)if(units[i].cap)++ready_units;
    __atomic_store_n(&frf_r76_usb_mass_count,ready_units,__ATOMIC_RELEASE);
}
static void kill_host(uint32_t host){
    /* A BOT transport timeout invalidates the entire physical endpoint,
     * including all its LUNs. Never reuse or retry ambiguous writes. */
    for(uint32_t i=0;i<used_units;i++)if(units[i].host==host){
        if(units[i].cap)++units[i].change;
        units[i].cap=0u;units[i].dead=1u;units[i].flags=0u;
    }
    publish_ready();
}
static int bot(uint32_t unit,const uint8_t *cdb,uint32_t cdb_len,
               uint8_t *data,uint32_t bytes,int direction_in){
    if(unit>=used_units || cdb_len==0u || cdb_len>16u ||
       bytes>sizeof(payload) || (bytes&&!data))return 0;
    struct R133Unit *u=&units[unit];
    if(u->dead || u->host>=frf_xhci_host_msc_count())return 0;
    uint32_t tag=++bot_tag,got=0u;
    zero(cbw,sizeof(cbw));zero(csw,sizeof(csw));
    cbw[0]='U';cbw[1]='S';cbw[2]='B';cbw[3]='C';
    putle(cbw+4,tag);putle(cbw+8,bytes);
    cbw[12]=(bytes&&direction_in)?0x80u:0u;
    cbw[13]=(uint8_t)u->lun;
    cbw[14]=(uint8_t)cdb_len;
    for(uint32_t i=0;i<cdb_len;i++)cbw[15u+i]=cdb[i];
    u->bot_error=0u;
    if(!frf_xhci_host_msc_bulk_out(u->host,cbw,31u,&got)||got!=31u){
        u->bot_error=1u;kill_host(u->host);return 0;
    }
    got=0u;
    if(bytes){
        int ok=direction_in?
            frf_xhci_host_msc_bulk_in(u->host,data,bytes,&got):
            frf_xhci_host_msc_bulk_out(u->host,data,bytes,&got);
        if(!ok||got!=bytes){
            u->bot_error=2u;kill_host(u->host);return 0;
        }
    }
    got=0u;
    if(!frf_xhci_host_msc_bulk_in(u->host,csw,13u,&got)||got!=13u){
        u->bot_error=3u;kill_host(u->host);return 0;
    }
    if(csw[0]!='U'||csw[1]!='S'||csw[2]!='B'||csw[3]!='S'||
       le32(csw+4)!=tag||le32(csw+8)!=0u){
        u->bot_error=4u;kill_host(u->host);return 0;
    }
    if(csw[12]!=0u){u->bot_error=6u;return 0;} /* CHECK CONDITION */
    return 1;
}
static void disk_cdb(uint8_t *cdb,uint8_t opcode,uint32_t lba,uint32_t n){
    zero(cdb,10u);
    cdb[0]=opcode;cdb[2]=(uint8_t)(lba>>24);
    cdb[3]=(uint8_t)(lba>>16);cdb[4]=(uint8_t)(lba>>8);cdb[5]=(uint8_t)lba;
    cdb[7]=(uint8_t)(n>>8);cdb[8]=(uint8_t)n;
}
static void detect_rdb(uint32_t i){
    struct R133Unit *u=&units[i];
    if(!u->cap||u->dead||u->scan_done)return;
    uint32_t n=u->cap<16u?u->cap:16u;
    uint8_t cdb[10];disk_cdb(cdb,0x28u,0u,n);
    /* One 8 KiB READ10 covers standard Amiga RDB search blocks 0..15. */
    if(!bot(i,cdb,10u,payload,n*512u,1))return;
    u->scan_done=1u;
    for(uint32_t b=0;b<n;b++){
        uint8_t *p=payload+b*512u;
        if(p[0]=='R'&&p[1]=='D'&&p[2]=='S'&&p[3]=='K'){
            u->flags|=FRF_UM_UNIT_RDB;
            kprintf("[R133-MSC] Amiga RDB found unit%u host%u LUN%u block%u (RO)\n",
                    i,u->host,u->lun,b);
            break;
        }
    }
#if defined(FRF_PI4_USB_MASS_RW)
    /* v0.5-compatible USB0 write path, never make an RDB target writable.
     * Units 1..7 are unconditionally RO until individual media validation. */
    if(i==0u && !(u->flags&FRF_UM_UNIT_RDB))
        u->flags|=FRF_UM_UNIT_WRITE;
#endif
}
static void try_capacity(uint32_t i){
    if(i>=used_units)return;
    struct R133Unit *u=&units[i];
    if(u->cap||u->dead||u->attempts>=12u)return;
    ++u->attempts;
    /* R140: card-reader LUN0 may contain no media. Issue a zero-data
     * TEST UNIT READY first. A failed READ CAPACITY has an 8-byte data
     * stage that some readers STALL or short-complete, which the old
     * BOT path treats as a fatal transport error for ALL reader LUNs.
     * TUR CHECK CONDITION is safe: its CSW is consumed, then continue
     * probing populated LUN1..N. Never write any USB sectors here. */
    uint8_t tur[6]={0x00u,0u,0u,0u,0u,0u};
    if(!bot(i,tur,6u,0,0u,1)){
        if(!u->reported_failure){
            kprintf("[R140-MSC] unit%u slot%u LUN%u not ready TUR/BOT=%u dead=%u\n",
                i,frf_xhci_host_msc_slot_at(u->host),u->lun,
                u->bot_error,u->dead);
            u->reported_failure=1u;
        }
        return;
    }
    uint8_t cdb[10]={0x25u,0u,0u,0u,0u,0u,0u,0u,0u,0u};
    if(!bot(i,cdb,10u,payload,8u,1)){
        if(!u->reported_failure){
            kprintf("[R133-MSC] unit%u slot%u LUN%u capacity pending BOT=%u\n",
                i,frf_xhci_host_msc_slot_at(u->host),u->lun,u->bot_error);
            u->reported_failure=1u;
        }
        return;
    }
    uint32_t last=be32(payload),block=be32(payload+4u);
    if(block!=512u||last==0xffffffffu||last==0u){
        kprintf("[R133-MSC] unit%u unsupported capacity blockSize=%u last=%u\n",
                i,block,last);
        u->attempts=12u;
        return;
    }
    u->cap=last+1u; ++u->change;
    kprintf("[R133-MSC] READY unit%u slot%u LUN%u 512-sector count=%u\n",
        i,frf_xhci_host_msc_slot_at(u->host),u->lun,u->cap);
    detect_rdb(i);
    publish_ready();
}
static void discover_hosts(void){
    uint32_t host_count=frf_xhci_host_msc_count();
    while(registered_hosts<host_count && used_units<UMAX){
        uint32_t h=registered_hosts++;
        uint32_t max_lun=frf_xhci_host_msc_max_lun(h);
        for(uint32_t lun=0;lun<=max_lun && used_units<UMAX;lun++){
            struct R133Unit *u=&units[used_units++];
            u->host=h;u->lun=lun;u->change=1u;
            kprintf("[R133-MSC] mapped unit%u slot%u LUN%u\n",
                    used_units-1u,frf_xhci_host_msc_slot_at(h),lun);
        }
        /* One synchronous bounded probe for LUN0 before continuing the
         * scan; defer secondary LUNs to the throttled CPU1 service loop. */
        uint32_t first=used_units-1u;
        for(uint32_t n=0;n<used_units;n++)if(units[n].host==h){first=n;break;}
        try_capacity(first);
    }
}
void frf_pi4_um_reset(void){
    /* Re-scan must never reuse previous-generation drive mappings. */
    zero((uint8_t*)units,sizeof(units));
    used_units=registered_hosts=ready_units=0u;
    next_global_retry=0u;retry_cursor=0u;
    __atomic_store_n(&frf_r76_usb_mass_count,0u,__ATOMIC_RELEASE);
}
void frf_pi4_um_probe_inline(void){ discover_hosts(); }
void frf_pi4_um_init(void){
    /* Preserve host enumeration, but probe every advertised USB LUN
     * once before telling the Amiga that USB enumeration is ready.
     * R133 did not probe card-reader LUN1 until later CPU1 polling,
     * potentially after CaffeineOS started PFS3 startup. Read-only
     * CAPACITY/RDSK probing; deferred retries remain unchanged. */
    discover_hosts();
    for(uint32_t i=0u;i<used_units;i++)
        if(!units[i].dead && !units[i].cap && !units[i].attempts)
            try_capacity(i);
    kprintf("[R138-MSC] init physicalHosts=%u logicalUnits=%u ready=%u\n",
            frf_xhci_host_msc_count(),used_units,ready_units);
}
void frf_pi4_um_retry(uint64_t now,uint64_t hz){
    if(!hz||hz>2000000000ull)return;
    discover_hosts();
    /* At most ONE BOT command group per 2 seconds, across ALL units.
     * Yield CPU1 promptly for HID, and never block on absent media. */
    if(!next_global_retry){next_global_retry=now+hz/4u;return;}
    if((int64_t)(now-next_global_retry)<0)return;
    /* Round robin prevents 12 retries of one empty multi-slot reader
     * from hiding an RDB card in a subsequent LUN for 24 seconds. */
    for(uint32_t step=0u;step<used_units;step++){
        uint32_t i=(retry_cursor+step)%used_units;
        struct R133Unit *u=&units[i];
        if(u->dead)continue;
        if(u->cap&&!u->scan_done){
            detect_rdb(i);retry_cursor=(i+1u)%used_units;break;
        }
        if(!u->cap&&u->attempts<12u){
            try_capacity(i);retry_cursor=(i+1u)%used_units;break;
        }
    }
    uint32_t untried=0u;
    for(uint32_t i=0;i<used_units;i++)
        if(!units[i].dead && !units[i].cap && !units[i].attempts)++untried;
    next_global_retry=now+(untried?hz/4u:hz*2u);
#if defined(FRF_BOOT_POST)
    extern void frf_boot_post_devices(void);
    frf_boot_post_devices();
#endif
}
uint32_t frf_pi4_um_capacity(void){return used_units?units[0].cap:0u;}
uint32_t frf_pi4_um_attempts(void){return used_units?units[0].attempts:0u;}
uint32_t frf_pi4_um_last_bot_error(void){return used_units?units[0].bot_error:0u;}
uint32_t frf_pi4_um_transport_dead(void){return used_units?units[0].dead:0u;}
uint32_t frf_pi4_um_transport_failures(void){
    uint32_t dead=0u;
    for(uint32_t i=0;i<used_units;i++)if(units[i].dead)++dead;
    return dead;
}
void frf_pi4_um_service(void){
    volatile struct frf_um_shared *s=__atomic_load_n(&frf_um_host,__ATOMIC_ACQUIRE);
    if(!s||s->magic!=FRF_UM_MAGIC||s->version!=FRF_UM_VERSION||
       s->bytes!=sizeof(struct frf_um_shared))return;
    struct frf_um_extension *e=extension(s);
    uint32_t cap=used_units?units[0].cap:0u;
    s->reserved[0]=frf_xhci_host_msc_slot_at(0u);
    s->reserved[1]=(used_units&&(units[0].flags&FRF_UM_UNIT_WRITE))?
        0x50493457u:0x50493452u; /* PI4W or PI4R for legacy v0.5 */
    if(s->capacity!=cap||s->present!=(cap?1u:0u)){
        s->capacity=cap;s->present=cap?1u:0u;
        ++s->media_change;
    }
    /* Publish the v2 magic once. Never clear it during normal service,
     * otherwise a concurrent AmigaOS GetGeometry/OpenDevice may falsely
     * report a disk change while CPU1 refreshes 32-bit unit fields. */
    e->version=FRF_UM_EXT_VERSION;
    e->count=used_units;
    for(uint32_t i=0;i<UMAX;i++){
        struct frf_um_ext_unit *out=&e->units[i];
        const struct R133Unit *u=&units[i];
        out->present=(i<used_units&&u->cap)?1u:0u;
        out->capacity=i<used_units?u->cap:0u;
        out->sector_size=512u;
        out->media_change=i<used_units?u->change:0u;
        out->flags=i<used_units?u->flags:0u;
        out->slot=i<used_units?frf_xhci_host_msc_slot_at(u->host):0u;
        out->lun=i<used_units?u->lun:0u;
        out->last_error=i<used_units?u->bot_error:0u;
    }
    __atomic_thread_fence(__ATOMIC_RELEASE);
    e->magic=FRF_UM_EXT_MAGIC;
    if(__atomic_load_n(&s->state,__ATOMIC_ACQUIRE)!=FRF_UM_REQ)return;
    uint32_t seq=s->sequence,op=s->command,lba=s->lba,n=s->sectors;
    /* An old v0.5 guest never selects anything but unit0. After every
     * request, clear the selector before the next guest can claim FRUM. */
    uint32_t index=e->request_unit;
    __atomic_store_n(&s->state,FRF_UM_BUSY,__ATOMIC_RELEASE);
    int32_t rc=-1;
    if(index<used_units && n>0u && n<=FRF_UM_MAX_SECTORS){
        struct R133Unit *u=&units[index];
        if(u->cap&&!u->dead&&lba<u->cap&&n<=u->cap-lba){
            uint8_t cdb[10];uint32_t bytes=n*512u;
            if(op==FRF_UM_READ){
                disk_cdb(cdb,0x28u,lba,n);
                if(bot(index,cdb,10u,payload,bytes,1)){
                    for(uint32_t i=0;i<bytes;i++)s->data[i]=payload[i];
                    rc=(int32_t)bytes;
                }else if(++u->read_errors>=3u){kill_host(u->host);}
            }else if(op==FRF_UM_WRITE){
                if(u->flags&FRF_UM_UNIT_WRITE){
                    /* Guest v0.6 stages in the FRUM mailbox before REQ. */
                    __atomic_thread_fence(__ATOMIC_ACQUIRE);
                    for(uint32_t i=0;i<bytes;i++)payload[i]=s->data[i];
                    disk_cdb(cdb,0x2au,lba,n);
                    if(bot(index,cdb,10u,payload,bytes,0))
                        rc=(int32_t)bytes;
                    else kill_host(u->host); /* no retries of ambiguous writes */
                }else rc=-30;
            }
        }
    }
    s->result=(uint32_t)rc;s->completed=seq;
    e->request_unit=0u;
    __atomic_thread_fence(__ATOMIC_RELEASE);
    __atomic_store_n(&s->state,
        ((op==FRF_UM_READ||op==FRF_UM_WRITE)&&rc==(int32_t)(n*512u))?
            FRF_UM_DONE:FRF_UM_ERROR,__ATOMIC_RELEASE);
}
