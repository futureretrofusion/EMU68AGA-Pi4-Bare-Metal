/* SPDX-License-Identifier: GPL-2.0-or-later
 * FRF usbmass.device 0.5 read/write, native Pi4 512-byte USB disk.
 * Loads as normal AmigaOS resident. Requires virtual Zorro II 6D73:0069
 * mapped by Emu68. USB controller is exclusively owned by ARM CPU1.
 */
#define __NOLIBBASE__
#define EXEC_BASE_NAME (*(struct ExecBase **)4UL)
#include <proto/exec.h>
#include <proto/expansion.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/devices.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <devices/trackdisk.h>
#include <devices/newstyle.h>
#include <libraries/configvars.h>
#include <stdint.h>
#include "frf_usb_mass.h"
#include "frf_r138_unit_map.h"
#include "frf_r139_scsi_rw.h"
#include <devices/scsidisk.h>

/* Executed if the load segment is inadvertently treated as an application.
 * This safe entry prologue satisfies the AmigaOS InitResident documentation;
 * a first-hunk resident tag is still mandatory and checked separately.
 * Do not place auto-init tables or pointers at the executable entry. */
__asm__(".section .text,\"ax\"\n"
        ".globl _usbmass_hunk_entry\n"
        "_usbmass_hunk_entry:\n"
        "moveq #-1,%d0\n"
        "rts\n");

struct R133GuestUnit {
    struct Unit base;
    ULONG index;
    ULONG physical;
    ULONG open_count;
};
struct USBMassDevice {
    struct Device dev;
    BPTR seg;
    volatile struct frf_um_shared *shared;
    ULONG users;
    ULONG busy;
    ULONG next_sequence;
    struct R133GuestUnit logical[FRF_UM_MAX_UNITS];
};
static volatile struct frf_um_extension *ext(volatile struct frf_um_shared *s){
    return (volatile struct frf_um_extension *)
        ((uintptr_t)s+FRF_UM_EXT_OFFSET);
}
static BOOL extended(struct USBMassDevice *d){
    volatile struct frf_um_shared *s=d->shared;
    if(!s)return FALSE;
    volatile struct frf_um_extension *e=ext(s);
    return e->magic==FRF_UM_EXT_MAGIC &&
           e->version==FRF_UM_EXT_VERSION && e->count<=FRF_UM_MAX_UNITS;
}
/* Every successful open pins its physical USB target for that handle's
 * lifetime. A late RDB enumeration must not silently redirect in-flight
 * filesystem reads or writes to another disk. */
static ULONG guest_unit(struct USBMassDevice *d,const struct IOStdReq *io){
    for(ULONG i=0;i<FRF_UM_MAX_UNITS;i++)
        if(io->io_Unit==(struct Unit*)&d->logical[i].base &&
           d->logical[i].open_count)
            return d->logical[i].physical;
    return FRF_UM_MAX_UNITS;
}
static ULONG capacity_for(struct USBMassDevice *d,ULONG index){
    if(index>=FRF_UM_MAX_UNITS||!d->shared)return 0u;
    if(extended(d)){
        volatile struct frf_um_extension *e=ext(d->shared);
        if(index>=e->count || !e->units[index].present)return 0u;
        return e->units[index].capacity;
    }
    return index==0u && d->shared->present?d->shared->capacity:0u;
}
static ULONG media_change_for(struct USBMassDevice *d,ULONG index){
    if(!d->shared)return 0u;
    if(extended(d)){
        volatile struct frf_um_extension *e=ext(d->shared);
        return index<e->count?e->units[index].media_change:0u;
    }
    return index==0u?d->shared->media_change:0u;
}
static char name[]="usbmass.device";
static char id[]="usbmass.device 0.8 (22.11.2026) FRF Pi4 R139 PFS3 DirectSCSI";
static char ver[]="\0$VER: usbmass.device 0.8 (22.11.2026)";
extern const UBYTE endOfCode;
void *initDev(struct USBMassDevice *d asm("d0"),BPTR seg asm("a0"),struct ExecBase *sys asm("a6"));
ULONG openDev(struct IOStdReq *io asm("a1"),ULONG num asm("d0"),ULONG flags asm("d1"),struct USBMassDevice *d asm("a6"));
ULONG closeDev(struct IOStdReq *io asm("a1"),struct USBMassDevice *d asm("a6"));
BPTR expungeDev(struct USBMassDevice *d asm("a6"));
ULONG extDev(struct USBMassDevice *d asm("a6"));
void beginDev(struct IOStdReq *io asm("a1"),struct USBMassDevice *d asm("a6"));
ULONG abortDev(struct IOStdReq *io asm("a1"),struct USBMassDevice *d asm("a6"));

