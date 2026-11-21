#!/usr/bin/env python3
"""Offline preflight checks for CITY LIFE 3D V7.2.1."""
from pathlib import Path
import re
import struct
import wave

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src" / "main.c"
WORKFLOW = ROOT / ".github" / "workflows" / "android-apk.yml"


def fail(message: str) -> None:
    raise SystemExit(f"ERROR: {message}")


def check_png(path: Path) -> tuple[int, int]:
    if not path.is_file():
        fail(f"required image is missing: {path.relative_to(ROOT)}")
    data = path.read_bytes()
    if len(data) < 24 or data[:8] != b"\x89PNG\r\n\x1a\n":
        fail(f"not a valid PNG: {path.relative_to(ROOT)}")
    return struct.unpack(">II", data[16:24])


def main() -> None:
    if not SOURCE.is_file():
        fail("src/main.c is missing")
    if not WORKFLOW.is_file():
        fail(".github/workflows/android-apk.yml is missing")

    source = SOURCE.read_text(encoding="utf-8")
    workflow = WORKFLOW.read_text(encoding="utf-8")

    # Four atlas columns, 25 rows, 100 labels, each cell is 256x64.
    width, height = check_png(ROOT / "assets" / "ui" / "arabic-ui.png")
    if (width, height) != (1024, 1600):
        fail(f"Arabic UI atlas must be 1024x1600; found {width}x{height}")
    if "#define AR_LABEL_COUNT 100" not in source:
        fail("Arabic atlas label count no longer matches the packed atlas")

    # Validate exactly the five supplied music tracks and their OGG signatures.
    music_dir = ROOT / "assets" / "music"
    playlist = music_dir / "playlist.txt"
    if not playlist.is_file():
        fail("assets/music/playlist.txt is missing")
    entries = []
    for raw in playlist.read_text(encoding="utf-8-sig").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        filename, sep, title = line.partition("|")
        filename = filename.strip()
        if not sep or not title.strip() or Path(filename).name != filename:
            fail(f"invalid playlist line: {line!r}")
        if Path(filename).suffix.lower() != ".ogg":
            fail(f"user radio track must be OGG: {filename}")
        track = music_dir / filename
        if not track.is_file() or track.stat().st_size < 256:
            fail(f"playlist track missing or too small: {filename}")
        if track.read_bytes()[:4] != b"OggS":
            fail(f"invalid OGG signature: {filename}")
        entries.append(filename)
    if len(entries) != 5 or len(set(entries)) != 5:
        fail(f"expected exactly five unique user tracks; found {len(entries)}")
    actual_ogg = sorted(x.name for x in music_dir.glob("*.ogg"))
    if actual_ogg != sorted(entries):
        fail("music folder contains tracks not listed in playlist or misses a listed track")

    # Validate sound files as readable PCM WAV resources.
    sfx_names = ["pistol.wav", "rifle.wav", "shotgun.wav", "reload.wav",
                 "jump.wav", "footstep.wav", "melee.wav", "impact.wav"]
    for filename in sfx_names:
        path = ROOT / "assets" / "sfx" / filename
        if not path.is_file():
            fail(f"required gameplay sound is missing: {filename}")
        try:
            with wave.open(str(path), "rb") as wav:
                if wav.getnchannels() not in (1, 2):
                    fail(f"unsupported channel count in {filename}")
                if wav.getframerate() < 8000 or wav.getsampwidth() not in (1, 2, 3, 4):
                    fail(f"unsupported PCM format in {filename}")
                if wav.getnframes() <= 0:
                    fail(f"empty WAV resource: {filename}")
        except (wave.Error, EOFError) as exc:
            fail(f"invalid WAV resource {filename}: {exc}")
        if f'"sfx/{filename}"' not in source:
            fail(f"source does not load required gameplay sound: {filename}")

    # Consistency checks for the fixed-size spatial city grid and save schema.
    checks = [
        "#define CITY_GRID_SIDE       50",
        "#define BUILDINGS_PER_BLOCK  4",
        "#define MAX_BUILDINGS        10600",
        '"citylife_profile.dat"',
        "SaveFileData(SAVE_FILE_NAME, slots, (int)sizeof(slots))",
        "LoadFileData(SAVE_FILE_NAME, &fileSize)",
        "SAVE_SCHEMA_VERSION  2",
        "DrawNearbyBuildings(actor)",
        "HasClearShot(player, h->position)",
        "ResolveCameraCollision(desiredTarget, desiredCamera)",
        "if (gPlayerInvulnerable) gPlayerHealth = 100;",
    ]
    for token in checks:
        if token not in source:
            fail(f"source consistency check failed: {token}")
    if "SaveStorageValue(" in source or "LoadStorageValue(" in source:
        fail("non-public raylib example storage helpers are still being called")
    if "firePressed" in source or "fireWasDown" in source:
        fail("legacy single-tap fire input remains in source")

    if '"app.version_name": "0.7.3"' not in workflow:
        fail("Android version_name does not match V7.2.1")
    if '"app.version_code": "12"' not in workflow:
        fail("Android version_code does not match V7.2.1")
    if "CITY-LIFE-3D-V7.2.1-ANDROID-SAVE-FIX-APK" not in workflow:
        fail("APK artifact name does not match V7.2.1")

    print("PASS: Arabic atlas 1024x1600 / 100 labels")
    print(f"PASS: {len(entries)} user OGG tracks only")
    print(f"PASS: {len(sfx_names)} readable WAV gameplay effects")
    print("PASS: source, save schema, city grid, camera and combat checks")
    print("PASS: Android workflow version 0.7.3 / versionCode 12")


if __name__ == "__main__":
    main()
