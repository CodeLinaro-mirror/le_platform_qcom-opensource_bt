/**
* Copyright (c) 2016, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

/**
@file
@internal
This file provides functions for marshalling and unmarshalling of data elements.
*/

#define __OI_MODULE__ OI_MODULE_DATAELEM

#include "oi_status.h"
#include "oi_debug.h"
#include "oi_assert.h"
#include "oi_dataelem.h"
#include "oi_bytestream.h"
#include "oi_std_utils.h"
#include "oi_marshaller.h"

#ifdef OI_TEST_HARNESS
    #include "../sdp/oi_sdpcli_test.h"
#endif

#define DATAELEM_BO   NETWORK_BYTE_ORDER


#define ELEMENT_DESCRIPTOR_SIZE  sizeof(OI_UINT8)


/*
 * Internal structure to crawl OI_DATAELEMENT tree without using recursion
 */
typedef struct _OI_CHAIN {
    OI_UINT16          Size;
    OI_UINT16          Length;
    const OI_DATAELEM *Element;
    struct _OI_CHAIN  *Next;
} OI_CHAIN;

/***************************** forward declarations *********************/

static OI_STATUS UnmarshalListElement(OI_BYTE_STREAM *ByteStream,
                                      OI_UINT8 ElemSizeIndex,
                                      OI_DATAELEM *ListElem);

/************************* end of forward declarations ******************/


/**
 * This function writes the element descriptor for a variable-length element
 * type to a bytestream. The function chooses the smallest element descriptor
 * that will hold the length specified.
 *
 * @note Since the Bluetooth specification does not support packets larger than
 * 64kbytes, the protocol stack does not need to support OI_DATAELEM_VAR32 on
 * writing, but the protocol stack still needs to be able to read them.
 */
static OI_INT32 MarshalVarSizeDescriptor(OI_BYTE_STREAM *ByteStream,
                                          OI_INT32 offset,
                                          OI_INT Size,
                                          OI_UINT8 ElemType)
{
    OI_UINT16 bytesAllowed = ByteStream_NumWriteBytesAllowed(*ByteStream);

    /*
     * The appropriate size index is chosen based on the Size parameter.
     */
    if (Size > OI_UINT8_MAX) {
        OI_ASSERT(Size < OI_UINT16_MAX);
        if (bytesAllowed < (sizeof(OI_UINT8) + sizeof(OI_UINT16))) {
            return -1;
        }
        if (offset) {
            offset -= (sizeof(OI_UINT8) + sizeof(OI_UINT16));
        } else {
            ByteStream_PutUINT8(*ByteStream, OI_DATAELEM_DESCRIPTOR(ElemType, OI_DATAELEM_VAR16));
            ByteStream_PutUINT16(*ByteStream, (OI_UINT16) Size, DATAELEM_BO);
        }
    } else {
        if (bytesAllowed < (sizeof(OI_UINT8) + sizeof(OI_UINT8))) {
            return -1;
        }
        if (offset) {
            offset -= (sizeof(OI_UINT8) + sizeof(OI_UINT8));
        } else {
            ByteStream_PutUINT8(*ByteStream, OI_DATAELEM_DESCRIPTOR(ElemType, OI_DATAELEM_VAR8));
            ByteStream_PutUINT8(*ByteStream, (OI_UINT8) Size);
        }
    }
    return offset;
}

/*
 * This function will write a 32 it integer to the bytestream only if there is
 * room for it. It is used for segmenting 64 and 128 bit integers.
 */
static OI_INT32 MarshalUINT32(OI_BYTE_STREAM *ByteStream,
                              OI_INT32 offset,
                              OI_UINT32 value)
{
    OI_UINT16 bytesAllowed = ByteStream_NumWriteBytesAllowed(*ByteStream);

    if (offset == -1) {
        return -1;
    }
    if (offset >= (int)sizeof(OI_UINT32)) {
        offset -= (int)sizeof(OI_UINT32);
        return offset;
    }
    bytesAllowed = ByteStream_NumWriteBytesAllowed(*ByteStream);
    if (bytesAllowed < (int)sizeof(OI_UINT32)) {
        return -1;
    }
    ByteStream_PutUINT32(*ByteStream, value, DATAELEM_BO);
    return offset;
}


/**
 * This function writes a data element to a byte stream.
 *
 * If offset is > 0 the element is skipped and the offset it decremented by the
 * size of the element, otherwise the element is written to the bytestream. If
 * the entire element does not fit in the bytestream a partial element is
 * written and this function returns -1.
 */
