/* FRF R121 Pi4 native xHCI SCSI WRITE10 host mock.
 * Executes unmodified ARM frf_pi4_usb_mass.c, with simulated USB wire.
 * No real disk writes. Strictly validates BOT CBW, DMA OUT, CSW.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "frf_xhci_host.h"
#include "frf_usb_mass.h"

volatile struct frf_um_shared *frf_um_host;
volatile uint32_t frf_r76_usb_mass_count;
static struct frf_um_shared mailbox;
static uint8_t cmd[31];
static uint8_t scratch[8192];
static uint32_t fake_slot=5u;
static uint32_t tag,opcode,lba,count,cbw_calls,write_calls,read_calls;
static int fail_data_out,csw_check_condition;
static void must(int b,const char *name){if(!b){fprintf(stderr,"FAIL %s\n",name);exit(2);}}
static uint32_t le32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static void le32_store(uint8_t *p,uint32_t n){p[0]=n;p[1]=n>>8;p[2]=n>>16;p[3]=n>>24;}
static uint32_t be32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
uint32_t frf_xhci_host_msc_slot(void){return fake_slot;}
int frf_xhci_host_bulk_out(const void *data,uint32_t size,uint32_t *actual)
{
    if(size==31u) {
        memcpy(cmd,data,31u);
        must(memcmp(cmd,"USBC",4u)==0,"CBW little endian");
        tag=le32(cmd+4);
        opcode=cmd[15];
        lba=be32(cmd+17);
        count=(uint32_t)cmd[22]<<8|cmd[23];
        must(cmd[13]==0u && cmd[14]==10u,"LUN0 CDB10");
        if(opcode==0x25u || opcode==0x28u)
            must(cmd[12]==0x80u,"READ10 data direction IN");
        else if(opcode==0x2au)
            must(cmd[12]==0u,"WRITE10 data direction OUT");
        else must(0,"unknown SCSI opcode");
        ++cbw_calls;
        if(actual)*actual=size;
        return 1;
    }
    must(opcode==0x2au,"data OUT must follow WRITE10");
    must(size==(count*512u) && le32(cmd+8)==size,"data OUT size matches CBW");
    must(size<=sizeof(scratch),"test scratch size");
    ++write_calls;
    if(fail_data_out) {
        if(actual)*actual=0u;
        return 0;
    }
    memcpy(scratch,data,size);
    if(actual)*actual=size;
    return 1;
}
int frf_xhci_host_bulk_in(void *data,uint32_t size,uint32_t *actual)
{
    uint8_t *b=data;
    if(size==13u) {
        memset(b,0,13u);
        memcpy(b,"USBS",4u);
        le32_store(b+4,tag);
        le32_store(b+8,0u);
        b[12]=csw_check_condition?1u:0u;
    } else if(size==8u && opcode==0x25u) {
        /* 8,192 sectors of 512 bytes */
        uint8_t cap[8]={0,0,0x1f,0xff,0,0,2,0};
        memcpy(b,cap,8u);
    } else {
        must(opcode==0x28u,"data IN must be READ10");
        must(size==count*512u,"READ10 matches CDB");
        memcpy(b,scratch,size);
        ++read_calls;
    }
    if(actual)*actual=size;
    return 1;
}
extern void frf_pi4_um_init(void);
extern void frf_pi4_um_probe_inline(void);
extern void frf_pi4_um_service(void);
extern uint32_t frf_pi4_um_capacity(void);
extern uint32_t frf_pi4_um_last_bot_error(void);
extern uint32_t frf_pi4_um_transport_dead(void);
static void req(uint32_t op,uint32_t lba_,uint32_t sectors,uint32_t seq)
{
    mailbox.state=FRF_UM_REQ;
    mailbox.command=op;
    mailbox.lba=lba_;
    mailbox.sectors=sectors;
    mailbox.sequence=seq;
    frf_pi4_um_service();
    must(mailbox.completed==seq,"mailbox completion sequence");
}
int main(void)
{
    memset(&mailbox,0,sizeof(mailbox));
    mailbox.magic=FRF_UM_MAGIC;
    mailbox.version=FRF_UM_VERSION;
    mailbox.bytes=sizeof(mailbox);
    mailbox.sector_size=FRF_UM_SECTOR_SIZE;
    frf_um_host=&mailbox;
    frf_pi4_um_init();
    frf_pi4_um_probe_inline();
    must(frf_pi4_um_capacity()==8192u,"capacity working");
    frf_pi4_um_service();
    must(mailbox.present==1u,"FRUM ready");
    must(mailbox.reserved[1]==0x50493457u,"PI4W capability marker");
    for(unsigned i=0;i<1024u;++i)mailbox.data[i]=(uint8_t)(i^0xC5u);
    uint8_t snapshot[1024];
    for(unsigned i=0;i<1024u;++i)snapshot[i]=mailbox.data[i];
    req(FRF_UM_WRITE,100u,2u,101u);
    must(mailbox.state==FRF_UM_DONE && mailbox.result==1024u,"WRITE10 succeeded");
    must(opcode==0x2au && lba==100u && count==2u,"WRITE10 LBA big endian");
    must(write_calls==1u && memcmp(scratch,snapshot,1024u)==0,
         "USB OUT payload matches guest staged bytes");
    mailbox.state=FRF_UM_IDLE;
    memset(mailbox.data,0,1024u);
    req(FRF_UM_READ,100u,2u,102u);
    must(mailbox.state==FRF_UM_DONE && mailbox.result==1024u,"READ10 succeeded");
    must(read_calls==1u && !memcmp(mailbox.data,snapshot,1024u),
         "readback matches write payload");

    mailbox.state=FRF_UM_IDLE;
    const unsigned cbw_before=cbw_calls;
    req(FRF_UM_WRITE,8191u,2u,103u);
    must(mailbox.state==FRF_UM_ERROR && cbw_calls==cbw_before,
         "reject out-of-capacity write before USB");
    mailbox.state=FRF_UM_IDLE;
    req(FRF_UM_WRITE,100u,17u,104u);
    must(mailbox.state==FRF_UM_ERROR && cbw_calls==cbw_before,
         "reject oversized write before USB");
    mailbox.state=FRF_UM_IDLE;
    fail_data_out=1;
    req(FRF_UM_WRITE,102u,1u,105u);
    must(mailbox.state==FRF_UM_ERROR && frf_pi4_um_last_bot_error()==12u,
         "failed USB OUT has distinct stage error");
    must(frf_pi4_um_transport_dead()!=0u && frf_pi4_um_capacity()==0u,
         "ambiguous failed write quarantines media");
    const unsigned before_dead=cbw_calls;
    mailbox.state=FRF_UM_IDLE;
    req(FRF_UM_WRITE,103u,1u,106u);
    must(mailbox.state==FRF_UM_ERROR && cbw_calls==before_dead,
         "no retries after ambiguous write");
    puts("PASS R121: SCSI WRITE10 CBW OUT+DATA OUT+CSW IN, 1024-byte readback, bounds, failed write fail-closed");
    return 0;
}
