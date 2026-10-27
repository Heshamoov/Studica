// =====================================================================
// SC2026 Skill 23 - fork / tray end effector
//
// THE OBJECT AND THE OPENING (SC2026_TP23 appendix)
//   flat piece    75 x 75 x 10 mm, R3 corners, lying FLAT
//   storage base  100 mm cube, 80 x 80 cavity, 5 mm walls,
//                 50 mm wide x 30 mm tall opening in the front face
//
// WHY A FORK AND NOT A GRIPPER
// The piece lies flat and is reached through a 50 x 30 mm hole. Nothing
// that squeezes can work: a parallel jaw needs to get around the object,
// and there is no room. A fork does not grip at all - it slides underneath,
// lifts a few millimetres, and carries the piece on top of itself. Fewer
// parts, no servo, no grip force to tune, and nothing to drop.
//
// THE SEQUENCE
//   1  lift to just below the piece
//   2  EXTEND the fork in through the opening, under the piece
//   3  lift ~8 mm  -> the piece is now resting on the fork
//   4  RETRACT
//   5  drive, lift to the stand height, extend, lower, retract
//
// HOW THE PIECE SITS - resolved from the isometric view
// The cavity is 80 x 80 with a 50 mm wide channel running through the middle
// of it. That leaves a LEDGE 15 mm wide down each side. The 75 mm piece
// spans the channel and rests on those two ledges, overlapping each by
// 12.5 mm. The 50 x 30 opening in the front face is the entrance to that
// channel - it exists so a fork can reach in UNDERNEATH the piece.
//
// That also fixes the rest height, which the drawing never states outright:
//   rim to ledge          12.50   (the recess depth, dashed on the drawing)
//   so the ledge sits at  100 - 12.5 = 87.5 mm
//   piece top at          87.5 + 10 = 97.5 mm, i.e. 2.5 mm below the rim
//   opening 30 mm tall, its top AT the ledge  ->  spans 57.5 .. 87.5 mm
// Every dimension on the drawing is accounted for by that reading.
//
// There is NO front wall above the ledge - just the 12.5 mm recess - so once
// the fork has lifted the piece past 87.5 mm it is in open air and can rise
// freely over the rim. Extraction is:
//   extend under the piece -> lift ~8 mm (load transfers to the fork)
//   -> lift to ~105 mm (piece clears the 100 mm rim) -> retract -> drive
//
// STILL WORTH A RULER: confirm 87.5 mm on the real base before printing a
// full set. It is the one number the drawing implies rather than states.
//
// =====================================================================

PART = "assembly";  // assembly | fork | bracket | pad | in_base

$fn = 48;

/* [The opening we must pass through] -------------------- from the drawing */
slot_w        = 50.0;   // front opening width
slot_h        = 30.0;   // front opening height
base_outer    = 100.0;
base_cavity   = 80.0;
base_wall     = 5.0;

/* [The piece] */
piece_w       = 75.0;
piece_t       = 10.0;
piece_r       = 3.0;

/* [Fork] */
// Width: the opening is 50, so 44 leaves 3 mm each side. Do not go wider -
// the robot will never be aligned to better than a couple of millimetres,
// and a fork that jams in the opening ends the run.
fork_w        = 44.0;
fork_t        = 5.0;    // thin enough to slide under a 10 mm piece with room
fork_len      = 105.0;  // must reach past the piece centre: 75 + clearance
tip_taper     = 14.0;   // wedge at the leading edge, noses under the piece
tip_thin      = 1.6;    // thickness at the very tip

/* [Retention] */
// The piece only has to survive being carried, not clamped. A rear stop and
// two low rails are enough; anything taller fouls the opening on the way in.
rail_h        = 3.0;
rail_w        = 3.0;
stop_h        = 11.0;   // just over the 10 mm piece
stop_t        = 5.0;

/* [Mount to the extension carriage] -------------------- Studica pattern */
// CONFIRMED from the catalogue: 3.5 mm holes on an 8 mm pitch, 3 mm 6061-T6.
kit_hole_d    = 3.6;
kit_pitch     = 8.0;
kit_rows      = 2;
kit_cols      = 5;
kit_row_pitch = 24.0;
plate_t       = 6.0;
plate_w       = 60.0;
plate_h       = 44.0;

/* [Print] */
nozzle        = 0.4;

// =====================================================================
// derived
// =====================================================================
clear_each_side = (slot_w - fork_w) / 2;
head_room       = slot_h - fork_t - piece_t;   // room left in the opening
reach_past      = fork_len - piece_w;

// =====================================================================
module rounded_plate(w, d, t, r) {
    hull() for (x = [r, w - r], y = [r, d - r])
        translate([x, y, 0]) cylinder(h = t, r = r);
}

