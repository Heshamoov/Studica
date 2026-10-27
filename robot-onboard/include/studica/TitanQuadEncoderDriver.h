#pragma once

#include <climits>

#include "TitanConstants.h"
#include "TitanQuadDriver.h"
#include "Titan.h"

extern "C" {

class TitanQuadEncoderJNI {
    public:
        static void c_TitanQuadEncoder_setEncoder(long handle, double distPerTick,
                                        double& encoderDistPerTick);
        static double c_TitanQuadEncoder_getEncoderDistance(long handle, int8_t encoder,
                                        double encoderDistPerTick, uint32_t encoderTicks, double& distance);
        static void c_TitanQuadEncoder_resetEncoder(long handle, int8_t encoder);
        static double c_TitanQuadEncoder_getRaw(long handle, int8_t encoder);
        static short c_TitanQuadEncoder_getSpeed(long handle, int8_t encoder);
};
}