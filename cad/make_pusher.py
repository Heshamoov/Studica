"""
Pusher arm - replaces the clamp arm. Print x1 (PETG). Print on its SIDE: the flat face of the arm plate on the bed; the paddle then stands up as a wall, no supports.

HOW IT WORKS (pendulum paddle)
  The servo sits ABOVE the back of the fork with its shaft horizontal and
  sideways (across the robot). This arm hangs straight down from the horn;
  its paddle's bottom edge sits just above the fork's top surface, behind
  the piece. Swinging the servo moves the paddle forward in an arc and slides
  the piece along the fork and off the tip, so it drops into the closed pocket.

  With arm radius R (shaft centre -> paddle bottom) and a swing of +-A degrees
  about straight-down:
      forward travel = 2 * R * sin(A)
      paddle rise at the ends = R * (1 - cos(A))   (must stay < piece thickness 10 mm)
  Defaults R = 115, A = 22 deg -> travel 86 mm, rise 8.4 mm. Enough to push a
  75 mm piece (back edge ~80 mm behind the fork tip) past the tip while still
  touching its back edge.

  Settings page: set Retract angle = centre - 22, Push angle = centre + 22
  (centre = the servo angle at which the arm hangs straight down), and keep
  Push speed slow (about 40 deg/s) so the piece slides instead of flipping.

MOUNTING
  Hub matches the Studica 76148 horn: Ø29 disc, Ø10.6 clearance for the horn
  boss, four Ø3.6 holes on a 16 x 16 mm square (M3 screws into the horn).
  Mount the servo with the existing servo_bracket on the carriage so that the
  shaft centre is R mm above the fork top surface and about R*sin(A) + 5 mm
  behind the piece's back edge.
"""
import os, math
import cadquery as cq

OUT = os.path.dirname(os.path.abspath(__file__))

R        = 115.0   # shaft centre -> paddle bottom edge (mm); 15 mm shorter (2026-10-01) to clear the fork mount
SWING    = 22.0    # degrees each side of straight down
HUB_D    = 29.0    # = Studica 76148 horn disc
HUB_T    = 5.0
BOSS_D   = 10.6
HOLE_D   = 3.6
HOLE_SQ  = 16.0    # horn screw square
ARM_W    = 14.0
ARM_T    = 5.0
PAD_W    = 60.0    # across the robot: wider than the 46 mm fork, narrower than the 75 mm piece
PAD_H    = 18.0    # contact face height
PAD_T    = 8.0
RIB      = 4.0


def make():
    # Built frame: arm plate lies in XY (the swing plane), Y = down along the
    # arm, Z = the servo shaft axis. The PADDLE is a crossbar along Z (across
    # the robot), so it pushes the whole back edge of the piece.
    hub = cq.Workplane("XY").circle(HUB_D / 2).extrude(HUB_T)
    bar_len = R - PAD_H / 2
    bar = (cq.Workplane("XY").center(0, -bar_len / 2)
           .rect(ARM_W, bar_len).extrude(ARM_T))
    # paddle: PAD_T along the swing, PAD_H radial, PAD_W across the robot,
    # centred on the arm plate's mid-thickness
    pad = (cq.Workplane("XY").center(0, -(R - PAD_H / 2))
           .rect(PAD_T, PAD_H).extrude(PAD_W)
           .translate((0, 0, ARM_T / 2 - PAD_W / 2)))
    # two ribs tying the paddle to the arm so it cannot twist
    ribs = None
    for sgn in (1, -1):
        r = (cq.Workplane("YZ")
             .polyline([(-(R - PAD_H), ARM_T / 2), (-(R - PAD_H) + 30, ARM_T / 2),
                        (-(R - PAD_H), ARM_T / 2 + sgn * (PAD_W / 2 - 3))]).close()
             .extrude(RIB / 2, both=True))
        ribs = r if ribs is None else ribs.union(r)
    solid = hub.union(bar).union(pad).union(ribs)
    solid = solid.cut(cq.Workplane("XY").circle(BOSS_D / 2).extrude(HUB_T + 2).translate((0, 0, -1)))
    h = HOLE_SQ / 2
    for x, y in [(h, h), (-h, h), (h, -h), (-h, -h)]:
        solid = solid.cut(cq.Workplane("XY").center(x, y).circle(HOLE_D / 2)
                          .extrude(HUB_T + 2).translate((0, 0, -1)))
    return solid


if __name__ == "__main__":
    part = make()
    cq.exporters.export(part, os.path.join(OUT, "pusher_arm.step"))
    cq.exporters.export(part, os.path.join(OUT, "pusher_arm.stl"))
    bb = part.val().BoundingBox()
    travel = 2 * R * math.sin(math.radians(SWING))
    rise = R * (1 - math.cos(math.radians(SWING)))
    print(f"pusher_arm: {bb.xlen:.1f} x {bb.ylen:.1f} x {bb.zlen:.1f} mm, "
          f"volume {part.val().Volume() / 1000:.1f} cm3")
    print(f"swing +-{SWING} deg -> forward travel {travel:.1f} mm, end rise {rise:.1f} mm")
