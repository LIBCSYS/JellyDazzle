#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "jellydazzle.h"
#define W 1280
#define H 960
void PATTERN(uint32_t*, int, int, int, int, uint32_t, const uint32_t*);
static uint32_t fb[W*H], prev[W*H], pal[32768];
int main(int argc, char **argv){
  int scheme = getenv("JD_SCHEME")?atoi(getenv("JD_SCHEME")):0;
  FILE *f=fopen("palette.bin","rb"); fseek(f,(long)scheme*32768L*4L,SEEK_SET);
  fread(pal,4,32768,f); fclose(f);
  int start=atoi(argv[1]), count=atoi(argv[2]);
  for(int fr=start;fr<start+count;fr++){
    memcpy(prev,fb,sizeof fb);
    PATTERN(fb,W,H,fr,fr&2047,0xC0FFEE11u^(uint32_t)(fr>>11),pal);
    if(fr>start){ uint64_t s=0;
      for(int i=0;i<W*H;i+=3){uint32_t c=fb[i],p=prev[i];
        int dr=(int)((c>>16)&255)-(int)((p>>16)&255),dg=(int)((c>>8)&255)-(int)((p>>8)&255),db=(int)(c&255)-(int)(p&255);
        s+=(dr<0?-dr:dr)+(dg<0?-dg:dg)+(db<0?-db:db);}
      double d=(double)s/((W*H/3)*3.0);
      if(d>4.0) printf("f=%d sl=%d d=%.2f\n",fr,fr&2047,d);
    }
  }
  return 0;
}
