#define GFX_IMPLEMENTATION
#define GFX_WINDOW_WIN32
#include "includes/samples_common.h"
#include "gi_shared.h"

#include <chrono>

constexpr const char* BISTRO_PATH = "../../../../data/bistro/bistro.gltf";
constexpr u32 WHITE = 0xFFFFFFFFu;
constexpr u32 HISTORY_LIMIT = 32;
constexpr u32 SVGF_ITERATIONS = 5;
constexpr f32 EXPOSURE = 1.0f;
constexpr vec3 SUN_COLOR = {7.0f, 6.2f, 5.0f};

gfx::Owned<gfx::Texture> CreateTarget(gfx::Device& device, const char* name, gfx::TextureFormat format, gfx::Vec2u size)
{
    return gfx::Owned<gfx::Texture>(device.CreateTexture(
    {
        .debug_name = name,
        .texture_format = format,
        .texture_usage = gfx::TextureUsage::Sample | gfx::TextureUsage::Storage,
        .texture_extent = {size.x, size.y, 1},
    }));
}

struct Targets
{
    void Create(gfx::Device& device, gfx::Vec2u size)
    {
        using enum gfx::TextureFormat;
        direct = CreateTarget(device, "direct", RGBA16_Float, size);
        diffuse_albedo = CreateTarget(device, "diffuse albedo", RGBA8_UNorm, size);
        specular_albedo = CreateTarget(device, "specular albedo", RGBA8_UNorm, size);
        diffuse = CreateTarget(device, "diffuse", RGBA16_Float, size);
        specular = CreateTarget(device, "specular", RGBA16_Float, size);
        motion = CreateTarget(device, "motion", RGBA32_Float, size);
        for (u32 i = 0; i < 2; ++i)
        {
            normal_depth[i] = CreateTarget(device, "normal depth", RGBA32_Float, size);
            diffuse_history[i] = CreateTarget(device, "diffuse history", RGBA16_Float, size);
            specular_history[i] = CreateTarget(device, "specular history", RGBA16_Float, size);
            moments_history[i] = CreateTarget(device, "moments history", RGBA32_Float, size);
            diffuse_filtered[i] = CreateTarget(device, "diffuse filtered", RGBA16_Float, size);
            specular_filtered[i] = CreateTarget(device, "specular filtered", RGBA16_Float, size);
        }
    }

    gfx::Owned<gfx::Texture> direct;
    gfx::Owned<gfx::Texture> diffuse_albedo;
    gfx::Owned<gfx::Texture> specular_albedo;
    gfx::Owned<gfx::Texture> diffuse;
    gfx::Owned<gfx::Texture> specular;
    gfx::Owned<gfx::Texture> motion;
    gfx::Owned<gfx::Texture> normal_depth[2];
    gfx::Owned<gfx::Texture> diffuse_history[2];
    gfx::Owned<gfx::Texture> specular_history[2];
    gfx::Owned<gfx::Texture> moments_history[2];
    gfx::Owned<gfx::Texture> diffuse_filtered[2];
    gfx::Owned<gfx::Texture> specular_filtered[2];
};

