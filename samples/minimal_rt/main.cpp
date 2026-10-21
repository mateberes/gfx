#define _CRT_SECURE_NO_WARNINGS
#define CGLTF_IMPLEMENTATION
#include "inc/cgltf.h"
#define GFX_IMPLEMENTATION
#define GFX_WINDOW_WIN32
#include "common/samples_common.h"
#include "rt_shared.h"

#include <chrono>

constexpr const char* SCENE_PATH = "../../../../data/picapica/scene.gltf";
constexpr u32 WHITE = 0xFFFFFFFFu;

struct Primitive
{
    u32 first_vertex = 0;
    u32 num_vertices = 0;
    u32 first_index = 0;
    u32 num_indices = 0;
    vec4 base_color = {1.0f, 1.0f, 1.0f, 1.0f};
    u32 texture = 0;
    f32 alpha_cutoff = 0.0f;
};

struct Instance
{
    f32 transform[3][4] = {};
    u32 primitive = 0;
};

struct SceneData
{
    bool Load(const std::string& path);

    std::vector<f32> positions;
    std::vector<f32> normals;
    std::vector<f32> uvs;
    std::vector<u32> indices;
    std::vector<Primitive> primitives;
    std::vector<Instance> instances;
    std::vector<std::string> textures;
    vec3 sun_direction = {-0.44f, 0.88f, -0.18f};
};

