#pragma once

#include <frc/PWMSpeedController.h>

// extern "C" {
namespace studica{
    class ServoContinuous;
}

class studica::ServoContinuous : public frc::PWMSpeedController {
    public:

        explicit ServoContinuous(int channel);
        ServoContinuous(ServoContinuous&&) = default;
        ServoContinuous& operator=(ServoContinuous&&) = default;
};
// }