static OI_INT32 MarshalElement(OI_BYTE_STREAM *ByteStream,
                        OI_INT32 offset,
                        const OI_DATAELEM *pElement)
{
    OI_UINT16 elemSize;
    OI_UINT16 size;
    OI_UINT16 bytesAllowed;
    const OI_DATAELEM *Element = pElement;
    const OI_DATAELEM *pElementRef;
    OI_CHAIN top_link = {1,0,NULL,NULL};
    OI_CHAIN *chain = &top_link;

    /*
     * We are passed a "list of one" on entry.
     */
    while (TRUE) {
        chain->Size--;
        bytesAllowed = ByteStream_NumWriteBytesAllowed(*ByteStream);

        /*
         * If no room left, return -1
         */
        if (!bytesAllowed) {
            OI_ASSERT(offset == 0);
            goto ReturnPartial;
        }

        /*
         * REF elements must be dereferenced before doing anything with them.
         */
        if (Element->ElemType == OI_DATAELEM_REF) {
            pElementRef = Element->Value.ElemRef;
        } else {
            pElementRef = Element;
        }

        elemSize = pElementRef->Size;

        switch (pElementRef->ElemType) {
            case OI_DATAELEM_NULL:
                if (offset) {
                    offset -= sizeof(OI_UINT8);
                    break;
                }
                ByteStream_PutUINT8(*ByteStream, OI_DATAELEM_DESCRIPTOR(OI_DATAELEM_NULL, 0));
                break;
            case OI_DATAELEM_UUID:
                /*
                 * Narrow UUID32 to UUID16 if possible
                 */
                if ((elemSize == sizeof(OI_UINT32)) && ((pElementRef->Value.UInt & 0xFFFF0000) == 0)) {
                    elemSize = sizeof(OI_UUID16);
                    #ifdef OI_TEST_HARNESS
                        if (OI_SDPCliTest.force32BitUUID) {
                            elemSize = sizeof(OI_UUID32);
                        }
                    #endif
                }
                /* falling through */
            case OI_DATAELEM_BOOL:
            case OI_DATAELEM_UINT:
            case OI_DATAELEM_SINT:
                /*
                 * Are we skipping the entire element?
                 */
                if (offset >= (elemSize + (OI_INT32)sizeof(OI_UINT8))) {
                    offset -= (elemSize + sizeof(OI_UINT8));
                    break;
                }
                switch (elemSize) {
                    case sizeof(OI_UINT8):
                        if (bytesAllowed < (sizeof(OI_UINT8) + sizeof(OI_UINT8))) {
                            goto ReturnPartial;
                        }
                        ByteStream_PutUINT8(*ByteStream, OI_DATAELEM_DESCRIPTOR(pElementRef->ElemType, OI_DATAELEM8));
                        ByteStream_PutUINT8(*ByteStream, (OI_UINT8) pElementRef->Value.UInt);
                        break;
                    case sizeof(OI_UINT16):
                        if (bytesAllowed < (sizeof(OI_UINT8) + sizeof(OI_UINT16))) {
                            goto ReturnPartial;
                        }
                        ByteStream_PutUINT8(*ByteStream, OI_DATAELEM_DESCRIPTOR(pElementRef->ElemType, OI_DATAELEM16));
                        ByteStream_PutUINT16(*ByteStream, (OI_UINT16) pElementRef->Value.UInt, DATAELEM_BO);
                        break;
                    case sizeof(OI_UINT32):
                        if (bytesAllowed < (sizeof(OI_UINT8) + sizeof(OI_UINT32))) {
                            goto ReturnPartial;
                        }
                        ByteStream_PutUINT8(*ByteStream, OI_DATAELEM_DESCRIPTOR(pElementRef->ElemType, OI_DATAELEM32));
                        ByteStream_PutUINT32(*ByteStream, pElementRef->Value.UInt, DATAELEM_BO);
                        break;
                    case sizeof(OI_UINT64):
                        if (offset) {
                            offset -= sizeof(OI_UINT8);
                        } else {
                            ByteStream_PutUINT8(*ByteStream, OI_DATAELEM_DESCRIPTOR(pElementRef->ElemType, OI_DATAELEM64));
                        }
                        offset = MarshalUINT32(ByteStream, offset, pElementRef->Value.UInt64->I1);
                        offset = MarshalUINT32(ByteStream, offset, pElementRef->Value.UInt64->I2);
                        if (offset == -1) {
                            goto ReturnPartial;
                        }
                        break;
                    case sizeof(OI_UINT128):
                        if (offset) {
                            offset -= sizeof(OI_UINT8);
                        } else {
                            ByteStream_PutUINT8(*ByteStream, OI_DATAELEM_DESCRIPTOR(pElementRef->ElemType, OI_DATAELEM128));
                        }
                        if (pElementRef->ElemType == OI_DATAELEM_UUID) {
                            offset = MarshalUINT32(ByteStream, offset, pElementRef->Value.LongUUID->ms32bits);
                            if (offset < 0) {
                                goto ReturnPartial;
                            }
                            /*
                             * Write until we fill the bytestream.
                             */
                            bytesAllowed = ByteStream_NumWriteBytesAllowed(*ByteStream);
                            size = (OI_UINT16) (sizeof(pElementRef->Value.LongUUID->base) - offset);
                            ByteStream_PutBytes(*ByteStream, pElementRef->Value.LongUUID->base + offset, OI_MIN(size, bytesAllowed));
                            if (bytesAllowed < size) {
                                goto ReturnPartial;
                            }
                            offset = 0;
                        } else {
                            offset = MarshalUINT32(ByteStream, offset, pElementRef->Value.UInt128->I1);
                            offset = MarshalUINT32(ByteStream, offset, pElementRef->Value.UInt128->I2);
                            offset = MarshalUINT32(ByteStream, offset, pElementRef->Value.UInt128->I3);
                            offset = MarshalUINT32(ByteStream, offset, pElementRef->Value.UInt128->I4);
                            if (offset == -1) {
                                goto ReturnPartial;
                            }
                        }
                        break;
                    default:
                        OI_ASSERT_FAIL("Bad element size");
                }
                break;
            case OI_DATAELEM_TEXT:
            case OI_DATAELEM_URL:
                offset = MarshalVarSizeDescriptor(ByteStream, offset, elemSize, pElementRef->ElemType);
                if (offset == -1) {
                    goto ReturnPartial;
                }
                if (offset >= elemSize) {
                    offset -= elemSize;
                } else {
                    bytesAllowed = ByteStream_NumWriteBytesAllowed(*ByteStream);
                    size = (OI_UINT16) (elemSize - offset);
                    ByteStream_PutBytes(*ByteStream, pElementRef->Value.RawBytes + offset, OI_MIN(size, bytesAllowed));
                    if (bytesAllowed < size) {
                        goto ReturnPartial;
                    }
                    offset = 0;
                }
                break;
            case OI_DATAELEM_SEQ:
            case OI_DATAELEM_ALT:
                /*
                 * For list elements, elemSize is the number of elements, not the byte size.
                 */
                size = OI_DataElement_MarshalledSize(Element);
                if (size == 0) {
                    /*
                     * If size == 0, we had a failure allocating dynamic memory
                     * for this sequence, and should terminate this packet now.
                     */
                    goto ReturnPartial;
                }
                /*
                 * Adjust size for VarSizeDescriptor length
                 */
                size -= 2;
                if (size > OI_UINT8_MAX) {
                    size -= 1;
                }
                offset = MarshalVarSizeDescriptor(ByteStream, offset, size, pElementRef->ElemType);
                if (offset == -1) {
                    goto ReturnPartial;
                }
                if (offset >= size) {
                    offset -= size;
                } else if ((pElementRef->Value.ElemSeq != NULL) && (pElementRef->Size != 0)) {
                    OI_CHAIN *next = OI_Malloc(sizeof(OI_CHAIN));

                    if (next != NULL) {
                        /*
                         * Remember the Sequence element we were at.
                         */
                        chain->Element = Element;
                        next->Next = chain;
                        chain = next;
                        chain->Size = pElementRef->Size;
                        Element = pElementRef->Value.ElemSeq;
                        continue;
                    } else {
                        /*
                         * Return -1 indicating that we do not have the
                         * resources to parse the tree further.
                         */
                        OI_SLOG_ERROR(OI_STATUS_OUT_OF_MEMORY, ("MarshalElement: %!", OI_STATUS_OUT_OF_MEMORY));
                        goto ReturnPartial;
                    }
                }
                break;
            default:
                OI_ASSERT_FAIL("Bad element type");
        }

        if (chain != &top_link) {
            if (chain->Size != 0) {
                /*
                 * This nested sequence has more elements, so continue with next one.
                 */
                Element++;
            } else {
                /*
                 * End of Sequence reached. Pop off chain and continue with next
                 * element in list, or continue poping until an unfinished
                 * sequence reached, or top_link reached.
                 */
                while ((chain != &top_link) && (chain->Size == 0)) {
                    OI_CHAIN *next;

                    next = chain->Next;
                    OI_Free(chain);
                    chain = next;
                }

                /*
                 * If outer sequence not finished, continue with next in sequence.
                 */
                if (chain != &top_link) {
                    Element = chain->Element;
                    Element++;
                }
            }
        }

        if (chain == &top_link) {
            OI_ASSERT(offset >= 0);
            return offset;
        }

    }

ReturnPartial:
    /*
     * Free memory used to traverse nested sequences
     */
    while (chain != &top_link) {
        OI_CHAIN *next = chain->Next;
        OI_Free(chain);
        chain = next;
    }
    return -1;
}


