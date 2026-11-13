import bpy
import bmesh
import math
from mathutils import Vector, Euler
import os
import sys

# Output Directory for Version 1 (v1) PCB-Matched 3D Enclosure
OUTPUT_DIR = os.path.dirname(os.path.abspath(__file__))
os.makedirs(OUTPUT_DIR, exist_ok=True)
LOGO_PATH = os.path.join(OUTPUT_DIR, "smarthivetech_logo.png")

# -------------------------------------------------------------
# Clean Scene
# -------------------------------------------------------------
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
for block in list(bpy.data.materials):
    bpy.data.materials.remove(block)
for block in list(bpy.data.meshes):
    bpy.data.meshes.remove(block)
for block in list(bpy.data.curves):
    bpy.data.curves.remove(block)
for block in list(bpy.data.cameras):
    bpy.data.cameras.remove(block)
for block in list(bpy.data.lights):
    bpy.data.lights.remove(block)

# Units configuration: Metric Millimeters (1 Blender Unit = 1 mm)
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.length_unit = 'MILLIMETERS'
bpy.context.scene.unit_settings.scale_length = 0.001

# -------------------------------------------------------------
# Material Helpers
# -------------------------------------------------------------
def create_mat(name, color, roughness=0.35, metallic=0.0, emission=None):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs['Base Color'].default_value = color
        bsdf.inputs['Roughness'].default_value = roughness
        bsdf.inputs['Metallic'].default_value = metallic
    if emission:
        emit = mat.node_tree.nodes.new(type='ShaderNodeEmission')
        emit.inputs['Color'].default_value = emission[0]
        emit.inputs['Strength'].default_value = emission[1]
        output = mat.node_tree.nodes.get("Material Output")
        mat.node_tree.links.new(emit.outputs['Emission'], output.inputs['Surface'])
    return mat

mat_ctrl_base    = create_mat("Mat_Ctrl_Base", (0.07, 0.09, 0.12, 1.0), roughness=0.45)
mat_ctrl_lid     = create_mat("Mat_Ctrl_Lid", (0.02, 0.40, 0.48, 1.0), roughness=0.32)
mat_rfid_base    = create_mat("Mat_RFID_Base", (0.06, 0.07, 0.09, 1.0), roughness=0.40)
mat_rfid_face    = create_mat("Mat_RFID_Face", (0.02, 0.45, 0.52, 1.0), roughness=0.28)
mat_pcb_green    = create_mat("Mat_PCB_Green", (0.03, 0.28, 0.14, 1.0), roughness=0.22)
mat_pcb_blue     = create_mat("Mat_PCB_Blue", (0.02, 0.15, 0.55, 1.0), roughness=0.25)
mat_metal_silver = create_mat("Mat_Metal_Silver", (0.85, 0.86, 0.88, 1.0), roughness=0.2, metallic=0.92)
mat_metal_gold   = create_mat("Mat_Metal_Gold", (0.85, 0.70, 0.22, 1.0), roughness=0.25, metallic=0.90)
mat_servo_blue   = create_mat("Mat_Servo_Blue", (0.05, 0.30, 0.88, 1.0), roughness=0.2)
mat_servo_white  = create_mat("Mat_Servo_White", (0.95, 0.95, 0.95, 1.0), roughness=0.3)
mat_switch_black = create_mat("Mat_Switch_Black", (0.04, 0.04, 0.04, 1.0), roughness=0.35)
mat_term_green   = create_mat("Mat_Term_Green", (0.05, 0.55, 0.22, 1.0), roughness=0.35)
mat_switch_metal = create_mat("Mat_Switch_Metal", (0.75, 0.78, 0.82, 1.0), roughness=0.2, metallic=0.85)
mat_ribbon_cable = create_mat("Mat_Ribbon_Cable", (0.2, 0.5, 0.8, 1.0), roughness=0.6)
mat_rgb_led_green= create_mat("Mat_RGB_LED_Green", (0.1, 0.9, 0.3, 1.0), emission=((0.1, 1.0, 0.3, 1.0), 6.0))
mat_pbank_alu    = create_mat("Mat_PBank_Alu", (0.12, 0.13, 0.16, 1.0), roughness=0.25, metallic=0.7)
mat_floor        = create_mat("Mat_Studio_Floor", (0.12, 0.13, 0.15, 1.0), roughness=0.85)

# -------------------------------------------------------------
# Primitive & Boolean Helpers
# -------------------------------------------------------------
def add_box(name, size, loc, mat=None, rot=(0,0,0)):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0,0,0))
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = size
    obj.location = (loc[0], loc[1], loc[2] + size[2] / 2.0)
    obj.rotation_euler = rot
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    if mat:
        obj.data.materials.append(mat)
    return obj

def add_cylinder(name, radius, depth, loc, rot=(0,0,0), mat=None, vertices=36):
    bpy.ops.mesh.primitive_cylinder_add(radius=radius, depth=depth, vertices=vertices, location=(0,0,0))
    obj = bpy.context.active_object
    obj.name = name
    obj.location = (loc[0], loc[1], loc[2] + depth / 2.0)
    obj.rotation_euler = rot
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    if mat:
        obj.data.materials.append(mat)
    return obj

def apply_boolean(target, tool_obj, op='DIFFERENCE'):
    mod = target.modifiers.new(name=f"Bool_{op}", type='BOOLEAN')
    mod.operation = op
    mod.object = tool_obj
    mod.solver = 'EXACT'
    bpy.context.view_layer.objects.active = target
    bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.data.objects.remove(tool_obj, do_unlink=True)

def add_text_mesh(name, text, size, extrude, loc, rot=(0,0,0), mat=None):
    bpy.ops.object.text_add(location=loc)
    obj = bpy.context.active_object
    obj.name = name
    obj.data.body = text
    obj.data.size = size
    obj.data.extrude = extrude / 2.0
    obj.data.align_x = 'CENTER'
    obj.data.align_y = 'CENTER'
    obj.rotation_euler = rot
    bpy.ops.object.convert(target='MESH')
    if mat:
        obj.data.materials.append(mat)
    return obj

# -------------------------------------------------------------
# Logo Extraction & 3D Vectorization Helper
# -------------------------------------------------------------
print("Analyzing and vectorizing Smarthivetech Logo...")
img = bpy.data.images.load(LOGO_PATH)
w, h = img.size[0], img.size[1]
raw_pixels = list(img.pixels)

# Mask out the 0.210 * rh center halo in logo_grid so the original S-swooshes
# wrap cleanly around the bold center 8-Ball without touching it
logo_grid = [[0]*w for _ in range(h)]
for y in range(h):
    for x in range(w):
        if raw_pixels[(y*w + x)*4 + 3] > 0.3:
            if math.hypot(x - 781.0, y - 781.0) >= 0.210 * 786.0:
                logo_grid[y][x] = -1

comp_counter = 1
for y in range(h):
    for x in range(w):
        if logo_grid[y][x] == -1:
            q = [(x, y)]
            logo_grid[y][x] = comp_counter
            while q:
                cx, cy = q.pop(0)
                for dx, dy in [(-1,0), (1,0), (0,-1), (0,1)]:
                    nx, ny = cx + dx, cy + dy
                    if 0 <= nx < w and 0 <= ny < h and logo_grid[ny][nx] == -1:
                        logo_grid[ny][nx] = comp_counter
                        q.append((nx, ny))
            comp_counter += 1

dirs_8 = [(1,0), (1,-1), (0,-1), (-1,-1), (-1,0), (-1,1), (0,1), (1,1)]
def trace_logo_contour(target_id):
    start = None
    for y in range(h-1, -1, -1):
        for x in range(w):
            if logo_grid[y][x] == target_id:
                start = (x, y)
                break
        if start: break
    if not start: return []

    loop = [start]
    curr = start
    search_dir = 7
    for _ in range(25000):
        found = False
        for i in range(8):
            d = (search_dir + i) % 8
            nx = curr[0] + dirs_8[d][0]
            ny = curr[1] + dirs_8[d][1]
            if 0 <= nx < w and 0 <= ny < h and logo_grid[ny][nx] == target_id:
                curr = (nx, ny)
                search_dir = (d + 5) % 8
                if curr == start:
                    return loop
                loop.append(curr)
                found = True
                break
        if not found:
            break
    return loop

c_top = trace_logo_contour(1)[::10]
c_bot = trace_logo_contour(2)[::10]

