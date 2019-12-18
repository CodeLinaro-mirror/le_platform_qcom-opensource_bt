/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
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

#ifndef TRUNCATEDFILTER_HPP_
#define TRUNCATEDFILTER_HPP_

#pragma once

#include <vector>
#include <iostream>

using namespace std;
using std::string;

namespace gatt{
class ScanFilter;
class ResultStorageDescriptor;

/**
 * A special scan filter that lets the client decide how the scan record should be stored.
 *
 */
class TruncatedFilter {
  private:
    const ScanFilter *mFilter;
    const std::vector<ResultStorageDescriptor*> mStorageDescriptors;

  public:
    /**
     * Constructor for TruncatedFilter.
     *
     * @param filter Scan filter of the truncated filter.
     * @param storageDescriptors Describes how the scan should be stored.
     */
    TruncatedFilter(ScanFilter *filter, std::vector<ResultStorageDescriptor*> storageDescriptors);

    /**
     * Returns the scan filter.
     */
    ScanFilter* getFilter();

    /**
     * Returns a list of descriptor for scan result storage.
     */
    std::vector<ResultStorageDescriptor*> getStorageDescriptors();
};

}//namespace gatt
#endif
