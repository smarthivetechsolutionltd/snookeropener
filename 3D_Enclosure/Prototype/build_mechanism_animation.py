import bpy
import math
import os
from mathutils import Vector

OUT_DIR = "/Users/user/Documents/Projects/arduino/snookeropener/3D_Enclosure/Prototype"
BLEND_PATH = os.path.join(OUT_DIR, "snooker_mechanism_animation.blend")
os.makedirs(OUT_DIR, exist_ok=True)

# 1. Reset scene
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 0.001  # 1 Blender unit = 1 mm
scene.render.fps = 30
scene.frame_start = 1
scene.frame_end = 180

try:
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
except Exception:
    scene.render.engine = 'BLENDER_EEVEE'
scene.render.resolution_x = 1280
scene.render.resolution_y = 720
scene.render.resolution_percentage = 100


def make_mat(name, color, roughness=0.45, metallic=0.0, emission=None, emission_strength=0.0):
    mat = bpy.data.materials.new(name=name)
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = (*color, 1.0)
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["Metallic"].default_value = metallic
    if emission is not None and "Emission Color" in bsdf.inputs:
        bsdf.inputs["Emission Color"].default_value = (*emission, 1.0)
        bsdf.inputs["Emission Strength"].default_value = emission_strength
    return mat


# Materials
mat_wall = make_mat("CabinetWall", (0.86, 0.86, 0.84), roughness=0.72)
mat_wood = make_mat("WoodFrame", (0.48, 0.34, 0.19), roughness=0.62)
mat_floor = make_mat("CabinetFloor", (0.28, 0.27, 0.25), roughness=0.8)
mat_black_petg = make_mat("BlackPETG", (0.04, 0.045, 0.055), roughness=0.36, metallic=0.05)
mat_emerald_petg = make_mat("EmeraldPETG", (0.05, 0.68, 0.46), roughness=0.30, metallic=0.05)
mat_dark_steel = make_mat("DarkSteel", (0.14, 0.15, 0.17), roughness=0.40, metallic=0.88)
mat_chrome = make_mat("SteelBolt", (0.80, 0.82, 0.85), roughness=0.20, metallic=0.95)
mat_servo_body = make_mat("ServoBody", (0.07, 0.07, 0.08), roughness=0.35, metallic=0.1)
mat_horn_metal = make_mat("AluminumHorn", (0.76, 0.12, 0.14), roughness=0.24, metallic=0.9)
mat_card = make_mat("RFIDCard", (0.95, 0.96, 0.98), roughness=0.20, metallic=0.05)
mat_card_stripe = make_mat("CardStripe", (0.05, 0.65, 0.45), roughness=0.25)
mat_led_green = make_mat("LEDGreen", (0.05, 0.95, 0.45), roughness=0.18, emission=(0.05, 1.0, 0.45), emission_strength=0.6)

ribbon_colors = [
    (0.85, 0.15, 0.15), (0.92, 0.45, 0.1), (0.92, 0.82, 0.12),
    (0.15, 0.72, 0.25), (0.12, 0.45, 0.88), (0.55, 0.2, 0.78),
]
ribbon_mats = [make_mat(f"Ribbon_{i}", c, roughness=0.4) for i, c in enumerate(ribbon_colors)]

ball_specs = [
    ("Ball_Red1", (0.78, 0.04, 0.04)),
    ("Ball_Red2", (0.75, 0.05, 0.05)),
    ("Ball_Yellow", (0.94, 0.78, 0.05)),
    ("Ball_Green", (0.04, 0.50, 0.16)),
    ("Ball_Blue", (0.05, 0.22, 0.80)),
    ("Ball_Black", (0.03, 0.03, 0.035)),
]
ball_mats = [make_mat(n, c, roughness=0.08, metallic=0.02) for n, c in ball_specs]


def add_box(name, size, loc, mat=None, bevel=0.0):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=loc)
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = size
    bpy.ops.object.transform_apply(scale=True)
    if bevel > 0:
        mod = obj.modifiers.new("Bevel", 'BEVEL')
        mod.width = bevel
        mod.segments = 2
    if mat:
        obj.data.materials.append(mat)
    return obj


def add_cyl(name, radius, depth, loc, rot=(0, 0, 0), mat=None, verts=32):
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts, radius=radius, depth=depth, location=loc, rotation=rot)
    obj = bpy.context.active_object
    obj.name = name
    if mat:
        obj.data.materials.append(mat)
    return obj


