# Copied from Hazel Engine (https://github.com/TheCherno/Hazel) (Apache License 2.0)
import os
import subprocess
import CheckPython
import platform

# Make sure everything we need is installed
CheckPython.ValidatePackages()

from Vulkan import VulkanConfiguration as VulkanRequirements

# Change from Scripts directory to root
os.chdir('../')

VulkanRequirements.Validate()

print("\nUpdating submodules...")
subprocess.call(["git", "submodule", "update", "--init", "--recursive"])

if platform.system() == "Windows":
    print("\nRunning premake...")
    subprocess.call([os.path.abspath("./GenerateProjects.bat"), "nopause"])

    print("\nSetup completed!")

