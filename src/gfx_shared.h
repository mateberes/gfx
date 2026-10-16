#ifndef GFX_SHARED_H
#define GFX_SHARED_H

#ifdef __cplusplus

#if defined(_MSC_VER) && !defined(__clang__) && (!defined(_MSVC_TRADITIONAL) || _MSVC_TRADITIONAL)
#error "gfx_shared.h needs /Zc:preprocessor"
#endif

#include "gfx.h"
#include <cstdio>

namespace gfx::shader
{
using uint = u32;

struct alignas(8) vec2
{
    f32 x;
    f32 y;
};

struct alignas(8) ivec2
{
    i32 x;
    i32 y;
};

struct alignas(8) uvec2
{
    u32 x;
    u32 y;
};

struct vec3
{
    f32 x;
    f32 y;
    f32 z;
};

struct ivec3
{
    i32 x;
    i32 y;
    i32 z;
};

struct uvec3
{
    u32 x;
    u32 y;
    u32 z;
};

struct alignas(16) vec4
{
    f32 x;
    f32 y;
    f32 z;
    f32 w;
};

struct alignas(16) ivec4
{
    i32 x;
    i32 y;
    i32 z;
    i32 w;
};

struct alignas(16) uvec4
{
    u32 x;
    u32 y;
    u32 z;
    u32 w;
};

struct alignas(16) mat4
{
    vec4 columns[4];
};

template <typename T>
struct alignas(8) GpuPtr
{
    using Target = T;
    u64 address = 0;
};
}

