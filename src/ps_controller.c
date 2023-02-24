#include "ps_controller.h"

int runApplication = TRUE;

static INTC Intc;   /* Instance of the Interrupt Controller */

int main(void)
{
	LONG Status;
	xil_printf("Entering into application. \r\n");
	/*
	 * Start the application.
	 */
	Status = PSSetupIntrSystem(&Intc);
	if (Status != XST_SUCCESS) {
		xil_printf("Error initiating PS interrupt.\r\n");
		return XST_FAILURE;
	}
	Status = EmacPsDmaStart();
	if (Status != XST_SUCCESS) {
		xil_printf("Error initiating GEM.\r\n");
		return XST_FAILURE;
	}
    Status = AxiDmaStart();
    if (Status != XST_SUCCESS) {
		xil_printf("Error initiating DMA.\r\n");
		return XST_FAILURE;
	}

    while(runApplication){
        /*
        * Start receiving ethernet packages.
        */
        Status = EmacPsDmaIntrRecv(&EmacPsInstance);
        if (Status != XST_SUCCESS) {
            xil_printf("Emacps receiving packages Failed.\r\n");
            return XST_FAILURE;
        }

        /*
        * Send the data to dma controller.
        */
        u8 *dataSet = getASDUDataSet();
        Status = psSendDataToPL(dataSet, MAX_ASDU_LENGTH);
        if (Status != XST_SUCCESS) {
            xil_printf("Error sending data to PL");
            return XST_FAILURE;
        }
    }

	/*
	 * Stop the device.
	 */
	EmacPsDmaStop();
    AxiDmaStop();

	xil_printf("Application finished, stop the device.\r\n");
	return XST_SUCCESS;
}


/*****************************************************************************/
/*
*
* This function setups the PS-PL interrupt system so interrupts can occur for the
* PS, it assumes INTC component exists in the hardware system.
*
* @param	IntcInstancePtr is a pointer to the instance of the INTC.
* @param	AxiDmaPtr is a pointer to the instance of the DMA engine
* @param	TxIntrId is the TX channel Interrupt ID.
*
* @return
*		- XST_SUCCESS if successful,
*		- XST_FAILURE.if not successful
*
* @note		None.
*
******************************************************************************/
static int PSSetupIntrSystem(INTC * IntcInstancePtr)
{
    int Status;

#ifdef XPAR_INTC_0_DEVICE_ID

    /* Initialize the interrupt controller and connect the ISRs */
	Status = XIntc_Initialize(IntcInstancePtr, INTC_DEVICE_ID);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed init intc\r\n");
		return XST_FAILURE;
	}

    Status = XIntc_Connect(IntcInstancePtr, FPGA_INTR_ID0,
			       (XInterruptHandler) TxIntrHandler, (void*)0);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed tx connect intc\r\n");
		return XST_FAILURE;
	}

    /* Start the interrupt controller */
	Status = XIntc_Start(IntcInstancePtr, XIN_REAL_MODE);
	if (Status != XST_SUCCESS) {

		xil_printf("Failed to start intc\r\n");
		return XST_FAILURE;
	}
#else
}

/*****************************************************************************/
/*
*
* This is the fpga0 interrupt handler that will be called when the fpga detects
* an error from the SV sampled value. It calls the goose publisher which publishes
* an error to the RTDS.
*
* @param	Callback is a pointer to PS application.
*
* @return   None.
*
* @note		None.
*
******************************************************************************/