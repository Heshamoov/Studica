#include "Settings.h"
#include "Constants.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

namespace {

enum class Kind { Dbl, Int, Bool };

struct Item
{
    const char *key, *group, *label, *unit, *help;
    Kind    kind;
    double *d;  int *i;  bool *b;
    double  def, lo, hi;
    bool    live;
    double  startValue;   // value when the program started (for "restart pending")
};

std::vector<Item> items;
const char *FILE_NAME = "settings.cfg";

double Get(const Item &it)
{
    switch (it.kind) {
        case Kind::Dbl:  return *it.d;
        case Kind::Int:  return static_cast<double>(*it.i);
        default:         return *it.b ? 1.0 : 0.0;
    }
}

void Put(Item &it, double v)
{
    switch (it.kind) {
        case Kind::Dbl:  *it.d = v; break;
        case Kind::Int:  *it.i = static_cast<int>(v + (v < 0 ? -0.5 : 0.5)); break;
        default:         *it.b = v > 0.5; break;
    }
}

void D(const char *k, const char *g, const char *label, const char *unit,
       const char *help, double &ref, double lo, double hi, bool live = true)
{ items.push_back({k, g, label, unit, help, Kind::Dbl, &ref, nullptr, nullptr, ref, lo, hi, live, ref}); }

void I(const char *k, const char *g, const char *label, const char *unit,
       const char *help, int &ref, double lo, double hi, bool live = false)
{ items.push_back({k, g, label, unit, help, Kind::Int, nullptr, &ref, nullptr, ref, lo, hi, live, static_cast<double>(ref)}); }

void B(const char *k, const char *g, const char *label, const char *help, bool &ref)
{ items.push_back({k, g, label, "", help, Kind::Bool, nullptr, nullptr, &ref, ref ? 1.0 : 0.0, 0, 1, true, ref ? 1.0 : 0.0}); }

// Values derived from others. Recomputed after every change.
void Recompute()
{
    using namespace constant;
    auto pinToPwm = [](int p) { return p < 12 ? p + 10 : p - 12; };
    SERVO_CHANNEL         = pinToPwm(SERVO_PIN);
    LIFT_SERVO_CHANNEL    = pinToPwm(LIFT_SERVO_PIN);
    K_LEVER               = HALF_LENGTH + HALF_WIDTH;
    DIST_PER_TICK         = (2.0 * M_PI * WHEEL_RADIUS) / (PULSE_PER_REV * GEAR_RATIO);
    SLOT_TOP              = SLOT_BOTTOM + SLOT_H;
    LIFT_UNDER            = SLOT_BOTTOM + 2.0;
    LIFT_LOADED           = LIFT_UNDER + LIFT_LOAD_RISE;
    LIFT_CLEAR_SLOT       = SLOT_TOP + 5.0;
    DRIVE_PICKPLACE_SPEED = SPEED_CREEP;
}

void Register()
{
    using namespace constant;
    const char *W = "Wiring (needs restart)";
    I("TITAN_ID",    W, "Drive Titan CAN ID",  "", "CAN ID of the Titan that runs the 4 wheels.", TITAN_ID, 0, 63);
    I("FRONT_LEFT",  W, "Front-left motor port",  "", "Which Titan port (0-3) the front-left wheel is plugged into.", FRONT_LEFT, 0, 3);
    I("FRONT_RIGHT", W, "Front-right motor port", "", "Port (0-3) of the front-right wheel.", FRONT_RIGHT, 0, 3);
    I("REAR_LEFT",   W, "Rear-left motor port",   "", "Port (0-3) of the rear-left wheel.", REAR_LEFT, 0, 3);
    I("REAR_RIGHT",  W, "Rear-right motor port",  "", "Port (0-3) of the rear-right wheel.", REAR_RIGHT, 0, 3);
    I("SERVO_PIN", W, "Pusher servo pin", "", "The pin number PRINTED on the VMX next to the servo's signal wire (0-21). FlexDIO 0-11 always work; 12-21 only with the HiCurrDIO jumper fitted.", SERVO_PIN, 0, 21);
    I("LIFT_SERVO_PIN", W, "Lift servo pin", "", "The pin number PRINTED on the VMX next to the servo's signal wire (0-21). FlexDIO 0-11 always work; 12-21 only with the HiCurrDIO jumper fitted.", LIFT_SERVO_PIN, 0, 21);
    I("START_BUTTON", W, "Start button pin", "", "Digital input for the start button. VMX: DIO 0-11 = FlexDIO pins (in or out); 12-21 = HiCurrDIO (outputs only if its jumper is set to OUTPUT).", START_BUTTON, 0, 25);
    I("STOP_BUTTON",  W, "Stop button pin",  "", "Digital input for the stop button. VMX: DIO 0-11 = FlexDIO pins (in or out); 12-21 = HiCurrDIO (outputs only if its jumper is set to OUTPUT).", STOP_BUTTON, 0, 25);
    I("RESET_BUTTON", W, "Reset button pin", "", "Digital input for the reset button. -1 = no reset button. Pressing it stops everything, clears the e-stop and zeroes the position, ready for START. Use a FlexDIO pin 0-11 or a HiCurrDIO pin 12-21 (input with the jumper removed).", RESET_BUTTON, -1, 25);
    I("RUNNING_LED",  W, "Running LED pin",  "", "Digital output for the running LED. VMX: DIO 0-11 = FlexDIO pins (in or out); 12-21 = HiCurrDIO (outputs only if its jumper is set to OUTPUT).", RUNNING_LED, 0, 25);
    I("STOPPED_LED",  W, "Stopped LED pin",  "", "Digital output for the stopped LED. VMX: DIO 0-11 = FlexDIO pins (in or out); 12-21 = HiCurrDIO (outputs only if its jumper is set to OUTPUT).", STOPPED_LED, 0, 25);
    I("LIFT_TOP_SWITCH", W, "Lift TOP limit switch pin", "", "Digital input of the yellow switch at the top of the elevator. Use a FlexDIO pin (0-11).", LIFT_TOP_SWITCH, 0, 25);
    I("LIFT_BOTTOM_SWITCH", W, "Lift BOTTOM limit switch pin", "", "Digital input of the yellow switch at the bottom of the elevator. Use a FlexDIO pin (0-11).", LIFT_BOTTOM_SWITCH, 0, 25);
    I("COMMAND_PORT", W, "Panel port", "", "Web port. Leave at 5800 unless you know why.", COMMAND_PORT, 1024, 65535);

    const char *M = "Motor directions";
    B("DRIVE_TANK", M, "Wheels point straight (no sideways)", "Tick if the wheels point straight forward like a car. Leave unticked for mecanum, or omni wheels at 45 degrees in the corners (X-drive) - those can drive sideways.", DRIVE_TANK);
    B("INVERT_FL", M, "Front-left reversed",  "Tick if the front-left wheel spins the wrong way.", INVERT_FL);
    B("INVERT_FR", M, "Front-right reversed", "Tick if the front-right wheel spins the wrong way.", INVERT_FR);
    B("INVERT_RL", M, "Rear-left reversed",   "Tick if the rear-left wheel spins the wrong way.", INVERT_RL);
    B("INVERT_RR", M, "Rear-right reversed",  "Tick if the rear-right wheel spins the wrong way.", INVERT_RR);
    B("STOP_ACTIVE_LOW", M, "Stop button reads 0 when pressed", "Leave ticked for Studica buttons. If the stop button shows PRESSED while you are not pressing it (and it is on the right pin), untick this.", STOP_ACTIVE_LOW);
    B("STOP_ENABLED", M, "Use the stop button", "Untick ONLY for bench testing without a stop button wired. The page STOP and Esc still work.", STOP_ENABLED);
    B("START_ENABLED", M, "Use the start button", "Untick ONLY for bench testing without a start button wired - an unconnected pin picks up noise and keeps starting the routine. The page Run button still works.", START_ENABLED);
    B("RESET_ACTIVE_LOW", M, "Reset button reads 0 when pressed", "Leave ticked for Studica buttons. Check with the live readout.", RESET_ACTIVE_LOW);
    B("START_ACTIVE_LOW", M, "Start button reads 0 when pressed", "Leave ticked for Studica buttons. Check with the live readout.", START_ACTIVE_LOW);
    B("LED_ACTIVE_LOW", M, "LEDs wired active-low", "Tick if an LED lights when its pin is LOW (wired from +5V to the pin).", LED_ACTIVE_LOW);
    B("LIFT_SWITCH_ACTIVE_LOW", M, "Limit switches read 0 when pressed", "Leave ticked if each switch connects the pin to ground. Check with the live readout on the Settings page.", LIFT_SWITCH_ACTIVE_LOW);
    B("ENC_INVERT_FL", M, "Front-left encoder reversed", "Tick if spinning the front-left wheel FORWARD makes its encoder reading go down.", ENC_INVERT_FL);
    B("ENC_INVERT_FR", M, "Front-right encoder reversed", "Tick if spinning the front-right wheel FORWARD makes its encoder reading go down.", ENC_INVERT_FR);
    B("ENC_INVERT_RL", M, "Rear-left encoder reversed", "Tick if spinning the rear-left wheel FORWARD makes its encoder reading go down.", ENC_INVERT_RL);
    B("ENC_INVERT_RR", M, "Rear-right encoder reversed", "Tick if spinning the rear-right wheel FORWARD makes its encoder reading go down.", ENC_INVERT_RR);
    B("INVERT_LIFT", M, "Lift reversed",      "Tick if the lift goes down when it should go up.", INVERT_LIFT);

    const char *Dt = "Drivetrain";
    D("WHEEL_RADIUS", Dt, "Wheel radius", "mm", "Measure your wheel. If 1000 mm drives 900, this is too big.", WHEEL_RADIUS, 20, 150);
    D("HALF_LENGTH", Dt, "Half chassis length", "mm", "Centre of robot to the wheel axle, front-back.", HALF_LENGTH, 50, 400);
    D("HALF_WIDTH",  Dt, "Half chassis width",  "mm", "Centre of robot to the wheel, left-right.", HALF_WIDTH, 50, 400);
    D("MAX_SPEED", Dt, "Top wheel speed", "mm/s", "Speed at full power. Scales every move.", MAX_SPEED, 100, 2000);
    D("SPEED_CREEP",  Dt, "Creep speed",  "mm/s", "Final approach.", SPEED_CREEP, 20, 500);
    D("SPEED_SLOW",   Dt, "Slow speed",   "mm/s", "Near obstacles.", SPEED_SLOW, 50, 800);
    D("SPEED_CRUISE", Dt, "Cruise speed", "mm/s", "Open floor.", SPEED_CRUISE, 100, 1500);
    D("TURN_SLOW", Dt, "Slow turn rate", "deg/s", "", TURN_SLOW, 10, 300);
    D("TURN_FAST", Dt, "Fast turn rate", "deg/s", "", TURN_FAST, 20, 500);
    D("POS_TOLERANCE", Dt, "Position tolerance", "mm", "How close counts as arrived.", POS_TOLERANCE, 1, 100);
    D("ANG_TOLERANCE", Dt, "Angle tolerance", "deg", "How close counts as turned.", ANG_TOLERANCE, 0.5, 20);
    D("KP_LINEAR",  Dt, "Drive gain", "", "Higher = snappier, too high = overshoots.", KP_LINEAR, 0.1, 10);
    D("KP_ANGULAR", Dt, "Turn gain",  "", "Higher = snappier turns.", KP_ANGULAR, 0.1, 10);

    const char *L = "Lift (servo)";
    B("LIFT_CONTINUOUS", L, "Lift servo is in CONTINUOUS mode", "Tick only if the lift servo spins all the way round. Then STOP sets it to 90 (stop). Height control needs ANGLE mode.", LIFT_CONTINUOUS);
    B("LIFT_HAS_BOTTOM_SWITCH", L, "Lift has a BOTTOM switch", "Untick if your lift has no bottom limit switch. Then 'Find bottom' runs down for the time below and assumes it reached the bottom.", LIFT_HAS_BOTTOM_SWITCH);
    B("LIFT_HAS_TOP_SWITCH", L, "Lift has a TOP switch", "Untick if your lift has no top limit switch.", LIFT_HAS_TOP_SWITCH);
    D("LIFT_DOWN_FULL_S", L, "Time to reach the bottom (no switch)", "s", "Only without a bottom switch: how long to run down so it surely reaches the bottom from the top.", LIFT_DOWN_FULL_S, 0.5, 30);
    D("LIFT_RAIL_MM", L, "Rail travel between the switches", "mm", "Measure: fork height at the TOP switch minus height at the BOTTOM switch. Continuous lift only.", LIFT_RAIL_MM, 10, 600);
    D("LIFT_UP_CMD", L, "Spin-up command", "", "Continuous lift: servo value that drives it UP (90 = stop). If it goes down instead, swap with the down command.", LIFT_UP_CMD, 0, 180);
    D("LIFT_DOWN_CMD", L, "Spin-down command", "", "Continuous lift: servo value that drives it DOWN.", LIFT_DOWN_CMD, 0, 180);
    D("LIFT_STOP_CMD", L, "Stop command", "", "Continuous lift: 90 normally. Adjust a little if it creeps when stopped.", LIFT_STOP_CMD, 60, 120);
    D("LIFT_UP_MM_S", L, "Up speed (measured)", "mm/s", "Set automatically by Calibrate lift.", LIFT_UP_MM_S, 0.5, 500);
    D("LIFT_DOWN_MM_S", L, "Down speed (measured)", "mm/s", "Set automatically by Calibrate lift.", LIFT_DOWN_MM_S, 0.5, 500);
    D("LIFT_ANGLE_AT_MIN", L, "Servo angle at lowest", "deg", "Servo angle when the fork is at the lowest height.", LIFT_ANGLE_AT_MIN, 0, 180);
    D("LIFT_ANGLE_AT_MAX", L, "Servo angle at highest", "deg", "Servo angle when the fork is at the highest height.", LIFT_ANGLE_AT_MAX, 0, 180);
    D("LIFT_MIN", L, "Lowest height", "mm", "Fork height at the lowest angle.", LIFT_MIN, 0, 100);
    D("LIFT_MAX", L, "Highest height", "mm", "Fork height at the highest angle.", LIFT_MAX, 60, 400);
    D("LIFT_SERVO_SPEED", L, "Lift speed", "mm/s", "How fast the lift servo moves; the robot waits distance / speed.", LIFT_SERVO_SPEED, 5, 500);
    D("LIFT_HOME",  L, "Home height",  "mm", "Height at start-up and after lift home.", LIFT_HOME, 0, 400);
    D("LIFT_CARRY", L, "Carry height", "mm", "Height while driving with a piece.", LIFT_CARRY, 0, 400);

    const char *C = "Pusher servo";
    D("PARK_DEG",    C, "Park angle",       "deg", "Arm swung back and up, clear of the storage base and stands. Used while picking and driving.", PARK_DEG, 0, 180);
    D("RETRACT_DEG", C, "Push start angle", "deg", "Paddle just behind the piece's back edge, touching nothing.", RETRACT_DEG, 0, 180);
    D("PUSH_DEG",    C, "Push end angle",   "deg", "Piece fully off the fork tip.", PUSH_DEG, 0, 180);
    B("PUSHER_CONTINUOUS", C, "Pusher servo is in CONTINUOUS mode", "Tick only if the pusher servo spins all the way round (set by the Studica servo programmer). Better: program it to ANGLE mode and leave this unticked.", PUSHER_CONTINUOUS);
    D("PUSH_RUN_S", C, "Push run time (continuous only)", "s", "How long a continuous pusher spins for one push / retract.", PUSH_RUN_S, 0.1, 5);
    D("PUSH_SPEED", C, "Push speed", "deg/s", "Slow = the piece slides off and drops flat. Too fast flicks and flips it.", PUSH_SPEED, 5, 360);
    D("RETRACT_SPEED", C, "Retract speed", "deg/s", "Coming back can be fast.", RETRACT_SPEED, 5, 720);
    D("PUSH_SETTLE_S", C, "Pause after move", "s", "Short wait after the pusher arrives.", PUSH_SETTLE_S, 0, 3);

    const char *P = "Pick and place";
    D("DRIVE_INSERT_STORAGE", P, "Insert into storage", "mm", "How far the robot drives forward to put the fork into the storage slot.", DRIVE_INSERT_STORAGE, 10, 300);
    D("DRIVE_INSERT_STAND",   P, "Drive over stand",  "mm", "How far the robot drives forward so the fork tip is over the pocket.", DRIVE_INSERT_STAND, 0, 300);
    D("LIFT_LOAD_RISE", P, "Lift-off rise", "mm", "How much the lift rises to take the piece's weight.", LIFT_LOAD_RISE, 1, 40);
    D("LIFT_PLACE_GAP", P, "Height above stand wall", "mm", "How far above the stand wall the fork is when the piece is pushed off.", LIFT_PLACE_GAP, 1, 60);

    const char *S = "Sensors (needs restart)";
    B("US_ENABLED", S, "Ultrasonic fitted", "Tick if the robot has a front ultrasonic sensor. Restart after changing.", US_ENABLED);
    I("US_PING", S, "Ultrasonic ping (trig) pin", "", "Printed DIO number of the ping / trig wire.", US_PING, 0, 21);
    I("US_ECHO", S, "Ultrasonic echo pin", "", "Printed DIO number of the echo wire.", US_ECHO, 0, 21);
    B("IR_ENABLED", S, "Sharp IR sensor fitted", "Tick if the robot has a front Sharp IR range sensor.", IR_ENABLED);
    I("IR_CHANNEL", S, "IR analog channel", "", "Printed Analog number 0-3.", IR_CHANNEL, 0, 3);
    B("COBRA_ENABLED", S, "Cobra line follower fitted", "Tick if a Cobra is plugged into the I2C port.", COBRA_ENABLED);
    D("COBRA_VREF", S, "Cobra reference voltage", "V", "5 for the Studica Cobra.", COBRA_VREF, 1, 5.5, false);
    D("COBRA_LINE_V", S, "Cobra line threshold", "V", "A channel above this voltage sees the line. Watch the live readout over the line and over the floor and pick a value in between.", COBRA_LINE_V, 0.1, 5);
    D("SENSOR_SPEED", S, "Speed for sensor moves", "mm/s", "Drive speed while approaching a distance or line.", SENSOR_SPEED, 30, 400);
    D("SENSOR_TOL", S, "Distance tolerance", "mm", "How close counts as arrived for drive_to_us / drive_to_ir.", SENSOR_TOL, 2, 50);

    const char *V = "Colour detection";
    D("RED_H_LO", V, "Red hue low", "", "OpenCV hue 0-180.", RED_H_LO, 0, 180);
    D("RED_H_HI", V, "Red hue high", "", "", RED_H_HI, 0, 180);
    D("RED_H_LO2", V, "Red hue low (wrap)", "", "Red also wraps around near 180.", RED_H_LO2, 0, 180);
    D("RED_H_HI2", V, "Red hue high (wrap)", "", "", RED_H_HI2, 0, 180);
    D("RED_S_MIN", V, "Red min saturation", "", "0-255.", RED_S_MIN, 0, 255);
    D("GREEN_H_LO", V, "Green hue low", "", "", GREEN_H_LO, 0, 180);
    D("GREEN_H_HI", V, "Green hue high", "", "", GREEN_H_HI, 0, 180);
    D("GREEN_S_MIN", V, "Green min saturation", "", "", GREEN_S_MIN, 0, 255);
    D("YELLOW_H_LO", V, "Yellow hue low", "", "", YELLOW_H_LO, 0, 180);
    D("YELLOW_H_HI", V, "Yellow hue high", "", "", YELLOW_H_HI, 0, 180);
    D("YELLOW_S_MIN", V, "Yellow min saturation", "", "", YELLOW_S_MIN, 0, 255);
    D("BLUE_H_LO", V, "Blue hue low", "", "", BLUE_H_LO, 0, 180);
    D("BLUE_H_HI", V, "Blue hue high", "", "", BLUE_H_HI, 0, 180);
    D("BLUE_S_MIN", V, "Blue min saturation", "", "", BLUE_S_MIN, 0, 255);
    D("SAT_MIN_COLOUR", V, "Colourless below", "", "Below this saturation the piece counts as white, grey or black.", SAT_MIN_COLOUR, 0, 255);
}

Item *Find(const std::string &key)
{
    for (auto &it : items) if (key == it.key) return &it;
    return nullptr;
}

std::string Num(double v)
{
    std::ostringstream o;
    o << std::setprecision(10) << v;
    return o.str();
}

std::string Esc(const char *s)
{
    std::string out;
    for (const char *p = s; *p; ++p) {
        if (*p == '"' || *p == '\\') out += '\\';
        out += *p;
    }
    return out;
}

// Write only values that differ from the default, so the file stays short and
// a later change to a built-in default still reaches teams that never
// touched that setting.
void Save()
{
    std::string tmp = std::string(FILE_NAME) + ".tmp";
    {
        std::ofstream f(tmp);
        f << "# Robot settings. Written by the web panel; safe to edit by hand.\n";
        for (auto &it : items)
            if (Get(it) != it.def) f << it.key << "=" << Num(Get(it)) << "\n";
    }
    std::rename(tmp.c_str(), FILE_NAME);
}

void Load()
{
    std::ifstream f(FILE_NAME);
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        Item *it = Find(line.substr(0, eq));
        if (!it) continue;                       // unknown key: ignore, don't crash
        std::string v = line.substr(eq + 1);
        char *end = nullptr;
        double x = std::strtod(v.c_str(), &end);
        if (end == v.c_str() || x < it->lo || x > it->hi) continue;
        Put(*it, x);
    }
    for (auto &it : items) it.startValue = Get(it);
}

} // namespace

