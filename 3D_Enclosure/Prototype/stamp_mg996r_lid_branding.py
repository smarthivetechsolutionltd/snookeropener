import bpy
import math
import os
import mathutils

OUT_DIR = "/Users/user/Documents/Projects/arduino/snookeropener/3D_Enclosure/Prototype"
LID_STL = os.path.join(OUT_DIR, "Controller_Box_MG996R_Pocket_Lid.stl")
LOGO_PATH = os.path.join(OUT_DIR, "smarthivetech_logo.png")

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.length_unit = 'MILLIMETERS'
bpy.context.scene.unit_settings.scale_length = 0.001

# 1. Vectorize official Smarthive logo
img = bpy.data.images.load(LOGO_PATH)
w, h = img.size[0], img.size[1]
raw_pixels = list(img.pixels)
logo_grid = [[0] * w for _ in range(h)]
for y in range(h):
    for x in range(w):
        if raw_pixels[(y * w + x) * 4 + 3] > 0.3:
            logo_grid[y][x] = -1

comp_counter = 1
for y in range(h):
    for x in range(w):
        if logo_grid[y][x] == -1:
            q = [(x, y)]
            logo_grid[y][x] = comp_counter
            while q:
                cx, cy = q.pop(0)
                for dx, dy in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                    nx, ny = cx + dx, cy + dy
                    if 0 <= nx < w and 0 <= ny < h and logo_grid[ny][nx] == -1:
                        logo_grid[ny][nx] = comp_counter
                        q.append((nx, ny))
            comp_counter += 1

dirs_8 = [(1, 0), (1, -1), (0, -1), (-1, -1), (-1, 0), (-1, 1), (0, 1), (1, 1)]

def trace_logo_contour(target_id):
    start = None
    for y in range(h - 1, -1, -1):
        for x in range(w):
            if logo_grid[y][x] == target_id:
                start = (x, y)
                break
        if start:
            break
    if not start:
        return []
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
logo_cx = (min(p[0] for p in all_pts) + max(p[0] for p in all_pts)) / 2.0
logo_cy = (min(p[1] for p in all_pts) + max(p[1] for p in all_pts)) / 2.0
logo_raw_h = max(p[1] for p in all_pts) - min(p[1] for p in all_pts)

# 2. Import FreeCAD lid STL (74mm X by 70mm Y, outer front face at Z=0, inner rim at Z=2.0..3.6)
bpy.ops.wm.stl_import(filepath=LID_STL)
lid = bpy.context.active_object
lid.name = "Controller_Box_MG996R_Pocket_Lid"

# Center X = 37.0, Y = 0..70 (vents are at Y = 12..30, so upper area Y = 34..64 is completely open!)
# Note: Outer face is at Z=0 facing -Z. Looking from -Z (outside the box, +Y up), +X is to the LEFT,
# so we rotate 180 deg around Y (scale_x = -1 via rot_y = pi) so logo & text read upright left-to-right!

def apply_bool(target, tool, op='DIFFERENCE'):
    mod = target.modifiers.new(name=f"Bool_{op}", type='BOOLEAN')
    mod.operation = op
    mod.object = tool
    mod.solver = 'EXACT'
    bpy.context.view_layer.objects.active = target
    bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.data.objects.remove(tool, do_unlink=True)

# 3. Create 18mm tall Smarthive Logo (debossed 0.85mm deep into Z=0 outer face -> support-free!)
scale = 18.0 / logo_raw_h
curve_data = bpy.data.curves.new("Logo_Curve", type='CURVE')
curve_data.dimensions = '2D'
curve_data.fill_mode = 'BOTH'
curve_data.extrude = 0.85  # Total Z thickness = 1.7mm (centered at Z=0 -> cuts 0.85mm deep!)
for pts in [c_top, c_bot]:
    spline = curve_data.splines.new('POLY')
    spline.points.add(len(pts) - 1)
    for i, (px, py) in enumerate(pts):
        spline.points[i].co = ((px - logo_cx) * scale, (py - logo_cy) * scale, 0.0, 1.0)
    spline.use_cyclic_u = True

logo_obj = bpy.data.objects.new("MG996R_Lid_Logo", curve_data)
bpy.context.collection.objects.link(logo_obj)
logo_obj.location = (37.0, 52.0, 0.0)
logo_obj.rotation_euler = (0.0, math.pi, 0.0)
bpy.context.view_layer.objects.active = logo_obj
bpy.ops.object.select_all(action='DESELECT')
logo_obj.select_set(True)
bpy.ops.object.convert(target='MESH')
bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
apply_bool(lid, logo_obj, 'DIFFERENCE')

# 4. Create bold "SMARTHIVE POOL" text using Arial Bold (clean non-self-intersecting strokes, 0.85mm deep!)
bpy.ops.object.text_add(location=(37.0, 36.5, 0.0))
txt = bpy.context.active_object
txt.name = "MG996R_Lid_Text"
font_path = "/System/Library/Fonts/Supplemental/Arial Bold.ttf"
if os.path.exists(font_path):
    txt.data.font = bpy.data.fonts.load(font_path)
txt.data.body = "SMARTHIVE POOL"
txt.data.size = 5.4
txt.data.extrude = 0.85
txt.data.align_x = 'CENTER'
txt.data.align_y = 'CENTER'
txt.rotation_euler = (0.0, math.pi, 0.0)
bpy.ops.object.convert(target='MESH')
bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
apply_bool(lid, txt, 'DIFFERENCE')

# 5. Export updated watertight STL
bpy.ops.object.select_all(action='DESELECT')
lid.select_set(True)
bpy.context.view_layer.objects.active = lid
bpy.ops.wm.stl_export(filepath=LID_STL, export_selected_objects=True)
print("Exported branded Controller_Box_MG996R_Pocket_Lid.stl!")

# 6. Render close-up of the outer front face (flipped upright so we verify readability!)
lid.rotation_euler = (0.0, math.pi, 0.0)
lid.location = (74.0, 0.0, 2.0)

m = bpy.data.materials.new("LidTeal")
m.use_nodes = True
bsdf = m.node_tree.nodes["Principled BSDF"]
bsdf.inputs["Base Color"].default_value = (0.05, 0.48, 0.54, 1.0)
bsdf.inputs["Roughness"].default_value = 0.32
lid.data.materials.append(m)

scene = bpy.context.scene
scene.render.engine = "BLENDER_EEVEE"
scene.render.resolution_x = 1280
scene.render.resolution_y = 720
world = bpy.data.worlds.new("W")
scene.world = world
world.use_nodes = True
world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.18, 0.20, 0.22, 1.0)

cam_d = bpy.data.cameras.new("Cam")
cam = bpy.data.objects.new("Cam", cam_d)
scene.collection.objects.link(cam)
scene.camera = cam
cam.location = (37.0, -35.0, 105.0)
direction = mathutils.Vector((37.0, 35.0, 2.0)) - cam.location
cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()

for loc, energy in [((100, -40, 120), 65000), ((-30, 80, 90), 30000)]:
    ld = bpy.data.lights.new("L", "AREA")
    ld.energy = energy
    ld.size = 70
    lo = bpy.data.objects.new("L", ld)
    lo.location = loc
    scene.collection.objects.link(lo)

scene.render.filepath = os.path.join(OUT_DIR, "render_mg996r_lid_branding.png")
bpy.ops.render.render(write_still=True)
