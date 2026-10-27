"""
Battery case for the Studica 12V 3000 mAh NiMH pack (PP45), to sit on a
Studica U-channel. Open at the TOP only; closed bottom and four walls.

Battery size from Studica's product page: 115 x 45 x 50 mm (5 x 2.1 x 2.1 in;
the inch figures say up to 127 mm long, so MEASURE yours and set BAT_* below).

    bottom  : screws onto the channel - M3 holes on the Studica 16 x 16 mm
              square at every 48 mm cell, so it lines up with the channel's holes
    one end : a slot for the power cable + connector
    sides   : two strap slots so a velcro strap can hold the battery down

Print bottom on the bed, no supports.      py make_battery_case.py
"""
import os
import cadquery as cq

OUT = os.path.dirname(os.path.abspath(__file__))

BAT_L, BAT_W, BAT_H = 115.0, 45.0, 50.0   # battery (MEASURE yours)          <-- CHECK
CLEAR = 2.0                               # room around the battery, each side
WALL = 2.0
FLOOR = 3.0
WALL_H = 35.0                             # wall height above the floor (battery sticks out on top)
CABLE_W, CABLE_H = 22.0, 18.0             # cable slot in one end wall, from the top edge
STRAP_W, STRAP_H = 22.0, 4.0              # velcro strap slots in the floor, across the case
HOLE = 3.6                                # M3
CELL = 48.0                               # Studica channel hole cell pitch
SQ = 16.0                                 # 16 x 16 mm hole square in each cell

IL, IW = BAT_L + 2 * CLEAR, BAT_W + 2 * CLEAR       # inside
OL, OW = IL + 2 * WALL, IW + 2 * WALL               # outside
OH = FLOOR + WALL_H


def make():
    s = cq.Workplane("XY").box(OL, OW, OH, centered=(True, True, False))
    s = s.cut(cq.Workplane("XY").box(IL, IW, OH, centered=(True, True, False)).translate((0, 0, FLOOR)))
    # cable slot in the +x end wall, open from the top
    s = s.cut(cq.Workplane("XY").box(WALL + 2, CABLE_W, CABLE_H + 1, centered=(True, True, False))
              .translate((OL / 2 - WALL / 2, 0, OH - CABLE_H)))
    # strap slots through the floor (strap goes under the case's floor, over the battery)
    for x in (-IL / 4, IL / 4):
        for y in (IW / 2 - STRAP_H / 2 - 1, -(IW / 2 - STRAP_H / 2 - 1)):
            s = s.cut(cq.Workplane("XY").box(STRAP_W, STRAP_H, FLOOR + 2, centered=(True, True, False))
                      .translate((x, y, -1)))
    # channel mounting holes: 16 x 16 square at cells -48, 0, +48 mm
    for cx in (-CELL, 0.0, CELL):
        if abs(cx) + SQ / 2 + HOLE > IL / 2:
            continue
        for dx in (-SQ / 2, SQ / 2):
            for dy in (-SQ / 2, SQ / 2):
                s = s.cut(cq.Workplane("XY").circle(HOLE / 2).extrude(FLOOR + 2)
                          .translate((cx + dx, dy, -1)))
    return s


if __name__ == "__main__":
    part = make()
    cq.exporters.export(part, os.path.join(OUT, "battery_case.step"))
    cq.exporters.export(part, os.path.join(OUT, "battery_case.stl"))
    bb = part.val().BoundingBox()
    v = part.val().Volume() / 1000
    print(f"battery_case: {bb.xlen:.0f} x {bb.ylen:.0f} x {bb.zlen:.0f} mm, {v:.0f} cm3, about {v*1.27:.0f} g PETG")