OI_BOOL OI_DataElement_Marshal(OI_BYTE_STREAM *ByteStream,
                               const OI_DATAELEM *Element)
{
    return MarshalElement(ByteStream, 0, Element) == 0;
}


OI_BOOL OI_DataElement_MarshalSegment(OI_BYTE_STREAM *ByteStream,
                                      OI_UINT16 *segmentOffset,
                                      const OI_DATAELEM *Element)
{
    OI_INT32 result;

    result = MarshalElement(ByteStream, *segmentOffset, Element);
    if (result == -1) {
        *segmentOffset = 0;
        return FALSE;  /* bytestream full */
    } else {
        *segmentOffset = (OI_UINT16) result;
        return TRUE;   /* bytstream not full */
    }
}

/**
 * This function returns the byte length of a data element in the SDP network
 * data element representation.
 */
OI_UINT16 OI_DataElement_MarshalledSize(const OI_DATAELEM *pElement)
{
    OI_UINT16 elemSize;
    const OI_DATAELEM *Element = pElement;
    const OI_DATAELEM *pElementRef;
    OI_CHAIN top_link = {1,0,NULL,NULL};
    OI_CHAIN *chain = &top_link;

    /*
     * We are passed a "list of one" on entry.
     */
    while (TRUE) {

        chain->Size--;
        chain->Length += ELEMENT_DESCRIPTOR_SIZE;

        /*
         * REF elements must be dereferenced before doing anything with them.
         */
        if (Element->ElemType == OI_DATAELEM_REF) {
            pElementRef = Element->Value.ElemRef;
        } else {
            pElementRef = Element;
        }

        elemSize = pElementRef->Size;

        switch (pElementRef->ElemType) {
            case OI_DATAELEM_NULL:
                break;
            case OI_DATAELEM_UUID:
                /*
                 * Narrow UUID32 to UUID16 if possible
                 */
                if ((elemSize == sizeof(OI_UINT32)) && ((pElementRef->Value.UInt & 0xFFFF0000) == 0)) {
                    elemSize = sizeof(OI_UINT16);
                    #ifdef OI_TEST_HARNESS
                        if (OI_SDPCliTest.force32BitUUID) {
                            elemSize = sizeof(OI_UUID32);
                        }
                    #endif
                }
                /* Falling through */
            case OI_DATAELEM_BOOL:
            case OI_DATAELEM_UINT:
            case OI_DATAELEM_SINT:
                chain->Length += elemSize;
                break;
            case OI_DATAELEM_TEXT:
            case OI_DATAELEM_URL:
                chain->Length += elemSize;
                chain->Length += (elemSize > OI_UINT8_MAX) ? 2 : 1;
                break;
            case OI_DATAELEM_SEQ:
            case OI_DATAELEM_ALT:
                if ((pElementRef->Value.ElemSeq != NULL) && (pElementRef->Size != 0)) {
                    OI_CHAIN *next = OI_Malloc(sizeof(OI_CHAIN));

                    if (next != NULL) {
                        /*
                         * Remember the element we were at.
                         */
                        chain->Element = Element;
                        next->Next = chain;
                        chain = next;
                        chain->Size = pElementRef->Size;
                        chain->Length = 0;
                        Element = pElementRef->Value.ElemSeq;
                        continue;
                    } else {
                        /*
                         * Return bad (zero) value which will cause an eventual.
                         * overflow in outgoing bytstream.
                         */
                        OI_SLOG_ERROR(OI_STATUS_OUT_OF_MEMORY, ("OI_DataElement_MarshalledSize: %!", OI_STATUS_OUT_OF_MEMORY));
                        while (chain != &top_link) {
                            next = chain->Next;
                            OI_Free(chain);
                            chain = next;
                        }
                        return 0;
                    }
                }
                break;
            default:
                OI_ASSERT_FAIL("Bad element type");
        }

        if (chain != &top_link) {
            if (chain->Size != 0) {
                Element++;
            } else {
                /*
                 * End of Sequence reached. Pop off chain, and save computed length
                 */
                while ((chain != &top_link) && (chain->Size == 0)) {
                    OI_CHAIN *next;

                    chain->Length += (chain->Length > OI_UINT8_MAX) ? 2 : 1;
                    next = chain->Next;
                    next->Length += chain->Length;
                    OI_Free(chain);
                    chain = next;
                }
                if (chain != &top_link) {
                    Element = chain->Element;
                    Element++;
                }
            }
        }

        if (chain == &top_link) {
            return top_link.Length;
        }
    }
}