all_pts = c_top + c_bot
logo_min_x = min(p[0] for p in all_pts)
logo_max_x = max(p[0] for p in all_pts)
logo_min_y = min(p[1] for p in all_pts)
logo_max_y = max(p[1] for p in all_pts)
logo_cx = (logo_min_x + logo_max_x) / 2.0
logo_cy = (logo_min_y + logo_max_y) / 2.0
logo_raw_w = logo_max_x - logo_min_x
logo_raw_h = logo_max_y - logo_min_y

def create_logo_mesh(name, target_height, loc, extrude=0.6, rot=(0, 0, 0)):
    scale = target_height / logo_raw_h
    curve_data = bpy.data.curves.new(f"{name}_Curve", type='CURVE')
    curve_data.dimensions = '2D'
    curve_data.fill_mode = 'BOTH'
    curve_data.extrude = extrude / 2.0

    # 1. Original Smarthive Dual S-Swoosh Contours (with center halo already contoured)
    for pts in [c_top, c_bot]:
        spline = curve_data.splines.new('POLY')
        spline.points.add(len(pts) - 1)
        for i, (px, py) in enumerate(pts):
            x = (px - logo_cx) * scale
            y = (py - logo_cy) * scale
            spline.points[i].co = (x, y, 0.0, 1.0)
        spline.use_cyclic_u = True

    # 2. Bold 8-Ball Outer Ring (R_ball = 0.185 * target_height, R_badge = 0.112 * target_height)
    n_seg = 48
    for r_val in [0.185 * target_height, 0.112 * target_height]:
        sp = curve_data.splines.new('POLY')
        sp.points.add(n_seg - 1)
        for i in range(n_seg):
            ang = 2.0 * math.pi * i / n_seg
            sp.points[i].co = (r_val * math.cos(ang), r_val * math.sin(ang), 0.0, 1.0)
        sp.use_cyclic_u = True

    obj = bpy.data.objects.new(name, curve_data)
    bpy.context.collection.objects.link(obj)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    obj.rotation_euler = rot
    bpy.ops.object.convert(target='MESH')
    obj.location = loc
    return obj

def engrave_logo_8ball(target_obj, name, target_height, loc, extrude=0.7, rot=(0, 0, 0)):
    logo_cut = create_logo_mesh(name, target_height=target_height, loc=loc, extrude=extrude, rot=rot)
    apply_boolean(target_obj, logo_cut, 'DIFFERENCE')

    eight_cut = add_text_mesh(
        f"{name}_Eight",
        "8",
        size=0.195 * target_height,
        extrude=extrude + 0.1,
        loc=loc,
        rot=rot
    )
    apply_boolean(target_obj, eight_cut, 'DIFFERENCE')

# =============================================================
# 1. OUTBOARD RFID + HW-479 RGB + TOP TYPE-C + SIDE SLIDE SWITCH POD
# =============================================================
print("1. Building Professional Outboard RFID Pod...")

RFID_X = 120.0
RFID_Y = 0.0
RFID_Z = 0.0

POD_W = 56.0
POD_L = 112.0
POD_BASE_H = 14.0
FACE_H = 3.0
RC522_CY = -10.0   # RC522 spans Y = -39.95 to +19.95 (leaves 12mm unobstructed bottom screw bay!)
HW479_CY = 30.0    # HW-479 spans Y = +22.25 to +37.75 (leaves unobstructed top screw bay at Y = +43.5!)
SW_CY = 36.0       # Side slide switch centered at Y = +36.0mm

# 1A. RFID Base Body
rfid_base = add_box("RFID_Pod_Base", (POD_W, POD_L, POD_BASE_H), (RFID_X, RFID_Y, RFID_Z), mat_rfid_base)

bev_rfid_base = rfid_base.modifiers.new("Bevel", 'BEVEL')
bev_rfid_base.width = 2.0
bev_rfid_base.segments = 6
bev_rfid_base.limit_method = 'ANGLE'
bev_rfid_base.angle_limit = math.radians(60)
bpy.context.view_layer.objects.active = rfid_base
bpy.ops.object.modifier_apply(modifier="Bevel")

# Main Internal Cavity (42.0mm W x 104.0mm L x 12.0mm deep, floor 2.0mm thick from Z=0 to Z=2.0)
rfid_cavity = add_box("RFID_Cavity", (42.0, 104.0, 13.0), (RFID_X, RFID_Y, RFID_Z + 2.0))
apply_boolean(rfid_base, rfid_cavity, 'DIFFERENCE')

# RC522 Standoffs with Pilot Holes (RC522 center at Y = -10.0mm, clear of bottom mounting screw!)
rc522_hole_offsets = [
    (-12.775, -23.10),
    ( 12.325, -23.10),
    (-17.375,  14.20),
    ( 17.175,  14.30),
]
for i, (ox, oy) in enumerate(rc522_hole_offsets):
    s_col = add_cylinder(f"RFID_Standoff_{i}", 2.8, 3.0, (RFID_X + ox, RFID_Y + RC522_CY + oy, RFID_Z + 2.0), mat=mat_rfid_base)
    apply_boolean(rfid_base, s_col, 'UNION')
    s_hole = add_cylinder(f"RFID_ScrewPilot_{i}", 1.15, 4.5, (RFID_X + ox, RFID_Y + RC522_CY + oy, RFID_Z + 1.2))
    apply_boolean(rfid_base, s_hole, 'DIFFERENCE')

# RECTANGULAR FLOOR PASS-THROUGH OPENING (24.0mm W x 10.0mm L through floor at Y = +18.0mm)
floor_passthrough = add_box("RFID_Floor_PassThrough", (24.0, 10.0, 6.0), (RFID_X, RFID_Y + 18.0, RFID_Z - 2.0))
apply_boolean(rfid_base, floor_passthrough, 'DIFFERENCE')

# HW-479 RGB LED MODULE MOUNTING CRADLE (20.5mm W x 15.5mm L pocket at Y = +30.0mm)
hw479_plat_l = add_box("HW479_Plat_L", (2.5, 15.5, 4.0), (RFID_X - 9.0, RFID_Y + HW479_CY, RFID_Z + 2.0), mat=mat_rfid_base)
hw479_plat_r = add_box("HW479_Plat_R", (2.5, 15.5, 4.0), (RFID_X + 9.0, RFID_Y + HW479_CY, RFID_Z + 2.0), mat=mat_rfid_base)
apply_boolean(rfid_base, hw479_plat_l, 'UNION')
apply_boolean(rfid_base, hw479_plat_r, 'UNION')

# PROFESSIONAL DEDICATED USB TYPE-C FEMALE PORT ON TOP END WALL (Y = +56.0mm, X = RFID_X)
type_c_slot_top = add_box("USB_TypeC_Top_Port", (9.2, 8.0, 3.8), (RFID_X, RFID_Y + 56.0 - 4.0, RFID_Z + 5.6))
apply_boolean(rfid_base, type_c_slot_top, 'DIFFERENCE')

type_c_pocket_top = add_box("USB_TypeC_Top_Pocket", (12.0, 4.5, 8.0), (RFID_X, RFID_Y + 56.0 - 4.5, RFID_Z + 4.5))
apply_boolean(rfid_base, type_c_pocket_top, 'DIFFERENCE')

# FACTORY-STANDARD GUIDED SLIDER BEZEL & SS-12D00 / SS-22D00 CRADLE
switch_bezel_recess = add_box("Switch_Bezel_Recess", (2.4, 13.0, 6.6), (RFID_X + 28.0, RFID_Y + SW_CY, RFID_Z + 4.5))
apply_boolean(rfid_base, switch_bezel_recess, 'DIFFERENCE')

switch_lever_slot = add_box("Switch_Lever_Through_Slot", (6.0, 6.5, 3.2), (RFID_X + 28.0 - 3.0, RFID_Y + SW_CY, RFID_Z + 6.2))
apply_boolean(rfid_base, switch_lever_slot, 'DIFFERENCE')

switch_pocket_side = add_box("Slide_Switch_Side_Pocket", (4.8, 9.6, 6.0), (RFID_X + 28.0 - 4.6, RFID_Y + SW_CY, RFID_Z + 4.8))
apply_boolean(rfid_base, switch_pocket_side, 'DIFFERENCE')

