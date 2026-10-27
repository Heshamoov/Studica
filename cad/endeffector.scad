/*******************************************************************************
 * SC2026 Skill 23 — End Effector  (updated: combined fork + elevator view)
 *
 * PART SELECTION
 *   "fork_combined"  — one-piece U-fork: both tines + crossbar, prints flat
 *                      NO supports needed.  Replaces the two separate fork_tine prints.
 *   "clamp_arm"      — print 1×, mounts on servo horn
 *   "servo_bracket"  — print 1×, mounts servo to the elevator carriage
 *   "elevator"       — preview of the full mechanism on a simplified carriage
 *   "assembly"       — close-up preview of the end-effector parts together
 *
 * PRINTING
 *   fork_combined   PETG, 4 walls, 40% gyroid infill, 0.2 mm layer
 *                   Lay it FLAT (Z = 5 mm, open side down).  No supports.
 *   clamp_arm       PETG, 4 walls, 30% infill, 0.2 mm layer
 *   servo_bracket   PETG, 4 walls, 40% infill — needs supports under servo pocket
 *
 * STUDICA HOLE GRID: 8 mm pitch, 3.5 mm holes (3.6 mm clearance used here).
 ******************************************************************************/

PART = "elevator";   // <-- change this

// ── global parameters (all mm) ───────────────────────────────────────────────
STUD_PITCH = 8.0;
STUD_HOLE  = 3.6;

// Flat piece
PIECE_W = 75.0;
PIECE_T = 10.0;

// Storage base opening (tines enter through here)
SLOT_W = 50.0;
SLOT_H = 30.0;

// Fork
TINE_W     = 18.0;
TINE_T     =  5.0;   // tine thickness = crossbar thickness (whole fork prints at 5 mm)
TINE_L     = 120.0;
TINE_TIP   = 20.0;
TINE_GAP   = 10.0;
MOUNT_W    = 2 * TINE_W + TINE_GAP;   // 46 mm  (fits inside 50 mm slot)
CROSS_D    = 30.0;   // crossbar depth (how far it overlaps onto the extension arm)

// Studica 76148 Servo Horn 25T (from the vendor STEP file)
HORN_DISC_D = 29.0;   // disc Ø
HORN_BOSS_D = 10.0;   // centre boss Ø

// Clamp arm
ARM_W      = 15.0;
ARM_T      =  5.0;
ARM_L      = 68.0;
ARM_HUB_OD = 14.0 + 6;   // bolt-circle dia + 6 mm wall
ARM_HUB_T  =  5.0;
PAD_W      = 30.0;
PAD_T      =  3.0;

// Servo bracket
// SB_W must clear the servo body's own length (SRV_H, defined below) plus
// both side walls, or the body pokes out the open side — 38 gives SRV_H(28)
// + 2×SB_T(4) + a little margin.
SB_W       = 38.0;
SB_H       = 38.0;
SB_T       =  4.0;
SB_POCKET_W = 23.5;
SB_POCKET_D = 13.0;
SB_FLANGE_T =  2.5;
SB_MOUNT_H  = 48.0;

// Generic servo + horn — VISUALISATION ONLY, not printed. Sized to a
// common micro servo (MG90S-class). Verify against your actual servo and
// correct SB_POCKET_W/D and ARM_HUB_OD if it differs before printing.
SRV_W        = 23.0;   // body width  (X)
SRV_D        = 12.5;   // body depth  (Y)
SRV_H        = 28.0;   // body height, sitting in the pocket floor
SRV_EAR_T    =  2.0;   // mounting ear thickness
SRV_EAR_W    =  4.5;   // ear width sticking out past the body each side
SRV_EAR_Z    = 12.0;   // position of the ears along the body's length
SRV_SHAFT_D  =  5.0;
SRV_SHAFT_H  =  8.0;   // pokes up through the bracket's top-cap slot
SRV_SCREW_D  =  2.5;   // mounting screw clearance (M2/M2.5 typical)

HORN_ARM_L   = 12.0;
HORN_ARM_W   =  4.0;
HORN_T       =  2.0;
HORN_HUB_D   =  7.0;

// Elevator (for visualisation only — not printed)
CAR_W  = 80;    // carriage width
CAR_D  = 45;    // carriage depth (front to back)
CAR_H  = 90;    // visible carriage height
EXT_W  = 36;    // extension arm width  (must be ≤ MOUNT_W = 46)
EXT_H  = 20;    // extension arm height
EXT_L  = 110;   // extension arm length in view

