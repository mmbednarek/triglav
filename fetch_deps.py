#!/usr/bin/python3
import argparse
import platform
import shutil
import sys
import tarfile
import tempfile
import urllib.request
import zipfile
from dataclasses import dataclass, field
from pathlib import Path

BASE_EXTERNAL_DIR = Path("library/external")


@dataclass(frozen=True)
class Dependency:
    name: str
    version: str
    urls_by_platform: dict[str, str] = field(default_factory=dict)
    strip_top_level_dir: bool = False


DEPENDENCIES: list[Dependency] = [
    Dependency(
        name="slang",
        version="2026.23",
        urls_by_platform={
            "linux": "https://github.com/shader-slang/slang/releases/download/v2026.23/slang-2026.23-linux-x86_64.zip",
            "windows": "https://github.com/shader-slang/slang/releases/download/v2026.23/slang-2026.23-windows-x86_64.zip",
        },
        strip_top_level_dir=False,
    ),
    # Example for adding another library later:
    # Dependency(
    #     name="glfw",
    #     version="3.4",
    #     urls_by_platform={
    #         "linux": "https://example.com/glfw-3.4-linux-x86_64.tar.gz",
    #         "windows": "https://example.com/glfw-3.4-windows-x86_64.zip",
    #     },
    #     strip_top_level_dir=True,
    # ),
]


def detect_platform_and_arch() -> tuple[str, str]:
    system = platform.system().lower()
    if system == "linux":
        plat = "linux"
    elif system == "windows":
        plat = "windows"
    else:
        raise RuntimeError(f"Unsupported platform: {platform.system()} (expected Windows or Linux)")

    machine = platform.machine().lower()
    if machine not in ("x86_64", "amd64"):
        raise RuntimeError(f"Unsupported architecture: {platform.machine()} (expected x86_64)")

    return plat, "x86_64"


def download_file(url: str, dest_path: Path) -> None:
    print(f"Downloading {url}...")
    request = urllib.request.Request(
        url,
        headers={"User-Agent": "python-dependency-fetcher/2.0"},
    )
    with urllib.request.urlopen(request, timeout=120) as response, open(dest_path, "wb") as out_file:
        shutil.copyfileobj(response, out_file)


def safe_extract_zip(archive_path: Path, target_dir: Path) -> None:
    target_resolved = target_dir.resolve()
    with zipfile.ZipFile(archive_path, "r") as zf:
        for member in zf.infolist():
            member_path = (target_dir / member.filename).resolve()
            if not member_path.is_relative_to(target_resolved):
                raise RuntimeError(f"Unsafe zip entry detected (path traversal): {member.filename}")
        zf.extractall(target_dir)


def safe_extract_tar(archive_path: Path, target_dir: Path) -> None:
    target_resolved = target_dir.resolve()
    with tarfile.open(archive_path, "r:*") as tf:
        for member in tf.getmembers():
            member_path = (target_dir / member.name).resolve()
            if not member_path.is_relative_to(target_resolved):
                raise RuntimeError(f"Unsafe tar entry detected (path traversal): {member.name}")
            if member.issym() or member.islnk():
                link_target = (member_path.parent / member.linkname).resolve()
                if not link_target.is_relative_to(target_resolved):
                    raise RuntimeError(f"Unsafe link target in archive: {member.name}")
        tf.extractall(target_dir, filter="data")


def extract_archive(archive_path: Path, target_dir: Path) -> None:
    name_lower = archive_path.name.lower()
    if name_lower.endswith(".zip"):
        safe_extract_zip(archive_path, target_dir)
    elif name_lower.endswith((".tar.gz", ".tgz", ".tar.xz", ".txz", ".tar.bz2", ".tbz2", ".tar")):
        safe_extract_tar(archive_path, target_dir)
    else:
        raise RuntimeError(f"Unsupported archive format: {archive_path.name}")


def flatten_single_top_level_dir(target_dir: Path) -> None:
    entries = list(target_dir.iterdir())
    if len(entries) == 1 and entries[0].is_dir():
        wrapper_dir = entries[0]
        for child in list(wrapper_dir.iterdir()):
            shutil.move(str(child), str(target_dir / child.name))
        wrapper_dir.rmdir()


def install_dependency(dep: Dependency, plat: str, arch: str, force: bool = False) -> Path:
    if plat not in dep.urls_by_platform:
        raise RuntimeError(f"Dependency '{dep.name}' has no URL configured for platform '{plat}'")

    url = dep.urls_by_platform[plat]
    dest_dir = BASE_EXTERNAL_DIR / dep.name / f"{plat}_{arch}"

    if dest_dir.exists() and not force:
        print(f"[{dep.name}] Already unpacked at {dest_dir}. Use --force to re-download and overwrite.")
        return dest_dir

    if dest_dir.exists() and force:
        shutil.rmtree(dest_dir)

    dest_dir.mkdir(parents=True, exist_ok=True)
    filename_from_url = url.split("/")[-1].split("?")[0] or f"{dep.name}-{dep.version}.archive"

    with tempfile.TemporaryDirectory() as tmp_dir:
        archive_path = Path(tmp_dir) / filename_from_url
        download_file(url, archive_path)
        print(f"[{dep.name}] Unpacking to {dest_dir}...")
        extract_archive(archive_path, dest_dir)
        if dep.strip_top_level_dir:
            flatten_single_top_level_dir(dest_dir)

    print(f"[{dep.name}] Installed {dep.version} -> {dest_dir}")
    return dest_dir


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Fetch and unpack external dependencies.")
    parser.add_argument(
        "names",
        nargs="*",
        help="Optional dependency names to install (defaults to all configured dependencies).",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Re-download and overwrite existing unpacked directories.",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        plat, arch = detect_platform_and_arch()

        by_name = {dep.name: dep for dep in DEPENDENCIES}
        if len(by_name) != len(DEPENDENCIES):
            raise RuntimeError("Duplicate dependency names found in DEPENDENCIES list.")

        if args.names:
            unknown = [n for n in args.names if n not in by_name]
            if unknown:
                raise RuntimeError(f"Unknown dependency name(s): {', '.join(unknown)}")
            selected = [by_name[n] for n in args.names]
        else:
            selected = DEPENDENCIES

        for dep in selected:
            install_dependency(dep, plat, arch, force=args.force)

        return 0
    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