# 2x UNOBSTRUCTED Table Wood Mounting Bosses & Countersunk Holes
# Bottom hole at Y = -46.5mm (6.5mm clear below RC522 PCB!)
# Top hole at Y = +43.5mm (clear between HW-479 LED at Y=+30 and USB-C at Y=+52!)
wood_screw_coords = [(0.0, -46.5), (0.0, 43.5)]
for i, (wx, wy) in enumerate(wood_screw_coords):
    w_boss = add_cylinder(f"RFID_Wood_Boss_{i}", 4.5, 2.2, (RFID_X + wx, RFID_Y + wy, RFID_Z + 2.0), mat=mat_rfid_base)
    apply_boolean(rfid_base, w_boss, 'UNION')
    w_hole = add_cylinder(f"RFID_Wood_Hole_{i}", 1.75, 8.0, (RFID_X + wx, RFID_Y + wy, RFID_Z - 1.0))
    apply_boolean(rfid_base, w_hole, 'DIFFERENCE')
    w_csink = add_cylinder(f"RFID_Wood_Csink_{i}", 3.5, 2.0, (RFID_X + wx, RFID_Y + wy, RFID_Z + 2.5))
    apply_boolean(rfid_base, w_csink, 'DIFFERENCE')

# 4 Corner Fastener Boss Holes for Faceplate (M2.5 screws at X = ±22.0mm, Y = ±50.0mm)
rfid_lid_bosses = [
    (-22.0, -50.0),
    ( 22.0, -50.0),
    (-22.0,  50.0),
    ( 22.0,  50.0),
]
for i, (bx, by) in enumerate(rfid_lid_bosses):
    boss_hole = add_cylinder(f"RFID_Face_BossHole_{i}", 1.15, 8.0, (RFID_X + bx, RFID_Y + by, RFID_Z + POD_BASE_H - 7.5))
    apply_boolean(rfid_base, boss_hole, 'DIFFERENCE')

# 1B. RFID Faceplate
print("Building RFID Faceplate with Logo, RGB Aperture & Dedicated I - O Labels...")
RFID_FACE_ELEV = 28.0

rfid_face = add_box("RFID_Pod_Faceplate", (POD_W, POD_L, FACE_H), (RFID_X, RFID_Y, RFID_FACE_ELEV), mat_rfid_face)

bev_rfid_face = rfid_face.modifiers.new("Bevel", 'BEVEL')
bev_rfid_face.width = 1.4
bev_rfid_face.segments = 5
bev_rfid_face.limit_method = 'ANGLE'
bev_rfid_face.angle_limit = math.radians(60)
bpy.context.view_layer.objects.active = rfid_face
bpy.ops.object.modifier_apply(modifier="Bevel")

rfid_rim = add_box("RFID_Face_Rim", (41.2, 103.2, 1.6), (RFID_X, RFID_Y, RFID_FACE_ELEV - 1.6), mat_rfid_face)
apply_boolean(rfid_face, rfid_rim, 'UNION')

for i, (bx, by) in enumerate(rfid_lid_bosses):
    lh = add_cylinder(f"RFID_Face_ScrewHole_{i}", 1.35, 6.0, (RFID_X + bx, RFID_Y + by, RFID_FACE_ELEV - 2.0))
    apply_boolean(rfid_face, lh, 'DIFFERENCE')
    lcs = add_cylinder(f"RFID_Face_Csink_{i}", 2.7, 1.6, (RFID_X + bx, RFID_Y + by, RFID_FACE_ELEV + FACE_H - 1.5))
    apply_boolean(rfid_face, lcs, 'DIFFERENCE')

rgb_hole = add_cylinder("Faceplate_RGB_Hole", 2.6, 6.0, (RFID_X, RFID_Y + HW479_CY, RFID_FACE_ELEV - 1.0))
apply_boolean(rfid_face, rgb_hole, 'DIFFERENCE')
rgb_bevel = add_cylinder("Faceplate_RGB_Bevel", 3.2, 1.2, (RFID_X, RFID_Y + HW479_CY, RFID_FACE_ELEV + FACE_H - 1.0))
apply_boolean(rfid_face, rgb_bevel, 'DIFFERENCE')

engrave_logo_8ball(
    rfid_face,
    "Logo_RFID_Cut",
    target_height=14.5,
    loc=(RFID_X, RFID_Y + 42.0, RFID_FACE_ELEV + FACE_H),
    extrude=0.7
)

brand_text_rfid = add_text_mesh(
    "RFID_Brand_Text",
    "SMARTHIVE POOL",
    size=2.8,
    extrude=0.7,
    loc=(RFID_X, RFID_Y + 21.5, RFID_FACE_ELEV + FACE_H - 0.25)
)
apply_boolean(rfid_face, brand_text_rfid, 'DIFFERENCE')

label_on = add_text_mesh(
    "Label_Switch_ON",
    "I",
    size=2.4,
    extrude=0.6,
    loc=(RFID_X + 23.5, RFID_Y + SW_CY + 3.5, RFID_FACE_ELEV + FACE_H - 0.25)
)
apply_boolean(rfid_face, label_on, 'DIFFERENCE')

label_dash = add_text_mesh(
    "Label_Switch_Dash",
    "-",
    size=2.4,
    extrude=0.6,
    loc=(RFID_X + 23.5, RFID_Y + SW_CY, RFID_FACE_ELEV + FACE_H - 0.25)
)
apply_boolean(rfid_face, label_dash, 'DIFFERENCE')

label_off = add_text_mesh(
    "Label_Switch_OFF",
    "O",
    size=2.4,
    extrude=0.6,
    loc=(RFID_X + 23.5, RFID_Y + SW_CY - 3.5, RFID_FACE_ELEV + FACE_H - 0.25)
)
apply_boolean(rfid_face, label_off, 'DIFFERENCE')

r_ring1 = add_cylinder("RFID_Graphic_Ring1", 14.0, 0.5, (RFID_X, RFID_Y - 16.0, RFID_FACE_ELEV + FACE_H - 0.35))
r_ring1_c = add_cylinder("RFID_Graphic_Ring1_c", 12.5, 1.2, (RFID_X, RFID_Y - 16.0, RFID_FACE_ELEV + FACE_H - 0.8))
apply_boolean(r_ring1, r_ring1_c, 'DIFFERENCE')
apply_boolean(rfid_face, r_ring1, 'DIFFERENCE')

r_ring2 = add_cylinder("RFID_Graphic_Ring2", 9.5, 0.5, (RFID_X, RFID_Y - 16.0, RFID_FACE_ELEV + FACE_H - 0.35))
r_ring2_c = add_cylinder("RFID_Graphic_Ring2_c", 8.25, 1.2, (RFID_X, RFID_Y - 16.0, RFID_FACE_ELEV + FACE_H - 0.8))
apply_boolean(r_ring2, r_ring2_c, 'DIFFERENCE')
apply_boolean(rfid_face, r_ring2, 'DIFFERENCE')

r_dot = add_cylinder("RFID_Graphic_Dot", 2.3, 0.5, (RFID_X, RFID_Y - 16.0, RFID_FACE_ELEV + FACE_H - 0.35))
apply_boolean(rfid_face, r_dot, 'DIFFERENCE')

card_box = add_box("RFID_Card_Outline", (20.0, 13.0, 0.5), (RFID_X, RFID_Y - 16.0, RFID_FACE_ELEV + FACE_H - 0.35))
card_box_c = add_box("RFID_Card_Outline_c", (18.2, 11.2, 1.2), (RFID_X, RFID_Y - 16.0, RFID_FACE_ELEV + FACE_H - 0.35 - 0.45))
apply_boolean(card_box, card_box_c, 'DIFFERENCE')
apply_boolean(rfid_face, card_box, 'DIFFERENCE')

# =============================================================
# 2. VERSION 1 INBOARD CONTROLLER BOX (FOR L-SHAPED V1 PCB + FEMALE SOCKETS + LAY-FLAT SERVO)
# =============================================================
print("2. Building Version 1 Inboard Controller Box for Contoured L-Shaped PCB & Female Header Sockets...")

