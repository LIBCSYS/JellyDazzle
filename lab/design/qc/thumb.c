/* dump a 64x48 RGB thumbnail of every pattern at a fixed scheme+frame */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "jellydazzle.h"
#define PW 256
#define PH 192
extern jd_pattern_fn jd_patterns[];
extern const int jd_pattern_count;
static uint32_t buf[PW*PH], pal[32768];
int main(int argc,char**argv){
  int sch=atoi(argv[1]); int nf=atoi(argv[2]);
  FILE *f=fopen("palette.bin","rb");
  fseek(f,(long)sch*32768L*4L,SEEK_SET); fread(pal,4,32768,f); fclose(f);
  FILE *o=fopen(argv[3],"wb");
  for(int p=0;p<jd_pattern_count;p++){
    memset(buf,0,sizeof buf);
    for(int fr=0;fr<nf;fr++) jd_patterns[p](buf,PW,PH,300+fr,(300+fr)&2047,0xC0FFEE11u,pal);
    for(int y=0;y<48;y++)for(int x=0;x<64;x++){
      long r=0,g=0,b=0;
      for(int j=0;j<4;j++)for(int i=0;i<4;i++){
        uint32_t c=buf[(y*4+j)*PW+(x*4+i)];
        r+=(c>>16)&255; g+=(c>>8)&255; b+=c&255;}
      fputc((int)(r/16),o);fputc((int)(g/16),o);fputc((int)(b/16),o);}
  }
  fclose(o);
  fprintf(stderr,"%d thumbs\n",jd_pattern_count);
  return 0;}
