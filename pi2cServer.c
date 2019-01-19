#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <bcm2835.h> /* RPI IO Access Lib */

/* Status */
#define SUCCESS 0
#define EC_IIC_INITFAIL 501

/* Constants */
#define SLEEP_MS 0x1F4

/* Types */
#define ERROR unsigned int

/* Debug */
#define CMG_DBG 1

#ifdef CMG_DBG
#define CMG_PRINT(x) x
#else
#define CMG_PRINT(x) do {} while (0)
#endif

/* helpers */
void die(char *dieMsg, ERROR dieCode)
{
  printf("exiting:%x %s\n", dieCode, dieMsg);
  exit(1);
}

ERROR iic_master_init()
{
  CMG_PRINT(printf("[iic_master_init] First, setup bcm2835 interface\n"));
  if (!bcm2835_init()) {
    die("could not access bcm2835 IO", EC_IIC_INITFAIL); 
  } else {
    CMG_PRINT(printf("[iic_master_init] bcm2835 init SUCCESS\n"));
  }
  return SUCCESS;
}

ERROR iic_master_close()
{
  ERROR status;

  CMG_PRINT(printf("[iic_master_close] dispose memory, disconnect bcm2835\n"));
  if (!(status = bcm2835_close())) {
    printf("[iic_master_close] Error closing bcm2835 IO:%x\n", status);
  } else {
    CMG_PRINT(printf("[iic_master_close] bcm2835_close SUCCESS\n"));
  }
  return status;
}

static void sigintHandler(int signum)
{
  ERROR status;

  status = 0;

  status = iic_master_close();
  die("User requested program interrupt", status);

  return;
}

void setupSignals()
{
  ERROR status;
  struct sigaction sigAct;
  struct sigaction *sigAct_p;

  sigAct_p = &sigAct;
  memset(sigAct_p, 0, sizeof(struct sigaction));
  sigAct_p->sa_handler = &sigintHandler;

  if(status = sigaction(SIGINT, sigAct_p, NULL)) {
    die("Could not setup SIGINT", status);
  } else {
    CMG_PRINT(printf("[setupSignals] SIGINT SUCCESS\n"));
  }
  return;
}

ERROR bacom()
{
  CMG_PRINT(printf("foijfowe\n"));
  return SUCCESS;
}

ERROR taskMain ()
{
  ERROR status;
  ERROR sleepStat;
  unsigned int loopVal;
  struct timespec timeReq;
  struct timespec *timeReq_p;

  status = 0;
  loopVal = 0;

  sleepStat = 0;
  timeReq_p = &timeReq;
  timeReq_p->tv_sec = 0;
  timeReq_p->tv_nsec = (1000 * 1000 * SLEEP_MS);

  while (1) {
    CMG_PRINT(printf("loopVal:%x\n", loopVal));
    if (loopVal > 0xF) {
      CMG_PRINT(printf("really?\n"));
      bacom();
      loopVal = 0;
    } else {
      loopVal += 1;
    }
    sleepStat = nanosleep(timeReq_p, NULL); /* suspend thread */
  }

  return SUCCESS;
}
/* Main Routine */
int main (int argc, char *argv[])
{
  ERROR status = 0;
  CMG_PRINT(printf("Hi chris\n"));

  /* Route signals to proper handlers */
  setupSignals();

  /* Main task entry point */
  status = taskMain();

  CMG_PRINT(printf("We're done:%x\n", status));
  return status;
}
