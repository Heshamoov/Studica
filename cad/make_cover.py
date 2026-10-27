"""
Robot cover (hood) - first draft from the measurements in the photos (2026-10-02).

A thin open-bottom shell that drops over the robot body.

    robot body  : 335 mm long x 240 mm wide, cover height 180 mm
    sits ON the frame: 2 mm smaller each side, 1 cm shorter at the front
    square hole : 50 x 50 mm through the top, starting 40 mm from the front, RIGHT side
    panel cut   : open top-left corner (camera side), 65 mm to 190 mm along the length

Everything below is a parameter - change a number, run again:
    py make_cover.py

It is too big for most printer beds (33.5 x 24 cm), so it is SPLIT into a
front half and a back half that overlap with a lap joint and bolt together
with M3 screws. Each half prints upside down (top on the bed), no supports
except the small mounting tabs.

ASSUMPTIONS TO CHECK (could not be read for certain from the photos):
  * the measurements are the robot's outside size; the cover adds CLEAR mm
    of clearance on each side
  * mast hole position (MAST_X from the front edge, MAST_Y from the left edge)
  * which side wall gets the cut-out, and how tall it is (CUT_SIDE, CUT_H)
"""
import os
import cadquery as cq

OUT = os.path.dirname(os.path.abspath(__file__))

# ---- robot size (from the photos) ----------------------------------------
ROBOT_L = 335.0          # length
ROBOT_W = 240.0          # width
HEIGHT  = 180.0          # cover height (top surface above its bottom edge)

# ---- cover ---------------------------------------------------------------
# The cover must REST ON the robot frame, not slide down over it, so it is a
# little SMALLER than the robot: its walls stand on the top of the frame rails.
INSET = 2.0              # cover is this much smaller than the robot, each side
FRONT_SHORT = 10.0       # 1 cm shorter at the FRONT (the depth-camera end)
WALL  = 1.2              # wall and top thickness: 1.2 mm = 3 lines of a 0.4 mm nozzle (~415 g total; 2.0 mm was ~690 g)
L = ROBOT_L - FRONT_SHORT - 2 * INSET
W = ROBOT_W - 2 * INSET

# ---- mast hole (top) -----------------------------------------------------
MAST = 50.0              # 5 x 5 cm square hole through the top
MAST_X = 40.0 + MAST / 2 # hole STARTS 4 cm from the front edge
MAST_Y = W - MAST / 2    # RIGHT side, flush with the edge: no plastic strip beside it

# ---- side cut-out --------------------------------------------------------
CUT_FROM = 65.0          # along the length, from the front edge
CUT_TO   = 190.0
# Sized from the panel measurements (triangle 7 / 14 / 17 cm): the button face
# (14 cm) leans ~24 deg from vertical, so from above it spans 14*sin(24) = 5.6 cm
# and it reaches 14*cos(24) = 12.8 cm down from just under the cover top. The
# opening shows only that button face; the rest of the panel stays covered.
PANEL_FACE = 70.0        # panel plate depth, seen edge-on in the side photo (7 cm)
PANEL_TILT = 53.4        # degrees from vertical (triangle 7/14/17 cm: angle between 7 and 17)
import math as _m
# Panel geometry (from the photos): button face 14 cm, tilted 24 deg from
# vertical, buttons facing UP and OUT. Its BOTTOM edge is outside the frame,
# its TOP edge is inside; more than half of it is outside.
PANEL_TOP_Z  = 170.0     # height of the panel's top edge (17 cm)           <-- CHECK
PANEL_OUT_MM = 60.0      # how far the panel's BOTTOM edge sticks out past the body (measured: 6 cm)
PANEL_GAP    = 3.0       # clearance around the plate
SPAN  = PANEL_FACE * _m.sin(_m.radians(PANEL_TILT))         # ~57 mm across
DROP  = PANEL_FACE * _m.cos(_m.radians(PANEL_TILT))         # ~128 mm tall
OUT_B = PANEL_OUT_MM                                        # bottom edge outside the wall
IN_T  = SPAN - OUT_B          # top edge: >0 inside the wall, <0 still outside (6 cm -> -4 mm)
PANEL_BOT_Z = PANEL_TOP_Z - DROP                            # ~42 mm
CROSS_Z = PANEL_BOT_Z + DROP * min(1.0, OUT_B / SPAN)      # where the plate passes the wall (top if it never does)
TOP_L = 140.0            # top opening over the panel: 14 cm along the length
TOP_W = 50.0             # ... and 5 cm in from the left edge
TOP_FROM = (CUT_FROM + CUT_TO) / 2 - TOP_L / 2   # centred on the wall opening (57.5-197.5 mm)
CUT_SIDE = "left"        # LEFT wall = the side facing the camera

# ---- split + joint + mounting -------------------------------------------
SPLIT_X = max(CUT_TO, TOP_FROM + TOP_L) + 15.0  # front/back joint just past the side cut-out, so the cut-out is in one piece
LAP = 15.0               # overlap of the lap joint
TAB = 14.0               # mounting tab size (inside, at the bottom edge)
HOLE = 3.6               # M3 clearance