# NOTE ON CAMERA ORIENTATION:
# Camera sits at Y = -410 looking toward +Y (back wall at Y = +6).
# That way -X is LEFT and +X is RIGHT — matching your video 1:1!
# Left (-X): Rebar + Gate Arm + PETG Clamp
# Center (0..+60): Black Servo Enclosure + MG996R Horn
# Right (+135): RFID Pod + Status LED + Staff RFID Card

# ============================================================
# 2. CABINET ENVIRONMENT
# ============================================================
add_box("Cabinet_BackWall", (600, 12, 340), (15, 6, 130), mat_wall)
add_box("Cabinet_Floor", (600, 300, 12), (15, -130, -6), mat_floor)
# Wooden ball-tray / drawer frame in foreground (-Y)
add_box("WoodDrawer_Back", (280, 16, 46), (10, -76, 23), mat_wood, bevel=1.0)
add_box("WoodDrawer_Left", (16, 130, 46), (-122, -134, 23), mat_wood, bevel=1.0)
add_box("WoodDrawer_Right", (16, 130, 46), (142, -134, 23), mat_wood, bevel=1.0)
# Metal L-bracket stop at bottom of vertical rebar
add_box("LBracket_Vert", (18, 4, 28), (-90, -2, 14), mat_chrome)
add_box("LBracket_Horiz", (18, 22, 4), (-90, -11, 2), mat_chrome)

# ============================================================
# 3. SMARTHIVE POOL COMPACT MG996R ENCLOSURE + SERVO
# ============================================================
# Positioned directly behind the gate rebar so the linkage pulls at a ~90° right angle!
enc_x, enc_y, enc_z = -64.0, -18.0, 46.0
add_box("Servo_Enclosure", (74, 34, 70), (enc_x, enc_y, enc_z), mat_black_petg, bevel=2.0)

# Louver vent slats on front face (Y = enc_y - 17.2)
for i in range(4):
    add_box(f"Vent_Slat_{i}", (36, 2.2, 2.4), (enc_x, enc_y - 17.2, enc_z - 22 + i * 5.2), mat_dark_steel)

# MG996R Servo at top-left of enclosure (almost dead behind rebar for perpendicular pull!)
servo_x = -92.0
servo_y = -20.0
servo_top_z = enc_z + 35.0  # Z = 81.0
add_box("MG996R_Body", (40, 20, 40), (servo_x, servo_y, servo_top_z - 16), mat_servo_body)
add_cyl("MG996R_Shaft", 3.0, 12.0, (servo_x, servo_y, servo_top_z + 6), mat=mat_chrome)

# Servo Horn Pivot (rotates around Z-axis!)
horn_z = servo_top_z + 10.0  # Z = 92.0
horn_pivot = bpy.data.objects.new("Servo_Horn_Pivot", None)
horn_pivot.location = (servo_x, servo_y, horn_z)
scene.collection.objects.link(horn_pivot)

horn_hub = add_cyl("Servo_Horn_Hub", 5.5, 5.0, (servo_x, servo_y, horn_z), mat=mat_horn_metal)
horn_arm = add_box("Servo_Horn_Arm", (26, 7.5, 3.8), (servo_x - 11.0, servo_y, horn_z), mat=mat_horn_metal, bevel=0.8)
horn_screw = add_cyl("Servo_Horn_Screw", 2.2, 6.5, (servo_x, servo_y, horn_z + 1.0), mat=mat_chrome)

# Pin Empty at the tip of the Servo Horn (X = servo_x - 20)
horn_tip_empty = bpy.data.objects.new("Servo_Horn_Tip_Pin", None)
horn_tip_empty.location = (servo_x - 20.0, servo_y, horn_z + 4.0)
scene.collection.objects.link(horn_tip_empty)
horn_tip_pin = add_cyl("Servo_Horn_PivotPin", 1.8, 10.0, (servo_x - 20.0, servo_y, horn_z + 2.5), mat=mat_chrome, verts=16)

for part in (horn_hub, horn_arm, horn_screw, horn_tip_empty, horn_tip_pin):
    part.parent = horn_pivot
    part.matrix_parent_inverse = horn_pivot.matrix_world.inverted()

