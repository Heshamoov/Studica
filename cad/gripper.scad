// =====================================================================
// ESC2026 Skill 23 - parallel-jaw gripper, rack & pinion, single servo
//
// Design brief taken straight from the CID:
//   Module G - delivery box is 60 x 60 x 150 mm, red/green/blue.
//              Shelves at 150-250 mm and 450-550 mm, 200 mm deep.
//   Module F - pick up an UNKNOWN object (sample: a chili pepper),
//              drive 1 m, turn, come back, put it down again, without
//              crushing it. "must apply appropriate forces and torques".
//   Robot must fit 600 x 600 x 600 mm at the start.
//   3D printed parts <= 1.5 kg total, ABS/PLA/Nylon/PETG/HIPS/ASA/TPU/CF.
//
// THE CENTRAL TENSION
// Module G wants a rigid jaw sized to a known 60 mm box. Module F wants
// compliance for something you have never seen. One gripper must do both.
//
// The answer here is a rigid parallel-jaw mechanism with SPRUNG jaw pads.
// The servo commands jaw *position*, and the pads deflect on a flexure to
// turn overtravel into a bounded, repeatable force. You cannot read servo
// current on a hobby servo, so you cannot close a force loop in software -
// so put the force control in the mechanism instead. Command a position
// ~6-8 mm tighter than the object and the flexure sets the squeeze.
//
// USAGE
//   openscad gripper.scad                      // preview the assembly
//   Set PART below, or use -D PART=\"base\" on the command line, then
//   Design > Render (F6) > Export as STL.
//
// EVERYTHING IS PARAMETRIC. The numbers you MUST verify on the real kit
// are marked  <-- MEASURE.  Do not print a full set until you have.
// =====================================================================

PART = "assembly";   // assembly | base | pinion | rack | jaw | pad | all_plate

$fn = 64;

/* [Object being gripped] */
box_w          = 60;    // CID G.3.1 delivery box cross-section
// 80 mm open leaves 10 mm of approach error either side of the 60 mm box.
// Was 95, but the mounting bracket under the camera is only ~110 mm long,
// and every mm of opening is a mm of plate length. 10 mm of side margin is
// enough IF the RealSense is doing the alignment - which is the whole point
// of putting the camera on the carriage.
jaw_open_max   = 80;
jaw_open_min   = 12;    // small enough for a chili pepper stem

/* [Servo] --------------------------------------------------- <-- MEASURE */
// Defaults are a standard 40x20x38 mm servo with a 25T spline.
servo_body_l   = 40.5;
servo_body_w   = 20.2;
servo_body_h   = 38.0;
servo_flange_l = 54.0;
servo_flange_t = 2.8;
servo_flange_hole_dx = 49.5;  // between the two mounting-hole centres
servo_flange_hole_dy = 10.0;
servo_screw_d  = 2.6;         // M2.5 self-tap
servo_shaft_d  = 5.9;         // spline OD - pinion bonds to a horn, not this
servo_horn_d   = 21.0;        // round horn OD the pinion seats over
servo_horn_hole_pcd = 16.0;   // horn screw circle
servo_horn_holes = 4;
servo_horn_screw_d  = 2.2;    // M2 through the pinion into the horn

/* [Gear] */
gear_module    = 2.0;
pinion_teeth   = 16;          // pitch dia 32 mm -> 50 mm rack travel per 180 deg
gear_face      = 8.0;         // tooth width
pressure_angle = 20;

/* [Structure] */
base_t         = 6.0;         // base plate thickness
rack_h         = 10.0;        // rack body height (above the teeth)
rack_t         = 8.0;         // rack thickness (into the plate)
jaw_h          = 55.0;        // jaw finger height - grabs the box mid-face
jaw_t          = 8.0;
pad_t          = 5.0;         // TPU pad thickness
clear          = 0.35;        // sliding-fit clearance, tune to your printer

/* [Compliant flexure] */
// The pad sits on two thin beams. Thinner = softer = safer on unknown
// objects, but too thin and the box wobbles while you drive. Start at 1.6
// and test with the actual box AND something fragile.
flex_t         = 1.6;
flex_l         = 14.0;
flex_gap       = 4.0;         // how far the pad can deflect before it bottoms
                              // out on a hard stop - this CAPS the force

/* [Mounting to the Studica kit] */
// The carriage bracket under the camera is 120 mm long with ~110 mm usable,
// so the mounting footprint must stay inside that.
mount_span     = 110.0;       // usable length on the camera bracket

// CONFIRMED from the Studica catalogue: the common hole pattern across all
// their structure is 3.5 mm holes on an 8 mm pitch, in 3 mm 6061-T6
// ALUMINIUM (not steel - it is much lighter than you'd assume from handling
// it, which matters when you are budgeting mass).
// The 14 mm you measured is the big round LIGHTENING holes, not the bolt
// grid. Bolts are M3 through the 3.5 mm holes.
kit_hole_pitch = 8.0;
kit_hole_d     = 3.6;         // M3 clearance in a printed part
kit_hole_cols  = 11;          // 80 mm span, inside the 110 mm bracket
kit_hole_rows  = 2;
kit_row_pitch  = 48.0;        // = the 48 mm U-channel profile width
kit_plate_t    = 3.0;         // channel / bracket wall thickness

