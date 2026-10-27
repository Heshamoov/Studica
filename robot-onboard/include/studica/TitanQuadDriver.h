#pragma once

#include "TitanConstants.h"
#include "Titan.h"

extern "C"{ 

class TitanQuadJNI{

    private:
        static std::thread * task;
        friend class TitanQuadEncoderJNI;
        static int deviceID;

    public:
        static long c_TitanQuad_create(int16_t deviceID, int16_t frequency, bool &stop, int8_t motor);
        static void c_TitanQuad_destroy(long handle);
        static int c_TitanQuad_getID(long handle);
        static int c_TitanQuad_getFrequency(long handle);

        static std::string c_TitanQuad_getFirmwareVersion(long handle);
        static std::string c_TitanQuad_getHardwareVersion(long handle);

        static void c_TitanQuad_configureMotor(long handle, int frequency, int8_t motor);
        static void c_TitanQuad_disableMotor(long handle, int8_t motor);
        // static double c_TitanQuad_getCurrent(long handle, int8_t motor);
        static double c_TitanQuad_getControllerTemp(long handle);
        static void c_TitanQuad_setMotorSpeed(long handle, int8_t motor, double speed);
        // static void c_TitanQuad_setCurrentLimit(long handle, int8_t motor);
        static int c_TitanQuad_getLimitSwitch(long handle, int8_t motor, bool direction);
        static short c_TitanQuad_getRPM(long handle, int8_t motor);
        // static int c_TitanQuad_getFaults(long handle, int8_t motor);
        // static void c_TitanQuad_clearFaults(long handle, int8_t motor);
        // static bool c_TitanQuad_enableSoftLimit(long handle, int8_t motor);
        static std::string c_TitanQuad_getSerialNumber(long handle);
        static double c_TitanQuad_getCypherPosition(long handle, int8_t port);
}; 
} 

