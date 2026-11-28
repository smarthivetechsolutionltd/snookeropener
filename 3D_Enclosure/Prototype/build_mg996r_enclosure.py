import os
import FreeCAD  # type: ignore[import-not-found]
import Part  # type: ignore[import-not-found]
import MeshPart  # type: ignore[import-not-found]

OUT_DIR = "/Users/user/Documents/Projects/arduino/snookeropener/3D_Enclosure/Prototype"
os.makedirs(OUT_DIR, exist_ok=True)


def build_mg996r_enclosure():
    """
    Compact, Low-Filament, Rib-Reinforced MG996R Servo + ESP32 Controller Enclosure.
    Cuts ~60% filament compared to the bulky solid-block version while transferring
    MG996R pull torque directly from the servo sleeve -> gussets -> top cabinet ears.

    Coordinate system (when mounted upright against the cabinet wall):
      X: Width  = 74 mm (0..74)   [MG996R pocket centered top, ESP32 DevKit below]
      Y: Depth  = 32 mm (0..32)   [Back wall at Y=0, Open front + slim lid at Y=32]
      Z: Height = 70 mm (0..70)   [Bottom at Z=0, Top deck with drop-in MG996R pocket at Z=70]

    Exact MG996R Datasheet Dimensions:
      - Body: 40.3 x 20.0 mm   -> Pocket = 41.0 x 20.6 mm (+0.6-0.7mm PETG clearance)
      - Flange overall: 53.6mm -> Recessed flange seat = 54.4 x 21.2 mm
      - Depth below flange: 26.6mm -> Pocket depth = 28.0 mm (+1.4mm bottom clearance)
      - 4x M3 Mounting holes: 48.5 mm x 10.0 mm center-to-center
    """
    W, D, H = 74.0, 34.0, 70.0
    WALL = 2.2
    BACK_WALL = 2.6

    # 1. Compact outer shell
    base = Part.makeBox(W, D, H, FreeCAD.Vector(0, 0, 0))

    # 2. Hollow electronics cavity (open toward +Y front face)
    cavity = Part.makeBox(
        W - 2 * WALL,
        D - BACK_WALL + 2.0,
        H - 2 * WALL,
        FreeCAD.Vector(WALL, BACK_WALL, WALL)
    )
    base = base.cut(cavity)

    # 3. Thin-Walled, Rib-Reinforced MG996R Servo Pocket (NOT a heavy solid brick!)
    # Centered at X = 35.0, Y = 14.8 (sits close to back wall Y=0 for minimal lever arm!)
    sx, sy = 35.0, 14.8
    pocket_x = 41.0
    pocket_y = 20.6
    pocket_depth = 28.0
    flange_x = 54.4
    flange_y = 21.2
    flange_seat_depth = 2.6
    sleeve_t = 2.6

    # 3A. Thin-walled outer sleeve wrapping the MG996R body (only 2.6mm thick!)
    sleeve_w = pocket_x + 2 * sleeve_t   # 46.2 mm
    sleeve_d = pocket_y + 2 * sleeve_t   # 25.8 mm
    sleeve_h = pocket_depth + flange_seat_depth + 2.2  # 32.8 mm
    servo_sleeve = Part.makeBox(
        sleeve_w, sleeve_d, sleeve_h,
        FreeCAD.Vector(sx - sleeve_w / 2.0, sy - sleeve_d / 2.0, H - sleeve_h)
    )
    base = base.fuse(servo_sleeve)

    # 3B. Targeted M3 Flange Screw Bosses (11mm deep solid meat ONLY under the 2 servo mounting ears!)
    boss_ear_w = 10.0
    boss_ear_d = 21.2 + 2 * sleeve_t  # 26.4 mm
    boss_ear_h = 12.0
    for side in (-1, 1):
        bx = sx + side * (pocket_x / 2.0 + boss_ear_w / 2.0 - 0.5) - boss_ear_w / 2.0
        ear_boss = Part.makeBox(
            boss_ear_w, boss_ear_d, boss_ear_h,
            FreeCAD.Vector(bx, sy - boss_ear_d / 2.0, H - boss_ear_h)
        )
        base = base.fuse(ear_boss)

    # 3C. 2x Structural Load-Transfer Gusset Ribs tying the servo sleeve & flange bosses to back wall
    for side in (-1, 1):
        gx = sx + side * (pocket_x / 2.0 + 3.0) - 1.3
        gusset = Part.makeBox(
            2.6, sy - pocket_y / 2.0 + 4.0, 18.0,
            FreeCAD.Vector(gx, BACK_WALL - 0.5, H - 28.0)
        )
        base = base.fuse(gusset)

    # 3D. Cut recessed mounting flange seat at top (Z = H - 2.6 .. H)
    flange_cut = Part.makeBox(
        flange_x, flange_y, flange_seat_depth + 2.0,
        FreeCAD.Vector(sx - flange_x / 2.0, sy - flange_y / 2.0, H - flange_seat_depth)
    )
    base = base.cut(flange_cut)

    # 3E. Cut vertical MG996R body drop-in pocket
    body_cut = Part.makeBox(
        pocket_x, pocket_y, pocket_depth + flange_seat_depth + 1.0,
        FreeCAD.Vector(sx - pocket_x / 2.0, sy - pocket_y / 2.0, H - flange_seat_depth - pocket_depth)
    )
    base = base.cut(body_cut)

    # 3F. Open-front servo wire exit relief slot (lets pre-soldered MG996R wires drop straight in!)
    wire_slot = Part.makeBox(
        10.0, D - (sy - 4.0) + 2.0, 9.0,
        FreeCAD.Vector(sx + pocket_x / 2.0 - 2.0, sy - 4.0, H - flange_seat_depth - pocket_depth + 1.0)
    )
    base = base.cut(wire_slot)

    # 3G. Cut 4 x M3 Servo Mounting Screw Pilot Holes (48.5mm x 10.0mm spacing, 10mm deep)
    for dx in (-48.5 / 2.0, 48.5 / 2.0):
        for dy in (-10.0 / 2.0, 10.0 / 2.0):
            shole = Part.makeCylinder(
                1.5, 12.0,
                FreeCAD.Vector(sx + dx, sy + dy, H - flange_seat_depth - 10.0),
                FreeCAD.Vector(0, 0, 1)
            )
            base = base.cut(shole)

    # 4. 4 x Compact Front Lid Corner Bosses (M3 screws at Y = D)
    boss_r = 3.5
    lid_screw_coords = [
        (WALL + boss_r, WALL + boss_r),
        (W - WALL - boss_r, WALL + boss_r),
        (W - WALL - boss_r, H - WALL - boss_r),
        (WALL + boss_r, H - WALL - boss_r),
    ]
    for lx, lz in lid_screw_coords:
        b_cyl = Part.makeCylinder(
            boss_r, 10.0,
            FreeCAD.Vector(lx, D - 10.0, lz),
            FreeCAD.Vector(0, 1, 0)
        )
        base = base.fuse(b_cyl)
        b_hole = Part.makeCylinder(
            1.45, 9.0,
            FreeCAD.Vector(lx, D - 8.5, lz),
            FreeCAD.Vector(0, 1, 0)
        )
        base = base.cut(b_hole)

    # 5. 4 x Compact External Cabinet Mounting Ears (Top pair right beside MG996R load zone!)
    ear_w = 11.0
    ear_h = 13.0
    ear_t = 4.0
    for ex in (-ear_w, W):
        for ez in (6.0, H - 5.0 - ear_h):
            ear = Part.makeBox(ear_w, ear_t, ear_h, FreeCAD.Vector(ex, 0, ez))
            base = base.fuse(ear)
            hx = ex + ear_w / 2.0
            hz = ez + ear_h / 2.0
            wh = Part.makeCylinder(2.1, ear_t + 4.0, FreeCAD.Vector(hx, -2.0, hz), FreeCAD.Vector(0, 1, 0))
            base = base.cut(wh)

    # 6. Open-Front Drop-In Cable U-Slots (no desoldering needed! Wires drop straight in from +Y front
    #    and the front lid closes completely flat over the slot!)
    # 6A. Bottom open-front U-slot (X = 42..62mm, Y = 12..D+2mm) for RFID harness + 5V power
    cable_exit_bottom = Part.makeBox(20.0, D - 10.0, 6.0, FreeCAD.Vector(42.0, 12.0, -1.0))
    base = base.cut(cable_exit_bottom)
    # 6B. Left-wall open-front U-slot (Z = 22..38mm, between the two left mounting ears) for side cable entry
    cable_exit_left = Part.makeBox(WALL + 4.0, D - 14.0, 16.0, FreeCAD.Vector(-2.0, 16.0, 22.0))
    base = base.cut(cable_exit_left)

    # 7. INTEGRATED ESP32 DEVKIT MOUNTING CRADLE (Lower Chamber: Z=4..36)
    # Fits 30-pin (51.5 x 28mm) and 38-pin (55 x 28mm) ESP32 DevKit boards!
    esp_cx = 40.0       # Positioned so right edge USB aligns cleanly with right wall X=74
    esp_cz = 19.5       # Safely below MG996R pocket (which ends at Z=37.2)
    esp_standoff_h = 5.5
    esp_pcb_l = 55.6
    esp_pcb_w = 28.6

    # Back-wall weight-relief & cooling cutout centered behind ESP32 (keeps upper servo zone 100% solid)
    back_vent = Part.makeBox(
        34.0, BACK_WALL + 4.0, 14.0,
        FreeCAD.Vector(esp_cx - 17.0, -2.0, esp_cz - 7.0)
    )
    base = base.cut(back_vent)

    # 4 x M2.5 Standoff Pillars (standard 49.0 x 23.0 mm ESP32 hole pattern)
    for dx in (-49.0 / 2.0, 49.0 / 2.0):
        for dz in (-23.0 / 2.0, 23.0 / 2.0):
            pillar = Part.makeCylinder(
                2.8, esp_standoff_h,
                FreeCAD.Vector(esp_cx + dx, BACK_WALL, esp_cz + dz),
                FreeCAD.Vector(0, 1, 0)
            )
            base = base.fuse(pillar)
            phole = Part.makeCylinder(
                1.2, esp_standoff_h + 1.0,
                FreeCAD.Vector(esp_cx + dx, BACK_WALL + 0.5, esp_cz + dz),
                FreeCAD.Vector(0, 1, 0)
            )
            base = base.cut(phole)

    # Slim Top & Bottom Guide Rails along ESP32 PCB edges
    rail_len = 32.0
    rail_depth = esp_standoff_h + 3.0
    for dz_edge in (-(esp_pcb_w / 2.0 + 1.8), (esp_pcb_w / 2.0)):
        rail = Part.makeBox(
            rail_len, rail_depth, 1.8,
            FreeCAD.Vector(esp_cx - rail_len / 2.0, BACK_WALL, esp_cz + dz_edge)
        )
        base = base.fuse(rail)

    # Left End-Stop Rib
    end_stop = Part.makeBox(
        2.0, rail_depth, 14.0,
        FreeCAD.Vector(esp_cx - esp_pcb_l / 2.0 - 2.0, BACK_WALL, esp_cz - 7.0)
    )
    base = base.fuse(end_stop)

    # Right-Wall USB-C / Micro-USB Access Cutout
    usb_cut = Part.makeBox(
        WALL + 4.0, 10.5, 14.0,
        FreeCAD.Vector(W - WALL - 2.0, BACK_WALL + esp_standoff_h - 1.0, esp_cz - 7.0)
    )
    base = base.cut(usb_cut)

    # 8. Slim Ventilated Front Lid (2.0mm thick, exported flat on XY)
    lid_t = 2.0
    lid = Part.makeBox(W, H, lid_t, FreeCAD.Vector(0, 0, 0))
    lip = Part.makeBox(
        W - 2 * WALL - 0.7,
        H - 2 * WALL - 0.7,
        1.6,
        FreeCAD.Vector(WALL + 0.35, WALL + 0.35, lid_t)
    )
    lip_cut = Part.makeBox(
        W - 2 * WALL - 4.3,
        H - 2 * WALL - 4.3,
        2.6,
        FreeCAD.Vector(WALL + 2.15, WALL + 2.15, lid_t - 0.2)
    )
    lid = lid.fuse(lip).cut(lip_cut)

    # 4 x M3 Countersunk Lid Screw Holes
    for lx, lz in lid_screw_coords:
        lh = Part.makeCylinder(1.65, lid_t + 4.0, FreeCAD.Vector(lx, lz, -1.0))
        lid = lid.cut(lh)
        lcs = Part.makeCone(3.1, 1.65, 1.4, FreeCAD.Vector(lx, lz, 0.0))
        lid = lid.cut(lcs)

    # 4 x Ventilation Louver Slots over lower ESP32 chamber
    for i in range(4):
        slot = Part.makeBox(36.0, 2.4, lid_t + 4.0, FreeCAD.Vector((W - 36.0) / 2.0, 12.0 + i * 5.2, -1.0))
        lid = lid.cut(slot)

    return base, lid


def export_stl(shape, filename):
    out_path = os.path.join(OUT_DIR, filename)
    mesh = MeshPart.meshFromShape(Shape=shape, LinearDeflection=0.1, AngularDeflection=0.25, Relative=False)
    mesh.write(out_path)
    bb = shape.BoundBox
    vol_cm3 = shape.Volume / 1000.0
    print(f"Exported {filename}: {mesh.CountFacets} tris | Size: {bb.XLength:.1f} x {bb.YLength:.1f} x {bb.ZLength:.1f} mm | Vol: {vol_cm3:.2f} cm3 (~{vol_cm3*1.27:.1f}g solid PETG) | Solid: {shape.isValid()}")


base, lid = build_mg996r_enclosure()
export_stl(base, "Controller_Box_MG996R_Pocket_Base.stl")
export_stl(lid, "Controller_Box_MG996R_Pocket_Lid.stl")
