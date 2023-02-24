/***************************** Include Files ********************************/
#include "sv_subscriber.h"

struct sSVSubscriber {
   // u8 ethAddr[6];
    u16 appId;
	u16 length;
};

struct sSVSubscriber_ASDU {

    char* svId;
    char* dataSet;

    u8* smpCnt;
    u8* confRev;
    u8* refrTm;
    u8* smpSynch;
    u8* smpMod;
    u8* smpRate;

    int dataBufferLength;
    u8* dataBuffer;
};

struct sASDU_data {
    int current[8];
    int voltage[8];
    int valueSet[16];
    u16 valueSet16bit[16];
}

sSVSubscriber_ASDU* asdu;
sASDU_data* valueSet;
u8 PacketsCounter = 0;


/****************************************************************************/
/**
*
* This function check the ethertype of the received frame. If the ethertype 
* matches the SV message thertype according to IEC61850-9-2 then handle the
* frame as an SV message.
*
* @param    FramePtr is a pointer to the frame to check.
*
* @return   None.
*
* @note     None
*
*****************************************************************************/
void parseSVMessage(EthernetFrame * FramePtr)
{
	u8 *frame_buffer = (u8 *)FramePtr;
	u16 bufPos;

    /* Ethernet source address */
    u8* dstAddr = frame_buffer;

    /* skip ethernet addresses */
    bufPos = 12u;
    // int headerLength = XEMACPS_HDR_SIZE;

    /* check for VLAN tag */
    if ((frame_buffer[bufPos] == 0x81) && (frame_buffer[bufPos + 1] == 0x00)) {
        /* skip VLAN tag */
		bufPos += 4u;
        // headerLength += 4;
    }

    /* check for SV Ethertype */
    if (frame_buffer[bufPos++] != 0x88){
        if (DEBUG_SV_SUBSCRIBER)
		    xil_printf("SV_SUBSCRIBER: SV message ignored due to mismatched ethertype\n");
		return;
	}
    if (frame_buffer[bufPos++] != 0xba){
        if (DEBUG_SV_SUBSCRIBER)
		    xil_printf("SV_SUBSCRIBER: SV message ignored due to mismatched ethertype\n");
		return;
	}    

    u16 appId;
    appId = frame_buffer[bufPos++] * 0x100;
    appId += frame_buffer[bufPos++];
	/* check for appID */
	if (appId > 0x7fff or app Id < 0x4000){
		xil_printf("SV_SUBSCRIBER: SV message ignored due to unknown appId\n");
		return;
	}

    u16 length;
    length = frame_buffer[bufPos++] * 0x100;
    length += frame_buffer[bufPos++];

	SVSubscriber sv_msg = NULL; 
	sv_msg.appId = appId;
	sv_msg.length = length;

    /* skip reserved fields */
    bufPos += 4u;

    u16 apduLength = length - 8u;

	xil_printf("SV_SUBSCRIBER: SV message: ----------------\n");
	xil_printf("SV_SUBSCRIBER:   APPID: %u\n", appId);
	xil_printf("SV_SUBSCRIBER:   LENGTH: %u\n", length);
	xil_printf("SV_SUBSCRIBER:   APDU length: %i\n", apduLength);

	parseSVPayload(frame_buffer + bufPos, apduLength);

}

/****************************************************************************/
/**
*
* This function check the header of an APDU area. It identifies the type 
* of the APDU including noASDU, ASDH, etc.
*
* @param    buffer is a pointer to the start of the APDU.
* @param    apduLength is the length of the APDU.
*
* @return   None.
*
* @note     None
*
*****************************************************************************/
void parseSVPayload(u8* buffer, u16 apduLength)
{
    int bufPos = 0;

    if (buffer[bufPos++] == 0x60) {
        int elementLength;
        bufPos = BerDecoder_decodeLength(buffer, &elementLength, bufPos, (int)apduLength);
        if (bufPos < 0) {
            if (DEBUG_SV_SUBSCRIBER) 
                xil_printf("SV_SUBSCRIBER: Malformed message: failed to decode BER length tag!\n");
            return;
        }

        int svEnd = bufPos + elementLength;
        while (bufPos < svEnd) {
            u8 tag = buffer[bufPos++];
            bufPos = BerDecoder_decodeLength(buffer, &elementLength, bufPos, svEnd);
            if (bufPos < 0){
                if (DEBUG_SV_SUBSCRIBER)
                    xil_printf("SV_SUBSCRIBER: Error: index out of range\n");
                return;
            }
            switch(tag) {
            case 0x80: /* noASDU (INTEGER) */
                /* ignore */
                break;

            case 0xa2: /* asdu (SEQUENCE) */
                parseSequenceOfASDU(buffer + bufPos, elementLength);
                break;

            default: /* ignore unknown tag */
                if (DEBUG_SV_SUBSCRIBER) 
                    xil_printf("SV_SUBSCRIBER: found unknown tag: %02x\n", tag);
                break;
            }

            bufPos += elementLength;
        }
    }

    return;
}