# Engineered for the Version 1.0 L-Shaped PCB (75.5mm W x 53.0mm L) + 2x 1x15 Female Pin Sockets + Plug-in NodeMCU v2:
# Outer Box: 94.0mm W x 70.0mm L x 22.0mm Base H (+ 3.5mm Lid = 25.5mm Total H)
# Internal Cavity: 88.0mm W x 64.0mm L x 20.0mm deep (Floor Z = 0..2.0mm)
# Z Stack-Up:
#   - Floor: Z = 0.0 .. 2.0mm
#   - 4x M2.5 PCB Standoffs (H1..H4): Z = 2.0 .. 5.5mm (3.5mm under-PCB solder clearance)
#   - Version 1.0 L-Shaped PCB: Z = 5.5 .. 7.1mm (1.6mm FR-4)
#   - 2x 1x15 2.54mm Female Header Sockets (J1_LEFT / J1_RIGHT): Z = 7.1 .. 15.6mm (8.5mm standard height)
#   - Plug-In NodeMCU v2 ESP8266 PCB: Z = 15.6 .. 17.2mm (ESP-12E shield & Micro-USB up to Z = 20.2mm)
CTRL_W = 94.0
CTRL_L = 70.0
CTRL_BASE_H = 26.5
CTRL_LID_H = 3.5

CTRL_X = 0.0
CTRL_Y = 0.0
CTRL_Z = 0.0

# 2A. Controller Base Body
ctrl_base = add_box("Controller_Base", (CTRL_W, CTRL_L, CTRL_BASE_H), (CTRL_X, CTRL_Y, CTRL_Z), mat_ctrl_base)

bev_ctrl = ctrl_base.modifiers.new("Bevel", 'BEVEL')
bev_ctrl.width = 2.5
bev_ctrl.segments = 5
bev_ctrl.limit_method = 'ANGLE'
bev_ctrl.angle_limit = math.radians(60)
bpy.context.view_layer.objects.active = ctrl_base
bpy.ops.object.modifier_apply(modifier="Bevel")

# Mounting Flanges on Left and Right (12mm W x 28mm L x 3.5mm H)
ear_l = add_box("Ctrl_Ear_L", (12.0, 28.0, 3.5), (CTRL_X - (CTRL_W/2.0 + 6.0), CTRL_Y, CTRL_Z), mat_ctrl_base)
ear_r = add_box("Ctrl_Ear_R", (12.0, 28.0, 3.5), (CTRL_X + (CTRL_W/2.0 + 6.0), CTRL_Y, CTRL_Z), mat_ctrl_base)
apply_boolean(ctrl_base, ear_l, 'UNION')
apply_boolean(ctrl_base, ear_r, 'UNION')

slot_l = add_box("Ctrl_Slot_L", (7.0, 4.5, 8.0), (CTRL_X - (CTRL_W/2.0 + 6.0), CTRL_Y, CTRL_Z - 2.0))
slot_r = add_box("Ctrl_Slot_R", (7.0, 4.5, 8.0), (CTRL_X + (CTRL_W/2.0 + 6.0), CTRL_Y, CTRL_Z - 2.0))
apply_boolean(ctrl_base, slot_l, 'DIFFERENCE')
apply_boolean(ctrl_base, slot_r, 'DIFFERENCE')

# Main Internal Cavity (88.0mm W x 64.0mm L x 24.5mm deep, floor 2.0mm)
cavity_ctrl = add_box("Ctrl_Cavity", (88.0, 64.0, 25.5), (CTRL_X, CTRL_Y, CTRL_Z + 2.0))
apply_boolean(ctrl_base, cavity_ctrl, 'DIFFERENCE')

# 4 Corner M3 Screw Pillars for Lid Attachment (X = ±41.5mm, Y = ±29.5mm — completely outside the 75.5x53mm PCB!)
corner_ctrl_coords = [
    (-41.5, -29.5),
    ( 41.5, -29.5),
    (-41.5,  29.5),
    ( 41.5,  29.5),
]
for i, (cx, cy) in enumerate(corner_ctrl_coords):
    post = add_cylinder(f"Ctrl_Post_{i}", 3.2, 24.5, (CTRL_X + cx, CTRL_Y + cy, CTRL_Z + 2.0), mat=mat_ctrl_base)
    apply_boolean(ctrl_base, post, 'UNION')
    hole = add_cylinder(f"Ctrl_Post_Hole_{i}", 1.45, 18.0, (CTRL_X + cx, CTRL_Y + cy, CTRL_Z + 8.5))
    apply_boolean(ctrl_base, hole, 'DIFFERENCE')

# 4x M2.5 Standoffs Matching Exact KiCad PCB Mounting Holes (H1, H2, H3, H4)
# Mapping from KiCad (X_k, Y_k) -> Blender (X_k - 100.0, 100.0 - Y_k):
#   H1: (65.0, 76.8)  -> (-35.0, +23.2)
#   H2: (134.2, 76.8) -> (+34.2, +23.2)
#   H3: (65.0, 123.2) -> (-35.0, -23.2)
#   H4: (94.2, 123.2) -> ( -5.8, -23.2)
PCB_STANDOFF_H = 3.5
PCB_Z_BOTTOM = CTRL_Z + 2.0 + PCB_STANDOFF_H  # Z = 5.5mm
pcb_mount_holes = [
    (-35.0,  23.2),  # H1 (Top-Left)
    ( 34.2,  23.2),  # H2 (Top-Right)
    (-35.0, -23.2),  # H3 (Bottom-Left)
    ( -5.8, -23.2),  # H4 (Bottom-Center of L-Wing)
]
for i, (hx, hy) in enumerate(pcb_mount_holes, 1):
    standoff = add_cylinder(f"PCB_Standoff_H{i}", 3.0, PCB_STANDOFF_H, (CTRL_X + hx, CTRL_Y + hy, CTRL_Z + 2.0), mat=mat_ctrl_base)
    apply_boolean(ctrl_base, standoff, 'UNION')
    s_hole = add_cylinder(f"PCB_Pilot_H{i}", 1.15, 4.5, (CTRL_X + hx, CTRL_Y + hy, CTRL_Z + 1.2))
    apply_boolean(ctrl_base, s_hole, 'DIFFERENCE')

# Additional PCB Support Ledge under Right Wing (X = +22.0mm, Y = +2.0mm, away from all pins)
pcb_rest_post = add_cylinder("PCB_Support_Post_Right", 2.4, PCB_STANDOFF_H, (CTRL_X + 22.0, CTRL_Y + 2.0, CTRL_Z + 2.0), mat=mat_ctrl_base)
apply_boolean(ctrl_base, pcb_rest_post, 'UNION')

# NodeMCU v2 Center in Blender coordinates: X = -20.0mm, Y = 0.0mm
NMCU_CX = -20.0
NMCU_CY = 0.0

# Micro-USB Access Window on Rear Wall (Aligned with Socketed NodeMCU at Z = 15.6..20.2mm)
usb_port = add_box("Ctrl_USB_Port", (12.0, 8.0, 6.8), (CTRL_X + NMCU_CX, CTRL_Y + 35.0, CTRL_Z + 14.2))
apply_boolean(ctrl_base, usb_port, 'DIFFERENCE')

# TOP/REAR WALL CABLE INPUT PORT FOR RFID, RGB LED & SWITCHED 5V HARNESS
# Aligned directly in front of J5 (X=+1.5), J2 (X=+20.0), and J6 (X=+25.0) at Z = 8.5..18.5mm!
wire_port = add_box("Ctrl_Top_Wire_Port", (26.0, 8.0, 10.0), (CTRL_X + 16.0, CTRL_Y + 35.0, CTRL_Z + 8.5))
apply_boolean(ctrl_base, wire_port, 'DIFFERENCE')

# -------------------------------------------------------------
# ZERO-CLASH UNIVERSAL LAY-FLAT SERVO BAY (SG90 & MG90 / MG90S)
# -------------------------------------------------------------
# Nested inside the bottom-right L-cutout of the Version 1.0 PCB (X = -2.0..+37.5mm, Y = -2.0..-26.5mm)
# Positioned at X = +18.0mm, Y = -18.5mm:
#   - >3.75mm air gap to PCB vertical cutout edge (X = -2.0mm)
#   - >10.2mm air gap to PCB horizontal cutout edge (Y = -2.0mm) for J3 servo plug & cable loop
#   - >8.0mm air gap to bottom-right M3 corner pillar (+41.5, -29.5)
SERVO_FLAT_X = CTRL_X + 18.0
SERVO_FLAT_Y = CTRL_Y - 18.5
SERVO_FLAT_Z = CTRL_Z + 2.0