# Rainbow Ribbon Cable routed neatly out bottom right of enclosure to RFID pod
for i, rmat in enumerate(ribbon_mats):
    add_cyl(
        f"Ribbon_Wire_{i}", 0.9, 105.0,
        (enc_x + 68.0, enc_y - 4.0 - i * 2.0, 3.0),
        rot=(0, math.radians(90), math.radians(-10)),
        mat=rmat, verts=12
    )

# ============================================================
# 4. RFID READER POD + STATUS RGB LED + STAFF RFID CARD (Right side)
# ============================================================
pod_x, pod_y, pod_z = 132.0, -24.0, 44.0
add_box("RFID_Pod_Case", (54, 22, 66), (pod_x, pod_y, pod_z), mat_black_petg, bevel=2.5)
add_box("RFID_Tap_Zone", (38, 2.0, 38), (pod_x, pod_y - 11.2, pod_z - 4), mat_dark_steel, bevel=1.0)
bpy.ops.mesh.primitive_uv_sphere_add(radius=4.5, location=(pod_x, pod_y - 11.5, pod_z + 22.0))
status_led = bpy.context.active_object
status_led.name = "Status_RGB_LED"
status_led.data.materials.append(mat_led_green)

# Staff RFID Card (animated to tap reader at frames 12..30!)
rfid_card = add_box("Staff_RFID_Card", (50, 1.5, 78), (pod_x + 30, pod_y - 80, pod_z + 15), mat_card, bevel=1.0)
card_accent = add_box("Staff_Card_Stripe", (50.2, 1.8, 14), (pod_x + 30, pod_y - 80, pod_z + 32), mat_card_stripe)
card_accent.parent = rfid_card
card_accent.matrix_parent_inverse = rfid_card.matrix_world.inverted()
rfid_card.rotation_euler = (math.radians(15), 0, math.radians(10))

# ============================================================
# 5. BALL-GATE ARM (CIRCULAR Z-PIVOT LIKE A SERVO ARM!) + REBAR + PETG CLAMP
# ============================================================
# Gate arm pivots in a circular horizontal arc around its fixed vertical hinge pin at (-175, -112)!
rod_x = -82.0
rod_y = -112.0
gate_px, gate_py = -175.0, rod_y
gate_pivot = bpy.data.objects.new("Ball_Gate_Pivot", None)
gate_pivot.location = (gate_px, gate_py, 148.0)
scene.collection.objects.link(gate_pivot)

# Visible fixed vertical hinge post so the circular pivot axis is unmistakable
add_cyl("Gate_Fixed_Hinge_Post", 7.0, 42.0, (gate_px, gate_py, 148.0), mat=mat_chrome, verts=24)

gate_arm = add_box("Gate_Steel_Arm", (118, 24, 24), (-125.0, rod_y, 148.0), mat_dark_steel, bevel=1.0)
rod_shaft = add_cyl("Gate_Rebar_Rod", 5.0, 128.0, (rod_x, rod_y, 80.0), mat=mat_dark_steel)
ribs = []
for rz in range(24, 136, 9):
    rib = add_cyl(f"Rebar_Rib_{rz}", 5.9, 2.6, (rod_x, rod_y, rz), rot=(math.radians(12), 0, 0), mat=mat_dark_steel, verts=20)
    ribs.append(rib)

# PETG Split Clamp Collar aligned at exact height of servo horn (Z = horn_z)
clamp_z = horn_z
clamp_pivot = bpy.data.objects.new("PETG_Clamp_Group", None)
clamp_pivot.location = (rod_x, rod_y, clamp_z)
scene.collection.objects.link(clamp_pivot)

clamp_front = add_box("PETG_Clamp_Front", (38, 11, 24), (rod_x, rod_y - 6.5, clamp_z), mat_emerald_petg, bevel=1.2)
clamp_back = add_box("PETG_Clamp_Back", (38, 11, 24), (rod_x, rod_y + 6.5, clamp_z), mat_emerald_petg, bevel=1.2)

# Flat horizontal clevis ear pointing at a 90° right angle (+Y, toward back wall & servo!)
tab_len = 26.0
clamp_tab = add_box("PETG_Clamp_ClevisTab", (tab_len, 14.0, 6.0), (rod_x + 18.0, rod_y, clamp_z - 1.5), mat_emerald_petg, bevel=1.0)
clamp_tab_tip = add_cyl("PETG_Clamp_ClevisTip", 7.0, 6.0, (rod_x + 31.0, rod_y, clamp_z - 1.5), mat=mat_emerald_petg, verts=24)

