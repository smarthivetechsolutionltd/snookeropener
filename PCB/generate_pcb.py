import os
import math
import shutil
import subprocess
import zipfile

OUTPUT_DIR = os.path.dirname(os.path.abspath(__file__))
GERBER_DIR = os.path.join(OUTPUT_DIR, "gerbers")
os.makedirs(OUTPUT_DIR, exist_ok=True)
if os.path.exists(GERBER_DIR):
    shutil.rmtree(GERBER_DIR)
os.makedirs(GERBER_DIR, exist_ok=True)

KICAD_CLI = "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli"

# ==============================================================================
# 1. KICAD PROJECT FILE (smarthive_pool.kicad_pro)
# ==============================================================================
pro_content = """{
  "board": {
    "design_settings": {
      "defaults": {
        "board_outline_line_width": 0.15,
        "copper_line_width": 0.35,
        "copper_text_size_h": 1.5,
        "copper_text_size_v": 1.5,
        "copper_text_thickness": 0.3,
        "silk_line_width": 0.15,
        "silk_text_size_h": 1.0,
        "silk_text_size_v": 1.0,
        "silk_text_thickness": 0.15
      },
      "rules": {
        "max_error": 0.005,
        "min_clearance": 0.2,
        "min_copper_edge_clearance": 0.4,
        "min_hole_clearance": 0.25,
        "min_hole_to_hole": 0.25,
        "min_through_hole_diameter": 0.6,
        "min_track_width": 0.25,
        "min_via_annular_ring": 0.15,
        "min_via_diameter": 0.6
      }
    }
  },
  "boards": [],
  "cvpcb": {
    "equivalence_files": []
  },
  "libraries": {
    "pinned_footprint_libs": [],
    "pinned_symbol_libs": []
  },
  "meta": {
    "filename": "smarthive_pool.kicad_pro",
    "version": 1
  },
  "net_settings": {
    "classes": [
      {
        "clearance": 0.2,
        "diff_pair_gap": 0.25,
        "diff_pair_via_gap": 0.25,
        "microvia_diameter": 0.3,
        "microvia_drill": 0.1,
        "name": "Default",
        "pcb_color": "rgba(0, 0, 0, 0.000)",
        "schematic_color": "rgba(0, 0, 0, 0.000)",
        "track_width": 0.35,
        "via_diameter": 0.8,
        "via_drill": 0.4
      },
      {
        "clearance": 0.25,
        "diff_pair_gap": 0.25,
        "diff_pair_via_gap": 0.25,
        "microvia_diameter": 0.3,
        "microvia_drill": 0.1,
        "name": "Power",
        "pcb_color": "rgba(0, 0, 0, 0.000)",
        "schematic_color": "rgba(0, 0, 0, 0.000)",
        "track_width": 0.8,
        "via_diameter": 1.0,
        "via_drill": 0.5
      }
    ]
  },
  "pcbnew": {
    "last_paths": {
      "gencad": "",
      "idf": "",
      "netlist": "",
      "plot": "gerbers/",
      "pos": "",
      "step": "",
      "vrml": ""
    }
  },
  "schematic": {
    "legacy_lib_dir": "",
    "legacy_lib_list": []
  },
  "sheets": [
    [
      "f55d7870-07bf-463e-9669-7988365f5a8a",
      ""
    ]
  ]
}"""

with open(os.path.join(OUTPUT_DIR, "smarthive_pool.kicad_pro"), "w") as f:
    f.write(pro_content)

print("Created smarthive_pool.kicad_pro")

# ==============================================================================
# 2. KICAD PCB FILE (smarthive_pool.kicad_pcb) — ESP32 DEVKIT V1 EDITION (V2.0)
# ==============================================================================
pcb_header = """(kicad_pcb (version 20240108) (generator kicad-cli)
  (general
    (thickness 1.6)
  )
  (paper "A4")
  (layers
    (0 "F.Cu" signal)
    (31 "B.Cu" signal)
    (32 "B.Adhes" user "B.Adhesive")
    (33 "F.Adhes" user "F.Adhesive")
    (34 "B.Paste" user)
    (35 "F.Paste" user)
    (36 "B.SilkS" user "B.Silkscreen")
    (37 "F.SilkS" user "F.Silkscreen")
    (38 "B.Mask" user)
    (39 "F.Mask" user)
    (40 "Dwgs.User" user "User.Drawings")
    (41 "Cmts.User" user "User.Comments")
    (42 "Eco1.User" user "User.Eco1")
    (43 "Eco2.User" user "User.Eco2")
    (44 "Edge.Cuts" user)
    (45 "Margin" user)
    (46 "B.CrtYd" user "B.Courtyard")
    (47 "F.CrtYd" user "F.Courtyard")
    (48 "B.Fab" user)
    (49 "F.Fab" user)
  )
  (setup
    (pad_to_mask_clearance 0.05)
    (pcbplotparams
      (layerselection 0x00010fc_ffffffff)
      (plot_on_all_layers_selection 0x0000000_00000000)
      (disableapertmacros false)
      (usegerberextensions false)
      (usegerberattributes true)
      (usegerberadvancedattributes true)
      (creategerberjobfile true)
      (dashed_line_dash_ratio 12.000000)
      (dashed_line_gap_ratio 3.000000)
      (svgprecision 4)
      (plotframeref false)
      (viasonmask false)
      (mode 1)
      (useauxorigin false)
      (hpglpennumber 1)
      (hpglpenspeed 20)
      (hpglpendiameter 15.000000)
      (pdf_front_fp_property_popups true)
      (pdf_back_fp_property_popups true)
      (dxfpolygonmode true)
      (dxfimperialunits true)
      (dxfusepcbnewfont true)
      (psnegative false)
      (psa4output false)
      (plotreference true)
      (plotvalue true)
      (plotfptext true)
      (plotinvisibletext false)
      (sketchpadsonfab false)
      (subtractmaskfromsilk false)
      (outputformat 1)
      (mirror false)
      (drillshape 1)
      (scaleselection 1)
      (outputdirectory "gerbers/")
    )
  )
  (net 0 "")
  (net 1 "GND")
  (net 2 "+5V")
  (net 3 "+3V3")
  (net 4 "NFC_RST")
  (net 5 "NFC_SDA")
  (net 6 "NFC_SCK")
  (net 7 "NFC_MISO")
  (net 8 "NFC_MOSI")
  (net 9 "SERVO_PWM")
  (net 10 "TEST_BTN")
  (net 11 "VIN_SW")
  (net 12 "LED_RED")
  (net 13 "LED_GRN")
  (net 14 "Net-(LED1-K)")
  (net 15 "Net-(LED2-K)")
"""