static const APTR funcs[]={
    (APTR)openDev,(APTR)closeDev,(APTR)expungeDev,(APTR)extDev,
    (APTR)beginDev,(APTR)abortDev,(APTR)-1
};
static const APTR initTab[]={
    (APTR)sizeof(struct USBMassDevice),(APTR)funcs,0,(APTR)initDev
};
static struct Resident const resident __attribute__((used))={
    RTC_MATCHWORD,(struct Resident*)&resident,(APTR)&endOfCode,
    RTF_AUTOINIT|RTF_AFTERDOS,5,NT_DEVICE,0,(APTR)name,(APTR)id,(APTR)initTab
};
/* Avoid a C runtime startup; Exec scans the resident structure. */
LONG __attribute__((used,no_reorder)) doNotExecute(void){return -1;}
/* EndSkip is emitted after every function, not before the entry points. */

static void finish(struct IOStdReq *io){
    if (!(io->io_Flags & IOF_QUICK))
        ReplyMsg((struct Message*)io);
}
static void copy_to(void *dst,const void *src,ULONG bytes){
    UBYTE *d=(UBYTE*)dst;const UBYTE *s=(const UBYTE*)src;
    while(bytes--)*d++=*s++;
}
static BOOL live(struct USBMassDevice *d,ULONG unit){
    volatile struct frf_um_shared *s=d->shared;
    return s && s->magic==FRF_UM_MAGIC &&
           s->version==FRF_UM_VERSION &&
           s->bytes==sizeof(struct frf_um_shared) &&
           s->sector_size==FRF_UM_SECTOR_SIZE &&
           capacity_for(d,unit)!=0u;
}
/* A v0.5 guest must stay genuinely read-only with the older R117
 * kernel, which intentionally reports "PI4R". R121 reports "PI4W".
 * Do not infer write support merely from a successful USB READ10. */
static BOOL writable(struct USBMassDevice *d,ULONG unit){
    if(!live(d,unit) || d->shared->reserved[1]!=0x50493457u)return FALSE;
    if(extended(d)){
        volatile struct frf_um_extension *e=ext(d->shared);
        if(unit>=e->count)return FALSE;
        return (e->units[unit].flags&FRF_UM_UNIT_WRITE)!=0u;
    }
    return unit==0u; /* old R132 kernel permits unit0 only */
}

