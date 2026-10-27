// =====================================================================
// ESC2026 Skill 23 - vertical lift: belt-driven carriage on a single mast
//
// WHY SINGLE-STAGE (the reach arithmetic that decides it)
//
//   Shelf 1 sits at 150-250 mm, shelf 2 at 450-550 mm (CID G.3.3).
//   The box is 150 mm tall and stands ON the shelf, so to grip one off
//   shelf 2 the jaw centreline must reach roughly 550 + 30 = 580 mm.
//   The robot must fit a 600 mm cube -- but only in its STARTING
//   configuration (CID 3.1), so the lift may extend past 600 mm in a run.
//
//   Mast 595 mm, carriage 110 mm tall  ->  carriage travel 485 mm.
//   The gripper mounts on a shelf at the TOP of the carriage, putting the
//   jaw centre 143 mm above the carriage bottom.
//
//     highest jaw centre = 485 + 143 = 628 mm   (need 580)  OK
//     lowest  jaw centre =   0 + 143 = 143 mm   (need 180)  OK
//
//   That clears both shelves with margin, on ONE stage. A two-stage
//   cascading elevator would give more, but it roughly triples the part
//   count and adds a second rigging failure mode. Do not pay for reach you
//   do not need.
//
//   ESCAPE HATCH if your measurements come out short: hinge the mast so it
//   starts tilted back and raises after the run begins. Only the starting
//   configuration is measured, so this is legal and buys ~80 mm.
//
// DRIVE
//   Belt loop, motor + drive pulley at the bottom, idler at the top, the
//   carriage clamped to one run of the belt. A Studica gearmotor with an
//   encoder gives closed-loop position off the Titan. Geared motors are
//   effectively non-backdriveable at this load (a box + gripper is well
//   under 0.5 kg -> ~0.03 Nm at a 20T GT2 pulley), so the box will not sag
//   when you stop. Confirm that on the bench before you trust it.
//
//   openscad lift.scad          // or run ..\build.ps1
// =====================================================================

PART = "assembly";  // assembly | carriage | idler_top | motor_base | belt_clamp

$fn = 48;

/* [Mast] ---------------------------------------------------- <-- MEASURE */
// Studica U-channel. THESE ARE GUESSES. Put calipers on the real channel
// before printing - every roller position below derives from them.
mast_w   = 32.0;    // outer width, flange to flange
mast_d   = 12.0;    // front-to-back depth of the channel
mast_h   = 595.0;   // cut length - keeps the START config inside 600 mm

/* [Rollers] */
// 608 skate bearings: 8 mm bore, 22 mm OD, 7 mm wide. Ubiquitous, cheap,
// almost certainly already in your spares box.
roller_od   = 22.0;
roller_w    = 7.0;
roller_bore = 8.0;
roller_dz   = 76.0;  // vertical span between roller pairs - bigger resists
                     // the tipping moment from the cantilevered gripper
preload_ecc = 0.8;   // eccentric offset on the rear rollers, for take-up

/* [Carriage] */
carriage_h  = 110.0;
plate_t     = 7.0;
shelf_d     = 78.0;  // how far the gripper shelf sticks out from the mast
gusset_t    = 5.0;

/* [Belt] */
belt_w      = 6.0;   // GT2 6 mm
belt_pitch  = 2.0;
pulley_teeth = 20;   // pitch dia 12.73 mm -> 40 mm of lift per revolution

/* [Gripper interface] */
// Must match base_l / base_w / kit_hole_* in gripper.scad.
grip_hole_pitch = 16.0;
grip_hole_d     = 4.4;
grip_hole_cols  = 3;
grip_hole_rows  = 2;

/* [Motor] --------------------------------------------------- <-- MEASURE */
motor_boss_d    = 22.0;   // centring boss on the gearbox face
motor_shaft_d   = 6.0;
motor_pcd       = 31.0;   // mounting screw circle
motor_screws    = 2;
motor_screw_d   = 3.4;

