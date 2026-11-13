import bpy
import bmesh
import math
from mathutils import Vector, Euler
import os
import sys

# -------------------------------------------------------------
# Configuration & Output Directory
# -------------------------------------------------------------
OUTPUT_DIR = os.path.dirname(os.path.abspath(__file__))
os.makedirs(OUTPUT_DIR, exist_ok=True)
PROTO_DIR = os.path.join(OUTPUT_DIR, "Prototype")
os.makedirs(PROTO_DIR, exist_ok=True)

# Clean Scene
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
for block in list(bpy.data.materials):
    bpy.data.materials.remove(block)
for block in list(bpy.data.meshes):
    bpy.data.meshes.remove(block)
for block in list(bpy.data.cameras):
    bpy.data.cameras.remove(block)
for block in list(bpy.data.lights):
    bpy.data.lights.remove(block)

# Units configuration: Metric Millimeters (1 Blender Unit = 1 mm)
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.length_unit = 'MILLIMETERS'
bpy.context.scene.unit_settings.scale_length = 0.001

# -------------------------------------------------------------
# Dimensions: 8 x 8 Inches (203.2 x 203.2 mm)
# -------------------------------------------------------------
PLATE_W = 203.2  # 8.0 inches
PLATE_L = 203.2  # 8.0 inches
PLATE_H = 4.0    # 4.0 mm rigid thickness
CORNER_RADIUS = 8.0  # 8.0 mm smooth rounded corners

# 4 Corner Screw Holes (Set in 0.5 inches / 12.7 mm from each edge)
HOLE_MARGIN = 12.7  # 0.5 inches from edge
HOLE_OFFSET_X = (PLATE_W / 2.0) - HOLE_MARGIN  # ±88.9 mm (7.0" spacing)
HOLE_OFFSET_Y = (PLATE_L / 2.0) - HOLE_MARGIN  # ±88.9 mm (7.0" spacing)

HOLE_DIA = 4.5        # 4.5 mm clearance for #8 wood screws / M4 machine screws
CSINK_TOP_DIA = 9.0   # 9.0 mm countersink top diameter (standard #8 flat-head)
CSINK_DEPTH = 2.25    # 90-degree countersink depth (tapers 9.0mm -> 4.5mm)

# -------------------------------------------------------------
# Material Definition
# -------------------------------------------------------------
mat_faceplate = bpy.data.materials.new("Mat_Faceplate_Black")
mat_faceplate.use_nodes = True
bsdf = mat_faceplate.node_tree.nodes.get("Principled BSDF")
if bsdf:
    bsdf.inputs['Base Color'].default_value = (0.05, 0.06, 0.07, 1.0)
    bsdf.inputs['Roughness'].default_value = 0.40
    bsdf.inputs['Metallic'].default_value = 0.05

# -------------------------------------------------------------
# Geometry Builders
# -------------------------------------------------------------
def create_rounded_slab(name, width, length, height, radius, segments_per_corner=16, mat=None):
    """Creates a mathematically clean, manifold rounded rectangular slab."""
    mesh = bpy.data.meshes.new(name + "_Mesh")
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)

    bm = bmesh.new()
    hw = width / 2.0
    hl = length / 2.0
    r = min(radius, hw, hl)

    corners = [
        ( hw - r,  hl - r, 0.0, math.pi / 2.0),
        (-hw + r,  hl - r, math.pi / 2.0, math.pi),
        (-hw + r, -hl + r, math.pi, 3.0 * math.pi / 2.0),
        ( hw - r, -hl + r, 3.0 * math.pi / 2.0, 2.0 * math.pi)
    ]

    verts_bottom = []
    for cx, cy, start_ang, end_ang in corners:
        for i in range(segments_per_corner):
            theta = start_ang + (end_ang - start_ang) * (i / float(segments_per_corner))
            x = cx + r * math.cos(theta)
            y = cy + r * math.sin(theta)
            verts_bottom.append(bm.verts.new((x, y, 0.0)))

    face_bottom = bm.faces.new(verts_bottom)

    # Extrude upward along Z by height
    ret = bmesh.ops.extrude_face_region(bm, geom=[face_bottom])
    verts_extruded = [v for v in ret['geom'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.translate(bm, vec=Vector((0, 0, height)), verts=verts_extruded)

    # Recalculate normals outwards
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)

    bm.to_mesh(mesh)
    bm.free()

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

def add_countersunk_hole_cutter(name, loc, hole_dia, csink_top_dia, csink_depth, plate_h):
    """Creates a combined through-hole and countersink conical cutter."""
    mesh = bpy.data.meshes.new(name + "_Mesh")
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)

    bm = bmesh.new()
    r_hole = hole_dia / 2.0
    r_top = csink_top_dia / 2.0
    segs = 32

    # Bottom through-hole (below plate by 1mm to ensure clean cut)
    z_bottom = -1.0
    z_trans = plate_h - csink_depth
    z_top = plate_h + 1.0  # Slightly above plate top to ensure clean surface cut

    # Generate rings of vertices
    v_bottom = []
    v_trans = []
    v_top = []
    for i in range(segs):
        ang = 2.0 * math.pi * (i / float(segs))
        cos_a = math.cos(ang)
        sin_a = math.sin(ang)

        v_bottom.append(bm.verts.new((loc[0] + r_hole * cos_a, loc[1] + r_hole * sin_a, z_bottom)))
        v_trans.append(bm.verts.new((loc[0] + r_hole * cos_a, loc[1] + r_hole * sin_a, z_trans)))
        v_top.append(bm.verts.new((loc[0] + r_top * cos_a, loc[1] + r_top * sin_a, z_top)))

    # Faces for through cylinder
    for i in range(segs):
        i_next = (i + 1) % segs
        bm.faces.new([v_bottom[i], v_bottom[i_next], v_trans[i_next], v_trans[i]])

    # Faces for conical countersink
    for i in range(segs):
        i_next = (i + 1) % segs
        bm.faces.new([v_trans[i], v_trans[i_next], v_top[i_next], v_top[i]])

    # End caps
    bm.faces.new(list(reversed(v_bottom)))
    bm.faces.new(v_top)

    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()
    return obj

