"""
Panel cover - a separate part that covers only the control panel.

    outside size : 145 x 80 x 55 mm (length x width x height)
    shell        : 4 side walls only - OPEN at the top (buttons stay reachable)
                   and open at the bottom
    screw tabs   : at the bottom edge, pointing INWARD, each with an M3 hole,
                   to screw it down to the main cover / frame

Print standing up (tabs on the bed): no supports. Every size is a parameter:
    py make_panel_cover.py
"""
import os
import cadquery as cq

OUT = os.path.dirname(os.path.abspath(__file__))

L, W, H = 145.0, 80.0, 55.0     # outside size (mm)
WALL = 1.6                      # wall / top thickness
TAB = 12.0                      # how far each screw tab reaches inward
TAB_T = 3.0                     # tab thickness
TAB_W = 14.0                    # tab width along the wall
HOLE = 3.4                      # M3 clearance
# tab positions: two on each long wall, one on each short wall
TABS = [("long", 0.22), ("long", 0.78), ("short", 0.5)]
# Cable notch: open from the BOTTOM edge of one wall, so the bundle can leave
# sideways. Set CABLE_WALL = None for no notch.
CABLE_WALL = None               # "back" / "front" (long walls) or "left" / "right" (short)  <-- CHECK
CABLE_W, CABLE_H = 25.0, 15.0   # notch width and height (mm)


def make():
    s = cq.Workplane("XY").box(L, W, H, centered=(True, True, False))
    # walls only: open at the TOP (buttons reachable) and at the bottom
    s = s.cut(cq.Workplane("XY").box(L - 2 * WALL, W - 2 * WALL, H + 2,
                                     centered=(True, True, False)).translate((0, 0, -1)))
    for side, f in TABS:
        if side == "long":
            x = -L / 2 + f * L
            for sy in (1, -1):
                y = sy * (W / 2 - WALL - TAB / 2)
                tab = cq.Workplane("XY").box(TAB_W, TAB, TAB_T, centered=(True, True, False)).translate((x, y, 0))
                tab = tab.cut(cq.Workplane("XY").circle(HOLE / 2).extrude(TAB_T + 2).translate((x, y, -1)))
                s = s.union(tab)
        else:
            for sx in (1, -1):
                x = sx * (L / 2 - WALL - TAB / 2)
                tab = cq.Workplane("XY").box(TAB, TAB_W, TAB_T, centered=(True, True, False)).translate((x, 0, 0))
                tab = tab.cut(cq.Workplane("XY").circle(HOLE / 2).extrude(TAB_T + 2).translate((x, 0, -1)))
                s = s.union(tab)
    if CABLE_WALL:
        if CABLE_WALL in ("back", "front"):
            y = (W / 2 - WALL / 2) * (1 if CABLE_WALL == "back" else -1)
            notch = cq.Workplane("XY").box(CABLE_W, WALL + 2, CABLE_H + 1, centered=(True, True, False)).translate((0, y, -1))
        else:
            x = (L / 2 - WALL / 2) * (1 if CABLE_WALL == "right" else -1)
            notch = cq.Workplane("XY").box(WALL + 2, CABLE_W, CABLE_H + 1, centered=(True, True, False)).translate((x, 0, -1))
        s = s.cut(notch)
    return s


if __name__ == "__main__":
    part = make()
    cq.exporters.export(part, os.path.join(OUT, "panel_cover.step"))
    cq.exporters.export(part, os.path.join(OUT, "panel_cover.stl"))
    bb = part.val().BoundingBox()
    v = part.val().Volume() / 1000
    print(f"panel_cover: {bb.xlen:.0f} x {bb.ylen:.0f} x {bb.zlen:.0f} mm, {v:.0f} cm3, about {v*1.27:.0f} g PETG")