void *initDev(struct USBMassDevice *d asm("d0"), BPTR seg asm("a0"),struct ExecBase *sys asm("a6")){
    (void)sys;(void)ver;
    d->seg=seg;
    d->users=0;d->busy=0;d->shared=0;d->next_sequence=0;
    d->dev.dd_Library.lib_Revision=8;
    for(ULONG i=0;i<FRF_UM_MAX_UNITS;i++){
        d->logical[i].index=i;
        d->logical[i].physical=i;
        d->logical[i].open_count=0u;
    }
    return d;
}
ULONG openDev(struct IOStdReq *io asm("a1"),ULONG unit asm("d0"),ULONG flags asm("d1"),struct USBMassDevice *d asm("a6")){
    (void)flags;
    /* R120 open-stage error codes, diagnostic during experimental release:
       -20: entered Open() but unit or request is invalid
       -21: expansion.library v36 could not be opened
       -22: Zorro FRUM board not found/mapped
       -23: FRUM header ABI incorrect
       -24: device capacity not yet published
       -25: module auto-init/open never completed normally
       Cold reboot after replacing an older resident device. */
    io->io_Error=-20;
    if(unit>=FRF_UM_MAX_UNITS ||
       io->io_Message.mn_Length<sizeof(struct IOStdReq))return 0;
    if(!d->shared){
        io->io_Error=-21;
        struct Library *lib=OpenLibrary("expansion.library",36);
        if(!lib)return 0;
        struct ExpansionBase *ExpansionBase=(struct ExpansionBase*)lib;
        struct ConfigDev *cfg=FindConfigDev(NULL,FRF_UM_VENDOR,FRF_UM_PRODUCT);
        io->io_Error=-22;
        if(cfg && cfg->cd_BoardAddr)d->shared=(struct frf_um_shared*)cfg->cd_BoardAddr;
        CloseLibrary(lib);
        if(!d->shared)return 0;
    }
    io->io_Error=-23;
    volatile struct frf_um_shared *m=d->shared;
    if(m->magic!=FRF_UM_MAGIC || m->version!=FRF_UM_VERSION ||
       m->bytes!=sizeof(struct frf_um_shared) ||
       m->sector_size!=FRF_UM_SECTOR_SIZE)return 0;
    io->io_Error=-24;
    /* Pin guest unit 0 to the first ready RDB disk (when one exists).
     * Other guest units transparently move up, preserving their access.
     * If a unit was already opened, it can never retarget mid-mount. */
    ULONG physical=d->logical[unit].open_count?
        d->logical[unit].physical:
        (extended(d)?frf_r138_physical_unit(ext(d->shared),unit):unit);
    if(physical>=FRF_UM_MAX_UNITS || !live(d,physical))return 0;
    if(!d->logical[unit].open_count)d->logical[unit].physical=physical;
    ++d->logical[unit].open_count;
    ++d->users;
    io->io_Device=(struct Device*)d;
    io->io_Unit=(struct Unit*)&d->logical[unit].base;
    io->io_Error=0;
    return 0;
}
ULONG closeDev(struct IOStdReq *io asm("a1"),struct USBMassDevice *d asm("a6")){
    for(ULONG i=0;i<FRF_UM_MAX_UNITS;i++)
        if(io->io_Unit==(struct Unit*)&d->logical[i].base){
            if(d->logical[i].open_count)--d->logical[i].open_count;
            break;
        }
    if(d->users)d->users--;
    io->io_Device=NULL;io->io_Unit=NULL;
    return 0;
}
BPTR expungeDev(struct USBMassDevice *d asm("a6")){(void)d;return 0;} /* pinned */
ULONG extDev(struct USBMassDevice *d asm("a6")){(void)d;return 0;}
ULONG abortDev(struct IOStdReq *io asm("a1"),struct USBMassDevice *d asm("a6")){
    (void)io;(void)d;return IOERR_NOCMD;
}
static LONG transfer_disk(struct USBMassDevice *d,struct IOStdReq *io,int large,int writing){
    volatile struct frf_um_shared *s=d->shared;
    const ULONG unit=guest_unit(d,io);
    if(unit>=FRF_UM_MAX_UNITS)return IOERR_BADADDRESS;
    UBYTE *dest=(UBYTE*)io->io_Data;
    uint64_t offset=(uint64_t)io->io_Offset;
    ULONG remain=io->io_Length;
    if(large)offset |= ((uint64_t)io->io_Actual << 32);
    io->io_Actual=0;
    if (!dest || (offset & 511u) || (remain & 511u))return IOERR_BADADDRESS;
    if (writing && !writable(d,unit))return TDERR_WriteProt;
    if (!live(d,unit))return TDERR_DiskChanged;
    const ULONG cap=capacity_for(d,unit);
    if (offset/512u > cap ||
        (uint64_t)(remain/512u) > cap-offset/512u)
        return TDERR_BadSecPreamble;

    while(remain){
        ULONG sectors=remain/FRF_UM_SECTOR_SIZE;
        ULONG bytes,seq;
        if(sectors>FRF_UM_MAX_SECTORS)sectors=FRF_UM_MAX_SECTORS;
        bytes=sectors*FRF_UM_SECTOR_SIZE;
        /* No concurrent callers can share one mailbox. Forbid only while
         * taking the lock; NEVER hold it while waiting on CPU1. */
        Forbid();
        if(d->busy || s->state!=FRF_UM_IDLE){
            Permit();return TDERR_DriveInUse;
        }
        d->busy=1;
        Permit();
        /* Stage bytes before REQ; other Amiga requests observe d->busy.
         * Avoid copying 8 KiB while interrupts/task switching are forbidden. */
        if(writing)copy_to((void*)s->data,(const void*)dest,bytes);
        Forbid();
        seq=++d->next_sequence;
        if(!seq)seq=++d->next_sequence;
        if(extended(d))ext(s)->request_unit=unit;
        else if(unit!=0u){
            d->busy=0;Permit();return TDERR_DiskChanged;
        }
        s->command=writing?FRF_UM_WRITE:FRF_UM_READ;
        s->lba=(ULONG)(offset/512u);
        s->sectors=sectors;
        s->sequence=seq;
        __asm__ volatile("" ::: "memory");
        s->state=FRF_UM_REQ;
        Permit();

        /* CPU1 progresses independently. Bounded spin is an emergency
         * stop for absent firmware; this is NOT a wall-clock deadline.
         * Timeout leaves the mailbox owned by CPU1, avoiding buffer races. */
        ULONG limit=120000000u;
        BOOL finished=FALSE;
        while(limit--){
            ULONG state=s->state;
            if ((state==FRF_UM_DONE || state==FRF_UM_ERROR) &&
                 s->completed==seq) {
                finished=TRUE;
                break;
            }
        }
        if (!finished || s->completed!=seq){
            d->busy=0;
            return TDERR_NotSpecified;
        }
        LONG result=(LONG)s->result;
        if(s->state!=FRF_UM_DONE || result!=(LONG)bytes){
            s->state=FRF_UM_IDLE;
            d->busy=0;
            return TDERR_NotSpecified;
        }
        if(!writing)copy_to(dest,(const void*)s->data,bytes);
        __asm__ volatile("" ::: "memory");
        s->state=FRF_UM_IDLE;
        d->busy=0;
        dest+=bytes;remain-=bytes;offset+=bytes;
        io->io_Actual+=bytes;
    }
    return 0;
}
/* R139: PDS/3 filesystem performs block I/O with HD_SCSICMD rather
 * than TD_READ64. Emulate the read-only SCSI direct-access subset over
 * the SAME ARM-owned USB BOT mailbox, preserving R138 RDB mapping.
 * This driver does not send arbitrary SCSI CDBs to the storage medium
 * or allow writes to RDB-backed physical units.
 */