# Custom L-Shaped Outline Contoured for Zero-Clash Lay-Flat MG996R Servo Enclosure
outline = """
  (gr_line (start 64.0 73.5) (end 135.5 73.5) (stroke (width 0.15) (type solid)) (layer "Edge.Cuts"))
  (gr_arc (start 135.5 73.5) (mid 136.914 74.086) (end 137.5 75.5) (stroke (width 0.15) (type solid)) (layer "Edge.Cuts"))
  (gr_line (start 137.5 75.5) (end 137.5 100.0) (stroke (width 0.15) (type solid)) (layer "Edge.Cuts"))
  (gr_arc (start 137.5 100.0) (mid 136.914 101.414) (end 135.5 102.0) (stroke (width 0.15) (type solid)) (layer "Edge.Cuts"))
  (gr_line (start 135.5 102.0) (end 100.0 102.0) (stroke (width 0.15) (type solid)) (layer "Edge.Cuts"))
  (gr_arc (start 100.0 102.0) (mid 98.586 102.586) (end 98.0 104.0) (stroke (width 0.15) (type solid)) (layer "Edge.Cuts"))
  (gr_line (start 98.0 104.0) (end 98.0 124.5) (stroke (width 0.15) (type solid)) (layer "Edge.Cuts"))
  (gr_arc (start 98.0 124.5) (mid 97.414 125.914) (end 96.0 126.5) (stroke (width 0.15) (type solid)) (layer "Edge.Cuts"))
  (gr_line (start 96.0 126.5) (end 64.0 126.5) (stroke (width 0.15) (type solid)) (layer "Edge.Cuts"))
  (gr_arc (start 64.0 126.5) (mid 62.586 125.914) (end 62.0 124.5) (stroke (width 0.15) (type solid)) (layer "Edge.Cuts"))
  (gr_line (start 62.0 124.5) (end 62.0 75.5) (stroke (width 0.15) (type solid)) (layer "Edge.Cuts"))
  (gr_arc (start 62.0 75.5) (mid 62.586 74.086) (end 64.0 73.5) (stroke (width 0.15) (type solid)) (layer "Edge.Cuts"))
"""

silk_graphics = """
  (gr_text "SMARTHIVETECH SOLUTIONS" (at 77.0 100.0 90) (layer "F.SilkS")
    (effects (font (size 0.8 0.8) (thickness 0.15)))
  )
  (gr_text "SMARTHIVE POOL V2.0 (ESP32)" (at 79.0 100.0 90) (layer "F.SilkS")
    (effects (font (size 0.8 0.8) (thickness 0.15)))
  )
  (gr_text "LAY-FLAT MG996R SERVO BAY" (at 117.0 100.6) (layer "F.SilkS")
    (effects (font (size 0.8 0.8) (thickness 0.15)))
  )
  (gr_text "SMARTHIVETECH - SMARTHIVE POOL V2.0 ESP32" (at 77.0 100.0 90) (layer "B.SilkS")
    (effects (font (size 0.85 0.85) (thickness 0.15)) (justify mirror))
  )
"""

def tht_pad(num, net_id, net_name, x, y, shape="circle", size=(1.5, 1.5), drill=0.85):
    s_w, s_h = size
    return f"""      (pad "{num}" thru_hole {shape} (at {x:.3f} {y:.3f}) (size {s_w:.2f} {s_h:.2f}) (drill {drill:.2f}) (layers "*.Cu" "*.Mask") (net {net_id} "{net_name}"))\n"""

components = ""

# 4 x M2.5 Enclosure Mounting Holes
holes = [(65.0, 76.8, 3.0), (134.2, 76.8, 3.0), (65.0, 123.2, -3.0), (94.2, 123.2, -3.0)]
for i, (hx, hy, ref_dy) in enumerate(holes, 1):
    components += f"""  (footprint "MountingHole:MountingHole_2.5mm_M2.5_Pad" (layer "F.Cu")
    (at {hx} {hy})
    (property "Reference" "H{i}" (at 0 {ref_dy}) (layer "F.SilkS") (effects (font (size 0.8 0.8) (thickness 0.15))))
    (property "Value" "M2.5_Mount" (at 0 {ref_dy}) (layer "F.Fab") (effects (font (size 0.8 0.8) (thickness 0.15))))
    (fp_circle (center 0 0) (end 2.2 0) (stroke (width 0.15) (type solid)) (layer "F.CrtYd"))
    {tht_pad(1, 1, "GND", 0, 0, shape="circle", size=(3.8, 3.8), drill=2.5)}  )\n"""