# Form-fitted recessed floor bed for servo body (23.4mm L x 12.6mm W)
servo_bed_pocket = add_box("Servo_Floor_Bed", (23.4, 12.6, 2.0), (SERVO_FLAT_X, SERVO_FLAT_Y, CTRL_Z + 1.0))
apply_boolean(ctrl_base, servo_bed_pocket, 'DIFFERENCE')

# Front wall pass-through window for horizontal output spline collar (fits SG90 & MG90S metal collar)
servo_gear_window = add_box("Servo_Gear_Window", (15.0, 10.0, 14.0), (SERVO_FLAT_X, CTRL_Y - 35.0, CTRL_Z + 2.0))
apply_boolean(ctrl_base, servo_gear_window, 'DIFFERENCE')

# 180° Free Swing Exterior Relief Recess (Zero wall contact for horn)
servo_swing_recess = add_box("Servo_Exterior_Swing_Recess", (38.0, 12.0, 15.5), (SERVO_FLAT_X, CTRL_Y - 39.0, CTRL_Z + 1.5))
apply_boolean(ctrl_base, servo_swing_recess, 'DIFFERENCE')

# Under-body wire relief channel leading directly toward J3 Servo Header (+14.0, +2.5)
servo_cable_relief = add_box("Servo_Wire_Relief", (10.0, 10.0, 3.5), (SERVO_FLAT_X - 2.0, SERVO_FLAT_Y + 10.0, CTRL_Z + 0.8))
apply_boolean(ctrl_base, servo_cable_relief, 'DIFFERENCE')

# Dedicated Lay-Flat Universal Retention Pillars for SG90 / MG90S (28.0mm Pitch)
servo_clamp_l = add_box("Servo_Clamp_L", (4.5, 8.5, 9.5), (SERVO_FLAT_X - 14.0, SERVO_FLAT_Y, CTRL_Z + 2.0), mat_ctrl_base)
servo_clamp_r = add_box("Servo_Clamp_R", (4.5, 8.5, 9.5), (SERVO_FLAT_X + 14.0, SERVO_FLAT_Y, CTRL_Z + 2.0), mat_ctrl_base)
apply_boolean(ctrl_base, servo_clamp_l, 'UNION')
apply_boolean(ctrl_base, servo_clamp_r, 'UNION')

shole_l = add_cylinder("Servo_Flat_Screw_L", 1.15, 8.0, (SERVO_FLAT_X - 14.0, SERVO_FLAT_Y, CTRL_Z + 4.5))
shole_r = add_cylinder("Servo_Flat_Screw_R", 1.15, 8.0, (SERVO_FLAT_X + 14.0, SERVO_FLAT_Y, CTRL_Z + 4.5))
apply_boolean(ctrl_base, shole_l, 'DIFFERENCE')
apply_boolean(ctrl_base, shole_r, 'DIFFERENCE')

# 2B. Controller Lid with Official Logo & Louvers
print("Building Version 1 Controller Lid...")
CTRL_LID_ELEV = 42.0

ctrl_lid = add_box("Controller_Lid", (CTRL_W, CTRL_L, CTRL_LID_H), (CTRL_X, CTRL_Y, CTRL_LID_ELEV), mat_ctrl_lid)

bev_ctrl_lid = ctrl_lid.modifiers.new("Bevel", 'BEVEL')
bev_ctrl_lid.width = 2.5
bev_ctrl_lid.segments = 5
bev_ctrl_lid.limit_method = 'ANGLE'
bev_ctrl_lid.angle_limit = math.radians(60)
bpy.context.view_layer.objects.active = ctrl_lid
bpy.ops.object.modifier_apply(modifier="Bevel")

# Underside Interlocking Rim (87.2mm x 63.2mm x 1.6mm)
lid_rim = add_box("Ctrl_Lid_Rim", (87.2, 63.2, 1.6), (CTRL_X, CTRL_Y, CTRL_LID_ELEV - 1.6), mat_ctrl_lid)
apply_boolean(ctrl_lid, lid_rim, 'UNION')

# Cut out small corner clearances on the lid rim around the 4 corner M3 pillars
for i, (cx, cy) in enumerate(corner_ctrl_coords):
    rim_clear = add_cylinder(f"Ctrl_Rim_Clear_{i}", 3.6, 3.0, (CTRL_X + cx, CTRL_Y + cy, CTRL_LID_ELEV - 2.0))
    apply_boolean(ctrl_lid, rim_clear, 'DIFFERENCE')
    chole = add_cylinder(f"Ctrl_Lid_Hole_{i}", 1.7, 6.0, (CTRL_X + cx, CTRL_Y + cy, CTRL_LID_ELEV - 2.0))
    apply_boolean(ctrl_lid, chole, 'DIFFERENCE')
    csink = add_cylinder(f"Ctrl_Lid_Csink_{i}", 3.4, 1.8, (CTRL_X + cx, CTRL_Y + cy, CTRL_LID_ELEV + CTRL_LID_H - 1.5))
    apply_boolean(ctrl_lid, csink, 'DIFFERENCE')

engrave_logo_8ball(
    ctrl_lid,
    "Logo_Ctrl_Cut",
    target_height=15.5,
    loc=(CTRL_X, CTRL_Y + 14.5, CTRL_LID_ELEV + CTRL_LID_H),
    extrude=0.7
)

brand_text_ctrl = add_text_mesh(
    "Ctrl_Brand_Text",
    "SMARTHIVE POOL",
    size=3.0,
    extrude=0.7,
    loc=(CTRL_X, CTRL_Y + 3.5, CTRL_LID_ELEV + CTRL_LID_H - 0.25)
)
apply_boolean(ctrl_lid, brand_text_ctrl, 'DIFFERENCE')

for l_idx in range(-2, 3):
    louver = add_box(f"Ctrl_Louver_{l_idx}", (46.0, 2.0, 3.0), (CTRL_X, CTRL_Y - 12.5 + l_idx * 4.4, CTRL_LID_ELEV + 1.8))
    apply_boolean(ctrl_lid, louver, 'DIFFERENCE')

# =============================================================
# 3. WALL-MOUNT POWER BANK HOLDER (FLUSH BACKPLATE & DUAL FLANGES)
# =============================================================
print("3. Building Wall-Mount Power Bank Holder...")

PBANK_W = 78.0
PBANK_D = 22.0
PBANK_H = 75.0
PBANK_CRADLE_X = -115.0
PBANK_CRADLE_Y = 0.0
PBANK_CRADLE_Z = 0.0

CRADLE_OUTER_W = PBANK_W + 7.0   # 85.0mm
CRADLE_OUTER_D = PBANK_D + 7.5   # 29.5mm
BACK_WALL_Y = PBANK_CRADLE_Y + CRADLE_OUTER_D / 2.0  # +14.75mm

pbank_cradle = add_box("Power_Bank_Cradle", (CRADLE_OUTER_W, CRADLE_OUTER_D, PBANK_H), (PBANK_CRADLE_X, PBANK_CRADLE_Y, PBANK_CRADLE_Z), mat_ctrl_base)

bev_pb = pbank_cradle.modifiers.new("Bevel", 'BEVEL')
bev_pb.width = 2.0
bev_pb.segments = 4
bev_pb.limit_method = 'ANGLE'
bev_pb.angle_limit = math.radians(60)
bpy.context.view_layer.objects.active = pbank_cradle
bpy.ops.object.modifier_apply(modifier="Bevel")

FLANGE_W = 15.0
FLANGE_T = 4.5
FLANGE_H = PBANK_H - 6.0
FLANGE_Y = BACK_WALL_Y - FLANGE_T / 2.0

pbank_ear_l = add_box("PBank_Wall_Flange_L", (FLANGE_W + 3.0, FLANGE_T, FLANGE_H), (PBANK_CRADLE_X - (CRADLE_OUTER_W / 2.0 + FLANGE_W / 2.0 - 1.5), FLANGE_Y, PBANK_CRADLE_Z + 3.0), mat_ctrl_base)
pbank_ear_r = add_box("PBank_Wall_Flange_R", (FLANGE_W + 3.0, FLANGE_T, FLANGE_H), (PBANK_CRADLE_X + (CRADLE_OUTER_W / 2.0 + FLANGE_W / 2.0 - 1.5), FLANGE_Y, PBANK_CRADLE_Z + 3.0), mat_ctrl_base)
apply_boolean(pbank_cradle, pbank_ear_l, 'UNION')
apply_boolean(pbank_cradle, pbank_ear_r, 'UNION')

