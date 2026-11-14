/* SPDX-License-Identifier: GPL-2.0-or-later
 * FRF EMU68AGA Pi4 R112, native xHCI USB Mass Storage (BOT/SCSI READ10).
 * One boot-present 512-byte SCSI disk, exclusively ARM CPU1, read-only.
 *
 * Uses the proven FRUM v1 Pi3 virtual Zorro-II mailbox and AmigaOS driver.
 * The VL805 bulk transport is completely independent from Pi3 DWC2/USPi.
 * No writes, device resets, filesystem mutation, auto-format or hotplug.
 */
#include <stdint.h>
#include <support.h>
#include "frf_xhci_host.h"
#include "frf_usb_mass.h"

extern volatile struct frf_um_shared *frf_um_host;

static uint32_t capacity_sectors;
static uint32_t media_reported;
static uint32_t bot_tag=0x52313200u;
static uint32_t capacity_attempts;
static uint64_t next_capacity_attempt;
static uint32_t last_bot_error;
static uint32_t capacity_check_first_reported;
/* DMA buffers must be cacheline aligned; never DMA directly into an
 * emulated 68k mapping or a small unaligned stack allocation. */
static uint8_t cbw[64] __attribute__((aligned(64)));
static uint8_t csw[64] __attribute__((aligned(64)));
static uint8_t payload[FRF_UM_MAX_SECTORS*FRF_UM_SECTOR_SIZE]
             __attribute__((aligned(64)));

static void mem_zero(uint8_t *dst,uint32_t bytes)
{
    while(bytes--) *dst++=0u;
}
static uint32_t le32_load(const uint8_t *p)
{
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static uint32_t be32_load(const uint8_t *p)
{
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|
           ((uint32_t)p[2]<<8)|(uint32_t)p[3];
}
static void le32_store(uint8_t *p,uint32_t n)
{
    p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);
    p[2]=(uint8_t)(n>>16);p[3]=(uint8_t)(n>>24);
}

static int scsi_bot_in(const uint8_t *cdb,uint32_t cdb_len,
                       uint8_t *dst,uint32_t bytes)
{
    if(!frf_xhci_host_msc_slot() || cdb_len<1u || cdb_len>16u ||
       bytes>sizeof(payload) || (bytes && !dst))return 0;

    mem_zero(cbw,sizeof(cbw));
    mem_zero(csw,sizeof(csw));
    /* USB BOT sends these signatures as little-endian BYTE sequences,
     * despite the ARM-side big-endian build configuration. */
    cbw[0]='U';cbw[1]='S';cbw[2]='B';cbw[3]='C';
    uint32_t tag=++bot_tag;
    le32_store(cbw+4,tag);
    le32_store(cbw+8,bytes);
    cbw[12]=bytes?0x80u:0u; /* data from mass-storage device */
    cbw[13]=0u;            /* LUN 0 */
    cbw[14]=(uint8_t)cdb_len;
    for(uint32_t i=0;i<cdb_len;++i)cbw[15+i]=cdb[i];

    uint32_t got=0u;
    last_bot_error=0u;
    if(!frf_xhci_host_bulk_out(cbw,31u,&got) || got!=31u) {
        last_bot_error=1u; /* CBW stage */
        return 0;
    }
    if(bytes && (!frf_xhci_host_bulk_in(dst,bytes,&got) || got!=bytes)) {
        last_bot_error=2u; /* data stage */
        return 0;
    }
    got=0u;
    if(!frf_xhci_host_bulk_in(csw,13u,&got) || got!=13u) {
        last_bot_error=3u; /* CSW stage */
        return 0;
    }
    if(csw[0]!='U' || csw[1]!='S' || csw[2]!='B' || csw[3]!='S') {
        last_bot_error=4u;
        return 0;
    }
    if(le32_load(csw+4)!=tag || le32_load(csw+8)!=0u) {
        last_bot_error=5u;
        return 0;
    }
    if(csw[12]!=0u) {
        last_bot_error=6u; /* SCSI CHECK CONDITION / drive not ready */
        return 0;
    }
    return 1;
}

uint32_t frf_pi4_um_capacity(void){return capacity_sectors;}

/* One bounded synchronous capacity probe. USB spinning disks may return
 * SCSI CHECK CONDITION for several seconds after USB configuration. */
static void try_read_capacity(void)
{
    if(!frf_xhci_host_msc_slot() || capacity_sectors ||
       capacity_attempts>=12u) return;
    ++capacity_attempts;
    const uint8_t cdb[10]={0x25u,0u,0u,0u,0u,0u,0u,0u,0u,0u};
    mem_zero(payload,64u);
    if(!scsi_bot_in(cdb,10u,payload,8u)) {
        if(!capacity_check_first_reported || capacity_attempts==12u) {
            kprintf("[R112-MSC] READ CAPACITY not ready attempt=%u/12 BOT_stage=%u (HDD spinup?)\n",
                    capacity_attempts,last_bot_error);
            capacity_check_first_reported=1u;
        }
        return;
    }
    const uint32_t last=be32_load(payload);
    const uint32_t block=be32_load(payload+4u);
    if(block!=FRF_UM_SECTOR_SIZE || last==0xffffffffu || !last) {
        /* Only 512-byte LUN 0 disks with a 32-bit LBA are supported.
         * Never misreport a 4 KiB-native/GPT disk as a 512-byte volume. */
        kprintf("[R112-MSC] unsupported SCSI geometry sectorSize=%u lastLBA=%u\n",
                block,last);
        capacity_attempts=12u;
        return;
    }
    capacity_sectors=last+1u;
    /* The POST and guest both need to observe late spin-up readiness. */
    {
        extern volatile uint32_t frf_r76_usb_mass_count;
        __atomic_store_n(&frf_r76_usb_mass_count,1u,__ATOMIC_RELEASE);
    }
    kprintf("[R112-MSC] USB unit0 RO READY slot=%u sectors=%u MiB=%u attempt=%u\n",
            frf_xhci_host_msc_slot(),capacity_sectors,
            capacity_sectors/2048u,capacity_attempts);
}

