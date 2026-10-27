#pragma once

#include <iostream>
#include <iomanip>
#include <string>
#include <numeric>
#include <iterator>
#include <cstdint>
#include <thread>
#include <sstream>

#include "hal/CANAPITypes.h"
#include "hal/Types.h"
#include "hal/Errors.h"
#include "hal/CANAPI.h"
#include "hal/HALBase.h"
#include "hal/CAN.h"

#include "TitanConstants.h"
#include "ErrorCodes.h"

class Titan {

    public: 
        static void delay(long handle, double seconds);
        static const char* getErrorMessage(int32_t errorCode);

        static void Run(long handle, bool stop, int32_t deviceID);
        static void Stop(bool &stop);

        static void sendMessage(uint32_t messageID, uint8_t* data, int32_t deviceID);
};