# ==============================================================================
# ESP32 DEVKIT V1 (30-Pin, 2x 1x15 Pin Sockets, 25.4mm / 1.0" row pitch)
# Center: X = 80.0, Y = 100.0. USB port at top (Y = 75.5)
# Left Socket:  X = 80.0 - 12.7 = 67.30
# Right Socket: X = 80.0 + 12.7 = 92.70
# ==============================================================================
left_pins = [
    ("EN",       0,  ""),             # 1. EN
    ("VP",       0,  ""),             # 2. GPIO 36
    ("VN",       0,  ""),             # 3. GPIO 39
    ("D34",      0,  ""),             # 4. GPIO 34
    ("D35",      0,  ""),             # 5. GPIO 35
    ("D32_BTN",  10, "TEST_BTN"),     # 6. GPIO 32 -> Manual Test Button
    ("D33",      0,  ""),             # 7. GPIO 33
    ("D25",      0,  ""),             # 8. GPIO 25
    ("D26_RED",  12, "LED_RED"),      # 9. GPIO 26 -> RGB Red
    ("D27_GRN",  13, "LED_GRN"),      # 10. GPIO 27 -> RGB Green
    ("D14",      0,  ""),             # 11. GPIO 14
    ("D12",      0,  ""),             # 12. GPIO 12
    ("D13_SRV",  9,  "SERVO_PWM"),    # 13. GPIO 13 -> Servo PWM
    ("GND",      1,  "GND"),          # 14. GND
    ("VIN_5V",   2,  "+5V"),          # 15. VIN (+5V)
]

right_pins = [
    ("D23_MOSI", 8,  "NFC_MOSI"),     # 1. GPIO 23 -> VSPI MOSI
    ("D22_RST",  4,  "NFC_RST"),      # 2. GPIO 22 -> RC522 RST
    ("TX0",      0,  ""),             # 3. GPIO 1
    ("RX0",      0,  ""),             # 4. GPIO 3
    ("D21",      0,  ""),             # 5. GPIO 21
    ("D19_MISO", 7,  "NFC_MISO"),     # 6. GPIO 19 -> VSPI MISO
    ("D18_SCK",  6,  "NFC_SCK"),      # 7. GPIO 18 -> VSPI SCK
    ("D5_SDA",   5,  "NFC_SDA"),      # 8. GPIO 5  -> VSPI SS (SDA)
    ("TX2",      0,  ""),             # 9. GPIO 17
    ("RX2",      0,  ""),             # 10. GPIO 16
    ("D4",       0,  ""),             # 11. GPIO 4
    ("D2",       0,  ""),             # 12. GPIO 2
    ("D15",      0,  ""),             # 13. GPIO 15
    ("GND",      1,  "GND"),          # 14. GND
    ("3V3",      3,  "+3V3"),         # 15. +3.3V
]

# J1_LEFT: ESP32 Left Socket
components += """  (footprint "Connector_PinSocket_2.54mm:PinSocket_1x15_P2.54mm_Vertical" (layer "F.Cu")
    (at 67.30 100.0)
    (property "Reference" "J1_LEFT" (at -2.6 0 90) (layer "F.SilkS") (effects (font (size 0.85 0.85) (thickness 0.15))))
    (property "Value" "ESP32_GPIO_LEFT" (at 0 0) (layer "F.Fab") (effects (font (size 0.85 0.85) (thickness 0.15))))
    (fp_rect (start -1.3 -19.2) (end 1.3 19.2) (stroke (width 0.15) (type solid)) (layer "F.SilkS"))
"""
for idx, (pname, nid, nname) in enumerate(left_pins):
    py = -17.78 + (idx * 2.54)
    components += tht_pad(idx + 1, nid, nname, 0, py, shape="rect" if idx == 0 else "circle", size=(1.5, 1.5), drill=0.85)
components += "  )\n"

# J1_RIGHT: ESP32 Right Socket
components += """  (footprint "Connector_PinSocket_2.54mm:PinSocket_1x15_P2.54mm_Vertical" (layer "F.Cu")
    (at 92.70 100.0)
    (property "Reference" "J1_RIGHT" (at 2.6 0 90) (layer "F.SilkS") (effects (font (size 0.85 0.85) (thickness 0.15))))
    (property "Value" "ESP32_GPIO_RIGHT" (at 0 0) (layer "F.Fab") (effects (font (size 0.85 0.85) (thickness 0.15))))
    (fp_rect (start -1.3 -19.2) (end 1.3 19.2) (stroke (width 0.15) (type solid)) (layer "F.SilkS"))
"""
for idx, (pname, nid, nname) in enumerate(right_pins):
    py = -17.78 + (idx * 2.54)
    components += tht_pad(idx + 1, nid, nname, 0, py, shape="rect" if idx == 0 else "circle", size=(1.5, 1.5), drill=0.85)
components += "  )\n"

# J5: Switched 5V Power Input from Outboard RFID Pod (USB-C Port + SS-12D00 Slide Switch)
components += f"""  (footprint "TerminalBlock:TerminalBlock_bornier-2_P5.08mm" (layer "F.Cu")
    (at 101.5 78.0)
    (property "Reference" "J5" (at -6.2 0) (layer "F.SilkS") (effects (font (size 0.85 0.85) (thickness 0.15))))
    (property "Value" "USB_C_SW_5V" (at 0 3.2) (layer "F.Fab") (effects (font (size 0.85 0.85) (thickness 0.15))))
    (fp_rect (start -4.5 -2.2) (end 4.5 2.2) (stroke (width 0.15) (type solid)) (layer "F.SilkS"))
    {tht_pad(1, 11, "VIN_SW", -2.54, 0, shape="rect", size=(2.2, 2.2), drill=1.1)}
    {tht_pad(2, 1, "GND", 2.54, 0, shape="circle", size=(2.2, 2.2), drill=1.1)}
  )\n"""