/**
 * This function unmarshals the size of a data element and return the size of
 * the element. The caller should check for ByteStream_Error(ByteStream).
 */
static OI_UINT16 UnmarshalElementSize(OI_BYTE_STREAM *ByteStream,
                                      OI_UINT8 ElemSizeIndex)
{
    OI_UINT16 valSize;

    switch (ElemSizeIndex) {
        case OI_DATAELEM8:
            valSize = sizeof(OI_UINT8);
            break;
        case OI_DATAELEM16:
            valSize = sizeof(OI_UINT16);
            break;
        case OI_DATAELEM32:
            valSize = sizeof(OI_UINT32);
            break;
        case OI_DATAELEM64:
            valSize = sizeof(OI_UINT64);
            break;
        case OI_DATAELEM128:
            valSize = sizeof(OI_UINT128);
            break;
        case OI_DATAELEM_VAR8:
            {
                OI_UINT8 len = 0;
                ByteStream_GetUINT8_Checked(*ByteStream, len);
                valSize = (OI_UINT16) len;
            }
            break;
        case OI_DATAELEM_VAR16:
            {
                OI_UINT16 len = 0;
                ByteStream_GetUINT16_Checked(*ByteStream, len, DATAELEM_BO);
                valSize = (OI_UINT16) len;
            }
            break;
        case OI_DATAELEM_VAR32:
            {
                OI_UINT32 len = 0;
                ByteStream_GetUINT32_Checked(*ByteStream, len, DATAELEM_BO);
                valSize = (OI_UINT16) len;
            }
            break;
        default:
            /*
             * Force a bytestream error to prevent this error from propogating
             */
            ByteStream_SetError(*ByteStream);
            OI_SLOG_ERROR(OI_SDP_CORRUPT_DATA_ELEMENT, ("Bad dataelement size index"));
            valSize = 0;
            break;
    }

    return valSize;
}


/**
 * This function counts the elements in an element list from the current position
 * in the byte stream given the total byte length of the element list. This
 * function leaves the position of the byte stream unchanged after the call. The
 * caller should check ByteStream_Error(ByteStream) to see if the list was correctly formed.
 */
static OI_UINT16 CountListElements(OI_BYTE_STREAM *ByteStream,
                                   OI_UINT16 Len)
{
    OI_UINT8 elemDescriptor = 0;
    OI_UINT16 size;
    OI_UINT16 startPos;
    OI_UINT16 endPos;
    OI_UINT16 pos;
    OI_UINT16 count = 0;

    pos = ByteStream_GetPos(*ByteStream);
    startPos = pos;
    endPos = pos + Len;

    while ((pos < endPos) && !ByteStream_Error(*ByteStream)) {
        /*
         * Get the element descriptor so that the size of the element can be computed.
         */
        ByteStream_GetUINT8_Checked(*ByteStream, elemDescriptor);
        if (elemDescriptor != OI_DATAELEM_NULL) {
            size = UnmarshalElementSize(ByteStream, OI_DATAELEM_SIZE_INDEX(elemDescriptor));
            ByteStream_Skip_Checked(*ByteStream, size);
        }
        ++count;
        /*
         * Get the current position in the byte stream so that we can test if we have
         * reached the end of the list
         */
        pos = ByteStream_GetPos(*ByteStream);
    }

    /*
     * Check that the length was exactly as expected and flag an error if it was not.
     */
    if (pos != endPos) {
        ByteStream_SetError(*ByteStream);
    }

    ByteStream_SetPos(*ByteStream, startPos);

    return count;
}