// =====================================================================
// derived
// =====================================================================
pitch_d   = gear_module * pinion_teeth;      // 32 mm
pitch_r   = pitch_d / 2;
tooth_p   = gear_module * PI;                // 6.283 mm circular pitch
addendum  = gear_module;
dedendum  = 1.25 * gear_module;

// travel needed per rack, from centre
travel    = (jaw_open_max - jaw_open_min) / 2;
rack_len  = jaw_open_max / 2 + travel + 25;  // teeth + overrun + jaw boss

// The two racks straddle the pinion at DIFFERENT heights, so they overlap
// each other freely - the plate never needs to be 2 x rack_len. It only has
// to span the fully-open jaws plus the guide rails. Sizing it off the rack
// length was simply wasteful and made the plate 238 mm long.
base_l    = jaw_open_max + 2 * jaw_t + 40;
base_w    = max(servo_flange_l + 10, 74);

servo_rot = travel / (PI * pitch_d) * 360;   // degrees of servo for full range

// =====================================================================
// gear teeth
//
// Simplified trapezoidal profile, not a true involute. At module 2 in
// plastic under a light gripper load this meshes fine and prints cleanly,
// and it keeps this file readable. If you later want a proper involute,
// swap in the BOSL2 gears library - the interface (pitch_d, module) is
// unchanged.
// =====================================================================
module tooth_2d(h_add, h_ded, w_pitch) {
    // half-angle taper from the pressure angle
    dx = (h_add + h_ded) * tan(pressure_angle);
    polygon([
        [-w_pitch/2 - dx*0.5, -h_ded],
        [ w_pitch/2 + dx*0.5, -h_ded],
        [ w_pitch/2 - dx*0.5,  h_add],
        [-w_pitch/2 + dx*0.5,  h_add],
    ]);
}

module pinion() {
    tw = tooth_p * 0.42;   // tooth thickness at pitch line, leaves backlash
    difference() {
        union() {
            cylinder(h = gear_face, d = pitch_d - 2*dedendum + 0.01);
            for (i = [0 : pinion_teeth - 1])
                rotate([0, 0, i * 360 / pinion_teeth])
                    translate([0, pitch_r, 0])
                        linear_extrude(gear_face)
                            tooth_2d(addendum, dedendum, tw);
            // hub to bond onto the servo horn
            translate([0, 0, gear_face])
                cylinder(h = 4, d = servo_horn_d + 6);
        }
        // horn pocket
        translate([0, 0, -0.1]) cylinder(h = 3.2, d = servo_horn_d + 0.4);
        translate([0, 0, -1])   cylinder(h = gear_face + 8, d = servo_shaft_d + 1.5);
        for (i = [0 : servo_horn_holes - 1])
            rotate([0, 0, i * 360 / servo_horn_holes])
                translate([servo_horn_hole_pcd/2, 0, -1])
                    cylinder(h = gear_face + 8, d = servo_horn_screw_d);
    }
}

// rack: teeth face DOWN toward the pinion at y=0
module rack(len = rack_len) {
    n = floor(len / tooth_p);
    tw = tooth_p * 0.42;
    union() {
        translate([0, 0, 0]) cube([len, rack_h, rack_t]);
        for (i = [0 : n - 1])
            translate([i * tooth_p + tooth_p/2, 0, 0])
                rotate([90, 0, 90])
                    translate([0, 0, -tw/2])
                        linear_extrude(tw)
                            polygon([
                                [-rack_t/2 - addendum*tan(pressure_angle), 0],
                                [ rack_t/2 + addendum*tan(pressure_angle), 0],
                                [ rack_t/2 - addendum*tan(pressure_angle), -addendum],
                                [-rack_t/2 + addendum*tan(pressure_angle), -addendum],
                            ]);
    }
}

// jaw finger with an integral flexure and a hard stop
module jaw() {
    difference() {
        union() {
            // spine
            cube([jaw_t, rack_h + 6, jaw_h]);
            // foot that bolts to the rack
            cube([jaw_t + 14, rack_h + 6, 10]);
        }
        // rack bolt holes
        for (z = [4]) for (y = [(rack_h+6)/2])
            translate([-1, y, z]) rotate([0, 90, 0])
                cylinder(h = jaw_t + 20, d = 3.4);
    }

    // --- compliant pad carrier ---
    // two thin beams in bending; flex_gap is the hard stop that bounds force
    translate([jaw_t, 0, 12]) {
        for (z = [0, jaw_h - 26])
            translate([0, 0, z])
                cube([flex_l, flex_t, 10]);
        translate([flex_l, 0, 0])
            cube([4, rack_h + 6, jaw_h - 16]);
    }
    // hard stop behind the flexure
    translate([jaw_t, 0, 12 + (jaw_h-26)/2 - 4])
        cube([flex_l - flex_gap, flex_t + 2, 8]);
}

