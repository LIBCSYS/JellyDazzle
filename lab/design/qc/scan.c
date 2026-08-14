/* scan.c — for every C pattern, measure luma sigma / coverage at several
 * palette schemes, at probe resolution. Flags flat grounds. */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "jellydazzle.h"
#define PW 320
#define PH 240
extern jd_pattern_fn jd_patterns[];
extern const int jd_pattern_count;
static uint32_t buf[PW*PH], pal[32768];
int main(int argc,char**argv){
  int nsch = argc-1;
  int sch[16]; for(int i=0;i<nsch;i++) sch[i]=atoi(argv[i+1]);
  FILE *f=fopen("palette.bin","rb");
  printf("pat  cover ");
  for(int k=0;k<nsch;k++) printf(" s%-3d ", sch[k]);
  printf("  minSig\n");
  for(int p=0;p<jd_pattern_count;p++){
    double sig[16]; double cov=0; double mins=1e9;
    for(int k=0;k<nsch;k++){
      fseek(f,(long)sch[k]*32768L*4L,SEEK_SET); fread(pal,4,32768,f);
      memset(buf,0,sizeof buf);
      for(int fr=0;fr<90;fr++)
        jd_patterns[p](buf,PW,PH,300+fr,(300+fr)&2047,0xC0FFEE11u,pal);
      double s=0,s2=0; long c=0;
      for(int i=0;i<PW*PH;i++){uint32_t z=buf[i];
        double l=(((z>>16)&255)*0.299+((z>>8)&255)*0.587+(z&255)*0.114);
        s+=l;s2+=l*l; if(l>16)c++;}
      double m=s/(PW*PH), v=s2/(PW*PH)-m*m; sig[k]=v>0?sqrt(v):0;
      if(k==0) cov=(double)c/(PW*PH);
      if(sig[k]<mins) mins=sig[k];
    }
    printf("%03d  %.3f ",p+1,cov);
    for(int k=0;k<nsch;k++) printf(" %5.1f",sig[k]);
    printf("   %5.1f\n",mins);
    fflush(stdout);
  }
  return 0;}
