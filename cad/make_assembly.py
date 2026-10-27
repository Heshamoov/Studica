"""
SC2026 Skill 23 — End Effector Assembly
CadQuery script — produces REAL solid STEP geometry (not a triangulated mesh)

Matches the corrected endeffector.scad (fork_combined, single U-shaped piece).

RUN
    py -m pip install cadquery      (already installed on this machine)
    cd C:\\Users\\Dell\\Documents\\Studica\\cad
    py make_assembly.py

WHAT IT PRODUCES
    fork_combined.step   print ×1 — both tines + crossbar, one piece
    clamp_arm.step        print ×1 — mounts on the servo horn
    servo_bracket.step    print ×1 — mounts to the elevator carriage
    assembly.step          all three, positioned as they sit on the robot

IMPORTING INTO AUTODESK INVENTOR
    File > Open > assembly.step
    Each part comes in as a separate solid BODY (real B-rep, like your
    claw.step reference — not a faceted mesh).  Inventor will offer to
    save each body as its own .ipt; accept, keep them in this cad/ folder.

    To make it move:
      ① Rotational Joint — clamp_arm hub axis ↔ servo_bracket shaft slot axis.
         Drive it 0°→90° to animate OPEN → HOLD.
      ② Prismatic (Slide) Joint — fork_combined ↔ whatever you model as the
         extension-arm rail, along its long axis, if you add the carriage
         itself as a part.
"""

import cadquery as cq
import math, os

OUT = os.path.dirname(os.path.abspath(__file__))

# ── dimensions (must match endeffector.scad) ──────────────────────────────────
STUD_PITCH  = 8.0
STUD_HOLE   = 3.6

TINE_W      = 18.0
TINE_T      =  5.0
TINE_L      = 120.0
TINE_TIP    = 20.0
TINE_GAP    = 10.0
CROSS_D     = 30.0
MOUNT_W     = 2 * TINE_W + TINE_GAP   # 46 mm

ARM_W       = 15.0
ARM_T       =  5.0
ARM_L       = 68.0
ARM_HUB_OD  = 20.0     # 14 mm bolt circle + 6 mm wall
ARM_HUB_T   =  5.0
PAD_W       = 30.0
PAD_T       =  3.0

SB_W        = 38.0   # clears servo body length (28) + 2 side walls (4 each) + margin
SB_H        = 38.0
SB_T        =  4.0
SB_POCKET_D = 13.0
SB_FLANGE_T =  2.5
SB_MOUNT_H  = 48.0
SRV_H       = 28.0   # servo body length along the shaft axis
SRV_EAR_Z   = 12.0   # ear position along that length
SRV_SCREW_D =  2.5


# ── Part 1 : COMBINED FORK  (one piece — both tines + crossbar) ───────────────
# Same coordinate convention as endeffector.scad:
#   origin = front face of the crossbar (where it meets the tines)
#   tines extend in −Y, crossbar extends in +Y, Z = up
#
def make_fork_combined() -> cq.Workplane:
    tine_body_len = TINE_L - CROSS_D - TINE_TIP   # 70 mm straight section

    # crossbar: MOUNT_W (46) already equals the tines' own combined span
    # (left tine X 0-18, gap, right tine X 28-46) — sits flush at X 0-46,
    # no offset needed. (Previously centred as if MOUNT_W were wider than
    # a single tine, which shifted it 14mm left and left the right tine
    # unsupported — same bug fixed in endeffector.scad.)
    crossbar = (
        cq.Workplane("XY")
          .box(MOUNT_W, CROSS_D, TINE_T, centered=(False, False, False))
    )

    # Tapered tip: loft between a full-height rectangle at the tine-body
    # junction and a half-height rectangle at the point, TINE_TIP mm apart.
    def make_tip(x_offset):
        y0 = -tine_body_len              # back of tip = front of tine body
        y1 = -tine_body_len - TINE_TIP   # tip point
        back_wire = (
            cq.Workplane("XZ", origin=(x_offset, y0, 0))
              .rect(TINE_W, TINE_T, centered=False)
              .val()
        )
        front_wire = (
            cq.Workplane("XZ", origin=(x_offset, y1, 0))
              .rect(TINE_W, TINE_T / 2, centered=False)
              .val()
        )
        solid = cq.Solid.makeLoft([back_wire, front_wire], ruled=True)
        return cq.Workplane(obj=solid)

    left_body  = (cq.Workplane("XY")
                  .box(TINE_W, tine_body_len, TINE_T, centered=(False, False, False))
                  .translate((0, -tine_body_len, 0)))
    right_body = (cq.Workplane("XY")
                  .box(TINE_W, tine_body_len, TINE_T, centered=(False, False, False))
                  .translate((TINE_W + TINE_GAP, -tine_body_len, 0)))

    left_tip  = make_tip(0)
    right_tip = make_tip(TINE_W + TINE_GAP)

    solid = (crossbar.union(left_body)
                      .union(right_body)
                      .union(left_tip)
                      .union(right_tip))

    # Studica holes through the crossbar (2×2, drilled in Z)
    # Studica U-channel pattern: 16 x 16 mm square of dia 3.5 holes
    hxs = [MOUNT_W/2 - 8, MOUNT_W/2 + 8]
    hys = [CROSS_D/2 - 8, CROSS_D/2 + 8]
    for hx in hxs:
        for hy in hys:
            hole = (cq.Workplane("XY").center(hx, hy)
                    .circle(STUD_HOLE/2).extrude(TINE_T + 2)
                    .translate((0, 0, -1)))
            solid = solid.cut(hole)

    return solid