/****************************************************************************/
/**
*
* This function iterates the sequences of the ASDUs.
*
* @param    buffer is a pointer to the start of the asdu->
* @param    length is the length of the sequence of the ASDUs.
*
* @return   None.
*
* @note     None
*
*****************************************************************************/
void parseSequenceOfASDU(u8* buffer, int length)
{
    int bufPos = 0;

    while (bufPos < length) {
        int elementLength;
        u8 tag = buffer[bufPos++];
        bufPos = BerDecoder_decodeLength(buffer, &elementLength, bufPos, length);
        if (bufPos < 0) {
            if (DEBUG_SV_SUBSCRIBER) 
                xil_printf("SV_SUBSCRIBER: Malformed message: failed to decode BER length tag!\n");
            return;
        }

        switch (tag) {
        case 0x30:
            parseASDU(buffer + bufPos, elementLength);
            break;

        default: /* ignore unknown tag */
            if (DEBUG_SV_SUBSCRIBER) 
                xil_printf("SV_SUBSCRIBER: found unknown tag %02x\n", tag);
            break;
        }

        bufPos += elementLength;
    }
}

/****************************************************************************/
/**
*
* This function checks the information in an asdu->
*
* @param    buffer is a pointer to the start of this asdu->
* @param    length is the length of this asdu->
*
* @return   None.
*
* @note     None
*
*****************************************************************************/

void parseASDU(u8* buffer, int length)
{
    /* count every 16 packets */
    if (++PacketsCounter != 16){
        xil_printf("SV_SUBSCRIBER: SV message ignored due to over sampling rate\n");
        return;
    }
    PacketsCounter = 0;

    int bufPos = 0;
    int svIdLength = 0;
    int datSetLength = 0;

    
    memset(asdu, 0, sizeof(sSVSubscriber_ASDU));

    while (bufPos < length) {
        int elementLength;

        u8 tag = buffer[bufPos++];

        bufPos = BerDecoder_decodeLength(buffer, &elementLength, bufPos, length);
        if (bufPos < 0) {
            if (DEBUG_SV_SUBSCRIBER) 
                xil_printf("SV_SUBSCRIBER: Malformed message: failed to decode BER length tag!\n");
            return;
        }

        switch (tag) {
        case 0x80:
            asdu->svId = (char*) (buffer + bufPos);
            svIdLength = elementLength;
            break;

        case 0x81:
            asdu->dataSet = (char*) (buffer + bufPos);
            datSetLength = elementLength;
            break;

        case 0x82:
            asdu->smpCnt = buffer + bufPos;
            break;

        case 0x83:
            asdu->confRev = buffer + bufPos;
            break;

        case 0x84:
            asdu->refrTm = buffer + bufPos;
            break;

        case 0x85:
            asdu->smpSynch = buffer + bufPos;
            break;

        case 0x86:
            asdu->smpRate = buffer + bufPos;
            break;

        case 0x87:
            asdu->dataBuffer = buffer + bufPos;
            asdu->dataBufferLength = elementLength;
            break;

        case 0x88:
            asdu->smpMod = buffer + bufPos;
            break;

        default: /* ignore unknown tag */
            if (DEBUG_SV_SUBSCRIBER) 
                xil_printf("SV_SUBSCRIBER: found unknown tag %02x\n", tag);
            break;
        }

        bufPos += elementLength;
    }

    if (asdu->svId != NULL)
        asdu->svId[svIdLength] = 0;
    if (asdu->dataSet != NULL)
        asdu->dataSet[datSetLength] = 0;
    
    if (DEBUG_SV_SUBSCRIBER) {
        xil_printf("SV_SUBSCRIBER:   SV ASDU: ----------------\n");
        xil_printf("SV_SUBSCRIBER:     DataLength: %d\n", asdu->dataBufferLength);
        xil_printf("SV_SUBSCRIBER:     SvId: %s\n", asdu->svId);
        
        if (asdu->dataSet != NULL)
            xil_printf("SV_SUBSCRIBER:     dataSet: %s\n", asdu->dataSet);
    }

    computeCurVol(&asdu);

    return;
}

/****************************************************************************/
/**
*
* This function computes the current and voltage data in the dataset of an asdu
*
* @param    asdu is a pointer to this asdu
*
* @return   None.
*
* @note     None
*
*****************************************************************************/
void computeCurVol(sSVSubscriber_ASDU* asdu){

    memset(dataSet, 0, sizeof(sASDU_data));
    u8* buffer = asdu->dataBuffer;
    int bufPos = 0;
    int dataBufferLength = asdu->dataBufferLength;

    while (bufPos < 8 && bufPos < dataBufferLength){
        valueSet->valueSet[bufPos] = 1000*((int)*(buffer+bufPos));
        valueSet->valueSet16bit[bufPos] = 1000*((u16)*(buffer+bufPos));
        valueSet->current[bufPos] = 1000*((int)*(buffer+bufPos++));
    }
    while (bufPos < 16 && bufPos < dataBufferLength){
        valueSet->valueSet[bufPos] = 100*((int)*(buffer+bufPos));
        valueSet->valueSet16bit[bufPos] = 100*((u16)*(buffer+bufPos));
        valueSet->voltage[bufPos] = 100*((int)*(buffer+bufPos++));
    }
    
    return;
}

/****************************************************************************/
/**
*
* This function gets the pointer to the dataset of the latest ASDU
*
* @param    None
*
* @return   An u8 type pointer to the dataset.
*
* @note     None
*
*****************************************************************************/
u8* getASDUDataSet16Bit(){
    return (u8*)valueSet->&valueSet16bit[0];
}




