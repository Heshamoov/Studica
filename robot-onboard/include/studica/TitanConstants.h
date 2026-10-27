#pragma once
#include "hal/CANAPITypes.h"

class TitanMessageID {

    public:
    
    /**
     * Manufacture ID and Device types
     */

    static constexpr int MANUFACTURE_ID             = 786432;
    static constexpr int DEVICE_TYPE                = 33554432;

    /**
     * API ID's
     */

    static constexpr int DISABLED_FLAG            = MANUFACTURE_ID + DEVICE_TYPE + (64*0);
    static constexpr int ENABLED_FLAG             = MANUFACTURE_ID + DEVICE_TYPE + (64*1);
    static constexpr int SET_MOTOR_SPEED          = MANUFACTURE_ID + DEVICE_TYPE + (64*2);
    static constexpr int DISABLE_MOTOR            = MANUFACTURE_ID + DEVICE_TYPE + (64*3);         
    static constexpr int GET_TITAN_INFO           = MANUFACTURE_ID + DEVICE_TYPE + (64*4);        
    static constexpr int RETURN_TITAN_INFO        = 5;
    static constexpr int GET_UNIQUE_ID            = MANUFACTURE_ID + DEVICE_TYPE + (64*6);
    static constexpr int RETURN_WORD_1            = 7;
    static constexpr int RETURN_WORD_2            = 8;
    static constexpr int RETURN_WORD_3            = 9;
    static constexpr int CONFIGURE_MOTOR          = MANUFACTURE_ID + DEVICE_TYPE + (64*10);
    static constexpr int GET_MOTOR_FREQUENCY      = MANUFACTURE_ID + DEVICE_TYPE + (64*11);
    static constexpr int RETURN_MOTOR_FREQUENCY   = 12;
    static constexpr int RESET_ENCODER            = MANUFACTURE_ID + DEVICE_TYPE + (64*13);
    static constexpr int SET_CURRENT_LIMIT        = MANUFACTURE_ID + DEVICE_TYPE + (64*14);
    static constexpr int SET_MOTOR_STOP_MODE      = MANUFACTURE_ID + DEVICE_TYPE + (64*15);
    static constexpr int SET_TARGET_VELOCITY      = MANUFACTURE_ID + DEVICE_TYPE + (64*16);
    static constexpr int SET_TARGET_DISTANCE      = MANUFACTURE_ID + DEVICE_TYPE + (64*17);

    static constexpr int CYPHER_OUTPUT            = 36;
    static constexpr int ENCODER_0_OUTPUT         = 37;
    static constexpr int ENCODER_1_OUTPUT         = 38;
    static constexpr int ENCODER_2_OUTPUT         = 39;
    static constexpr int ENCODER_3_OUTPUT         = 40;
    static constexpr int RPM_0_OUTPUT             = 41;
    static constexpr int RPM_1_OUTPUT             = 42;
    static constexpr int RPM_2_OUTPUT             = 43;
    static constexpr int RPM_3_OUTPUT             = 44;
    static constexpr int LIMIT_SWITCH_OUTPUT      = 45;
    static constexpr int CURRENT_OUTPUT           = MANUFACTURE_ID + DEVICE_TYPE + (64*46);
    static constexpr int MCU_TEMP                 = 47;
    static constexpr int MAX_RANGE                = MANUFACTURE_ID + DEVICE_TYPE + (64*1023);
};