namespace settings
{

void Init()
{
    if (!items.empty()) return;
    items.reserve(96);
    Register();
    Load();
    Recompute();
    std::printf("settings: %zu values, file %s\n", items.size(), FILE_NAME);
    std::fflush(stdout);
}

bool RestartPending()
{
    for (auto &it : items)
        if (!it.live && Get(it) != it.startValue) return true;
    return false;
}

std::string Json()
{
    std::ostringstream o;
    o << "{\"restart_pending\":" << (RestartPending() ? "true" : "false") << ",\"settings\":[";
    bool first = true;
    for (auto &it : items) {
        if (!first) o << ",";
        first = false;
        o << "{\"key\":\"" << it.key << "\",\"group\":\"" << Esc(it.group)
          << "\",\"label\":\"" << Esc(it.label) << "\",\"unit\":\"" << Esc(it.unit)
          << "\",\"help\":\"" << Esc(it.help) << "\",\"type\":\""
          << (it.kind == Kind::Bool ? "bool" : it.kind == Kind::Int ? "int" : "number")
          << "\",\"value\":" << Num(Get(it)) << ",\"default\":" << Num(it.def)
          << ",\"min\":" << Num(it.lo) << ",\"max\":" << Num(it.hi)
          << ",\"live\":" << (it.live ? "true" : "false") << "}";
    }
    o << "]}";
    return o.str();
}

bool Set(const std::string &key, const std::string &value, std::string &msg)
{
    Item *it = Find(key);
    if (!it) { msg = "unknown setting: " + key; return false; }

    double x;
    if (it->kind == Kind::Bool) {
        if (value == "true" || value == "1")       x = 1.0;
        else if (value == "false" || value == "0") x = 0.0;
        else { msg = "expected true or false"; return false; }
    } else {
        char *end = nullptr;
        x = std::strtod(value.c_str(), &end);
        if (value.empty() || end == value.c_str() || *end != '\0') { msg = "not a number: " + value; return false; }
        if (x < it->lo || x > it->hi) {
            msg = std::string(it->label) + " must be between " + Num(it->lo) + " and " + Num(it->hi);
            return false;
        }
    }
    Put(*it, x);
    Recompute();
    Save();
    msg = it->live ? "saved and applied" : "saved - restart the robot program to apply";
    return true;
}

bool Reset(const std::string &key, std::string &msg)
{
    if (key == "all") {
        for (auto &it : items) Put(it, it.def);
    } else {
        Item *it = Find(key);
        if (!it) { msg = "unknown setting: " + key; return false; }
        Put(*it, it->def);
    }
    Recompute();
    Save();
    msg = "reset to default";
    return true;
}

} // namespace settings