clamp_pin_empty = bpy.data.objects.new("Clamp_Clevis_Pin", None)
clamp_pin_empty.location = (rod_x + 31.0, rod_y, clamp_z + 4.0)
scene.collection.objects.link(clamp_pin_empty)
clamp_pin = add_cyl("Clamp_Clevis_PivotPin", 2.0, 16.0, (rod_x + 31.0, rod_y, clamp_z + 2.5), mat=mat_chrome, verts=18)

clamp_bolts = []
for sx in (-13, 13):
    for sz in (-5.5, 5.5):
        b = add_cyl(
            f"Clamp_Bolt_{sx}_{sz}", 2.2, 26.0,
            (rod_x + sx, rod_y, clamp_z + sz),
            rot=(math.radians(90), 0, 0),
            mat=mat_chrome, verts=16
        )
        clamp_bolts.append(b)

for cpart in [clamp_front, clamp_back, clamp_tab, clamp_tab_tip, clamp_pin_empty, clamp_pin] + clamp_bolts:
    cpart.parent = clamp_pivot
    cpart.matrix_parent_inverse = clamp_pivot.matrix_world.inverted()

# Orient clamp clevis ear straight back (+Y, 90° perpendicular to gate arm!)
clamp_pivot.rotation_euler = (0, 0, math.radians(90.0))

for part in [gate_arm, rod_shaft, clamp_pivot] + ribs:
    part.parent = gate_pivot
    part.matrix_parent_inverse = gate_pivot.matrix_world.inverted()

# ============================================================
# 6. EXACT 4-BAR CRANK-ROCKER SOLVER (CIRCULAR ARC GATE PIVOT!)
# ============================================================
horn_r = 20.0  # Distance from servo shaft to horn tip pin
# Horn starts pointing toward the clamp (-85°) and sweeps 140° clockwise to -225° (+135°),
# pulling the gate arm backward at a ~90° right angle along its circular arc!
theta_locked = math.radians(-85.0)
theta_open = math.radians(-225.0)

# Exact clamp pin rest position in world XY (clevis ear points +Y by 31mm from rod)
c0_x = rod_x
c0_y = rod_y + 31.0

# Rocker circle radius & rest angle of clamp pin around fixed gate_pivot (gate_px, gate_py)
rocker_R = math.hypot(c0_x - gate_px, c0_y - gate_py)
rocker_beta0 = math.atan2(c0_y - gate_py, c0_x - gate_px)

# Exact servo horn pin rest position (at theta_locked)
h0_x = servo_x + horn_r * math.cos(theta_locked)
h0_y = servo_y + horn_r * math.sin(theta_locked)

# Exact rigid bar pin-to-pin length L!
link_L = math.hypot(h0_x - c0_x, h0_y - c0_y)
print(f"Exact 4-Bar Linkage Length L = {link_L:.2f} mm | Gate Rocker R = {rocker_R:.2f} mm")

# Build rounded-end PETG Linkage Bar with pin centers at X = 0 and X = link_L!
bar_w = 11.0
bar_h = 5.0
bar_body = add_box("PETG_Link_Body", (link_L, bar_w, bar_h), (link_L / 2.0, 0, 0), mat_emerald_petg)
bar_end1 = add_cyl("PETG_Link_End1", bar_w / 2.0, bar_h, (0, 0, 0), mat=mat_emerald_petg, verts=24)
bar_end2 = add_cyl("PETG_Link_End2", bar_w / 2.0, bar_h, (link_L, 0, 0), mat=mat_emerald_petg, verts=24)

# Join into single mesh with origin cleanly at (0, 0, 0) (the Clamp Pin!)
bpy.ops.object.select_all(action='DESELECT')
bar_body.select_set(True)
bar_end1.select_set(True)
bar_end2.select_set(True)
bpy.context.view_layer.objects.active = bar_end1
bpy.ops.object.join()
link_bar = bpy.context.active_object
link_bar.name = "PETG_Slotted_Linkage_Bar"

