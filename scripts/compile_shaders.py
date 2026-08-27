import os
import subprocess
from pathlib import Path

SOURCE_DIR = Path("./examples")
BUILD_DIR = Path("build/examples")

EXTENSIONS = [".comp", ".cmp", ".vert", ".frag"]

def compile_shaders():
    if(not SOURCE_DIR.exists()):
        print("Shaders source folder doesn't exist. Quiting ...")
        exit(1)

    for ext in EXTENSIONS:
        for f in SOURCE_DIR.rglob(f"*{ext}"):
            rel_path = f.relative_to(SOURCE_DIR)

            output_file = BUILD_DIR / rel_path.parent / f"{f.name}.spv"
            output_file.parent.mkdir(parents=True, exist_ok=True)

            print(f"Compiling {f.name}...")

            subprocess.run(["glslc", "-o", str(output_file), str(f.absolute())])

if __name__ == "__main__":
    compile_shaders()