static void r139_put_be32(UBYTE *p,ULONG n){
    p[0]=(UBYTE)(n>>24);p[1]=(UBYTE)(n>>16);
    p[2]=(UBYTE)(n>>8);p[3]=(UBYTE)n;
}
static ULONG r139_min(ULONG x,ULONG y){return x<y?x:y;}
static LONG r139_sense(struct SCSICmd *sc,UBYTE key,UBYTE asc){
    sc->scsi_Status=2; /* CHECK CONDITION */
    if(sc->scsi_SenseData && (sc->scsi_Flags&SCSIF_AUTOSENSE) &&
       sc->scsi_SenseLength>=4u){
        ULONG size=r139_min(sc->scsi_SenseLength,18u);
        UBYTE *b=sc->scsi_SenseData;
        for(ULONG i=0;i<size;i++)b[i]=0u;
        b[0]=0x70u; /* current fixed sense */
        if(size>2u)b[2]=key;
        if(size>7u)b[7]=(UBYTE)(size-8u);
        if(size>12u)b[12]=asc;
        sc->scsi_SenseActual=(UWORD)size;
    }
    return HFERR_BadStatus;
}
static LONG r139_scsi(struct USBMassDevice *d,struct IOStdReq *io){
    struct SCSICmd *sc=(struct SCSICmd*)io->io_Data;
    if(!sc || io->io_Length<sizeof(*sc) ||
       !sc->scsi_Command || sc->scsi_CmdLength==0u ||
       sc->scsi_CmdLength>16u)return IOERR_BADLENGTH;
    sc->scsi_Actual=0u;
    sc->scsi_CmdActual=0u;
    sc->scsi_SenseActual=0u;
    sc->scsi_Status=0u;
    const UBYTE op=sc->scsi_Command[0];
    ULONG unit=guest_unit(d,io);
    if(unit>=FRF_UM_MAX_UNITS || !live(d,unit))
        return r139_sense(sc,0x02u,0x3au); /* NOT READY, no medium */
    struct FrfScsiRW rw;
    int decoded=frf_r139_scsi_rw(sc->scsi_Command,sc->scsi_CmdLength,&rw);
    sc->scsi_CmdActual=sc->scsi_CmdLength;
    if(decoded<0)return r139_sense(sc,0x05u,0x24u);
    if(decoded){
        if((rw.bytes>sc->scsi_Length) ||
           (rw.bytes && !sc->scsi_Data) ||
           (!(sc->scsi_Flags&SCSIF_READ) && !rw.writing) ||
           ((sc->scsi_Flags&SCSIF_READ) && rw.writing))
            return r139_sense(sc,0x05u,0x24u);
        if(rw.bytes==0u)return 0;
        struct IOStdReq tmp=*io; /* don't overwrite original SCSICmd ptr */
        tmp.io_Data=(APTR)sc->scsi_Data;
        tmp.io_Length=rw.bytes;
        tmp.io_Offset=(ULONG)rw.offset;
        tmp.io_Actual=(ULONG)(rw.offset>>32);
        LONG err=transfer_disk(d,&tmp,1,rw.writing);
        sc->scsi_Actual=tmp.io_Actual;
        if(err==TDERR_WriteProt)return r139_sense(sc,0x07u,0x27u);
        if(err)return r139_sense(sc,0x03u,0x11u);
        return 0;
    }
    switch(op){
    case 0x00u: /* TEST UNIT READY */
    case 0x1bu: /* START STOP UNIT: non-actioning, boot-present disk */
    case 0x35u: /* SYNCHRONIZE CACHE: shared BOT writes complete serially */
        return 0;
    case 0x25u: { /* READ CAPACITY(10) */
        if(sc->scsi_CmdLength<10u || sc->scsi_Length<8u || !sc->scsi_Data)
            return r139_sense(sc,0x05u,0x24u);
        UBYTE *p=(UBYTE*)sc->scsi_Data;
        ULONG cap=capacity_for(d,unit);
        r139_put_be32(p,cap?cap-1u:0u);
        r139_put_be32(p+4u,512u);
        sc->scsi_Actual=8u;
        return 0;
    }
    case 0x12u: { /* INQUIRY standard direct-access device */
        if(sc->scsi_CmdLength<6u || (sc->scsi_Command[1]&1u))
            return r139_sense(sc,0x05u,0x24u);
        UBYTE b[36];
        for(unsigned i=0u;i<sizeof b;i++)b[i]=0u;
        b[0]=0x00u;b[1]=0x80u;b[2]=0x02u;b[3]=0x02u;b[4]=31u;
        const char *vendor="FRF     ";
        const char *product="EMU68AGA USB BOT";
        const char *rev="0139";
        for(unsigned i=0;i<8u;i++)b[8u+i]=(UBYTE)vendor[i];
        for(unsigned i=0;i<16u;i++)b[16u+i]=(UBYTE)product[i];
        for(unsigned i=0;i<4u;i++)b[32u+i]=(UBYTE)rev[i];
        ULONG n=r139_min(sc->scsi_Length,
                         r139_min(sc->scsi_Command[4],(ULONG)sizeof b));
        if(n && !sc->scsi_Data)return r139_sense(sc,0x05u,0x24u);
        for(ULONG i=0;i<n;i++)((UBYTE*)sc->scsi_Data)[i]=b[i];
        sc->scsi_Actual=n;
        return 0;
    }
    case 0x03u: { /* REQUEST SENSE: no error pending */
        if(sc->scsi_CmdLength<6u)return r139_sense(sc,0x05u,0x24u);
        ULONG n=r139_min(sc->scsi_Length,
                         r139_min(sc->scsi_Command[4],18u));
        if(n && !sc->scsi_Data)return r139_sense(sc,0x05u,0x24u);
        for(ULONG i=0;i<n;i++)((UBYTE*)sc->scsi_Data)[i]=0u;
        if(n)((UBYTE*)sc->scsi_Data)[0]=0x70u;
        if(n>7u)((UBYTE*)sc->scsi_Data)[7]=(UBYTE)(n-8u);
        sc->scsi_Actual=n;
        return 0;
    }
    case 0x1au: { /* MODE SENSE(6): no pages, protected status */
        if(sc->scsi_CmdLength<6u)return r139_sense(sc,0x05u,0x24u);
        UBYTE b[4]={3u,0u,0u,0u};
        if(!writable(d,unit))b[2]=0x80u;
        ULONG n=r139_min(sc->scsi_Length,
                         r139_min(sc->scsi_Command[4],4u));
        if(n && !sc->scsi_Data)return r139_sense(sc,0x05u,0x24u);
        for(ULONG i=0;i<n;i++)((UBYTE*)sc->scsi_Data)[i]=b[i];
        sc->scsi_Actual=n;
        return 0;
    }
    case 0x5au: { /* MODE SENSE(10): no pages, protected status */
        if(sc->scsi_CmdLength<10u)return r139_sense(sc,0x05u,0x24u);
        UBYTE b[8]={0u,6u,0u,0u,0u,0u,0u,0u};
        if(!writable(d,unit))b[3]=0x80u;
        ULONG n=r139_min(sc->scsi_Length,
                       r139_min(((ULONG)sc->scsi_Command[7]<<8)|
                              sc->scsi_Command[8],8u));
        if(n && !sc->scsi_Data)return r139_sense(sc,0x05u,0x24u);
        for(ULONG i=0;i<n;i++)((UBYTE*)sc->scsi_Data)[i]=b[i];
        sc->scsi_Actual=n;
        return 0;
    }
    default:
        return r139_sense(sc,0x05u,0x20u); /* ILLEGAL REQUEST opcode */
    }
}