$fn = 32;

// ── helpers ──────────────────────────────────────────────────────────────────
module stud_hole(depth = 20) {
    cylinder(d = STUD_HOLE, h = depth, center = true);
}

// Tapered fork tip (reused for left and right tine in fork_combined)
// Placed with back face at Y=0, tip pointing in –Y direction.
module tine_tip() {
    polyhedron(
        points = [
            [0,         0,       0],
            [TINE_W,    0,       0],
            [TINE_W,  -TINE_TIP, 0],
            [0,       -TINE_TIP, 0],
            [0,         0,       TINE_T],
            [TINE_W,    0,       TINE_T],
            [TINE_W,  -TINE_TIP, TINE_T/2],
            [0,       -TINE_TIP, TINE_T/2],
        ],
        faces = [
            [0,1,2,3],[4,7,6,5],
            [0,4,5,1],[1,5,6,2],[2,6,7,3],[3,7,4,0]
        ]
    );
}

// ═══════════════════════════════════════════════════════════════════════════
// PART 1 — COMBINED FORK  (one piece, print flat, no supports)
// ═══════════════════════════════════════════════════════════════════════════
// Coordinate origin: front face of the crossbar (where crossbar meets tines).
//   Tines extend in –Y (into the storage base).
//   Crossbar extends in +Y (overlaps the extension arm for bolting).
//   Z = up from printer bed surface.
//
// Cross-section is 5 mm throughout — the whole fork prints lying flat
// on a 46 × 120 mm footprint, 5 mm off the bed.
//
module fork_combined() {
    tine_body_len = TINE_L - CROSS_D - TINE_TIP;   // straight section

