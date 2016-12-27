#include <stdio.h>
#include <time.h>

static unsigned int randseed;  
/* 实现伪随机数的支持 */  
unsigned int Curl_rand(void)  
{  
  unsigned int r;  
  /* 返回一个无符号32位整型的伪随机数. */  
  r = randseed = randseed * 1103515245 + 12345;  
  return (r << 16) | ((r >> 16) & 0xFFFF);  
}  
  
void Curl_srand(void)  
{  
  /* 产生随机的伪随机数序列。 */  
  randseed = (unsigned int) time(NULL);  
  Curl_rand();  
  Curl_rand();  
  Curl_rand();  
}  
  
int main(void)  
{  
    Curl_srand();  
    unsigned int i ;  
    int j = 10;  
    printf("产生10个随机数：\n");  
    while(j != 0){  
        i = Curl_rand() % 100 ;  
        printf("i:%d\n",i);  
        j-- ;  
    }  
    return 0 ;   
}   

