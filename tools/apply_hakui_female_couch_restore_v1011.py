from pathlib import Path


def replace_exact(path: Path, old: str, new: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match in {path}, found {count}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")
    print(f"[HAKUI FEMALE v1.011] patched {label}: {path}")


room = Path("src/world/BlackRoom.cpp")
app = Path("src/core/HakuiApp.cpp")

# The couch already has two explicit cushion primitives. Each one was also
# configured with repeatCount=2, which rendered four cushions and pushed the
# final repeated slab beyond the right arm. Keep the two explicit cushions, but
# render each exactly once. Seat semantics live in SeatAnchor/affordance data and
# are intentionally untouched by this visual-only restore.
replace_exact(
    room,
    "    {WorldPrimitiveKind::Furniture, MaterialRole::PowderConcrete,\n"
    "     4.90f, 0.73f, 4.30f, 1.25f, 0.22f, 0.82f,\n"
    "     0.0f, 0.0f, 0.0f, 2, 1.40f, 0.0f, 0.0f},\n",
    "    {WorldPrimitiveKind::Furniture, MaterialRole::PowderConcrete,\n"
    "     4.90f, 0.73f, 4.30f, 1.25f, 0.22f, 0.82f},\n",
    "left couch cushion single-instance restore",
)

replace_exact(
    room,
    "    {WorldPrimitiveKind::Furniture, MaterialRole::PowderConcrete,\n"
    "     6.54f, 0.73f, 4.30f, 1.25f, 0.22f, 0.82f,\n"
    "     0.0f, 0.0f, 0.0f, 2, 1.40f, 0.0f, 0.0f},\n",
    "    {WorldPrimitiveKind::Furniture, MaterialRole::PowderConcrete,\n"
    "     6.54f, 0.73f, 4.30f, 1.25f, 0.22f, 0.82f},\n",
    "right couch cushion single-instance restore",
)

# Traceable runtime marker for the patched female executable.
replace_exact(
    app,
    "        ? \"[HAKUI FEMALE] WORLD ONLINE // female shell // ghost rig authoritative\"\n",
    "        ? \"[HAKUI FEMALE v1.011] WORLD ONLINE // female shell // couch restore // ghost rig authoritative\"\n",
    "female couch-restore boot identity",
)

print("[HAKUI FEMALE v1.011] couch presentation restore complete // seat semantics preserved")
