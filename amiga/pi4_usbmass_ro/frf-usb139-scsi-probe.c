/* R139 AmigaOS read-only PDS/3 DirectSCSI smoke-test on unit0.
 * HD_SCSICMD INQUIRY/READ CAPACITY/READ10, compare CMD_READ sectors.
 * NO WRITE operations, no mounts, no filesystem repair or formatting. */
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/io.h>
#include <devices/trackdisk.h>
#include <devices/scsidisk.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
static UBYTE scbuf[8192] __attribute__((aligned(16)));
static UBYTE tdbuf[8192] __attribute__((aligned(16)));
static UBYTE sense[32] __attribute__((aligned(16)));
static ULONG be32(const UBYTE *p){
 return ((ULONG)p[0]<<24)|((ULONG)p[1]<<16)|((ULONG)p[2]<<8)|p[3];
}
static LONG issue(struct IOStdReq *io,UBYTE *cdb,UWORD cmdlen,void *buf,
                  ULONG size,ULONG sense_size){
 struct SCSICmd sc;
 memset(&sc,0,sizeof(sc));
 sc.scsi_Command=cdb;sc.scsi_CmdLength=cmdlen;
 sc.scsi_Data=(UWORD*)buf;sc.scsi_Length=size;
 sc.scsi_Flags=SCSIF_READ|SCSIF_AUTOSENSE;
 sc.scsi_SenseData=sense;sc.scsi_SenseLength=(UWORD)sense_size;
 io->io_Command=HD_SCSICMD;io->io_Data=&sc;io->io_Length=sizeof sc;
 io->io_Offset=0;io->io_Actual=0;
 DoIO((struct IORequest*)io);
 printf(" SCSI CDB=%02x err=%ld status=%u actual=%lu cmdActual=%u senseActual=%u\n",
  cdb[0],(long)io->io_Error,(unsigned)sc.scsi_Status,
  (unsigned long)sc.scsi_Actual,(unsigned)sc.scsi_CmdActual,
  (unsigned)sc.scsi_SenseActual);
 if(io->io_Error && sc.scsi_SenseActual>=14u)
  printf(" sense key=%02x ASC=%02x\n",sense[2],sense[12]);
 return io->io_Error;
}
int main(void){
 puts("FRF R139 DirectSCSI PDS/3 compatibility probe (READ ONLY)");
 struct MsgPort *port=CreateMsgPort();
 if(!port){puts("CreateMsgPort failed");return 20;}
 struct IOStdReq *io=(struct IOStdReq*)CreateIORequest(port,sizeof(struct IOExtTD));
 if(!io){DeleteMsgPort(port);return 20;}
 LONG err=OpenDevice("usbmass.device",0,(struct IORequest*)io,0);
 if(err){printf("OpenDevice usbmass.device unit0 error=%ld\n",(long)err);
  DeleteIORequest((struct IORequest*)io);DeleteMsgPort(port);return 20;}
 struct DriveGeometry g;
 memset(&g,0,sizeof g);
 io->io_Command=TD_GETGEOMETRY;io->io_Data=&g;io->io_Length=sizeof g;
 io->io_Actual=0;io->io_Offset=0;DoIO((struct IORequest*)io);
 printf(" unit0 geometry err=%ld sectors=%lu sectorBytes=%lu\n",
  (long)io->io_Error,(unsigned long)g.dg_TotalSectors,
  (unsigned long)g.dg_SectorSize);
 UBYTE inq[6]={0x12,0,0,0,36,0};
 if(!issue(io,inq,6,scbuf,36,sizeof sense)){
  printf(" inquiry vendor=%.8s product=%.16s revision=%.4s\n",
         scbuf+8,scbuf+16,scbuf+32);
 }
 UBYTE cap[10]={0x25,0,0,0,0,0,0,0,0,0};
 if(!issue(io,cap,10,scbuf,8,sizeof sense))
  printf(" SCSI lastLBA=%lu blockBytes=%lu\n",
     (unsigned long)be32(scbuf),(unsigned long)be32(scbuf+4));
 UBYTE rd[10]={0x28,0,0,0,0,0,0,0,16,0};
 if(!issue(io,rd,10,scbuf,sizeof scbuf,sizeof sense)){
  io->io_Command=CMD_READ;io->io_Data=tdbuf;io->io_Length=sizeof tdbuf;
  io->io_Offset=0;io->io_Actual=0;DoIO((struct IORequest*)io);
  printf(" CMD_READ err=%ld actual=%lu exactMatch=%s\n",(long)io->io_Error,
    (unsigned long)io->io_Actual,(!io->io_Error &&
      io->io_Actual==sizeof tdbuf && !memcmp(scbuf,tdbuf,sizeof tdbuf))?
      "YES":"NO");
  unsigned rdb=0u;
  for(unsigned b=0;b<16u;b++)
   if(!memcmp(scbuf+b*512u,"RDSK",4u)){
    printf(" RDB found at LBA%u\n",b);++rdb;
   }
  if(!rdb)puts(" RDB not found in LBA0..15; check R138 map/boot media");
 }
 puts("No USB sector writes were requested.");
 CloseDevice((struct IORequest*)io);
 DeleteIORequest((struct IORequest*)io);DeleteMsgPort(port);
 return 0;
}