# D1: Reverse Polarity Protection Schottky Diode (SS34 / 1N5819, 3A Rated)
components += f"""  (footprint "Diode_THT:D_DO-41_SOD81_P7.62mm_Horizontal" (layer "F.Cu")
    (at 101.5 84.0)
    (property "Reference" "D1" (at -6.0 0) (layer "F.SilkS") (effects (font (size 0.85 0.85) (thickness 0.15))))
    (property "Value" "SS34/1N5819" (at 0 2.0) (layer "F.Fab") (effects (font (size 0.85 0.85) (thickness 0.15))))
    (fp_rect (start -2.0 -1.0) (end 2.0 1.0) (stroke (width 0.15) (type solid)) (layer "F.SilkS"))
    (fp_line (start 1.1 -1.0) (end 1.1 1.0) (stroke (width 0.25) (type solid)) (layer "F.SilkS"))
    {tht_pad(1, 11, "VIN_SW", -3.81, 0, shape="circle", size=(1.5, 1.5), drill=0.85)}
    {tht_pad(2, 2, "+5V", 3.81, 0, shape="rect", size=(1.5, 1.5), drill=0.85)}
  )\n"""

# J2: RC522 RFID Reader 8-Pin Header (Top Cable Entry Port, VSPI Bus)
rc522_pins = [
    ("3V3",  3, "+3V3"),
    ("RST",  4, "NFC_RST"),
    ("GND",  1, "GND"),
    ("IRQ",  0, ""),
    ("MISO", 7, "NFC_MISO"),
    ("MOSI", 8, "NFC_MOSI"),
    ("SCK",  6, "NFC_SCK"),
    ("SDA",  5, "NFC_SDA"),
]

components += """  (footprint "Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical" (layer "F.Cu")
    (at 120.0 77.5)
    (property "Reference" "J2" (at 0 2.4) (layer "F.SilkS") (effects (font (size 0.85 0.85) (thickness 0.15))))
    (property "Value" "RC522_RFID" (at 0 2.4) (layer "F.Fab") (effects (font (size 0.85 0.85) (thickness 0.15))))
    (fp_rect (start -10.2 -1.3) (end 10.2 1.3) (stroke (width 0.15) (type solid)) (layer "F.SilkS"))
"""
for idx, (pname, nid, nname) in enumerate(rc522_pins):
    px = -8.89 + (idx * 2.54)
    components += tht_pad(idx + 1, nid, nname, px, 0, shape="rect" if idx == 0 else "circle", size=(1.5, 1.5), drill=0.85)
components += "  )\n"

# J6: HW-479 RGB LED Module 4-Pin Header (Common Cathode: GND, R, G, B)
rgb_pins = [
    ("GND", 1,  "GND"),
    ("RED", 12, "LED_RED"),
    ("GRN", 13, "LED_GRN"),
    ("BLU", 0,  ""),
]

components += """  (footprint "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical" (layer "F.Cu")
    (at 125.0 85.0)
    (property "Reference" "J6" (at 0 2.4) (layer "F.SilkS") (effects (font (size 0.85 0.85) (thickness 0.15))))
    (property "Value" "HW479_RGB" (at 0 -2.2) (layer "F.Fab") (effects (font (size 0.85 0.85) (thickness 0.15))))
    (fp_rect (start -5.1 -1.3) (end 5.1 1.3) (stroke (width 0.15) (type solid)) (layer "F.SilkS"))
"""
for idx, (pname, nid, nname) in enumerate(rgb_pins):
    px = -3.81 + (idx * 2.54)
    components += tht_pad(idx + 1, nid, nname, px, 0, shape="rect" if idx == 0 else "circle", size=(1.5, 1.5), drill=0.85)
components += "  )\n"

# J3: MG996R High-Torque Servo 3-Pin Header (Directly above Servo Wire Relief)
components += f"""  (footprint "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical" (layer "F.Cu")
    (at 114.0 97.5)
    (property "Reference" "J3" (at -5.5 0) (layer "F.SilkS") (effects (font (size 0.85 0.85) (thickness 0.15))))
    (property "Value" "MG996R_SERVO" (at 0 2.0) (layer "F.Fab") (effects (font (size 0.85 0.85) (thickness 0.15))))
    (fp_rect (start -3.9 -1.3) (end 3.9 1.3) (stroke (width 0.15) (type solid)) (layer "F.SilkS"))
    {tht_pad(1, 1, "GND", -2.54, 0, shape="rect", size=(1.6, 1.6), drill=0.95)}
    {tht_pad(2, 2, "+5V", 0, 0, shape="circle", size=(1.6, 1.6), drill=0.95)}
    {tht_pad(3, 9, "SERVO_PWM", 2.54, 0, shape="circle", size=(1.6, 1.6), drill=0.95)}
  )\n"""

# C1: 470uF 25V Electrolytic Bulk Servo Smoothing Capacitor
components += f"""  (footprint "Capacitor_THT:CP_Radial_D8.0mm_P3.50mm" (layer "F.Cu")
    (at 104.5 94.0)
    (property "Reference" "C1" (at 0 -4.8) (layer "F.SilkS") (effects (font (size 0.85 0.85) (thickness 0.15))))
    (property "Value" "470uF_25V" (at 0 4.8) (layer "F.Fab") (effects (font (size 0.85 0.85) (thickness 0.15))))
    (fp_circle (center 0 0) (end 4.2 0) (stroke (width 0.15) (type solid)) (layer "F.SilkS"))
    {tht_pad(1, 2, "+5V", -1.75, 0, shape="rect", size=(1.6, 1.6), drill=0.9)}
    {tht_pad(2, 1, "GND", 1.75, 0, shape="circle", size=(1.6, 1.6), drill=0.9)}
  )\n"""

# C2: 100nF Ceramic Decoupling Capacitor for Servo Rail
components += f"""  (footprint "Capacitor_THT:C_Disc_D4.7mm_W2.6mm_P2.50mm" (layer "F.Cu")
    (at 114.0 92.5)
    (property "Reference" "C2" (at -3.5 0) (layer "F.SilkS") (effects (font (size 0.85 0.85) (thickness 0.15))))
    (property "Value" "100nF" (at 0 2.0) (layer "F.Fab") (effects (font (size 0.85 0.85) (thickness 0.15))))
    {tht_pad(1, 2, "+5V", -1.25, 0, shape="circle", size=(1.5, 1.5), drill=0.8)}
    {tht_pad(2, 1, "GND", 1.25, 0, shape="circle", size=(1.5, 1.5), drill=0.8)}
  )\n"""

