/* FRF Pi4 R120: Loaded resident vs per-stage OpenDevice diagnostics.
 * Read-only: never sends disk sector requests, no startup/mount changes.
 */
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/io.h>
#include <exec/resident.h>
#include <stdio.h>
static void examine(const char *name)
{
    struct Resident *r=FindResident((char*)name);
    if(r)printf("RESIDENT %s found at %08lx flags=%02x version=%u type=%u\n",
                name,(unsigned long)r,r->rt_Flags,r->rt_Version,r->rt_Type);
    else printf("RESIDENT %s NOT_LOADED_YET\n",name);
}
static void open_test(const char *name)
{
    struct MsgPort *port=CreateMsgPort();
    if(!port) {puts("CreateMsgPort FAILED");return;}
    struct IOStdReq *io=(struct IOStdReq*)CreateIORequest(port,sizeof(struct IOStdReq));
    if(!io) {DeleteMsgPort(port);puts("CreateIORequest FAILED");return;}
    LONG rc=OpenDevice((char*)name,0,(struct IORequest*)io,0);
    printf("OPEN %s -> rc=%ld io_Error=%d reqLen=%u\n",
           name,(long)rc,(int)io->io_Error,(unsigned)io->io_Message.mn_Length);
    if(!rc)CloseDevice((struct IORequest*)io);
    DeleteIORequest((struct IORequest*)io);
    DeleteMsgPort(port);
}
int main(void)
{
    puts("FRF Pi4 R120 production-name device LoadSeg/OpenDevice diagnostic");
    puts("READ ONLY. New v0.4 requires cold reboot to evict any loaded v0.2.");
    BPTR seg=LoadSeg("DEVS:pi4usbmass.device");
    if(seg) {
        puts("LOADSEG DEVS:pi4usbmass.device PASS (can read/relocate Amiga HUNK)");
        UnLoadSeg(seg);
    } else {
        printf("LOADSEG DEVS:pi4usbmass.device FAIL IoErr=%ld\n",
               (long)IoErr());
    }
    examine("pi4usbmass.device");
    open_test("pi4usbmass.device");
    examine("pi4usbmass.device");
    /* A separate test of explicit DEVS: lookup excludes assign issues. */
    open_test("DEVS:pi4usbmass.device");
    puts("Open rc=0 -> usable; -20..-24 -> R120 Open() reached; -1 -> lookup or loader.");
    puts("rc=-1 with no resident -> OS LoadSeg/InitResident/DEV: search failure.");
    puts("rc=-1 with resident -> check load and device vector table.");
    puts("END R120: no sector or FAT95 I/O.");
    return 0;
}
