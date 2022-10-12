/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"){}
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include<vector>
#include<iostream>
#include<list>
#include<cmath>
#include<cstring>
#include "GattCharacteristic.hpp"

using namespace std;
namespace gatt{

const float GattCharacteristic::INVALID_FLOAT = -1.0f;
GattCharacteristic::~GattCharacteristic()
{
  if ( mService != NULL) {
    mService = NULL;
  }

  if (mValue != NULL ) {
    delete[] mValue;
    mValue = NULL;
  }

  for (GattDescriptor *svc_des: mDescriptors)
    delete svc_des;

  mDescriptors.clear();
}

void GattCharacteristic::initCharacteristic(GattService *service, Uuid uuid, int instanceId,
                                                  int properties, int permissions)
{
  mUuid = uuid;
  mInstance = instanceId;
  mProperties = properties;
  mPermissions = permissions;
  mService = service;
  if ((mProperties & PROPERTY_WRITE_NO_RESPONSE) != 0) {
    mWriteType = WRITE_TYPE_NO_RESPONSE;
  } else {
    mWriteType = WRITE_TYPE_DEFAULT;
  }
}

GattCharacteristic::GattCharacteristic(Uuid uuid, int properties, int permissions)
{
  initCharacteristic(NULL, uuid, 0, properties, permissions);
}

GattCharacteristic::GattCharacteristic(Uuid uuid, int instanceId,
                                             int properties, int permissions)
{
  initCharacteristic(NULL, uuid, instanceId, properties, permissions);
}

bool GattCharacteristic::addDescriptor(GattDescriptor *descriptor)
{
  mDescriptors.push_back(descriptor);
  descriptor->setCharacteristic(this);
  return true;
}

GattDescriptor* GattCharacteristic::getDescriptor(Uuid uuid, int instanceId)
{
  for (GattDescriptor *descriptor : mDescriptors) {
    if (descriptor->getUuid() == uuid && descriptor->getInstanceId() == instanceId) {
      return descriptor;
    }
  }
  return NULL;
}

GattService* GattCharacteristic::getService()
{
  return mService;
}

void GattCharacteristic::setService(GattService *service)
{
  mService = service;
}

Uuid GattCharacteristic::getUuid()
{
  return mUuid;
}

int GattCharacteristic::getInstanceId()
{
  return mInstance;
}

void GattCharacteristic::setInstanceId(int instanceId)
{
  mInstance = instanceId;
}

int GattCharacteristic::getProperties()
{
  return mProperties;
}

int GattCharacteristic::getPermissions()
{
  return mPermissions;
}

int GattCharacteristic::getWriteType()
{
  return mWriteType;
}

void GattCharacteristic::setWriteType(int writeType)
{
  mWriteType = writeType;
}

void GattCharacteristic::setKeySize(int keySize)
{
  mKeySize = keySize;
}

int GattCharacteristic::getKeySize()
{
    return mKeySize;
}
std::vector<GattDescriptor*> GattCharacteristic::getDescriptors()
{
  return mDescriptors;
}

GattDescriptor* GattCharacteristic::getDescriptor(Uuid uuid)
{
  for (GattDescriptor *descriptor : mDescriptors) {
    if (descriptor->getUuid() == uuid) {
      return descriptor;
    }
  }
  return NULL;
}

uint8_t* GattCharacteristic::getValue()
{
  return mValue;
}

int GattCharacteristic::getValueLength()
{
  return mValueLength;
}

int GattCharacteristic::getIntValue(int formatType, int offset)
{
  if ((offset + getTypeLen(formatType)) > mValueLength) return INVALID_INT;

  switch (formatType) {
    case FORMAT_UINT8:
      return unsignedByteToInt(mValue[offset]);

    case FORMAT_UINT16:
      return unsignedBytesToInt(mValue[offset], mValue[offset + 1]);

    case FORMAT_UINT32:
      return unsignedBytesToInt(mValue[offset], mValue[offset + 1],
                mValue[offset + 2], mValue[offset + 3]);
    case FORMAT_SINT8:
      return unsignedToSigned(unsignedByteToInt(mValue[offset]), 8);

    case FORMAT_SINT16:
      return unsignedToSigned(unsignedBytesToInt(mValue[offset],
                mValue[offset + 1]), 16);

    case FORMAT_SINT32:
      return unsignedToSigned(unsignedBytesToInt(mValue[offset],
                mValue[offset + 1], mValue[offset + 2], mValue[offset + 3]), 32);
  }

  return INVALID_INT;
}

float GattCharacteristic::getFloatValue(int formatType, int offset)
{
  if ((offset + getTypeLen(formatType)) > mValueLength) return INVALID_FLOAT;

  switch (formatType) {
    case FORMAT_SFLOAT:
      return bytesToFloat(mValue[offset], mValue[offset + 1]);

    case FORMAT_FLOAT:
      return bytesToFloat(mValue[offset], mValue[offset + 1],
                mValue[offset + 2], mValue[offset + 3]);
  }

  return INVALID_FLOAT;
}

string GattCharacteristic::getStringValue(int offset)
{
  if (mValue == NULL) return NULL;
  size_t mValueSize = strlen((char*)mValue);

  if (offset >mValueSize) return NULL;

  uint8_t strBytes[mValueLength - offset];
  for (int i = 0; i != (mValueLength - offset); ++i)
    strBytes[i] = mValue[offset + i];

  return std::string(strBytes, strBytes + mValueLength);
}

bool GattCharacteristic::setValue(uint8_t *value, int length)
{
  if (value == NULL || length == 0) {
    return false;
  }

  if(mValue != NULL)
    delete [] mValue;

  mValueLength = length;
  mValue = new uint8_t[mValueLength];
  if(!mValue) return false;
  std::memcpy(mValue, value, mValueLength);

  return true;
}

bool GattCharacteristic::setValue(int value, int formatType, int offset)
{
  int len = offset + getTypeLen(formatType);

  if (mValue == NULL) mValue = new uint8_t[len];
  if(!mValue) return false;
  if (len > sizeof(mValue)/sizeof(mValue[0]))
    return false;

  switch (formatType) {
    case FORMAT_SINT8:
      value = intToSignedBits(value, 8);
      // Fall-through intended
    case FORMAT_UINT8:
      mValue[offset++] = (uint8_t) (value & 0xFF);
      break;
    case FORMAT_SINT16:
      value = intToSignedBits(value, 16);
      // Fall-through intended
    case FORMAT_UINT16:
      mValue[offset++] = (uint8_t) (value & 0xFF);
      mValue[offset++] = (uint8_t) ((value >> 8) & 0xFF);
      break;
    case FORMAT_SINT32:
      value = intToSignedBits(value, 32);
      // Fall-through intended
    case FORMAT_UINT32:
      mValue[offset++] = (uint8_t) (value & 0xFF);
      mValue[offset++] = (uint8_t) ((value >> 8) & 0xFF);
      mValue[offset++] = (uint8_t) ((value >> 16) & 0xFF);
      mValue[offset++] = (uint8_t) ((value >> 24) & 0xFF);
      break;
    default:
      return false;
  }
  return true;
}

bool GattCharacteristic::setValue(int mantissa, int exponent, int formatType, int offset)
{
  int len = offset + getTypeLen(formatType);

  if (mValue == NULL) mValue = new uint8_t[len];
  if(!mValue) return false;
  if (len > sizeof(mValue)/sizeof(mValue[0]))
    return false;

  switch (formatType) {
    case FORMAT_SFLOAT:
      mantissa = intToSignedBits(mantissa, 12);
      exponent = intToSignedBits(exponent, 4);
      mValue[offset++] = (uint8_t) (mantissa & 0xFF);
      mValue[offset] = (uint8_t) ((mantissa >> 8) & 0x0F);
      mValue[offset] += (uint8_t) ((exponent & 0x0F) << 4);
      break;
    case FORMAT_FLOAT:
      mantissa = intToSignedBits(mantissa, 24);
      exponent = intToSignedBits(exponent, 8);
      mValue[offset++] = (uint8_t) (mantissa & 0xFF);
      mValue[offset++] = (uint8_t) ((mantissa >> 8) & 0xFF);
      mValue[offset++] = (uint8_t) ((mantissa >> 16) & 0xFF);
      mValue[offset] += (uint8_t) (exponent & 0xFF);
      break;
    default:
      return false;
  }
  return true;
}

bool GattCharacteristic::setValue(string value)
{
  if (mValue != NULL) delete [] mValue;

  mValue = new uint8_t[value.length() + 1];
  if(!mValue) return false;
  std::memcpy (mValue, value.data(), value.length());
  mValue[value.length()] = '\0';

  return true;
}

int GattCharacteristic::getTypeLen(int formatType)
{
  return formatType & 0xF;
}

int GattCharacteristic::unsignedByteToInt(uint8_t b)
{
  return b & 0xFF;
}

int GattCharacteristic::unsignedBytesToInt(uint8_t b0, uint8_t b1)
{
  return (unsignedByteToInt(b0) + (unsignedByteToInt(b1) << 8));
}

int GattCharacteristic::unsignedBytesToInt(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3)
{
  return (unsignedByteToInt(b0) + (unsignedByteToInt(b1) << 8))
              + (unsignedByteToInt(b2) << 16) + (unsignedByteToInt(b3) << 24);
}

float GattCharacteristic::bytesToFloat(uint8_t b0, uint8_t b1)
{
  int mantissa = unsignedToSigned(unsignedByteToInt(b0)
                + ((unsignedByteToInt(b1) & 0x0F) << 8), 12);
  int exponent = unsignedToSigned(unsignedByteToInt(b1) >> 4, 4);

  return (float) (mantissa * pow(10, exponent));
}

float GattCharacteristic::bytesToFloat(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3)
{
  int mantissa = unsignedToSigned(unsignedByteToInt(b0)
                + (unsignedByteToInt(b1) << 8)
                + (unsignedByteToInt(b2) << 16), 24);

  return (float) (mantissa * pow(10, b3));
}

int GattCharacteristic::unsignedToSigned(int un_signed, int size)
{
  if ((un_signed & (1 << size - 1)) != 0) {
    un_signed = -1 * ((1 << size - 1) - (un_signed & ((1 << size - 1) - 1)));
  }
  return un_signed;
}

int GattCharacteristic::intToSignedBits(int i, int size)
{
  if (i < 0) {
    i = (1 << size - 1) + (i & ((1 << size - 1) - 1));
  }
  return i;
}
}//namespace gatt
