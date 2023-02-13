/*
 *  ber_decoder.c
 *
 *  Copyright 2013 Michael Zillgith
 *
 *  This file is part of libIEC61850.
 *
 *  libIEC61850 is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  libIEC61850 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libIEC61850.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  See COPYING file for the complete license text.
 */

#include "ber_decode.h"

int getIndefiniteLength(u8* buffer, int bufPos, int maxBufPos, int depth, int maxDepth)
{
    depth++;

    if (depth > maxDepth)
        return -1;

    int length = 0;

    while (bufPos < maxBufPos) {
        if ((buffer[bufPos] == 0) && ((bufPos + 1) < maxBufPos) && (buffer[bufPos+1] == 0)) {
            return length + 2;
        }
        else {
            length++;

            if ((buffer[bufPos++] & 0x1f) == 0x1f) {
                /* handle extended tags */
                bufPos++;
                length++;
            }

            int subLength = -1;

            int newBufPos = BerDecoder_decodeLengthRecursive(buffer, &subLength, bufPos, maxBufPos, depth, maxDepth);

            if (newBufPos == -1)
                return -1;

            length += subLength + newBufPos - bufPos;

            bufPos = newBufPos + subLength;
        }
    }

    return -1;
}

int BerDecoder_decodeLengthRecursive(u8* buffer, int* length, int bufPos, int maxBufPos, int depth, int maxDepth)
{
    if (bufPos >= maxBufPos)
        return -1;

    u8 len1 = buffer[bufPos++];

    if (len1 & 0x80) {
        int lenLength = len1 & 0x7f;

        if (lenLength == 0) { /* indefinite length form */
            *length = getIndefiniteLength(buffer, bufPos, maxBufPos, depth, maxDepth);
        }
        else {
            *length = 0;

            int i;
            for (i = 0; i < lenLength; i++) {
                if (bufPos >= maxBufPos)
                    return -1;

                if (bufPos + (*length) > maxBufPos)
                    return -1;

                *length <<= 8;
                *length += buffer[bufPos++];
            }
        }

    }
    else {
        *length = len1;
    }

    if (*length < 0)
        return -1;

    if (*length > maxBufPos)
        return -1;

    if (bufPos + (*length) > maxBufPos)
        return -1;

    return bufPos;
}

int BerDecoder_decodeLength(u8* buffer, int* length, int bufPos, int maxBufPos)
{
    return BerDecoder_decodeLengthRecursive(buffer, length, bufPos, maxBufPos, 0, 50);
}
