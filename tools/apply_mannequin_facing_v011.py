from pathlib import Path

path = Path("src/mannequin_lab/MannequinLabApp.cpp")
text = path.read_text(encoding="utf-8")
old = "    mannequin_.yaw = 0.0f;\n"
new = (
    "    // Lab presentation convention: +Z is the mannequin's anatomical forward,\n"
    "    // while the default studio camera begins on the -Z side. Rotate the\n"
    "    // mannequin 180 degrees so Neutral/T/A/Crouch/Ollie-Pop present the\n"
    "    // FRONT of the body to the scientist by default. HAKUI gameplay is untouched.\n"
    "    mannequin_.yaw = kPi;\n"
)
count = text.count(old)
if count != 1:
    raise RuntimeError(f"expected one default mannequin yaw assignment, found {count}")
path.write_text(text.replace(old, new, 1), encoding="utf-8")
print("[MANNEQUIN LAB] default facing corrected // FRONT -> CAMERA")
