#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#define W 1280
#define H 960
extern void jd_frame(uint32_t*, int, int, int);
static uint32_t fb[W*H];
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec*1000.0+t.tv_nsec/1e6;}
int main(int argc,char**argv){
  int n = argc>1?atoi(argv[1]):3400;
  for(int f=0;f<n;f++){double a=now();jd_frame(fb,W,H,f);double e=now()-a;
    if(e>20.0) printf("SLOW f=%d %.1f ms\n",f,e);}
  return 0;}
