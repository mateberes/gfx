#define GFX_IMPLEMENTATION
#define GFX_WINDOW_WIN32
#include "common/samples_common.h"
#include "gfx.h"

int main()
{
    Window window;
    if (!window.Initialize(1280, 720, "triangle"))
        return 1;

    gfx::Device& device = gfx::CreateDevice(
    {
        .app_name = "triangle",
        .window = window.GetHandle(),
    });

    const gfx::Handle<gfx::ShaderModule> vertex_shader = LoadShader(device, "triangle.vert.spv", gfx::ShaderStage::Vertex);
    const gfx::Handle<gfx::ShaderModule> fragment_shader = LoadShader(device, "triangle.frag.spv", gfx::ShaderStage::Fragment);

    const gfx::Handle<gfx::Pipeline> pipeline = device.CreatePipeline(
    {
        .debug_name = "triangle",
        .VS = vertex_shader,
        .FS = fragment_shader,
        .num_color_targets = 1,
        .color_targets = {{ .format = device.GetSwapchain().format }},
    });

    while (window.IsOpen())
    {
        gfx::Frame frame = device.NewFrame();
        const gfx::Handle<gfx::Texture> backbuffer = device.GetSwapchainTexture();
        if (!backbuffer.IsValid())
        {
            device.SubmitFrame(frame);
            continue;
        }

        gfx::CommandBuffer& cmd = frame.cmd;

        cmd.BeginRenderPass(
        {
            .debug_name = "triangle",
            .color_targets =
            {
                {
                    .texture = backbuffer,
                    .clear_color = { 0.05f, 0.05f, 0.08f, 1.0f },
                },
            },
        });

        cmd.BindPipeline(pipeline, gfx::BindPoint::Graphics);
        cmd.Draw(3, 1);
        cmd.EndRenderPass();

        device.SubmitFrame(frame);
    }

    device.Destroy(pipeline);
    device.Destroy(vertex_shader);
    device.Destroy(fragment_shader);

    gfx::DestroyDevice(device);
    window.Close();
    return 0;
}