pbank_pocket = add_box("PBank_Cavity", (PBANK_W, PBANK_D, PBANK_H + 5.0), (PBANK_CRADLE_X, PBANK_CRADLE_Y - 0.25, PBANK_CRADLE_Z + 4.0))
apply_boolean(pbank_cradle, pbank_pocket, 'DIFFERENCE')

# Deep front retaining basket (Z = 0.0 to 54.0mm solid wall, Z = 54.0 to 75.0mm upper finger opening)
# Deep 54.0mm front pocket ensures smaller/shorter power banks are securely retained and cannot tip or fall out!
PBANK_WALL_H = 54.0  # 54.0mm tall solid front basket (72% of total 75.0mm height)
FRONT_WALL_Y = PBANK_CRADLE_Y - CRADLE_OUTER_D / 2.0  # -14.75mm

pbank_window = add_box("PBank_LED_Window", (PBANK_W - 16.0, 10.0, PBANK_H - PBANK_WALL_H + 5.0), (PBANK_CRADLE_X, FRONT_WALL_Y, PBANK_CRADLE_Z + PBANK_WALL_H))
apply_boolean(pbank_cradle, pbank_window, 'DIFFERENCE')

# Side Wall Finger Grip Windows (upper section above the retaining basket)
pbank_side_win_l = add_box("PBank_Side_Win_L", (10.0, PBANK_D - 8.0, PBANK_H - PBANK_WALL_H + 5.0), (PBANK_CRADLE_X - CRADLE_OUTER_W / 2.0, PBANK_CRADLE_Y - 0.25, PBANK_CRADLE_Z + PBANK_WALL_H))
pbank_side_win_r = add_box("PBank_Side_Win_R", (10.0, PBANK_D - 8.0, PBANK_H - PBANK_WALL_H + 5.0), (PBANK_CRADLE_X + CRADLE_OUTER_W / 2.0, PBANK_CRADLE_Y - 0.25, PBANK_CRADLE_Z + PBANK_WALL_H))
apply_boolean(pbank_cradle, pbank_side_win_l, 'DIFFERENCE')
apply_boolean(pbank_cradle, pbank_side_win_r, 'DIFFERENCE')

# Boldly engrave "SMARTHIVE POOL" on the front retaining wall without logo
brand_text_pbank = add_text_mesh(
    "PBank_Brand_Text",
    "SMARTHIVE POOL",
    size=7.0,
    extrude=2.0,
    loc=(PBANK_CRADLE_X, FRONT_WALL_Y, PBANK_CRADLE_Z + 27.0),
    rot=(math.radians(90), 0, 0)
)
apply_boolean(pbank_cradle, brand_text_pbank, 'DIFFERENCE')

pbank_bot_slot = add_box("PBank_Bottom_Slot", (34.0, 14.0, 10.0), (PBANK_CRADLE_X, PBANK_CRADLE_Y - 0.25, PBANK_CRADLE_Z - 2.0))
apply_boolean(pbank_cradle, pbank_bot_slot, 'DIFFERENCE')

flange_screw_x = CRADLE_OUTER_W / 2.0 + FLANGE_W / 2.0
for sx_sign in [-1.0, 1.0]:
    for sz in [16.0, 59.0]:
        hole_x = PBANK_CRADLE_X + sx_sign * flange_screw_x
        hole_z = PBANK_CRADLE_Z + sz
        w_shaft = add_cylinder(f"PB_Wall_Screw_{sx_sign}_{sz}", 2.2, 12.0, (hole_x, FLANGE_Y, hole_z - 6.0), rot=(math.radians(90), 0, 0))
        apply_boolean(pbank_cradle, w_shaft, 'DIFFERENCE')
        w_csink = add_cylinder(f"PB_Wall_Csink_{sx_sign}_{sz}", 4.2, 3.0, (hole_x, FLANGE_Y - FLANGE_T / 2.0, hole_z - 1.5), rot=(math.radians(90), 0, 0))
        apply_boolean(pbank_cradle, w_csink, 'DIFFERENCE')

for kx in [-22.0, 22.0]:
    kh_x = PBANK_CRADLE_X + kx
    kh_z = PBANK_CRADLE_Z + 52.0
    kh_slot = add_box(f"PB_Keyhole_Slot_{kx}", (4.4, 10.0, 7.0), (kh_x, BACK_WALL_Y - 2.0, kh_z))
    apply_boolean(pbank_cradle, kh_slot, 'DIFFERENCE')
    kh_bore = add_cylinder(f"PB_Keyhole_Bore_{kx}", 4.0, 10.0, (kh_x, BACK_WALL_Y - 2.0, kh_z - 5.0), rot=(math.radians(90), 0, 0))
    apply_boolean(pbank_cradle, kh_bore, 'DIFFERENCE')
    kh_recess = add_box(f"PB_Keyhole_Recess_{kx}", (8.4, 2.0, 8.0), (kh_x, BACK_WALL_Y - 3.5, kh_z - 1.0))
    apply_boolean(pbank_cradle, kh_recess, 'DIFFERENCE')

# =============================================================
# 4. ACCURATE VERSION 1.0 L-SHAPED PCB, FEMALE SOCKETS & COMPONENT DUMMIES
# =============================================================
print("4. Adding Version 1.0 Contoured L-Shaped PCB, Female Header Sockets & Component Dummies...")

# 4A. Version 1.0 Contoured L-Shaped PCB (75.5mm W x 53.0mm L x 1.6mm thick at Z = 5.5mm)
# Full rectangle X = -38.0..+37.5 (center -0.25, W 75.5), Y = -26.5..+26.5 (center 0.0, L 53.0)
# Minus bottom-right servo bay notch X = -2.0..+38.0, Y = -27.0..-2.0
v1_pcb = add_box("Snooker_Opener_V1_PCB", (75.5, 53.0, 1.6), (CTRL_X - 0.25, CTRL_Y, PCB_Z_BOTTOM), mat_pcb_green)
pcb_servo_notch = add_box("PCB_Servo_Bay_Notch", (41.0, 26.0, 4.0), (CTRL_X + 18.5, CTRL_Y - 15.0, PCB_Z_BOTTOM - 1.0))
apply_boolean(v1_pcb, pcb_servo_notch, 'DIFFERENCE')

for i, (hx, hy) in enumerate(pcb_mount_holes, 1):
    p_hole = add_cylinder(f"PCB_Hole_H{i}", 1.25, 4.0, (CTRL_X + hx, CTRL_Y + hy, PCB_Z_BOTTOM - 1.0))
    apply_boolean(v1_pcb, p_hole, 'DIFFERENCE')
    m_screw = add_cylinder(f"PCB_Screw_H{i}", 2.1, 1.4, (CTRL_X + hx, CTRL_Y + hy, PCB_Z_BOTTOM + 1.6), mat=mat_metal_silver)

PCB_Z_TOP = PCB_Z_BOTTOM + 1.6  # Z = 7.1mm

# 4B. 2x 1x15 2.54mm Female Pin Header Sockets Soldered on PCB (J1_LEFT at X=-31.43, J1_RIGHT at X=-8.57)
SOCKET_H = 8.5
j1_left_socket  = add_box("J1_Left_Female_Socket",  (2.54, 38.5, SOCKET_H), (CTRL_X - 31.43, CTRL_Y, PCB_Z_TOP), mat_switch_black)
j1_right_socket = add_box("J1_Right_Female_Socket", (2.54, 38.5, SOCKET_H), (CTRL_X - 8.57,  CTRL_Y, PCB_Z_TOP), mat_switch_black)

# 4C. NodeMCU v2 ESP8266 Plugged into Female Sockets (No Soldering to ESP!)
NMCU_Z = PCB_Z_TOP + SOCKET_H  # Z = 15.6mm
nodemcu_pcb = add_box("NodeMCU_PCB", (26.0, 49.0, 1.6), (CTRL_X + NMCU_CX, CTRL_Y + NMCU_CY, NMCU_Z), mat_pcb_blue)
esp12_can   = add_box("ESP12_Shield", (16.0, 24.0, 2.8), (CTRL_X + NMCU_CX, CTRL_Y + NMCU_CY - 5.0, NMCU_Z + 1.6), mat_metal_silver)
usb_conn    = add_box("NodeMCU_USB_Conn", (7.5, 6.5, 3.0), (CTRL_X + NMCU_CX, CTRL_Y + NMCU_CY + 24.5, NMCU_Z + 1.6), mat_metal_silver)

