# gfx.h
gfx is a fast, modern, single-header, minimal-dependency Vulkan graphics library, written in C++20.

It's designed to be simple to use: it wraps the tedious parts of Vulkan (descriptors, barriers, memory, resource lifetime) and keeps everything else close to the API.

## Features
* **No descriptor sets to manage:** Textures and buffers are bindless, data is passed through push constants and buffer device addresses
* **Automatic barriers:** Resource states are tracked by the library, and barriers are placed automatically from what each pass reads and writes
* **Safe resource lifetime:** Destruction is deferred until the GPU is done with the resource, and it gets released automatically
* **Shared C++/GLSL structs:** with automatic shader validation through SPIRV-Reflect
* **Easy debugging:** validation layer, object names, debug labels and GPU pass timings
* **Fast built-in O(1) TLSF GPU allocator**
* **Supports graphics, compute, mesh shaders, ray tracing pipelines and ray queries**
* **No per frame heap allocations**

## Usage
In exactly one .cpp file:

```cpp
#define GFX_IMPLEMENTATION
#define GFX_WINDOW_WIN32
#include "gfx.h"
```

Everywhere else just include `gfx.h`.

## Showcase
<p align="center">  
  <img src="images/god.png" width="98%" alt="flying world">
  <img src="images/picapica.png" width="49%" alt="picapica">
  <img src="images/picapica_emissive.png" width="49%" alt="picapica emissive">
  <img src="images/spaceship.png" width="49%" alt="spaceship">
  <img src="images/bistro.png" width="49%" alt="bistro">
</p>

## Requirements
* A GPU and driver with Vulkan 1.3
* The Vulkan SDK headers
* A C++20 compiler, tested with MSVC on Windows

Optional: GLFW, VMA, SPIRV-Reflect and glm.

## Minimal sample

```cpp
gfx::Device& device = gfx::CreateDevice(
{
    .app_name = "hello",
    .window = hwnd,
});

struct TriangleConstants
{
    float color[4];
    float time;
};

while (running)
{
    gfx::Frame frame = device.NewFrame();
    gfx::CommandBuffer& cmd = frame.cmd;

    const gfx::Handle<gfx::Texture> backbuffer = device.GetSwapchainTexture();
    if (backbuffer.IsValid())
    {
        cmd.BeginRenderPass(
        {
            .color_targets =
            {
                {
                    .texture = backbuffer,
                    .clear_color = { 0.1f, 0.1f, 0.1f, 1.0f },
                },
            },
        });
        cmd.BindPipeline(pipeline, gfx::BindPoint::Graphics);
        cmd.PushConstants(TriangleConstants
        {
            .color = { 1.0f, 0.5f, 0.2f, 1.0f },
            .time = time,
        });
        cmd.Draw(3, 1);
        cmd.EndRenderPass();
    }
    device.SubmitFrame(frame);
}

gfx::DestroyDevice(device);
```

## Shared structs
Write a struct once in a header that both C++ and GLSL include:

```cpp
// shared.h
#include "gfx_shared.h"

struct FrameData
{
    mat4 view_projection;
    vec4 camera_position;
    float time;
};
GFX_LAYOUT(FrameData, GFX_MEMBERS(view_projection, camera_position, time))
```

```cpp
cmd.SetGlobalConstants(FrameData{ .view_projection = view_projection, .camera_position = camera_position, .time = time });
```

```glsl
#include "shared.h"

layout(set = 1, binding = 0) uniform Globals { FrameData frame; };
```
`GFX_LAYOUT` checks the std430 layout at compile time and against the shader at runtime

## Configuration
Define these before including `gfx.h`

| Option | Default | |
|---|---|---|
| `GFX_WINDOW_WIN32` / `GFX_WINDOW_GLFW` | headless | Window type of `DeviceConfig::window` |
| `GFX_DEBUG` | on unless `NDEBUG` | Debug checks and logging |
| `GFX_ENABLE_VALIDATION` | `GFX_DEBUG` | Validation layer, debug names and labels |
| `GFX_USE_SPIRV_REFLECT` | off | Shader reflection, `GFX_LAYOUT` checks against shaders |
| `GFX_USE_VMA` | off | VMA instead of the built-in allocator |
| `GFX_USE_GLM` | off | glm types for vectors |
| `GFX_HANDLE_64` | off | 64 bit handles instead of 32 bit |
| `GFX_LOG_FN` | none | add `#define GFX_LOG_FN gfx::GFX_Log` for logging |


## Optional includes
Include `gfx_bindless.h` from shaders: it declares the bindless arrays for GLSL

If you want to use layout validation, include `gfx_shared.h` from C++ and GLSL

## Performance
CPU time to record and submit a frame compared to NVRHI, Slang RHI and Filament.

<p align="center">
  <img src="images/perf.png" width="98%" alt="performance">
</p>

## Samples
`samples/triangle` Vulkan triangle in 68 lines

`samples/pathtracer` GPU path tracer

`samples/rt_gi` Real-time stochastic GI with denoising


## Building the samples
Requires the Vulkan SDK (`VULKAN_SDK` set) and Visual Studio 2022+

Run `samples\build_samples.bat`

Compiled executables go to `bin/x64/Release/<sample>/`