# ── Part 2 : CLAMP ARM ────────────────────────────────────────────────────────
def make_clamp_arm() -> cq.Workplane:
    hub = cq.Workplane("XY").circle(29.0 / 2).extrude(ARM_HUB_T)   # same diameter as the Studica 76148 horn
    arm_bar = (cq.Workplane("XY")
               .box(ARM_W, ARM_L, ARM_T, centered=(False, False, False))
               .translate((-ARM_W/2, 0, 0)))
    pad = (cq.Workplane("XY")
           .box(PAD_W, PAD_W/2 + 2, ARM_T + PAD_T, centered=(False, False, False))
           .translate((-PAD_W/2, ARM_L - PAD_W/2, 0)))

    solid = hub.union(arm_bar).union(pad)

    # Studica 76148 Servo Horn 25T (vendor STEP): centre boss dia 10,
    # dia 3.5 holes on a 16 x 16 square.
    boss = (cq.Workplane("XY").circle(10.6/2)
            .extrude(ARM_HUB_T + 2).translate((0, 0, -1)))
    solid = solid.cut(boss)
    for bx in (-8, 8):
        for by in (-8, 8):
            bh = (cq.Workplane("XY").center(bx, by).circle(STUD_HOLE/2)
                  .extrude(ARM_HUB_T + 2).translate((0, 0, -1)))
            solid = solid.cut(bh)

    return solid


# -- build -------------------------------------------------------------------
# The servo bracket is NOT built here any more - see make_servo_bracket.py
# (real Studica 75002 servo dimensions). This script builds the fork and the
# clamp arm, then assembles everything around the real Studica servo.
# Vendor STEP files ship inside the repo (cad/vendor_step/) so this script
# works on any team's computer, unmodified - not tied to one person's
# Downloads folder. Override with the STUDICA_STEP_DIR env var if you keep
# them somewhere else.
D = os.environ.get("STUDICA_STEP_DIR", os.path.join(OUT, "vendor_step"))

print("Building fork_combined ...")
fork = make_fork_combined()
print("  solids:", len(fork.vals()))
print("Building clamp_arm ...")
arm = make_clamp_arm()
print("  solids:", len(arm.vals()))
cq.exporters.export(fork, os.path.join(OUT, "fork_combined.step"))
cq.exporters.export(arm, os.path.join(OUT, "clamp_arm.step"))

# -- assembly, in the SERVO's own frame (z = shaft axis) ----------------------
import make_servo_bracket as msb
bracket = msb.make()
servo = cq.importers.importStep(os.path.join(D, "75002-servo.stp"))
horn = cq.importers.importStep(os.path.join(D, "76148-horn.stp"))

SHAFT_X = -9.9
# horn: its axis is Y in the vendor file -> servo Z; disc sits on the servo boss
horn = horn.rotate((0, 0, 0), (1, 0, 0), 90).translate((SHAFT_X, 0, -25.0))
hb = horn.val().BoundingBox()
print("horn z %.1f..%.1f" % (hb.zmin, hb.zmax))
HORN_DISC_TOP = 5.0            # disc top face, 3 mm plate on the boss at z=2
# arm hub bottom on the horn disc, HOLD pose: arm points straight down
# (down = +x of the servo frame)
arm_pose = (arm.rotate((0, 0, 0), (0, 0, 1), -90)
               .translate((SHAFT_X, 0, HORN_DISC_TOP)))

s_servo = servo.val(); s_brk = bracket.val()
s_horn = horn.val(); s_arm = arm_pose.val()
for a_name, a_sh, b_name, b_sh in [("arm", s_arm, "servo", s_servo),
                                   ("arm", s_arm, "bracket", s_brk),
                                   ("horn", s_horn, "servo", s_servo),
                                   ("horn", s_horn, "bracket", s_brk),
                                   ("bracket", s_brk, "servo", s_servo)]:
    v = a_sh.intersect(b_sh).Volume()
    print("interference %-7s x %-8s %.2f mm3" % (a_name, b_name, v))

asm = (cq.Assembly()
       .add(servo, name="servo_75002", color=cq.Color(0.25, 0.25, 0.28))
       .add(bracket, name="servo_bracket", color=cq.Color(0.55, 0.55, 0.6))
       .add(horn, name="horn_76148", color=cq.Color(0.1, 0.1, 0.1))
       .add(arm_pose, name="clamp_arm", color=cq.Color(1.0, 0.6, 0.2)))
asm.export(os.path.join(OUT, "assembly.step"))
print("wrote assembly.step (servo frame: servo, bracket, horn, clamp arm)")
