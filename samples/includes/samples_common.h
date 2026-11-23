#pragma once

#define _USE_MATH_DEFINES
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef GFX_LOG_FN
#define GFX_LOG_FN gfx::GFX_Log
#endif
#include "gfx.h"
#include "gfx_shared.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include "stb_image.h"

#define TINYDDSLOADER_IMPLEMENTATION
#include "tinyddsloader.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

constexpr const char* WINDOW_CLASS_NAME = "gfx_window";
constexpr f32 DEFAULT_FOV = static_cast<f32>(M_PI / 3.0);
constexpr f32 MAX_PITCH = 1.6f;
constexpr f32 MOVE_SPEED = 4.0f;
constexpr f32 LOOK_SPEED = 0.003f;
constexpr std::chrono::milliseconds TITLE_UPDATE_INTERVAL = std::chrono::milliseconds(250);

inline std::vector<uint8_t> ReadFile(const char* path)
{
    std::vector<uint8_t> bytes;

    FILE* file = nullptr;
    if (fopen_s(&file, path, "rb") != 0 || !file)
        return bytes;

    fseek(file, 0, SEEK_END);
    const long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size > 0)
    {
        bytes.resize(static_cast<size_t>(size));
        if (fread(bytes.data(), 1, bytes.size(), file) != bytes.size())
            bytes.clear();
    }

    fclose(file);
    return bytes;
}

inline gfx::Handle<gfx::ShaderModule> LoadShader(gfx::Device& device, const char* path, gfx::ShaderStage stage)
{
    std::vector<uint8_t> code = ReadFile(path);
    if (code.empty())
        GFX_LOG_FATAL("%s not found, run from the output directory", path);

    return device.CreateShader(
    {
        .debug_name = path,
        .byte_code = std::move(code),
        .stage = stage,
    });
}

inline gfx::TextureFormat DdsFormat(tinyddsloader::DDSFile::DXGIFormat format)
{
    switch (format)
    {
    case tinyddsloader::DDSFile::DXGIFormat::BC1_UNorm:
        return gfx::TextureFormat::BC1_UNorm;
    case tinyddsloader::DDSFile::DXGIFormat::BC1_UNorm_SRGB:
        return gfx::TextureFormat::BC1_sRGB;
    case tinyddsloader::DDSFile::DXGIFormat::BC3_UNorm:
        return gfx::TextureFormat::BC3_UNorm;
    case tinyddsloader::DDSFile::DXGIFormat::BC3_UNorm_SRGB:
        return gfx::TextureFormat::BC3_sRGB;
    case tinyddsloader::DDSFile::DXGIFormat::BC7_UNorm:
        return gfx::TextureFormat::BC7_UNorm;
    case tinyddsloader::DDSFile::DXGIFormat::BC7_UNorm_SRGB:
        return gfx::TextureFormat::BC7_sRGB;
    default:
        GFX_LOG_FATAL("unsupported DDS format %u", static_cast<u32>(format));
        return gfx::TextureFormat::None;
    }
}

inline gfx::Handle<gfx::Texture> LoadDDS(gfx::Device& device, const std::string& path, gfx::Handle<gfx::Sampler> sampler)
{
    tinyddsloader::DDSFile dds;
    if (dds.Load(path.c_str()) != tinyddsloader::Result::Success || dds.GetArraySize() != 1)
        GFX_LOG_FATAL("could not load %s", path.c_str());

    return device.CreateTexture(
    {
        .debug_name = path.c_str(),
        .data = dds.GetImageData(0, 0)->m_mem,
        .texture_format = DdsFormat(dds.GetFormat()),
        .texture_usage = gfx::TextureUsage::Sample,
        .texture_extent = {dds.GetWidth(), dds.GetHeight(), 1},
        .num_mipmaps = dds.GetMipCount(),
        .sampler = sampler,
    });
}

inline gfx::Handle<gfx::Texture> LoadTexture(gfx::Device& device, const std::string& path, gfx::Handle<gfx::Sampler> sampler)
{
    if (std::filesystem::path(path).extension() == ".dds")
        return LoadDDS(device, path, sampler);

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!pixels)
        GFX_LOG_FATAL("could not load %s", path.c_str());

    const gfx::Handle<gfx::Texture> texture = device.CreateTexture(
    {
        .debug_name = path.c_str(),
        .data = pixels,
        .texture_format = gfx::TextureFormat::RGBA8_sRGB,
        .texture_usage = gfx::TextureUsage::Sample,
        .texture_extent = {static_cast<u32>(width), static_cast<u32>(height), 1},
        .sampler = sampler,
    });

    stbi_image_free(pixels);
    return texture;
}

