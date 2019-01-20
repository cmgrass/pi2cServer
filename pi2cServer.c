#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <bcm2835.h> /* RPI IO Access Lib */

/* Status */
#define SUCCESS           0
#define EC_SIGINT       490
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
  return;
}

void iic_master_init()
{
  CMG_PRINT(printf("[iic_master_init] First, setup bcm2835 interface\n"));
  if (!bcm2835_init()) {
    die("[iic_master_init] Could not access bcm2835 IO", EC_IIC_INITFAIL); 
  } else {
    CMG_PRINT(printf("[iic_master_init] bcm2835 init SUCCESS\n"));
  }

  CMG_PRINT(printf("[iic_master_init] Second, setup i2c pins\n"));
  if (!bcm2835_i2c_begin()) {
    die("[iic_master_init] Could not setup i2c hardware.\nPlease run as root",
        EC_IIC_INITFAIL);
  } else {
    CMG_PRINT(printf("[iic_master_init] i2c setup SUCCESS\n"));
  }
  return;
}

void iic_master_close()
{
  CMG_PRINT(printf("[iic_master_close] dispose memory, disconnect bcm2835\n"));
  if (!bcm2835_close()) {
    printf("[iic_master_close] Error closing bcm2835 IO:%x\n", status);
  } else {
    CMG_PRINT(printf("[iic_master_close] bcm2835_close SUCCESS\n"));
  }
  return;
}

static void sigintHandler(int signum)
{
  iic_master_close();
  die("User requested program interrupt", EC_SIGINT);
  return;
}

void setupSignals()
{
  struct sigaction sigAct;
  struct sigaction *sigAct_p;

  sigAct_p = &sigAct;
  memset(sigAct_p, 0, sizeof(struct sigaction));
  sigAct_p->sa_handler = &sigintHandler;

  if(sigaction(SIGINT, sigAct_p, NULL)) {
    die("Could not setup SIGINT", EC_SIGINT);
  } else {
    CMG_PRINT(printf("[setupSignals] SIGINT SUCCESS\n"));
  }
  return;
}

ERROR bacon()
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
      bacon();
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
  ERROR status;

  status = SUCCESS;

  CMG_PRINT(printf("Hi chris\n"));

  /* Route signals to proper handlers */
  setupSignals();

  /* Initialize i2c */
  iic_master_init();

  /* Main task entry point */
  status = taskMain();

  /* SHOULD NEVER GET HERE: cleanup just incase */
  iic_master_close();

  return status;
}