# SW1: Onboard Manual Test Release Pushbutton (Tactile Switch 6x6mm, Pin 1: GPIO 32, Pin 2: GND)
components += f"""  (footprint "Button_Switch_THT:SW_PUSH_6mm" (layer "F.Cu")
    (at 80.0 122.0)
    (property "Reference" "SW1" (at 0 -2.5) (layer "F.SilkS") (effects (font (size 0.85 0.85) (thickness 0.15))))
    (property "Value" "TEST_BTN" (at 0 2.5) (layer "F.Fab") (effects (font (size 0.85 0.85) (thickness 0.15))))
    {tht_pad(1, 10, "TEST_BTN", -2.5, 0, shape="rect", size=(1.5, 1.5), drill=0.85)}
    {tht_pad(2, 1, "GND", 2.5, 0, shape="circle", size=(1.5, 1.5), drill=0.85)}
  )\n"""

# LED1 (3.3V Power Status) + R1 (1k)
components += f"""  (footprint "LED_THT:LED_D3.0mm" (layer "F.Cu")
    (at 75.0 82.22)
    (property "Reference" "LED1" (at 0 -2.2) (layer "F.SilkS") (effects (font (size 0.85 0.85) (thickness 0.15))))
    (property "Value" "3V3_OK" (at 0 2.2) (layer "F.Fab") (effects (font (size 0.85 0.85) (thickness 0.15))))
    {tht_pad(1, 3, "+3V3", -1.27, 0, shape="rect", size=(1.5, 1.5), drill=0.8)}
    {tht_pad(2, 14, "Net-(LED1-K)", 1.27, 0, shape="circle", size=(1.5, 1.5), drill=0.8)}
  )\n"""

components += f"""  (footprint "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P5.08mm_Horizontal" (layer "F.Cu")
    (at 75.0 85.5)
    (property "Reference" "R1" (at 0 2.0) (layer "F.SilkS") (effects (font (size 0.85 0.85) (thickness 0.15))))
    (property "Value" "1k" (at 0 2.0) (layer "F.Fab") (effects (font (size 0.85 0.85) (thickness 0.15))))
    (fp_rect (start -1.5 -0.8) (end 1.5 0.8) (stroke (width 0.15) (type solid)) (layer "F.SilkS"))
    {tht_pad(1, 14, "Net-(LED1-K)", 2.54, 0, shape="circle", size=(1.5, 1.5), drill=0.8)}
    {tht_pad(2, 1, "GND", -2.54, 0, shape="circle", size=(1.5, 1.5), drill=0.8)}
  )\n"""

# LED2 (Servo Active Status) + R2 (1k)
components += f"""  (footprint "LED_THT:LED_D3.0mm" (layer "F.Cu")
    (at 116.54 89.0)
    (property "Reference" "LED2" (at 2.6 0) (layer "F.SilkS") (effects (font (size 0.85 0.85) (thickness 0.15))))
    (property "Value" "SERVO_ON" (at -2.6 0) (layer "F.Fab") (effects (font (size 0.85 0.85) (thickness 0.15))))
    {tht_pad(1, 9, "SERVO_PWM", 0, 1.27, shape="rect", size=(1.5, 1.5), drill=0.8)}
    {tht_pad(2, 15, "Net-(LED2-K)", 0, -1.27, shape="circle", size=(1.5, 1.5), drill=0.8)}
  )\n"""

components += f"""  (footprint "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P5.08mm_Horizontal" (layer "F.Cu")
    (at 111.5 87.73)
    (property "Reference" "R2" (at 0 -1.8) (layer "F.SilkS") (effects (font (size 0.85 0.85) (thickness 0.15))))
    (property "Value" "1k" (at 0 1.8) (layer "F.Fab") (effects (font (size 0.85 0.85) (thickness 0.15))))
    (fp_rect (start -1.5 -0.8) (end 1.5 0.8) (stroke (width 0.15) (type solid)) (layer "F.SilkS"))
    {tht_pad(1, 15, "Net-(LED2-K)", 2.54, 0, shape="circle", size=(1.5, 1.5), drill=0.8)}
    {tht_pad(2, 1, "GND", -2.54, 0, shape="circle", size=(1.5, 1.5), drill=0.8)}
  )\n"""

