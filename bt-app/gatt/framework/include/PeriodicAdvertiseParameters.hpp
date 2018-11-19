/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef PERIODICADVERTISEPARAMETERS_HPP_
#define PERIODICADVERTISEPARAMETERS_HPP_

#pragma once
using namespace std;
using std::string;

namespace gatt{
  /**
   * The PeriodicAdvertiseParameters provide a way to adjust periodic
   * advertising preferences for each Bluetooth LE advertising set.
   * AdvertisingSetParameters.Builder to create an instance of this class.
   */
class PeriodicAdvertiseParameters final {
  private:
      static const int INTERVAL_MIN = 80;
      static const int INTERVAL_MAX = 65519;

      const bool mIncludeTxPower;
      const int mInterval;

      PeriodicAdvertiseParameters(bool includeTxPower, int interval);

  public:
      /**
       * Returns whether the TX Power will be included.
       */
      bool getIncludeTxPower();

      /**
       * Returns the periodic advertising interval, in 1.25ms unit.
       * Valid values are from 80 (100ms) to 65519 (81.89875s).
       */
      int getInterval();


      class Builder {
        private:
          bool mIncludeTxPower = false;
          int mInterval = INTERVAL_MAX;

        public:
          /**
           * Whether the transmission power level should be included in the periodic
           * packet.
           */
          Builder setIncludeTxPower(bool includeTxPower);

          /**
           * Set advertising interval for periodic advertising, in 1.25ms unit.
           * Valid values are from 80 (100ms) to 65519 (81.89875s).
           * Value from range [interval, interval+20ms] will be picked as the actual value.
           *
           * @throws std::invalid_argument If the interval is invalid.
           */
          Builder setInterval(int interval);

          /**
           * Build the AdvertisingSetParameters object.
           */
          PeriodicAdvertiseParameters* build();
      };
  };
}//namespace gatt
#endif