    difference() {
        union() {
            // ── crossbar (bolts to extension arm) ──────────────────────────
            // MOUNT_W (46) already equals the tines' own combined span
            // (left tine at X 0-18, gap, right tine at X 28-46) — the
            // crossbar sits flush at X 0-46, no offset. (An earlier version
            // centred it as if MOUNT_W were wider than a single tine, which
            // shifted it 14 mm left and left the right tine unsupported.)
            cube([MOUNT_W, CROSS_D, TINE_T]);

            // ── left tine body ─────────────────────────────────────────────
            translate([0, -tine_body_len, 0])
                cube([TINE_W, tine_body_len, TINE_T]);

            // ── right tine body ────────────────────────────────────────────
            translate([TINE_W + TINE_GAP, -tine_body_len, 0])
                cube([TINE_W, tine_body_len, TINE_T]);

            // ── left tine tip (tapered) ────────────────────────────────────
            // tine_tip()'s back face sits at its local Y=0, so it must be
            // placed at the tine body's front edge (Y = -tine_body_len),
            // NOT tine_body_len+TINE_TIP further out — that gap left the
            // tapered tip floating disconnected from the tine body.
            translate([0, -tine_body_len, 0])
                tine_tip();

            // ── right tine tip (tapered) ───────────────────────────────────
            translate([TINE_W + TINE_GAP, -tine_body_len, 0])
                tine_tip();
        }

        // Studica U-channel pattern (76014 STEP): Ø3.5 holes at +-8 mm
        // around each 48 mm-pitch centre -> a 16 x 16 mm square, centred
        // on the crossbar so it lines up with one channel cell.
        for (hx = [MOUNT_W/2 - 8, MOUNT_W/2 + 8]) {
            for (hy = [CROSS_D/2 - 8, CROSS_D/2 + 8]) {
                translate([hx, hy, -1])
                    cylinder(d = STUD_HOLE, h = TINE_T + 2);
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// PART 2 — CLAMP ARM  (1×, mounts on servo horn)
// ═══════════════════════════════════════════════════════════════════════════
// Origin = servo output shaft axis, bottom of hub.
// Arm extends in +Y.
//
module clamp_arm() {
    difference() {
        union() {
            cylinder(d = HORN_DISC_D, h = ARM_HUB_T);       // hub, same Ø as the horn
            translate([-ARM_W/2, 0, 0]) cube([ARM_W, ARM_L, ARM_T]);
            translate([-PAD_W/2, ARM_L - PAD_W/2, 0])
                cube([PAD_W, PAD_W/2 + 2, ARM_T + PAD_T]);
        }
        // Studica 76148 Servo Horn 25T: Ø3.5 holes on a 16 x 16 square
        // (+-8, +-8), centre boss Ø10 — measured from the vendor STEP file
        cylinder(d = HORN_BOSS_D + 0.6, h = ARM_HUB_T + 2, center = true);
        for (hx = [-8, 8], hy = [-8, 8])
            translate([hx, hy, 0])
                cylinder(d = STUD_HOLE, h = ARM_HUB_T + 2, center = true);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// PART 3 — SERVO BRACKET  (1×, mounts to elevator carriage)
// ═══════════════════════════════════════════════════════════════════════════
// SHAFT AXIS IS HORIZONTAL (through the right wall, pointing +X), not
// vertical. A vertical shaft (through the top cap) can only swivel the arm
// sideways in a flat plane — it can never swing down onto a piece. A
// horizontal shaft lets the arm swing through a vertical arc, like a gate,
// from "open/up" to "pressing down" — see elevator()/assembly() below.
module servo_bracket() {
    // X position of the servo's mounting ears, matching servo_generic()'s
    // own ear_x so the screw holes actually land on the ear tabs.
    srv_bx = SB_W - SB_T - SRV_H;
    ear_x  = srv_bx + SRV_EAR_Z;

    // The base flange and the bottom shelf sit at the same Z (0..SB_T) and
    // would overlap if the flange were short — a flange bolt hole in that
    // shared zone punches straight through the pocket floor under the
    // servo. FLANGE_D extends the flange past the shelf's front edge
    // (SB_T + SB_POCKET_D) so its own hole lands in wholly separate
    // material, with no shared footprint.
    FLANGE_D = SB_T + SB_POCKET_D + 8;

    difference() {
        union() {
            cube([SB_W, SB_T, SB_MOUNT_H]);                         // back wall
            cube([SB_W, FLANGE_D, SB_T]);                           // base flange
            translate([0, SB_T, 0])
                cube([SB_T, SB_POCKET_D, SB_H]);                    // left wall
            translate([SB_W - SB_T, SB_T, 0])
                cube([SB_T, SB_POCKET_D, SB_H]);                    // right wall (shaft exits here)
            translate([0, SB_T, SB_H - SB_T])
                cube([SB_W, SB_POCKET_D, SB_T]);                    // top cap
            translate([0, SB_T, 0])
                cube([SB_W, SB_POCKET_D, SB_T]);                    // bottom shelf — retains the servo's ears
        }
        // back-wall Studica holes (in Y direction)
        for (hx = [STUD_PITCH/2, SB_W - STUD_PITCH/2],
             hz = [STUD_PITCH/2, SB_MOUNT_H - STUD_PITCH/2 - SB_H/3])
            translate([hx, SB_T/2, hz])
                rotate([90, 0, 0]) stud_hole(SB_T + 2);
        // flange holes (in Z direction) — past the shelf's front edge,
        // clear of the pocket floor entirely
        for (hx = [STUD_PITCH/2, SB_W - STUD_PITCH/2])
            translate([hx, SB_T + SB_POCKET_D + 4, SB_T/2])
                stud_hole(SB_T + 2);
        // servo shaft slot through the RIGHT wall — horizontal axis
        translate([SB_W - SB_T - 1, SB_T + SB_POCKET_D/2, SB_H/2])
            rotate([0, 90, 0])
                cylinder(d = 7, h = SB_T + 3);
        // servo mounting screws — straight down through the top cap and
        // straight up through the bottom shelf, landing on the ear tabs
        translate([ear_x, SB_T + SB_POCKET_D/2, SB_H - SB_T - 1])
            cylinder(d = SRV_SCREW_D, h = SB_T + 2);
        translate([ear_x, SB_T + SB_POCKET_D/2, -1])
            cylinder(d = SRV_SCREW_D, h = SB_T + 2);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// GENERIC SERVO + HORN — VISUALISATION ONLY, not printed
// ═══════════════════════════════════════════════════════════════════════════
// Same local frame as servo_bracket(). Shaft axis is X (matches the
// bracket's right-wall exit, not a top exit) — mounted "on its side" so the
// arm it drives can sweep through a vertical arc. Bought parts, not
// designed here — swap the dimensions above for your actual servo/horn
// before trusting fit.
module servo_generic() {
    // body: SRV_H (was the shaft-axis / body-height dimension) now runs
    // along X, flush against the inside of the right wall; SRV_W (was
    // body width) now runs vertically in Z.
    bx = SB_W - SB_T - SRV_H;
    by = SB_T + SB_POCKET_D/2 - SRV_D/2;
    bz = SB_H/2 - SRV_W/2;
    ear_x = bx + SRV_EAR_Z;   // ear position along the body's length

    color("DimGray") {
        translate([bx, by, bz])
            cube([SRV_H, SRV_D, SRV_W]);
        // mounting ears, top and bottom of the body (were left/right
        // before the body was rotated onto its side)
        translate([ear_x, by, bz - SRV_EAR_W])
            cube([SRV_EAR_T, SRV_D, SRV_EAR_W]);
        translate([ear_x, by, bz + SRV_W])
            cube([SRV_EAR_T, SRV_D, SRV_EAR_W]);
    }
    // output shaft, pokes through the bracket's right-wall slot (+X)
    color("Black")
        translate([SB_W - SB_T, SB_T + SB_POCKET_D/2, SB_H/2])
            rotate([0, 90, 0])
                cylinder(d = SRV_SHAFT_D, h = SRV_SHAFT_H);
}

// Horn: bolts to the servo shaft, clamp_arm's hub then bolts to THIS.
// Hub disc faces +X (shaft direction); arms extend in ±Y so they don't
// collide with the bracket's top cap or base flange.
module horn_generic() {
    color("Black") {
        rotate([0, 90, 0])
            cylinder(d = HORN_HUB_D, h = HORN_T);
        translate([0, 0, -HORN_ARM_W/2])
            cube([HORN_T, HORN_ARM_L, HORN_ARM_W]);
        translate([0, -HORN_ARM_L, -HORN_ARM_W/2])
            cube([HORN_T, HORN_ARM_L, HORN_ARM_W]);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// ELEVATOR PREVIEW
// ═══════════════════════════════════════════════════════════════════════════
// Shows the end effector mounted on a simplified carriage + extension arm.
// The carriage origin is at the bottom-front-centre of the carriage block.
// Extension arm extends in –Y (same direction as fork tines).
//
module elevator() {
    // ── carriage block ──────────────────────────────────────────────────────
    color("DimGray", 0.75)
        translate([-CAR_W/2, 0, 0])
            cube([CAR_W, CAR_D, CAR_H]);

    // ── mast channel (partial, behind carriage) ─────────────────────────────
    color("Gray", 0.5) {
        translate([-24, CAR_D, -60]) cube([4,  4, CAR_H + 120]);   // left rail
        translate([ 20, CAR_D, -60]) cube([4,  4, CAR_H + 120]);   // right rail
        translate([-24, CAR_D, -60]) cube([48, 4, 4]);              // bottom tie
    }

    // ── extension arm (the horizontal slide that carries the fork) ───────────
    // Retracted: arm end at Y = 0 (flush with carriage front)
    // Extended:  arm end would be at Y = –EXT_L  (into the storage base)
    // Showing a partially-extended position for clarity.
    EXT_OUT = 80;   // how far extended in this preview
    arm_z = 18;     // height of arm centre from carriage bottom

    color("Silver", 0.85)
        translate([-EXT_W/2, -EXT_OUT, arm_z - EXT_H/2])
            cube([EXT_W, EXT_OUT + CAR_D, EXT_H]);

    // ── fork (combined, one piece) mounted at end of extension arm ────────────
    // Fork crossbar bolts on top of the extension arm.
    // Crossbar front face aligns with the arm tip.
    fork_ox = -TINE_W - TINE_GAP/2;    // centre fork over arm (MOUNT_W/2 = 23 → –TINE_W–5)
    fork_z  = arm_z + EXT_H/2;         // sits on top of arm

    color("SteelBlue")
        translate([fork_ox, -EXT_OUT, fork_z])
            fork_combined();

    // ── servo bracket, bolted to carriage front face ─────────────────────────
    // Bracket back-wall flush with carriage front, centred in X over fork.
    brkt_x = -SB_W/2;
    brkt_y = 0;          // back-wall at Y=0 (carriage front face)
    brkt_z = arm_z + EXT_H/2 + 120;    // 120 mm above the fork base

    color("Gray", 0.7)
        translate([brkt_x, brkt_y, brkt_z])
            servo_bracket();

    // ── the physical servo + horn (bought parts, shown for context) ──────────
    translate([brkt_x, brkt_y, brkt_z])
        servo_generic();
    translate([brkt_x + SB_W - SB_T, brkt_y + SB_T + SB_POCKET_D/2, brkt_z + SB_H/2])
        horn_generic();

    // ── clamp arm in OPEN position ────────────────────────────────────────
    // Shaft is horizontal (X axis), so the arm sweeps through a vertical
    // arc: OPEN = tipped back/up clear of the piece, HOLD = swung down
    // flat onto it. Not a sideways swivel — that would never reach the piece.
    arm_cx = brkt_x + SB_W - SB_T + HORN_T;
    arm_cy = brkt_y + SB_T + SB_POCKET_D/2;
    arm_cz = brkt_z + SB_H/2;

    color("Orange", 0.9)
        translate([arm_cx, arm_cy, arm_cz])
            rotate([0, -90, 0])
                clamp_arm();

    // ── ghost flat piece resting on the fork ─────────────────────────────────
    piece_oy = -EXT_OUT + CROSS_D + (TINE_L - CROSS_D - PIECE_W) / 2;
    color("Red", 0.25)
        translate([fork_ox + (MOUNT_W - PIECE_W)/2, piece_oy, fork_z + TINE_T])
            cube([PIECE_W, PIECE_W, PIECE_T]);

    // ── storage base ghost (transparent grey) ─────────────────────────────────
    // Shows where the fork enters during a pick.
    BASE_OD = 100;
    BASE_ID =  80;
    BASE_H  = 100;
    BASE_SLOT_W = 50;
    BASE_SLOT_H = 30;
    LEDGE_H = 87.5;

    color("LightGray", 0.15)
        translate([-BASE_OD/2, -EXT_OUT - BASE_OD - 10, 0]) {
            difference() {
                cube([BASE_OD, BASE_OD, BASE_H]);
                // hollow interior
                translate([(BASE_OD - BASE_ID)/2, (BASE_OD - BASE_ID)/2, 2])
                    cube([BASE_ID, BASE_ID, BASE_H]);
                // front slot (tines enter here)
                translate([(BASE_OD - BASE_SLOT_W)/2, -1, LEDGE_H - BASE_SLOT_H])
                    cube([BASE_SLOT_W, BASE_OD/2 + 2, BASE_SLOT_H + 2]);
            }
            // ledges the piece rests on
            color("LightGray", 0.25) {
                translate([(BASE_OD - BASE_ID)/2, (BASE_OD - BASE_ID)/2, LEDGE_H])
                    cube([5, BASE_ID, 2]);   // back ledge
                translate([BASE_OD - (BASE_OD - BASE_ID)/2 - 5, (BASE_OD - BASE_ID)/2, LEDGE_H])
                    cube([5, BASE_ID, 2]);   // side ledge
            }
        }
}

// ═══════════════════════════════════════════════════════════════════════════
// CLOSE-UP ASSEMBLY PREVIEW
// ═══════════════════════════════════════════════════════════════════════════
module assembly() {
    // combined fork at origin — crossbar spans X 0..MOUNT_W (46), matching
    // the tines' own combined span, so everything below centres on that
    // same 0..46 range with no extra offset.
    color("SteelBlue")
        fork_combined();

    // bracket behind the fork, 120 mm up
    brkt_x = (MOUNT_W - SB_W) / 2;
    brkt_y = CROSS_D + 2;
    brkt_z = 120;
    color("Gray", 0.7)
        translate([brkt_x, brkt_y, brkt_z])
            servo_bracket();

    // the physical servo + horn (bought parts, shown for context)
    translate([brkt_x, brkt_y, brkt_z])
        servo_generic();
    translate([brkt_x + SB_W - SB_T, brkt_y + SB_T + SB_POCKET_D/2, brkt_z + SB_H/2])
        horn_generic();

    // arm in OPEN position — shaft is horizontal (X axis), arm sweeps
    // through a vertical arc, not a sideways swivel.
    color("Orange", 0.8)
        translate([brkt_x + SB_W - SB_T + HORN_T,
                   brkt_y + SB_T + SB_POCKET_D/2,
                   brkt_z + SB_H/2])
            rotate([0, -90, 0])
                clamp_arm();

    // ghost piece
    color("Red", 0.25)
        translate([(MOUNT_W - PIECE_W)/2,
                   -(TINE_L - CROSS_D - (TINE_L - CROSS_D - PIECE_W)/2),
                   TINE_T])
            cube([PIECE_W, PIECE_W, PIECE_T]);
}

// ═══════════════════════════════════════════════════════════════════════════
// RENDER
// ═══════════════════════════════════════════════════════════════════════════
if      (PART == "fork_combined")  fork_combined();
else if (PART == "clamp_arm")      clamp_arm();
else if (PART == "servo_bracket")  servo_bracket();
else if (PART == "assembly")       assembly();
else                               elevator();
