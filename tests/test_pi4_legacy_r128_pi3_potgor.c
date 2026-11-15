#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "frf_potgor_model.h"
static void require(int cond,const char *msg) {
 if(!cond){fprintf(stderr,"FAIL: %s\n",msg);exit(4);}
}
int main(void)
{
 /* Pi4 R127 suppressed button2 if OUTRY=1; Pi3 RC13 deliberately
  * overrides this so external gamepad button2 always pulls low. */
 const uint16_t potgo_cases[]={0u,0x8000u,0xC000u,0x2000u,0x3000u,
                              0xa000u,0xf000u,0xffffu,0x0800u,0x0200u};
 for(unsigned i=0;i<sizeof(potgo_cases)/sizeof(potgo_cases[0]);i++){
  const uint16_t pg=potgo_cases[i];
  const uint16_t neutral=frf_potgor_model(pg,0u,0u);
  const uint16_t second=frf_potgor_model(pg,0u,0x4000u);
  const uint16_t third=frf_potgor_model(pg,0u,0x1000u);
  const uint16_t both=frf_potgor_model(pg,0u,0x5000u);
  require(!(second&0x4000u),"B2 visible for all Pi3 POTGO modes");
  require(!(both&0x4000u),"B2 remains visible with B3 held");
  require((second&0x1000u)==(neutral&0x1000u),
          "B2 does not spuriously activate B3");
  require((third&0x4000u)==(neutral&0x4000u),
          "B3 does not spuriously activate B2");
  /* Pi3 port2 B3 obeys POTGO OUTRX; input pulled low only when input. */
  if(!(pg&0x2000u))require(!(third&0x1000u),"B3 under input mode");
  else require((third&0x1000u)==(neutral&0x1000u),
               "B3 respects Pi3 POTGO output ownership");
 }
 require((frf_potgor_model(0u,0u,0x4000u)&0x4000u)==0,
         "B2 low in default port2 mode");
 require((frf_potgor_model(0u,0u,0x1000u)&0x1000u)==0,
         "B3 low in default port2 mode");
 require((frf_potgor_model(0u,0u,0x5000u)&0x5000u)==0,
         "B2/B3 both pressed independent");
 require((frf_potgor_model(0x8000u,0u,0x4000u)&0x4000u)==0,
         "regression: B2 not blanked by POTGO 0x8000");
 /* Mouse (port1) pins are separate. */
 require((frf_potgor_model(0u,2u,0u)&0x0400u)==0,
         "Pi3 mouse right button on left-port pin");
 require((frf_potgor_model(0u,4u,0u)&0x0100u)==0,
         "Pi3 mouse middle button on left-port pin");
 puts("PASS: Pi3 exact POTGOR model across 10 POTGO modes, port2 B2 override, B3 direction, mouse");
 return 0;
}