# Constraint 1: Pin 1 locked to Clamp Clevis Pin
c_loc = link_bar.constraints.new('COPY_LOCATION')
c_loc.target = clamp_pin_empty
# Constraint 2: +X axis tracks Servo Horn Tip Pin (Pin 2 sits dead-on Horn Tip Pin on EVERY frame!)
c_trk = link_bar.constraints.new('TRACK_TO')
c_trk.target = horn_tip_empty
c_trk.track_axis = 'TRACK_X'
c_trk.up_axis = 'UP_Z'

# ============================================================
# 7. SNOOKER BALLS IN CHUTE (Roll left -> right into wooden tray!)
# ============================================================
balls = []
for idx, ((bname, _), bmat) in enumerate(zip(ball_specs, ball_mats)):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=32, ring_count=16, radius=13.5, location=(-155 - idx * 29, -112, 14.0))
    b = bpy.context.active_object
    b.name = bname
    b.data.materials.append(bmat)
    for poly in b.data.polygons:
        poly.use_smooth = True
    balls.append(b)

# ============================================================
# 8. KEYFRAME ANIMATION (180 Frames @ 30fps = 6.0s Loop)
# ============================================================
# A) Staff RFID Card taps reader (Frames 1 -> 18 -> 30 -> 46)
rfid_card.location = (pod_x + 28, pod_y - 78, pod_z + 12)
rfid_card.keyframe_insert(data_path="location", frame=1)
rfid_card.location = (pod_x, pod_y - 16, pod_z - 2)
rfid_card.keyframe_insert(data_path="location", frame=18)
rfid_card.keyframe_insert(data_path="location", frame=30)
rfid_card.location = (pod_x + 42, pod_y - 90, pod_z + 18)
rfid_card.keyframe_insert(data_path="location", frame=46)

# B & C) Exact Per-Frame 4-Bar Crank-Rocker Kinematics (Gate Arm pivots around fixed Z-axis!)
def smoothstep(t):
    t = max(0.0, min(1.0, t))
    return t * t * (3.0 - 2.0 * t)

def solve_gate_rocker_angle(theta):
    """Exact circle-circle intersection: |C(phi) - H(theta)| == link_L on rocker circle (G, rocker_R)."""
    hx = servo_x + horn_r * math.cos(theta)
    hy = servo_y + horn_r * math.sin(theta)
    dx, dy = hx - gate_px, hy - gate_py
    d = math.hypot(dx, dy)
    # Law of cosines for angle offset from vector G->H to vector G->C
    cos_a = (rocker_R * rocker_R + d * d - link_L * link_L) / (2.0 * rocker_R * d)
    cos_a = max(-1.0, min(1.0, cos_a))
    a = math.acos(cos_a)
    base_ang = math.atan2(dy, dx)
    # Pick the branch closest to the lower/front side (beta = base_ang - a -> matches beta0 at theta_locked!)
    cand1 = base_ang - a
    cand2 = base_ang + a
    # Choose the branch that gives smaller |cand - rocker_beta0| at locked; continuously stays on cand1
    beta = cand1 if abs(cand1 - rocker_beta0) <= abs(cand2 - rocker_beta0) else cand2
    return beta - rocker_beta0

for f in range(1, 181):
    if f <= 28:
        alpha = 0.0
    elif f <= 52:
        alpha = smoothstep((f - 28) / 24.0)
    elif f <= 135:
        alpha = 1.0
    elif f <= 160:
        alpha = 1.0 - smoothstep((f - 135) / 25.0)
    else:
        alpha = 0.0

    theta = theta_locked + alpha * (theta_open - theta_locked)
    horn_pivot.rotation_euler = (0, 0, theta - math.pi)
    horn_pivot.keyframe_insert(data_path="rotation_euler", frame=f)

    gate_rot_z = solve_gate_rocker_angle(theta)
    gate_pivot.rotation_euler = (0, 0, gate_rot_z)
    gate_pivot.keyframe_insert(data_path="rotation_euler", frame=f)

# D) Snooker Balls roll into the wooden tray (Frames 48 -> 125)
for idx, b in enumerate(balls):
    start_x = -165.0 - idx * 29.0
    end_x = 110.0 - idx * 30.0
    f_start = 46 + idx * 7
    f_end = 90 + idx * 7
    b.location = (start_x, -112.0, 14.0)
    b.keyframe_insert(data_path="location", frame=1)
    b.keyframe_insert(data_path="location", frame=f_start)
    b.location = (end_x, -112.0, 14.0)
    b.keyframe_insert(data_path="location", frame=f_end)
    b.keyframe_insert(data_path="location", frame=180)

