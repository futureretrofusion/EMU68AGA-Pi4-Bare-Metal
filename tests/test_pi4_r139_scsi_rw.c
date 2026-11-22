/* R139 SCSI Direct CDB pure decode unit test, no device/media writes. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "frf_r139_scsi_rw.h"
int main(void){
 struct FrfScsiRW r;
 uint8_t read10[10]={0x28,0x00,0x00,0x12,0x34,0x56,0,0,16,0};
 assert(frf_r139_scsi_rw(read10,10,&r)==1);
 assert(r.offset==((uint64_t)0x123456)*512 && r.bytes==8192 && !r.writing);
 uint8_t write10[10]={0x2a,0,0,0,0,1,0,0,1,0};
 assert(frf_r139_scsi_rw(write10,10,&r)==1 && r.offset==512u && r.bytes==512u && r.writing);
 uint8_t read6[6]={0x08,0x01,0x02,0x03,0,0};
 assert(frf_r139_scsi_rw(read6,6,&r)==1 && r.bytes==256u*512u && !r.writing);
 assert(r.offset==((uint64_t)0x010203u)*512u);
 uint8_t write6[6]={0x0a,0,0,1,1,0};
 assert(frf_r139_scsi_rw(write6,6,&r)==1 && r.bytes==512u && r.writing);
 uint8_t read12[12]={0xa8,0,0x40,0x00,0x00,0x00,0,0,0,16,0,0};
 assert(frf_r139_scsi_rw(read12,12,&r)==1 && r.offset==((uint64_t)0x40000000u)*512u && r.bytes==8192u);
 uint8_t bad12[12]={0xa8,0,0,0,0,0,0,0x80,0,0,0,0};
 assert(frf_r139_scsi_rw(bad12,12,&r)==-1);
 uint8_t invalid[10]={0x28,0,0,0,0,0,0,0,1,0};
 assert(frf_r139_scsi_rw(invalid,8,&r)==-1);
 uint8_t inquiry[6]={0x12,0,0,0,36,0};
 assert(frf_r139_scsi_rw(inquiry,6,&r)==0);
 uint8_t zero[10]={0x28,0,0,0,0,0,0,0,0,0};
 assert(frf_r139_scsi_rw(zero,10,&r)==1 && r.bytes==0);
 assert(frf_r139_scsi_rw(NULL,10,&r)==-1);
 puts("PASS R139: SCSI READ6/10/12 WRITE6/10 decode, high LBAs, zero length, invalid CDBs");
 return 0;
}
