/* FRF EMU68AGA Pi4 R112 — read-only Zorro-II USB mass transport probe.
 * Proves guest-side autoconfiguration separately from USB enumeration and
 * filesystem mounting. Does not OpenDevice, USB reset, read or write a disk.
 */
#define __NOLIBBASE__
#define EXEC_BASE_NAME (*(struct ExecBase **)4UL)
#include <proto/exec.h>
#include <proto/expansion.h>
#include <exec/types.h>
#include <libraries/configvars.h>
#include <stdint.h>
#include <stdio.h>
#include "frf_usb_mass.h"
#define UL(v) ((unsigned long)(v))
int main(void)
{
    struct Library *lib=OpenLibrary("expansion.library",36);
    if(!lib){puts("USBMASS Z2: expansion.library not available");return 20;}
    struct ExpansionBase *ExpansionBase=(struct ExpansionBase*)lib;
    struct ConfigDev *cfg=FindConfigDev(NULL,FRF_UM_VENDOR,FRF_UM_PRODUCT);
    if(!cfg) {
        puts("USBMASS Z2: FRF 6D73:0069 board NOT PRESENT");
        puts("Kernel must present virtual Zorro-II board before Amiga device can open.");
        CloseLibrary(lib);
        return 10;
    }
    printf("USBMASS Z2: board=%08lx bytes=%lu\n",
           UL((uintptr_t)cfg->cd_BoardAddr),UL(cfg->cd_BoardSize));
    if(!cfg->cd_BoardAddr || cfg->cd_BoardSize<sizeof(struct frf_um_shared)) {
        puts("USBMASS Z2: configuration contains no valid 64 KiB mailbox");
        CloseLibrary(lib);return 10;
    }
    volatile struct frf_um_shared *s=(volatile struct frf_um_shared *)cfg->cd_BoardAddr;
    const int abi=s->magic==FRF_UM_MAGIC &&
                  s->version==FRF_UM_VERSION &&
                  s->bytes==sizeof(*s) &&
                  s->sector_size==512u;
    printf("USBMASS Z2: magic=%08lx version=%lu size=%lu sector=%lu\n",
           UL(s->magic),UL(s->version),UL(s->bytes),UL(s->sector_size));
    if(!abi){
        puts("USBMASS Z2: ABI MISMATCH. Do not use a mismatched usbmass.device.");
        CloseLibrary(lib);return 5;
    }
    printf("USBMASS Z2: present=%lu sectors=%lu MiB=%lu change=%lu state=%lu\n",
           UL(s->present),UL(s->capacity),UL(s->capacity/2048u),
           UL(s->media_change),UL(s->state));
    printf("USBMASS Z2: native USB slot=%lu marker=%08lx (expect 50493452)\n",
           UL(s->reserved[0]),UL(s->reserved[1]));
    if(s->reserved[1]!=0x50493452u) {
        puts("USBMASS Z2: ARM Pi4 mailbox service not active or old kernel.");
    } else if(!s->reserved[0]) {
        puts("USBMASS Z2: no VL805 mass-storage slot configured.");
    } else if(!s->present || !s->capacity) {
        puts("USBMASS Z2: USB BOT slot configured but drive NOT READY.");
        puts("Check HDD power/spin-up, BOT protocol, and R112 POST stage.");
    } else {
        puts("USBMASS Z2: USB drive exposed to AmigaOS. Run usbmass-info-pi4.");
        puts("A filesystem DOSDriver/partition mount is still required for an icon.");
    }
    CloseLibrary(lib);
    return 0;
}
