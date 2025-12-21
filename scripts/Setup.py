import os
import subprocess
import platform

# Change from Scripts directory to root
os.chdir('../')

print("\nUpdating submodules...")
subprocess.call(["git", "submodule", "update", "--init", "--recursive"])

if platform.system() == "Windows":
    build_dir = "build"
    print("\nGenerating Visual Studio 2022 solution with CMake...")
    subprocess.call(["cmake", "-S", ".", "-B", build_dir, "-G", "Visual Studio 17 2022", "-A", "x64"])

    print("\nSetup completed!")

