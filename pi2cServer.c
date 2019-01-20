#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>
#include <bcm2835.h> /* RPI IO Access Lib */

/* Status */
#define SUCCESS           0
#define EC_SIGINT       490
#define EC_IIC_INITFAIL 501
#define EC_IIC_MALLOC   505
#define EC_IIC_INVAL    510

/* Constants */
#define SLEEP_MS 0x1F4
#define IIC_MAX_SLAVES 1
#define IIC_SLAVE_CLOSED 0
#define IIC_SLAVE_ACTIVE 1

/* Types */
#define ERROR unsigned int

typedef struct iic_slave_t {
  ERROR status;
  uint8_t state;
  struct iic_slave_t *root_slave_p; /* typedef not okay in itself, use struct */
  struct iic_slave_t *next_slave_p;
} iic_slave_t;

/* Globals */
/*
 * NOTE: `static` in this context declares "file" global scope"
 *
 * BE CAREFUL with assignments and accesses, who knows where these are used.
 *
*/
static iic_slave_t *slaves_p; /* Global so we can cleanup memory from signal handlers */

/* Debug */
#define DO_DEBUG 1

#ifdef DO_DEBUG
#define CMG_DBG(x) x
#else
#define CMG_DBG(x) do {} while (0)
#endif

/* prototypes */
void die(char *dieMsg, ERROR dieCode);

static void handle_sigint(int signum);

void route_signals();

void iic_master_init();

void iic_master_quit();

ERROR iic_alloc_slaves(iic_slave_t **slaves_p);

void iic_free_slaves(iic_slave_t **slaves_p);

ERROR iic_process_slaves();

ERROR iic_main();

/* routines */
void die(char *dieMsg, ERROR dieCode)
{
  printf("exiting:%x %s\n", dieCode, dieMsg);
  exit(1);
  return;
}

static void handle_sigint(int signum)
{
  iic_master_quit();
  die("User requested program interrupt", EC_SIGINT);
  return;
}

void route_signals()
{
  struct sigaction sigAct;
  struct sigaction *sigAct_p;

  sigAct_p = &sigAct;
  memset(sigAct_p, 0, sizeof(struct sigaction));
  sigAct_p->sa_handler = &handle_sigint;

  if(sigaction(SIGINT, sigAct_p, NULL)) {
    die("Could not setup SIGINT", EC_SIGINT);
  } else {
    CMG_DBG(printf("[route_signals] SIGINT SUCCESS\n"));
  }
  return;
}

void iic_master_init()
{
  CMG_DBG(printf("[iic_master_init] Setup bcm2835 interface\n"));
  if (!bcm2835_init()) {
    die("[iic_master_init] Could not access bcm2835 IO", EC_IIC_INITFAIL); 
  } else {
    CMG_DBG(printf("[iic_master_init] bcm2835 init SUCCESS\n"));
  }

  CMG_DBG(printf("[iic_master_init] Setup i2c pins\n"));
  if (!bcm2835_i2c_begin()) {
    die("[iic_master_init] FAILED setting up i2c hardware.\nPlease run as root",
        EC_IIC_INITFAIL);
  } else {
    CMG_DBG(printf("[iic_master_init] i2c setup SUCCESS\n"));
  }
  return;
}

void iic_master_quit()
{
  iic_free_slaves(&slaves_p);

  bcm2835_i2c_end(); /* May be redundant since we're closing bcm2835 next */

  CMG_DBG(printf("[iic_master_quit] Disconnect bcm2835\n"));
  if (!bcm2835_close()) {
    printf("[iic_master_quit] Error closing bcm2835 IO\n");
  } else {
    CMG_DBG(printf("[iic_master_quit] bcm2835_close SUCCESS\n"));
  }

  return;
}

ERROR iic_alloc_slaves(iic_slave_t **slaves_p)
/* Returns number of slaves allocated */
/* Kills program on error */
{
  ERROR status;
  uint8_t idx;
  iic_slave_t *curr_p;
  iic_slave_t *last_p;

  status = SUCCESS;

  if (IIC_MAX_SLAVES > UINT8_MAX) {
    CMG_DBG(printf("[iic_alloc_slaves] IIC_MAX_SLAVES:%x exceed type:%x\n",
           IIC_MAX_SLAVES, UINT8_MAX));
    status = EC_IIC_INVAL;
  } else if (IIC_MAX_SLAVES < 1) {
    CMG_DBG(printf("[iic_alloc_slaves] IIC_MAX_SLAVES:%x too small\n",
              IIC_MAX_SLAVES));
    status = EC_IIC_INVAL;
  }

  if (status) {
    die("Fatal parameter error (IIC_MAX_SLAVES)", status);
  }

  /* Make sure die always occurs above on error */
  *slaves_p = NULL;
  curr_p = NULL;
  last_p = NULL;

  for (idx = 0; idx < IIC_MAX_SLAVES; idx++) {
    /* allocate a new structure */
    curr_p = malloc(sizeof(iic_slave_t));
    if (!curr_p) {
      CMG_DBG(printf("[iic_alloc_slaves] could not allocate slave struct\n"));
      die("Fatal memory allocation", EC_IIC_MALLOC);
    } else {
      memset(curr_p, 0, sizeof(iic_slave_t));
    }

    CMG_DBG(printf("[iic_alloc_slaves] allocated slave:%lx\n", curr_p));

    /* attach to linked list */
    if (idx == 0) {
      *slaves_p = curr_p;
      (*slaves_p)->root_slave_p = curr_p;
    } else {
      if (!last_p) {
        CMG_DBG(printf("[iic_alloc_slaves] slave setup pointer invalid\n"));
        die("Fatal slave_p error", EC_IIC_INVAL);
      } else {
        last_p->next_slave_p = curr_p;
      }
    }
    last_p = curr_p;
  }
  CMG_DBG(printf("[iic_alloc_slaves] allocated %x slaves starting @:%lx\n",
            idx, *slaves_p));
  return idx;
}

void iic_free_slaves(iic_slave_t **slaves_p)
{
  iic_slave_t *curr_p;
  iic_slave_t *next_p;

  curr_p = NULL;
  next_p = NULL;

  curr_p = *slaves_p;
  while (curr_p != NULL) {
    next_p = curr_p->next_slave_p;
    CMG_DBG(printf("[iic_free_slaves] freeing slave:%lx, next:%lx\n",
              curr_p, next_p));
    free(curr_p);
    curr_p = next_p;
  }

  *slaves_p = NULL;

  return;
}

ERROR iic_process_slaves()
{
  CMG_DBG(printf("process i2c slaves\n"));
  return SUCCESS;
}

ERROR iic_main()
{
  ERROR status;
  ERROR sleepStat;
  unsigned int loopVal;
  struct timespec timeReq;
  struct timespec *timeReq_p;

  status = 0;
  loopVal = 0;

  /* setup sleep timer */
  sleepStat = 0;
  timeReq_p = &timeReq;
  timeReq_p->tv_sec = 0;
  timeReq_p->tv_nsec = (1000 * 1000 * SLEEP_MS);

  status = iic_alloc_slaves(&slaves_p);

  /* main loop */
  status = 0;

  while (1) {
    CMG_DBG(printf("loopVal:%x\n", loopVal));
    if (loopVal > 0xF) {
      status = iic_process_slaves();
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

  CMG_DBG(printf("Hi chris\n"));

  /* Register system signals to our handlers */
  route_signals();

  /* Initialize i2c */
  iic_master_init();

  /* Main task entry point */
  status = iic_main();

  /* --- SHOULD NEVER GET HERE ---
   * cleanup just incase
  */
  iic_master_quit();
  return status;
}
