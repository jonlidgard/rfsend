// rfsend.c
// Send RF commands to devices using the common PT2260 / EV1527 protocols.
//
// Uses one of the built in 200MHZ Programmable Realtime Units (PRU's) 
// of the Sitara processor used on the Beaglebone series of Microcontrollers.
// Based on work done for the Arduino with the rcswitch library.
// (https://github.com/sui77/rc-switch)
//
//
// It uses Linux Userspace IO (uio).
// Make sure uio_pruss module is loaded along with a suitable cape
// (uio can be selected in /boot/uEnv.txt)
// cape_universal will work - use config-pin p8.11 pruout

#include <prussdrv.h>
#include <pruss_intc_mapping.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <argp.h>
#include "rfsendpru_bin.h"

#define HIGH_PULSE 0
#define LOW_PULSE 1

typedef struct Protocol {
  uint16_t pulseLength; // in us
  uint8_t syncFactor[2];
  uint8_t zero[2];
  uint8_t one[2];
  /** @brief if true inverts the high and low logic levels */
  bool invert;
} Protocol;

typedef struct Message {
  uint8_t repeat_count;
  int8_t signal_index;
  uint16_t pulse_length;
  uint8_t gpio_pin;
  uint8_t signal_length;
  uint8_t sub_bit;
  uint32_t command;
  Protocol protocol;
} Message_t;

typedef struct {
  uint32_t result;
  uint32_t syncHigh;
  uint32_t syncLow;
  uint32_t zeroHigh;
  uint32_t zeroLow;
  uint32_t oneHigh;
  uint32_t oneLow;
  uint32_t command;
  uint8_t length;
  uint8_t repeats;
  uint8_t inverted;
  uint8_t spare;
} PrussDataRam_t;


//const uint32_t rc5sockets0313[]={1070396,1070387,1070540,1070531,1070860,1070851,1072396,1072387,1078540,1078531};

static const Protocol proto[] = {
  { 350, {  1, 31 }, {  1,  3 }, {  3,  1 }, false },    // protocol 1
  { 650, {  1, 10 }, {  1,  2 }, {  2,  1 }, false },    // protocol 2
  { 100, { 30, 71 }, {  4, 11 }, {  9,  6 }, false },    // protocol 3
  { 380, {  1,  6 }, {  1,  3 }, {  3,  1 }, false },    // protocol 4
  { 500, {  6, 14 }, {  1,  2 }, {  2,  1 }, false },    // protocol 5
  { 450, { 23,  1 }, {  1,  2 }, {  2,  1 }, true }      // protocol 6 (HT6P20B)
};


const char *argp_program_version =
  "rfsend 1.0";

/* Program documentation. */
static char doc[] =
  "For Beaglebone Microcontrollers only.\nSends a string of commands over RF to control RC5 type switches\
  \vUses PRU0 via the pru_uio overlay.\nEnsure you have an appropriate cape loaded\n\
  (e.g. cape_universal + config-pin p8.11 pruout)\n\
  Connect P8_11 to <DATA> on a cheap 433Mhz transmitter module\n\
  Protocol ID: 1 - PT2260 (350us pulse timing)\n\
  Protocol ID: 2 - ?? (650us pulse timing)\n\
  Protocol ID: 3 - ?? (100us pulse timing)\n\
  Protocol ID: 4 - ?? (380us pulse timing)\n\
  Protocol ID: 5 - ?? (500us pulse timing)\n\
  Protocol ID: 6 - HT6P20B (450us pulse timing)\n\n\
  ** MUST BE RUN WITH ROOT PRIVILEGES **";

/* A description of the arguments we accept. */
static char args_doc[] = "[COMMAND...]";

