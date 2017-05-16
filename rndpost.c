#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define URL        "http://test1-proxy-01/ioreg"
#define SN_PREX    "PT=0032&mo=63514W&sn=28:65:6b:00"
#define START_ADDR 40001
#define END_ADDR   40128
#define PT         1
#define MAX_DEV    2048
#define MAX_DELAY  1000000 // 1 sec

extern char *optarg;
int shouldStop = 0;
int retry[MAX_DEV] = {0};

struct DevInfo {
    int cmpIdx;
    int devIdx;
    int numDev;
};

void postData(void* _devInfo)
{
  char sn[48], _sn[18];
  struct DevInfo *devInfo = (struct DevInfo*) _devInfo;
  snprintf(_sn, sizeof _sn, "28:65:6b:00:%02x:%02x", devInfo->cmpIdx, devInfo->devIdx);
  snprintf(sn, sizeof sn, "%s:%02x:%02x&", SN_PREX, devInfo->cmpIdx, devInfo->devIdx);
  // printf("sn=> %s\n", sn);
  // printf("_sn=> %s\n", _sn);

  CURL *curl;
  CURLcode res;
  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if(curl == NULL) {
      curl_global_cleanup();
      return;
  }
  for(;;) {
      if(shouldStop == 1) {
          exit(1);
      }
      char data[2048];
      memset(data, 0, strlen(data));
      strncat(data, sn, strlen(sn));

      for(int addr = START_ADDR; addr < END_ADDR; addr++) {
          char reg[10];
          memset(reg, 0, strlen(reg));
          if(addr == START_ADDR) {
              snprintf(reg, sizeof reg, "%d=%x", addr, (int)(rand() * 1000));
          } else {
              snprintf(reg, sizeof reg, "&%d=%x", addr, (int)(rand() * 1000));
          }
          strncat(data, reg, strlen(reg));
      }
      // printf("data %s\n", data);

      // Curl Options
      curl_easy_setopt(curl, CURLOPT_URL, URL);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
      curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);  /* enable TCP keep-alive for this transfer */
      curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L); /* keep-alive idle time to 120 seconds */
      curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L); /* interval time between keep-alive probes: 60 seconds */

      struct timespec tstart = {0,0}, tend = {0,0};
      clock_gettime(CLOCK_MONOTONIC, &tstart);
      res = curl_easy_perform(curl);
      if(res == CURLE_OK) {
          clock_gettime(CLOCK_MONOTONIC, &tend);
          double rtt = ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);
          printf("SN: %s, RTT: %.5f seconds\n", _sn, rtt);
          retry[devInfo->numDev] = 0;
      } else {
          fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
          if(retry[devInfo->numDev]++ > 6) {
              shouldStop = 1;
          }
      }
      sleep(PT);
  }
  curl_easy_cleanup(curl);
  curl_global_cleanup();
}

void usage(char *argv0)
{
    printf("Usage: %s [-n device] [-s offset]\n", argv0);
}

int main(int argc, char* argv[])
{
  int offset = 0;
	int numCmp = 1;
	int numDev = 1;
	for (;;) {
  		int cnt = getopt(argc, argv, "n:s:");
  		if (cnt == EOF) { 
          break;
      }
  		switch (cnt) {
      		case 'n': {
      		  numDev = atoi(optarg);
            numDev = (numDev >= MAX_DEV) ? MAX_DEV : numDev;
      			break;
          }
          case 's': {
            offset = atoi(optarg);
            break;
          }          
      		default: { 
      			usage(argv[0]);
      			exit(1);
          }
  		}
	}
  numDev += offset;
  numCmp = numDev / 100 + 1;
  
  int delay;
  pthread_t pid[numDev];
  for(int c = 0; c < numCmp; c++) {
      int maxDdev = (c == (numCmp - 1)) ? (numDev % 100) : 100;
  		for(int i = 0; i < maxDdev; i++) {
          int devIdx = c * 100 + i;
          printf("c %d, i %d, devIdx %d\n", c, i, devIdx);
          if(devIdx < offset) {
              continue;
          }

          struct DevInfo *devInfo;
          devInfo = malloc(sizeof(struct DevInfo));
          devInfo->cmpIdx = c;
          devInfo->devIdx = i;
          devInfo->numDev = devIdx;

    			int ret = pthread_create(&pid[devIdx], NULL, (void *) postData, (void *) devInfo);
    			if(ret != 0) {
      		    printf("Create pthread error!\n");
      				exit(1);
    			}

          delay = devIdx * 10 * 1000;
    			printf("Comp %d, Dev %d, Total %d, Delay %d\n", c, i, devIdx, delay);
          delay = (delay >= MAX_DELAY) ? MAX_DELAY : delay;
          usleep(delay);
  		}
	}
  for(int i = offset; i < numDev; i++) {
      pthread_join(pid[i], NULL);
  }
  exit(0);
}
