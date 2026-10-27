"""
Whole end-effector with the PUSHER, in world coordinates, for viewing and
for Inventor (pusher_assembly_*.step), plus PNG renders.

World frame: X forward (fork tip direction), Y left, Z up, floor at z = 0,
fork TIP at x = 0. The fork top is at the lift height given below.

Parts: fork_combined (ours), Studica U-channel mast (vendor STEP), Studica
75002 servo + 76148 horn (vendor STEP), servo_bracket (ours), pusher_arm
(ours), plus a flat piece, a storage base and a delivery stand for context.
The servo bracket is shown approximately placed; confirm on the real carriage.
"""
import os, math
import cadquery as cq
import make_servo_bracket as msb
import make_pusher as mp

OUT = os.path.dirname(os.path.abspath(__file__))
VS = os.path.join(OUT, "vendor_step")

R = mp.R                  # shaft -> paddle bottom
SHAFT_BEHIND_TIP = 34.0   # shaft x = -34
SHAFT_ABOVE_FORK = R + 1  # paddle bottom 1 mm above the fork top when hanging down
ARM_PLANE_Y = 0.0         # arm plate on the robot centreline

def fork(fork_top):
    import make_assembly as ma   # reuses its fork builder
    f = ma.make_fork_combined()
    # built: width in X 0..46, tines toward -Y (tip at y=-90), thickness Z 0..5
    return (f.rotate((0, 0, 0), (0, 0, 1), 90)
             .translate((-90 + 0.0, -23.0, 0)).translate((90 - 90, 0, fork_top - 5)))

def servo_group(fork_top, arm_deg):
    """Servo, horn, bracket and pusher arm. arm_deg: 0 = hanging straight down,
    negative = swung back (park = -150), positive = forward (push)."""
    SHAFT_X = -9.9                 # shaft position in the servo's own frame
    servo = cq.importers.importStep(os.path.join(VS, "75002-servo.stp"))
    horn = (cq.importers.importStep(os.path.join(VS, "76148-horn.stp"))
            .rotate((0, 0, 0), (1, 0, 0), 90).translate((SHAFT_X, 0, -25.0)))
    bracket = msb.make()
    arm = mp.make()                # hub at origin, arm down -Y, shaft = +Z
    # servo frame: shaft along +z at (SHAFT_X, 0); horn disc top at z = 5.
    # put the arm on the horn, pointing +x of the servo frame (= world down)
    arm = arm.rotate((0, 0, 0), (0, 0, 1), 90).translate((SHAFT_X, 0, 5.0))

    # servo frame -> world: x_s -> -Z (down), z_s -> +Y... we want the arm
    # plane at y = ARM_PLANE_Y, so shaft points toward -Y (servo body on +Y).
    #   x_s -> (0,0,-1),  z_s -> (0,-1,0),  y_s = z_s x x_s -> (1,0,0)
    def to_world(shape):
        o = cq.Vector(0, 0, 0)
        return (shape.rotate(o, cq.Vector(1, 0, 0), 90)
                     .rotate(o, cq.Vector(0, 1, 0), 90))
    sx, sz = -SHAFT_BEHIND_TIP, fork_top + SHAFT_ABOVE_FORK
    parts = {}
    for name, w in [("servo_75002", servo), ("horn_76148", horn),
                    ("servo_bracket", bracket), ("pusher_arm", arm)]:
        sh = to_world(w.val())
        # shaft point (SHAFT_X,0,0) went to (0, 0, -SHAFT_X)
        sh = sh.translate(cq.Vector(sx, ARM_PLANE_Y + 5.0 + mp.ARM_T / 2 - 2.5, sz + SHAFT_X))
        parts[name] = sh
    # swing the arm about the shaft (world Y axis through the shaft point)
    pivot = cq.Vector(sx, 0, sz)
    parts["pusher_arm"] = parts["pusher_arm"].rotate(pivot, pivot + cq.Vector(0, 1, 0), -arm_deg)
    return parts

def box(x0, y0, z0, dx, dy, dz):
    return cq.Workplane("XY").box(dx, dy, dz, centered=False).translate((x0, y0, z0)).val()

def storage_base(front_x):
    """100 mm cube, 80x80 cavity, front slot 50 wide x 30 tall, 30 mm off the floor."""
    b = cq.Workplane("XY").box(100, 100, 100, centered=False).translate((front_x, -50, 0))
    b = b.cut(cq.Workplane("XY").box(80, 80, 100, centered=False).translate((front_x + 10, -40, 10)))
    b = b.cut(cq.Workplane("XY").box(12, 50, 30, centered=False).translate((front_x - 1, -25, 30)))
    return b.val()

def stand(front_x, h):
    s = cq.Workplane("XY").box(120, 120, h, centered=False).translate((front_x, -60, 0))
    s = s.cut(cq.Workplane("XY").box(95, 95, 40, centered=False).translate((front_x + 12.5, -47.5, h - 40)))
    return s.val()

