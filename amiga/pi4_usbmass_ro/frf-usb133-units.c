/* FRF R133 AmigaOS native USB multi-disk and RDB diagnostic.
 * Entirely read-only: enumerates units 0..7 and scans raw LBA0..15 for RDSK.
 * Does NOT mount, partition, repair, format or write any filesystem. */
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/expansion.h>
#include <exec/io.h>
#include <devices/trackdisk.h>
#include <libraries/configvars.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "frf_usb_mass.h"

static unsigned char first16[FRF_UM_MAX_SECTORS*512u] __attribute__((aligned(16)));
static void show_mapping(void){
    struct Library *lib=OpenLibrary("expansion.library",36);
    if(!lib){puts("FRUM: expansion.library unavailable");return;}
    struct ExpansionBase *ExpansionBase=(struct ExpansionBase*)lib;
    struct ConfigDev *cfg=FindConfigDev(NULL,FRF_UM_VENDOR,FRF_UM_PRODUCT);
    if(cfg&&cfg->cd_BoardAddr){
        volatile uint8_t *board=(volatile uint8_t*)cfg->cd_BoardAddr;
        volatile struct frf_um_extension *e=
             (volatile struct frf_um_extension*)(board+FRF_UM_EXT_OFFSET);
        if(e->magic==FRF_UM_EXT_MAGIC && e->version==FRF_UM_EXT_VERSION &&
           e->count<=FRF_UM_MAX_UNITS){
            printf("FRUM R133 v2: %lu logical units\n",(unsigned long)e->count);
            for(unsigned i=0;i<e->count;i++){
                const volatile struct frf_um_ext_unit *u=&e->units[i];
                printf(" unit%u slot%lu LUN%lu present=%lu sectors=%lu flags=%08lx\n",
                    i,(unsigned long)u->slot,(unsigned long)u->lun,
                    (unsigned long)u->present,(unsigned long)u->capacity,
                    (unsigned long)u->flags);
            }
        } else puts("FRUM: no v2 multi-device table (R132 or older kernel?)");
    }else puts("FRUM: Zorro USB mass-storage board not found");
    CloseLibrary(lib);
}
int main(void){
    puts("FRF R133 multi-USB / Amiga RDB probe -- READ ONLY");
    show_mapping();
    struct MsgPort *port=CreateMsgPort();
    if(!port){puts("Unable to create message port");return 20;}
    struct IOStdReq *io=(struct IOStdReq*)CreateIORequest(port,sizeof(struct IOExtTD));
    if(!io){DeleteMsgPort(port);return 20;}
    unsigned found=0u;
    for(unsigned u=0;u<FRF_UM_MAX_UNITS;u++){
        LONG err=OpenDevice("usbmass.device",u,(struct IORequest*)io,0);
        if(err){
            printf(" unit%u OpenDevice not ready (error=%ld)\n",u,(long)err);
            continue;
        }
        ++found;
        struct DriveGeometry g;
        memset(&g,0,sizeof g);
        io->io_Command=TD_GETGEOMETRY;io->io_Data=&g;
        io->io_Length=sizeof g;io->io_Offset=0;
        DoIO((struct IORequest*)io);
        printf(" unit%u: sectorBytes=%lu count=%lu geometryErr=%d\n",u,
            (unsigned long)g.dg_SectorSize,
            (unsigned long)g.dg_TotalSectors,(int)io->io_Error);
        io->io_Command=TD_PROTSTATUS;io->io_Data=NULL;io->io_Length=0;
        DoIO((struct IORequest*)io);
        printf(" unit%u: writeProtected=%lu\n",u,(unsigned long)io->io_Actual);
        if(!io->io_Error && g.dg_SectorSize==512 && g.dg_TotalSectors){
            unsigned n=g.dg_TotalSectors<16u?g.dg_TotalSectors:16u;
            memset(first16,0,n*512u);
            io->io_Command=CMD_READ;io->io_Data=first16;
            io->io_Length=n*512u;io->io_Offset=0u;
            DoIO((struct IORequest*)io);
            if(io->io_Error || io->io_Actual!=n*512u)
                printf(" unit%u: raw READ10 failed error=%d actual=%lu\n",
                    u,(int)io->io_Error,(unsigned long)io->io_Actual);
            else {
                int block=-1;
                for(unsigned b=0;b<n;b++)
                    if(!memcmp(first16+b*512u,"RDSK",4u)){
                        block=(int)b;break;
                    }
                if(block>=0)
                    printf(" unit%u: AMIGA RDB HEADER at LBA%d\n",u,block);
                else
                    printf(" unit%u: no RDSK in first %u sectors; LBA0 %02x %02x %02x %02x\n",
                        u,n,first16[0],first16[1],first16[2],first16[3]);
            }
        }
        CloseDevice((struct IORequest*)io);
    }
    DeleteIORequest((struct IORequest*)io);DeleteMsgPort(port);
    printf("FRF R133: %u ready USB disk units. No writes performed.\n",found);
    return 0;
}
