import os
import math
import FreeCAD  # type: ignore[import-not-found]
import Part  # type: ignore[import-not-found]
import MeshPart  # type: ignore[import-not-found]

OUT_DIR = "/Users/user/Documents/Projects/arduino/snookeropener/3D_Enclosure/Prototype"
os.makedirs(OUT_DIR, exist_ok=True)


def make_hex_prism(radius_across_flats, height, center=(0, 0, 0), axis=(0, 1, 0)):
    """Create a hexagonal prism for recessed M4 nut pockets."""
    r_corner = (radius_across_flats / 2.0) / math.cos(math.pi / 6.0)
    pts = []
    for i in range(6):
        ang = math.radians(60 * i + 30)
        pts.append(FreeCAD.Vector(r_corner * math.cos(ang), r_corner * math.sin(ang), 0))
    pts.append(pts[0])
    wire = Part.makePolygon(pts)
    face = Part.Face(wire)
    prism = face.extrude(FreeCAD.Vector(0, 0, height))
    if axis == (0, 1, 0):
        prism.rotate(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(1, 0, 0), -90)
    elif axis == (0, -1, 0):
        prism.rotate(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(1, 0, 0), 90)
    prism.translate(FreeCAD.Vector(*center))
    return prism


def build_rebar_clamp_halves():
    """
    Two-piece bolted PETG split clamp collar for ~10mm (9-12mm) ribbed steel rod.
    Matches the updated Blender animation 1:1:
    - Horizontal flat clevis ear extending along +X with a VERTICAL M3 (3.4mm) pivot hole
      so the flat PETG linkage bar rides completely flush on top!
    """
    rod_r = 5.1        # 10.2 mm bore (clamps tightly onto 9-12mm ribbed rebar)
    width = 38.0       # X: -19..19
    height = 24.0      # Z: 0..24
    half_y = 11.0      # thickness of each half
    gap = 1.0          # 2mm total clamping gap between halves
    bolt_x = 13.0      # M4 bolts at X = +/-13mm
    bolt_r = 2.25      # 4.5 mm M4 clearance hole

    rod_cyl = Part.makeCylinder(rod_r, height + 2, FreeCAD.Vector(0, 0, -1))

    # --- BACK HALF (Y: -gap-half_y .. -gap) ---
    back = Part.makeBox(width, half_y, height, FreeCAD.Vector(-width / 2, -gap - half_y, 0))
    back = back.cut(rod_cyl)

    for sx in (-1, 1):
        for bz in (6.5, height - 6.5):
            bolt = Part.makeCylinder(
                bolt_r, 30.0,
                FreeCAD.Vector(sx * bolt_x, -gap - half_y - 2.0, bz),
                FreeCAD.Vector(0, 1, 0)
            )
            back = back.cut(bolt)
            nut = make_hex_prism(7.4, 3.8, center=(sx * bolt_x, -gap - half_y - 0.2, bz), axis=(0, 1, 0))
            back = back.cut(nut)

    # --- FRONT HALF (Y: +gap .. +gap+half_y) + FLAT HORIZONTAL CLEVIS EAR ---
    front = Part.makeBox(width, half_y, height, FreeCAD.Vector(-width / 2, gap, 0))

    # Flat horizontal clevis ear extending in +X (sits centered on Z=height/2, 6mm thick in Z, 14mm wide in Y)
    tab_len = 16.0
    tab_w_y = 14.0
    tab_thk_z = 6.0
    tab_y_center = gap + half_y / 2.0
    tab_z0 = (height - tab_thk_z) / 2.0

    tab_box = Part.makeBox(tab_len, tab_w_y, tab_thk_z, FreeCAD.Vector(width / 2 - 2.0, tab_y_center - tab_w_y / 2.0, tab_z0))
    tab_tip = Part.makeCylinder(
        tab_w_y / 2.0, tab_thk_z,
        FreeCAD.Vector(width / 2 - 2.0 + tab_len, tab_y_center, tab_z0),
        FreeCAD.Vector(0, 0, 1)
    )
    front = front.fuse(tab_box).fuse(tab_tip)

    # Cut rod bore
    front = front.cut(rod_cyl)

    # Cut M4 clamping bolt holes + counterbores on outer +Y face
    for sx in (-1, 1):
        for bz in (6.5, height - 6.5):
            bolt = Part.makeCylinder(
                bolt_r, 30.0,
                FreeCAD.Vector(sx * bolt_x, gap - 2.0, bz),
                FreeCAD.Vector(0, 1, 0)
            )
            front = front.cut(bolt)
            cbore = Part.makeCylinder(
                3.8, 3.5,
                FreeCAD.Vector(sx * bolt_x, gap + half_y - 3.3, bz),
                FreeCAD.Vector(0, 1, 0)
            )
            front = front.cut(cbore)

    # VERTICAL M3 pivot hole (3.4mm) through the horizontal clevis ear tip!
    pivot_x = width / 2 - 2.0 + tab_len
    pivot_hole = Part.makeCylinder(
        1.7, tab_thk_z + 6.0,
        FreeCAD.Vector(pivot_x, tab_y_center, tab_z0 - 3.0),
        FreeCAD.Vector(0, 0, 1)
    )
    front = front.cut(pivot_hole)

    return front, back