namespace gfx
{
enum class LayoutKind : u8
{
    Unknown = 0,
    Bool,
    Int,
    Uint,
    Float,
    Struct,
    Pointer,
};

struct LayoutType;

struct LayoutMember
{
    const char* name = nullptr;
    u32 offset = 0;
    u32 array_count = 0;
    u32 array_stride = 0;
    const LayoutType& (*type)() = nullptr;
};

struct LayoutType
{
    const char* name = nullptr;
    LayoutKind kind = LayoutKind::Unknown;
    u8 components = 1;
    u8 columns = 1;
    u32 size = 0;
    const LayoutMember* members = nullptr;
    u32 num_members = 0;
    const LayoutType& (*target)() = nullptr;
};

template <typename T>
struct LayoutOf
{
    static constexpr bool DECLARED = false;
};

struct ShaderTypeInfo
{
    LayoutKind kind = LayoutKind::Unknown;
    u8 components = 1;
    u8 columns = 1;
    u32 alignment = 0;
    u32 size = 0;
};

template <typename T>
constexpr bool IS_GPU_PTR = false;

template <typename T>
constexpr bool IS_GPU_PTR<shader::GpuPtr<T>> = true;

template <typename T>
constexpr ShaderTypeInfo GetShaderTypeInfo()
{
    if constexpr (std::is_array_v<T>)
    {
        constexpr ShaderTypeInfo element = GetShaderTypeInfo<std::remove_extent_t<T>>();
        constexpr u32 stride = (element.size + element.alignment - 1) / element.alignment * element.alignment;
        return {element.kind, element.components, element.columns, element.alignment, stride * static_cast<u32>(std::extent_v<T>)};
    }
    else if constexpr (IS_GPU_PTR<T>)
        return { LayoutKind::Pointer, 1, 1, 8, 8 };
    else if constexpr (LayoutOf<T>::DECLARED)
        return { LayoutKind::Struct, 1, 1, LayoutOf<T>::STD430_ALIGNMENT, LayoutOf<T>::STD430_SIZE};
    else if constexpr (std::is_same_v<T, f32>)
        return { LayoutKind::Float, 1, 1, 4, 4 };
    else if constexpr (std::is_same_v<T, i32>)
        return { LayoutKind::Int, 1, 1, 4, 4 };
    else if constexpr (std::is_same_v<T, u32>)
        return { LayoutKind::Uint, 1, 1, 4, 4 };
    else if constexpr (std::is_same_v<T, i64>)
        return { LayoutKind::Int, 1, 1, 8, 8 };
    else if constexpr (std::is_same_v<T, u64>)
        return { LayoutKind::Uint, 1, 1, 8, 8 };
    else if constexpr (std::is_same_v<T, shader::vec2>)
        return { LayoutKind::Float, 2, 1, 8, 8 };
    else if constexpr (std::is_same_v<T, shader::ivec2>)
        return { LayoutKind::Int, 2, 1, 8, 8 };
    else if constexpr (std::is_same_v<T, shader::uvec2>)
        return { LayoutKind::Uint, 2, 1, 8, 8 };
    else if constexpr (std::is_same_v<T, shader::vec3>)
        return { LayoutKind::Float, 3, 1, 16, 12 };
    else if constexpr (std::is_same_v<T, shader::ivec3>)
        return { LayoutKind::Int, 3, 1, 16, 12 };
    else if constexpr (std::is_same_v<T, shader::uvec3>)
        return { LayoutKind::Uint, 3, 1, 16, 12 };
    else if constexpr (std::is_same_v<T, shader::vec4>)
        return { LayoutKind::Float, 4, 1, 16, 16 };
    else if constexpr (std::is_same_v<T, shader::ivec4>)
        return { LayoutKind::Int, 4, 1, 16, 16 };
    else if constexpr (std::is_same_v<T, shader::uvec4>)
        return { LayoutKind::Uint, 4, 1, 16, 16 };
    else if constexpr (std::is_same_v<T, shader::mat4>)
        return { LayoutKind::Float, 4, 4, 16, 64 };
    else
        static_assert(!sizeof(T), "Unsupported type");
}

template <typename T>
const LayoutType& LayoutTypeOf()
{
    if constexpr (IS_GPU_PTR<T>)
    {
        static const LayoutType type =
        {
            .kind = LayoutKind::Pointer,
            .size = static_cast<u32>(sizeof(T)),
            .target = &LayoutTypeOf<typename T::Target>,
        };
        return type;
    }
    else if constexpr (LayoutOf<T>::DECLARED)
    {
        return LayoutOf<T>::Get();
    }
    else
    {
        constexpr ShaderTypeInfo info = GetShaderTypeInfo<T>();
        static const LayoutType type =
        {
            .kind = info.kind,
            .components = info.components,
            .columns = info.columns,
            .size = static_cast<u32>(sizeof(T)),
        };
        return type;
    }
}

struct Std430Member
{
    u32 cpp_offset = 0;
    u32 alignment = 0;
    u32 size = 0;
};

template <size_t N>
constexpr u32 Std430Offset(const Std430Member (&members)[N], u32 cpp_offset)
{
    u32 offset = 0;
    for (const Std430Member& member : members)
    {
        offset = (offset + member.alignment - 1) / member.alignment * member.alignment;
        if (member.cpp_offset == cpp_offset)
            return offset;
        offset += member.size;
    }
    return ~0u;
}

template <size_t N>
constexpr u32 Std430Alignment(const Std430Member (&members)[N])
{
    u32 alignment = 1;
    for (const Std430Member& member : members)
        alignment = std::max(alignment, member.alignment);
    return alignment;
}

template <size_t N>
constexpr u32 Std430Size(const Std430Member (&members)[N])
{
    const Std430Member& last = members[N - 1];
    const u32 end = Std430Offset(members, last.cpp_offset) + last.size;
    const u32 alignment = Std430Alignment(members);
    return (end + alignment - 1) / alignment * alignment;
}

#if GFX_DEBUG && GFX_USE_SPIRV_REFLECT
inline void FormatLayoutType(LayoutKind kind, u32 components, u32 columns, char* text, size_t text_size)
{
    constexpr const char* KIND_NAMES[] = {"unknown", "bool", "int", "uint", "float", "struct", "pointer"};
    const char* name = KIND_NAMES[static_cast<u32>(kind)];
    if (columns > 1)
        snprintf(text, text_size, "%s%ux%u", name, columns, components);
    else if (components > 1)
        snprintf(text, text_size, "%s%u", name, components);
    else
        snprintf(text, text_size, "%s", name);
}

inline bool CompareLayoutStruct(const SpvReflectBlockVariable& block, const LayoutType& type, const char* path, const char* what, u32 depth);

inline bool CompareLayoutValue(const SpvReflectBlockVariable& variable, const LayoutType& type, const char* path, const char* what, u32 depth)
{
    constexpr u32 MAX_DEPTH = 8;
    const SpvReflectTypeDescription& spirv = *variable.type_description;

    LayoutKind kind = LayoutKind::Unknown;
    u32 components = 1;
    u32 columns = 1;
    u32 size = 0;
    if (spirv.op == SpvOpTypePointer && spirv.storage_class == SpvStorageClassPhysicalStorageBuffer)
    {
        kind = LayoutKind::Pointer;
        size = sizeof(u64);
    }
    else if (spirv.type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT)
    {
        kind = LayoutKind::Struct;
    }
    else
    {
        const SpvReflectNumericTraits& numeric = spirv.traits.numeric;
        if (spirv.type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT)
            kind = LayoutKind::Float;
        else if (spirv.type_flags & SPV_REFLECT_TYPE_FLAG_INT)
            kind = numeric.scalar.signedness ? LayoutKind::Int : LayoutKind::Uint;
        else if (spirv.type_flags & SPV_REFLECT_TYPE_FLAG_BOOL)
            kind = LayoutKind::Bool;

        const u32 scalar_size = numeric.scalar.width / 8;
        if (spirv.type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX)
        {
            components = numeric.matrix.row_count;
            columns = numeric.matrix.column_count;
            size = (variable.numeric.matrix.stride ? variable.numeric.matrix.stride : components * scalar_size) * columns;
        }
        else
        {
            components = std::max(numeric.vector.component_count, 1u);
            size = scalar_size * components;
        }
    }

    if (kind != type.kind || components != type.components || columns != type.columns || (kind != LayoutKind::Struct && size != type.size))
    {
        char shader_type[32];
        char cpp_type[32];
        FormatLayoutType(kind, components, columns, shader_type, sizeof(shader_type));
        FormatLayoutType(type.kind, type.components, type.columns, cpp_type, sizeof(cpp_type));
        GFX_LOG_ERROR("%s: %s: %s, %u bytes (shader) vs %s, %u bytes (C++)", what, path, shader_type, size, cpp_type, type.size);
        return false;
    }

    if (kind == LayoutKind::Struct)
        return CompareLayoutStruct(variable, type, path, what, depth);

    if (kind != LayoutKind::Pointer || variable.member_count == 0 || depth >= MAX_DEPTH)
        return true;

    const LayoutType& target = type.target();

    const SpvReflectBlockVariable& first_member = variable.members[0];
    if (variable.member_count == 1 && first_member.type_description->op == SpvOpTypeRuntimeArray)
    {
        const u32 stride = first_member.type_description->traits.array.stride;
        if (stride != target.size)
        {
            GFX_LOG_ERROR("%s: %s: element stride %u (shader) vs %u (C++)", what, path, stride, target.size);
            return false;
        }
        return CompareLayoutValue(first_member, target, path, what, depth + 1);
    }

    if (target.kind != LayoutKind::Struct)
    {
        GFX_LOG_ERROR("%s: %s: struct (shader) vs not a struct (C++)", what, path);
        return false;
    }
    return CompareLayoutStruct(variable, target, path, what, depth + 1);
}

inline bool CompareLayoutStruct(const SpvReflectBlockVariable& block, const LayoutType& type, const char* path, const char* what, u32 depth)
{
    for (u32 i = 0; i < type.num_members; ++i)
    {
        const LayoutMember& member = type.members[i];
        char member_path[256];
        snprintf(member_path, sizeof(member_path), "%s.%s", path, member.name);

        const SpvReflectBlockVariable* variable = nullptr;
        for (u32 j = 0; j < block.member_count && !variable; ++j)
        {
            if (block.members[j].name && strcmp(block.members[j].name, member.name) == 0)
                variable = &block.members[j];
        }
        if (!variable)
        {
            GFX_LOG_ERROR("%s: %s: in C++, missing from shader", what, member_path);
            return false;
        }
        if (variable->offset != member.offset)
        {
            GFX_LOG_ERROR("%s: %s: offset %u (shader) vs %u (C++)", what, member_path, variable->offset, member.offset);
            return false;
        }

        const SpvReflectTypeDescription& spirv = *variable->type_description;
        const bool is_array = spirv.op == SpvOpTypeArray || spirv.op == SpvOpTypeRuntimeArray;
        const u32 array_count = spirv.op == SpvOpTypeArray && spirv.traits.array.dims_count == 1 ? spirv.traits.array.dims[0] : 0;
        const u32 array_stride = is_array ? spirv.traits.array.stride : 0;
        if ((is_array && array_count == 0) || array_count != member.array_count || array_stride != member.array_stride)
        {
            GFX_LOG_ERROR("%s: %s: array of %u, stride %u (shader) vs %u, stride %u (C++)", what, member_path, array_count, array_stride, member.array_count, member.array_stride);
            return false;
        }

        if (!CompareLayoutValue(*variable, member.type(), member_path, what, depth))
            return false;
    }

    for (u32 i = 0; i < block.member_count; ++i)
    {
        bool found = false;
        for (u32 j = 0; j < type.num_members && !found; ++j)
            found = block.members[i].name && strcmp(block.members[i].name, type.members[j].name) == 0;

        if (!found)
        {
            GFX_LOG_ERROR("%s: %s.%s: in the shader, missing from C++", what, path, block.members[i].name ? block.members[i].name : "?");
            return false;
        }
    }
    return true;
}

inline bool ValidateLayoutBlock(const SpvReflectBlockVariable& block, const LayoutType& type, const char* what)
{
    const SpvReflectBlockVariable* root = &block;
    if (block.member_count == 1 && block.members[0].type_description->op == SpvOpTypeStruct)
    {
        bool named = false;
        for (u32 i = 0; i < type.num_members && !named; ++i)
            named = block.members[0].name && strcmp(block.members[0].name, type.members[i].name) == 0;

        if (!named)
            root = &block.members[0];
    }
    return CompareLayoutStruct(*root, type, type.name, what, 0);
}

inline void ValidatePushLayout(const CommandBuffer& cmd, const LayoutType& type)
{
    GFX_ASSERT(IsAlive(cmd.current_pipeline) && "no bound pipeline");
    Pipeline& pipeline = Get(cmd.current_pipeline);
    if (pipeline.validated_push_layout == &type)
        return;

    for (const SpvReflectShaderModule& module : pipeline.reflections)
    {
        if (module.push_constant_block_count == 0)
            continue;

        const bool valid = ValidateLayoutBlock(module.push_constant_blocks[0], type, "push constants");
        GFX_ASSERT(valid && "Push constant layout is different between shader and C++");
    }
    pipeline.validated_push_layout = &type;
}

template <typename T>
void ValidateGlobalLayout(CommandBuffer& cmd)
{
    const LayoutType& type = LayoutOf<T>::Get();
    if (!IsAlive(cmd.current_pipeline))
        return;

    Pipeline& pipeline = Get(cmd.current_pipeline);
    if (pipeline.validated_global_layout == &type)
        return;

    for (const SpvReflectShaderModule& module : pipeline.reflections)
    {
        for (u32 i = 0; i < module.descriptor_binding_count; ++i)
        {
            const SpvReflectDescriptorBinding& binding = module.descriptor_bindings[i];
            if (binding.set != 1 || binding.binding != 0)
                continue;

            const bool valid = ValidateLayoutBlock(binding.block, type, "global constants");
            GFX_ASSERT(valid && "Global constant layout is different between shader and C++");
        }
    }
    pipeline.validated_global_layout = &type;
}
#endif
}  // namespace gfx