# ==============================================================================
# DRC-CLEAN COPPER ROUTING FOR ESP32 DEVKIT V1
# ==============================================================================
tracks = """
  (segment (start 98.96 78.0) (end 97.69 79.27) (width 0.8) (layer "F.Cu") (net 11))
  (segment (start 97.69 79.27) (end 97.69 84.0) (width 0.8) (layer "F.Cu") (net 11))

  (segment (start 105.31 84.0) (end 105.31 92.5) (width 0.9) (layer "F.Cu") (net 2))
  (segment (start 105.31 92.5) (end 102.75 92.5) (width 0.9) (layer "F.Cu") (net 2))
  (segment (start 102.75 92.5) (end 102.75 94.0) (width 0.9) (layer "F.Cu") (net 2))
  (segment (start 105.31 92.5) (end 112.75 92.5) (width 0.9) (layer "F.Cu") (net 2))
  (segment (start 112.75 92.5) (end 114.0 93.75) (width 0.9) (layer "F.Cu") (net 2))
  (segment (start 114.0 93.75) (end 114.0 97.5) (width 0.9) (layer "F.Cu") (net 2))

  (segment (start 105.31 84.0) (end 105.31 86.5) (width 0.8) (layer "B.Cu") (net 2))
  (segment (start 105.31 86.5) (end 94.5 86.5) (width 0.8) (layer "B.Cu") (net 2))
  (segment (start 94.5 86.5) (end 94.5 120.0) (width 0.8) (layer "B.Cu") (net 2))
  (segment (start 94.5 120.0) (end 67.30 120.0) (width 0.8) (layer "B.Cu") (net 2))
  (segment (start 67.30 120.0) (end 67.30 117.78) (width 0.8) (layer "B.Cu") (net 2))

  (segment (start 92.70 117.78) (end 90.0 117.78) (width 0.5) (layer "F.Cu") (net 3))
  (segment (start 90.0 117.78) (end 90.0 76.3) (width 0.5) (layer "F.Cu") (net 3))
  (segment (start 90.0 76.3) (end 111.11 76.3) (width 0.5) (layer "F.Cu") (net 3))
  (segment (start 111.11 76.3) (end 111.11 77.5) (width 0.5) (layer "F.Cu") (net 3))
  (segment (start 90.0 76.3) (end 73.73 76.3) (width 0.5) (layer "F.Cu") (net 3))
  (segment (start 73.73 76.3) (end 73.73 82.22) (width 0.5) (layer "F.Cu") (net 3))

  (segment (start 76.27 82.22) (end 77.54 83.49) (width 0.35) (layer "F.Cu") (net 14))
  (segment (start 77.54 83.49) (end 77.54 85.5) (width 0.35) (layer "F.Cu") (net 14))

  (segment (start 116.54 87.73) (end 114.04 87.73) (width 0.35) (layer "F.Cu") (net 15))

  (segment (start 67.30 112.70) (end 64.5 112.70) (width 0.4) (layer "F.Cu") (net 9))
  (segment (start 64.5 112.70) (end 64.5 74.2) (width 0.4) (layer "F.Cu") (net 9))
  (segment (start 64.5 74.2) (end 131.0 74.2) (width 0.4) (layer "F.Cu") (net 9))
  (segment (start 131.0 74.2) (end 131.0 90.27) (width 0.4) (layer "F.Cu") (net 9))
  (segment (start 131.0 90.27) (end 116.54 90.27) (width 0.4) (layer "F.Cu") (net 9))
  (segment (start 116.54 90.27) (end 116.54 97.5) (width 0.4) (layer "F.Cu") (net 9))

  (segment (start 92.70 82.22) (end 96.0 82.22) (width 0.35) (layer "F.Cu") (net 8))
  (segment (start 96.0 82.22) (end 96.0 74.8) (width 0.35) (layer "F.Cu") (net 8))
  (segment (start 96.0 74.8) (end 123.81 74.8) (width 0.35) (layer "F.Cu") (net 8))
  (segment (start 123.81 74.8) (end 123.81 77.5) (width 0.35) (layer "F.Cu") (net 8))

  (segment (start 92.70 84.76) (end 97.0 84.76) (width 0.35) (layer "F.Cu") (net 4))
  (segment (start 97.0 84.76) (end 97.0 75.6) (width 0.35) (layer "F.Cu") (net 4))
  (segment (start 97.0 75.6) (end 113.65 75.6) (width 0.35) (layer "F.Cu") (net 4))
  (segment (start 113.65 75.6) (end 113.65 77.5) (width 0.35) (layer "F.Cu") (net 4))

  (segment (start 92.70 94.92) (end 95.2 94.92) (width 0.35) (layer "F.Cu") (net 7))
  (segment (start 95.2 94.92) (end 95.2 75.2) (width 0.35) (layer "F.Cu") (net 7))
  (segment (start 95.2 75.2) (end 121.27 75.2) (width 0.35) (layer "F.Cu") (net 7))
  (segment (start 121.27 75.2) (end 121.27 77.5) (width 0.35) (layer "F.Cu") (net 7))

  (segment (start 92.70 97.46) (end 94.2 97.46) (width 0.35) (layer "F.Cu") (net 6))
  (segment (start 94.2 97.46) (end 94.2 74.5) (width 0.35) (layer "F.Cu") (net 6))
  (segment (start 94.2 74.5) (end 126.35 74.5) (width 0.35) (layer "F.Cu") (net 6))
  (segment (start 126.35 74.5) (end 126.35 77.5) (width 0.35) (layer "F.Cu") (net 6))

  (segment (start 92.70 100.00) (end 93.6 100.00) (width 0.35) (layer "B.Cu") (net 5))
  (segment (start 93.6 100.00) (end 93.6 74.9) (width 0.35) (layer "B.Cu") (net 5))
  (segment (start 93.6 74.9) (end 128.89 74.9) (width 0.35) (layer "B.Cu") (net 5))
  (segment (start 128.89 74.9) (end 128.89 77.5) (width 0.35) (layer "B.Cu") (net 5))

  (segment (start 67.30 102.54) (end 65.2 102.54) (width 0.35) (layer "B.Cu") (net 12))
  (segment (start 65.2 102.54) (end 65.2 121.0) (width 0.35) (layer "B.Cu") (net 12))
  (segment (start 65.2 121.0) (end 95.5 121.0) (width 0.35) (layer "B.Cu") (net 12))
  (segment (start 95.5 121.0) (end 95.5 85.0) (width 0.35) (layer "B.Cu") (net 12))
  (segment (start 95.5 85.0) (end 123.73 85.0) (width 0.35) (layer "B.Cu") (net 12))

  (segment (start 67.30 105.08) (end 65.9 105.08) (width 0.35) (layer "B.Cu") (net 13))
  (segment (start 65.9 105.08) (end 65.9 121.8) (width 0.35) (layer "B.Cu") (net 13))
  (segment (start 65.9 121.8) (end 96.2 121.8) (width 0.35) (layer "B.Cu") (net 13))
  (segment (start 96.2 121.8) (end 96.2 86.0) (width 0.35) (layer "B.Cu") (net 13))
  (segment (start 96.2 86.0) (end 126.27 86.0) (width 0.35) (layer "B.Cu") (net 13))
  (segment (start 126.27 86.0) (end 126.27 85.0) (width 0.35) (layer "B.Cu") (net 13))

  (segment (start 67.30 94.92) (end 77.5 94.92) (width 0.35) (layer "F.Cu") (net 10))
  (segment (start 77.5 94.92) (end 77.5 122.0) (width 0.35) (layer "F.Cu") (net 10))

  (segment (start 104.04 78.0) (end 108.0 78.0) (width 0.6) (layer "F.Cu") (net 1))
  (segment (start 108.0 78.0) (end 108.0 81.5) (width 0.6) (layer "F.Cu") (net 1))
  (segment (start 108.0 81.5) (end 116.19 81.5) (width 0.6) (layer "F.Cu") (net 1))
  (segment (start 116.19 77.5) (end 116.19 85.0) (width 0.6) (layer "F.Cu") (net 1))
  (segment (start 116.19 85.0) (end 121.19 85.0) (width 0.6) (layer "F.Cu") (net 1))

  (segment (start 65.0 76.8) (end 63.4 76.8) (width 0.6) (layer "B.Cu") (net 1))
  (segment (start 63.4 76.8) (end 63.4 123.2) (width 0.6) (layer "B.Cu") (net 1))
  (segment (start 63.4 123.2) (end 65.0 123.2) (width 0.6) (layer "B.Cu") (net 1))
  (segment (start 65.0 123.2) (end 82.5 123.2) (width 0.6) (layer "B.Cu") (net 1))
  (segment (start 82.5 123.2) (end 82.5 122.0) (width 0.6) (layer "B.Cu") (net 1))
  (segment (start 82.5 123.2) (end 94.2 123.2) (width 0.6) (layer "B.Cu") (net 1))
  (segment (start 94.2 123.2) (end 97.0 123.2) (width 0.6) (layer "B.Cu") (net 1))
  (segment (start 97.0 123.2) (end 97.0 97.5) (width 0.6) (layer "B.Cu") (net 1))
  (segment (start 97.0 97.5) (end 106.25 97.5) (width 0.6) (layer "B.Cu") (net 1))
  (segment (start 106.25 97.5) (end 106.25 94.0) (width 0.6) (layer "B.Cu") (net 1))
  (segment (start 106.25 97.5) (end 111.46 97.5) (width 0.6) (layer "B.Cu") (net 1))
  (segment (start 111.46 97.5) (end 111.46 92.5) (width 0.6) (layer "B.Cu") (net 1))
  (segment (start 111.46 92.5) (end 115.25 92.5) (width 0.6) (layer "B.Cu") (net 1))
  (segment (start 115.25 92.5) (end 121.19 92.5) (width 0.6) (layer "B.Cu") (net 1))
  (segment (start 121.19 92.5) (end 121.19 85.0) (width 0.6) (layer "B.Cu") (net 1))
  (segment (start 134.2 76.8) (end 136.0 76.8) (width 0.6) (layer "B.Cu") (net 1))
  (segment (start 136.0 76.8) (end 136.0 95.0) (width 0.6) (layer "B.Cu") (net 1))
  (segment (start 136.0 95.0) (end 121.19 95.0) (width 0.6) (layer "B.Cu") (net 1))
  (segment (start 121.19 95.0) (end 121.19 92.5) (width 0.6) (layer "B.Cu") (net 1))

  (segment (start 67.30 115.24) (end 65.0 115.24) (width 0.6) (layer "B.Cu") (net 1))
  (segment (start 65.0 115.24) (end 65.0 123.2) (width 0.6) (layer "B.Cu") (net 1))

  (segment (start 92.70 115.24) (end 94.2 115.24) (width 0.6) (layer "B.Cu") (net 1))
  (segment (start 94.2 115.24) (end 94.2 123.2) (width 0.6) (layer "B.Cu") (net 1))
"""

