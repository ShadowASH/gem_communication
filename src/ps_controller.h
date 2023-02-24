#include "gem.h"
#include "sv_subcruiber.h"
#include "dma_controller.h"

#include <stdio.h>
#include <stdlib.h>
#include "xparameters.h"
#include "xil_types.h"
#include "xil_assert.h"
#include "xil_io.h"
#include "xil_exception.h"
#include "xil_cache.h"
#include "xil_printf.h"
#ifdef XPAR_INTC_0_DEVICE_ID
#include "xintc.h"
#else
#include "xscugic.h"
#endif

#ifndef __MICROBLAZE__
#include "sleep.h"
#include "xparameters_ps.h"	/* defines XPAR values */
#include "xpseudo_asm.h"
#endif


#define FPGA_INTR_ID0    XPS_FPGA0_INT_ID

#ifdef XPAR_INTC_0_DEVICE_ID
 #define INTC		XIntc
 #define INTC_HANDLER	XIntc_InterruptHandler
#else
 #define INTC		XScuGic
 #define INTC_HANDLER	XScuGic_InterruptHandler
#endif

/* 16 integer(32bit) values in one ASDU, so 16*(32bit/8bit) = 64 */
#define MAX_ASDU_LENGTH           64
#define MAX_ASDU_LENGTH_16BIT     32