# 4D. PCB Headers, Terminal Blocks, Capacitor C1 & Diode D1 on Version 1.0 PCB
j5_term = add_box("J5_USBC_5V_Term", (10.16, 7.5, 10.0), (CTRL_X + 1.5,  CTRL_Y + 22.0, PCB_Z_TOP), mat_term_green)
d1_body = add_cylinder("D1_Schottky", 1.3, 5.0, (CTRL_X + 1.5, CTRL_Y + 16.0, PCB_Z_TOP + 1.5), rot=(0, math.radians(90), 0), mat=mat_switch_black)
j2_hdr  = add_box("J2_RC522_Header", (20.32, 2.54, 8.5), (CTRL_X + 20.0, CTRL_Y + 22.5, PCB_Z_TOP), mat_metal_gold)
j6_hdr  = add_box("J6_HW479_Header", (10.16, 2.54, 8.5), (CTRL_X + 25.0, CTRL_Y + 15.0, PCB_Z_TOP), mat_metal_gold)
c1_cap  = add_cylinder("C1_470uF_Cap", 4.0, 11.0, (CTRL_X + 4.5, CTRL_Y + 6.0, PCB_Z_TOP), mat=mat_switch_black)
j3_hdr  = add_box("J3_Servo_Header", (7.62, 2.54, 8.5), (CTRL_X + 14.0, CTRL_Y + 2.5, PCB_Z_TOP), mat_metal_gold)
j4_term = add_box("J4_Aux_Sw_Term",  (10.16, 7.5, 10.0), (CTRL_X + 31.0, CTRL_Y + 4.5, PCB_Z_TOP), mat_term_green)
sw1_btn = add_box("SW1_Test_Button", (6.0, 6.0, 4.5),    (CTRL_X - 20.0, CTRL_Y - 22.0, PCB_Z_TOP), mat_switch_metal)

# 4E. SG90 / MG90S SERVO LAYING FLAT IN DEDICATED BAY (Nested in PCB Cutout)
servo_flat_body   = add_box("Servo_Flat_Body", (23.0, 22.8, 12.2), (SERVO_FLAT_X, SERVO_FLAT_Y, SERVO_FLAT_Z), mat_servo_blue)
servo_flat_flange = add_box("Servo_Flat_Flange", (32.0, 22.8, 2.4), (SERVO_FLAT_X, SERVO_FLAT_Y, SERVO_FLAT_Z + 6.0), mat_servo_blue)
servo_output_shaft= add_cylinder("Servo_Output_Shaft", 2.4, 7.0, (SERVO_FLAT_X, CTRL_Y - 33.0, SERVO_FLAT_Z + 6.1), rot=(math.radians(90), 0, 0), mat=mat_servo_white)
servo_horn_vert   = add_box("Servo_Vertical_Horn", (4.0, 3.0, 24.0), (SERVO_FLAT_X, CTRL_Y - 37.0, SERVO_FLAT_Z + 6.1), mat_servo_white, rot=(0, math.radians(25), 0))

# 4F. RC522 PCB Dummy inside Outboard Pod (Centered at RC522_CY = -10.0mm, leaving bottom mounting screw 100% unobstructed!)
rc522_pcb = add_box("RC522_PCB", (39.25, 59.9, 1.6), (RFID_X, RFID_Y + RC522_CY, RFID_Z + 5.0), mat_pcb_blue)
for i, (ox, oy) in enumerate(rc522_hole_offsets):
    p_h = add_cylinder(f"RC522_Hole_{i}", 1.65, 3.0, (RFID_X + ox, RFID_Y + RC522_CY + oy, RFID_Z + 4.5))
    apply_boolean(rc522_pcb, p_h, 'DIFFERENCE')
    screw = add_cylinder(f"RC522_Screw_{i}", 2.6, 1.4, (RFID_X + ox, RFID_Y + RC522_CY + oy, RFID_Z + 6.6), mat=mat_metal_silver)

rc522_chip = add_box("RC522_IC_Chip", (5.0, 5.0, 1.2), (RFID_X, RFID_Y + RC522_CY + 8.0, RFID_Z + 6.6), mat_switch_black)
rc522_xtal = add_box("RC522_Crystal", (4.5, 11.0, 3.2), (RFID_X + 14.0, RFID_Y + RC522_CY + 15.0, RFID_Z + 6.6), mat_metal_silver)
rc522_hdr  = add_box("RC522_Pin_Header", (20.0, 2.5, 8.5), (RFID_X, RFID_Y + RC522_CY + 28.5, RFID_Z + 6.6), mat_metal_gold)

# HW-479 RGB LED Module Dummy in Upper Cradle
hw479_pcb = add_box("HW479_PCB", (20.0, 15.0, 1.6), (RFID_X, RFID_Y + HW479_CY, RFID_Z + 6.0), mat_pcb_blue)
hw479_led = add_cylinder("HW479_5mm_LED", 2.5, 8.5, (RFID_X, RFID_Y + HW479_CY, RFID_Z + 7.6), mat=mat_rgb_led_green)
hw479_hdr = add_box("HW479_Pin_Header", (10.0, 2.5, 7.0), (RFID_X, RFID_Y + HW479_CY - 6.5, RFID_Z + 7.6), mat_metal_gold)

# USB Type-C Female Port Dummy on TOP END WALL
typec_conn_top = add_box("USB_C_Top_Connector", (9.0, 7.0, 3.2), (RFID_X, RFID_Y + 55.5, RFID_Z + 6.0), mat_metal_silver)

# SS-12D00 Mini Slide Switch Dummy on RIGHT SIDE WALL
switch_metal_case = add_box("SS12D00_Case", (4.4, 8.6, 4.5), (RFID_X + 27.0, RFID_Y + SW_CY, RFID_Z + 5.5), mat_switch_metal)
switch_lever_knob = add_box("SS12D00_Knob", (3.0, 2.0, 4.0), (RFID_X + 28.5, RFID_Y + SW_CY + 1.0, RFID_Z + 5.8), mat_switch_black)

# FACTORY-STANDARD CLIP-ON SWITCH KNOB CAP (SS-12D00 / SS-22D00)
print("Building Factory-Standard Clip-On Switch Knob Cap...")
cap_body = add_box("Slide_Switch_Knob_Cap", (3.2, 8.2, 5.4), (RFID_X + 28.0 + 1.2, RFID_Y + SW_CY + 1.0, RFID_Z + 5.1), mat_switch_black)

cap_skirt = add_box("Cap_Gliding_Skirt", (1.0, 10.0, 6.2), (RFID_X + 28.0 - 0.4, RFID_Y + SW_CY + 1.0, RFID_Z + 4.7), mat_switch_black)
apply_boolean(cap_body, cap_skirt, 'UNION')

cap_socket = add_box("Cap_Clip_Socket", (3.6, 2.1, 2.1), (RFID_X + 28.0 - 0.5, RFID_Y + SW_CY + 1.0, RFID_Z + 6.75))
apply_boolean(cap_body, cap_socket, 'DIFFERENCE')

for ro in [-2.2, 0.0, 2.2]:
    rib = add_box(f"Cap_Rib_{ro}", (0.5, 0.65, 4.4), (RFID_X + 28.0 + 2.8, RFID_Y + SW_CY + 1.0 + ro, RFID_Z + 5.6), mat_switch_black)
    apply_boolean(cap_body, rib, 'UNION')

bev_cap = cap_body.modifiers.new("Bevel", 'BEVEL')
bev_cap.width = 0.45
bev_cap.segments = 3
bev_cap.limit_method = 'ANGLE'
bev_cap.angle_limit = math.radians(60)
bpy.context.view_layer.objects.active = cap_body
bpy.ops.object.modifier_apply(modifier="Bevel")

# Power Bank Dummy in Wall-Mount Cradle
pbank_dummy = add_box("PowerBank_Pack", (74.0, 18.0, 70.0), (PBANK_CRADLE_X, PBANK_CRADLE_Y - 0.25, PBANK_CRADLE_Z + 4.5), mat_pbank_alu)