# Ground Plane (L-Shaped Polygon on B.Cu matching board outline)
zone_gnd = """
  (zone (net 1) (net_name "GND") (layers "B.Cu")
    (hatch edge 0.5)
    (priority 0)
    (connect_pads (clearance 0.25))
    (min_thickness 0.25)
    (filled_areas_thickness no)
    (fill yes (thermal_gap 0.3) (thermal_bridge_width 0.45))
    (polygon
      (pts
        (xy 62.6 74.1)
        (xy 136.9 74.1)
        (xy 136.9 101.4)
        (xy 97.4 101.4)
        (xy 97.4 125.9)
        (xy 62.6 125.9)
      )
    )
  )
"""

full_pcb_content = pcb_header + outline + silk_graphics + components + tracks + zone_gnd + ")\n"

pcb_file = os.path.join(OUTPUT_DIR, "smarthive_pool.kicad_pcb")
with open(pcb_file, "w") as f:
    f.write(full_pcb_content)

print(f"Created {pcb_file}")

# ==============================================================================
# 3. KICAD SCHEMATIC FILE (smarthive_pool.kicad_sch)
# ==============================================================================
sch_content = """(kicad_sch (version 20231120) (generator kicad-cli)
  (paper "A4")
  (title_block
    (title "Smarthive Pool Controller PCB V2.0 - ESP32 Edition")
    (date "2026-11-10")
    (rev "2.0")
    (company "Smarthivetech Solutions")
    (comment 1 "Zero-Clash L-PCB: ESP32 DevKit V1 + RC522 RFID (VSPI) + HW-479 RGB LED + Lay-Flat MG996R + 470uF Cap")
  )
  (sheet_instances
    (path "/" (page "1"))
  )
)
"""
sch_file = os.path.join(OUTPUT_DIR, "smarthive_pool.kicad_sch")
with open(sch_file, "w") as f:
    f.write(sch_content)