#define GFX_PARENS ()
#define GFX_EXPAND(...) GFX_EXPAND_3(GFX_EXPAND_3(GFX_EXPAND_3(GFX_EXPAND_3(__VA_ARGS__))))
#define GFX_EXPAND_3(...) GFX_EXPAND_2(GFX_EXPAND_2(GFX_EXPAND_2(GFX_EXPAND_2(__VA_ARGS__))))
#define GFX_EXPAND_2(...) GFX_EXPAND_1(GFX_EXPAND_1(GFX_EXPAND_1(GFX_EXPAND_1(__VA_ARGS__))))
#define GFX_EXPAND_1(...) __VA_ARGS__
#define GFX_FOR_EACH(M, T, ...) __VA_OPT__(GFX_EXPAND(GFX_FOR_EACH_STEP(M, T, __VA_ARGS__)))
#define GFX_FOR_EACH_STEP(M, T, NAME, ...) M(T, NAME) __VA_OPT__(GFX_FOR_EACH_AGAIN GFX_PARENS(M, T, __VA_ARGS__))
#define GFX_FOR_EACH_AGAIN() GFX_FOR_EACH_STEP

#define GFX_MEMBERS(...) __VA_ARGS__

#define GFX_LAYOUT_STD430(TYPE, NAME) {static_cast<u32>(offsetof(TYPE, NAME)), gfx::GetShaderTypeInfo<decltype(TYPE::NAME)>().alignment, gfx::GetShaderTypeInfo<decltype(TYPE::NAME)>().size},

