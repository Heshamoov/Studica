#pragma once

#include <frc/PWM.h>
#include <frc/SpeedController.h>

// extern "C" {
namespace studica {
    class Servo;
}

/** 
 * @brief Servo classs c++ documentation
 * 
 * @author Studica Limited
 */
class studica::Servo : public frc::PWM {
    public:
        /** 
         * Constructor for Servo
         * @param channel The channel that the servo is plugged into
         */
        explicit Servo(int channel);

        /**
         * Copy constructor for Servo
         */
        Servo(Servo&&) = default;

        /**
         * Equivalet operator overload for servo class
         */
        Servo& operator=(Servo&&) = default;

        /**
         * Sets the position of the servo
         * @param value Servo position value, range from 0.0 to 1.0
         */
        void Set(double value);

        /**
         * Sets the servo to offline
         */
        void SetOffline();

        /**
         * Gets the servo position
         * 
         * @return Position from 0.0 to 1.0
         */
        double Get() const;

        /**
         * Sets the servo angle
         * @param angle The angle in degrees to set the servo
         */
        void SetAngle(double angle);

        /** 
         * Gets the servo angle
         * 
         * @return The angle in degrees to which the servo is set
         */
        double GetAngle() const;

        /**
         * Gets the maximum angle of the servo
         * 
         * @return The maximum angle of the servo in degrees
         */
        double GetMaxAngle() const;

        /** 
         * Gets the minimum angle of the servo
         * 
         * @return The minimum angle of the servo in degrees
         */
        double GetMinAngle() const;

        void InitSendable(frc::SendableBuilder& builder) override;

        private:
        double GetServoAngleRange() const;

        static constexpr double kMaxServoAngle = 300.0;
        static constexpr double kMinServoAngle = 0.0;

        static constexpr double kDefaultMaxServoPWM = 2.5;
        static constexpr double kDefaultMinServoPWM = 0.5;
};
// }