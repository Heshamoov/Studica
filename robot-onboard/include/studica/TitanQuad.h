#pragma once

#include <iostream>
#include <cstdint>
#include <frc/SpeedController.h>


// extern "C" {
namespace studica {
    class TitanQuad;
    class TitanQuadEncoder;
}

/** @brief TitanQuad class testing c++ documentation.
 * 
 * @author Studica Limited
 * @date April 2020
 */


class studica::TitanQuad : public frc::SpeedController{

    private: 
        int canID;
        std::string fw_version;
        std::string hw_version;
        double speed;
        long handle;
        int motor;
        bool isInverted = false;
        bool invertRPM = false;
        bool stop;
        int frequency = 15600;
        friend class TitanQuadEncoder;

    public:



        /**
         * Constructor for Titan Motor Controller
         * @param deviceID Device id for the controller 
         * @param motor_set The motor to initialize
         */

        TitanQuad(int deviceID, int motor_set);

        /**
         * Constructor for Titan Motor Controller
         * <p>
         * Allows for a custom frequency to be set
         * @param deviceID Device id for the controller
         * @param frequency_set Frequency internal motor pwm operates at, range 0 - 20kHz
         * @param motor_set THe motor to initialize
         */
        TitanQuad(int deviceID, int frequency_set, int motor_set);
        

        /** 
         * Closes Titan.
         */
        ~TitanQuad();

        /**
         * Gets the device id
         * @return the device id 
         */
        int GetID();

        /**
         * Gets the frequency that the motor internal PWM is operating at
         * @return frequency in Hz
         */
        int GetFrequency();

        /**
         * Gets the firmware version 
         * @return a string with the firmware version
         */
        std::string GetFirmwareVersion();

        /**
         * Gets the hardware and hardware version
         * @return a string with hardware type and version number
         */
        std::string GetHardwareVersion();

        // /**
        //  * Grabs the specified motors current pull
        //  * @param motor motor to be checked
        //  * @return current in amps
        //  */
        // double GetCurrent();

        /**
         * Gets controller temp value
         * @return controller temp calue
         */
        double GetControllerTemp();

        /**
         * Gets the Cypher max angle position from the titan
         * @return position in degrees
         */
        double GetCypherPosition();

        /**** Speed Controller Interface ****/

        /**
         * Common interface for setting the speed of a speed controller.
         *
         * @param speed The speed to set.  Value should be between -1.0 and 1.0.
         */
        void Set(double speed) override;

        double Get() const override;

        void SetInverted(bool isInverted) override;

        bool GetInverted() const override;

        void Disable() override;
        
        void StopMotor() override;

        void PIDWrite(double output) override;

        // /**
        //  * set current limit
        //  * @param motor - description
        //  */
        // void SetCurrentLimit();

        /**
         * get limit switch
         * @param motor - description
         * @param direction - forward or reverse
         * @return limit switch
         */
        double GetLimitSwitch(bool direction);

        void InvertRPM();

        /**
         * Gets the round per minute of encoder
         */
        int GetRPM();

        // /**
        //  * get faults
        //  * @param motor - description
        //  * @return faults
        //  */
        // int GetFaults();

        // /**
        //  * clear faults
        //  * @param motor - description
        //  */
        // void ClearFaults();

        // /**
        //  * enable soft limit
        //  * @param motor - description
        //  * @return enabled or disabled
        //  */
        // bool EnableSoftLimit();

    /**
     * Serial Number is required for any support requesets 
     * @return Serial Number 
     */
        std::string GetSerialNumber();
}; 