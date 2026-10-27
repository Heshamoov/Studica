#pragma once

#include <iostream>
#include <cstdint>

// extern "C" {
namespace studica {
    class Cobra;
}

/** 
 * @brief Cobra class c++ documentation
 * 
 * @author Studica Limited
 */
class studica::Cobra {
    private:

        float vRef;

    public:

        /**
         * Default constructor for Cobra
         */
        Cobra();

        /**
         * Constructor for Cobra 
         * @param _vRef Referenced voltage
         */
        Cobra(float _vRef);

        /**
         * Closes Cobra
         */
        ~Cobra();

        /**
         * Gets the raw value of the sensor
         * @param channel The channel to read from 
         * 
         * @return The raw value of the sensor
         */
        int GetRawValue(int channel);
        
        /**
         * Gets the voltage of the sensor
         * @param chaannel The channel to read from
         * 
         * @return The voltage of the sensor 
         */
        float GetVoltage(int channel);
};
// }