static const UWORD supported_commands[]={
    CMD_READ,TD_READ64,NSCMD_TD_READ64,
    CMD_WRITE,TD_WRITE64,NSCMD_TD_WRITE64,
    TD_FORMAT,TD_FORMAT64,NSCMD_TD_FORMAT64,
    TD_GETGEOMETRY,TD_CHANGESTATE,TD_CHANGENUM,TD_PROTSTATUS,
    TD_GETDRIVETYPE,TD_MOTOR,TD_SEEK,CMD_CLEAR,CMD_UPDATE,
    NSCMD_DEVICEQUERY,HD_SCSICMD,0
};
void beginDev(struct IOStdReq *io asm("a1"),struct USBMassDevice *d asm("a6")){
    const ULONG unit=guest_unit(d,io);
    io->io_Error=0;
    if(unit>=FRF_UM_MAX_UNITS){
        io->io_Error=IOERR_BADADDRESS;finish(io);return;
    }
    switch(io->io_Command){
    case HD_SCSICMD:
        io->io_Error=(BYTE)r139_scsi(d,io);
        break;
    case CMD_READ:
    case TD_READ64:
    case NSCMD_TD_READ64:
        io->io_Error=(BYTE)transfer_disk(d,io,io->io_Command!=CMD_READ,0);
        break;
    case CMD_WRITE:
    case TD_WRITE64:
    case NSCMD_TD_WRITE64:
        io->io_Error=(BYTE)transfer_disk(d,io,io->io_Command!=CMD_WRITE,1);
        break;
    case TD_FORMAT:
    case TD_FORMAT64:
    case NSCMD_TD_FORMAT64:
        /* Trackdisk FORMAT writes block contents; not SCSI FORMAT UNIT.
         * Only the OS filesystem may request this, never automatically. */
        io->io_Error=(BYTE)transfer_disk(d,io,
                                         io->io_Command!=TD_FORMAT,1);
        break;
    case TD_CHANGESTATE:
        io->io_Actual=live(d,unit)?0:1;
        break;
    case TD_PROTSTATUS:
        io->io_Actual=writable(d,unit)?0u:1u;
        break;
    case TD_CHANGENUM:
        io->io_Actual=media_change_for(d,unit);
        break;
    case TD_GETDRIVETYPE:
        io->io_Actual=DG_DIRECT_ACCESS;
        break;
    case TD_GETGEOMETRY: {
        struct DriveGeometry *g=(struct DriveGeometry*)io->io_Data;
        if(!g || io->io_Length<sizeof(*g)){
            io->io_Error=IOERR_BADLENGTH;break;
        }
        g->dg_SectorSize=512;
        g->dg_TotalSectors=capacity_for(d,unit);
        g->dg_Heads=1;
        g->dg_TrackSectors=32;
        g->dg_CylSectors=32;
        g->dg_Cylinders=(g->dg_TotalSectors+31u)/32u;
        g->dg_BufMemType=MEMF_PUBLIC;
        g->dg_DeviceType=DG_DIRECT_ACCESS;
        g->dg_Flags=DGF_REMOVABLE;
        g->dg_Reserved=0;
        io->io_Actual=sizeof(*g);
        break;
    }
    case NSCMD_DEVICEQUERY: {
        struct NSDeviceQueryResult *q=(struct NSDeviceQueryResult*)io->io_Data;
        if(!q || io->io_Length<sizeof(*q)){
            io->io_Error=IOERR_BADLENGTH;break;
        }
        q->nsdqr_DevQueryFormat=0;
        q->nsdqr_SizeAvailable=sizeof(*q);
        q->nsdqr_DeviceType=NSDEVTYPE_TRACKDISK;
        q->nsdqr_DeviceSubType=0;
        q->nsdqr_SupportedCommands=(APTR)supported_commands;
        io->io_Actual=sizeof(*q);
        break;
    }
    case CMD_RESET:
    case CMD_CLEAR:
    case CMD_UPDATE:
    case TD_SEEK:
    case TD_SEEK64:
    case TD_MOTOR:
        io->io_Actual=0;break;
    default:
        io->io_Error=IOERR_NOCMD;break;
    }
    finish(io);
}

__asm__(".section .text\n.globl _endOfCode\n_endOfCode:\n");
