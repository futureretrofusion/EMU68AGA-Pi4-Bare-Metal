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

/* R137 readonly: enumerate actual RDB partition and filesystem metadata.
 * Read only, no RDB modifications, no PFS3 mount or recovery attempts. */
static uint8_t r137_block[512u] __attribute__((aligned(16)));
static uint32_t r137_be32(const uint8_t *b,unsigned n){
    return ((uint32_t)b[n]<<24)|((uint32_t)b[n+1]<<16)|
           ((uint32_t)b[n+2]<<8)|(uint32_t)b[n+3];
}
static int r137_read(struct IOStdReq *io,uint32_t lba,uint32_t capacity){
    if(lba>=capacity || lba>65535u)return 0; /* RDB chains always in reserved sectors */
    memset(r137_block,0,sizeof r137_block);
    io->io_Command=CMD_READ;
    io->io_Data=r137_block;
    io->io_Length=512u;
    io->io_Offset=lba*512u;
    io->io_Actual=0;
    DoIO((struct IORequest*)io);
    return !io->io_Error && io->io_Actual==512u;
}
static int r137_chain_valid(uint32_t n,uint32_t capacity){
    return n!=0xffffffffu && n<capacity && n<65536u;
}
static void r137_inspect(struct IOStdReq *io,unsigned unit,
                          unsigned root_block,uint32_t capacity){
    if(!r137_read(io,root_block,capacity) ||
       memcmp(r137_block,"RDSK",4u)){
        printf(" unit%u: RDB reread failed\n",unit);return;
    }
    const uint32_t sector_bytes=r137_be32(r137_block,0x10u);
    const uint32_t part_head=r137_be32(r137_block,0x1cu);
    const uint32_t fs_head=r137_be32(r137_block,0x20u);
    printf(" unit%u: RDSK blockBytes=%lu PART=%lu FSHD=%lu\n",
           unit,(unsigned long)sector_bytes,(unsigned long)part_head,
           (unsigned long)fs_head);
    if(sector_bytes!=512u){
        puts("  R137: unsupported RDB block size; not walking chains");
        return;
    }
    uint32_t ptr=part_head;
    for(unsigned j=0u;j<24u && r137_chain_valid(ptr,capacity);j++){
        if(!r137_read(io,ptr,capacity) ||
           memcmp(r137_block,"PART",4u)){
            printf("  PART[%u] invalid/read failure at LBA%lu\n",
                   j,(unsigned long)ptr);break;
        }
        char name[32u];
        unsigned len=r137_block[0x24u];
        if(len>31u)len=31u;
        memcpy(name,&r137_block[0x25u],len);name[len]=0;
        uint32_t env=0x80u;
        uint32_t surfaces=r137_be32(r137_block,env+3u*4u);
        uint32_t bpt=r137_be32(r137_block,env+5u*4u);
        uint32_t low=r137_be32(r137_block,env+9u*4u);
        uint32_t high=r137_be32(r137_block,env+10u*4u);
        uint32_t maxtransfer=r137_be32(r137_block,env+13u*4u);
        uint32_t dos=r137_be32(r137_block,env+16u*4u);
        uint32_t bootpri=r137_be32(r137_block,env+15u*4u);
        unsigned long long begin=(unsigned long long)low*surfaces*bpt;
        unsigned long long end=((unsigned long long)high+1u)*surfaces*bpt;
        printf("  PART[%u] %s LBA%lu type=%08lx low/high=%lu/%lu geom=%luH/%luS pri=%ld\n",
             j,name,(unsigned long)ptr,(unsigned long)dos,
             (unsigned long)low,(unsigned long)high,
             (unsigned long)surfaces,(unsigned long)bpt,(long)bootpri);
        printf("    raw LBA %llu..%llu maxTransfer=%08lx%s\n",
             begin,end?end-1u:0u,(unsigned long)maxtransfer,
             end>capacity?" *** EXCEEDS USB UNIT CAPACITY ***":"");
        if(dos==0x50445303u)
            puts("    PDS/3 DirectSCSI: usbmass.device 0.6 does not implement HD_SCSICMD");
        else if(dos==0x50465303u)
            puts("    PFS/3 TD64: check mountlist geometry, FS driver and read-only state");
        else if((dos&0xffffff00u)==0x50465300u)
            puts("    PFS variant: check exact PFS filesystem version");
        uint32_t next=r137_be32(r137_block,0x10u);
        if(next==ptr){puts("    CYCLIC PART chain");break;}
        ptr=next;
    }
    ptr=fs_head;
    for(unsigned j=0u;j<24u && r137_chain_valid(ptr,capacity);j++){
        if(!r137_read(io,ptr,capacity) ||
           memcmp(r137_block,"FSHD",4u)){
            printf("  FSHD[%u] invalid/read failure at LBA%lu\n",
                   j,(unsigned long)ptr);break;
        }
        uint32_t dos=r137_be32(r137_block,0x20u);
        uint32_t ver=r137_be32(r137_block,0x24u);
        uint32_t seg=r137_be32(r137_block,0x48u);
        printf("  FSHD[%u] LBA%lu type=%08lx version=%lu.%lu LSEG=%lu\n",
             j,(unsigned long)ptr,(unsigned long)dos,
             (unsigned long)(ver>>16),(unsigned long)(ver&65535u),
             (unsigned long)seg);
        uint32_t next=r137_be32(r137_block,0x10u);
        if(next==ptr){puts("    CYCLIC FSHD chain");break;}
        ptr=next;
    }
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
                if(block>=0){
                    printf(" unit%u: AMIGA RDB HEADER at LBA%d\n",u,block);
                    r137_inspect(io,u,(unsigned)block,g.dg_TotalSectors);
                }
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