/**
 * This function gets the size of a text element, allocates memory for a
 * null-terminated string, and initalizes it from the byte stream.
 */
static OI_STATUS UnmarshalTextElement(OI_BYTE_STREAM *ByteStream,
                                      OI_UINT8 ElemSizeIndex,
                                      OI_DATAELEM *Element)
{
    Element->Size = UnmarshalElementSize(ByteStream, ElemSizeIndex);
    if (ByteStream_Error(*ByteStream)) {
        return OI_SDP_CORRUPT_DATA_ELEMENT;
    }
    /*
     * Allocate an extra two bytes so that the string can be double
     * null-terminated this will allow ASCII and Unicode strings to be handled
     * by functions that detect null termination such as printf(). Note that the
     * element size does not reflect the added nulls so are NOT part of the data
     * element.
     */
    Element->Value.Text = (OI_CHAR*) OI_Malloc(Element->Size + 2);
    if (Element->Value.Text == NULL) {
        return OI_STATUS_OUT_OF_MEMORY;
    }
    ByteStream_GetBytes_Checked(*ByteStream, Element->Value.Text, Element->Size);
    /*
     * Null-terminate the text string.
     */
    Element->Value.Text[Element->Size] = 0;
    Element->Value.Text[Element->Size + 1] = 0;
    if (ByteStream_Error(*ByteStream)) {
        return OI_SDP_CORRUPT_DATA_ELEMENT;
    } else {
        return OI_OK;
    }
}


OI_STATUS OI_DataElement_Peek(OI_BYTE_STREAM *ByteStream,
                              OI_UINT8 *elemType,
                              OI_UINT16 *size)
{
    OI_STATUS status = OI_OK;
    OI_UINT16 pos;
    OI_UINT16 pos2;
    OI_UINT8 elemSizeIndex;
    OI_UINT8 elemDescriptor = 0;

    pos = ByteStream_GetPos(*ByteStream);
    ByteStream_GetUINT8_Checked(*ByteStream, elemDescriptor);
    if (ByteStream_Error(*ByteStream)) {
        status =  OI_SDP_DATA_ELEMENT_TRUNCATED;
    } else {
        *elemType = OI_DATAELEM_TYPE(elemDescriptor);
        /*
         * Get size (in bytes) if the caller wants it.
         */
        if (size != NULL) {
            if (*elemType == OI_DATAELEM_NULL) {
                *size = sizeof(OI_UINT8);
            } else {
                elemSizeIndex = OI_DATAELEM_SIZE_INDEX(elemDescriptor);
                *size = UnmarshalElementSize(ByteStream, elemSizeIndex);
                pos2 = ByteStream_GetPos(*ByteStream);
                *size += pos2 - pos;
            }
        }
        if (ByteStream_Error(*ByteStream)) {
            status =  OI_SDP_DATA_ELEMENT_TRUNCATED;
        }
    }
    ByteStream_SetPos(*ByteStream, pos);
    return status;
}


OI_STATUS OI_DataElement_UnmarshallListHeader(OI_BYTE_STREAM *ByteStream,
                                              OI_UINT16 *headerBytes,
                                              OI_UINT16 *listBytes)
{
    OI_UINT8 elemDescriptor = 0;
    OI_UINT8 elemSizeIndex;

    ByteStream_GetUINT8_Checked(*ByteStream, elemDescriptor);
    if (ByteStream_Error(*ByteStream)) {
        return OI_SDP_DATA_ELEMENT_TRUNCATED;
    }
    if ((OI_DATAELEM_TYPE(elemDescriptor) != OI_DATAELEM_SEQ) && (OI_DATAELEM_TYPE(elemDescriptor) != OI_DATAELEM_ALT)) {
        return OI_SDP_CORRUPT_DATA_ELEMENT;
    }
    elemSizeIndex = OI_DATAELEM_SIZE_INDEX(elemDescriptor);
    switch (elemSizeIndex) {
        case OI_DATAELEM_VAR8:
            *headerBytes = OI_DATAELEM_VAR8_SIZE;
            break;
        case OI_DATAELEM_VAR16:
            *headerBytes = OI_DATAELEM_VAR16_SIZE;
            break;
        case OI_DATAELEM_VAR32:
            *headerBytes = OI_DATAELEM_VAR32_SIZE;
            break;
        default:
            return OI_SDP_CORRUPT_DATA_ELEMENT;
    }
    *listBytes = UnmarshalElementSize(ByteStream, elemSizeIndex);
    if (ByteStream_Error(*ByteStream)) {
        return OI_SDP_DATA_ELEMENT_TRUNCATED;
    } else {
        return OI_OK;
    }
}


/**
 * This function unmarshals a data element.
 */
