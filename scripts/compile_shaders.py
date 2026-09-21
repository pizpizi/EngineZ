import os
import re
import subprocess
from pathlib import Path

SOURCE_DIR = Path("./examples")
BUILD_DIR = Path("build/examples")

EXTENSIONS = [".comp", ".cmp", ".vert", ".frag", ".hlsl"]

def get_kernels(file_path: Path):
    content = file_path.read_text()
    return re.findall(r"#ifdef\s+KERNEL_([A-Za-z0-9_]+)", content)

def disassemble_spirv(spv_file: Path, out_txt_file: Path):
    subprocess.run(["spirv-dis", str(spv_file), "-o", str(out_txt_file)])

def compile_glsl(src: Path, out_spv: Path, kernel_def: str = None):
    cmd = ["glslangValidator", "-V", "-g", str(src.absolute())]
    if kernel_def:
        cmd.append(f"-D{kernel_def}")
    cmd.extend(["-o", str(out_spv)])
    subprocess.run(cmd)

def compile_hlsl(src: Path, out_spv: Path, kernel_def: str = None):
    cmd = [
        "dxc",
        "-spirv",
        "-T", "cs_6_0",
        "-E", "main",
        "-Zi",
        # "-fspv-debug=vulkan-with-source",
        "-O3",
        str(src.absolute())
    ]
    if kernel_def:
        cmd.extend(["-D", kernel_def])
    cmd.extend(["-Fo", str(out_spv)])
    subprocess.run(cmd)

def compile_shaders():
    if not SOURCE_DIR.exists():
        print("Shaders source folder doesn't exist. Quiting ...")
        exit(1)

    for ext in EXTENSIONS:
        for f in SOURCE_DIR.rglob(f"*{ext}"):
            rel_path = f.relative_to(SOURCE_DIR)
            out_base_dir = BUILD_DIR / rel_path.parent
            out_base_dir.mkdir(parents=True, exist_ok=True)

            if ext in [".comp", ".cmp", ".hlsl"]:
                kernels = get_kernels(f)
                compile_fn = compile_hlsl if ext == ".hlsl" else compile_glsl

                if kernels:
                    for kernel in kernels:
                        output_spv = out_base_dir / f"{f.stem}_{kernel.lower()}.spv"
                        output_txt = out_base_dir / f"{f.stem}_{kernel.lower()}.spvasm"
                        print(f"Compiling {f.name} [KERNEL_{kernel}] -> {output_spv.name}...")
                        compile_fn(f, output_spv, f"KERNEL_{kernel}")
                        disassemble_spirv(output_spv, output_txt)
                else:
                    output_spv = out_base_dir / f"{f.name}.spv"
                    output_txt = out_base_dir / f"{f.name}.spvasm"
                    print(f"Compiling {f.name} -> {output_spv.name}...")
                    compile_fn(f, output_spv)
                    disassemble_spirv(output_spv, output_txt)
            else:
                output_spv = out_base_dir / f"{f.name}.spv"
                output_txt = out_base_dir / f"{f.name}.spvasm"
                print(f"Compiling {f.name} -> {output_spv.name}...")
                compile_glsl(f, output_spv)
                disassemble_spirv(output_spv, output_txt)

if __name__ == "__main__":
    compile_shaders()