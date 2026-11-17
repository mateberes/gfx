#define GFX_IMPLEMENTATION
#define GFX_WINDOW_WIN32
#include "includes/samples_common.h"
#include "pt_shared.h"

#include <chrono>

constexpr const char* BISTRO_PATH = "../../../../data/bistro/bistro.gltf";
constexpr const char* CAR_PATH = "../../../../data/336_lrm/scene.gltf";
constexpr u32 WHITE = 0xFFFFFFFFu;
constexpr u32 HISTORY_LIMIT = 4096;
constexpr f32 EXPOSURE = 1.0f;
constexpr vec3 SUN_COLOR = {3.0f, 2.85f, 2.6f};
constexpr f32 CAR_SCALE = 0.015f;

void Run(gfx::Device& device, Window& window, const SceneData& scene)
{
    const gfx::Owned<gfx::ShaderModule> trace_shader(LoadShader(device, "pathtrace.comp.spv", gfx::ShaderStage::Compute));
    const gfx::Owned<gfx::ShaderModule> tonemap_vertex(LoadShader(device, "tonemap.vert.spv", gfx::ShaderStage::Vertex));
    const gfx::Owned<gfx::ShaderModule> tonemap_fragment(LoadShader(device, "tonemap.frag.spv", gfx::ShaderStage::Fragment));

    const gfx::Owned<gfx::Pipeline> trace_pipeline(device.CreateComputePipeline(
    {
        .debug_name = "pathtrace",
        .CS = trace_shader,
    }));

    const gfx::Owned<gfx::Pipeline> tonemap_pipeline(device.CreatePipeline(
    {
        .debug_name = "tonemap",
        .VS = tonemap_vertex,
        .FS = tonemap_fragment,
        .num_color_targets = 1,
        .color_targets = {{.format = device.GetSwapchain().format}},
    }));

    const gfx::Handle<gfx::Sampler> sampler = device.GetSampler(gfx::DefaultSampler::LinearWrap);
    std::vector<gfx::Owned<gfx::Texture>> textures;
    textures.emplace_back(device.CreateTexture(
    {
        .debug_name = "white",
        .data = &WHITE,
        .texture_format = gfx::TextureFormat::RGBA8_UNorm,
        .texture_usage = gfx::TextureUsage::Sample,
        .texture_extent = {1, 1, 1},
        .sampler = sampler,
    }));

    for (const std::string& path : scene.textures)
        textures.emplace_back(LoadTexture(device, path, sampler));

    const gfx::Owned<gfx::Buffer> positions(device.CreateBuffer(
    {
        .debug_name = "positions",
        .data = scene.positions.data(),
        .buffer_size = scene.positions.size() * sizeof(f32),
        .buffer_usage = gfx::BufferUsage::Storage | gfx::BufferUsage::AccelerationStructureInput,
        .memory_type = gfx::MemoryType::GPU,
    }));
    const gfx::Owned<gfx::Buffer> normals(device.CreateBuffer(
    {
        .debug_name = "normals",
        .data = scene.normals.data(),
        .buffer_size = scene.normals.size() * sizeof(f32),
        .buffer_usage = gfx::BufferUsage::Storage,
        .memory_type = gfx::MemoryType::GPU,
    }));
    const gfx::Owned<gfx::Buffer> uvs(device.CreateBuffer(
    {
        .debug_name = "uvs",
        .data = scene.uvs.data(),
        .buffer_size = scene.uvs.size() * sizeof(f32),
        .buffer_usage = gfx::BufferUsage::Storage,
        .memory_type = gfx::MemoryType::GPU,
    }));
    const gfx::Owned<gfx::Buffer> indices(device.CreateBuffer(
    {
        .debug_name = "indices",
        .data = scene.indices.data(),
        .buffer_size = scene.indices.size() * sizeof(u32),
        .buffer_usage = gfx::BufferUsage::Storage | gfx::BufferUsage::AccelerationStructureInput,
        .memory_type = gfx::MemoryType::GPU,
    }));

    std::vector<Mesh> meshes;
    std::vector<gfx::Owned<gfx::AccelerationStructure>> blas;
    std::vector<gfx::Handle<gfx::AccelerationStructure>> blas_handles;
    gfx::CommandBuffer setup = device.AcquireCommandBuffer();

    for (const Primitive& primitive : scene.primitives)
    {
        meshes.push_back(
        {
            .albedo = primitive.base_color,
            .emissive = primitive.emissive,
            .positions = {.address = positions->GetBufferAddress() + primitive.first_vertex * 3 * sizeof(f32)},
            .normals = {.address = normals->GetBufferAddress() + primitive.first_vertex * 3 * sizeof(f32)},
            .uvs = {.address = uvs->GetBufferAddress() + primitive.first_vertex * 2 * sizeof(f32)},
            .indices = {.address = indices->GetBufferAddress() + primitive.first_index * sizeof(u32)},
            .albedo_index = textures[primitive.texture].Index(),
            .alpha_cutoff = primitive.alpha_cutoff,
            .metallic = primitive.metallic,
            .roughness = primitive.roughness,
        });

        const gfx::Handle<gfx::AccelerationStructure> handle = device.CreateAccelerationStructure(
        {
            .geometry =
            {
                .vertex_buffer = positions,
                .vertex_offset = primitive.first_vertex * 3 * sizeof(f32),
                .num_vertices = primitive.num_vertices,
                .index_buffer = indices,
                .index_offset = primitive.first_index * sizeof(u32),
                .num_triangles = primitive.num_indices / 3,
                .opaque = primitive.alpha_cutoff == 0.0f,
            },
        });
        blas.emplace_back(handle);
        blas_handles.push_back(handle);
    }
    setup.BuildAccelerationStructures({blas_handles.data(), blas_handles.size()});

    const gfx::Owned<gfx::Buffer> mesh_buffer(device.CreateBuffer(
    {
        .debug_name = "meshes",
        .data = meshes.data(),
        .buffer_size = meshes.size() * sizeof(Mesh),
        .buffer_usage = gfx::BufferUsage::Storage,
        .memory_type = gfx::MemoryType::GPU,
    }));

    std::vector<gfx::AccelerationStructureInstance> instances;
    for (const Instance& source : scene.instances)
    {
        gfx::AccelerationStructureInstance instance =
        {
            .acceleration_structure_address = blas_handles[source.primitive]->device_address,
        };
        std::memcpy(instance.transform, source.transform, sizeof(instance.transform));
        instance.custom_index = source.primitive;
        instances.push_back(instance);
    }
    const gfx::Owned<gfx::Buffer> instance_buffer(device.CreateBuffer(
    {
        .debug_name = "instances",
        .data = instances.data(),
        .buffer_size = instances.size() * sizeof(gfx::AccelerationStructureInstance),
        .buffer_usage = gfx::BufferUsage::AccelerationStructureInput,
        .memory_type = gfx::MemoryType::GPU,
    }));

    const gfx::Owned<gfx::AccelerationStructure> top_level(device.CreateAccelerationStructure(
    {
        .debug_name = "scene",
        .level = gfx::AccelerationLevel::Top,
        .build = gfx::AccelerationBuild::FastTrace,
        .max_instances = static_cast<u32>(instances.size()),
    }));
    setup.BuildAccelerationStructure(top_level, instance_buffer, 0, static_cast<u32>(instances.size()));
    device.WaitFor(device.Submit(setup));

    Camera camera =
    {
        .pos = {-26.4f, 3.2f, 11.2f},
        .yaw = 1.86f,
        .pitch = -0.1f,
    };

    gfx::Vec2u radiance_size = {};
    gfx::Owned<gfx::Texture> radiance;
    u32 frame_index = 0;
    u32 sample_count = 0;
    std::chrono::steady_clock::time_point last_time = std::chrono::steady_clock::now();

    while (window.IsOpen())
    {
        const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        const f32 delta = std::chrono::duration<f32>(now - last_time).count();
        last_time = now;

        if (camera.Update(window, delta))
            sample_count = 0;

        gfx::Frame frame = device.NewFrame();
        if (!frame.swapchain_valid)
        {
            device.SubmitFrame(frame);
            continue;
        }

        const gfx::Vec2u size = frame.swapchain_size;
        if (size.x != radiance_size.x || size.y != radiance_size.y)
        {
            radiance_size = size;
            radiance = gfx::Owned<gfx::Texture>(device.CreateTexture(
            {
                .debug_name = "radiance",
                .texture_format = gfx::TextureFormat::RGBA32_Float,
                .texture_usage = gfx::TextureUsage::Sample | gfx::TextureUsage::Storage,
                .texture_extent = {size.x, size.y, 1},
            }));
            sample_count = 0;
        }

        ++frame_index;
        ++sample_count;

        char status[32] = {};
        snprintf(status, sizeof(status), "%u samples", sample_count);
        window.UpdateTitle(device, status);

        const vec3 forward = camera.GetForward();
        const vec3 right = Normalize(Cross(forward, {0.0f, 1.0f, 0.0f}));
        const vec3 up = Cross(right, forward);

        const f32 tan_half_fov = std::tan(0.5f * camera.fov);
        const f32 aspect = static_cast<f32>(size.x) / static_cast<f32>(size.y);
        const vec3 sun = scene.sun_direction;

        gfx::CommandBuffer& cmd = frame.cmd;
        cmd.SetGlobalConstants(FrameData
        {
            .position = {camera.pos.x, camera.pos.y, camera.pos.z, 0.0f},
            .forward = {forward.x, forward.y, forward.z, 0.0f},
            .right = {right.x * tan_half_fov * aspect, right.y * tan_half_fov * aspect, right.z * tan_half_fov * aspect, 0.0f},
            .up = {up.x * tan_half_fov, up.y * tan_half_fov, up.z * tan_half_fov, 0.0f},
            .sun_direction = {sun.x, sun.y, sun.z, 0.0f},
            .sun_color = {SUN_COLOR.x, SUN_COLOR.y, SUN_COLOR.z, 0.0f},
            .pixel_size = {1.0f / static_cast<f32>(size.x), 1.0f / static_cast<f32>(size.y), 0.0f, 0.0f},
            .frame_index = frame_index,
            .sample_count = sample_count,
        });

        cmd.BeginComputePass(
        {
            .debug_name = "pathtrace",
            .inputs =
            {
                { .buffer = mesh_buffer, .access = gfx::PassAccess::Read },
                { .buffer = positions, .access = gfx::PassAccess::Read },
                { .buffer = normals, .access = gfx::PassAccess::Read },
                { .buffer = uvs, .access = gfx::PassAccess::Read },
                { .buffer = indices, .access = gfx::PassAccess::Read },
            },
            .outputs =
            {
                { .texture = radiance, .access = gfx::PassAccess::ReadWrite },
            },
        });

        cmd.BindPipeline(trace_pipeline, gfx::BindPoint::Compute);
        cmd.PushConstants(TraceConstants
        {
            .meshes = {.address = mesh_buffer->GetBufferAddress()},
            .tlas_index = top_level.Index(),
            .radiance_index = radiance.Index(),
            .history_limit = HISTORY_LIMIT,
        });

        cmd.Dispatch({size.x, size.y, 1});
        cmd.EndComputePass();

        const gfx::Handle<gfx::Texture> backbuffer = device.GetSwapchainTexture();
        if (backbuffer.IsValid())
        {
            cmd.BeginRenderPass(
            {
                .debug_name = "tonemap",
                .color_targets =
                {
                    { .texture = backbuffer },
                },
                .inputs =
                {
                    { .texture = radiance, .access = gfx::PassAccess::Sampled },
                },
            });

            cmd.BindPipeline(tonemap_pipeline, gfx::BindPoint::Graphics);
            cmd.PushConstants(TonemapConstants
            {
                .radiance_index = radiance.Index(),
                .exposure = EXPOSURE,
            });
            cmd.Draw(3, 1);
            cmd.EndRenderPass();
        }

        device.SubmitFrame(frame);
    }
}

int main()
{
    SceneData scene;
    if (!scene.Load(BISTRO_PATH))
        GFX_LOG_FATAL("could not load %s", BISTRO_PATH);

    const mat4 car_transform =
    {
        .columns =
        {
            {CAR_SCALE, 0.0f, 0.0f, 0.0f},
            {0.0f, CAR_SCALE, 0.0f, 0.0f},
            {0.0f, 0.0f, CAR_SCALE, 0.0f},
            {-18.5f, 0.2f, 9.5f, 1.0f},
        },
    };
    if (!scene.Load(CAR_PATH, car_transform))
        GFX_LOG_FATAL("could not load %s", CAR_PATH);

    Window window;
    if (!window.Initialize(1280, 720, "pathtracer"))
        return 1;

    gfx::Device& device = gfx::CreateDevice(
    {
        .app_name = "pathtracer",
        .window = window.GetHandle(),
        .required_features = gfx::Feature::RayQuery,
    });

    Run(device, window, scene);

    gfx::DestroyDevice(device);
    return 0;
}
