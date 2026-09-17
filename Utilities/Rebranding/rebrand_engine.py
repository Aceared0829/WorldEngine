#!/usr/bin/env python3
"""Plan a tracked-file rebrand. Default is read-only; apply fails on blockers.

This is a lexical migration planner, not a C++ or asset-format converter.
It intentionally cannot certify that the resulting engine builds or loads assets.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import binary_formats


URL = re.compile(r"(?:https?://|git@|data:[a-z0-9.+-]+/[a-z0-9.+-]+[;,])[^\s<>\"')]+|(?:www\.)?ezengine\.(?:net|github\.io)\b|\bezEngine/[A-Za-z0-9_.-]+|(?<=derived from )ezEngine\b|[A-Za-z0-9+/]{128,}={0,2}", re.I)
LEGAL = re.compile(r"copyright|SPDX-|\(c\)\s*\d{4}", re.I)
# Do not change ordinary words (freeze, Bezier). Fixed-width binary magic
# requires an explicit mapping. Underscores are CMake compound boundaries.
TOKEN = re.compile(r"(?<![A-Za-z0-9])(?:(?:ezEngine|ezengine|EZENGINE)(?![A-Za-z0-9])|ez(?=[A-Z_])|EZ(?=_)|ez\b|EZ\b)")
UNKNOWN = re.compile(r"(?<![A-Za-z0-9])(?:ez[a-z][A-Za-z0-9_]*|EZ[A-Z][A-Z0-9_]*)")
ARCHIVE_MAGIC = re.compile(r"\bEZARCHIVE(?:-END)?\b")
SPECIAL = {
    'ezsv': 'wsv', 'ezs': 'Ws', 'eztest': 'Wtest', 'eztCreateFile': 'WtCreateFile',
    'EZEMBREE_FOUND': 'WEMBREE_FOUND', 'EZVULKAN_FOUND': 'WVULKAN_FOUND', 'EZFMOD_FOUND': 'WFMOD_FOUND',
    'EZBINIMAGEDATA': 'WBINIMAGEDATA', 'EZBINTEXTURE2D': 'WBINTEXTURE2D',
    'EZBINTEXTURE3D': 'WBINTEXTURE3D', 'EZBINTEXTUREATLAS': 'WBINTEXTUREATLAS',
    'EZBINTEXTURECUBE': 'WBINTEXTURECUBE', 'EZIMAGE': 'WIMAGE',
    'EZCompat': 'WCompat',
}
SPECIAL_PATTERN = re.compile(r'(?<![A-Za-z0-9])(?:' + '|'.join(SPECIAL) + r')(?![A-Za-z0-9])')
# These are fixed-width on-wire four-character codes, not symbol prefixes.
WIRE_IDS = {'EZBC', 'EZID', 'EZFS', 'EZIP', 'EZPZ'}
SELF = "Utilities/Rebranding/"
PRODUCT_NAME = 'WorldEngine'


def rename(text):
    # A literal suffix starting _Uppercase is reserved by C++.
    text = re.sub(r'\b_Wsv\b', '_wsv', text)
    text = text.replace('-DEZ_', '-DW_')
    text = ARCHIVE_MAGIC.sub(lambda m: m[0].replace("EZARCHIVE", "WEARCHIVE"), text)
    text = SPECIAL_PATTERN.sub(lambda m: SPECIAL[m[0]], text)
    text = text.replace('EzEmbree', 'WEmbree').replace('EzVulkan', 'WVulkan').replace('EzFmod', 'WFmod')
    return TOKEN.sub(lambda m: {"ezEngine": PRODUCT_NAME, "ezengine": PRODUCT_NAME.lower(),
                               "EZENGINE": PRODUCT_NAME.upper()}.get(m[0], "W"), text)


def transform(text):
    # Keep fixed-width file identifiers distinct from C++ types / extensions.
    text = text.replace('"ezAsset"', '"WEAsset"').replace('[ezBinaryScene]', '[WEBinaryScene]')
    result = []
    for line in text.splitlines(keepends=True):
        if LEGAL.search(line):
            result.append(line)
            continue
        start = 0
        for match in URL.finditer(line):
            result.append(rename(line[start:match.start()]))
            result.append(match[0])
            start = match.end()
        result.append(rename(line[start:]))
    return "".join(result)


def git(root, *args):
    return subprocess.check_output(["git", "-C", str(root), *args])


def tracked(root, prefix=""):
    for entry in git(root, "ls-files", "--stage", "-z").split(b"\0"):
        if not entry:
            continue
        info, raw_path = entry.split(b"\t", 1)
        mode, _, stage = info.split()
        if stage != b"0":
            raise RuntimeError("Resolve Git conflicts before planning.")
        path = raw_path.decode("utf-8")
        yield prefix + path, mode.decode()


def allowed(path):
    p = Path(path)
    if path.startswith(SELF) or path == ".gitmodules":
        return False
    if '/Sonnis/README.txt' in path:
        return False
    if any(part in {"LICENSES", "Licenses"} for part in p.parts):
        return False
    if p.name.lower().startswith(("license", "copying", "copyright", "notice")):
        return False
    if "ThirdParty" in p.parts:
        # Engine build glue is mixed into vendor directories.
        return p.name == "CMakeLists.txt" or p.suffix == ".cmake" or p.name in {"ezReadme.txt", "stbImplementation.cpp"}
    return True


def decode(raw):
    if raw.startswith((b"\xff\xfe", b"\xfe\xff")):
        encoding = "utf-16-le" if raw[:2] == b"\xff\xfe" else "utf-16-be"
        try:
            return raw[2:].decode(encoding), encoding, raw[:2]
        except UnicodeDecodeError:
            return None
    if b"\0" in raw:
        return None
    try:
        return raw.decode("utf-8"), "utf-8", b""
    except UnicodeDecodeError:
        return None


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def plan(root):
    files = list(tracked(root))
    blockers, changes, skipped = [], [], []
    for path, mode in list(files):
        if mode == "160000":
            if path != "Data/Tools/Precompiled":
                if not (root / path / ".git").exists():
                    blockers.append({"path": path, "reason": "Submodule is not initialized"})
                else:
                    files.extend(tracked(root / path, path + "/"))
            else:
                skipped.append({"path": path, "reason": "External submodule; retained unchanged"})
    destinations = {}
    payloads = {}
    for path, mode in files:
        if mode == "160000":
            continue
        if not allowed(path):
            skipped.append({"path": path, "reason": "Provenance, vendor source, or migration tooling"})
            continue
        # Support another audit after the tracked files were renamed in the
        # working tree, without staging or committing anything for the user.
        if not (root / path).exists() and (root / rename(path)).is_file():
            path = rename(path)
        source = root / path
        if mode != "100644" and mode != "100755":
            blockers.append({"path": path, "reason": "Unsupported Git file mode " + mode})
            continue
        if not source.is_file() or source.is_symlink():
            blockers.append({"path": path, "reason": "Missing file or symlink"})
            continue
        raw = source.read_bytes()
        target = rename(path)
        key = target.casefold()
        if key in destinations and destinations[key] != path:
            blockers.append({"path": path, "reason": "Destination collision with " + destinations[key]})
        destinations[key] = path
        if path != target and (root / target).exists():
            blockers.append({"path": path, "reason": "Destination already exists: " + target})
        decoded = decode(raw)
        new_raw = raw
        if decoded is None:
            try:
                new_raw, reason = binary_formats.convert(path, raw, rename, root)
                skipped.append({"path": path, "reason": reason})
            except (ValueError, OSError, AttributeError) as error:
                blockers.append({"path": path, "reason": "Binary conversion blocked: " + str(error)})
        else:
            text, encoding, bom = decoded
            if (path == 'README.md' and text.startswith('# WorldEngine')) or (path == '.gitignore' and 'Code/Engine/WBuildInfo.h' in text):
                changed = text
            elif path.startswith('Code/Engine/Texture/DirectXTex/'):
                # Only the engine's integration macros/header; leave Microsoft's
                # algorithm identifiers (e.g. EZC) and patch attribution intact.
                changed = re.sub(r'\bEZ_', 'W_', text).replace('EZCompat.h', 'WCompat.h')
            else:
                changed = transform(text)
            new_raw = bom + changed.encode(encoding)
            audit = "\n".join(URL.sub("", line) for line in changed.splitlines() if not LEGAL.search(line))
            unknown = sorted(set(UNKNOWN.findall(audit)) - WIRE_IDS)
            if path == 'README.md' and text.startswith('# WorldEngine'):
                unknown = []  # User-authored provenance and old-format documentation.
            if path.startswith('Code/Engine/Texture/DirectXTex/'):
                unknown = [token for token in unknown if token != 'EZC']
            if unknown:
                blockers.append({"path": path, "reason": "Unmapped tokens need review", "tokens": unknown})
        if raw != new_raw or path != target:
            changes.append({"source": path, "target": target, "before_sha256": digest(raw),
                            "after_sha256": digest(new_raw), "content_changed": raw != new_raw})
            payloads[path] = new_raw
    report = {"product": PRODUCT_NAME, "chinese_name": "寰宇引擎", "prefix": "W",
              "root": str(root), "head": git(root, "rev-parse", "HEAD").decode().strip(),
              "summary": {"tracked_entries": len(files), "changes": len(changes),
                          "renamed_files": sum(c["source"] != c["target"] for c in changes),
                          "blockers": len(blockers)},
              "limitations": ["No C++ semantic, compilation, or runtime validation",
                              "Archive magic explicitly mapped to WEARCHIVE; upstream URLs retained",
                              "Untracked files and generated build outputs excluded",
                              "Existing serialized assets and external projects require migration",
                              "Old plugin binaries and build trees must not be reused"],
              "changes": changes, "blockers": blockers, "skipped": skipped}
    return report, payloads


def safe_path(root, relative):
    candidate = (root / relative).resolve()
    if not candidate.is_relative_to(root.resolve()):
        raise RuntimeError("Path escapes root: " + relative)
    return candidate


def restore(root, backup):
    manifest = json.loads((backup / "manifest.json").read_text(encoding="utf-8"))
    if Path(manifest["root"]).resolve() != root:
        raise RuntimeError("Backup belongs to a different repository")
    # Validate everything before restoring anything. Never overwrite later edits.
    for item in manifest["changes"]:
        original = safe_path(backup / "files", item["source"])
        if digest(original.read_bytes()) != item["before_sha256"]:
            raise RuntimeError("Backup checksum mismatch: " + item["source"])
        for rel in {item["source"], item["target"]}:
            p = safe_path(root, rel)
            if p.exists() and digest(p.read_bytes()) not in {item["before_sha256"], item["after_sha256"]}:
                raise RuntimeError("Later edit would be overwritten: " + rel)
    for item in manifest["changes"]:
        source, target = safe_path(root, item["source"]), safe_path(root, item["target"])
        source.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(safe_path(backup / "files", item["source"]), source)
        if source != target and target.exists():
            target.unlink()


def apply(root, backup, report, payloads):
    if report["blockers"]:
        raise RuntimeError("Apply refused: resolve the blockers listed in the report first.")
    if backup.exists():
        raise RuntimeError("Backup directory must not already exist")
    if backup.is_relative_to(root):
        raise RuntimeError("Keep backups outside the repository")
    # Check all inputs before making the durable backup or touching any source.
    for item in report["changes"]:
        if digest(safe_path(root, item["source"]).read_bytes()) != item["before_sha256"]:
            raise RuntimeError("Source changed during planning: " + item["source"])
    backup.mkdir(parents=True)
    for item in report["changes"]:
        dst = safe_path(backup / "files", item["source"])
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(safe_path(root, item["source"]), dst)
    (backup / "manifest.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    try:
        for item in report["changes"]:
            source, target = safe_path(root, item["source"]), safe_path(root, item["target"])
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(payloads[item["source"]])
            if source != target:
                source.unlink()
    except Exception:
        # Preserve partial writes for inspection; the manifest allows restoration
        # when checksums still match. No destructive Git reset is ever used.
        print("Apply interrupted. Backup: " + str(backup), file=sys.stderr)
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--report", type=Path)
    action = parser.add_mutually_exclusive_group()
    action.add_argument("--apply", action="store_true")
    action.add_argument("--restore", type=Path, metavar="BACKUP")
    parser.add_argument("--backup", type=Path, help="Required for --apply; new directory outside repository")
    args = parser.parse_args()
    root = args.root.resolve()
    if args.restore:
        restore(root, args.restore.resolve())
        print("Restored source files; empty directories may remain.")
        return
    report, payloads = plan(root)
    if args.report:
        # Exclusive creation prevents accidentally overwriting a source file.
        args.report.parent.mkdir(parents=True, exist_ok=True)
        with args.report.open("x", encoding="utf-8") as stream:
            json.dump(report, stream, ensure_ascii=False, indent=2)
    print(json.dumps(report["summary"], ensure_ascii=False, indent=2))
    if args.apply:
        if not args.backup:
            parser.error("--apply requires --backup")
        apply(root, args.backup.resolve(), report, payloads)
        print("Text/path migration applied. Build and runtime validation are still required.")
    else:
        print("Preview only. No engine files were modified.")


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, OSError, subprocess.CalledProcessError) as error:
        sys.exit(str(error))