uint32_t frf_pi4_um_last_bot_error(void){return last_bot_error;}
uint32_t frf_pi4_um_attempts(void){return capacity_attempts;}

void frf_pi4_um_init(void)
{
    capacity_sectors=0u;
    media_reported=0u;
    capacity_attempts=0u;
    next_capacity_attempt=0u;
    last_bot_error=0u;
    capacity_check_first_reported=0u;
    if(!frf_xhci_host_msc_slot()) {
        kprintf("[R112-MSC] no USB BOT slot: no drive enumerated at power-on\n");
        return;
    }
    try_read_capacity();
}

/* CPU1 calls this with its monotonic CNT ticks. Polling the disk is throttled
 * to 2-second intervals and at most 12 attempts, not a busy retry loop.
 * The xHCI host's asynchronous HID completion queue remains independent. */
void frf_pi4_um_retry(uint64_t now,uint64_t hz)
{
    if(capacity_sectors || !frf_xhci_host_msc_slot() ||
       capacity_attempts>=12u || !hz || hz>2000000000ull) return;
    if(!next_capacity_attempt) {
        next_capacity_attempt=now+2u*hz;
        return;
    }
    if((int64_t)(now-next_capacity_attempt)<0)return;
    next_capacity_attempt=now+2u*hz;
    try_read_capacity();
}

void frf_pi4_um_service(void)
{
    volatile struct frf_um_shared *s=
        __atomic_load_n(&frf_um_host,__ATOMIC_ACQUIRE);
    if(!s || s->magic!=FRF_UM_MAGIC ||
       s->version!=FRF_UM_VERSION ||
       s->bytes!=sizeof(struct frf_um_shared))
        return;

    s->reserved[0]=frf_xhci_host_msc_slot();
    s->reserved[1]=0x50493452u; /* "PI4R": read-only Pi4 transport */
    /* The 68k may autoconfigure the FRUM board before a spinning HDD
     * becomes ready. Publish late capacity changes without overwriting
     * a guest request or pretending the disk is ready prematurely. */
    if(!media_reported || s->capacity!=capacity_sectors) {
        s->capacity=capacity_sectors;
        s->present=capacity_sectors?1u:0u;
        s->media_change=media_reported?s->media_change+1u:1u;
        __atomic_thread_fence(__ATOMIC_RELEASE);
        media_reported=1u;
    }

    if(__atomic_load_n(&s->state,__ATOMIC_ACQUIRE)!=FRF_UM_REQ)
        return;
    const uint32_t seq=s->sequence;
    const uint32_t command=s->command;
    const uint32_t lba=s->lba;
    const uint32_t sectors=s->sectors;
    __atomic_store_n(&s->state,FRF_UM_BUSY,__ATOMIC_RELEASE);

    int32_t rc=-1;
    if(command==FRF_UM_WRITE) {
        /* Explicitly fail every write, even a malformed 68k command. */
        rc=-30;
    } else if(command==FRF_UM_READ && capacity_sectors && sectors>0u &&
              sectors<=FRF_UM_MAX_SECTORS && lba<capacity_sectors &&
              sectors<=capacity_sectors-lba) {
        uint8_t cdb[10]={0x28u,0u,0u,0u,0u,0u,0u,0u,0u,0u};
        cdb[2]=(uint8_t)(lba>>24);cdb[3]=(uint8_t)(lba>>16);
        cdb[4]=(uint8_t)(lba>>8);cdb[5]=(uint8_t)lba;
        cdb[7]=(uint8_t)(sectors>>8);cdb[8]=(uint8_t)sectors;
        const uint32_t bytes=sectors*FRF_UM_SECTOR_SIZE;
        if(scsi_bot_in(cdb,10u,payload,bytes)) {
            for(uint32_t i=0u;i<bytes;i++)s->data[i]=payload[i];
            rc=(int32_t)bytes;
        } else {
            kprintf("[R112-MSC] READ10 failed LBA=%u count=%u\n",
                    lba,sectors);
        }
    }
    s->result=(uint32_t)rc;
    s->completed=seq;
    __atomic_thread_fence(__ATOMIC_RELEASE);
    __atomic_store_n(&s->state,
           (command==FRF_UM_READ && rc==(int32_t)(sectors*FRF_UM_SECTOR_SIZE)) ?
           FRF_UM_DONE:FRF_UM_ERROR,__ATOMIC_RELEASE);
}