# E) Status RGB LED Rapid 5Hz Green Blink while gate is open (Frames 28..158)
bsdf_led = mat_led_green.node_tree.nodes.get("Principled BSDF")
em_in = bsdf_led.inputs["Emission Strength"]
em_in.default_value = 0.6
em_in.keyframe_insert(data_path="default_value", frame=1)
em_in.keyframe_insert(data_path="default_value", frame=27)
for f in range(28, 159, 3):
    em_in.default_value = 16.0 if ((f // 3) % 2 == 0) else 0.1
    em_in.keyframe_insert(data_path="default_value", frame=f)
em_in.default_value = 0.6
em_in.keyframe_insert(data_path="default_value", frame=162)
em_in.keyframe_insert(data_path="default_value", frame=180)

# ============================================================
# 9. CAMERA & STUDIO LIGHTING
# ============================================================
world = bpy.data.worlds.new("StudioWorld")
scene.world = world
bg = world.node_tree.nodes.get("Background")
bg.inputs["Color"].default_value = (0.08, 0.09, 0.11, 1.0)
bg.inputs["Strength"].default_value = 0.8


def add_area_light(name, loc, target, energy, size, color=(1, 1, 1)):
    ldata = bpy.data.lights.new(name=name, type='AREA')
    ldata.energy = energy
    ldata.size = size
    ldata.color = color
    lobj = bpy.data.objects.new(name=name, object_data=ldata)
    lobj.location = loc
    direction = Vector(target) - Vector(loc)
    lobj.rotation_euler = direction.to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(lobj)
    return lobj


add_area_light("Key_Light", (50, -320, 260), (20, -25, 65), energy=950000.0, size=200.0, color=(1.0, 0.98, 0.94))
add_area_light("Fill_Light", (-240, -240, 150), (0, -25, 60), energy=360000.0, size=200.0, color=(0.82, 0.90, 1.0))
add_area_light("Rim_Light", (220, -80, 220), (20, -25, 75), energy=380000.0, size=140.0, color=(0.75, 0.98, 0.90))

# Wide framing: Rebar + PETG Clamp (Left) -> Servo Box + Linkage (Center) -> RFID Pod + Card (Right) -> Ball Tray (Bottom)
cam_data = bpy.data.cameras.new("MainCamera")
cam_data.lens = 34.0
cam_obj = bpy.data.objects.new("MainCamera", cam_data)
cam_obj.location = (22.0, -405.0, 148.0)
cam_dir = Vector((20.0, -30.0, 62.0)) - cam_obj.location
cam_obj.rotation_euler = cam_dir.to_track_quat('-Z', 'Y').to_euler()
scene.collection.objects.link(cam_obj)
scene.camera = cam_obj

# Save .blend file
bpy.ops.wm.save_as_mainfile(filepath=BLEND_PATH)
print("Saved animated Blender scene:", BLEND_PATH)

# Render front preview + top-3/4 isometric preview so 3D offset is clearly verified
for frame, fname in [(20, "anim_preview_tap_locked.png"), (90, "anim_preview_open_balls.png")]:
    scene.frame_set(frame)
    scene.render.filepath = os.path.join(OUT_DIR, fname)
    bpy.ops.render.render(write_still=True)
    print("Rendered preview:", scene.render.filepath)

# Also render a 3/4 top-angle preview at frame 90
iso_cam_data = bpy.data.cameras.new("IsoCamera")
iso_cam_data.lens = 35.0
iso_cam = bpy.data.objects.new("IsoCamera", iso_cam_data)
iso_cam.location = (-165.0, -340.0, 290.0)
iso_dir = Vector((-10.0, -60.0, 65.0)) - iso_cam.location
iso_cam.rotation_euler = iso_dir.to_track_quat('-Z', 'Y').to_euler()
scene.collection.objects.link(iso_cam)
scene.camera = iso_cam
scene.frame_set(90)
scene.render.filepath = os.path.join(OUT_DIR, "anim_preview_iso_top.png")
bpy.ops.render.render(write_still=True)
scene.camera = cam_obj
bpy.ops.wm.save_as_mainfile(filepath=BLEND_PATH)
print("Rendered iso preview & re-saved blend.")
