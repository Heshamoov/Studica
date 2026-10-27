"""
Servo bracket for the Studica 75002 Multi-Mode Smart Servo.

Built in the SERVO'S OWN coordinate frame (the frame of "75002 - Multi-Mode
Smart Servo.stp"), so servo_bracket.step and the servo STEP drop into an
Inventor assembly with their origins coincident and no positioning needed.

All servo numbers below were measured from the vendor STEP:
  ears          z -11.2 .. -7.2, x +-27.2, y +-10
  ear holes     dia 4.5 at (+-24, +-5), axis along z
  lower body    z -38 .. -11.2, x -19.9 .. 20.5, y +-10
  upper cap     z -5.6 .. 0,  x +-19.9, y +-10
  shaft         x = -9.9, y = 0, tip at z = +5.7

The bracket: two shelves that the ears sit on (screws go down through the
ear holes into the shelf) joined by one back plate that bolts to a Studica
U-channel (76014). Channel holes are dia 3.5 at +-8 mm around each 48 mm-pitch
cell centre, plus the ring at 16 mm; the plate carries that pattern.
"""
import os
import cadquery as cq

OUT = os.path.dirname(os.path.abspath(__file__))
# Ships inside the repo (cad/vendor_step/) so this works on any computer,
# not tied to one person's Downloads folder. Override with STUDICA_STEP_DIR
# if you keep vendor STEP files somewhere else.
SERVO = os.path.join(
    os.environ.get("STUDICA_STEP_DIR", os.path.join(OUT, "vendor_step")),
    "75002-servo.stp")

CLR      = 0.4     # printed-part clearance to the servo body
PLATE_T  = 4.0     # back plate thickness
SHELF_T  = 6.0     # shelf thickness (screw grips 6 mm of plastic)
EAR_BOT  = -11.2   # underside of the ears
EAR_HOLE = [(-24.0, -5.0), (-24.0, 5.0), (24.0, -5.0), (24.0, 5.0)]
SCREW_D  = 2.9     # self-tapping M3 pilot; enlarge to 3.4 for a nut

BODY_XMIN, BODY_XMAX = -19.9, 20.5      # lower body, below the ears
SHELF_OUT = 31.5                        # outer end of each shelf
Y_SERVO = 10.0

PLATE_Z0, PLATE_Z1 = -44.0, 0.0
PLATE_X = SHELF_OUT
CELL_X, CELL_Z = 0.0, -22.0             # channel cell centre on the plate

def box(x0, x1, y0, y1, z0, z1):
    return (cq.Workplane("XY")
            .box(x1 - x0, y1 - y0, z1 - z0, centered=False)
            .translate((x0, y0, z0)))

def make():
    y_plate1 = -(Y_SERVO + CLR)
    y_plate0 = y_plate1 - PLATE_T

    plate = box(-PLATE_X, PLATE_X, y_plate0, y_plate1, PLATE_Z0, PLATE_Z1)

    z_top = EAR_BOT - 0.05
    z_bot = z_top - SHELF_T
    yb, yt = y_plate1, Y_SERVO + CLR
    shelf_r = box(BODY_XMAX + CLR, SHELF_OUT, yb, yt, z_bot, z_top)
    shelf_l = box(-SHELF_OUT, BODY_XMIN - CLR, yb, yt, z_bot, z_top)

    # Second plate: a copy of the first, bottom edge, extending AWAY from the
    # servo (-y), so the two plates make an L that wraps one channel corner.
    # Deep enough to be exactly one 48 mm channel cell (y_plate0-44 .. y_plate1).
    PLATE2_D = 48.0
    plate2 = box(-PLATE_X, PLATE_X, y_plate1 - PLATE2_D, y_plate1,
                 PLATE_Z0, PLATE_Z0 + PLATE_T)

    solid = plate.union(plate2).union(shelf_r).union(shelf_l)

    # gussets under the shelves, so they cannot peel off the plate
    for xs in ((BODY_XMAX + CLR, SHELF_OUT), (-SHELF_OUT, BODY_XMIN - CLR)):
        g = (cq.Workplane("YZ")
             .polyline([(y_plate1, z_bot), (y_plate1 + 12, z_bot), (y_plate1, z_bot - 12)])
             .close().extrude(xs[1] - xs[0]).translate((xs[0], 0, 0)))
        solid = solid.union(g)

    # servo ear screws, straight down through the ear into the shelf
    for (hx, hy) in EAR_HOLE:
        h = (cq.Workplane("XY").center(hx, hy).circle(SCREW_D / 2)
             .extrude(SHELF_T + 2).translate((0, 0, z_bot - 1)))
        solid = solid.cut(h)

    # channel pattern: dia 3.5 (3.6 printed) at +-8 mm, plus ring at 16 mm
    pts = [(sx * 8, sz * 8) for sx in (-1, 1) for sz in (-1, 1)]
    pts += [(-16, 0), (16, 0), (0, -16), (0, 16)]
    for (px, pz) in pts:
        h = (cq.Workplane("XZ").center(CELL_X + px, CELL_Z + pz).circle(3.6 / 2)
             .extrude(-(PLATE_T + 2)).translate((0, y_plate0 - 1, 0)))
        solid = solid.cut(h)

    # same pattern on plate 2 (holes drilled in z), cell centred on the plate
    p2y = y_plate1 - PLATE2_D / 2
    for (px, py) in pts:
        h = (cq.Workplane("XY").center(CELL_X + px, p2y + py).circle(3.6 / 2)
             .extrude(PLATE_T + 2).translate((0, 0, PLATE_Z0 - 1)))
        solid = solid.cut(h)
    c2 = (cq.Workplane("XY").center(CELL_X, p2y).circle(14.6 / 2)
          .extrude(PLATE_T + 2).translate((0, 0, PLATE_Z0 - 1)))
    solid = solid.cut(c2)

    # centre clearance for the channel's dia 14 hole
    c = (cq.Workplane("XZ").center(CELL_X, CELL_Z).circle(14.6 / 2)
         .extrude(-(PLATE_T + 2)).translate((0, y_plate0 - 1, 0)))
    solid = solid.cut(c)
    return solid

if __name__ == "__main__":
    b = make()
    print("solids:", len(b.solids().vals()))
    bb = b.val().BoundingBox()
    print("size X %.1f Y %.1f Z %.1f" % (bb.xlen, bb.ylen, bb.zlen))

    servo = cq.importers.importStep(SERVO).val()
    inter = servo.intersect(b.val()).Volume()
    print("interference with real servo: %.3f mm3 (must be 0)" % inter)
    print("closest approach to servo: %.2f mm" % servo.distance(b.val()))

    cq.exporters.export(b, os.path.join(OUT, "servo_bracket.step"))
    # print orientation: plate 2 (bottom face) flat on the bed, plate 1 upright
    pb = b.val().BoundingBox()
    p = b.translate((-(pb.xmin + pb.xlen / 2), -pb.ymin, -pb.zmin))
    cq.exporters.export(p, os.path.join(OUT, "servo_bracket.stl"))
    print("wrote servo_bracket.step (servo frame) and servo_bracket.stl (print pose)")