void Run(gfx::Device& device, Window& window, const SceneData& scene)
{
    const gfx::Owned<gfx::ShaderModule> trace_shader(LoadShader(device, "trace.comp.spv", gfx::ShaderStage::Compute));
    const gfx::Owned<gfx::ShaderModule> temporal_shader(LoadShader(device, "temporal.comp.spv", gfx::ShaderStage::Compute));
    const gfx::Owned<gfx::ShaderModule> svgf_shader(LoadShader(device, "svgf.comp.spv", gfx::ShaderStage::Compute));
    const gfx::Owned<gfx::ShaderModule> fullscreen_vertex(LoadShader(device, "fullscreen.vert.spv", gfx::ShaderStage::Vertex));
    const gfx::Owned<gfx::ShaderModule> composite_fragment(LoadShader(device, "composite.frag.spv", gfx::ShaderStage::Fragment));

    const gfx::Owned<gfx::Pipeline> trace_pipeline(device.CreateComputePipeline(
    {
        .debug_name = "trace",
        .CS = trace_shader,
    }));

    const gfx::Owned<gfx::Pipeline> temporal_pipeline(device.CreateComputePipeline(
    {
        .debug_name = "temporal",
        .CS = temporal_shader,
    }));

    const gfx::Owned<gfx::Pipeline> svgf_pipeline(device.CreateComputePipeline(
    {
        .debug_name = "svgf",
        .CS = svgf_shader,
    }));

    const gfx::Owned<gfx::Pipeline> composite_pipeline(device.CreatePipeline(
    {
        .debug_name = "composite",
        .VS = fullscreen_vertex,
        .FS = composite_fragment,
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

    for (const std::string& texture : scene.textures)
        textures.emplace_back(LoadTexture(device, texture, sampler));

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
        if (scene.primitives[source.primitive].transmission > 0.0f)
            continue;

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

    gfx::Vec2u targets_size = {};
    Targets targets;
    bool reset_history = true;
    u32 frame_index = 0;
    vec4 prev_position = {};
    mat4 prev_view_projection = {};
    std::chrono::steady_clock::time_point last_time = std::chrono::steady_clock::now();

    while (window.IsOpen())
    {
        const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        const f32 delta = std::chrono::duration<f32>(now - last_time).count();
        last_time = now;

        camera.Update(window, delta);

        gfx::Frame frame = device.NewFrame();
        if (!frame.swapchain_valid)
        {
            device.SubmitFrame(frame);
            continue;
        }

        const gfx::Vec2u size = frame.swapchain_size;
        if (size.x != targets_size.x || size.y != targets_size.y)
        {
            targets_size = size;
            targets.Create(device, size);
            reset_history = true;
        }

        ++frame_index;
        const u32 current = frame_index & 1;
        const u32 previous = current ^ 1;

        window.UpdateTitle(device, "SVGF");

        const vec3 forward = camera.GetForward();
        const vec3 right = Normalize(Cross(forward, {0.0f, 1.0f, 0.0f}));
        const vec3 up = Cross(right, forward);

        const f32 tan_half_fov = std::tan(0.5f * camera.fov);
        const f32 aspect = static_cast<f32>(size.x) / static_cast<f32>(size.y);
        const vec3 sun = scene.sun_direction;

        const vec4 position = {camera.pos.x, camera.pos.y, camera.pos.z, 0.0f};
        const vec4 forward_axis = {forward.x, forward.y, forward.z, 0.0f};
        const vec4 right_axis = {right.x * tan_half_fov * aspect, right.y * tan_half_fov * aspect, right.z * tan_half_fov * aspect, 0.0f};
        const vec4 up_axis = {up.x * tan_half_fov, up.y * tan_half_fov, up.z * tan_half_fov, 0.0f};
        const mat4 view_projection = GetViewProjectionMatrix(camera.pos, forward, right, up, tan_half_fov, aspect);
        if (reset_history)
        {
            prev_position = position;
            prev_view_projection = view_projection;
        }

        gfx::CommandBuffer& cmd = frame.cmd;
        cmd.SetGlobalConstants(FrameData
        {
            .position = position,
            .forward = forward_axis,
            .right = right_axis,
            .up = up_axis,
            .prev_position = prev_position,
            .prev_view_projection = prev_view_projection,
            .sun_direction = {sun.x, sun.y, sun.z, 0.0f},
            .sun_color = {SUN_COLOR.x, SUN_COLOR.y, SUN_COLOR.z, 0.0f},
            .pixel_size = {1.0f / static_cast<f32>(size.x), 1.0f / static_cast<f32>(size.y), 0.0f, 0.0f},
            .frame_index = frame_index,
        });

        prev_position = position;
        prev_view_projection = view_projection;

        cmd.BeginComputePass(
        {
            .debug_name = "trace",
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
                { .texture = targets.direct, .access = gfx::PassAccess::Write },
                { .texture = targets.diffuse_albedo, .access = gfx::PassAccess::Write },
                { .texture = targets.specular_albedo, .access = gfx::PassAccess::Write },
                { .texture = targets.diffuse, .access = gfx::PassAccess::Write },
                { .texture = targets.specular, .access = gfx::PassAccess::Write },
                { .texture = targets.motion, .access = gfx::PassAccess::Write },
                { .texture = targets.normal_depth[current], .access = gfx::PassAccess::Write },
            },
        });

        cmd.BindPipeline(trace_pipeline, gfx::BindPoint::Compute);
        cmd.PushConstants(TraceConstants
        {
            .meshes = {.address = mesh_buffer->GetBufferAddress()},
            .tlas_index = top_level.Index(),
            .direct_index = targets.direct.Index(),
            .diffuse_albedo_index = targets.diffuse_albedo.Index(),
            .specular_albedo_index = targets.specular_albedo.Index(),
            .diffuse_index = targets.diffuse.Index(),
            .specular_index = targets.specular.Index(),
            .motion_index = targets.motion.Index(),
            .normal_depth_index = targets.normal_depth[current].Index(),
        });
        cmd.Dispatch({size.x, size.y, 1});
        cmd.EndComputePass();

        cmd.BeginComputePass(
        {
            .debug_name = "temporal",
            .inputs =
            {
                { .texture = targets.diffuse, .access = gfx::PassAccess::Read },
                { .texture = targets.specular, .access = gfx::PassAccess::Read },
                { .texture = targets.specular_albedo, .access = gfx::PassAccess::Read },
                { .texture = targets.motion, .access = gfx::PassAccess::Read },
                { .texture = targets.normal_depth[current], .access = gfx::PassAccess::Read },
                { .texture = targets.normal_depth[previous], .access = gfx::PassAccess::Read },
                { .texture = targets.diffuse_history[previous], .access = gfx::PassAccess::Read },
                { .texture = targets.specular_history[previous], .access = gfx::PassAccess::Read },
                { .texture = targets.moments_history[previous], .access = gfx::PassAccess::Read },
            },
            .outputs =
            {
                { .texture = targets.diffuse_history[current], .access = gfx::PassAccess::Write },
                { .texture = targets.specular_history[current], .access = gfx::PassAccess::Write },
                { .texture = targets.moments_history[current], .access = gfx::PassAccess::Write },
                { .texture = targets.diffuse_filtered[0], .access = gfx::PassAccess::Write },
                { .texture = targets.specular_filtered[0], .access = gfx::PassAccess::Write },
            },
        });

        cmd.BindPipeline(temporal_pipeline, gfx::BindPoint::Compute);
        cmd.PushConstants(TemporalConstants
        {
            .diffuse_index = targets.diffuse.Index(),
            .specular_index = targets.specular.Index(),
            .specular_albedo_index = targets.specular_albedo.Index(),
            .motion_index = targets.motion.Index(),
            .normal_depth_index = targets.normal_depth[current].Index(),
            .prev_normal_depth_index = targets.normal_depth[previous].Index(),
            .diffuse_history_index = targets.diffuse_history[current].Index(),
            .prev_diffuse_history_index = targets.diffuse_history[previous].Index(),
            .specular_history_index = targets.specular_history[current].Index(),
            .prev_specular_history_index = targets.specular_history[previous].Index(),
            .moments_history_index = targets.moments_history[current].Index(),
            .prev_moments_history_index = targets.moments_history[previous].Index(),
            .diffuse_output_index = targets.diffuse_filtered[0].Index(),
            .specular_output_index = targets.specular_filtered[0].Index(),
            .history_limit = HISTORY_LIMIT,
            .reset = reset_history ? 1u : 0u,
        });
        cmd.Dispatch({size.x, size.y, 1});
        cmd.EndComputePass();
        reset_history = false;

        u32 source = 0;
        for (u32 i = 0; i < SVGF_ITERATIONS; ++i)
        {
            const u32 destination = source ^ 1;
            cmd.BeginComputePass(
            {
                .debug_name = "svgf",
                .inputs =
                {
                    { .texture = targets.diffuse_filtered[source], .access = gfx::PassAccess::Read },
                    { .texture = targets.specular_filtered[source], .access = gfx::PassAccess::Read },
                    { .texture = targets.normal_depth[current], .access = gfx::PassAccess::Read },
                },
                .outputs =
                {
                    { .texture = targets.diffuse_filtered[destination], .access = gfx::PassAccess::Write },
                    { .texture = targets.specular_filtered[destination], .access = gfx::PassAccess::Write },
                },
            });

            cmd.BindPipeline(svgf_pipeline, gfx::BindPoint::Compute);
            cmd.PushConstants(SvgfConstants
            {
                .diffuse_input_index = targets.diffuse_filtered[source].Index(),
                .specular_input_index = targets.specular_filtered[source].Index(),
                .diffuse_output_index = targets.diffuse_filtered[destination].Index(),
                .specular_output_index = targets.specular_filtered[destination].Index(),
                .normal_depth_index = targets.normal_depth[current].Index(),
                .step_size = 1u << i,
            });
            cmd.Dispatch({size.x, size.y, 1});
            cmd.EndComputePass();
            source = destination;
        }

        const gfx::Handle<gfx::Texture> backbuffer = device.GetSwapchainTexture();
        if (backbuffer.IsValid())
        {
            cmd.BeginRenderPass(
            {
                .debug_name = "composite",
                .color_targets =
                {
                    { .texture = backbuffer },
                },
                .inputs =
                {
                    { .texture = targets.direct, .access = gfx::PassAccess::Sampled },
                    { .texture = targets.diffuse_albedo, .access = gfx::PassAccess::Sampled },
                    { .texture = targets.specular_albedo, .access = gfx::PassAccess::Sampled },
                    { .texture = targets.diffuse_filtered[source], .access = gfx::PassAccess::Sampled },
                    { .texture = targets.specular_filtered[source], .access = gfx::PassAccess::Sampled },
                },
            });

            cmd.BindPipeline(composite_pipeline, gfx::BindPoint::Graphics);
            cmd.PushConstants(CompositeConstants
            {
                .direct_index = targets.direct.Index(),
                .diffuse_albedo_index = targets.diffuse_albedo.Index(),
                .specular_albedo_index = targets.specular_albedo.Index(),
                .diffuse_index = targets.diffuse_filtered[source].Index(),
                .specular_index = targets.specular_filtered[source].Index(),
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
    std::filesystem::path p = std::filesystem::absolute(BISTRO_PATH);
    if (!scene.Load(p.string()))
        GFX_LOG_FATAL("could not load %s", BISTRO_PATH);

    Window window;
    if (!window.Initialize(1280, 720, "rt gi"))
        return 1;

    gfx::Device& device = gfx::CreateDevice(
    {
        .app_name = "rt gi",
        .window = window.GetHandle(),
        .required_features = gfx::Feature::RayQuery,
    });

    Run(device, window, scene);

    gfx::DestroyDevice(device);
    return 0;
}