# Under-Table Hidden Wiring Harness connecting RFID Floor Slot to Controller Box Top Port
wire_harness = add_box("Underneath_Wire_Harness", (RFID_X - (CTRL_X + 16.0), 10.0, 3.0), ((RFID_X + CTRL_X + 16.0)/2.0, RFID_Y + 22.0, CTRL_Z + 11.0), mat_ribbon_cable)

# =============================================================
# 5. STUDIO LIGHTING & CAMERA SETUP
# =============================================================
print("5. Setting up Lighting and Cameras...")

floor = add_box("Studio_Floor", (850, 650, 2), (10, 0, -5), mat_floor)

sun_data = bpy.data.lights.new("Studio_Sun", type='SUN')
sun_data.energy = 4.2
sun_data.color = (1.0, 0.98, 0.95)
sun_obj = bpy.data.objects.new("Studio_Sun", sun_data)
sun_obj.location = (40, -100, 180)
sun_obj.rotation_euler = (math.radians(35), math.radians(20), math.radians(-25))
bpy.context.collection.objects.link(sun_obj)

fill_data = bpy.data.lights.new("Studio_Fill", type='AREA')
fill_data.energy = 180.0
fill_data.size = 120.0
fill_data.color = (0.85, 0.92, 1.0)
fill_obj = bpy.data.objects.new("Studio_Fill", fill_data)
fill_obj.location = (60, 120, 120)
fill_obj.rotation_euler = (math.radians(-45), 0, 0)
bpy.context.collection.objects.link(fill_obj)

rim_data = bpy.data.lights.new("Studio_Rim", type='AREA')
rim_data.energy = 110.0
rim_data.size = 80.0
rim_data.color = (0.2, 0.8, 1.0)
rim_obj = bpy.data.objects.new("Studio_Rim", rim_data)
rim_obj.location = (RFID_X + 50, -80, 60)
bpy.context.collection.objects.link(rim_obj)

cam_data = bpy.data.cameras.new("Master_Camera")
cam_data.lens = 30.0
cam_data.clip_start = 1.0
cam_data.clip_end = 25000.0
cam_obj = bpy.data.objects.new("Master_Camera", cam_data)
bpy.context.collection.objects.link(cam_obj)
bpy.context.scene.camera = cam_obj

aim_empty = bpy.data.objects.new("Cam_Target", None)
aim_empty.location = (5, 0, 18)
bpy.context.collection.objects.link(aim_empty)

track = cam_obj.constraints.new(type='TRACK_TO')
track.target = aim_empty
track.track_axis = 'TRACK_NEGATIVE_Z'
track.up_axis = 'UP_Y'

scene = bpy.context.scene
scene.render.engine = 'CYCLES'
scene.cycles.device = 'CPU'
scene.cycles.samples = 48
scene.render.resolution_x = 1920
scene.render.resolution_y = 1080

# =============================================================
# 6. RENDER PHOTO-REALISTIC SHOTS
# =============================================================
print("6. Rendering Exploded Isometric View (Showing Socketed NodeMCU, L-PCB & Enclosures)...")
cam_data.lens = 30.0
cam_obj.location = (5, -290, 190)
aim_empty.location = (5, 0, 26)
# Float the clip-on switch cap, power bank, and plug-in NodeMCU slightly upward to highlight the female sockets!
cap_body.location.x += 16.0
pbank_dummy.location.z += 38.0
nodemcu_pcb.location.z += 10.0
esp12_can.location.z += 10.0
usb_conn.location.z += 10.0
render_exploded_path = os.path.join(OUTPUT_DIR, "render_exploded_view.png")
scene.render.filepath = render_exploded_path
bpy.ops.render.render(write_still=True)
print(f"Saved: {render_exploded_path}")

# Snap components back to their seated assembled positions
cap_body.location.x -= 16.0
pbank_dummy.location.z -= 38.0
nodemcu_pcb.location.z -= 10.0
esp12_can.location.z -= 10.0
usb_conn.location.z -= 10.0

print("Rendering Outboard RFID Pod Detail View...")
cam_data.lens = 45.0
cam_obj.location = (RFID_X + 70.0, RFID_Y + 72.0, 58.0)
aim_empty.location = (RFID_X + 10.0, RFID_Y + 32.0, 12.0)
render_rfid_path = os.path.join(OUTPUT_DIR, "render_rfid_enclosure_detail.png")
scene.render.filepath = render_rfid_path
bpy.ops.render.render(write_still=True)
print(f"Saved: {render_rfid_path}")

print("Rendering Closed / Assembled System View...")
ctrl_lid.location.z = CTRL_BASE_H
rfid_face.location.z = POD_BASE_H

cam_data.lens = 30.0
cam_obj.location = (5, -280, 170)
aim_empty.location = (5, 0, 20)
render_closed_path = os.path.join(OUTPUT_DIR, "render_closed_view.png")
scene.render.filepath = render_closed_path
bpy.ops.render.render(write_still=True)
print(f"Saved: {render_closed_path}")

print("Rendering Interior Component & L-Shaped PCB Clearance View...")
ctrl_lid.hide_render = True
rfid_face.hide_render = True

cam_data.lens = 35.0
cam_obj.location = (55, 0, 260)
aim_empty.location = (55, 0, 10)
render_interior_path = os.path.join(OUTPUT_DIR, "render_interior_layout.png")
scene.render.filepath = render_interior_path
bpy.ops.render.render(write_still=True)
print(f"Saved: {render_interior_path}")

ctrl_lid.hide_render = False
rfid_face.hide_render = False

# =============================================================
# 7. EXPORT 3D PRINTABLE STL FILES IN 3D_Enclosure/v1
# =============================================================
print("7. Exporting 3D Printable Precision STL Files to 3D_Enclosure/v1...")
floor.hide_set(True)

def export_stl(obj, filename):
    bpy.ops.object.select_all(action='DESELECT')
    orig_loc = obj.location.copy()
    obj.location = (0, 0, 0)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    filepath = os.path.join(OUTPUT_DIR, filename)
    try:
        bpy.ops.wm.stl_export(filepath=filepath, export_selected_objects=True, global_scale=1.0)
    except Exception:
        bpy.ops.export_mesh.stl(filepath=filepath, use_selection=True, global_scale=1.0)
    obj.location = orig_loc
    print(f"Exported STL: {filename}")
    return filepath

stl_rfid_base  = export_stl(rfid_base,    "RFID_Pod_Base.stl")
stl_rfid_face  = export_stl(rfid_face,    "RFID_Pod_Faceplate.stl")
stl_ctrl_base  = export_stl(ctrl_base,    "Controller_Base.stl")
stl_ctrl_lid   = export_stl(ctrl_lid,     "Controller_Lid.stl")
stl_pb_cradle  = export_stl(pbank_cradle, "Power_Bank_Cradle.stl")
stl_pb_wall    = export_stl(pbank_cradle, "Power_Bank_Wall_Mount.stl")
stl_sw_cap     = export_stl(cap_body,     "Slide_Switch_Knob_Cap.stl")

export_stl(cap_body,  "RFID_Switch_Knob_Cap.stl")
export_stl(ctrl_base, "Enclosure_Base.stl")
export_stl(ctrl_lid,  "Enclosure_Lid.stl")
export_stl(rfid_base, "RFID_Enclosure_Base.stl")
export_stl(rfid_face, "RFID_Enclosure_Faceplate.stl")

blend_path = os.path.join(OUTPUT_DIR, "smarthive_pool_enclosure.blend")
bpy.ops.wm.save_as_mainfile(filepath=blend_path)
old_blend = os.path.join(OUTPUT_DIR, "snooker_opener_enclosure.blend")
if os.path.exists(old_blend):
    os.remove(old_blend)
print(f"Master Blender Scene Saved: {blend_path}")

print("="*65)
print("SUCCESS: SMARTHIVE POOL VERSION 1 (v1) 3D ENCLOSURES GENERATED!")
print(f"- Outboard RFID Pod Base:        {stl_rfid_base}")
print(f"- Outboard RFID Pod Faceplate:   {stl_rfid_face}")
print(f"- Inboard Controller Box Base:   {stl_ctrl_base}")
print(f"- Inboard Controller Box Lid:    {stl_ctrl_lid}")
print(f"- Wall-Mount Power Bank Holder:  {stl_pb_wall}")
print(f"- Clip-On Switch Knob Cap:       {stl_sw_cap}")
print("="*65)