#define GFX_LAYOUT_MEMBER(TYPE, NAME)                                                                                          \
    {                                                                                                                          \
        #NAME,                                                                                                                 \
        static_cast<u32>(offsetof(TYPE, NAME)),                                                                                \
        static_cast<u32>(std::extent_v<decltype(TYPE::NAME)>),                                                                 \
        static_cast<u32>(std::is_array_v<decltype(TYPE::NAME)> ? sizeof(std::remove_extent_t<decltype(TYPE::NAME)>) : 0),       \
        &gfx::LayoutTypeOf<std::remove_all_extents_t<decltype(TYPE::NAME)>>,                                                   \
    },

#define GFX_LAYOUT_ASSERT(TYPE, NAME)                                                                                          \
    static_assert(gfx::Std430Offset(gfx::LayoutOf<TYPE>::STD430, offsetof(TYPE, NAME)) == offsetof(TYPE, NAME) &&              \
                  gfx::GetShaderTypeInfo<decltype(TYPE::NAME)>().size == sizeof(TYPE::NAME),                                   \
                  "GFX_LAYOUT(" #TYPE "): " #NAME " offset or size is not std430, reorder or pad");

#if GFX_DEBUG && GFX_USE_SPIRV_REFLECT
#define GFX_LAYOUT_VALIDATION(TYPE)                                                                                            \
    inline void ValidatePushConstants(const gfx::CommandBuffer& cmd, const TYPE&)                                              \
    {                                                                                                                          \
        gfx::ValidatePushLayout(cmd, gfx::LayoutOf<TYPE>::Get());                                                              \
    }                                                                                                                          \
    inline void ValidateGlobalConstants(gfx::CommandBuffer& cmd, const TYPE&)                                                  \
    {                                                                                                                          \
        cmd.global_constants_validator = &gfx::ValidateGlobalLayout<TYPE>;                                                     \
        gfx::ValidateGlobalLayout<TYPE>(cmd);                                                                                  \
    }
#else
#define GFX_LAYOUT_VALIDATION(TYPE)
#endif

#define GFX_LAYOUT(TYPE, MEMBERS)                                                                                             \
    template <>                                                                                                                \
    struct gfx::LayoutOf<TYPE>                                                                                                 \
    {                                                                                                                          \
        static constexpr bool DECLARED = true;                                                                                 \
        static constexpr gfx::Std430Member STD430[] = { GFX_FOR_EACH(GFX_LAYOUT_STD430, TYPE, MEMBERS) };                      \
        static constexpr u32 STD430_ALIGNMENT = gfx::Std430Alignment(STD430);                                                  \
        static constexpr u32 STD430_SIZE = gfx::Std430Size(STD430);                                                            \
                                                                                                                               \
        static const gfx::LayoutType& Get()                                                                                    \
        {                                                                                                                      \
            static const gfx::LayoutMember members[] = { GFX_FOR_EACH(GFX_LAYOUT_MEMBER, TYPE, MEMBERS) };                     \
            static const gfx::LayoutType type =                                                                                \
            {                                                                                                                  \
                .name = #TYPE,                                                                                                 \
                .kind = gfx::LayoutKind::Struct,                                                                               \
                .size = static_cast<u32>(sizeof(TYPE)),                                                                        \
                .members = members,                                                                                            \
                .num_members = static_cast<u32>(std::size(members)),                                                           \
            };                                                                                                                 \
            return type;                                                                                                       \
        }                                                                                                                      \
    };                                                                                                                         \
    GFX_FOR_EACH(GFX_LAYOUT_ASSERT, TYPE, MEMBERS)                                                                             \
    static_assert(sizeof(TYPE) == gfx::LayoutOf<TYPE>::STD430_SIZE, "GFX_LAYOUT(" #TYPE "): size must be std430"); \
    GFX_LAYOUT_VALIDATION(TYPE)

#define GFX_BUFFER(NAME, TYPE, MEMBER, ALIGNMENT)                                                                              \
    using NAME = gfx::shader::GpuPtr<TYPE>;                                                                                    \
    static_assert(ALIGNMENT == gfx::GetShaderTypeInfo<TYPE>().alignment, "GFX_BUFFER(" #NAME "): alignment must be std430: " #TYPE);

#define GFX_RW_BUFFER(NAME, TYPE, MEMBER, ALIGNMENT) GFX_BUFFER(NAME, TYPE, MEMBER, ALIGNMENT)

using namespace gfx::shader;

#else

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#define GFX_LAYOUT(TYPE, MEMBERS)
#define GFX_BUFFER(NAME, TYPE, MEMBER, ALIGNMENT) layout(buffer_reference, std430, buffer_reference_align = ALIGNMENT) readonly buffer NAME { TYPE MEMBER[]; };
#define GFX_RW_BUFFER(NAME, TYPE, MEMBER, ALIGNMENT) layout(buffer_reference, std430, buffer_reference_align = ALIGNMENT) buffer NAME { TYPE MEMBER[]; };

#endif

#endif
