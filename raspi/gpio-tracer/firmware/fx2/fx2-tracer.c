/**
 * Copyright (C) 2009 Ubixum, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 * This firmware builds on the bulkloop example from external/fx2lib.
 * Data is sampled by continuesly sampling the slave FIFO data lines using a two state
 * GPIF Waveform. The Waveform data is generated using the FX2 GPIF Designer Utility.
 **/
/* #include <stdio.h> */

#include <i2c.h>
#include <fx2regs.h>
#include <fx2macros.h>
/* #include <serial.h> */
#include <delay.h>
#include <autovector.h>
#include <lights.h>
#include <setupdat.h>
#include <eputils.h>
#include <gpif.h>

/* #define DEBUG_PROG 1 */

#ifdef DEBUG_PROG
#define debug_printf(...) printf(__VA_ARGS__)
#else
#define debug_printf(...)
#endif


#define SYNCDELAY SYNCDELAY16
#define REARMVAL 0x80
#define REARMEP2()    EP2BCL=REARMVAL
#define REARMEP1OUT() EP1OUTBC=REARMVAL // it does not matter which value is written here, so just reuse REARMVAL

#define ENABLE_IBN() NAKIE |= bmIBN; // Ping & combined IN-BULK_NAK (IBN) interrupt enable
#define ENABLE_EP2IBN() IBNIE |= bmEP2IBN; // Inidividual IBN interrupt enable
#define DISABLE_EPxIBN() IBNIE = 0x00;

static struct __spi_conf {
  unsigned char CLK;
  unsigned char MISO;
  unsigned char MOSI;
  unsigned char CS;
} soft_spi_configuration = {0};

enum tracer_status_codes {
  TRACER_GPIF_SETUP = 0x00,
  TRACER_GPIF_READY = 0x01,
  TRACER_GPIF_STOPPED = 0x02,
  TRACER_GPIF_RUNNING = 0x03,
};

void setup_soft_spi_ports();
unsigned char soft_spi_write(unsigned char *write_buf, unsigned char *read_buf, unsigned char length);
void set_gpif_delay(unsigned char delay);
unsigned char get_gpif_delay();
char gpif_poll_finished();

const char __xdata testdata[] = { 0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xf8, 0x80, 0x98, 0x88, 0x83, 0xc6, 0xa1, 0x86, 0x8e  };

enum tracer_status_codes __xdata tracer_state = TRACER_GPIF_SETUP;

// Auto generated by GPIF Designer
const char __xdata WaveData[128] =
{
// Wave 0
/* LenBr */ 0x01,     0x01,     0x01,     0x01,     0x01,     0x01,     0x01,     0x07,
/* Opcode*/ 0x00,     0x00,     0x00,     0x00,     0x00,     0x00,     0x00,     0x00,
/* Output*/ 0x00,     0x00,     0x00,     0x00,     0x00,     0x00,     0x00,     0x00,
/* LFun  */ 0x00,     0x00,     0x00,     0x00,     0x00,     0x00,     0x00,     0x3F,
// Wave 1
/* LenBr */ 0x01,     0x01,     0x01,     0x01,     0x01,     0x01,     0x01,     0x07,
/* Opcode*/ 0x00,     0x00,     0x00,     0x00,     0x00,     0x00,     0x00,     0x00,
/* Output*/ 0x00,     0x00,     0x00,     0x00,     0x00,     0x00,     0x00,     0x00,
/* LFun  */ 0x00,     0x00,     0x00,     0x00,     0x00,     0x00,     0x00,     0x3F,
// Wave 2
/* LenBr */ 0x05,     0xB8,     0x01,     0x01,     0x01,     0x01,     0x01,     0x07,
/* Opcode*/ 0x02,     0x01,     0x00,     0x00,     0x00,     0x00,     0x00,     0x00,
/* Output*/ 0x00,     0x00,     0x00,     0x00,     0x00,     0x00,     0x00,     0x00,
/* LFun  */ 0x00,     0x36,     0x00,     0x00,     0x00,     0x00,     0x00,     0x3F,
// Wave 3
/* LenBr */ 0x01,     0x01,     0x01,     0x01,     0x01,     0x01,     0x01,     0x07,
/* Opcode*/ 0x00,     0x00,     0x00,     0x00,     0x00,     0x00,     0x00,     0x00,
/* Output*/ 0x00,     0x00,     0x00,     0x00,     0x00,     0x00,     0x00,     0x00,
/* LFun  */ 0x00,     0x00,     0x00,     0x00,     0x00,     0x00,     0x00,     0x3F,
};

