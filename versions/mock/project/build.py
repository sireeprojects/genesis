import os
import subprocess
import shutil
from pathlib import Path

def build_and_run():
    # 1. Define paths
    project_root = Path.cwd()
    build_dir = project_root / "build"
    # Assuming 'myapp' is generated in the build folder
    executable_path = build_dir / "myapp" 

    # 2. Create 'build' directory (Clear it if it already exists for a clean build)
    if build_dir.exists():
        print(f"-- Removing existing build directory: {build_dir}")
        shutil.rmtree(build_dir)
    
    build_dir.mkdir()
    print(f"-- Created build directory.")

    try:
        # 3. Execute CMake
        print("-- Running CMake...")
        subprocess.run(["cmake", ".."], cwd=build_dir, check=True)

        # 4. Run Make
        print("-- Running Make...")
        subprocess.run(["make"], cwd=build_dir, check=True)

        # 5. Execute the application
        if executable_path.exists():
            print(f"-- Executing {executable_path.name}...\n" + "="*20)
            # Use 'check=False' here if you want the script to finish even if your app returns an error code
            subprocess.run([f"./{executable_path.name}"], cwd=build_dir, check=True)
        else:
            print(f"Error: Executable '{executable_path.name}' not found in build directory.")

    except subprocess.CalledProcessError as e:
        print(f"\n[!] Command failed: {e.cmd}")
        print(f"[!] Return code: {e.returncode}")
    except FileNotFoundError:
        print("\n[!] Error: Ensure 'cmake' and 'make' are installed and in your system PATH.")

if __name__ == "__main__":
    build_and_run()