def build_linkage_bar(total_span_mm, step_mm=7.0):
    """
    Rigid PETG slotted pull-link bar printed flat on XY plane (Z: 0..5mm).
    - Left end (X = 0..10mm): 10mm slotted hole (3.5mm wide) for M3 pivot on rebar clamp clevis.
    - Full length (X = 18mm to total_span_mm): M3 (3.3mm) holes all the way to the end
      with shallow side trim-guide notches between holes so you can cleanly snip/break off
      any unused tail behind the servo horn (zero cabinet-wall collision!).
    """
    w = 12.0
    r = w / 2.0
    thk = 5.0
    slot_len = 10.0
    slot_r = 1.75

    # Outer capsule from X=0 to X=total_span_mm
    body = Part.makeBox(total_span_mm, w, thk, FreeCAD.Vector(0, -r, 0))
    c_left = Part.makeCylinder(r, thk, FreeCAD.Vector(0, 0, 0))
    c_right = Part.makeCylinder(r, thk, FreeCAD.Vector(total_span_mm, 0, 0))
    bar = body.fuse(c_left).fuse(c_right)

    # Slotted hole at left end (X = 0..slot_len)
    s_box = Part.makeBox(slot_len, slot_r * 2, thk + 2, FreeCAD.Vector(0, -slot_r, -1))
    s_c1 = Part.makeCylinder(slot_r, thk + 2, FreeCAD.Vector(0, 0, -1))
    s_c2 = Part.makeCylinder(slot_r, thk + 2, FreeCAD.Vector(slot_len, 0, -1))
    slot = s_box.fuse(s_c1).fuse(s_c2)
    bar = bar.cut(slot)

    # M3 pivot holes ALL THE WAY from start (18mm) to finish!
    x = 18.0
    while x <= total_span_mm - 1.0:
        h = Part.makeCylinder(1.65, thk + 2, FreeCAD.Vector(x, 0, -1))
        bar = bar.cut(h)
        # Small 1.0mm side cut-guide notch halfway to next hole (makes snipping excess super clean)
        nx = x + step_mm / 2.0
        if nx < total_span_mm - 4.0:
            for sy in (-r, r):
                notch = Part.makeCylinder(0.9, thk + 2, FreeCAD.Vector(nx, sy, -1))
                bar = bar.cut(notch)
        x += step_mm

    return bar


def export_stl(shape, filename):
    out_path = os.path.join(OUT_DIR, filename)
    mesh = MeshPart.meshFromShape(Shape=shape, LinearDeflection=0.1, AngularDeflection=0.25, Relative=False)
    mesh.write(out_path)
    bb = shape.BoundBox
    print(f"Exported {filename}: {mesh.CountFacets} tris | Size: {bb.XLength:.1f} x {bb.YLength:.1f} x {bb.ZLength:.1f} mm | Solid: {shape.isValid()}")


front, back = build_rebar_clamp_halves()
export_stl(front, "Rebar_Clamp_Front_10mm.stl")
export_stl(back, "Rebar_Clamp_Back_10mm.stl")

bar_std = build_linkage_bar(total_span_mm=110.0, step_mm=7.0)
export_stl(bar_std, "Servo_Linkage_Bar_Adjustable_35_105mm.stl")

bar_long = build_linkage_bar(total_span_mm=236.0, step_mm=7.0)
export_stl(bar_long, "Servo_Linkage_Bar_Long_230mm.stl")