bool SceneData::Load(const std::string& path)
{
    const cgltf_options options = {};
    cgltf_data* data = nullptr;
    
    if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success)
        return false;
    
    if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success)
    {
        cgltf_free(data);
        return false;
    }

    const std::string folder = path.substr(0, path.find_last_of("\\/") + 1);
    std::vector<u32> first_primitive(data->meshes_count, 0);

    for (cgltf_size i = 0; i < data->meshes_count; ++i)
    {
        first_primitive[i] = static_cast<u32>(primitives.size());
        for (cgltf_size j = 0; j < data->meshes[i].primitives_count; ++j)
        {
            const cgltf_primitive& source = data->meshes[i].primitives[j];
            const cgltf_material* material = source.material;
            const cgltf_size num_vertices = source.attributes[0].data->count;
            
            Primitive primitive =
            {
                .first_vertex = static_cast<u32>(positions.size() / 3),
                .num_vertices = static_cast<u32>(num_vertices),
                .first_index = static_cast<u32>(indices.size()),
                .num_indices = static_cast<u32>(source.indices->count),
            };

            positions.resize(positions.size() + num_vertices * 3, 0.0f);
            normals.resize(normals.size() + num_vertices * 3, 0.0f);
            uvs.resize(uvs.size() + num_vertices * 2, 0.0f);
            
            for (cgltf_size k = 0; k < source.attributes_count; ++k)
            {
                const cgltf_attribute& attribute = source.attributes[k];
                if (attribute.type == cgltf_attribute_type_position)
                    cgltf_accessor_unpack_floats(attribute.data, positions.data() + primitive.first_vertex * 3, num_vertices * 3);
                else if (attribute.type == cgltf_attribute_type_normal)
                    cgltf_accessor_unpack_floats(attribute.data, normals.data() + primitive.first_vertex * 3, num_vertices * 3);
                else if (attribute.type == cgltf_attribute_type_texcoord && attribute.index == 0)
                    cgltf_accessor_unpack_floats(attribute.data, uvs.data() + primitive.first_vertex * 2, num_vertices * 2);
            }
            
            for (cgltf_size k = 0; k < source.indices->count; ++k)
                indices.push_back(static_cast<u32>(cgltf_accessor_read_index(source.indices, k)));

            if (material)
            {
                const bool specular_glossiness = material->has_pbr_specular_glossiness;
                const cgltf_float* factor = specular_glossiness ? material->pbr_specular_glossiness.diffuse_factor : material->pbr_metallic_roughness.base_color_factor;
                const cgltf_texture* texture = specular_glossiness ? material->pbr_specular_glossiness.diffuse_texture.texture : material->pbr_metallic_roughness.base_color_texture.texture;
                primitive.base_color = {factor[0], factor[1], factor[2], factor[3]};
                if (material->alpha_mode == cgltf_alpha_mode_mask)
                    primitive.alpha_cutoff = material->alpha_cutoff;

                if (texture)
                {
                    const std::string texture_path = folder + texture->image->uri;
                    const u32 texture_index = static_cast<u32>(std::find(textures.begin(), textures.end(), texture_path) - textures.begin());
                    if (texture_index == textures.size())
                        textures.push_back(texture_path);
                    primitive.texture = texture_index + 1;
                }
            }

            primitives.push_back(primitive);
        }
    }

    for (cgltf_size i = 0; i < data->nodes_count; ++i)
    {
        const cgltf_node& node = data->nodes[i];
        cgltf_float matrix[16] = {};
        cgltf_node_transform_world(&node, matrix);

        if (node.mesh)
        {
            const u32 first = first_primitive[cgltf_mesh_index(data, node.mesh)];
            for (cgltf_size j = 0; j < node.mesh->primitives_count; ++j)
            {
                const cgltf_material* material = node.mesh->primitives[j].material;
                if (material && (material->has_transmission || material->alpha_mode == cgltf_alpha_mode_blend))
                    continue;

                Instance instance =
                {
                    .transform =
                    {
                        {matrix[0], matrix[4], matrix[8], matrix[12]},
                        {matrix[1], matrix[5], matrix[9], matrix[13]},
                        {matrix[2], matrix[6], matrix[10], matrix[14]},
                    },
                    .primitive = static_cast<u32>(first + j),
                };
                instances.push_back(instance);
            }
        }

        if (node.light && node.light->type == cgltf_light_type_directional)
            sun_direction = Normalize({matrix[8], matrix[9], matrix[10]});
    }

    cgltf_free(data);
    return true;
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

    const gfx::Handle<gfx::ShaderModule> ray_gen = LoadShader(device, "trace.rgen.spv", gfx::ShaderStage::RayGen);
    const gfx::Handle<gfx::ShaderModule> miss = LoadShader(device, "miss.rmiss.spv", gfx::ShaderStage::Miss);
    const gfx::Handle<gfx::ShaderModule> shadow_miss = LoadShader(device, "shadow.rmiss.spv", gfx::ShaderStage::Miss);
    const gfx::Handle<gfx::ShaderModule> closest_hit = LoadShader(device, "hit.rchit.spv", gfx::ShaderStage::ClosestHit);
    const gfx::Handle<gfx::ShaderModule> any_hit = LoadShader(device, "alpha.rahit.spv", gfx::ShaderStage::AnyHit);
    const gfx::Handle<gfx::ShaderModule> blit_vertex = LoadShader(device, "blit.vert.spv", gfx::ShaderStage::Vertex);
    const gfx::Handle<gfx::ShaderModule> blit_fragment = LoadShader(device, "blit.frag.spv", gfx::ShaderStage::Fragment);

    const gfx::Handle<gfx::Pipeline> trace_pipeline = device.CreateRayTracingPipeline(
    {
        .ray_gen = ray_gen,
        .ray_miss = {miss, shadow_miss},
        .hit_groups =
        {
            { .closest_hit = closest_hit, .any_hit = any_hit },
        },
        .max_recursion = 2,
    });

    const gfx::Handle<gfx::Pipeline> blit_pipeline = device.CreatePipeline(
    {
        .VS = blit_vertex,
        .FS = blit_fragment,
        .num_color_targets = 1,
        .color_targets = {{.format = device.GetSwapchain().format}},
    });

    const gfx::Handle<gfx::Sampler> sampler = device.GetSampler(gfx::DefaultSampler::LinearWrap);
    std::vector<gfx::Handle<gfx::Texture>> textures;
    textures.push_back(device.CreateTexture(
    {
        .debug_name = "white",
        .data = &WHITE,
        .texture_format = gfx::TextureFormat::RGBA8_UNorm,
        .texture_usage = gfx::TextureUsage::Sample,
        .texture_extent = {1, 1, 1},
        .sampler = sampler,
    }));
    
    for (const std::string& path : scene.textures)
        textures.push_back(LoadTexture(device, path, sampler));

    const gfx::Handle<gfx::Buffer> positions = device.CreateBuffer(
    {
        .debug_name = "positions",
        .data = scene.positions.data(),
        .buffer_size = scene.positions.size() * sizeof(f32),
        .buffer_usage = gfx::BufferUsage::Storage | gfx::BufferUsage::AccelerationStructureInput,
        .memory_type = gfx::MemoryType::GPU,
    });
    const gfx::Handle<gfx::Buffer> normals = device.CreateBuffer(
    {
        .debug_name = "normals",
        .data = scene.normals.data(),
        .buffer_size = scene.normals.size() * sizeof(f32),
        .buffer_usage = gfx::BufferUsage::Storage,
        .memory_type = gfx::MemoryType::GPU,
    });
    const gfx::Handle<gfx::Buffer> uvs = device.CreateBuffer(
    {
        .debug_name = "uvs",
        .data = scene.uvs.data(),
        .buffer_size = scene.uvs.size() * sizeof(f32),
        .buffer_usage = gfx::BufferUsage::Storage,
        .memory_type = gfx::MemoryType::GPU,
    });
    const gfx::Handle<gfx::Buffer> indices = device.CreateBuffer(
    {
        .debug_name = "indices",
        .data = scene.indices.data(),
        .buffer_size = scene.indices.size() * sizeof(u32),
        .buffer_usage = gfx::BufferUsage::Storage | gfx::BufferUsage::AccelerationStructureInput,
        .memory_type = gfx::MemoryType::GPU,
    });

    std::vector<Mesh> meshes;
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

        const gfx::Handle<gfx::AccelerationStructure> blas = device.CreateAccelerationStructure(
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
        blas_handles.push_back(blas);
    }
    setup.BuildAccelerationStructures({blas_handles.data(), blas_handles.size()});
    const gfx::Handle<gfx::Buffer> mesh_buffer = device.CreateBuffer(
    {
        .debug_name = "meshes",
        .data = meshes.data(),
        .buffer_size = meshes.size() * sizeof(Mesh),
        .buffer_usage = gfx::BufferUsage::Storage,
        .memory_type = gfx::MemoryType::GPU,
    });

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
    const gfx::Handle<gfx::Buffer> instance_buffer = device.CreateBuffer(
    {
        .debug_name = "instances",
        .data = instances.data(),
        .buffer_size = instances.size() * sizeof(gfx::AccelerationStructureInstance),
        .buffer_usage = gfx::BufferUsage::AccelerationStructureInput,
        .memory_type = gfx::MemoryType::GPU,
    });

    const gfx::Handle<gfx::AccelerationStructure> top_level = device.CreateAccelerationStructure(
    {
        .debug_name = "scene",
        .level = gfx::AccelerationLevel::Top,
        .build = gfx::AccelerationBuild::FastTrace,
        .max_instances = static_cast<u32>(instances.size()),
    });
    setup.BuildAccelerationStructure(top_level, instance_buffer, 0, static_cast<u32>(instances.size()));
    device.WaitFor(device.Submit(setup));

    Camera camera =
    {
        .pos = {-2.0f, 21.0f, 38.0f},
        .pitch = -0.42f,
    };
    
    gfx::Vec2u image_size = {};
    gfx::Handle<gfx::Texture> image;
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
            device.Destroy(image);
            image_size = size;
            image = device.CreateTexture(
            {
                .debug_name = "rt image",
                .texture_format = gfx::TextureFormat::RGBA8_UNorm,
                .texture_usage = gfx::TextureUsage::Sample | gfx::TextureUsage::Storage,
                .texture_extent = {size.x, size.y, 1},
            });
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

    device.Destroy(image);
    device.Destroy(top_level);
    device.Destroy(instance_buffer);
    device.Destroy(mesh_buffer);
    device.Destroy(indices);
    device.Destroy(uvs);
    device.Destroy(normals);
    device.Destroy(positions);
    device.Destroy(blit_pipeline);
    device.Destroy(trace_pipeline);
    device.Destroy(blit_fragment);
    device.Destroy(blit_vertex);
    device.Destroy(any_hit);
    device.Destroy(closest_hit);
    device.Destroy(shadow_miss);
    device.Destroy(miss);
    device.Destroy(ray_gen);

    for (gfx::Handle<gfx::AccelerationStructure> blas : blas_handles)
        device.Destroy(blas);
    
    for (gfx::Handle<gfx::Texture> texture : textures)
        device.Destroy(texture);

    gfx::DestroyDevice(device);
    window.Close();
    return 0;
}