// =====================================================================
// derived
// =====================================================================
pulley_pd   = pulley_teeth * belt_pitch / PI;   // 12.73 mm
lift_per_rev = PI * pulley_pd;                  // 40 mm
travel      = mast_h - carriage_h;              // 485 mm
jaw_above   = carriage_h + 6 + 27;              // gripper base + half jaw
plate_w     = mast_w + 2 * (roller_od + 8);
roller_x    = mast_w / 2 + roller_od / 2 - 3;   // slight overlap onto flange

// =====================================================================
module roller_pocket(ecc = 0) {
    // through-hole for an M8 shoulder bolt; ecc shifts it for take-up
    translate([0, ecc, 0]) rotate([90, 0, 0])
        cylinder(h = 60, d = roller_bore + 0.4, center = true);
}

module hole_grid(cols, rows, pitch, d, depth) {
    for (c = [0 : cols - 1], r = [0 : rows - 1])
        translate([(c - (cols - 1) / 2) * pitch,
                   (r - (rows - 1) / 2) * pitch, -1])
            cylinder(h = depth + 2, d = d);
}

// The carriage: a vertical plate that rides the mast on six rollers, with a
// horizontal shelf at the TOP carrying the gripper. Shelf at the top, not
// the middle, is what buys the 143 mm of extra jaw height.
module carriage() {
    difference() {
        union() {
            // vertical back plate
            translate([-plate_w/2, 0, 0]) cube([plate_w, plate_t, carriage_h]);

            // arms wrapping behind the mast for the rear (preload) rollers
            for (s = [-1, 1])
                translate([s * (mast_w/2 + 4) - (s < 0 ? gusset_t : 0), 0, 0])
                    cube([gusset_t, -(mast_d + roller_od + 6), carriage_h]);

            // gripper shelf
            translate([-plate_w/2, plate_t, carriage_h - plate_t])
                cube([plate_w, shelf_d, plate_t]);

            // gussets under the shelf - this joint carries the whole
            // cantilevered gripper + box moment
            for (s = [-1, 1])
                translate([s * (plate_w/2 - gusset_t) - (s < 0 ? 0 : gusset_t),
                           plate_t, carriage_h - plate_t - 34])
                    rotate([90, 0, 90])
                        linear_extrude(gusset_t)
                            polygon([[0, 0], [shelf_d - 6, 0], [0, 34]]);

            // belt clamp boss
            translate([-16, plate_t, carriage_h/2 - 12]) cube([32, 10, 24]);
        }

        // --- front rollers: 2 top, 2 bottom, running on the mast front face
        for (sx = [-1, 1], z = [carriage_h/2 - roller_dz/2,
                                carriage_h/2 + roller_dz/2])
            translate([sx * roller_x, 0, z]) roller_pocket();

        // --- rear rollers on eccentrics, one top one bottom, for take-up
        for (z = [carriage_h/2 - roller_dz/2, carriage_h/2 + roller_dz/2])
            translate([0, -(mast_d + roller_od/2), z]) roller_pocket(preload_ecc);

        // gripper mounting grid
        translate([0, plate_t + shelf_d/2, carriage_h - plate_t])
            hole_grid(grip_hole_cols, grip_hole_rows, grip_hole_pitch,
                      grip_hole_d, plate_t);

        // belt clamp screws
        for (sx = [-1, 1])
            translate([sx * 11, plate_t - 1, carriage_h/2])
                rotate([-90, 0, 0]) cylinder(h = 14, d = 3.4);

        // weight relief - remember the 1.5 kg budget across the whole robot
        for (z = [22, carriage_h - 22])
            translate([0, -1, z]) rotate([-90, 0, 0])
                cylinder(h = plate_t + 2, d = 16, $fn = 6);
    }
}