/* The options we understand. */
static struct argp_option options[] = {
  {"debug", 'd', 0, 0, "Produce debug output" },
  {0,0,0,0, "" },
  {"invert", 'i', 0, 0, "Invert signal" },
  {"repeat", 'r', "COUNT", OPTION_ARG_OPTIONAL,
   "Repeat the output COUNT (default 3) times"},
  {"protocol", 'p', "ID", OPTION_ARG_OPTIONAL,
   "Message protocol id (default 1)"},
  {"time", 't', "MICROSECS", OPTION_ARG_OPTIONAL,
   "Override default protocol pulse timing"},
  {"length", 'l', "BITS", OPTION_ARG_OPTIONAL,
   "Command length (default 24) bits"},
  { 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments
{
  uint32_t time;
  int debug;
  int invert;
  int repeat_count; 
  int protocol_id;
  int length;
  char **commands;
};

/* Parse a single option. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;

  switch (key)
    {
    case 'd':
      arguments->debug = 1;
      break;
	case 'i':
      arguments->invert = 1;
      break;
	case 'r':
      arguments->repeat_count = arg ? atoi (arg) : 3;
	  break;
	case 'p':
      arguments->protocol_id = arg ? atoi (arg) : 1;
	  break;
	case 't':
      arguments->time = arg ? atoi (arg) : 0;
	  break;
	case 'l':
      arguments->length = arg ? atoi (arg) : 24;
	  break;
	      
    case ARGP_KEY_NO_ARGS:
      argp_usage (state);
    
    case ARGP_KEY_ARG:
 	  arguments->commands = &state->argv[state->next-1];
      state->next = state->argc;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc };



int main(int argc, char * argv[])
{
  tpruss_intc_initdata prussIntCInitData = PRUSS_INTC_INITDATA;
  PrussDataRam_t * prussDataRam;
  Message_t msg;
  int ret, i;
  
  
   struct arguments arguments;

  /* Default values. */
  arguments.debug = 0;
  arguments.repeat_count = 3;
  arguments.protocol_id = 1;
  arguments.time = 0;
  arguments.length = 24;
  arguments.invert = -1;
  
  /* Parse our arguments; every option seen by parse_opt will
     be reflected in arguments. */
  argp_parse (&argp, argc, argv, 0, 0, &arguments);
  
	
  // First, initialize the driver and open the kernel device
  prussdrv_init();
  ret = prussdrv_open(PRU_EVTOUT_0);
  
  if(ret != 0) {
    printf("Failed to open PRUSS driver!\n");
    return ret;
  }
    
  // Set up the interrupt mapping so we can wait on INTC later
  prussdrv_pruintc_init(&prussIntCInitData);
  // Map PRU DATARAM; reinterpret the pointer type as a pointer to
  // our defined memory mapping struct. We could also use uint8_t *
  // to access the RAM as a plain array of bytes, or uint32_t * to
  // access it as words.
  prussdrv_map_prumem(PRUSS0_PRU0_DATARAM, (void * *)&prussDataRam);
  // Manually initialize PRU DATARAM - this struct is mapped to the
  // PRU, so these assignments actually modify DATARAM directly.

  msg.protocol = proto[arguments.protocol_id-1];

  uint32_t pulse_length = arguments.time * 100;
  if (pulse_length == 0) 
    pulse_length = msg.protocol.pulseLength * 100; //nanoseconds/10
  
  prussDataRam->syncHigh = msg.protocol.syncFactor[HIGH_PULSE] * pulse_length;
  prussDataRam->syncLow = msg.protocol.syncFactor[LOW_PULSE] * pulse_length;

  prussDataRam->zeroHigh = msg.protocol.zero[HIGH_PULSE] * pulse_length;
  prussDataRam->zeroLow = msg.protocol.zero[LOW_PULSE] * pulse_length;
  
  prussDataRam->oneHigh = msg.protocol.one[HIGH_PULSE] * pulse_length;
  prussDataRam->oneLow = msg.protocol.one[LOW_PULSE] * pulse_length;
  prussDataRam->length = arguments.length;  
  prussDataRam->result = 0;

  if (arguments.invert != -1)
    prussDataRam->inverted = arguments.invert;
  else
    prussDataRam->inverted = msg.protocol.invert;

  for (i = 0; arguments.commands[i]; i++)
  {
  prussDataRam->command = atoi(arguments.commands[i]);
  prussDataRam->repeats = arguments.repeat_count;

  if (arguments.debug)
  {
    printf("Command: %d\n",prussDataRam->command);
  }  
  // Memory fence: not strictly needed here, as compiler will insert
  // an implicit fence when prussdrv_exec_code(...) is called, but
  // a good habit to be in.
  // This ensures that the writes to x, y, sum are fully complete
  // before the PRU code is executed: imagine what kind of painful-
  // to-debug problems you'd see if the compiler or hardware deferred
  // the writes until after the PRU started running!

  __sync_synchronize();

  prussdrv_exec_code(0, prussPru0Code, sizeof prussPru0Code);

  // Wait for INTC from the PRU, signaling it's about to HALT...
  prussdrv_pru_wait_event(PRU_EVTOUT_0);
  // Clear the event: if you don't do this you will not be able to
  // wait again.
  prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);
  // Make absolutely sure we read sum again below, after the PRU
  // writes to it. Otherwise, the compiler or hardware might cache
  // the value we wrote above and just return us that. Again, not
  // actually necessary because the compiler inserts an implicit
  // fence at prussdrv_pru_wait_event(...), but a good habit.
  __sync_synchronize();
  }
  
  // Read the result returned by the PRU
  // Disable the PRU and exit; if we don't do this the PRU may
  // continue running after our program quits! The TI kernel driver
  // is not very careful about cleaning up after us.
  // Since it is possible for the PRU to trash memory and otherwise
  // cause lockups or crashes, especially if it's manipulating
  // peripherals or writing to shared DDR, this is important!
  prussdrv_pru_disable(0);

  prussdrv_exit();

  exit(1);
}