// set delay for wave
#define SET_WAVE_DELAY(wave, state, delay) WaveData[32 * wave + state] = delay;

const char __xdata FlowStates[36] =
{
/* Wave 0 FlowStates */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/* Wave 1 FlowStates */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/* Wave 2 FlowStates */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/* Wave 3 FlowStates */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

const char __xdata InitData[7] =
{
/* Regs  */ 0xC0,0x00,0x00,0x00,0xEE,0xAA,0x00
};

volatile __bit got_sud;
__bit on;

void setup() {
  REVCTL=0; // not using advanced endpoint controls

  d2off();
  on=0;
  got_sud=FALSE;

  // renumerate
  RENUMERATE_UNCOND();


  SETCPUFREQ(CLK_48M);
  SETIF48MHZ();
  /* sio0_init(57600); */

  USE_USB_INTS();
  ENABLE_SUDAV();
  ENABLE_IBN();
  ENABLE_EP2IBN();
  ENABLE_EP1OUT();
  ENABLE_SOF();
  ENABLE_HISPEED();
  ENABLE_USBRESET();


  // we use EndPoint 2 for bulk transfers to the host
  EP2CFG = bmSIZE | bmTYPE1 | bmDIR | bmVALID | bmBUF;
  SYNCDELAY; //
  EP1INCFG = bmVALID | bmTYPE1;
  SYNCDELAY;
  EP1OUTCFG = bmVALID | bmTYPE1;
  SYNCDELAY;
  // disable all other endpoints
  EP6CFG &= ~bmVALID;
  SYNCDELAY;
  EP4CFG &= ~bmVALID;
  SYNCDELAY;
  EP8CFG &= ~bmVALID;
  SYNCDELAY;


  // TODO is this needed?
  RESETFIFO(0x02);

  // make it so we enumerate
  EA=1; // global interrupt enable
  debug_printf ( "Done initializing stuff\n" );
}

char gpif_abort_transaction() {
  SYNCDELAY;
  GPIFABORT = 0xFF;
  SYNCDELAY;
  RESETTOGGLE(0x82);
  SYNCDELAY;
  RESETFIFO(0x02);
  SYNCDELAY;

  if(gpif_poll_finished()) {
    tracer_state = TRACER_GPIF_STOPPED;
  }

  return 0;
}


// -- Auto generated Code by GPIF Designer Utility --
void _gpif_init( void )
{
  IFCONFIG =
    bmIFCLKSRC     |  // IFCLKSRC=1   , FIFOs executes on internal clk source
    bm3048MHZ      |  // xMHz=1       , 48MHz internal clk rate
    bmASYNC        |  // ASYNC=1      , master samples asynchronous
    bmGSTATE       |  // GSTATE=1     , Drive GPIF states out on PORTE[2:0], debug WF
    bmIFGPIF;     // IFCFG[1:0]=10, FX2 in GPIF master mode

  SYNCDELAY;

  gpif_abort_transaction();

  SYNCDELAY;
  // call gpif_init from fx2lib (contains pretty much the default init code generated by gpif designer)
  gpif_init(WaveData, InitData);
}


#define GPIFFLAGFULL 0x02
#define GPIFSTOPONFLAG 0x01
void gpif_endpoint_setup() {
  debug_printf("choosing gpif settings for endpoint 2");
  SYNCDELAY;
  // Select which FIFO Flag should be made available to the Waveform as a control input
  EP2GPIFFLGSEL = GPIFFLAGFULL;
  SYNCDELAY;
  // transition GPIF to done state when FIFO Flag criteria is met
  EP2GPIFPFSTOP = GPIFSTOPONFLAG;
  SYNCDELAY;

  EP2AUTOINLENH = 0x02;
  EP2AUTOINLENL = 0x00;

  // Endpoint 2 FIFO configuration
  EP2FIFOCFG = bmAUTOIN; // enable automatic commit to Serial interface Engine (SIE)
  SYNCDELAY;

  // zero out all other endpoints. If any WORDWIDE bit is 1, Port D is put into alternate IO mode
  EP4FIFOCFG = 0x00;
  SYNCDELAY;
  EP6FIFOCFG = 0x00;
  SYNCDELAY;
  EP8FIFOCFG = 0x00;
}

char gpif_poll_finished() {
  return GPIFIDLECS & 0x80;
}


// starts the gpif sampling routine
char gpif_start_sampling() {
  debug_printf("Starting sampling");
  set_gpif_delay(5);

 // the programmed waveform loops indefinitely, until it is manually stopped or a FIFO overflow occurs.
  gpif_set_tc16(1);

  gpif_fifo_read(GPIF_EP2);

  SYNCDELAY;

  tracer_state = TRACER_GPIF_RUNNING;

  return 0;
}

void main() {
  setup();
  SYNCDELAY;
  gpif_endpoint_setup();
  SYNCDELAY;
  _gpif_init();
  SYNCDELAY;
  // setup gpio ports for soft-SPI
  setup_soft_spi_ports();

  REARMEP1OUT();
  /* REARMEP2(); */
  d3off();


  static unsigned char write_data[1] = {0xF4};
  static unsigned char read_data[1];
  static unsigned char cnt = 0;

  tracer_state = TRACER_GPIF_READY;
  while(TRUE) {
    SYNCDELAY16;

    if ( got_sud ) {
      debug_printf ( "Handle setupdata\n" );
      handle_setupdata();
      got_sud=FALSE;
    }
  }
}

// copied routines from setupdat.h
// use default handler from setup.c
BOOL handle_get_descriptor() {
  return FALSE;
}

// value (low byte) = ep
typedef enum {
  VC_EPSTAT = 0xB1,
  VC_FWSTAT,
  VC_START_SAMP,
  VC_STOP_SAMP,
  VC_RENUM,
  VC_SET_DELAY,
  VC_GET_DELAY,
} VENDOR_COMMANDS;


BOOL handle_vendorcommand(BYTE cmd) {

  switch ( cmd ) {

  case VC_EPSTAT:
    {
      __xdata BYTE* pep= ep_addr(SETUPDAT[2]);
      debug_printf ( "ep %02x\n" , *pep );
      if (pep) {
        EP0BUF[0] = *pep;
        EP0BCH=0;
        EP0BCL=1;
        return TRUE;
      }
    }
  case VC_FWSTAT:
    {
      debug_printf("returning Firmware status\n");

      EP0BUF[0] = tracer_state;
      EP0BCH=0;
      EP0BCL=1;

      return TRUE;
    }
  case VC_START_SAMP:
    {
      debug_printf("start sampling\n");
      gpif_start_sampling();
      return TRUE;
    }
  case VC_STOP_SAMP:
    {
      debug_printf("stop sampling\n");
      gpif_abort_transaction();
      SYNCDELAY; SYNCDELAY; SYNCDELAY; SYNCDELAY; SYNCDELAY; SYNCDELAY;
      /* USBCS |= bmDISCON; */

      /* delay(1500); */
      /* USBCS &= bmDISCON; */
      return TRUE;
    }
  case VC_RENUM:
    {
      debug_printf("defer usb handling to fx2lp\n");
      if(!gpif_poll_finished()) {
        gpif_abort_transaction();
      }

      USBCS &= ~bmRENUM;
      return TRUE;
    }
  case VC_SET_DELAY:
    set_gpif_delay(EP0BUF[0]);

    EP0BCL = 0x00; // write any vale to EP0BCL to rearm
    return TRUE;
  case VC_GET_DELAY:
      EP0BUF[0] = get_gpif_delay();
      EP0BCH = 0x00; // write any vale to EP0BCL to rearm
      EP0BCL = 0x01; // write any vale to EP0BCL to rearm

    return TRUE;
  default:
    debug_printf ( "Need to implement vendor command: %02x\n", cmd );
  }
  return FALSE;
}

/* i2c_write(42, 0, NULL, 4, testdata); */

// this firmware only supports 0,0
BOOL handle_get_interface(BYTE ifc, BYTE* alt_ifc) {
  debug_printf ( "Get Interface\n" );
  if (ifc==0) {*alt_ifc=0; return TRUE;} else { return FALSE;}
}


BOOL handle_set_interface(BYTE ifc, BYTE alt_ifc) {
  debug_printf ( "Set interface %d to alt: %d\n" , ifc, alt_ifc );

  return FALSE;
}

// get/set configuration
BYTE handle_get_configuration() {
  return 1;
}

BOOL handle_set_configuration(BYTE cfg) {
  return cfg==1 ? TRUE : FALSE; // we only handle cfg 1
}

// copied usb jt routines from usbjt.h
void sudav_isr() __interrupt SUDAV_ISR {
  got_sud=TRUE;
  CLEAR_SUDAV();
}

// asserted when host sends data into endpoint which does not contain any data.
void ibn_isr(void) __interrupt IBN_ISR {
  // disable ibn interrupts for all endpoints
  DISABLE_EPxIBN();

  // handling as per p. 94 of fx2 technical specification
  CLEAR_USBINT();

  if(IBNIRQ & bmEP2IBN) {
    // todo
  }

  // clear IRQ bits by writing 1 to each
  IBNIRQ = 0xFF;

  NAKIRQ = bmIBN;
  SYNCDELAY;

  // we only use interrupts for endpoint 2 so only enable them
  ENABLE_EP2IBN();
  SYNCDELAY;
}

#define EP1OUTBUF_START 0xE780

void ep1out_isr() __interrupt EP1OUT_ISR{
  BYTE byte_count;

  CLEAR_USBINT();

  byte_count = EP1OUTBC;

  i2c_write(42, 0, NULL, byte_count, (BYTE *) EP1OUTBUF_START);

  // rearm endpoint for OUT transfers
  REARMEP1OUT();
}

#define CLKpin 1
#define MISOpin 2
#define MOSIpin 3
#define CSpin 4

#define bmCLK  (1 << CLKpin)
#define bmMISO (1 << MISOpin)
#define bmMOSI (1 << MOSIpin)
#define bmCS   (1 << CSpin)

#define IO_DELAY do {                           \
    SYNCDELAY; SYNCDELAY; \
  } while(0);