def shell():
    outer = cq.Workplane("XY").box(L, W, HEIGHT, centered=(False, False, False))
    inner = (cq.Workplane("XY").box(L - 2 * WALL, W - 2 * WALL, HEIGHT - WALL,
                                     centered=(False, False, False))
             .translate((WALL, WALL, 0)))
    s = outer.cut(inner)
    # mast hole through the top
    # ...and the RIGHT side wall below it is open too, full height (a slot
    # through the top edge and down the whole right wall).
    s = s.cut(cq.Workplane("XY").box(MAST, MAST + 2, WALL + 2)
              .translate((MAST_X, MAST_Y + 1, HEIGHT - WALL / 2 + 1)))
    s = s.cut(cq.Workplane("XY").box(MAST, WALL + 2, HEIGHT + 2, centered=False)
              .translate((MAST_X - MAST / 2, W - WALL - 1, -1)))
    # side cut-out, open from the bottom edge
    # Axes: x = 0 at the FRONT (camera end), x grows toward the back.
    # Standing behind the robot looking forward: LEFT wall is y = 0, RIGHT is y = W.
    # Control panel (left wall): OPEN, like the slot on the right side - the
    # left wall is removed for the panel's whole length (CUT_FROM..CUT_TO) and
    # the whole height, bottom edge to top. No plastic inside that opening.
    s = s.cut(cq.Workplane("XY").box(CUT_TO - CUT_FROM, WALL + 2, HEIGHT + 2, centered=False)
              .translate((CUT_FROM, -1, -1)))
    # ...and the TOP over the panel is open too: 14 x 5 cm from the left edge
    s = s.cut(cq.Workplane("XY").box(TOP_L, TOP_W + 1, WALL + 2, centered=False)
              .translate((TOP_FROM, -1, HEIGHT - WALL - 1)))
    # four mounting tabs inside the corners at the bottom edge, with an M3 hole
    for (x, y) in [(WALL, WALL), (L - WALL - TAB, WALL),
                   (WALL, W - WALL - TAB), (L - WALL - TAB, W - WALL - TAB)]:
        tab = (cq.Workplane("XY").box(TAB, TAB, 3, centered=False).translate((x, y, 0))
               .cut(cq.Workplane("XY").circle(HOLE / 2).extrude(5)
                    .translate((x + TAB / 2, y + TAB / 2, -1))))
        s = s.union(tab)
    return s


def halves(s):
    """Split at SPLIT_X with a lap joint: the back half keeps an inner lip
    LAP mm long that slides under the front half, with M3 holes through both."""
    big = 2000
    front = s.intersect(cq.Workplane("XY").box(SPLIT_X, big, big, centered=False)
                        .translate((0, -big / 2, -big / 2)))
    back = s.intersect(cq.Workplane("XY").box(big, big, big, centered=False)
                       .translate((SPLIT_X, -big / 2, -big / 2)))
    # inner lip on the back half (a sleeve just inside the walls/top)
    lip_outer = (cq.Workplane("XY").box(LAP, W - 2 * WALL, HEIGHT - WALL, centered=False)
                 .translate((SPLIT_X - LAP, WALL, 0)))
    lip_inner = (cq.Workplane("XY").box(LAP + 2, W - 4 * WALL, HEIGHT - 2 * WALL, centered=False)
                 .translate((SPLIT_X - LAP - 1, 2 * WALL, 0)))
    lip = lip_outer.cut(lip_inner)
    back = back.union(lip)
    # bolt holes through front wall + lip: 2 per side wall, 2 in the top
    hx = SPLIT_X - LAP / 2
    holes = []
    for z in (HEIGHT * 0.3, HEIGHT * 0.7):
        holes.append(cq.Workplane("XZ").center(hx, z).circle(HOLE / 2).extrude(-(W + 10)).translate((0, -5, 0)))
    for y in (W * 0.3, W * 0.7):
        holes.append(cq.Workplane("XY").center(hx, y).circle(HOLE / 2).extrude(20).translate((0, 0, HEIGHT - 15)))
    for h in holes:
        front = front.cut(h)
        back = back.cut(h)
    return front, back


if __name__ == "__main__":
    s = shell()
    front, back = halves(s)
    for name, part in [("cover_front", front), ("cover_back", back)]:
        cq.exporters.export(part, os.path.join(OUT, name + ".step"))
        cq.exporters.export(part, os.path.join(OUT, name + ".stl"))
        bb = part.val().BoundingBox()
        v = part.val().Volume() / 1000.0
        print(f"{name}: {bb.xlen:.0f} x {bb.ylen:.0f} x {bb.zlen:.0f} mm, {v:.0f} cm3, "
              f"about {v * 1.27:.0f} g PETG at 100% (walls are solid)")
    asm = cq.Assembly().add(front, name="cover_front", color=cq.Color(0.2, 0.45, 0.9)) \
                       .add(back, name="cover_back", color=cq.Color(0.3, 0.6, 1.0))
    asm.export(os.path.join(OUT, "cover_assembly.step"))
    print(f"outside size {L:.0f} x {W:.0f} x {HEIGHT:.0f} mm")