COL = {"fork": (0.55, 0.55, 0.58), "servo_75002": (0.2, 0.2, 0.22), "horn_76148": (0.05, 0.05, 0.05),
       "servo_bracket": (0.62, 0.62, 0.68), "pusher_arm": (1.0, 0.55, 0.1), "piece": (0.9, 0.2, 0.2),
       "storage_base": (0.3, 0.55, 0.9), "stand": (0.35, 0.75, 0.45), "mast": (0.75, 0.75, 0.78), "support_beam": (0.6, 0.6, 0.66)}

def scene(kind):
    if kind == "pick":            # fork inside the storage base, pusher parked
        ft = 40.0
        parts = {"fork": fork(ft).val()}
        parts.update(servo_group(ft, -150))
        parts["storage_base"] = storage_base(-90)
        parts["piece"] = box(-80, -37.5, ft, 75, 75, 10)
    else:                         # over stand 3 (75 mm), end of the push
        ft = 75 + 6
        parts = {"fork": fork(ft).val()}
        parts.update(servo_group(ft, 22))
        # fork tip ~10 mm past the pocket's near inner wall: the piece tips off
        # as its middle passes the tip and lands inside the 95 mm pocket
        parts["stand"] = stand(-22.5, 75)
        parts["piece"] = box(-2, -37.5, 75 - 40, 75, 75, 10)   # landed on the pocket floor
    uc = cq.importers.importStep(os.path.join(VS, "76014-u-channel.stp")).val()   # 240 long in X
    o = cq.Vector(0, 0, 0)
    # mast: vertical U-channel behind the fork crossbar
    mast = uc.rotate(o, cq.Vector(0, 1, 0), -90)
    bb = mast.BoundingBox()
    parts["mast"] = mast.translate(cq.Vector(-172 - bb.xmin, -bb.center.y, -bb.zmin))
    # support beam: horizontal U-channel from the carriage forward over the
    # fork, carrying the servo bracket (moves up/down with the lift)
    beam = uc
    bb = beam.BoundingBox()
    beam_top_z = parts["servo_bracket"].BoundingBox().zmax
    beam = beam.translate(cq.Vector(-124 - bb.xmin - 110, 28 - bb.ymin, beam_top_z - bb.zmin))
    parts["support_beam"] = beam
    return parts

def render(parts, path, title, views):
    import numpy as np, matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from mpl_toolkits.mplot3d.art3d import Poly3DCollection
    fig = plt.figure(figsize=(6 * len(views), 6))
    light = np.array([0.4, -0.5, 0.8]); light /= np.linalg.norm(light)
    for i, (el, az, lbl) in enumerate(views):
        ax = fig.add_subplot(1, len(views), i + 1, projection="3d")
        for name, sh in parts.items():
            verts, tris = sh.tessellate(0.6, 0.5)
            V = np.array([(v.x, v.y, v.z) for v in verts])
            T = np.array(tris)
            if len(T) == 0: continue
            P = V[T]
            n = np.cross(P[:, 1] - P[:, 0], P[:, 2] - P[:, 0])
            n /= (np.linalg.norm(n, axis=1)[:, None] + 1e-9)
            k = 0.45 + 0.55 * np.abs(n @ light)
            c = np.array(COL.get(name, (0.7, 0.7, 0.7)))
            fc = np.clip(k[:, None] * c, 0, 1)
            pc = Poly3DCollection(P, facecolors=fc, edgecolors="none",
                                  alpha=0.35 if name in ("storage_base", "stand") else 1.0)
            ax.add_collection3d(pc)
        ax.set_xlim(-260, 120); ax.set_ylim(-190, 190); ax.set_zlim(0, 380)
        ax.set_box_aspect((380, 380, 380))
        ax.view_init(el, az); ax.set_axis_off(); ax.set_title(lbl, fontsize=11)
    fig.suptitle(title, fontsize=13)
    fig.tight_layout()
    fig.savefig(path, dpi=100)
    plt.close(fig)

if __name__ == "__main__":
    for kind, title in [("pick", "PICK from storage base - pusher PARKED (arm up and back)"),
                        ("place", "PLACE on stand 3 - end of the slow PUSH, piece dropping in")]:
        parts = scene(kind)
        asm = cq.Assembly()
        for name, sh in parts.items():
            asm.add(cq.Workplane().add(sh), name=name, color=cq.Color(*COL.get(name, (0.7, 0.7, 0.7))))
        asm.export(os.path.join(OUT, f"pusher_assembly_{kind}.step"))
        # interference check of the moving parts against the base / stand
        env = parts.get("storage_base") or parts.get("stand")
        for n in ("pusher_arm", "servo_75002", "servo_bracket", "horn_76148"):
            v = parts[n].intersect(env).Volume()
            print(f"{kind}: {n:14s} x {'base' if kind=='pick' else 'stand'}: {v:8.2f} mm3")
        render(parts, os.path.join(OUT, f"pusher_{kind}.png"), title,
               [(18, -60, "3/4 view"), (0, -90, "side view")])
        print("wrote", kind)
