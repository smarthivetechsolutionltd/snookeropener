"""
CoppeliaSim Real-Time Rigid-Body Physics Simulation
Smarthive Pool — MG996R 4-Bar Snooker Ball-Gate Mechanism

Prerequisites:
  1. Download & open CoppeliaSim Edu (macOS): https://www.coppeliarobotics.com/
  2. Install the ZMQ Remote API client:
       pip3 install coppeliasim-zmqremoteapi-client
  3. With CoppeliaSim open, run:
       python3 simulate_coppeliasim.py

What this builds & simulates in real-time physics (Bullet 2.83 / MuJoCo):
  - Angled snooker ball chute + wooden ball-catch tray
  - 6 dynamic snooker balls rolling under gravity against the steel rebar gate
  - Fixed vertical Z-axis gate hinge (circular-arc rocker arm, just like a servo arm)
  - MG996R servo joint (realistic 0.98 N·m / 10 kg·cm stall torque, 60°/0.17s speed)
  - 4-bar linkage bar with loop-closure dummy pair pulling the gate at ~90° right angle
  - Live printout of MG996R torque (kg·cm), horn angle, and gate swing angle!
"""

import math
import time
from coppeliasim_zmqremoteapi_client import RemoteAPIClient  # type: ignore[import-not-found]