// The tine. Flat, tapered at the tip, rails and a back stop.
module fork() {
    difference() {
        union() {
            // main blade, with the leading edge wedged
            hull() {
                translate([0, 0, 0]) cube([fork_len - tip_taper, fork_w, fork_t]);
                translate([fork_len - 0.01, 0, 0])
                    cube([0.01, fork_w, tip_thin]);
            }
            // NO SIDE RAILS. The piece is 75 mm wide and the fork can only be
            // 44 mm, so it overhangs by ~15 mm each side - rails would sit
            // under the piece and guide nothing. Retention is the back stop
            // plus a high-friction pad; see pad() below.
            // back stop: what actually keeps the piece aboard while driving
            cube([stop_t, fork_w, stop_h]);
        }
        // lightening slots - the 1.2 kg print budget is the binding constraint
        for (i = [0 : 2])
            translate([26 + i * 22, fork_w / 2, -1])
                hull() for (dy = [-7, 7])
                    translate([0, dy, 0]) cylinder(h = fork_t + 2, d = 8);
    }
}

// TPU friction pad, printed separately and glued to the top of the fork.
// This is what stops a 75 mm piece sliding off a 44 mm tine while the robot
// accelerates. Print in TPU 95A at ~30% infill.
module pad() {
    pad_l = fork_len - tip_taper - stop_t - 18;
    difference() {
        translate([stop_t + 4, 3, 0]) cube([pad_l, fork_w - 6, 1.6]);
        for (i = [0 : 2])
            translate([26 + i * 22, fork_w / 2, -1])
                hull() for (dy = [-7, 7])
                    translate([0, dy, 0]) cylinder(h = 4, d = 8);
    }
}


// Bolts the fork to the extension carriage.
module bracket() {
    difference() {
        union() {
            // vertical face against the carriage
            cube([plate_t, plate_w, plate_h]);
            // horizontal shelf the fork bolts onto
            cube([34, plate_w, plate_t]);
            // gussets: this joint carries the whole cantilevered load
            for (y = [0, plate_w - 5])
                translate([plate_t, y, plate_t])
                    rotate([0, 0, 0])
                        linear_extrude(5)
                            polygon([[0, 0], [22, 0], [0, 26]]);
        }
        // carriage hole grid
        for (r = [0 : kit_rows - 1], c = [0 : kit_cols - 1])
            translate([-1,
                       plate_w/2 + (c - (kit_cols-1)/2) * kit_pitch,
                       plate_h/2 + (r - (kit_rows-1)/2) * kit_row_pitch])
                rotate([0, 90, 0]) cylinder(h = plate_t + 2, d = kit_hole_d);
        // fork mounting holes
        for (c = [-1, 1])
            translate([20, plate_w/2 + c * 14, -1])
                cylinder(h = plate_t + 2, d = kit_hole_d);
    }
}

// --------------------------------------------------------------- previews
module piece() {
    color("Crimson")
        translate([0, 0, 0])
            hull() for (x = [piece_r, piece_w - piece_r],
                        y = [piece_r, piece_w - piece_r])
                translate([x, y, 0]) cylinder(h = piece_t, r = piece_r);
}

module storage_base() {
    ledge_h = base_outer - 12.5;      // 87.5 mm - see the header
    color("WhiteSmoke", 0.5)
    difference() {
        cube([base_outer, base_outer, base_outer]);
        // recess above the ledges, open to the top
        translate([base_wall, base_wall, ledge_h])
            cube([base_cavity, base_cavity, base_outer]);
        // the channel through the middle, below the ledges
        translate([-1, (base_outer - slot_w)/2, ledge_h - slot_h])
            cube([base_outer + 2, slot_w, slot_h + base_outer]);
        // hollow underneath, so it reads like the real part
        translate([base_wall, base_wall, -1])
            cube([base_cavity, base_cavity, ledge_h - slot_h + 1]);
    }
}

// The fork mid-insertion, to check nothing fouls.
module in_base(insert = 78, lifted = 0) {
    ledge_h = base_outer - 12.5;
    storage_base();
    // the piece, resting on the two 15 mm ledges
    translate([(base_outer - piece_w)/2, (base_outer - piece_w)/2,
               ledge_h + lifted])
        piece();
    // the fork, in the channel underneath it
    color("SteelBlue")
        translate([base_outer - insert, (base_outer - fork_w)/2,
                   ledge_h - fork_t - 1 + lifted])
            fork();
}

module assembly() {
    color("SteelBlue") translate([34, 0, 0]) fork();
    color("DimGray")   translate([34, 0, fork_t]) pad();
    color("Goldenrod") bracket();
    translate([34 + 22, (fork_w - piece_w)/2, fork_t + 1.6]) piece();
}

if      (PART == "assembly") assembly();
else if (PART == "fork")     fork();
else if (PART == "bracket")  bracket();
else if (PART == "pad")      pad();
else if (PART == "in_base")  in_base();

echo(str("fork ", fork_w, " x ", fork_t, " mm in a ", slot_w, " x ", slot_h,
         " mm opening"));
echo(str("clearance each side  = ", clear_each_side, " mm"));
echo(str("head room above fork = ", head_room, " mm  (piece is ", piece_t, ")"));
echo(str("reach past the piece = ", reach_past, " mm"));
echo(str("ledge / piece rest   = ", base_outer - 12.5, " mm  <- CONFIRM"));
echo(str("rise to clear rim    = ", 12.5 + 5, " mm above the ledge"));
echo(str("MUST be positive: side ", clear_each_side > 1,
         "  head ", head_room > 0, "  reach ", reach_past > 0));
