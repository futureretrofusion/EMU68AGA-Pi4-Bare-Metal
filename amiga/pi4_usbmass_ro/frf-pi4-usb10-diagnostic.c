/* FRF EMU68AGA Pi4 USB10 diagnosis: Guest Zorro board + resident drivers.
 * This program is purely observational and never reads/writes a USB sector,
 * resets USB or changes startup/mount configuration.
 * Exit 0 after printing diagnoses: avoids ambiguous return code 10.
 */
#define __NOLIBBASE__
#define EXEC_BASE_NAME (*(struct ExecBase **)4UL)
#include <proto/exec.h>
#include <proto/expansion.h>
#include <exec/types.h>
#include <exec/io.h>
#include <libraries/configvars.h>
#include <stdint.h>
#include <stdio.h>
#include "frf_usb_mass.h"

#define VENDOR 0x6d73u
#define FRFNET_PRODUCT 0x68u
#define USBMASS_PRODUCT 0x69u

static void probe(const char *name)
{
    struct MsgPort *port=CreateMsgPort();
    if(!port) {
        printf("  %-22s SKIP (cannot allocate message port)\n",name);
        return;
    }
    struct IOStdReq *io=(struct IOStdReq*)CreateIORequest(port,sizeof(struct IOStdReq));
    if(!io) {
        printf("  %-22s SKIP (cannot allocate IO request)\n",name);
        DeleteMsgPort(port);
        return;
    }
    LONG rc=OpenDevice((STRPTR)name,0,(struct IORequest*)io,0);
    printf("  %-22s OpenDevice rc=%ld [%s]\n",
           name,(long)rc,rc?"FAILED":"OPEN OK");
    if(!rc)CloseDevice((struct IORequest*)io);
    DeleteIORequest((struct IORequest*)io);
    DeleteMsgPort(port);
}
int main(void)
{
    puts("FRF Pi4 USB10 guest diagnostic v1 / AmigaOS 680x0");
    puts("No USB sector writes, no mount changes, no hardware reset.");
    struct Library *lib=OpenLibrary("expansion.library",36);
    if(!lib){
        puts("FAIL: expansion.library v36 not available. Cannot inspect Zorro.");
        puts("RESULT=EXPANSION_LIBRARY_MISSING");
        return 0;
    }
    struct ExpansionBase *ExpansionBase=(struct ExpansionBase*)lib;
    struct ConfigDev *cd=NULL,*usb=NULL;
    unsigned scanned=0,listed=0;
    puts("Zorro autoconfig boards discovered by expansion.library:");
    while((cd=FindConfigDev(cd,-1,-1))!=NULL && scanned<96u) {
        ++scanned;
        uint32_t manufacturer=cd->cd_Rom.er_Manufacturer;
        uint32_t product=cd->cd_Rom.er_Product;
        if(manufacturer==VENDOR || listed<16u) {
            printf("  #%02u M=%04lx P=%02lx TYPE=%02lx ADDR=%08lx SIZE=%lu\n",
                  scanned,(unsigned long)manufacturer,
                  (unsigned long)product,
                  (unsigned long)cd->cd_Rom.er_Type,
                  (unsigned long)(uintptr_t)cd->cd_BoardAddr,
                  (unsigned long)cd->cd_BoardSize);
            ++listed;
        }
        if(manufacturer==VENDOR && product==USBMASS_PRODUCT)usb=cd;
    }
    printf("Zorro boards scanned=%u\n",scanned);
    if(!usb) {
        puts("BOARD=NOT_FOUND 6D73:0069");
        puts("Meaning: R115 native xHCI MSC READY does not prove 68k Zorro registration.");
        puts("The device files alone cannot fix missing Zorro autoconfig.");
    } else if(!usb->cd_BoardAddr ||
              usb->cd_BoardSize<sizeof(struct frf_um_shared)) {
        printf("BOARD=INVALID_MAPPING addr=%08lx size=%lu\n",
               (unsigned long)(uintptr_t)usb->cd_BoardAddr,
               (unsigned long)usb->cd_BoardSize);
        puts("Meaning: virtual board registered but the 64KiB mailbox is not valid.");
    } else {
        volatile struct frf_um_shared *s=(volatile struct frf_um_shared*)
                  usb->cd_BoardAddr;
        printf("BOARD=FOUND addr=%08lx size=%lu\n",
               (unsigned long)(uintptr_t)usb->cd_BoardAddr,
               (unsigned long)usb->cd_BoardSize);
        printf("FRUM magic=%08lx version=%lu structBytes=%lu sectorBytes=%lu\n",
               (unsigned long)s->magic,(unsigned long)s->version,
               (unsigned long)s->bytes,(unsigned long)s->sector_size);
        if(s->magic!=FRF_UM_MAGIC ||
           s->version!=FRF_UM_VERSION ||
           s->bytes!=sizeof(struct frf_um_shared) ||
           s->sector_size!=FRF_UM_SECTOR_SIZE) {
            puts("BOARD=ABI_MISMATCH - do not try a FAT95 mount");
        } else {
            printf("MAILBOX present=%lu capacity=%lu slot=%lu tag=%08lx state=%lu\n",
                   (unsigned long)s->present,(unsigned long)s->capacity,
                   (unsigned long)s->reserved[0],(unsigned long)s->reserved[1],
                   (unsigned long)s->state);
            if(!s->present || !s->capacity)
                puts("BOARD=FRUM_PRESENT_BUT_MEDIA_NOT_REPORTED");
            else
                puts("BOARD=READY_FOR_DRIVER_OPEN");
        }
    }
    CloseLibrary(lib);
    puts("AmigaOS driver opening results (no USB sectors accessed):");
    if(!usb || !usb->cd_BoardAddr ||
       usb->cd_BoardSize<sizeof(struct frf_um_shared)) {
        puts("  SKIPPED: absent virtual Zorro mailbox; opening driver cannot help.");
    } else {
        probe("usbmass.device");
        probe("pi4usbmass.device");
    }
    puts("END USB10 - program exit code zero. Report printed BOARD and OpenDevice.");
    return 0;
}
