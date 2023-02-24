/***************************** Include Files ********************************/

#include "sv_subcruiber.h"

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

/************************** Constant Definitions ****************************/

#define EMACPS_LOOPBACK_SPEED    100	/* 100Mbps */
#define EMACPS_LOOPBACK_SPEED_1G 1000	/* 1000Mbps */
#define EMACPS_PHY_DELAY_SEC     4	/* Amount of time to delay waiting on
					   PHY to reset */
#define EMACPS_SLCR_DIV_MASK	0xFC0FC0FF

#define CSU_VERSION		0xFFCA0044
#define PLATFORM_MASK		0xF000
#define PLATFORM_SILICON	0x0000
#define VERSAL_VERSION		0xF11A0004
#define PLATFORM_MASK_VERSAL	0xF000000
#define PLATFORM_VERSALEMU	0x1000000
#define PLATFORM_VERSALSIL	0x0000000

// Ethertype of SV message
#define ETHERTYPE_SV    0x88ba
#define ETHERTYPE_AREA_SV       16U
#define ETHERTYPE_SIZE  2U
#define APPID_SIZE 2U
/* The sampling rate of RTDS is 4khz and FDTS is 250hz
   so only every one of the 16 packets needs to be saved*/
#define ETHER_SAMPLING_RATE     16


/***************** Macros (Inline Functions) Definitions ********************/


/**************************** Type Definitions ******************************/

/*
 * Define an aligned data type for an ethernet frame. This declaration is
 * specific to the GNU compiler
 */
#ifdef __ICCARM__
typedef char EthernetFrame[XEMACPS_MAX_VLAN_FRAME_SIZE_JUMBO];
#else
typedef char EthernetFrame[XEMACPS_MAX_VLAN_FRAME_SIZE_JUMBO]
	__attribute__ ((aligned(64)));
#endif

/************************** Function Prototypes *****************************/

/*
 * Application
 */
LONG EmacPsDmaStart();
LONG EmacPsDmaInit(INTC * IntcInstancePtr,
			  XEmacPs * EmacPsInstancePtr,
			  u16 EmacPsDeviceId)
void EmacPsDmaStop();
LONG EmacPsDmaIntrRecv(XEmacPs *EmacPsInstancePtr);
EthernetFrame getRxFrame();

/*
 * Utility functions implemented in gem_util.c
 */
void EmacPsUtilSetupUart(void);
void EmacPsUtilFrameHdrFormatMAC(EthernetFrame * FramePtr, char *DestAddr);
void EmacPsUtilFrameHdrFormatType(EthernetFrame * FramePtr, u16 FrameType);
void EmacPsUtilFrameSetPayloadData(EthernetFrame * FramePtr, u32 PayloadSize);
LONG EmacPsUtilFrameVerify(EthernetFrame * CheckFrame,
			   EthernetFrame * ActualFrame);
void EmacPsUtilFrameMemClear(EthernetFrame * FramePtr);
LONG EmacPsUtilEnterLoopback(XEmacPs * XEmacPsInstancePtr, u32 Speed);
void EmacPsUtilstrncpy(char *Destination, const char *Source, u32 n);
void EmacPsUtilErrorTrap(const char *Message);
void EmacpsDelay(u32 delay);

/************************** Variable Definitions ****************************/

extern XEmacPs EmacPsInstance;	/* Device instance used throughout examples */
extern char EmacPsMAC[];	/* Local MAC address */
extern u32 Platform;

extern u8 PacketsCounter;


#endif /* XEMACPS_EXAMPLE_H */
