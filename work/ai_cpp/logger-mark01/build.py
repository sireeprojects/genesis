import os
import subprocess
import shutil
from pathlib import Path

def run_pipeline():
    # Setup paths
    build_dir = Path("build_output")
    
    # 1. Clean previous runs
    if build_dir.exists():
        shutil.rmtree(build_dir)
    build_dir.mkdir()

    # 2. Run Make
    print("--- Building Project ---")
    try:
        subprocess.run(["make"], check=True)
        
        # Move binary to build folder (optional, keeping project clean)
        if os.path.exists("myapp"):
            shutil.move("myapp", build_dir / "myapp")
            
        # 3. Execute
        print("\n--- Running Application ---")
        subprocess.run(["./myapp"], cwd=build_dir, check=True)
        
        print("\n--- Process Complete. Check .log files in build folder. ---")
        
    except subprocess.CalledProcessError as e:
        print(f"An error occurred during build/run: {e}")

if __name__ == "__main__":
    run_pipeline()
