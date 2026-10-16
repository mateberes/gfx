#pragma once

#define _USE_MATH_DEFINES
#ifndef GFX_LOG_FN
#define GFX_LOG_FN gfx::GFX_Log
#endif
#include "gfx.h"
#include "gfx_shared.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "inc/stb_image.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

constexpr const char* WINDOW_CLASS_NAME = "gfx_window";
constexpr f32 DEFAULT_FOV = static_cast<f32>(M_PI / 3.0);
constexpr f32 MAX_PITCH = 1.6f;
constexpr f32 MOVE_SPEED = 4.0f;
constexpr f32 LOOK_SPEED = 0.003f;

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

inline gfx::Handle<gfx::Texture> LoadTexture(gfx::Device& device, const std::string& path, gfx::Handle<gfx::Sampler> sampler)
{
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

struct Window
{
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

    bool Initialize(uint32_t width, uint32_t height, const char* title)
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

        hwnd = CreateWindowExA(0, WINDOW_CLASS_NAME, title, style, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, instance, this);
        if (!hwnd)
            return false;

        ShowWindow(hwnd, SW_SHOW);
        return true;
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

    void Update(const Window& window, f32 delta_time)
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
    }

    vec3 pos = {};
    f32 yaw = 0.0f;
    f32 pitch = 0.0f;
    f32 fov = DEFAULT_FOV;
};
