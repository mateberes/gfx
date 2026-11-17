#define GFX_IMPLEMENTATION
#define GFX_WINDOW_WIN32
#include "includes/samples_common.h"
#include "rt_shared.h"

#include <chrono>

constexpr const char* SCENE_PATH = "../../../../data/picapica/scene.gltf";
constexpr u32 WHITE = 0xFFFFFFFFu;

void Run(gfx::Device& device, Window& window, const SceneData& scene)
{
    const gfx::Owned<gfx::ShaderModule> ray_gen(LoadShader(device, "trace.rgen.spv", gfx::ShaderStage::RayGen));
    const gfx::Owned<gfx::ShaderModule> miss(LoadShader(device, "miss.rmiss.spv", gfx::ShaderStage::Miss));
    const gfx::Owned<gfx::ShaderModule> shadow_miss(LoadShader(device, "shadow.rmiss.spv", gfx::ShaderStage::Miss));
    const gfx::Owned<gfx::ShaderModule> closest_hit(LoadShader(device, "hit.rchit.spv", gfx::ShaderStage::ClosestHit));
    const gfx::Owned<gfx::ShaderModule> any_hit(LoadShader(device, "alpha.rahit.spv", gfx::ShaderStage::AnyHit));
    const gfx::Owned<gfx::ShaderModule> blit_vertex(LoadShader(device, "blit.vert.spv", gfx::ShaderStage::Vertex));
    const gfx::Owned<gfx::ShaderModule> blit_fragment(LoadShader(device, "blit.frag.spv", gfx::ShaderStage::Fragment));

    const gfx::Owned<gfx::Pipeline> trace_pipeline(device.CreateRayTracingPipeline(
    {
        .ray_gen = ray_gen,
        .ray_miss = {miss, shadow_miss},
        .hit_groups =
        {
            { .closest_hit = closest_hit, .any_hit = any_hit },
        },
        .max_recursion = 2,
    }));

    const gfx::Owned<gfx::Pipeline> blit_pipeline(device.CreatePipeline(
    {
        .VS = blit_vertex,
        .FS = blit_fragment,
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
            .positions = {.address = positions->GetBufferAddress() + primitive.first_vertex * 3 * sizeof(f32)},
            .normals = {.address = normals->GetBufferAddress() + primitive.first_vertex * 3 * sizeof(f32)},
            .uvs = {.address = uvs->GetBufferAddress() + primitive.first_vertex * 2 * sizeof(f32)},
            .indices = {.address = indices->GetBufferAddress() + primitive.first_index * sizeof(u32)},
            .albedo_index = textures[primitive.texture].Index(),
            .alpha_cutoff = primitive.alpha_cutoff,
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
        .pos = {-2.0f, 21.0f, 38.0f},
        .pitch = -0.42f,
    };
    
    gfx::Vec2u image_size = {};
    gfx::Owned<gfx::Texture> image;
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
        if (size.x != image_size.x || size.y != image_size.y)
        {
            image_size = size;
            image = gfx::Owned<gfx::Texture>(device.CreateTexture(
            {
                .debug_name = "rt image",
                .texture_format = gfx::TextureFormat::RGBA8_UNorm,
                .texture_usage = gfx::TextureUsage::Sample | gfx::TextureUsage::Storage,
                .texture_extent = {size.x, size.y, 1},
            }));
        }

        const vec3 forward = camera.GetForward();
        const vec3 right = Normalize(Cross(forward, {0.0f, 1.0f, 0.0f}));
        const vec3 up = Cross(right, forward);
        
        const f32 tan_half_fov = std::tan(0.5f * camera.fov);
        const f32 aspect = static_cast<f32>(size.x) / static_cast<f32>(size.y);
        const vec3 light = scene.sun_direction;

        gfx::CommandBuffer& cmd = frame.cmd;
        cmd.SetGlobalConstants(FrameData
        {
            .position = {camera.pos.x, camera.pos.y, camera.pos.z, 0.0f},
            .forward = {forward.x, forward.y, forward.z, 0.0f},
            .right = {right.x * tan_half_fov * aspect, right.y * tan_half_fov * aspect, right.z * tan_half_fov * aspect, 0.0f},
            .up = {up.x * tan_half_fov, up.y * tan_half_fov, up.z * tan_half_fov, 0.0f},
            .light_direction = {light.x, light.y, light.z, 0.0f},
            .pixel_spread = 2.0f * tan_half_fov / static_cast<f32>(size.y),
        });
        
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
                { .texture = image, .access = gfx::PassAccess::Write },
            },
            .stages = gfx::PipelineStage::RayTracing,
        });
        
        cmd.BindPipeline(trace_pipeline, gfx::BindPoint::RayTracing);
        cmd.PushConstants(TraceConstants
        {
            .meshes = {.address = mesh_buffer->GetBufferAddress()},
            .tlas_index = top_level.Index(),
            .image_index = image.Index(),
        });
        
        cmd.TraceRays(size.x, size.y);
        cmd.EndComputePass();

        const gfx::Handle<gfx::Texture> backbuffer = device.GetSwapchainTexture();
        if (backbuffer.IsValid())
        {
            cmd.BeginRenderPass(
            {
                .debug_name = "blit",
                .color_targets =
                { 
                    { .texture = backbuffer },
                },
                .inputs =
                {
                    { .texture = image, .access = gfx::PassAccess::Sampled },
                },
            });

            cmd.BindPipeline(blit_pipeline, gfx::BindPoint::Graphics);
            cmd.PushConstants(BlitConstants{ .image_index = image.Index() });
            cmd.Draw(3, 1);
            cmd.EndRenderPass();
        }

        device.SubmitFrame(frame);
    }
}

int main()
{
    SceneData scene;
    if (!scene.Load(SCENE_PATH))
        GFX_LOG_FATAL("could not load %s", SCENE_PATH);

    Window window;
    if (!window.Initialize(1280, 720, "minimal_rt"))
        return 1;

    gfx::Device& device = gfx::CreateDevice(
    {
        .app_name = "minimal_rt",
        .window = window.GetHandle(),
        .required_features = gfx::Feature::RayTracing,
    });

    Run(device, window, scene);

    gfx::DestroyDevice(device);
    return 0;
}