static OI_STATUS UnmarshalElement(OI_BYTE_STREAM *ByteStream,
                                  OI_DATAELEM    *pElement)
{
    OI_DATAELEM *Element = pElement;
    OI_STATUS status = OI_OK;
    OI_UINT8 elemSizeIndex;
    OI_UINT8 elemDescriptor = 0;

    while (OI_SUCCESS(status)) {
        /*
         * If the current Element is a Parent Pointer, we are at the end of
         * parsing a list and must pop back to the parent list Element. If
         * the parent list element is the original passed in element we are
         * done. Otherwise we are in an enclosing (nested) list, and must
         * continue parsing the next element in that list.
         */
        while (Element->ElemType == OI_DATAELEM_PARENT) {
            Element = Element->Value.ListParent;
            goto ContinueUnmarshall;
        }

        ByteStream_GetUINT8_Checked(*ByteStream, elemDescriptor);
        if (ByteStream_Error(*ByteStream)) {
            return OI_SDP_CORRUPT_DATA_ELEMENT;
        }

        Element->ElemType = OI_DATAELEM_TYPE(elemDescriptor);

        /*
         * Nothing to parse if this is a null element
         */
        if (elemDescriptor == OI_DATAELEM_NULL) {
            Element->Size = 0;
            goto ContinueUnmarshall;
        }

        elemSizeIndex = OI_DATAELEM_SIZE_INDEX(elemDescriptor);

        /*
         * Verify Type vs Size Combination, per BT Core v2.0+EDR, Sec 3.2, Tab 3.1
         */

        switch (Element->ElemType) {
            /* Standard BT Type Descriptors */
            case OI_DATAELEM_NULL:
            case OI_DATAELEM_BOOL:
                if (elemSizeIndex != OI_DATAELEM8) {
                    return OI_SDP_CORRUPT_DATA_ELEMENT;
                }
                break;

                /* Standard BT Type Descriptor */
            case OI_DATAELEM_UUID:
                if ((elemSizeIndex == OI_DATAELEM8) || (elemSizeIndex == OI_DATAELEM64)) {
                    return OI_SDP_CORRUPT_DATA_ELEMENT;
                }
                /* Fall Through */

                /* Standard BT Type Descriptors */
            case OI_DATAELEM_UINT:
            case OI_DATAELEM_SINT:
                if (elemSizeIndex > OI_DATAELEM128) {
                    return OI_SDP_CORRUPT_DATA_ELEMENT;
                }
                break;

                /* Standard BT Type Descriptors */
            case OI_DATAELEM_TEXT:
            case OI_DATAELEM_SEQ:
            case OI_DATAELEM_ALT:
            case OI_DATAELEM_URL:
                if (elemSizeIndex < OI_DATAELEM_VAR8) {
                    return OI_SDP_CORRUPT_DATA_ELEMENT;
                }
                break;

            default:
                status = OI_SDP_CORRUPT_DATA_ELEMENT;
                OI_SLOG_ERROR(status, ("Reserved Elem Desc Rxed, discarded: %2x\n",elemDescriptor));
                return status;
        }

        switch (elemSizeIndex) {
            case OI_DATAELEM8:
                {
                    OI_UINT8 val = 0;
                    ByteStream_GetUINT8_Checked(*ByteStream, val);
                    Element->Value.UInt = (OI_UINT32) val;
                    Element->Size = sizeof(val);
                }
                break;
            case OI_DATAELEM16:
                {
                    OI_UINT16 val = 0;
                    ByteStream_GetUINT16_Checked(*ByteStream, val, DATAELEM_BO);
                    Element->Value.UInt = (OI_UINT32) val;
                    Element->Size = sizeof(val);
                }
                break;
            case OI_DATAELEM32:
                {
                    OI_UINT32 val = 0;
                    ByteStream_GetUINT32_Checked(*ByteStream, val, DATAELEM_BO);
                    Element->Value.UInt = (OI_UINT32) val;
                    Element->Size = sizeof(val);
                }
                break;
            case OI_DATAELEM64:
                Element->Value.UInt64 = (OI_UINT64*) OI_Malloc(sizeof(OI_UINT64));
                if (Element->Value.UInt64 == NULL) {
                    return OI_STATUS_OUT_OF_MEMORY;
                }
                ByteStream_GetUINT32_Checked(*ByteStream, Element->Value.UInt64->I1, DATAELEM_BO);
                ByteStream_GetUINT32_Checked(*ByteStream, Element->Value.UInt64->I2, DATAELEM_BO);
                Element->Size = sizeof(OI_UINT64);
                break;
            case OI_DATAELEM128:
                if (Element->ElemType == OI_DATAELEM_UUID) {
                    Element->Value.LongUUID = (OI_UUID128*) OI_Malloc(sizeof(OI_UUID128));
                    if (Element->Value.LongUUID == NULL) {
                        return OI_STATUS_OUT_OF_MEMORY;
                    }
                    ByteStream_GetUINT32_Checked(*ByteStream, Element->Value.LongUUID->ms32bits, DATAELEM_BO);
                    ByteStream_GetBytes_Checked(*ByteStream, Element->Value.LongUUID->base, sizeof(Element->Value.LongUUID->base));
                    Element->Size = sizeof(OI_UUID128);
                } else {
                    Element->Value.UInt128 = (OI_UINT128*) OI_Malloc(sizeof(OI_UINT128));
                    if (Element->Value.UInt128 == NULL) {
                        return OI_STATUS_OUT_OF_MEMORY;
                    }
                    ByteStream_GetUINT32_Checked(*ByteStream, Element->Value.UInt128->I1, DATAELEM_BO);
                    ByteStream_GetUINT32_Checked(*ByteStream, Element->Value.UInt128->I2, DATAELEM_BO);
                    ByteStream_GetUINT32_Checked(*ByteStream, Element->Value.UInt128->I3, DATAELEM_BO);
                    ByteStream_GetUINT32_Checked(*ByteStream, Element->Value.UInt128->I4, DATAELEM_BO);
                    Element->Size = sizeof(OI_UINT128);
                }
                break;
            case OI_DATAELEM_VAR8:
            case OI_DATAELEM_VAR16:
            case OI_DATAELEM_VAR32:
                switch (Element->ElemType) {
                    case OI_DATAELEM_SEQ:
                    case OI_DATAELEM_ALT:
                        status = UnmarshalListElement(ByteStream, elemSizeIndex, Element);
                        if (Element->Size) {
                            /*
                             * Step into the nested list
                             */
                            Element = Element->Value.ElemSeq;

                            /*
                             * continue the while(TRUE) loop
                             */
                            continue;
                        }
                        break;

                    case OI_DATAELEM_TEXT:
                    case OI_DATAELEM_URL:
                        status = UnmarshalTextElement(ByteStream, elemSizeIndex, Element);
                        break;
                    default:
                        status = OI_SDP_CORRUPT_DATA_ELEMENT;
                }
                break;
        }

        if (OI_SUCCESS(status) && ByteStream_Error(*ByteStream)) {
            status =  OI_SDP_CORRUPT_DATA_ELEMENT;
            OI_SLOG_ERROR(status, ("Unmarshal error"));
        }

ContinueUnmarshall:

        /*
         * If we have completed parsing the current element, we are done.
         */
        if (Element++ == pElement) {
            break;
        }
    }

    return status;
}

