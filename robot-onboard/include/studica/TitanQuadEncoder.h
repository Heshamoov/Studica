#pragma once

#include "TitanQuad.h"
#include <frc/CounterBase.h>

// extern "C" {
/**
 * @brief TitanQuad Encoder class c++ documentation
 * 
 * @author Studica Limited
 */

class studica::TitanQuadEncoder : frc::CounterBase{

    private:
        long handle;
        int encoder;
        double encoderDistPerTick;
        int encoderTicks;
        double distance; 
        bool reverseDirection;
        bool invertSpeed;

    
    public:
        /**
         * Constructor for encoders
         * @param titan - The reference to the TitanQuad encoders that are being initialize
         * @param encoder - The encoder
         * @param distPerTick - Distance per tick value for encoder 0
         */
        TitanQuadEncoder(TitanQuad &titan, int encoder, double distPerTick);



        int Get() const override;

        void Reset() override;

        double GetPeriod() const override;

        void SetMaxPeriod(double maxPeriod) override;

        bool GetStopped() const override;


        /**
         * Will give the distance traveled based on encoder data
         * @param encoder encoder to be used
         * @return distance traveled
         */
        double GetEncoderDistance();

        /**
         * Gets the raw count of encoder
         */
        double GetRaw();

        void InvertSpeed();

        /**
         * Gets the speed of the encoder
         */
        double GetSpeed();

        /**
         * Gets the direction of the encoder
         */
        bool GetDirection() const override;

        /**
         * Sets the direction of the encoder
         */
        void SetReverseDirection();
};