// hidden 0 -> maximum delay 256
void set_gpif_delay(unsigned char delay) {
  char *delay_state = (char *)0xE400 + 64;

  *delay_state = delay;
}

unsigned char get_gpif_delay() {
  char *delay_state = (char *)0xE400 + 64;
  return *delay_state;
}

char soft_spi_write(unsigned char *write_buf, unsigned char *read_buf, unsigned char length) {
  unsigned char written = 0;
  unsigned char read_byte = 0;
  while (written < length) {
    read_byte = 0;
    IOD &= ~bmCS;
    for(unsigned char k = 0; k < 8; k++) {
      IOD &= ~bmCLK;
      IO_DELAY;
      IOD = (IOD & ~bmMOSI) | (((write_buf[written] & (1 << k)) > 0) << MOSIpin);
      IO_DELAY;
      IOD |= bmCLK;
      IO_DELAY;
      read_byte |= ((IOD & bmMISO) > 0) << k;
      IO_DELAY;
    }
    read_buf[written] = read_byte;
    IO_DELAY;
    IOD |= bmCS;
    written++;
  }

  return 0;
}

void setup_soft_spi_ports() {
  union {
    struct __spi_conf cnf;
    unsigned char raw[4];
  } __port_conf;

  // enable pins 4-1 of Port D
  // 1 -> clk
  // 2 -> MISO
  // 3 -> MOSI
  // 4 -> CS
  __port_conf.cnf.CLK = 1;
  __port_conf.cnf.MISO = 2;
  __port_conf.cnf.MOSI = 3;
  __port_conf.cnf.CS = 4;

  unsigned oed_conf = 0;

  for (unsigned char p = 0; p < sizeof(__port_conf.raw); p++) {
    unsigned char direction = p ==  1 ? 0 : 1;
    oed_conf |= direction << __port_conf.raw[p] ;
  }

  OED = oed_conf;

  soft_spi_configuration = __port_conf.cnf;
}

__bit on5;
__xdata WORD sofct=0;
void sof_isr () __interrupt SOF_ISR __using 1 {
  ++sofct;
  if(sofct==8000) { // about 8000 sof interrupts per second at high speed
    on5=!on5;
    if (on5) {d5on();} else {d5off();}
    sofct=0;
  }
  CLEAR_SOF();
}

void usbreset_isr() __interrupt USBRESET_ISR {
  handle_hispeed(FALSE);
  CLEAR_USBRESET();
}
void hispeed_isr() __interrupt HISPEED_ISR {
  handle_hispeed(TRUE);
  CLEAR_HISPEED();
}