def main():
    print("Connecting to CoppeliaSim ZMQ Remote API...")
    client = RemoteAPIClient()
    sim = client.require("sim")

    # Reset to a clean scene
    if sim.getSimulationState() != sim.simulation_stopped:
        sim.stopSimulation()
        while sim.getSimulationState() != sim.simulation_stopped:
            time.sleep(0.05)

    sim.closeScene()

    # Configure Bullet physics engine (fallback safely to default engine)
    engine = getattr(sim, "physics_bullet", getattr(sim, "physics_mujoco", None))
    if engine is not None:
        sim.setInt32Param(sim.intparam_dynamic_engine, engine)
    sim.setFloatParam(sim.floatparam_simulation_time_step, 0.01)

    # All CoppeliaSim units are METERS (1 mm = 0.001 m)
    MM = 0.001

    def make_box(name, size_mm, pos_mm, color, mass=0.0, respondable=True, rot_deg=(0, 0, 0)):
        options = 0
        if respondable:
            options |= 8  # Respondable
        h = sim.createPrimitiveShape(
            sim.primitiveshape_cuboid,
            [size_mm[0] * MM, size_mm[1] * MM, size_mm[2] * MM],
            options,
        )
        sim.setObjectAlias(h, name)
        sim.setObjectPosition(h, [pos_mm[0] * MM, pos_mm[1] * MM, pos_mm[2] * MM])
        sim.setObjectOrientation(h, [math.radians(a) for a in rot_deg])
        sim.setShapeColor(h, None, sim.colorcomponent_ambient_diffuse, color)
        if mass > 0.0:
            sim.setShapeMass(h, mass)
            sim.setObjectInt32Param(h, sim.shapeintparam_static, 0)
        else:
            sim.setObjectInt32Param(h, sim.shapeintparam_static, 1)
        sim.setObjectInt32Param(h, sim.shapeintparam_respondable, 1 if respondable else 0)
        return h

    def make_cyl(name, dia_mm, len_mm, pos_mm, color, mass=0.0, respondable=True, rot_deg=(0, 0, 0)):
        options = 8 if respondable else 0
        h = sim.createPrimitiveShape(
            sim.primitiveshape_cylinder,
            [dia_mm * MM, dia_mm * MM, len_mm * MM],
            options,
        )
        sim.setObjectAlias(h, name)
        sim.setObjectPosition(h, [pos_mm[0] * MM, pos_mm[1] * MM, pos_mm[2] * MM])
        sim.setObjectOrientation(h, [math.radians(a) for a in rot_deg])
        sim.setShapeColor(h, None, sim.colorcomponent_ambient_diffuse, color)
        if mass > 0.0:
            sim.setShapeMass(h, mass)
            sim.setObjectInt32Param(h, sim.shapeintparam_static, 0)
        else:
            sim.setObjectInt32Param(h, sim.shapeintparam_static, 1)
        sim.setObjectInt32Param(h, sim.shapeintparam_respondable, 1 if respondable else 0)
        return h

    # ------------------------------------------------------------
    # 1. CABINET, SLOPED BALL CHUTE & WOODEN TRAY (Static)
    # ------------------------------------------------------------
    make_box("Cabinet_BackWall", (520, 12, 260), (0, 6, 130), [0.85, 0.85, 0.82])
    # Slightly sloped chute floor (3.5° downward toward +X so balls roll naturally!)
    make_box("Ball_Chute_Floor", (440, 48, 8), (-20, -112, 16), [0.32, 0.30, 0.28], rot_deg=(0, 3.5, 0))
    make_box("Chute_BackRail", (440, 8, 36), (-20, -84, 30), [0.48, 0.34, 0.19])
    make_box("Chute_FrontRail", (440, 8, 36), (-20, -140, 30), [0.48, 0.34, 0.19])
    make_box("Tray_RightStop", (10, 64, 42), (195, -112, 24), [0.48, 0.34, 0.19])

    # Compact MG996R Enclosure (Static parent for MG996R_Servo_Joint!)
    enclosure = make_box("MG996R_Enclosure", (74, 34, 70), (-64, -18, 46), [0.08, 0.10, 0.12], respondable=False)

    # ------------------------------------------------------------
    # 2. GATE ARM (CIRCULAR Z-AXIS ROCKER PIVOT + 10mm REBAR)
    # ------------------------------------------------------------
    gate_px, gate_py = -175.0, -112.0
    rod_x, rod_y = -82.0, -112.0
    horn_z = 91.0

    # Fixed vertical hinge post (Static parent for Gate_Pivot_Joint!)
    hinge_post = make_cyl("Gate_Hinge_Post", 12, 42, (gate_px, gate_py, 148), [0.75, 0.77, 0.80], respondable=False)

    # Gate revolute joint (free-pivoting around Z-axis!)
    gate_joint = sim.createJoint(sim.joint_revolute_subtype, sim.jointmode_dynamic, 0, [0.025, 0.008])
    sim.setObjectAlias(gate_joint, "Gate_Pivot_Joint")
    sim.setObjectPosition(gate_joint, [gate_px * MM, gate_py * MM, 148.0 * MM])
    sim.setObjectParent(gate_joint, hinge_post, True)
    sim.setObjectInt32Param(gate_joint, sim.jointintparam_dynctrlmode, sim.jointdynctrl_free)

    # Dynamic gate arm + rebar rod + PETG clamp grouped into one rigid body
    arm_part = make_box("Gate_Steel_Arm", (118, 22, 22), (-125, rod_y, 148), [0.22, 0.24, 0.27], mass=0.18, respondable=False)
    rod_part = make_cyl("Gate_Rebar_Rod", 10, 128, (rod_x, rod_y, 82), [0.18, 0.20, 0.22], mass=0.08, respondable=True)
    clamp_part = make_box("PETG_Clamp", (36, 22, 24), (rod_x, rod_y, horn_z), [0.05, 0.68, 0.46], mass=0.03, respondable=False)
    clevis_part = make_box("PETG_Clevis_Tab", (14, 34, 6), (rod_x, rod_y + 17, horn_z - 1.5), [0.05, 0.68, 0.46], mass=0.01, respondable=False)

    gate_body = sim.groupShapes([rod_part, arm_part, clamp_part, clevis_part])
    sim.setObjectAlias(gate_body, "Gate_Assembly_Body")
    sim.setShapeMass(gate_body, 0.30)
    sim.setObjectInt32Param(gate_body, sim.shapeintparam_static, 0)
    sim.setObjectInt32Param(gate_body, sim.shapeintparam_respondable, 1)
    sim.setObjectParent(gate_body, gate_joint, True)

    # ------------------------------------------------------------
    # 3. MG996R SERVO MOTOR + SHORT 14mm HORN + DEAD-CENTER 4-BAR LINKAGE
    # ------------------------------------------------------------
    # High-leverage anti-burnout geometry:
    #   - Uses 14mm inner horn hole (+43% more pull force than 20mm outer hole!)
    #   - Sweeps 172° to straight-line dead-center when OPEN -> pull force passes
    #     straight through the servo shaft, dropping 4-second holding load near ZERO!
    servo_x, servo_y = -92.0, -24.0
    horn_r = 14.0
    theta_locked = math.radians(-85.0)
    theta_open = math.radians(-257.0)  # 172° sweep -> aligns horn & bar near straight-line dead-center!

    # MG996R Revolute Joint (Position-controlled, parented to static MG996R_Enclosure!)
    servo_joint = sim.createJoint(sim.joint_revolute_subtype, sim.jointmode_dynamic, 0, [0.025, 0.008])
    sim.setObjectAlias(servo_joint, "MG996R_Servo_Joint")
    sim.setObjectPosition(servo_joint, [servo_x * MM, servo_y * MM, horn_z * MM])
    sim.setObjectParent(servo_joint, enclosure, True)
    sim.setObjectInt32Param(servo_joint, sim.jointintparam_dynctrlmode, sim.jointdynctrl_position)
    sim.setJointTargetForce(servo_joint, 0.98)  # 10 kg·cm MG996R torque
    sim.setObjectFloatParam(servo_joint, sim.jointfloatparam_maxvel, math.radians(350.0))  # 60°/0.17s

    # Horn tip rest position at theta_locked
    h0_x = servo_x + horn_r * math.cos(theta_locked)
    h0_y = servo_y + horn_r * math.sin(theta_locked)
    c0_x = rod_x
    c0_y = rod_y + 31.0
    link_L = math.hypot(h0_x - c0_x, h0_y - c0_y)
    link_ang = math.degrees(math.atan2(c0_y - h0_y, c0_x - h0_x))

    # Servo Horn body (compact 20mm arm using 14mm inner pin hole)
    horn_mid_x = (servo_x + h0_x) / 2.0
    horn_mid_y = (servo_y + h0_y) / 2.0
    horn_body = make_box(
        "MG996R_Aluminum_Horn", (20, 7.5, 4),
        (horn_mid_x, horn_mid_y, horn_z),
        [0.80, 0.14, 0.16], mass=0.012, respondable=False,
        rot_deg=(0, 0, math.degrees(theta_locked)),
    )
    sim.setObjectParent(horn_body, servo_joint, True)

    # Pin Joint 1 at Servo Horn Tip
    pin_horn_joint = sim.createJoint(sim.joint_revolute_subtype, sim.jointmode_dynamic, 0, [0.015, 0.004])
    sim.setObjectAlias(pin_horn_joint, "Pin_Horn_Joint")
    sim.setObjectPosition(pin_horn_joint, [h0_x * MM, h0_y * MM, (horn_z + 4.0) * MM])
    sim.setObjectInt32Param(pin_horn_joint, sim.jointintparam_dynctrlmode, sim.jointdynctrl_free)
    sim.setObjectParent(pin_horn_joint, horn_body, True)

    # Linkage Bar body
    bar_mid_x = (h0_x + c0_x) / 2.0
    bar_mid_y = (h0_y + c0_y) / 2.0
    link_body = make_box(
        "PETG_Linkage_Bar", (link_L + 10.0, 11.0, 5.0),
        (bar_mid_x, bar_mid_y, horn_z + 4.0),
        [0.05, 0.72, 0.48], mass=0.02, respondable=False,
        rot_deg=(0, 0, link_ang),
    )
    sim.setObjectParent(link_body, pin_horn_joint, True)

    # Pin Joint 2 at Gate Clevis End (completes the 4 revolute joints of the 4-bar linkage!)
    # Without this 4th revolute joint, loop-closure welds all 6 DOFs and locks the mechanism!
    pin_clevis_joint = sim.createJoint(sim.joint_revolute_subtype, sim.jointmode_dynamic, 0, [0.015, 0.004])
    sim.setObjectAlias(pin_clevis_joint, "Pin_Clevis_Joint")
    sim.setObjectPosition(pin_clevis_joint, [c0_x * MM, c0_y * MM, (horn_z + 4.0) * MM])
    sim.setObjectInt32Param(pin_clevis_joint, sim.jointintparam_dynctrlmode, sim.jointdynctrl_free)
    sim.setObjectParent(pin_clevis_joint, link_body, True)

    # Small dynamic pin hub on the clevis joint to carry the loop-closure dummy
    clevis_pin_body = make_cyl("Clevis_Pin_Hub", 4.0, 6.0, (c0_x, c0_y, horn_z + 4.0), [0.85, 0.85, 0.88], mass=0.005, respondable=False)
    sim.setObjectParent(clevis_pin_body, pin_clevis_joint, True)

    # Loop-Closure Dummy Pair connecting Clevis_Pin_Hub <-> Gate_Assembly_Body!
    dummy_bar = sim.createDummy(0.008)
    sim.setObjectAlias(dummy_bar, "Loop_Dummy_Bar")
    sim.setObjectPosition(dummy_bar, [c0_x * MM, c0_y * MM, (horn_z + 4.0) * MM])
    sim.setObjectParent(dummy_bar, clevis_pin_body, True)

    dummy_gate = sim.createDummy(0.008)
    sim.setObjectAlias(dummy_gate, "Loop_Dummy_Gate")
    sim.setObjectPosition(dummy_gate, [c0_x * MM, c0_y * MM, (horn_z + 4.0) * MM])
    sim.setObjectParent(dummy_gate, gate_body, True)

    sim.setLinkDummy(dummy_bar, dummy_gate)
    sim.setObjectInt32Param(dummy_bar, sim.dummyintparam_link_type, sim.dummy_linktype_dynamics_loop_closure)

    # ------------------------------------------------------------
    # 4. 6 DYNAMIC SNOOKER BALLS IN THE CHUTE
    # ------------------------------------------------------------
    ball_colors = [
        [0.82, 0.05, 0.05], [0.78, 0.06, 0.06], [0.95, 0.80, 0.05],
        [0.05, 0.55, 0.18], [0.06, 0.25, 0.85], [0.06, 0.06, 0.08],
    ]
    for i, col in enumerate(ball_colors):
        bx = -105.0 - i * 30.0
        bz = 36.0 + i * 1.85  # Follow 3.5° chute slope
        b = sim.createPrimitiveShape(sim.primitiveshape_spheroid, [27.0 * MM, 27.0 * MM, 27.0 * MM], 8)
        sim.setObjectAlias(b, f"Snooker_Ball_{i+1}")
        sim.setObjectPosition(b, [bx * MM, -112.0 * MM, bz * MM])
        sim.setShapeColor(b, None, sim.colorcomponent_ambient_diffuse, col)
        sim.setShapeMass(b, 0.14)
        sim.setObjectInt32Param(b, sim.shapeintparam_static, 0)
        sim.setObjectInt32Param(b, sim.shapeintparam_respondable, 1)

    # ------------------------------------------------------------
    # 5. RUN REAL-TIME PHYSICS SIMULATION (LOCK -> OPEN -> RELEASE -> CLOSE)
    # ------------------------------------------------------------
    # Save reusable CoppeliaSim scene (.ttt)
    ttt_path = "/Users/user/Documents/Projects/arduino/snookeropener/3D_Enclosure/Prototype/snooker_mechanism.ttt"
    sim.saveScene(ttt_path)
    print(f"Scene saved to {ttt_path}")
    print(f"4-Bar Linkage Length = {link_L:.2f} mm | Starting real-time physics simulation...")
    sim.setBoolParam(sim.boolparam_realtime_simulation, True)
    sim.startSimulation()

    try:
        t0 = sim.getSimulationTime()
        while sim.getSimulationTime() - t0 < 8.0:
            t = sim.getSimulationTime() - t0
            if t < 1.5:
                target_rad = 0.0
                phase = "LOCKED (holding balls)"
            elif t < 5.5:
                target_rad = theta_open - theta_locked  # -140° clockwise pull
                phase = "OPEN   (balls rolling)"
            else:
                target_rad = 0.0
                phase = "CLOSING"

            sim.setJointTargetPosition(servo_joint, target_rad)
            servo_ang = math.degrees(sim.getJointPosition(servo_joint))
            gate_ang = math.degrees(sim.getJointPosition(gate_joint))
            torque_nm = abs(sim.getJointForce(servo_joint))
            torque_kgcm = torque_nm * 10.197

            print(f"t={t:4.2f}s | {phase:22s} | Servo={servo_ang:6.1f}° | GateArc={gate_ang:5.1f}° | Load={torque_kgcm:4.2f} kg·cm", end="\r")
            time.sleep(0.05)
        print("\nSimulation cycle complete!")
    finally:
        sim.stopSimulation()


if __name__ == "__main__":
    main()
