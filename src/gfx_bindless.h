#ifndef GFX_BINDLESS_H
#define GFX_BINDLESS_H

#ifdef __cplusplus
#error "GLSL only header"
#endif

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_shader_image_load_formatted : require

layout(set = 0, binding = 0) uniform sampler2D textures_2d[];
layout(set = 0, binding = 0) uniform sampler2DArray textures_2d_array[];
layout(set = 0, binding = 0) uniform sampler3D textures_3d[];
layout(set = 0, binding = 0) uniform samplerCube textures_cube[];
layout(set = 0, binding = 0) uniform samplerCubeArray textures_cube_array[];
layout(set = 0, binding = 0) uniform usampler2D utextures_2d[];
layout(set = 0, binding = 0) uniform usampler2DArray utextures_2d_array[];
layout(set = 0, binding = 0) uniform usampler3D utextures_3d[];
layout(set = 0, binding = 0) uniform usamplerCube utextures_cube[];
layout(set = 0, binding = 0) uniform sampler2DMS textures_2d_ms[];
layout(set = 0, binding = 0) uniform usampler2DMS utextures_2d_ms[];

layout(set = 0, binding = 1) uniform image2D images_2d[];
layout(set = 0, binding = 1) uniform image2DArray images_2d_array[];
layout(set = 0, binding = 1) uniform image3D images_3d[];
layout(set = 0, binding = 1) uniform imageCube images_cube[];
layout(set = 0, binding = 1) uniform uimage2D uimages_2d[];
layout(set = 0, binding = 1) uniform uimage2DArray uimages_2d_array[];
layout(set = 0, binding = 1) uniform uimage3D uimages_3d[];
layout(set = 0, binding = 1) uniform uimageCube uimages_cube[];

layout(set = 0, binding = 1) uniform readonly image2D read_images_2d[];
layout(set = 0, binding = 1) uniform readonly image2DArray read_images_2d_array[];
layout(set = 0, binding = 1) uniform readonly image3D read_images_3d[];
layout(set = 0, binding = 1) uniform readonly imageCube read_images_cube[];
layout(set = 0, binding = 1) uniform readonly uimage2D read_uimages_2d[];
layout(set = 0, binding = 1) uniform readonly uimage2DArray read_uimages_2d_array[];
layout(set = 0, binding = 1) uniform readonly uimage3D read_uimages_3d[];
layout(set = 0, binding = 1) uniform readonly uimageCube read_uimages_cube[];

layout(set = 0, binding = 1) uniform writeonly image2D write_images_2d[];
layout(set = 0, binding = 1) uniform writeonly image2DArray write_images_2d_array[];
layout(set = 0, binding = 1) uniform writeonly image3D write_images_3d[];
layout(set = 0, binding = 1) uniform writeonly imageCube write_images_cube[];
layout(set = 0, binding = 1) uniform writeonly uimage2D write_uimages_2d[];
layout(set = 0, binding = 1) uniform writeonly uimage2DArray write_uimages_2d_array[];
layout(set = 0, binding = 1) uniform writeonly uimage3D write_uimages_3d[];
layout(set = 0, binding = 1) uniform writeonly uimageCube write_uimages_cube[];

#ifdef GFX_BINDLESS_RAY_TRACING
layout(set = 0, binding = 2) uniform accelerationStructureEXT tlas[];
#endif

#endif