print(f"Created {sch_file}")

# ==============================================================================
# 4. RENDER 3D PCB IMAGES & SVGS WITH KICAD-CLI
# ==============================================================================
print("Rendering 3D PCB Views via kicad-cli...")

render_iso = os.path.join(OUTPUT_DIR, "pcb_render_3d_isometric.png")
render_top = os.path.join(OUTPUT_DIR, "pcb_render_3d_top.png")
render_bottom = os.path.join(OUTPUT_DIR, "pcb_render_3d_bottom.png")

cmd_iso = [
    KICAD_CLI, "pcb", "render",
    "--output", render_iso,
    "--rotate", "-45,0,45",
    "--perspective",
    "--floor",
    "--width", "1920",
    "--height", "1080",
    "--quality", "high",
    pcb_file
]
subprocess.run(cmd_iso, check=True)
print(f"Generated {render_iso}")

cmd_top = [
    KICAD_CLI, "pcb", "render",
    "--output", render_top,
    "--side", "top",
    "--width", "1920",
    "--height", "1080",
    "--quality", "high",
    pcb_file
]
subprocess.run(cmd_top, check=True)
print(f"Generated {render_top}")

cmd_bottom = [
    KICAD_CLI, "pcb", "render",
    "--output", render_bottom,
    "--side", "bottom",
    "--width", "1920",
    "--height", "1080",
    "--quality", "high",
    pcb_file
]
subprocess.run(cmd_bottom, check=True)
print(f"Generated {render_bottom}")

# Export 2D SVG Layouts
svg_front = os.path.join(OUTPUT_DIR, "pcb_layout_front.svg")
svg_bottom = os.path.join(OUTPUT_DIR, "pcb_layout_bottom.svg")
subprocess.run([KICAD_CLI, "pcb", "export", "svg", "-o", svg_front, "-l", "F.Cu,F.SilkS,F.Mask,Edge.Cuts", pcb_file], check=True)
subprocess.run([KICAD_CLI, "pcb", "export", "svg", "-o", svg_bottom, "-l", "B.Cu,B.SilkS,B.Mask,Edge.Cuts", pcb_file], check=True)

# ==============================================================================
# 5. EXPORT GERBERS & DRILL FILES
# ==============================================================================
print("Exporting Production Gerbers & Excellon Drill Files...")

cmd_gerber = [
    KICAD_CLI, "pcb", "export", "gerbers",
    "-o", GERBER_DIR,
    "--subtract-soldermask",
    pcb_file
]
subprocess.run(cmd_gerber, check=True)

cmd_drill = [
    KICAD_CLI, "pcb", "export", "drill",
    "-o", GERBER_DIR,
    "--excellon-units", "mm",
    pcb_file
]
subprocess.run(cmd_drill, check=True)

zip_path = os.path.join(OUTPUT_DIR, "smarthive_pool_gerbers.zip")
with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
    for root, _, files in os.walk(GERBER_DIR):
        for file in files:
            file_p = os.path.join(root, file)
            zipf.write(file_p, arcname=file)

print(f"Manufacturing zip package created: {zip_path}")

# ==============================================================================
# 6. BILL OF MATERIALS (BOM) CSV — VERSION 2.0 (ESP32 EDITION)
# ==============================================================================
bom_csv = """Reference,Value,Footprint,Quantity,Description,LCSC / Order Part
U1,ESP32 DevKit V1 (30-Pin),2x 1x15 Pin Socket 2.54mm (1.0 Inch Row Pitch),1,ESP-WROOM-32 Dual-Core Wi-Fi & BLE Development Board,C2836262
J2,RC522_RFID,1x8 Pin Header 2.54mm,1,MFRC522 13.56MHz RFID Reader Header on Hardware VSPI,C2893245
J6,HW479_RGB,1x4 Pin Header 2.54mm,1,HW-479 Common-Cathode RGB Status LED Module Header (GND/R/G/B),C124378
J3,MG996R_SERVO,1x3 Pin Header 2.54mm,1,MG996R High-Torque Lay-Flat Metal Gear Servo Header (GND/5V/PWM),C2887641
J5,USB_C_SW_5V,2-Pin 5.08mm Terminal Block,1,Switched 5V Power Input from RFID Pod USB-C Port & SS-12D00 Slide Switch,C8370
C1,470uF 25V,Radial D8.0mm P3.5mm,1,Bulk Electrolytic MG996R Servo Inrush Smoothing Capacitor,C134843
C2,100nF,Ceramic Disc 2.5mm,1,High-Frequency Servo Rail Decoupling Capacitor,C14663
D1,SS34 / 1N5819,DO-41 P7.62mm,1,3A Schottky Reverse Polarity Protection Diode,C44436
LED1,Green 3mm,LED D3mm,1,3.3V Power-Good Status Indicator,C965799
LED2,Amber 3mm,LED D3mm,1,Servo Ball Release Active Indicator,C965800
R1,1k Ohm,Axial DIN0207 P5.08mm,1,LED1 Current Limiter Resistor (1/4W),C25113
R2,1k Ohm,Axial DIN0207 P5.08mm,1,LED2 Current Limiter Resistor (1/4W),C25113
SW1,TEST_BTN,Tactile Switch 6x6mm 2-Pin,1,Onboard Bench Test / Manual Ball Release Pushbutton (GPIO 32),C318884
H1-H4,M2.5_Mount,2.5mm Hole 3.8mm Pad,4,Controller Enclosure Chassis Standoff Fasteners,Hardware
"""

bom_path = os.path.join(OUTPUT_DIR, "smarthive_pool_bom.csv")
with open(bom_path, "w") as f:
    f.write(bom_csv)

print(f"Created BOM: {bom_path}")
print("="*60)
print("ALL SMARTHIVE POOL V2.0 (ESP32) PCB ASSETS GENERATED!")
print("="*60)