// TPU pad - print in TPU 95A, ~30% infill
module pad() {
    difference() {
        union() {
            cube([pad_t, rack_h + 6, jaw_h - 16]);
            // shallow V so cylindrical objects (the chili pepper) self-centre
            translate([pad_t, (rack_h+6)/2, 0])
                rotate([0, 0, 45])
                    cube([4, 4, jaw_h - 16]);
        }
        for (z = [10, jaw_h - 30])
            translate([-1, (rack_h+6)/2, z]) rotate([0, 90, 0])
                cylinder(h = pad_t + 2, d = 2.6);
    }
}

module kit_holes() {
    for (r = [0 : kit_hole_rows - 1], c = [0 : kit_hole_cols - 1])
        translate([base_l/2 + (c - (kit_hole_cols-1)/2) * kit_hole_pitch,
                   base_w/2 + (r - (kit_hole_rows-1)/2) * kit_row_pitch,
                   -1])
            cylinder(h = base_t + 2, d = kit_hole_d);
}

module base() {
    difference() {
        union() {
            cube([base_l, base_w, base_t]);
            // rack guide rails
            for (s = [0, 1])
                translate([0, base_w/2 + (s ? 1 : -1) * (rack_h + 6 + clear), 0])
                    cube([base_l, 5, base_t + rack_t + 3]);
        }
        // servo pocket
        translate([base_l/2 - servo_body_l/2 - clear,
                   base_w/2 - servo_body_w/2 - clear, -1])
            cube([servo_body_l + 2*clear, servo_body_w + 2*clear, base_t + 2]);
        // servo flange screws
        for (sx = [-1, 1], sy = [-1, 1])
            translate([base_l/2 + sx * servo_flange_hole_dx/2,
                       base_w/2 + sy * servo_flange_hole_dy/2, -1])
                cylinder(h = base_t + 2, d = servo_screw_d);
        kit_holes();
        // weight relief - you have a 1.5 kg budget across ALL printed parts
        for (i = [-3 : 3])
            translate([base_l/2 + i * 22, base_w/2, -1])
                cylinder(h = base_t + 2, d = 11, $fn = 6);
    }
}

// =====================================================================
// `open` = jaw gap in mm, so you can eyeball the closed and open extremes:
//    assembly(jaw_open_max);  assembly(box_w);  assembly(jaw_open_min);
//
// The two racks MUST straddle the pinion - one meshing above its centre, one
// below. That is what turns a single servo rotation into two jaws moving in
// OPPOSITE directions. Put both racks on the same side and the jaws travel
// together and the gripper never closes.
module assembly(open = jaw_open_max) {
    cx = base_l / 2;
    cy = base_w / 2;
    d  = open / 2;               // each jaw face this far from centreline
    jy = cy - (rack_h + 6) / 2;  // jaws centred in y; the foot bridges out to
                                 // its own rack, abstracted away in this preview

    color("DimGray") base();
    color("Gold") translate([cx, cy, base_t + rack_t + 1]) pinion();

    // rack A - meshes ABOVE the pinion, teeth pointing -y. Drives the LEFT jaw.
    color("SteelBlue")
        translate([cx - d - jaw_t, cy + pitch_r, base_t]) rack();
    color("Tomato")
        translate([cx - d - jaw_t, jy, base_t + rack_t]) jaw();

    // rack B - meshes BELOW the pinion, teeth pointing +y. Drives the RIGHT jaw.
    color("SteelBlue")
        translate([cx + d + jaw_t, cy - pitch_r, base_t])
            rotate([0, 0, 180]) rack();
    color("Tomato")
        translate([cx + d + jaw_t, jy, base_t + rack_t])
            mirror([1, 0, 0]) jaw();
}

module all_plate() {
    base();
    translate([0, base_w + 15, 0]) pinion();
    translate([60, base_w + 15, 0]) rack();
    translate([0, base_w + 60, 0]) jaw();
    translate([40, base_w + 60, 0]) jaw();
}

if      (PART == "assembly") assembly();
else if (PART == "closed")   assembly(box_w);
else if (PART == "base")     base();
else if (PART == "pinion")   pinion();
else if (PART == "rack")     rack();
else if (PART == "jaw")      jaw();
else if (PART == "pad")      pad();
else if (PART == "all_plate") all_plate();

echo(str("pitch dia = ", pitch_d, " mm"));
echo(str("travel per rack = ", travel, " mm"));
echo(str("servo rotation needed = ", servo_rot, " deg  (must be < 180)"));
echo(str("base plate = ", base_l, " x ", base_w, " mm   (bracket usable = ", mount_span, " mm)"));
echo(str("jaw span at full open = ", jaw_open_max + 2*jaw_t, " mm"));
