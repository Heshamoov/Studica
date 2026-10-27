#pragma once

#define TITAN_INITIALIZE_ERROR 1000
#define TITAN_INITIALIZE_MESSAGE "Titan could not be initialized"
#define TITAN_FREQ_OUT_OF_RANGE 1001
#define TITAN_FREQ_OUT_OF_RANGE_MESSAGE \
    "Motor frequency out of range [0;20000]"
#define TITAN_ID_OUT_OF_RANGE 1002
#define TITAN_ID_OUT_OF_RANGE_MESSAGE \
    "Device ID out of range [1;63]"

#define TITAN_GET_ID_ERROR 1010
#define TITAN_GET_ID_ERROR_MESSAGE \
    "Device ID could not be obtained"
#define TITAN_GET_FREQ_ERROR 1011
#define TITAN_GET_FREQ_ERROR_MESSAGE \
    "Motor frequency could not be obtained"
#define TITAN_GET_FW_VER_ERROR 1012
#define TITAN_GET_FW_VER_ERROR_MESSAGE \
    "Firmware version could not be obtained"
#define TITAN_GET_HW_VER_ERROR 1013
#define TITAN_GET_HW_VER_ERROR_MESSAGE \
    "Hardware version could not be obtained"
#define TITAN_GET_CURRENT_ERROR 1014
#define TITAN_GET_CURRENT_ERROR_MESSAGE \
    "Motor current could not be obtained"
#define TITAN_GET_CONTROLLERTEMP_ERROR 1015
#define TITAN_GET_CONTROLLERTEMP_ERROR_MESSAGE \
    "Controller temp could not be obtained"
#define TITAN_GET_LIMITSWITCH_ERROR 1016
#define TITAN_GET_LIMITSWITCH_ERROR_MESSAGE \
    "Limit Switch could not be obtained"
#define TITAN_GET_RPM_0_ERROR 1017
#define TITAN_GET_RPM_0_ERROR_MESSAGE \
    "RPM for motor 0 could not be obtained"
#define TITAN_GET_RPM_1_ERROR 1018
#define TITAN_GET_RPM_1_ERROR_MESSAGE \
    "RPM for motor 1 could not be obtained"
#define TITAN_GET_RPM_2_ERROR 1019
#define TITAN_GET_RPM_2_ERROR_MESSAGE \
    "RPM for motor 2 could not be obtained"
#define TTIAN_GET_RPM_3_ERROR 1020
#define TITAN_GET_RPM_3_ERROR_MESSAGE \
    "RPM for motor 3 could not be obtained"
#define TITAN_GET_SERIAL_WORD_1_ERROR 1021
#define TITAN_GET_SERIAL_WORD_1_ERROR_MESSAGE \
    "Serial number word 1 could not be obtained"
#define TITAN_GET_SERIAL_WORD_2_ERROR 1022
#define TITAN_GET_SERIAL_WORD_2_ERROR_MESSAGE \
    "Serial number word 2 could not be obtained"
#define TITAN_GET_SERIAL_WORD_3_ERROR 1023
#define TITAN_GET_SERIAL_WORD_3_ERROR_MESSAGE \
    "Serial number word 3 could not be obtained"

#define TITAN_GET_ENC_DST_0_ERROR 1100
#define TITAN_GET_ENC_DST_0_ERROR_MESSAGE \
    "Encoder distance for encoder 0 could not be obtained"
#define TITAN_GET_ENC_DST_1_ERROR 1101
#define TITAN_GET_ENC_DST_1_ERROR_MESSAGE \
    "Encoder distance for encoder 1 could not be obtained"
#define TITAN_GET_ENC_DST_2_ERROR 1102
#define TITAN_GET_ENC_DST_2_ERROR_MESSAGE \
    "Encoder distance for encoder 2 could not be obtained"
#define TITAN_GET_ENC_DST_3_ERROR 1103
#define TITAN_GET_ENC_DST_3_ERROR_MESSAGE \
    "Encoder distance for encoder 3 could not be obtained"
#define TITAN_GET_ENC_RAW_0_ERROR 1104
#define TITAN_GET_ENC_RAW_0_ERROR_MESSAGE \
    "Raw distance for encoder 0 could not be obtained"
#define TITAN_GET_ENC_RAW_1_ERROR 1105
#define TITAN_GET_ENC_RAW_1_ERROR_MESSAGE \
    "Raw distance for encoder 1 could not be obtained"
#define TITAN_GET_ENC_RAW_2_ERROR 1106
#define TITAN_GET_ENC_RAW_2_ERROR_MESSAGE \
    "Raw distance for encoder 2 could not be obtained"
#define TITAN_GET_ENC_RAW_3_ERROR 1107
#define TITAN_GET_ENC_RAW_3_ERROR_MESSAGE \
    "Raw distance for encoder 3 could not be obtained"
#define TITAN_GET_ENC_SPEED_0_error 1108
#define TITAN_GET_ENC_SPEED_0_ERROR_MESSAGE \
    "Speed for encoder 0 could not be obtained"
#define TITAN_GET_ENC_SPEED_1_error 1109
#define TITAN_GET_ENC_SPEED_1_ERROR_MESSAGE \
    "Speed for encoder 1 could not be obtained"
#define TITAN_GET_ENC_SPEED_2_error 1110
#define TITAN_GET_ENC_SPEED_2_ERROR_MESSAGE \
    "Speed for encoder 2 could not be obtained"
#define TITAN_GET_ENC_SPEED_3_error 1111
#define TITAN_GET_ENC_SPEED_3_ERROR_MESSAGE \
    "Speed for encoder 3 could not be obtained"


#define COBRA_NOT_CONNECTED_ERROR 2001
#define COBRA_NOT_CONNECTED_ERROR_MESSAGE \
    "I2C device not connected"
#define COBRA_IS_CONNECTED 2002
#define COBRA_IS_CONNECTED_MESSAGE \
    "I2C device is connected"
#define COBRA_READ_COUNT_OUT_OF_RANGE 2010
#define COBRA_READ_COUNT_OUT_OF_RANGE_MESSAGE \
    "Read count bytes out of range [1;)"
#define COBRA_READ_BUFFER_NULL_PARAMETER 2011
#define COBRA_READ_BUFFER_NULL_PARAMETER_MESSAGE \
    "Read buffer is null"