/**
 * This function unmarshals a data element of list type.
 */
static OI_STATUS UnmarshalListElement(OI_BYTE_STREAM *ByteStream,
                                      OI_UINT8 ElemSizeIndex,
                                      OI_DATAELEM *ListElem)
{
    OI_STATUS status = OI_OK;
    OI_DATAELEM *list;
    OI_UINT16 bytes;
    OI_UINT16 size;
    OI_UINT16 count;

    bytes = UnmarshalElementSize(ByteStream, ElemSizeIndex);
    if (ByteStream_Error(*ByteStream)) {
        status = OI_SDP_CORRUPT_DATA_ELEMENT;
        OI_SLOG_ERROR(status, ("UnmarshalListElement corrupt data element"));
        return status;
    }

    ListElem->Size = 0;
    ListElem->Value.ElemSeq = NULL;

    /*
     * An empty list is OK.
     */
    if (bytes > 0) {
        /*
         * Make a pass over the byte stream to count the data elements in the list.
         */
        count = CountListElements(ByteStream, bytes);
        if (ByteStream_Error(*ByteStream) || (count == 0)) {
            status = OI_SDP_CORRUPT_DATA_ELEMENT;
            OI_SLOG_ERROR(status, ("UnmarshalListElement corrupt data element"));
            return status;
        }
        /*
         * Allocate the element list, with one extra for back-track link
         */
        size = (count + 1) * sizeof(OI_DATAELEM);
        list = (OI_DATAELEM*) OI_Calloc(size);
        if (list == NULL) {
            return OI_STATUS_OUT_OF_MEMORY;
        }

        /*
         * Save data about the parent element of this list in order to backtrack
         * later.
         */
        list[count].ElemType = OI_DATAELEM_PARENT;
        list[count].Size = bytes;
        list[count].Value.ListParent = ListElem;
        ListElem->Size = count;
        ListElem->Value.ElemSeq = list;
    }

    return status;
}


/*
 * This function frees any memory that was allocated to store a data element
 * value. This includes crawling sequence trees, freeing nested elements.
 *
 * This function may be called with a data element list that has only been
 * partially initialized due to an error status during the unmarshalling or
 * cloning. Therefore, this function needs to check for non-null values when
 * freeing variable-length value data.
 */
static void FreeElementValue(OI_DATAELEM *pElement)
{
    OI_DATAELEM *Element;
    OI_DATAELEM *Last;
    OI_INT       size;
    OI_BOOL      nested;

    do {
        Last = Element = pElement;
        nested = FALSE;
        size = 1;

        while (size--) {
            switch (Element->ElemType) {
                case OI_DATAELEM_NULL:
                case OI_DATAELEM_BOOL:
                    break;
                case OI_DATAELEM_UINT:
                case OI_DATAELEM_SINT:
                case OI_DATAELEM_UUID:
                    if (Element->Size <= sizeof(OI_UINT32)) {
                        break;
                    }
                    /* Falling through */
                case OI_DATAELEM_URL:
                case OI_DATAELEM_TEXT:
                    OI_FreeIf(&Element->Value.RawBytes);
                    break;
                case OI_DATAELEM_SEQ:
                case OI_DATAELEM_ALT:
                    if (Element->Value.ElemSeq != NULL) {
                        nested = TRUE;
                        Last = Element;
                        size = Element->Size;
                        Element = Element->Value.ElemSeq;
                        continue;
                    }
                    break;
                case OI_DATAELEM_REF:
                    OI_ASSERT_FAIL("Freeing OI_DATAELEM_REF not allowed!");
                default:
                    /*
                     * If we received a malformed packet from the remote side, we may
                     * get to here. However, no additional memory will have been
                     * malloc-ed, so we can silently return.
                     */
                    break;
            }

            /*
             * Advance to next element in sequence
             */
            Element++;
        }

        if (nested) {
            /*
             * Remove deepest sequence and return to be called again
             */
            OI_Free(Last->Value.ElemSeq);
        }
        OI_SET_NULL_ELEMENT(*Last);

        /*
         * Continue to loop while nested
         */
    } while (nested);
}

