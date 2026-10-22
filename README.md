# PBR-Renderer

*Physically based deferred renderer with HBAO ambient occlusion*

A physically based deferred renderer written in C++ with Qt and OpenGL 3.3. It loads OBJ meshes and renders them with image-based lighting (IBL), a G-buffer, and horizon-based ambient occlusion (HBAO).

## Features

- **Deferred rendering:** a G-buffer pass stores albedo and normals, and later passes read from it.
- **Physically based shading:** color, roughness and metalness maps, with a Fresnel term.
- **Image-based lighting:** diffuse irradiance map, prefiltered specular environment map (GGX importance sampling) and a BRDF integration LUT (split-sum approximation).
- **HBAO:** horizon-based ambient occlusion computed from the depth and normal buffers, followed by a separable Gaussian blur and applied to the albedo.
- **Debug views:** G-buffer visualizer (albedo, normals, depth), reflection and sky shaders, and a Phong reference shader.

Each shader is described in [`shaders/README.md`](shaders/README.md).

## Requirements

- [Qt](https://www.qt.io) 6.0 or newer
- [GLM](https://github.com/g-truc/glm) (included in the repository)
- A C++ compiler with OpenGL 3.3 drivers

On Linux, for example:

```sh
sudo apt update
sudo apt install build-essential libgl1-mesa-dev libglm-dev
```

Install Qt 6 with the [Qt online installer](https://www.qt.io/download-qt-installer-oss), selecting **Qt 6.X for Desktop Development**.

## Building

With qmake:

```sh
qmake
make
```

Or open `ViewerPBS.pro` in Qt Creator. On Windows, `PBR-Renderer.sln` is also included.

## Credits

The viewer skeleton (mesh loading, camera, Qt window and OpenGL setup) was created by Imanol Munoz-Pandilella, based on a previous skeleton by Marc Comino, for the Fast Realistic Rendering course at UPC. It is distributed under the [CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/) license.

The shaders and the rendering pipeline are the author's own work.

---

Developed for the Fast Realistic Rendering course.
