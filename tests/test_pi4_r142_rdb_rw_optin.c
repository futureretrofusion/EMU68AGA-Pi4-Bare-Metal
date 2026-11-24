/* R141 host-executed test: per-LUN CHECK CONDITION must not kill reader.
 * Simulates physical slots 5 and 9, with host0: LUN0 FAT32, LUN1 RDB;
 * host1: LUN0 FAT32. Tests separate-unit routing and write confinement. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "frf_xhci_host.h"
#include "frf_usb_mass.h"

static uint8_t board[FRF_UM_BOARD_BYTES] __attribute__((aligned(64)));
volatile struct frf_um_shared *frf_um_host;
volatile uint32_t frf_r76_usb_mass_count;
static struct {
    uint8_t cbw[31];
    uint32_t opcode,lun,lba,blocks,tag,bytes;
    uint32_t writes;
} h[2];
static uint32_t writer_unit0,simulate_empty_lun0,unexpected_empty_read_capacity;
static uint32_t simulate_bad_rdb_sector;
static uint32_t turs[2],readcapacities[2];
static void must(int ok,const char *name){
    if(!ok){fprintf(stderr,"FAIL R133 %s\n",name);exit(2);}
}
static uint32_t getle(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void putle(uint8_t *p,uint32_t n){
    p[0]=n;p[1]=n>>8;p[2]=n>>16;p[3]=n>>24;
}
static void putbe(uint8_t *p,uint32_t n){
    p[0]=n>>24;p[1]=n>>16;p[2]=n>>8;p[3]=n;
}
uint32_t frf_xhci_host_msc_count(void){return 2u;}
uint32_t frf_xhci_host_msc_slot_at(uint32_t i){return i==0u?5u:i==1u?9u:0u;}
uint32_t frf_xhci_host_msc_max_lun(uint32_t i){return i==0u?1u:0u;}
int frf_xhci_host_msc_bulk_out(uint32_t index,const void *data,
                               uint32_t length,uint32_t *actual){
    must(index<2u,"bulk OUT host index");
    if(length==31u){
        const uint8_t *b=data;
        must(!memcmp(b,"USBC",4u),"CBW signature");
        memcpy(h[index].cbw,b,31u);
        h[index].opcode=b[15];h[index].lun=b[13];
        if(h[index].opcode==0x00u)++turs[index];
        if(h[index].opcode==0x25u){
            ++readcapacities[index];
            if(simulate_empty_lun0 && index==0u && h[index].lun==0u){
                ++unexpected_empty_read_capacity;
                must(0,"read capacity issued to known empty reader LUN0: would stall data stage");
            }
        }
        h[index].lba=((uint32_t)b[17]<<24)|((uint32_t)b[18]<<16)|
                     ((uint32_t)b[19]<<8)|b[20];
        h[index].blocks=((uint32_t)b[22]<<8)|b[23];
        h[index].tag=getle(b+4);
        h[index].bytes=getle(b+8);
        if(actual)*actual=31u;
        return 1;
    }
    must(h[index].opcode==0x2au,"bulk payload follows WRITE10");
    must(h[index].bytes==length,"WRITE10 byte count");
    ++h[index].writes;
    if(index==0u&&h[index].lun==0u)++writer_unit0;
    if(actual)*actual=length;
    return 1;
}
int frf_xhci_host_msc_bulk_in(uint32_t index,void *data,
                              uint32_t length,uint32_t *actual){
    must(index<2u,"bulk IN host index");
    uint8_t *b=data;
    memset(b,0,length);
    if(length==13u){
        memcpy(b,"USBS",4u);putle(b+4,h[index].tag);
        if(simulate_empty_lun0 && index==0u && h[index].lun==0u &&
           h[index].opcode==0x00u)b[12]=1u; /* TUR CHECK, zero data */
        if(simulate_bad_rdb_sector && index==0u && h[index].lun==1u &&
           h[index].opcode==0x28u && h[index].lba==1026143u)
            b[12]=1u; /* SCSI CHECK CONDITION on a valid sector read */
    }else if(length==8u && h[index].opcode==0x25u){
        uint32_t cap=index==1u?8192u:h[index].lun==1u?2097152u:16384u;
        putbe(b,cap-1u);putbe(b+4,512u);
    }else {
        must(h[index].opcode==0x28u,"READ10 data IN");
        must(length==h[index].blocks*512u,"READ10 count");
        if(index==0u&&h[index].lun==1u &&
           h[index].lba<=3u&&3u<h[index].lba+h[index].blocks){
            uint32_t offset=(3u-h[index].lba)*512u;
            memcpy(b+offset,"RDSK",4u);
        }else if(length) b[0]=0xA5u;
    }
    if(actual)*actual=length;
    return 1;
}
extern void frf_pi4_um_probe_inline(void);
extern void frf_pi4_um_retry(uint64_t,uint64_t);
extern void frf_pi4_um_service(void);
extern void frf_pi4_um_init(void);
extern void frf_pi4_um_reset(void);
int main(void){
    frf_um_host=(volatile struct frf_um_shared*)board;
    volatile struct frf_um_shared *s=frf_um_host;
    s->magic=FRF_UM_MAGIC;
    s->version=FRF_UM_VERSION;
    s->bytes=sizeof(struct frf_um_shared);
    s->sector_size=512u;
    s->state=FRF_UM_IDLE;
    frf_pi4_um_probe_inline();
    frf_pi4_um_init();
    frf_pi4_um_service();
    volatile struct frf_um_extension *e=
       (volatile struct frf_um_extension*)(board+FRF_UM_EXT_OFFSET);
    must(e->magic==FRF_UM_EXT_MAGIC&&e->version==2u,"FRUM additive v2");
    must(e->count==3u,"3 logical units (two physical hosts)");
    /* R138 boot-only eager detection: secondary LUN1 RDB should
     * be known BEFORE CaffeineOS can execute its USBR0..USBR4 mounts. */
    must(e->units[1].capacity==2097152u &&
         (e->units[1].flags&FRF_UM_UNIT_RDB)!=0u,
         "R138 boot exposes LUN1 RDB before Amiga mount startup");
    must(turs[0]>0u,"TUR checked first before READ CAPACITY");
    must(e->units[0].capacity==16384u&&e->units[0].slot==5u&&
         e->units[0].lun==0u,"unit0 FAT disk");
    must(e->units[2].capacity==8192u&&e->units[2].slot==9u,
         "unit2 on second physical MSC host");
    must((e->units[0].flags&FRF_UM_UNIT_WRITE)==0u,"R142 RDB-only opt-in leaves FAT unit0 RO");
    frf_pi4_um_retry(1000u,1000000u);
    frf_pi4_um_retry(1000001u,1000000u);
    frf_pi4_um_service();
    must(e->units[1].capacity==2097152u &&e->units[1].lun==1u,
         "second LUN capacity");
    must(e->units[1].flags&FRF_UM_UNIT_RDB,"RDB discovered in blocks0-15");
    must((e->units[1].flags&FRF_UM_UNIT_WRITE)!=0u,"RDB write opt-in only after RDSK");
    must(frf_r76_usb_mass_count==3u,"all media ready");
    e->request_unit=1u;
    s->command=FRF_UM_READ;s->lba=3u;s->sectors=1u;s->sequence=111u;
    s->state=FRF_UM_REQ;frf_pi4_um_service();
    must(s->state==FRF_UM_DONE && s->completed==111u &&
         !memcmp((const void*)s->data,"RDSK",4u),"RDB LBA3 read unit1");
    must(e->request_unit==0u,"reset unit selector to 0");
    s->state=FRF_UM_IDLE;
    e->request_unit=1u;
    s->command=FRF_UM_WRITE;s->lba=3u;s->sectors=1u;s->sequence=112u;
    s->state=FRF_UM_REQ;frf_pi4_um_service();
    must(s->state==FRF_UM_DONE && s->result==512u,
         "opt-in RDB write routes successfully");
    must(h[0].writes==1u &&h[0].lun==1u &&h[0].lba==3u &&
         h[1].writes==0u,
         "RDB WRITE10 goes ONLY to correct physical host0 LUN1");
    s->state=FRF_UM_IDLE;
    e->request_unit=0u;
    s->command=FRF_UM_WRITE;s->lba=100u;s->sectors=1u;s->sequence=113u;
    s->state=FRF_UM_REQ;frf_pi4_um_service();
    must(s->state==FRF_UM_ERROR && (int32_t)s->result==-30 &&
         writer_unit0==0u,
         "R142 RDB-only opt-in must NOT enable FAT unit0 writes");
    /* Some multi-slot readers advertise an EMPTY LUN0 and a populated
     * LUN1. Ensure retries do not starve LUN1 behind repeated LUN0 CHECKs. */
    simulate_empty_lun0=1u;
    unexpected_empty_read_capacity=0u;
    frf_pi4_um_reset();
    s->state=FRF_UM_IDLE;
    frf_pi4_um_probe_inline();
    frf_pi4_um_init(); /* boot readiness must sweep additional reader LUNs */
    frf_pi4_um_service();
    must(e->units[0].capacity==0u &&e->units[2].capacity==8192u,
         "empty LUN0 cannot hide second physical USB device");
    must(unexpected_empty_read_capacity==0u,
         "empty reader LUN0 skipped dangerous READ CAPACITY data stage");
    must(e->units[1].capacity==2097152u &&
         (e->units[1].flags&FRF_UM_UNIT_RDB) &&
         (e->units[1].flags&FRF_UM_UNIT_WRITE),
         "populated LUN1 RDB writable even when LUN0 empty");
    frf_pi4_um_retry(2000000u,1000000u);
    frf_pi4_um_retry(2300000u,1000000u);
    frf_pi4_um_retry(2600000u,1000000u);
    frf_pi4_um_service();
    must(e->units[1].capacity==2097152u &&
        (e->units[1].flags&FRF_UM_UNIT_RDB)!=0u,
        "round robin locates RDB in LUN1 despite empty LUN0");
    /* Reproduce report: failing PFS3 read at end of USBR0, even though
     * RDB LUN is otherwise present and the other physical device works.
     * Ordinary CSW CHECK CONDITION must be a per-LUN error, not trigger
     * kill_host across the entire reader after three failures. */
    simulate_bad_rdb_sector=1u;
    for(unsigned k=0;k<5u;k++){
        s->state=FRF_UM_IDLE;
        e->request_unit=1u;
        s->command=FRF_UM_READ;s->lba=1026143u;s->sectors=1u;
        s->sequence=200u+k;
        s->state=FRF_UM_REQ;
        frf_pi4_um_service();
        must(s->state==FRF_UM_ERROR && s->completed==200u+k,
             "PFS3 boundary read failure is correctly reported");
        must(e->units[1].present && e->units[1].capacity==2097152u,
             "per-LUN SCSI check must NOT quarantine entire card reader");
        must(e->units[2].present && e->units[2].capacity==8192u,
             "unrelated MSC host still present after boundary failures");
    }
    simulate_bad_rdb_sector=0u;
    s->state=FRF_UM_IDLE;e->request_unit=1u;
    s->command=FRF_UM_READ;s->lba=1026143u;s->sectors=1u;
    s->sequence=210u;s->state=FRF_UM_REQ;frf_pi4_um_service();
    must(s->state==FRF_UM_DONE && s->completed==210u &&
         s->result==512u,"boundary read recovers after SCSI sense clears");
    must(e->units[1].present && e->units[2].present,
         "all media available after recovered read");
    puts("PASS R141: repeated SCSI CHECK at PFS3 LBA1026143 does not kill USB reader; reads recover; sibling survives; RDB write opt-in only");

    return 0;
}