/**
 * This function reads a marshalled data element from a byte stream and
 * unmarshals the element into dynamically allocated memory. The caller should
 * call OI_DataElement_Free() when done with the unmarshalled element.
 *
 * Data element lists are allocated as contiguous element lists.
 */
OI_STATUS OI_DataElement_Unmarshal(OI_BYTE_STREAM *ByteStream,
                                   OI_DATAELEM *Element)
{
    OI_STATUS status;

    OI_ASSERT(Element != NULL);

    /*
     * Ensure that the element is initialized in case it needs to be freed.
     */
    OI_SET_NULL_ELEMENT(*Element);

    status = UnmarshalElement(ByteStream, Element);
    if (!OI_SUCCESS(status)) {
        OI_SLOG_ERROR(status, ("OI_DataElement_Unmarshal error status %!", status));
        FreeElementValue(Element);
    }
    return status;
}


/**
 * Recursively copy a data element tree.
 */
OI_STATUS OI_DataElement_Clone(OI_DATAELEM       *pToElem,
                               const OI_DATAELEM *fromElem)
{
    OI_STATUS status = OI_OK;
    OI_DATAELEM *toElem = pToElem;

    OI_SET_NULL_ELEMENT(*pToElem);

    /*
     * Clone as long as nothing bad has happened
     */
    while (OI_SUCCESS(status)) {
        /*
         * If the current Element is a Parent Pointer, we are at the end of
         * parsing a list and must pop back to the parent list Element. If
         * the parent list element is the original passed in element we are
         * done. Otherwise we are in an enclosing (nested) list, and must
         * continue parsing the next element in that list.
         */
        while (toElem->ElemType == OI_DATAELEM_PARENT) {
            fromElem = toElem[1].Value.ListParent;
            fromElem++;
            toElem = toElem->Value.ListParent;
            if (toElem == pToElem) {
                return status;
            } else {
                toElem++;
            }
        }

        OI_MemCopy(toElem, fromElem, sizeof(OI_DATAELEM));

        switch (fromElem->ElemType) {
            case OI_DATAELEM_NULL:
            case OI_DATAELEM_BOOL:
                break;
            case OI_DATAELEM_UINT:
            case OI_DATAELEM_SINT:
            case OI_DATAELEM_UUID:
                if (fromElem->Size <= sizeof(OI_UINT32)) {
                    break;
                }
                toElem->Value.RawBytes = OI_Malloc(fromElem->Size);
                if (toElem->Value.RawBytes == NULL) {
                    status = OI_STATUS_OUT_OF_MEMORY;
                } else {
                    OI_MemCopy(toElem->Value.RawBytes, fromElem->Value.RawBytes, fromElem->Size);
                }
                break;
            case OI_DATAELEM_URL:
            case OI_DATAELEM_TEXT:
                /* Allocate extra byte to allow NULL termination */
                toElem->Value.Text = OI_Malloc(fromElem->Size + 1);
                if (toElem->Value.Text == NULL) {
                    status = OI_STATUS_OUT_OF_MEMORY;
                } else {
                    OI_MemCopy(toElem->Value.Text, fromElem->Value.Text, fromElem->Size);
                    toElem->Value.Text[toElem->Size] = 0;
                }
                break;
            case OI_DATAELEM_SEQ:
            case OI_DATAELEM_ALT:
                if (fromElem->Value.ElemSeq == NULL) {
                    break;
                }
                if (fromElem->Size != 0) {
                    toElem->Value.ElemSeq = OI_Calloc(sizeof(OI_DATAELEM) * (fromElem->Size + 2));
                    if (toElem->Value.ElemSeq == NULL) {
                        status = OI_STATUS_OUT_OF_MEMORY;
                    } else {
                        /*
                         * Store track-back pointers
                         */
                        toElem->Value.ElemSeq[fromElem->Size].ElemType = OI_DATAELEM_PARENT;
                        toElem->Value.ElemSeq[fromElem->Size].Value.ListParent = toElem;
                        toElem->Value.ElemSeq[fromElem->Size+1].Value.__ListParent = fromElem;
                        toElem = toElem->Value.ElemSeq;
                        fromElem = fromElem->Value.ElemSeq;

                        /*
                         * Continue with while loop to avoid advancing to next element.
                         */
                        continue;
                    }
                } else {
                    /*
                     * Empty Sequence OK
                     */
                    toElem->Value.ElemSeq = NULL;
                }
                break;
            default:
                toElem->ElemType = OI_DATAELEM_NULL;
                status = OI_STATUS_DATA_ERROR;
                break;
        }

        /*
         * If just completely cloned object is our
         * original clone, we are done.
         */
        fromElem++;
        if (toElem == pToElem) {
            break;
        } else {
            toElem++;
        }
    }

    if (!OI_SUCCESS(status)) {
        /*
         * Clean up everything cloned up to now.
         */
        FreeElementValue(pToElem);
    }
    return status;
}


void OI_DataElement_Transfer(OI_DATAELEM *toElem,
                             OI_DATAELEM *fromElem)
{
    *toElem = *fromElem;
    OI_SET_NULL_ELEMENT(*fromElem);
}


/**
 * This function recursively frees memory allocated for a data element.
 * The data element itself is not freed.
 */
void OI_DataElement_Free(OI_DATAELEM *Element)
{
    OI_ASSERT(Element != NULL);
    FreeElementValue(Element);
}


/*************************************************************************/
