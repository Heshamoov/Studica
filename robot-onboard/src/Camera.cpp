/*****************************************************************************
 * SC2026 Skill 23 - Camera colour classifier.
 *
 * HOW IT WORKS
 * 1. Opens the first USB camera (index 0) via OpenCV / cscore.
 * 2. Every 100 ms grabs a frame and averages the HSV of the centre 120x120
 *    region (the ROI). Averaging filters out camera noise without expensive
 *    per-pixel classification.
 * 3. Classifies the averaged hue/saturation into one of five colours. The
 *    thresholds below were derived from the SC2026 colour spec; re-tune them
 *    on the day if your camera's white balance shifts them.
 * 4. Pushes "colour", "hue", "sat", "val", and "confidence" to Shuffleboard
 *    so the judge can see the detection live.
 *
 * COLOUR THRESHOLDS (OpenCV HSV: H 0-180, S 0-255, V 0-255)
 *   red    hue < 10 or hue > 170, sat > 90
 *   green  hue 40-90,             sat > 80
 *   yellow hue 22-38,             sat > 100
 *   blue   hue 100-135,           sat > 80
 *   hazard any saturation < 40 AND val > 180  ->  "none"
 *          any colour that does not match the above -> "hazard"
 *          (the surprise hazard is added to the arena; if the camera sees
 *           something saturated but not red/green/yellow/blue, flag it)
 *
 * TUNING TIP
 * Run the robot in test mode and watch the hue/sat/val widgets while holding
 * each piece in front of the camera. Adjust the constants below until every
 * piece reads the correct label every time.
 *****************************************************************************/

#include "Camera.h"
#include "Constants.h"

#include <frc/smartdashboard/SmartDashboard.h>

// cscore / OpenCV are included with WPILib's vendordep.
#include <cscore.h>
#include <cscore_cv.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <chrono>
#include <thread>

// ---- colour thresholds (H: 0-180, S: 0-255, V: 0-255) ------------------
namespace {
    const double &RED_H_LO = constant::RED_H_LO;  const double &RED_H_HI = constant::RED_H_HI;
    const double &RED_H_LO2 = constant::RED_H_LO2;  const double &RED_H_HI2 = constant::RED_H_HI2;
    const double &RED_S_MIN = constant::RED_S_MIN;

    const double &GREEN_H_LO = constant::GREEN_H_LO; const double &GREEN_H_HI = constant::GREEN_H_HI;
    const double &GREEN_S_MIN = constant::GREEN_S_MIN;

    const double &YELLOW_H_LO = constant::YELLOW_H_LO; const double &YELLOW_H_HI = constant::YELLOW_H_HI;
    const double &YELLOW_S_MIN = constant::YELLOW_S_MIN;

    const double &BLUE_H_LO = constant::BLUE_H_LO; const double &BLUE_H_HI = constant::BLUE_H_HI;
    const double &BLUE_S_MIN = constant::BLUE_S_MIN;

    // Low saturation = white/grey/black → "none"
    const double &SAT_MIN_COLOUR = constant::SAT_MIN_COLOUR;

    // ROI: centre square of the frame
    constexpr int ROI_SIZE = 120;   // pixels
}

static std::string Classify(double h, double s, double /*v*/)
{
    if (s < SAT_MIN_COLOUR) return "none";

    if (s >= RED_S_MIN && (
            (h >= RED_H_LO  && h <= RED_H_HI) ||
            (h >= RED_H_LO2 && h <= RED_H_HI2)))
        return "red";

    if (s >= GREEN_S_MIN  && h >= GREEN_H_LO  && h <= GREEN_H_HI)  return "green";
    if (s >= YELLOW_S_MIN && h >= YELLOW_H_LO && h <= YELLOW_H_HI) return "yellow";
    if (s >= BLUE_S_MIN   && h >= BLUE_H_LO   && h <= BLUE_H_HI)   return "blue";

    // Saturated but not a known colour → the surprise hazard
    return "hazard";
}

// ---- Camera implementation ----------------------------------------------

Camera::Camera() = default;

Camera::~Camera()
{
    running_ = false;
    if (thread_.joinable()) thread_.join();
}

void Camera::Start()
{
    running_ = true;
    thread_  = std::thread([this] { Loop(); });
}

void Camera::Loop()
{
    cs::UsbCamera cam{"cam0", 0};
    cam.SetResolution(constant::CAM_WIDTH, constant::CAM_HEIGHT);
    cam.SetFPS(15);

    cs::CvSink sink{"sink"};
    sink.SetSource(cam);

    // Also put the stream on the Shuffleboard camera widget.
    cs::MjpegServer server{"stream", 1181};
    server.SetSource(cam);

    cv::Mat frame, hsv;

    while (running_)
    {
        uint64_t ts = sink.GrabFrame(frame);
        if (ts == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }

        cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

        // Centre ROI
        int cx = hsv.cols / 2, cy = hsv.rows / 2;
        int half = ROI_SIZE / 2;
        cx = std::max(half, std::min(hsv.cols - half - 1, cx));
        cy = std::max(half, std::min(hsv.rows - half - 1, cy));
        cv::Rect roi(cx - half, cy - half, ROI_SIZE, ROI_SIZE);
        cv::Mat region = hsv(roi);

        cv::Scalar mean = cv::mean(region);
        double h = mean[0], s = mean[1], v = mean[2];
        std::string col = Classify(h, s, v);

        {
            std::lock_guard<std::mutex> lk(mutex_);
            hue_ = h; sat_ = s; val_ = v;
            colour_ = col;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Camera::Publish()
{
    std::string col;
    double h, s, v;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        col = colour_; h = hue_; s = sat_; v = val_;
    }
    frc::SmartDashboard::PutString("colour",     col);
    frc::SmartDashboard::PutNumber("colour_hue", h);
    frc::SmartDashboard::PutNumber("colour_sat", s);
    frc::SmartDashboard::PutNumber("colour_val", v);
}

std::string Camera::GetColour() const
{
    std::lock_guard<std::mutex> lk(mutex_);
    return colour_;
}