inline vec3 Normalize(vec3 v)
{
    const f32 length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return {v.x / length, v.y / length, v.z / length};
}

inline vec3 Cross(vec3 a, vec3 b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline mat4 GetViewProjectionMatrix(vec3 position, vec3 forward, vec3 right, vec3 up, f32 tan_half_fov, f32 aspect)
{
    const f32 scale_x = 1.0f / (tan_half_fov * aspect);
    const f32 scale_y = -1.0f / tan_half_fov;
    const f32 right_offset = right.x * position.x + right.y * position.y + right.z * position.z;
    const f32 up_offset = up.x * position.x + up.y * position.y + up.z * position.z;
    const f32 forward_offset = forward.x * position.x + forward.y * position.y + forward.z * position.z;
    return
    {
        .columns =
        {
            {right.x * scale_x, up.x * scale_y, 0.0f, forward.x},
            {right.y * scale_x, up.y * scale_y, 0.0f, forward.y},
            {right.z * scale_x, up.z * scale_y, 0.0f, forward.z},
            {-right_offset * scale_x, -up_offset * scale_y, 0.0f, -forward_offset},
        },
    };
}

struct Window
{
    Window() = default;
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    ~Window()
    {
        Close();
    }

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
    {
        if (message == WM_NCCREATE)
        {
            const CREATESTRUCTA* create = reinterpret_cast<const CREATESTRUCTA*>(lparam);
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
            return DefWindowProcA(hwnd, message, wparam, lparam);
        }

        Window* window = reinterpret_cast<Window*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
        if (!window)
            return DefWindowProcA(hwnd, message, wparam, lparam);

        switch (message)
        {
        case WM_KEYDOWN:
            window->keys[wparam & 0xFF] = true;
            if (wparam == VK_ESCAPE)
                window->is_open = false;
            return 0;

        case WM_KEYUP:
            window->keys[wparam & 0xFF] = false;
            return 0;

        case WM_KILLFOCUS:
            memset(window->keys, 0, sizeof(window->keys));
            window->mouse_look = false;
            return 0;

        case WM_RBUTTONDOWN:
            window->mouse_look = true;
            window->mouse_x = static_cast<int16_t>(LOWORD(lparam));
            window->mouse_y = static_cast<int16_t>(HIWORD(lparam));
            SetCapture(hwnd);
            return 0;

        case WM_RBUTTONUP:
            window->mouse_look = false;
            ReleaseCapture();
            return 0;

        case WM_MOUSEMOVE:
            if (window->mouse_look)
            {
                window->mouse_delta_x += static_cast<int16_t>(LOWORD(lparam)) - window->mouse_x;
                window->mouse_delta_y += static_cast<int16_t>(HIWORD(lparam)) - window->mouse_y;
            }
            window->mouse_x = static_cast<int16_t>(LOWORD(lparam));
            window->mouse_y = static_cast<int16_t>(HIWORD(lparam));
            return 0;

        case WM_CLOSE:
            window->is_open = false;
            return 0;
        }

        return DefWindowProcA(hwnd, message, wparam, lparam);
    }

    bool Initialize(uint32_t width, uint32_t height, const char* title_text)
    {
        const HINSTANCE instance = GetModuleHandleA(nullptr);

        const WNDCLASSEXA window_class =
        {
            .cbSize = sizeof(WNDCLASSEXA),
            .style = CS_HREDRAW | CS_VREDRAW,
            .lpfnWndProc = WindowProc,
            .hInstance = instance,
            .hCursor = LoadCursor(nullptr, IDC_ARROW),
            .lpszClassName = WINDOW_CLASS_NAME,
        };
        if (!RegisterClassExA(&window_class))
            return false;

        const DWORD style = WS_OVERLAPPEDWINDOW;
        RECT rect =
        {
            .left = 0,
            .top = 0,
            .right = static_cast<LONG>(width),
            .bottom = static_cast<LONG>(height),
        };
        AdjustWindowRect(&rect, style, FALSE);

        hwnd = CreateWindowExA(0, WINDOW_CLASS_NAME, title_text, style, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, instance, this);
        if (!hwnd)
            return false;

        title = title_text;
        ShowWindow(hwnd, SW_SHOW);
        return true;
    }

    void UpdateTitle(const gfx::Device& device, const char* status)
    {
        const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        if (now - last_title_update < TITLE_UPDATE_INTERVAL)
            return;
        last_title_update = now;

        const f64 timestamp_period_ms = device.GetInfo().timestamp_period_ns * 1e-6;
        std::string text = title + " | " + status;
        for (const gfx::Device::GpuTiming& timing : device.GetGpuTimings())
        {
            const f64 milliseconds = static_cast<f64>(timing.timestamp_end - timing.timestamp_start) * timestamp_period_ms;
            char pass[128] = {};
            snprintf(pass, sizeof(pass), " | %s %.2f ms", timing.name, milliseconds);
            text += pass;
        }
        SetWindowTextA(hwnd, text.c_str());
    }

    void Close()
    {
        if (hwnd)
        {
            DestroyWindow(hwnd);
            hwnd = nullptr;
        }
        UnregisterClassA(WINDOW_CLASS_NAME, GetModuleHandleA(nullptr));
    }

    void PollEvents()
    {
        mouse_delta_x = 0;
        mouse_delta_y = 0;

        MSG msg = {};
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    bool IsOpen()
    {
        PollEvents();
        return is_open;
    }

    HWND GetHandle() const
    {
        return hwnd;
    }

    bool IsKeyDown(char key) const
    {
        return keys[static_cast<uint8_t>(key)];
    }

    HWND hwnd = nullptr;
    std::string title;
    std::chrono::steady_clock::time_point last_title_update = {};
    bool is_open = true;
    bool mouse_look = false;
    bool keys[256] = {};
    int32_t mouse_x = 0;
    int32_t mouse_y = 0;
    int32_t mouse_delta_x = 0;
    int32_t mouse_delta_y = 0;
};

struct Camera
{
    vec3 GetForward() const
    {
        return { std::sin(yaw) * std::cos(pitch), std::sin(pitch), -std::cos(yaw) * std::cos(pitch) };
    }

    bool Update(const Window& window, f32 delta_time)
    {
        yaw += static_cast<f32>(window.mouse_delta_x) * LOOK_SPEED;
        pitch = std::clamp(pitch - static_cast<f32>(window.mouse_delta_y) * LOOK_SPEED, -MAX_PITCH, MAX_PITCH);

        const vec3 forward = GetForward();
        const vec3 right = Normalize(Cross(forward, {0.0f, 1.0f, 0.0f}));

        f32 forward_input = 0.0f;
        f32 right_input = 0.0f;
        f32 up_input = 0.0f;
        
        if (window.IsKeyDown('W'))
            forward_input += 1.0f;
        if (window.IsKeyDown('S'))
            forward_input -= 1.0f;
        if (window.IsKeyDown('D'))
            right_input += 1.0f;
        if (window.IsKeyDown('A'))
            right_input -= 1.0f;
        if (window.IsKeyDown('E'))
            up_input += 1.0f;
        if (window.IsKeyDown('Q'))
            up_input -= 1.0f;
        
        const f32 step = MOVE_SPEED * delta_time;

        pos.x += (forward.x * forward_input + right.x * right_input) * step;
        pos.y += (forward.y * forward_input + up_input) * step;
        pos.z += (forward.z * forward_input + right.z * right_input) * step;

        return window.mouse_delta_x != 0 || window.mouse_delta_y != 0 || forward_input != 0.0f || right_input != 0.0f || up_input != 0.0f;
    }

    vec3 pos = {};
    f32 yaw = 0.0f;
    f32 pitch = 0.0f;
    f32 fov = DEFAULT_FOV;
};

struct Primitive
{
    u32 first_vertex = 0;
    u32 num_vertices = 0;
    u32 first_index = 0;
    u32 num_indices = 0;
    vec4 base_color = {1.0f, 1.0f, 1.0f, 1.0f};
    vec4 emissive = {0.0f, 0.0f, 0.0f, 0.0f};
    u32 texture = 0;
    f32 alpha_cutoff = 0.0f;
    f32 metallic = 0.0f;
    f32 roughness = 1.0f;
    f32 transmission = 0.0f;
    f32 ior = 1.5f;
};

struct Instance
{
    f32 transform[3][4] = {};
    u32 primitive = 0;
};

constexpr mat4 IDENTITY_TRANSFORM =
{
    .columns =
    {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f},
    },
};

struct SceneData
{
    bool Load(const std::string& requested_path, const mat4& transform = IDENTITY_TRANSFORM);

    std::vector<f32> positions;
    std::vector<f32> normals;
    std::vector<f32> uvs;
    std::vector<u32> indices;
    std::vector<Primitive> primitives;
    std::vector<Instance> instances;
    std::vector<std::string> textures;
    vec3 sun_direction = {-0.44f, 0.88f, -0.18f};
};

inline void ReadMaterial(const cgltf_material& material, const std::string& folder, Primitive& primitive, std::vector<std::string>& textures)
{
    const bool specular_glossiness = material.has_pbr_specular_glossiness;
    const cgltf_float* factor = specular_glossiness ? material.pbr_specular_glossiness.diffuse_factor : material.pbr_metallic_roughness.base_color_factor;
    const cgltf_texture* texture = specular_glossiness ? material.pbr_specular_glossiness.diffuse_texture.texture : material.pbr_metallic_roughness.base_color_texture.texture;
    primitive.base_color = {factor[0], factor[1], factor[2], factor[3]};

    if (specular_glossiness)
    {
        primitive.metallic = 0.0f;
        primitive.roughness = 1.0f - material.pbr_specular_glossiness.glossiness_factor;
    }
    else
    {
        primitive.metallic = material.pbr_metallic_roughness.metallic_factor;
        primitive.roughness = material.pbr_metallic_roughness.roughness_factor;
    }

    const f32 emissive_strength = material.has_emissive_strength ? material.emissive_strength.emissive_strength : 1.0f;
    primitive.emissive = {material.emissive_factor[0] * emissive_strength, material.emissive_factor[1] * emissive_strength, material.emissive_factor[2] * emissive_strength, 0.0f};

    if (material.alpha_mode == cgltf_alpha_mode_mask)
        primitive.alpha_cutoff = material.alpha_cutoff;

    if (material.has_transmission)
        primitive.transmission = material.transmission.transmission_factor;
    if (material.has_ior)
        primitive.ior = material.ior.ior;

    if (texture)
    {
        std::string texture_path = folder + texture->image->uri;
        if (GetFileAttributesA(texture_path.c_str()) == INVALID_FILE_ATTRIBUTES)
            texture_path = texture_path.substr(0, texture_path.find_last_of('.')) + ".dds";

        const u32 texture_index = static_cast<u32>(std::find(textures.begin(), textures.end(), texture_path) - textures.begin());
        if (texture_index == textures.size())
            textures.push_back(texture_path);
        primitive.texture = texture_index + 1;
    }
}

inline bool SceneData::Load(const std::string& requested_path, const mat4& transform)
{
    std::string path = requested_path;
    const size_t data_folder = path.find("data/");
    if (!std::filesystem::exists(path) && data_folder != std::string::npos)
        path = path.substr(data_folder);

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

            if (source.material)
                ReadMaterial(*source.material, folder, primitive, textures);

            primitives.push_back(primitive);
        }
    }

    for (cgltf_size i = 0; i < data->nodes_count; ++i)
    {
        const cgltf_node& node = data->nodes[i];
        cgltf_float node_matrix[16] = {};
        cgltf_node_transform_world(&node, node_matrix);

        cgltf_float matrix[16] = {};
        for (u32 column = 0; column < 4; ++column)
        {
            const cgltf_float* source = node_matrix + column * 4;
            matrix[column * 4 + 0] = transform.columns[0].x * source[0] + transform.columns[1].x * source[1] + transform.columns[2].x * source[2] + transform.columns[3].x * source[3];
            matrix[column * 4 + 1] = transform.columns[0].y * source[0] + transform.columns[1].y * source[1] + transform.columns[2].y * source[2] + transform.columns[3].y * source[3];
            matrix[column * 4 + 2] = transform.columns[0].z * source[0] + transform.columns[1].z * source[1] + transform.columns[2].z * source[2] + transform.columns[3].z * source[3];
            matrix[column * 4 + 3] = transform.columns[0].w * source[0] + transform.columns[1].w * source[1] + transform.columns[2].w * source[2] + transform.columns[3].w * source[3];
        }

        if (node.mesh)
        {
            const u32 first = first_primitive[cgltf_mesh_index(data, node.mesh)];
            for (cgltf_size j = 0; j < node.mesh->primitives_count; ++j)
            {
                const cgltf_material* material = node.mesh->primitives[j].material;
                if (material && !material->has_transmission && material->alpha_mode == cgltf_alpha_mode_blend)
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
