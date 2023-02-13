/***************************** Include Files ********************************/
#include <stdio.h>
#include <stdlib.h>
#include "xparameters.h"
#include "xil_types.h"
#include "xil_assert.h"
#include "xil_io.h"
#include "xil_exception.h"
#include "xil_cache.h"
#include "xil_printf.h"
#include "xemacps.h"		/* defines XEmacPs API */

#include "gem.h"
#include "ber_decode.h"
#include "dma_controller.h"

#define DEBUG_SV_SUBSCRIBER TRUE

u8 PacketsCounter = 0;
extern u8 *dmaDataSet;