// Clamps the belt to the carriage boss. Print the teeth face DOWN so the
// belt profile comes out crisp.
module belt_clamp() {
    difference() {
        cube([32, 9, 24], center = true);
        // belt tooth grooves
        for (i = [-7 : 7])
            translate([i * belt_pitch, -4.5, 0])
                rotate([0, 90, 0])
                    cylinder(h = 1.2, d = belt_pitch * 0.95, center = true, $fn = 16);
        for (sx = [-1, 1])
            translate([sx * 11, 0, 0]) rotate([90, 0, 0])
                cylinder(h = 20, d = 3.4, center = true);
    }
}

// Top idler bracket. The slots are the belt tensioner - slide the idler up,
// then lock. Tension it firm; a slack belt costs you repeatability, and
// repeatability is the whole game in Module G.
module idler_top() {
    w = mast_w + 2 * 8;
    difference() {
        union() {
            translate([-w/2, 0, 0]) cube([w, plate_t, 70]);
            for (s = [-1, 1])
                translate([s * (mast_w/2 + 8) - (s < 0 ? 8 : 0), 0, 0])
                    cube([8, 42, 70]);
        }
        // mast bolt holes
        for (z = [12, 30])
            translate([0, -1, z]) rotate([-90, 0, 0])
                cylinder(h = plate_t + 2, d = 4.4);
        // tension slots for the idler axle
        for (s = [-1, 1])
            hull() for (z = [48, 62])
                translate([s * (mast_w/2 + 12), 20, z])
                    rotate([0, 90, 0]) cylinder(h = 30, d = 5.2, center = true);
    }
}

// Bottom motor plate: gearmotor on one side, drive pulley on the other.
module motor_base() {
    w = mast_w + 2 * 22;
    difference() {
        union() {
            translate([-w/2, 0, 0]) cube([w, plate_t, 84]);
            translate([-w/2, plate_t, 0]) cube([w, 40, plate_t]);
        }
        // motor boss + screws
        translate([0, -1, 50]) rotate([-90, 0, 0]) {
            cylinder(h = plate_t + 2, d = motor_boss_d + 0.6);
            for (i = [0 : motor_screws - 1])
                rotate([0, 0, i * 360 / motor_screws])
                    translate([motor_pcd/2, 0, 0])
                        cylinder(h = plate_t + 2, d = motor_screw_d);
        }
        // mast bolt holes
        for (z = [10, 26])
            translate([0, -1, z]) rotate([-90, 0, 0])
                cylinder(h = plate_t + 2, d = 4.4);
        // chassis mounting
        translate([0, plate_t + 20, 0])
            hole_grid(4, 1, grip_hole_pitch, grip_hole_d, plate_t);
    }
}

// crude mast stand-in, just to read the proportions
module mast_stub(h = mast_h) {
    color("Silver")
        translate([-mast_w/2, -mast_d, 0]) cube([mast_w, mast_d, h]);
}

module assembly(carriage_z = travel) {
    mast_stub();
    color("SteelBlue") translate([0, 0, carriage_z]) carriage();
    color("Gold")      translate([0, 0, mast_h - 70]) idler_top();
    color("Tomato")    translate([0, 0, 0])           motor_base();
}

if      (PART == "assembly")   assembly();
else if (PART == "low")        assembly(0);
else if (PART == "carriage")   carriage();
else if (PART == "idler_top")  idler_top();
else if (PART == "motor_base") motor_base();
else if (PART == "belt_clamp") belt_clamp();

echo(str("carriage travel      = ", travel, " mm"));
echo(str("jaw centre above cg  = ", jaw_above, " mm"));
echo(str("HIGHEST jaw centre   = ", travel + jaw_above, " mm  (need ~580)"));
echo(str("LOWEST  jaw centre   = ", jaw_above, " mm  (need <=180)"));
echo(str("lift per motor rev   = ", lift_per_rev, " mm"));
echo(str("start height         = ", mast_h, " mm  (must be <= 600)"));
