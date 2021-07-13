<img align="left" width="128" src="https://github.com/ErfanMo77/Syndra/blob/master/Syndra-Editor/assets/Logo/GITLOGO.png"/>

# Syndra Engine

Syndra is an early-stage 3D game engine with a focus on real-time rendering. Syndra uses **OpenGL** graphics API and is being developed for windows platform.

# Download
[Syndra-Engine](https://github.com/ErfanMo77/Syndra/releases/download/v1.1.0-alpha/Syndra-v1.1.0-alpha.zip) version alpha-1.1.0 

# Showcase



# Features
* Engine
  * Editor (with docking support)
  * Entity Component System (ECS)
  * Event handling system
  * Console logging system
  * Model and texture loading
  * Scene saving and loading using YAML serialization and deserialization
  * Immediate mode GUI
  * Component editor (material, mesh, tag and camera)
  * Viewport gizmos
  * HDRI skymap loading
  
* Rendering
  * Deferred Rendering
  * PBR Rendering (Cook–Torrance GGX)
  * Materials (with texture loading)
  * Dynamic shadows
  * Soft shadows using **PCSS** algorithm
  * Image Based Lighting (IBL)
  * Fast Aproximate Anti Aliasing (FXAA)
  * Normal mapping
  
  
# Compiling
Syndra only supports windows for now.
Visual Studio 2019 is recommended.

Start by cloning the repository with `git clone --recursive https://github.com/ErfanMo77/Syndra-Engine`.
If the repository was cloned non-recursively previously, use `git submodule update --init` to clone the necessary submodules.

Syndra requires vulkan sdk to compile shaders, so first, you have to run **scripts/setup.py** to install vulkan sdk on your device.
Then, you can execute GenerateProjects.bat to generate visual studio solution file.


# Dependencies
* Windows 10.
* [Microsoft Visual C++ Redistributable for Visual Studio 2019](https://aka.ms/vs/16/release/VC_redist.x64.exe).

# Authors

* **Erfan Momeni** - [erfanmo](https://github.com/ErfanMo77)

# Acknowledgments
[The Cherno](https://www.youtube.com/channel/UCQ-W1KE9EYfdxhL6S4twUNw) game engine series

Logo design : [AmirMohammad Abedini](https://gitlab.com/musashi1997)

# License
- Licensed under the MIT license, see [LICENSE](https://github.com/ErfanMo77/Syndra/blob/master/LICENSE) for details.
