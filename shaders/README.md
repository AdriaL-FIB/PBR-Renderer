# Shaders

GLSL 3.30 shaders used by the renderer. Each program is a `.vert` and `.frag` pair with the same base name.

## Geometry and G-buffer

- **`gbuffer.vert` / `gbuffer.frag`**: geometry pass. Writes the albedo and the eye-space normal into two render targets of the G-buffer. The albedo is white unless `USE_TEXTURES` is defined.
- **`gbuffer-view.vert` / `gbuffer-view.frag`**: debug visualizer. Draws a full-screen quad showing one G-buffer channel (albedo, normals or linearized depth), selected with `current_texture`.

## Ambient occlusion (HBAO)

- **`hbao.vert` / `hbao.frag`**: horizon-based ambient occlusion. Rebuilds the view-space position from the depth buffer, field of view and aspect ratio, then searches for horizons along several directions (`numDirs`, `nSamples`, `searchRadius`). The `improvements` uniform switches an optional variant of the method on or off.
- **`blur.vert` / `blur.frag`**: separable 5-tap Gaussian blur, applied horizontally or vertically depending on `dir`. It smooths the AO term.
- **`applyAO.vert` / `applyAO.frag`**: multiplies the albedo by the blurred AO texture.

## Physically based shading

- **`pbs.vert` / `pbs.frag`**: physically based shading using color, roughness and metalness maps, a Fresnel term (`fresnel` uniform), a light given in world space by the `light` uniform, and a specular environment map.
- **`ibl-pbs.vert` / `ibl-pbs.frag`**: the same material model with image-based lighting. It combines the diffuse irradiance map, the prefiltered specular map and the BRDF LUT.

## Environment maps and precomputation

- **`computeIrradiance.frag`** (with its `.vert`): convolves the environment cubemap into a diffuse irradiance map by sampling the hemisphere around each direction.
- **`prefiltered-env.frag`** (with its `.vert`): builds the prefiltered specular map for a given `roughness`, using GGX importance sampling with Hammersley points.
- **`precomputeBRDF.frag`** (with its `.vert`): precomputes the 2D BRDF integration LUT for the split-sum approximation, using Hammersley points and GGX importance sampling.

The `.vert` files of these three programs are simple pass-throughs. The cubemap is sampled with the vertex position as direction.

## Reference and debug shaders

- **`reflection.vert` / `reflection.frag`**: perfect mirror reflection. Samples the environment map along `reflect(-V, N)`.
- **`sky.vert` / `sky.frag`**: draws the skybox from the environment cubemap, with the depth set to the far plane (`xyww`).
- **`phong.vert` / `phong.frag`**: Phong shading with fixed ambient, diffuse and specular coefficients. Used as a reference.
- **`texMap.vert` / `texMap.frag`**: draws a texture directly on the mesh, for checking UVs and textures.