# -------------------------------------------------------------
# Build 8x8 Faceplate
# -------------------------------------------------------------
print("Building 8x8 inch (203.2 x 203.2 mm) Solid Faceplate...")
faceplate = create_rounded_slab("Faceplate_8x8", PLATE_W, PLATE_L, PLATE_H, CORNER_RADIUS, segments_per_corner=16, mat=mat_faceplate)

# Top perimeter subtle chamfer (0.8mm at 45 deg)
top_bevel = faceplate.modifiers.new(name="Top_Chamfer", type='BEVEL')
top_bevel.width = 0.8
top_bevel.segments = 2
top_bevel.limit_method = 'ANGLE'
top_bevel.angle_limit = math.radians(70)
bpy.context.view_layer.objects.active = faceplate
bpy.ops.object.modifier_apply(modifier=top_bevel.name)

# Add 4 Corner Countersunk Mounting Screw Holes
hole_coords = [
    ( HOLE_OFFSET_X,  HOLE_OFFSET_Y),
    (-HOLE_OFFSET_X,  HOLE_OFFSET_Y),
    (-HOLE_OFFSET_X, -HOLE_OFFSET_Y),
    ( HOLE_OFFSET_X, -HOLE_OFFSET_Y)
]

print("Applying 4 corner countersunk screw holes at ±88.9 mm...")
for idx, (hx, hy) in enumerate(hole_coords):
    cutter = add_countersunk_hole_cutter(f"CSink_Cutter_{idx}", (hx, hy), HOLE_DIA, CSINK_TOP_DIA, CSINK_DEPTH, PLATE_H)
    apply_boolean(faceplate, cutter, 'DIFFERENCE')

# -------------------------------------------------------------
# Export STL Files
# -------------------------------------------------------------
def export_stl(obj, out_path):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    try:
        bpy.ops.wm.stl_export(filepath=out_path, export_selected_objects=True, global_scale=1.0)
    except Exception:
        bpy.ops.export_mesh.stl(filepath=out_path, use_selection=True, global_scale=1.0)
    print(f"Exported STL: {out_path}")

stl_root = os.path.join(OUTPUT_DIR, "Faceplate_8x8.stl")
stl_proto = os.path.join(PROTO_DIR, "Faceplate_8x8.stl")
export_stl(faceplate, stl_root)
export_stl(faceplate, stl_proto)

# -------------------------------------------------------------
# Studio Lighting & Camera Render
# -------------------------------------------------------------
print("Setting up studio render...")
cam_data = bpy.data.cameras.new("RenderCam")
cam_obj = bpy.data.objects.new("RenderCam", cam_data)
bpy.context.collection.objects.link(cam_obj)
cam_obj.location = (180.0, -220.0, 200.0)
cam_obj.rotation_euler = (math.radians(52), 0, math.radians(40))
bpy.context.scene.camera = cam_obj

# Key light
key_light_data = bpy.data.lights.new("KeyLight", type='AREA')
key_light_data.energy = 180.0
key_light_data.size = 300.0
key_light = bpy.data.objects.new("KeyLight", key_light_data)
bpy.context.collection.objects.link(key_light)
key_light.location = (200.0, -180.0, 250.0)

# Fill light
fill_light_data = bpy.data.lights.new("FillLight", type='AREA')
fill_light_data.energy = 80.0
fill_light_data.size = 400.0
fill_light = bpy.data.objects.new("FillLight", fill_light_data)
bpy.context.collection.objects.link(fill_light)
fill_light.location = (-180.0, -150.0, 180.0)

# Rim light
rim_light_data = bpy.data.lights.new("RimLight", type='AREA')
rim_light_data.energy = 120.0
rim_light_data.size = 250.0
rim_light = bpy.data.objects.new("RimLight", rim_light_data)
bpy.context.collection.objects.link(rim_light)
rim_light.location = (0.0, 220.0, 180.0)

# Render settings
bpy.context.scene.render.engine = 'CYCLES'
bpy.context.scene.cycles.samples = 64
bpy.context.scene.render.resolution_x = 1200
bpy.context.scene.render.resolution_y = 900
bpy.context.scene.render.image_settings.file_format = 'PNG'

render_path = os.path.join(OUTPUT_DIR, "render_faceplate_8x8.png")
bpy.context.scene.render.filepath = render_path
bpy.ops.render.render(write_still=True)
print(f"Render saved: {render_path}")

print("=== Faceplate Build Completed Successfully ===")

