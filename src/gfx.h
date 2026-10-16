#pragma once

// gfx.h - single header vulkan graphics library
//
// Options: define these before including gfx.h:
// #define GFX_IMPLEMENTATION       // add this to exactly one .cpp file
// #define GFX_WINDOW_WIN32         // enable win32 window
// #define GFX_WINDOW_GLFW          // enable glfw window
// #define GFX_USE_GLM              // enable glm, glm, glm should be in the include path (default: off)
// #define GFX_USE_VMA 1            // enable VMA as a default allocator, vma should be in the include path (default: off)
// #define GFX_HANDLE_64 1          // enable 64 bit handles (default: off)
// #define GFX_DEBUG 1              // enable debug logging, push constant layout and render target format checks (default: on unless NDEBUG)
// #define GFX_USE_SPIRV_REFLECT 1  // enable shader reflection, Dispatch() local size, shader stage checks, GFX_LAYOUT validation (recommended, default: off)
// #define GFX_ENABLE_VALIDATION 1  // enable vulkan validation layers, debug names and labels (default: GFX_DEBUG)
// #define GFX_LOG_FN gfx::GFX_Log  // enable default logging (default: off, fatal errors always print)

#ifndef GFX_DEBUG
#ifdef NDEBUG
#define GFX_DEBUG 0
#else
#define GFX_DEBUG 1
#endif
#endif
#ifndef GFX_USE_VMA
#define GFX_USE_VMA 0
#endif
#ifndef GFX_USE_SPIRV_REFLECT
#define GFX_USE_SPIRV_REFLECT 0
#endif
#ifndef GFX_HANDLE_64
#define GFX_HANDLE_64 0
#endif
#ifndef GFX_ENABLE_VALIDATION
#define GFX_ENABLE_VALIDATION GFX_DEBUG
#endif
#ifndef GFX_MAX_FRAMES_IN_FLIGHT
#define GFX_MAX_FRAMES_IN_FLIGHT 2
#endif
#ifndef GFX_MAX_GPUS
#define GFX_MAX_GPUS 4u
#endif
#ifndef GFX_MAX_SWAPCHAIN_IMAGES
#define GFX_MAX_SWAPCHAIN_IMAGES 3
#endif
#ifndef GFX_MAX_QUEUE_FAMILIES
#define GFX_MAX_QUEUE_FAMILIES 8
#endif
#ifndef GFX_TEMP_FRAME_SIZE
#define GFX_TEMP_FRAME_SIZE (64ull << 20)
#endif
#ifndef GFX_UPLOAD_RING_SIZE
#define GFX_UPLOAD_RING_SIZE (64ull << 20)
#endif
#ifndef GFX_GPU_BLOCK_SIZE
#define GFX_GPU_BLOCK_SIZE (256ull << 20)
#endif
#ifndef GFX_MAX_BINDLESS_DESCRIPTORS
#define GFX_MAX_BINDLESS_DESCRIPTORS 16384
#endif
#ifndef GFX_MAX_QUERY_TIMESTAMPS
#define GFX_MAX_QUERY_TIMESTAMPS 1000
#endif
#ifndef GFX_MAX_COLOR_TARGETS
#define GFX_MAX_COLOR_TARGETS 5
#endif
#ifndef GFX_MAX_COMMAND_SLOTS
#define GFX_MAX_COMMAND_SLOTS 64
#endif
#ifndef GFX_API_VERSION
#define GFX_API_VERSION VK_API_VERSION_1_3
#endif

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/utility/vk_struct_helper.hpp>

#if GFX_USE_SPIRV_REFLECT
#include <spirv_reflect.h>
#endif

#if GFX_USE_VMA
#include <vma/vk_mem_alloc.h>
#endif

#ifdef GFX_USE_GLM
#include <glm/glm.hpp>
#endif

#define VK_CHECK(x)                                                                                                                                                                                    \
    do                                                                                                                                                                                                 \
    {                                                                                                                                                                                                  \
        const VkResult check_result = x;                                                                                                                                                                 \
        if (check_result != VK_SUCCESS)                                                                                                                                                                 \
            GFX_LOG_FATAL("%s failed: %s", #x, string_VkResult(check_result));                                                                                                                      \
    } while (0)

#define GFX_INLINE inline
#if defined(__clang__) || defined(__GNUC__)
#define GFX_FORCEINLINE inline __attribute__((always_inline))
#else
#define GFX_FORCEINLINE __forceinline
#endif
#define GFX_NOEXCEPT noexcept
#define GFX_ASSERT(x)                                                       \
    do                                                                      \
    {                                                                       \
        if (!(x))                                                           \
            GFX_LOG_FATAL("assert: %s (%s:%d)", #x, __FILE__, __LINE__);    \
    } while (0)

#if defined(__clang__) || defined(__GNUC__)
#define GFX_LIFETIMEBOUND [[clang::lifetimebound]]
#elif defined(_MSC_VER)
#define GFX_LIFETIMEBOUND [[msvc::lifetimebound]]
#else
#define GFX_LIFETIMEBOUND
#endif

#define GFX_NON_COPYABLE(Type)                     \
    Type(const Type&) = delete;                    \
    Type& operator=(const Type&) = delete;         \
    Type(Type&&) GFX_NOEXCEPT = default;           \
    Type& operator=(Type&&) GFX_NOEXCEPT = default

#define GFX_NON_MOVABLE(Type)                      \
    Type(const Type&) = delete;                    \
    Type& operator=(const Type&) = delete;         \
    Type(Type&&) = delete;                         \
    Type& operator=(Type&&) = delete

using u8 = unsigned char;
using i8 = signed char;
using u16 = unsigned short;
using i16 = signed short;
using u32 = unsigned int;
using i32 = signed int;
using u64 = unsigned long long int;
using i64 = signed long long int;
using f32 = float;
using f64 = double;

namespace gfx
{

enum class LogLevel
{
    Info,
    Debug,
    Warning,
    Error,
    Fatal
};

void GFX_Log(LogLevel log_level, const char* format = "", ...);

#define GFX_LOG_UNUSED(...) ((void)sizeof(gfx::GFX_Log(gfx::LogLevel::Info, __VA_ARGS__), 0))

#ifdef GFX_LOG_FN
#define GFX_LOG(...) GFX_LOG_FN(gfx::LogLevel::Info, __VA_ARGS__)
#if GFX_DEBUG
#define GFX_LOG_DEBUG(...) GFX_LOG_FN(gfx::LogLevel::Debug, __VA_ARGS__)
#else
#define GFX_LOG_DEBUG(...) GFX_LOG_UNUSED(__VA_ARGS__)
#endif
#define GFX_LOG_WARNING(...) GFX_LOG_FN(gfx::LogLevel::Warning, __VA_ARGS__)
#define GFX_LOG_ERROR(...) GFX_LOG_FN(gfx::LogLevel::Error, __VA_ARGS__)
#define GFX_LOG_FATAL(...) GFX_LOG_FN(gfx::LogLevel::Fatal, __VA_ARGS__)
#else
#define GFX_LOG(...) GFX_LOG_UNUSED(__VA_ARGS__)
#define GFX_LOG_DEBUG(...) GFX_LOG_UNUSED(__VA_ARGS__)
#define GFX_LOG_WARNING(...) GFX_LOG_UNUSED(__VA_ARGS__)
#define GFX_LOG_ERROR(...) GFX_LOG_UNUSED(__VA_ARGS__)
#define GFX_LOG_FATAL(...) gfx::GFX_Log(gfx::LogLevel::Fatal, __VA_ARGS__)
#endif

GFX_FORCEINLINE bool IsPow2(u64 value)
{
    return value != 0 && (value & (value - 1)) == 0;
}

GFX_FORCEINLINE u64 AlignUpPowerOfTwo(u64 value, u64 alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

GFX_FORCEINLINE u64 AlignUpToMultiple(u64 value, u64 alignment)
{
    return (value + alignment - 1) / alignment * alignment;
}

#define GFX_ENUM_FLAGS(Enum)                                                                        \
    inline constexpr Enum operator|(Enum lhs, Enum rhs)                                             \
    {                                                                                               \
        using EnumType = std::underlying_type_t<Enum>;                                              \
        return static_cast<Enum>(static_cast<EnumType>(lhs) | static_cast<EnumType>(rhs));          \
    }                                                                                               \
    inline constexpr Enum& operator|=(Enum& lhs, Enum rhs)                                          \
    {                                                                                               \
        return lhs = lhs | rhs;                                                                     \
    }                                                                                               \
    inline constexpr bool operator&(Enum lhs, Enum rhs)                                             \
    {                                                                                               \
        using EnumType = std::underlying_type_t<Enum>;                                              \
        return (static_cast<EnumType>(lhs) & static_cast<EnumType>(rhs)) != 0;                      \
    }

template <typename T>
class Span
{
  public:
    GFX_FORCEINLINE Span() = default;
    GFX_FORCEINLINE Span(T* data, size_t size) : m_data(data), m_size(size)
    {
    }

    template <size_t N>
    GFX_FORCEINLINE Span(T (&arr)[N]) : m_data(arr), m_size(N)
    {
    }

    GFX_FORCEINLINE Span(std::initializer_list<T> list GFX_LIFETIMEBOUND)
        : m_data(const_cast<T*>(list.begin()))
        , m_size(list.size())
    {
    }

    GFX_FORCEINLINE const T& operator[](size_t i) const
    {
        GFX_ASSERT(i < m_size);
        return m_data[i];
    }

    GFX_FORCEINLINE T& operator[](size_t i)
    {
        GFX_ASSERT(i < m_size);
        return m_data[i];
    }

    GFX_FORCEINLINE size_t size() const
    {
        return m_size;
    }

    GFX_FORCEINLINE bool empty() const
    {
        return m_size == 0;
    }

    GFX_FORCEINLINE T* data()
    {
        return m_data;
    }

    GFX_FORCEINLINE const T* data() const
    {
        return m_data;
    }

    GFX_FORCEINLINE T* begin()
    {
        return m_data;
    }

    GFX_FORCEINLINE const T* begin() const
    {
        return m_data;
    }

    GFX_FORCEINLINE T* end()
    {
        return m_data + m_size;
    }

    GFX_FORCEINLINE const T* end() const
    {
        return m_data + m_size;
    }

    const T* cbegin() const
    {
        return m_data;
    }

    const T* cend() const
    {
        return m_data + m_size;
    }

  private:
    T* m_data = nullptr;
    size_t m_size = 0;
};

#ifdef GFX_USE_GLM
using Vec2u = glm::uvec2;
using Vec3 = glm::vec3;
using Vec3i = glm::ivec3;
using Vec3u = glm::uvec3;
#else
struct Vec2u
{
    u32 x = 0;
    u32 y = 0;
};

struct Vec3
{
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = 0.0f;
};

struct Vec3i
{
    i32 x = 0;
    i32 y = 0;
    i32 z = 0;
};

struct Vec3u
{
    u32 x = 0;
    u32 y = 0;
    u32 z = 0;
};
#endif

struct Device;
struct Texture;
struct Buffer;
struct Swapchain;
struct Pipeline;
struct ShaderModule;
struct AccelerationStructure;
class CommandBuffer;

struct HandleBits
{
#if GFX_HANDLE_64
    using SizeType = u64;
    constexpr static int IndexBits = 30;
    constexpr static int GenerationBits = 32;
#else
    using SizeType = u32;
    constexpr static int IndexBits = 16;
    constexpr static int GenerationBits = 14;
#endif
    constexpr static int DeviceBits = GFX_MAX_GPUS > 1 ? std::bit_width(GFX_MAX_GPUS - 1u) : 1;
};

template <class Type, class Size = HandleBits::SizeType, int IndexSize = HandleBits::IndexBits, int GenerationSize = HandleBits::GenerationBits, int DeviceSize = HandleBits::DeviceBits>
struct Handle;

template <class T>
T& Get(Handle<T> handle);

template <class T>
bool IsAlive(Handle<T> handle);

template <class T>
void Destroy(Handle<T> handle);

template <class Type, class Size, int IndexSize, int GenerationSize, int DeviceSize>
struct Handle
{
    using ValueType = Type;
    using SizeType = Size;

    constexpr static int INDEX_BITS = IndexSize;
    constexpr static int GENERATION_BITS = GenerationSize;
    constexpr static int DEVICE_BITS = DeviceSize;
    constexpr static SizeType INDEX_INVALID = (SizeType(1) << INDEX_BITS) - 1;
    constexpr static SizeType GENERATION_INVALID = (SizeType(1) << GENERATION_BITS) - 1;

    GFX_FORCEINLINE Handle() = default;

    GFX_FORCEINLINE Handle(SizeType index, SizeType generation, SizeType device)
        : index(index)
        , generation(generation)
        , device_index(device)
    {
    }

    GFX_FORCEINLINE bool IsValid() const
    {
        return index != INDEX_INVALID;
    }

    GFX_FORCEINLINE void Reset()
    {
        index = INDEX_INVALID;
        generation = GENERATION_INVALID;
        device_index = 0;
    }

    GFX_FORCEINLINE explicit operator bool() const
    {
        return IsValid();
    }

    GFX_FORCEINLINE SizeType Index() const
    {
        return index;
    }

    GFX_FORCEINLINE SizeType Generation() const
    {
        return generation;
    }

    GFX_FORCEINLINE SizeType DeviceIndex() const
    {
        return device_index;
    }

    GFX_FORCEINLINE Type& Get() const
    {
        return gfx::Get(*this);
    }

    GFX_FORCEINLINE Type* operator->() const
    {
        return &gfx::Get(*this);
    }

    GFX_FORCEINLINE friend bool operator==(Handle lhs, Handle rhs)
    {
        return lhs.index == rhs.index && lhs.generation == rhs.generation && lhs.device_index == rhs.device_index;
    }

    GFX_FORCEINLINE friend bool operator!=(Handle lhs, Handle rhs)
    {
        return !(lhs == rhs);
    }

    SizeType index : INDEX_BITS = INDEX_INVALID;
    SizeType generation : GENERATION_BITS = GENERATION_INVALID;
    SizeType device_index : DEVICE_BITS = 0;
};

static_assert(sizeof(Handle<Device>) == sizeof(HandleBits::SizeType), "Handle must fit in HandleBits::SizeType");

template <class T>
class Owned
{
  public:
    Owned() = default;
    Owned(const Owned&) = delete;
    Owned& operator=(const Owned&) = delete;

    GFX_FORCEINLINE explicit Owned(Handle<T> handle)
        : handle(handle)
    {
    }

    GFX_FORCEINLINE ~Owned()
    {
        Reset();
    }

    GFX_FORCEINLINE Owned(Owned&& other) GFX_NOEXCEPT
        : handle(other.handle)
    {
        other.handle = {};
    }

    GFX_FORCEINLINE Owned& operator=(Owned&& other) GFX_NOEXCEPT
    {
        if (this != &other)
        {
            Reset();
            handle = other.handle;
            other.handle = {};
        }

        return *this;
    }

    GFX_FORCEINLINE operator Handle<T>() const
    {
        return handle;
    }

    GFX_FORCEINLINE void Reset()
    {
        if (handle.IsValid())
        {
            gfx::Destroy(handle);
        }

        handle = {};
    }

    GFX_FORCEINLINE bool IsValid() const
    {
        return handle.IsValid();
    }

    GFX_FORCEINLINE T& Get() const
    {
        return gfx::Get(handle);
    }

    GFX_FORCEINLINE T* operator->() const
    {
        return &gfx::Get(handle);
    }

    GFX_FORCEINLINE friend bool operator==(const Owned& lhs, const Owned& rhs)
    {
        return lhs.handle == rhs.handle;
    }

    GFX_FORCEINLINE friend bool operator==(const Owned& lhs, Handle<T> rhs)
    {
        return lhs.handle == rhs;
    }

private:
    Handle<T> handle;
};

template <class ItemType, class SizeType = HandleBits::SizeType, int IndexBits = HandleBits::IndexBits, int GenerationBits = HandleBits::GenerationBits, int DeviceBits = HandleBits::DeviceBits>
struct Pool
{
    using HandleType = Handle<ItemType, SizeType, IndexBits, GenerationBits, DeviceBits>;

    constexpr static SizeType INDEX_ACTIVE = HandleType::INDEX_INVALID - 1;
    constexpr static SizeType INDEX_RETIRED = HandleType::INDEX_INVALID - 2;
    constexpr static SizeType MAX_GENERATION = HandleType::GENERATION_INVALID;

    struct Slot
    {
        SizeType generation = 0;
        SizeType next_free_index = 0;
    };

    GFX_FORCEINLINE void Initialize(SizeType max_items, SizeType device = 0)
    {
        GFX_ASSERT(max_items <= INDEX_RETIRED && "max items does not fit in the handle index, use 64 bit handles");
        entries.reserve(max_items);
        slots.reserve(max_items);
        capacity = max_items;
        device_index = device;
    }

    GFX_FORCEINLINE bool IsValidHandle(HandleType handle) const
    {
        if (handle.index >= slots.size() || handle.device_index != device_index)
            return false;

        if (slots[handle.index].generation != handle.generation)
            return false;

        return IsActiveSlot(handle.index);
    }

    GFX_FORCEINLINE bool IsActiveSlot(SizeType idx) const
    {
        return slots[idx].next_free_index == INDEX_ACTIVE;
    }

    template <typename... Args>
    GFX_FORCEINLINE HandleType Alloc(Args&&... args)
    {
        SizeType index = free_head;
        if (index == HandleType::INDEX_INVALID)
        {
            GFX_ASSERT(entries.size() < capacity && "Pool is full");
            ++size;
            entries.emplace_back(std::forward<Args>(args)...);
            slots.emplace_back(0, INDEX_ACTIVE);
            return { static_cast<SizeType>(entries.size() - 1), 0, device_index };
        }

        ++size;
        ItemType& entry = entries[index];

        entry = ItemType(std::forward<Args>(args)...);

        Slot& slot = slots[index];
        free_head = slot.next_free_index;
        slot.next_free_index = INDEX_ACTIVE;

        return HandleType{index, slot.generation, device_index};
    }

    GFX_FORCEINLINE void Free(HandleType handle)
    {
        if (!IsValidHandle(handle))
        {
            return;
        }

        const SizeType index = handle.index;

        --size;
        entries[index] = ItemType{};

        Slot& slot = slots[index];
        slot.next_free_index = INDEX_RETIRED;
        if (slot.generation != MAX_GENERATION)
            ++slot.generation;
    }

    GFX_FORCEINLINE void Release(SizeType index)
    {
        GFX_ASSERT(index < slots.size() && slots[index].next_free_index == INDEX_RETIRED);

        Slot& slot = slots[index];
        if (slot.generation == MAX_GENERATION)
        {
            // Quarantine
            slot.next_free_index = HandleType::INDEX_INVALID;
            return;
        }

        slot.next_free_index = free_head;
        free_head = index;
    }

    GFX_FORCEINLINE const ItemType& GetByIndex(SizeType index) const
    {
        return entries[index];
    }

    GFX_FORCEINLINE ItemType& GetByIndex(SizeType index)
    {
        return entries[index];
    }

    GFX_FORCEINLINE HandleType GetHandle(SizeType index) const
    {
        GFX_ASSERT(index < slots.size());
        return HandleType{index, slots[index].generation, device_index};
    }

    GFX_FORCEINLINE ItemType& Get(HandleType handle)
    {
        GFX_ASSERT(IsValidHandle(handle));
        ItemType& entry = entries[handle.index];
        return entry;
    }

    GFX_FORCEINLINE const ItemType& Get(HandleType handle) const
    {
        GFX_ASSERT(IsValidHandle(handle));
        const ItemType& entry = entries[handle.index];
        return entry;
    }

    GFX_FORCEINLINE ItemType* GetPtr(HandleType handle)
    {
        if (!IsValidHandle(handle))
        {
            return nullptr;
        }

        ItemType& entry = entries[handle.index];
        return &entry;
    }

    GFX_FORCEINLINE const ItemType* GetPtr(HandleType handle) const
    {
        if (!IsValidHandle(handle))
        {
            return nullptr;
        }

        const ItemType& entry = entries[handle.index];
        return &entry;
    }

    GFX_FORCEINLINE void Clear()
    {
        entries.clear();
        slots.clear();
        free_head = HandleType::INDEX_INVALID;
        size = 0;
    }

    GFX_FORCEINLINE SizeType Size() const
    {
        return size;
    }

    GFX_FORCEINLINE SizeType Slots() const
    {
        return static_cast<SizeType>(slots.size());
    }

    template <bool IsConst>
    class PoolIterator
    {
      public:
        using ElementType = std::conditional_t<IsConst, const ItemType, ItemType>;
        using PoolType = std::conditional_t<IsConst, const Pool, Pool>;

        PoolIterator(PoolType* pool, SizeType index) : pool(pool), index(index)
        {
            Advance();
        }

        GFX_FORCEINLINE ElementType& operator*() const
        {
            return pool->entries[index];
        }

        GFX_FORCEINLINE ElementType* operator->() const
        {
            return &pool->entries[index];
        }

        GFX_FORCEINLINE PoolIterator& operator++()
        {
            ++index;
            Advance();
            return *this;
        }

        bool operator==(const PoolIterator& other) const = default;

        GFX_FORCEINLINE HandleType GetHandle() const
        {
            return pool->GetHandle(index);
        }

      private:
        GFX_FORCEINLINE void Advance()
        {
            while (index < pool->Slots() && !pool->IsActiveSlot(index))
                ++index;
        }

        PoolType* pool = nullptr;
        SizeType index = 0;
    };

    using Iterator = PoolIterator<false>;
    using ConstIterator = PoolIterator<true>;

    GFX_FORCEINLINE Iterator begin()
    {
        return Iterator(this, 0);
    }

    GFX_FORCEINLINE Iterator end()
    {
        return Iterator(this, Slots());
    }

    GFX_FORCEINLINE ConstIterator begin() const
    {
        return ConstIterator(this, 0);
    }

    GFX_FORCEINLINE ConstIterator end() const
    {
        return ConstIterator(this, Slots());
    }

  private:
    std::vector<ItemType> entries;
    std::vector<Slot> slots;
    SizeType free_head = HandleType::INDEX_INVALID;
    SizeType size = 0;
    SizeType capacity = 0;
    SizeType device_index = 0;
};

constexpr static int MAX_MIP_LEVELS = 16;
constexpr static u64 MAX_INLINE_BUFFER_UPDATE = 4096;
constexpr static u32 MAX_PUSH_CONSTANTS_SIZE = 128;
constexpr static u64 ACCELERATION_STRUCTURE_INPUT_ALIGNMENT = 16;
constexpr static u64 ACCELERATION_STRUCTURE_SCRATCH_SIZE = 64ull << 20;
constexpr static u32 MAX_ACCELERATION_STRUCTURE_BATCH = 64;
constexpr static u32 MAX_RAY_MISS_SHADERS = 16;
constexpr static u32 MAX_HIT_GROUPS = 64;
constexpr static u32 MAX_PENDING_BARRIERS = 32;
constexpr static u32 MAX_INSTANCE_EXTENSIONS = 8;
constexpr static u32 INDEX_INVALID = UINT32_MAX;
constexpr static u32 INVALID_QUEUE_FAMILY = UINT32_MAX;

enum class BufferUsage : u32
{
    None = 0,
    Uniform = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    Storage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    IndexBuffer = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    VertexBuffer = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    Indirect = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
    AccelerationStructureInput = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
    AccelerationStructureStorage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
    ShaderBindingTable = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
};
GFX_ENUM_FLAGS(BufferUsage);

enum class MemoryType : u8
{
    CPU_GPU,
    GPU_CPU,
    GPU,
};

constexpr VkMemoryPropertyFlags ToVkRequiredMemoryFlags(MemoryType memory_type)
{
    switch (memory_type)
    {
    case MemoryType::CPU_GPU:
    case MemoryType::GPU_CPU:
        return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    case MemoryType::GPU:
        return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    default:
        return 0;
    }
}

constexpr VkMemoryPropertyFlags ToVkPreferredMemoryFlags(MemoryType memory_type)
{
    switch (memory_type)
    {
    case MemoryType::GPU_CPU:
        return ToVkRequiredMemoryFlags(memory_type) | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    default:
        return ToVkRequiredMemoryFlags(memory_type);
    }
}

enum class SharingMode : u8
{
    Exclusive = 0,
    Concurrent,
    MAX
};

enum class TextureType : u8
{
    Texture1D = VK_IMAGE_VIEW_TYPE_1D,
    Texture2D = VK_IMAGE_VIEW_TYPE_2D,
    Texture3D = VK_IMAGE_VIEW_TYPE_3D,
    TextureCube = VK_IMAGE_VIEW_TYPE_CUBE,
    Texture2DArray = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
    TextureCubeArray = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,
    MAX
};

enum class TextureUsage : u8
{
    None = 0,
    Sample = 1 << 0,
    InputAttachment = 1 << 1,
    Attachment = 1 << 2,
    Storage = 1 << 3,
};
GFX_ENUM_FLAGS(TextureUsage);

enum class TextureAspect : u8
{
    Auto = 0,
    Color = VK_IMAGE_ASPECT_COLOR_BIT,
    Depth = VK_IMAGE_ASPECT_DEPTH_BIT,
    Stencil = VK_IMAGE_ASPECT_STENCIL_BIT,
};

enum class SamplerFilter : u8
{
    Nearest = VK_FILTER_NEAREST,
    Linear = VK_FILTER_LINEAR,
};

enum class SamplerAddressMode : u8
{
    Repeat = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    MirrorRepeat = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
    ClampToEdge = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    ClampToBorder = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    MirrorClampToEdge = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
    MAX
};

enum class SamplerMipMode : u8
{
    Nearest = VK_SAMPLER_MIPMAP_MODE_NEAREST,
    Linear = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    MAX
};

enum class BorderColor : u8
{
    TransparentBlack = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
    OpaqueBlack = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
    OpaqueWhite = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
};

enum class TextureFormat : u8
{
    None = VK_FORMAT_UNDEFINED,

    R8_UNorm = VK_FORMAT_R8_UNORM,
    R8_sRGB = VK_FORMAT_R8_SRGB,
    R8_SNorm = VK_FORMAT_R8_SNORM,
    R8_UInt = VK_FORMAT_R8_UINT,
    R8_SInt = VK_FORMAT_R8_SINT,

    RG8_UNorm = VK_FORMAT_R8G8_UNORM,
    RG8_sRGB = VK_FORMAT_R8G8_SRGB,
    RG8_SNorm = VK_FORMAT_R8G8_SNORM,
    RG8_UInt = VK_FORMAT_R8G8_UINT,
    RG8_SInt = VK_FORMAT_R8G8_SINT,

    RGBA8_UNorm = VK_FORMAT_R8G8B8A8_UNORM,
    RGBA8_sRGB = VK_FORMAT_R8G8B8A8_SRGB,
    RGBA8_SNorm = VK_FORMAT_R8G8B8A8_SNORM,
    RGBA8_UInt = VK_FORMAT_R8G8B8A8_UINT,
    RGBA8_SInt = VK_FORMAT_R8G8B8A8_SINT,

    BGRA8_UNorm = VK_FORMAT_B8G8R8A8_UNORM,
    BGRA8_sRGB = VK_FORMAT_B8G8R8A8_SRGB,

    R16_UNorm = VK_FORMAT_R16_UNORM,
    R16_SNorm = VK_FORMAT_R16_SNORM,
    R16_UInt = VK_FORMAT_R16_UINT,
    R16_SInt = VK_FORMAT_R16_SINT,
    R16_Float = VK_FORMAT_R16_SFLOAT,

    RG16_UNorm = VK_FORMAT_R16G16_UNORM,
    RG16_SNorm = VK_FORMAT_R16G16_SNORM,
    RG16_UInt = VK_FORMAT_R16G16_UINT,
    RG16_SInt = VK_FORMAT_R16G16_SINT,
    RG16_Float = VK_FORMAT_R16G16_SFLOAT,

    RGBA16_UNorm = VK_FORMAT_R16G16B16A16_UNORM,
    RGBA16_SNorm = VK_FORMAT_R16G16B16A16_SNORM,
    RGBA16_UInt = VK_FORMAT_R16G16B16A16_UINT,
    RGBA16_SInt = VK_FORMAT_R16G16B16A16_SINT,
    RGBA16_Float = VK_FORMAT_R16G16B16A16_SFLOAT,

    R32_UInt = VK_FORMAT_R32_UINT,
    R32_SInt = VK_FORMAT_R32_SINT,
    R32_Float = VK_FORMAT_R32_SFLOAT,

    RG32_UInt = VK_FORMAT_R32G32_UINT,
    RG32_SInt = VK_FORMAT_R32G32_SINT,
    RG32_Float = VK_FORMAT_R32G32_SFLOAT,

    RGBA32_UInt = VK_FORMAT_R32G32B32A32_UINT,
    RGBA32_SInt = VK_FORMAT_R32G32B32A32_SINT,
    RGBA32_Float = VK_FORMAT_R32G32B32A32_SFLOAT,

    RGB10A2_UNorm = VK_FORMAT_A2B10G10R10_UNORM_PACK32,
    RGB10A2_UInt = VK_FORMAT_A2B10G10R10_UINT_PACK32,
    RG11B10_Float = VK_FORMAT_B10G11R11_UFLOAT_PACK32,
    RGB9E5_Float = VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,

    D16_UNorm = VK_FORMAT_D16_UNORM,
    D24_UNorm_S8_UInt = VK_FORMAT_D24_UNORM_S8_UINT,
    D32_Float = VK_FORMAT_D32_SFLOAT,
    D32_Float_S8_UInt = VK_FORMAT_D32_SFLOAT_S8_UINT,
    S8_UInt = VK_FORMAT_S8_UINT,

    BC1_UNorm = VK_FORMAT_BC1_RGBA_UNORM_BLOCK,
    BC1_sRGB = VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
    BC2_UNorm = VK_FORMAT_BC2_UNORM_BLOCK,
    BC2_sRGB = VK_FORMAT_BC2_SRGB_BLOCK,
    BC3_UNorm = VK_FORMAT_BC3_UNORM_BLOCK,
    BC3_sRGB = VK_FORMAT_BC3_SRGB_BLOCK,
    BC4_UNorm = VK_FORMAT_BC4_UNORM_BLOCK,
    BC4_SNorm = VK_FORMAT_BC4_SNORM_BLOCK,
    BC5_UNorm = VK_FORMAT_BC5_UNORM_BLOCK,
    BC5_SNorm = VK_FORMAT_BC5_SNORM_BLOCK,
    BC6H_UFloat = VK_FORMAT_BC6H_UFLOAT_BLOCK,
    BC6H_SFloat = VK_FORMAT_BC6H_SFLOAT_BLOCK,
    BC7_UNorm = VK_FORMAT_BC7_UNORM_BLOCK,
    BC7_sRGB = VK_FORMAT_BC7_SRGB_BLOCK,

    ETC2_RGB8_UNorm = VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,
    ETC2_RGB8_sRGB = VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,
    ETC2_RGBA8_UNorm = VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,
    ETC2_RGBA8_sRGB = VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,
    MAX
};

struct FormatInfo
{
    u8 block_width = 1;
    u8 block_height = 1;
    u8 block_bytes = 0;
    bool depth = false;
    bool stencil = false;

    GFX_FORCEINLINE u64 GetSize(Vec3u extent) const { return static_cast<u64>((extent.x + block_width - 1) / block_width) * ((extent.y + block_height - 1) / block_height) * extent.z * block_bytes; }
};

constexpr FormatInfo GetFormatInfo(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::R8_UNorm:
    case TextureFormat::R8_sRGB:
    case TextureFormat::R8_SNorm:
    case TextureFormat::R8_UInt:
    case TextureFormat::R8_SInt:
        return { .block_bytes = 1 };

    case TextureFormat::S8_UInt:
        return
        {
            .block_bytes = 1,
            .stencil = true,
        };

    case TextureFormat::RG8_UNorm:
    case TextureFormat::RG8_sRGB:
    case TextureFormat::RG8_SNorm:
    case TextureFormat::RG8_UInt:
    case TextureFormat::RG8_SInt:
    case TextureFormat::R16_UNorm:
    case TextureFormat::R16_SNorm:
    case TextureFormat::R16_UInt:
    case TextureFormat::R16_SInt:
    case TextureFormat::R16_Float:
        return { .block_bytes = 2 };

    case TextureFormat::D16_UNorm:
        return
        {
            .block_bytes = 2,
            .depth = true,
        };

    case TextureFormat::RGBA8_UNorm:
    case TextureFormat::RGBA8_sRGB:
    case TextureFormat::RGBA8_SNorm:
    case TextureFormat::RGBA8_UInt:
    case TextureFormat::RGBA8_SInt:
    case TextureFormat::BGRA8_UNorm:
    case TextureFormat::BGRA8_sRGB:
    case TextureFormat::RG16_UNorm:
    case TextureFormat::RG16_SNorm:
    case TextureFormat::RG16_UInt:
    case TextureFormat::RG16_SInt:
    case TextureFormat::RG16_Float:
    case TextureFormat::R32_UInt:
    case TextureFormat::R32_SInt:
    case TextureFormat::R32_Float:
    case TextureFormat::RGB10A2_UNorm:
    case TextureFormat::RGB10A2_UInt:
    case TextureFormat::RG11B10_Float:
    case TextureFormat::RGB9E5_Float:
        return { .block_bytes = 4 };

    case TextureFormat::D32_Float:
        return
        {
            .block_bytes = 4,
            .depth = true,
        };

    case TextureFormat::D24_UNorm_S8_UInt:
        return
        {
            .block_bytes = 4,
            .depth = true,
            .stencil = true,
        };

    case TextureFormat::RGBA16_UNorm:
    case TextureFormat::RGBA16_SNorm:
    case TextureFormat::RGBA16_UInt:
    case TextureFormat::RGBA16_SInt:
    case TextureFormat::RGBA16_Float:
    case TextureFormat::RG32_UInt:
    case TextureFormat::RG32_SInt:
    case TextureFormat::RG32_Float:
        return { .block_bytes = 8 };

    case TextureFormat::D32_Float_S8_UInt:
        return
        {
            .block_bytes = 8,
            .depth = true,
            .stencil = true,
        };

    case TextureFormat::RGBA32_UInt:
    case TextureFormat::RGBA32_SInt:
    case TextureFormat::RGBA32_Float:
        return { .block_bytes = 16 };

    case TextureFormat::BC1_UNorm:
    case TextureFormat::BC1_sRGB:
    case TextureFormat::BC4_UNorm:
    case TextureFormat::BC4_SNorm:
    case TextureFormat::ETC2_RGB8_UNorm:
    case TextureFormat::ETC2_RGB8_sRGB:
        return
        {
            .block_width = 4,
            .block_height = 4,
            .block_bytes = 8,
        };

    case TextureFormat::BC2_UNorm:
    case TextureFormat::BC2_sRGB:
    case TextureFormat::BC3_UNorm:
    case TextureFormat::BC3_sRGB:
    case TextureFormat::BC5_UNorm:
    case TextureFormat::BC5_SNorm:
    case TextureFormat::BC6H_UFloat:
    case TextureFormat::BC6H_SFloat:
    case TextureFormat::BC7_UNorm:
    case TextureFormat::BC7_sRGB:
    case TextureFormat::ETC2_RGBA8_UNorm:
    case TextureFormat::ETC2_RGBA8_sRGB:
        return
        {
            .block_width = 4,
            .block_height = 4,
            .block_bytes = 16,
        };

    default:
        return {};
    }
}

enum class LoadOp : u8
{
    Load = VK_ATTACHMENT_LOAD_OP_LOAD,
    Clear = VK_ATTACHMENT_LOAD_OP_CLEAR,
    DontCare = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    MAX
};

enum class StoreOp : u32
{
    Store = VK_ATTACHMENT_STORE_OP_STORE,
    DontCare = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    None = VK_ATTACHMENT_STORE_OP_NONE,
    MAX
};

enum class ResolveMode : u32
{
    SampleZero = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT,
    Average = VK_RESOLVE_MODE_AVERAGE_BIT,
    Min = VK_RESOLVE_MODE_MIN_BIT,
    Max = VK_RESOLVE_MODE_MAX_BIT,
};

enum class CullMode : u8
{
    None = VK_CULL_MODE_NONE,
    Front = VK_CULL_MODE_FRONT_BIT,
    Back = VK_CULL_MODE_BACK_BIT,
    FrontBack = VK_CULL_MODE_FRONT_AND_BACK,
    MAX
};

enum class Topology : u8
{
    PointList = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
    LineList = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
    TriangleList = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    MAX
};

enum class CompareOp : u8
{
    Never = VK_COMPARE_OP_NEVER,
    Less = VK_COMPARE_OP_LESS,
    Equal = VK_COMPARE_OP_EQUAL,
    LessOrEqual = VK_COMPARE_OP_LESS_OR_EQUAL,
    Greater = VK_COMPARE_OP_GREATER,
    NotEqual = VK_COMPARE_OP_NOT_EQUAL,
    GreaterOrEqual = VK_COMPARE_OP_GREATER_OR_EQUAL,
    Always = VK_COMPARE_OP_ALWAYS,
    MAX
};

enum class FrontFace : u8
{
    CounterClockwise = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    Clockwise = VK_FRONT_FACE_CLOCKWISE,
};

enum class BlendFactor : u8
{
    Zero = VK_BLEND_FACTOR_ZERO,
    One = VK_BLEND_FACTOR_ONE,
    SrcColor = VK_BLEND_FACTOR_SRC_COLOR,
    OneMinusSrcColor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
    DstColor = VK_BLEND_FACTOR_DST_COLOR,
    OneMinusDstColor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
    SrcAlpha = VK_BLEND_FACTOR_SRC_ALPHA,
    OneMinusSrcAlpha = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    DstAlpha = VK_BLEND_FACTOR_DST_ALPHA,
    OneMinusDstAlpha = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
    SrcAlphaSaturate = VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
};

enum class BlendOp : u8
{
    Add = VK_BLEND_OP_ADD,
    Subtract = VK_BLEND_OP_SUBTRACT,
    ReverseSubtract = VK_BLEND_OP_REVERSE_SUBTRACT,
    Min = VK_BLEND_OP_MIN,
    Max = VK_BLEND_OP_MAX,
};

enum class ColorWriteMask : u8
{
    None = 0,
    R = VK_COLOR_COMPONENT_R_BIT,
    G = VK_COLOR_COMPONENT_G_BIT,
    B = VK_COLOR_COMPONENT_B_BIT,
    A = VK_COLOR_COMPONENT_A_BIT,
    All = R | G | B | A,
};
GFX_ENUM_FLAGS(ColorWriteMask);

enum class StencilOp : u8
{
    Keep = VK_STENCIL_OP_KEEP,
    Zero = VK_STENCIL_OP_ZERO,
    Replace = VK_STENCIL_OP_REPLACE,
    IncrementClamp = VK_STENCIL_OP_INCREMENT_AND_CLAMP,
    DecrementClamp = VK_STENCIL_OP_DECREMENT_AND_CLAMP,
    Invert = VK_STENCIL_OP_INVERT,
    IncrementWrap = VK_STENCIL_OP_INCREMENT_AND_WRAP,
    DecrementWrap = VK_STENCIL_OP_DECREMENT_AND_WRAP,
};

enum class PresentMode : u8
{
    Immediate = VK_PRESENT_MODE_IMMEDIATE_KHR,
    Mailbox = VK_PRESENT_MODE_MAILBOX_KHR,
    Fifo = VK_PRESENT_MODE_FIFO_KHR
};

enum class ShaderStage : u32
{
    None = 0,
    Vertex = VK_SHADER_STAGE_VERTEX_BIT,
    Geometry = VK_SHADER_STAGE_GEOMETRY_BIT,
    Fragment = VK_SHADER_STAGE_FRAGMENT_BIT,
    Compute = VK_SHADER_STAGE_COMPUTE_BIT,
    Task = VK_SHADER_STAGE_TASK_BIT_EXT,
    Mesh = VK_SHADER_STAGE_MESH_BIT_EXT,
    RayGen = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
    AnyHit = VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
    ClosestHit = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
    Miss = VK_SHADER_STAGE_MISS_BIT_KHR,
    Intersection = VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
    Callable = VK_SHADER_STAGE_CALLABLE_BIT_KHR,
};
GFX_ENUM_FLAGS(ShaderStage);

enum class PipelineStage : u64
{
    None = VK_PIPELINE_STAGE_2_NONE,
    Indirect = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,
    IndexInput = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
    Vertex = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
    Task = VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT,
    Mesh = VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT,
    PreRasterization = VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT,
    DepthStencil = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
    Fragment = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
    ColorOutput = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    Compute = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
    Transfer = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,
    Host = VK_PIPELINE_STAGE_2_HOST_BIT,
    AccelerationStructureBuild = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
    RayTracing = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
    AllShaders = PreRasterization | Fragment | Compute,
    AllCommands = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
};
GFX_ENUM_FLAGS(PipelineStage);

enum class StageAccess : u64
{
    None = VK_ACCESS_2_NONE,
    IndirectRead = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT,
    IndexRead = VK_ACCESS_2_INDEX_READ_BIT,
    ShaderRead = VK_ACCESS_2_SHADER_READ_BIT,
    ShaderWrite = VK_ACCESS_2_SHADER_WRITE_BIT,
    ColorRead = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT,
    ColorWrite = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
    DepthStencilRead = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
    DepthStencilWrite = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    TransferRead = VK_ACCESS_2_TRANSFER_READ_BIT,
    TransferWrite = VK_ACCESS_2_TRANSFER_WRITE_BIT,
    HostRead = VK_ACCESS_2_HOST_READ_BIT,
    HostWrite = VK_ACCESS_2_HOST_WRITE_BIT,
    AccelerationStructureRead = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR,
    AccelerationStructureWrite = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
    ShaderBindingTableRead = VK_ACCESS_2_SHADER_BINDING_TABLE_READ_BIT_KHR,
    AllWrites = ShaderWrite | ColorWrite | DepthStencilWrite | TransferWrite | HostWrite,
};
GFX_ENUM_FLAGS(StageAccess);

enum class TextureState : u8
{
    Undefined,
    ShaderRead,
    Storage,
    ColorAttachment,
    DepthStencilAttachment,
    TransferSrc,
    TransferDst,
    Present,
};

struct TextureStateInfo
{
    VkImageLayout layout;
    StageAccess access;
    PipelineStage stages;
};

constexpr TextureStateInfo GetTextureStateInfo(TextureState state)
{
    switch (state)
    {
    case TextureState::Undefined:
        return
        {
            .layout = VK_IMAGE_LAYOUT_UNDEFINED,
            .access = StageAccess::None,
            .stages = PipelineStage::None,
        };
    case TextureState::ShaderRead:
        return
        {
            .layout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
            .access = StageAccess::ShaderRead,
            .stages = PipelineStage::AllShaders,
        };
    case TextureState::Storage:
        return
        {
            .layout = VK_IMAGE_LAYOUT_GENERAL,
            .access = StageAccess::ShaderRead | StageAccess::ShaderWrite,
            .stages = PipelineStage::AllShaders,
        };
    case TextureState::ColorAttachment:
        return
        {
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .access = StageAccess::ColorRead | StageAccess::ColorWrite,
            .stages = PipelineStage::ColorOutput,
        };
    case TextureState::DepthStencilAttachment:
        return
        {
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .access = StageAccess::DepthStencilRead | StageAccess::DepthStencilWrite,
            .stages = PipelineStage::DepthStencil,
        };
    case TextureState::TransferSrc:
        return
        {
            .layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .access = StageAccess::TransferRead,
            .stages = PipelineStage::Transfer,
        };
    case TextureState::TransferDst:
        return
        {
            .layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .access = StageAccess::TransferWrite,
            .stages = PipelineStage::Transfer,
        };
    case TextureState::Present:
        return
        {
            .layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .access = StageAccess::None,
            .stages = PipelineStage::None,
        };
    }
    return {};
}

GFX_INLINE bool IsDepthFormat(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return true;
    default:
        return false;
    }
}

constexpr VkImageType ToVkImageType(TextureType texture_type)
{
    switch (texture_type)
    {
    case TextureType::Texture1D:
        return VK_IMAGE_TYPE_1D;
    case TextureType::Texture3D:
        return VK_IMAGE_TYPE_3D;
    default:
        return VK_IMAGE_TYPE_2D;
    }
}

constexpr bool IsStencilFormat(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_S8_UINT:
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return true;
    default:
        return false;
    }
}

GFX_INLINE VkImageAspectFlags GetImageAspectFlags(VkFormat format)
{
    VkImageAspectFlags aspect_flags = 0;
    if (IsDepthFormat(format))
        aspect_flags |= VK_IMAGE_ASPECT_DEPTH_BIT;
    if (IsStencilFormat(format))
        aspect_flags |= VK_IMAGE_ASPECT_STENCIL_BIT;
    return aspect_flags ? aspect_flags : VK_IMAGE_ASPECT_COLOR_BIT;
}

struct BindlessResourcePool
{
    BindlessResourcePool() = default;
    GFX_NON_MOVABLE(BindlessResourcePool);

    enum BindingSlot : u8
    {
        TextureBinding = 0,
        StorageBinding = 1,
        RayAccelerationBinding = 2,
        MAX = 3
    };

    void Initialize(Device& device);
    void Shutdown(Device& device);
    void CreateDescriptorSet(Device& device);
    void CreateBindlessLayout(Device& device);
    void CreateNullResources(Device& device);
    void InitializeSlots(const Device& device);

    void StoreTexture(Device& device, Handle<Texture> handle);
    void StoreAccelerationStructure(Device& device, Handle<AccelerationStructure> handle);
    void ClearTexture(Device& device, u32 slot);
    void ClearAccelerationStructure(Device& device, u32 slot);

    VkDescriptorPool bindless_pool = nullptr;
    VkDescriptorSet bindless_descriptor_set = nullptr;
    VkDescriptorSetLayout bindless_layout = nullptr;
    Handle<Texture> null_texture;
    Handle<AccelerationStructure> null_acceleration_structure;
    VkDescriptorImageInfo null_sampled_info = {};
    VkDescriptorImageInfo null_storage_info = {};
    VkAccelerationStructureKHR null_vk_acceleration_structure = nullptr;
    VkDescriptorSetLayout uniform_layout = nullptr;
    VkDescriptorPool uniform_pool = nullptr;
    VkDescriptorSet uniform_descriptor_set = nullptr;
    VkPipelineLayout pipeline_layout = nullptr;
};

struct Swapchain
{
    Swapchain() = default;
    Swapchain(VkExtent2D extent, VkFormat format, VkPresentModeKHR present_mode)
        : extent{extent.width, extent.height, 1}
        , format(static_cast<TextureFormat>(format))
        , present_mode(static_cast<PresentMode>(present_mode)) {}
    
    GFX_NON_COPYABLE(Swapchain);

    void Destroy(Device& device);

    GFX_FORCEINLINE Handle<Texture> GetCurrentTexture() const
    {
        GFX_ASSERT(acquired);
        return swapchain_textures[image_index];
    }
    GFX_FORCEINLINE Handle<Texture> GetDepthTexture() const { return swapchain_depth_texture; }
    GFX_FORCEINLINE VkSemaphore GetRenderFinishedSemaphore() const { return render_finished_semaphores[image_index]; }
    GFX_FORCEINLINE Vec2u GetSize() const { return Vec2u {extent.x, extent.y}; }

    VkSwapchainKHR vk_swapchain = nullptr;
    Vec3u extent = {};
    TextureFormat format = TextureFormat::None;
    PresentMode present_mode = PresentMode::Fifo;

    std::array<Handle<Texture>, GFX_MAX_SWAPCHAIN_IMAGES> swapchain_textures = {};
    u32 num_swapchain_textures = 0;

    Handle<Texture> swapchain_depth_texture;
    std::array<VkSemaphore, GFX_MAX_SWAPCHAIN_IMAGES> render_finished_semaphores = {};
    u32 image_index = 0;
    bool needs_recreation = false;
    bool acquired = false;
};

// Based on: https://github.com/sebbbi/OffsetAllocator
struct OffsetAllocator
{
    constexpr static u32 NO_SPACE = UINT32_MAX;

    using NodeIndex = u32;
    constexpr static NodeIndex NODE_UNUSED = static_cast<NodeIndex>(UINT32_MAX);
    constexpr static NodeIndex NODE_USED = static_cast<NodeIndex>(NODE_UNUSED - 1);

    struct Node
    {
        u32 offset;
        u32 size;
        NodeIndex bin_prev_idx;
        NodeIndex bin_next_idx;
        NodeIndex neighbor_prev_idx;
        NodeIndex neighbor_next_idx;

        GFX_FORCEINLINE bool IsUsed() const { return bin_prev_idx == NODE_USED; }
    };

    struct Allocation
    {
        u32 offset = NO_SPACE;
        NodeIndex node_idx = NODE_UNUSED;

        GFX_FORCEINLINE bool IsValid() const { return offset != NO_SPACE; }
    };

    struct Stats
    {
        u32 free_bytes = 0;
        u32 largest_free_range = 0;
        u32 num_free_ranges = 0;
    };

    OffsetAllocator() = default;
    GFX_NON_COPYABLE(OffsetAllocator);

    void Initialize(u32 total_size, u32 capacity);
    void Reset();

    Allocation Allocate(u32 allocation_size, u32 alignment = 1);
    void Free(Allocation allocation);

    u32 GetAllocationSize(Allocation allocation) const;
    Stats GetStats() const;

    static u32 BinIndexRoundUp(u32 size);
    static u32 BinIndexRoundDown(u32 size);

    GFX_FORCEINLINE u32 GetSize() const { return size; }
    GFX_FORCEINLINE u32 GetNumAllocations() const { return num_allocations; }
    GFX_FORCEINLINE u32 GetMaxAllocations() const { return max_allocations; }

    constexpr static u32 NUM_TOP_BINS = 32;
    constexpr static u32 BINS_PER_LEAF = 8;
    constexpr static u32 TOP_BINS_INDEX_SHIFT = 3;
    constexpr static u32 LEAF_BINS_INDEX_MASK = BINS_PER_LEAF - 1;
    constexpr static u32 NUM_LEAF_BINS = NUM_TOP_BINS * BINS_PER_LEAF;
    constexpr static u32 MANTISSA_BITS = 3;
    constexpr static u32 MANTISSA_VALUE = 1u << MANTISSA_BITS;
    constexpr static u32 MANTISSA_MASK = MANTISSA_VALUE - 1;

    NodeIndex FindFreeNodeIndex(u32 allocation_size, u32 alignment) const;
    NodeIndex CreateNode(u32 offset, u32 node_size);
    void ReleaseNode(NodeIndex node_idx);
    
    void AddToBin(NodeIndex node_idx);
    void RemoveFromBin(NodeIndex node_idx);
    
    void LinkPrev(NodeIndex node_idx, NodeIndex new_idx);
    void LinkNext(NodeIndex node_idx, NodeIndex new_idx);
    void Unlink(NodeIndex node_idx);

    static u32 FindHighestSetBitIndex(u32 mask);
    static u32 FindLowestSetBitIndexFrom(u32 mask, u32 start_bit_index);

    u32 size = 0;
    u32 max_allocations = 0;
    u32 num_allocations = 0;
    u32 free_bytes = 0;
    u32 top_bin_mask = 0;
    std::array<u8, NUM_TOP_BINS> leaf_bin_masks = {};
    std::array<NodeIndex, NUM_LEAF_BINS> bin_head_indices = {};
    std::vector<Node> nodes;
    std::vector<NodeIndex> free_node_indices;
    u32 num_nodes = 0;
    u32 num_free_nodes = 0;
};

struct MemoryStats
{
    u32 num_memory_objects = 0;
    u32 num_allocations = 0;
    u64 allocated_bytes = 0;
    u64 used_bytes = 0;
};

#if GFX_USE_VMA
using GpuAllocation = VmaAllocation;
#else
struct GpuAllocation
{
    VkDeviceMemory vk_memory = nullptr;
    u8* mapped_memory = nullptr;
    u64 offset = 0;
    u64 size = 0;
    u32 block = INDEX_INVALID;
    OffsetAllocator::NodeIndex node_index = OffsetAllocator::NODE_UNUSED;

    GFX_FORCEINLINE bool IsDedicated() const { return block == INDEX_INVALID; }
};

struct MemoryBlock
{
    VkDeviceMemory vk_memory = nullptr;
    u8* mapped_memory = nullptr;
    u64 size = 0;
    u32 memory_type = 0;
    bool images = false;
    OffsetAllocator ranges;
};

struct GpuAllocator
{
    constexpr static u32 MAX_BLOCK_ALLOCATIONS = 16384;
    constexpr static u32 NUM_SMALLER_BLOCKS = 3;
    constexpr static u64 MAX_SHARED_GRANULARITY = 4096;

    GpuAllocator() = default;
    GFX_NON_MOVABLE(GpuAllocator);

    void Initialize(Device& device);
    void Shutdown(Device& device);

    GpuAllocation AllocateBuffer(Device& device, VkBuffer buffer, MemoryType memory_type, u64 min_alignment);
    GpuAllocation AllocateTexture(Device& device, VkImage image, MemoryType memory_type);
    void Free(Device& device, const GpuAllocation& allocation);

    GpuAllocation Allocate(Device& device, const VkMemoryRequirements& requirements, bool dedicated, u32 memory_type_idx, bool images, const VkMemoryDedicatedAllocateInfo& dedicated_info);
    MemoryBlock AllocateMemory(Device& device, u64 size, u32 memory_type, bool images, const VkMemoryDedicatedAllocateInfo* dedicated_info);

    u32 CreateBlock(Device& device, u64 size, u32 memory_type, bool images);
    void DestroyBlock(Device& device, u32 index);
    
    MemoryStats GetStats() const;

    std::vector<MemoryBlock> blocks;
    std::array<u64, VK_MAX_MEMORY_HEAPS> block_sizes = {};
    u64 granularity = 1;
    bool separate_image_blocks = false;
    u32 num_dedicated = 0;
    u32 num_allocations = 0;
    u64 allocated_bytes = 0;
    u64 used_bytes = 0;
    u64 peak_allocated_bytes = 0;
};
#endif

struct Buffer
{
    Buffer() = default;
    GFX_NON_COPYABLE(Buffer);

    bool PrepareBarrier(PipelineStage new_stages, StageAccess new_access, VkBufferMemoryBarrier2& barrier);

    GFX_FORCEINLINE VkDeviceAddress GetBufferAddress() const { return vk_device_address; }

    VkBuffer vk_buffer = nullptr;
    GpuAllocation vk_allocation = {};
    VkDeviceAddress vk_device_address = 0;
    VkDeviceSize buffer_size = 0;
    MemoryType memory_type = MemoryType::CPU_GPU;
    BufferUsage buffer_usage = BufferUsage::None;
    void* mapped_memory = nullptr;
    PipelineStage stages = PipelineStage::None;
    StageAccess access = StageAccess::None;
};

struct Sampler
{
    Sampler() = default;
    GFX_NON_COPYABLE(Sampler);

    VkSampler vk_sampler = nullptr;
};

struct Texture
{
    Texture() = default;
    GFX_NON_COPYABLE(Texture);

    bool PrepareBarrier(TextureState new_state, VkImageMemoryBarrier2& barrier, PipelineStage new_stages = PipelineStage::None, StageAccess new_access = StageAccess::None);

    GFX_FORCEINLINE bool IsDepthTexture() const { return IsDepthFormat(texture_format); }
    GFX_FORCEINLINE bool IsStencilTexture() const { return IsStencilFormat(texture_format); }
    GFX_FORCEINLINE bool IsView() const { return parent.IsValid(); }

    VkImage vk_image = nullptr;
    GpuAllocation vk_allocation = {};
    VkImageView vk_image_view = nullptr;
    VkImageViewType vk_view_type = VK_IMAGE_VIEW_TYPE_2D;
    VkDeviceSize allocated_size = 0;
    Vec3u texture_extent = {};
    VkFormat texture_format = VK_FORMAT_UNDEFINED;
    VkImageUsageFlags vk_texture_usage_flags = 0;
    TextureUsage texture_usage = TextureUsage::None;
    TextureState state = TextureState::Undefined;
    PipelineStage stages = PipelineStage::None;
    StageAccess access = StageAccess::None;
    u32 owner_queue_family = 0;
    u32 num_mipmaps = 0;
    u32 num_layers = 0;
    u32 num_samples = 1;
    u32 base_mip_level = 0;
    u32 base_array_layer = 0;
    Handle<Sampler> sampler;
    Handle<Texture> parent;
    bool bindless = false;
};

struct ShaderDesc
{
    const char* debug_name = nullptr;
    std::vector<u8> byte_code;
    const char* entry_name = "main";
    ShaderStage stage = ShaderStage::Vertex;
};

struct ShaderMetaData
{
    Vec3u local_group_size;
};

struct ShaderModule
{
    ShaderModule() = default;
    GFX_NON_COPYABLE(ShaderModule);

    VkShaderModule vk_shader_module = nullptr;
    ShaderStage stage = ShaderStage::Vertex;
    char entry_name[32] = {};
    ShaderMetaData shader_meta_data = {};
#if GFX_USE_SPIRV_REFLECT
    SpvReflectShaderModule reflection = {};
#endif
};

enum class PassAccess : u8
{
    Sampled,
    Read,
    Write,
    ReadWrite,
};

struct PassResource
{
    Handle<Texture> texture;
    Handle<Buffer> buffer;
    PassAccess access = PassAccess::Sampled;
    PipelineStage stages = PipelineStage::None;
};

struct ClearColor
{
    constexpr ClearColor() = default;
    constexpr ClearColor(f32 r, f32 g, f32 b, f32 a) : float32{r, g, b, a} {}
    constexpr ClearColor(u32 r, u32 g, u32 b, u32 a) : uint32{r, g, b, a} {}
    constexpr ClearColor(i32 r, i32 g, i32 b, i32 a) : int32{r, g, b, a} {}

    union
    {
        f32 float32[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        u32 uint32[4];
        i32 int32[4];
    };
};

struct ColorTarget
{
    Handle<Texture> texture;
    LoadOp load_op = LoadOp::Clear;
    StoreOp store_op = StoreOp::Store;
    ClearColor clear_color = {};
    Handle<Texture> resolve_texture;
    ResolveMode resolve_mode = ResolveMode::Average;
};

struct DepthTarget
{
    Handle<Texture> texture;
    LoadOp load_op = LoadOp::Clear;
    StoreOp store_op = StoreOp::Store;
    f32 clear_depth = 1.0f;
    Handle<Texture> resolve_texture;
    ResolveMode resolve_mode = ResolveMode::SampleZero;
};

struct StencilTarget
{
    Handle<Texture> texture;
    LoadOp load_op = LoadOp::Clear;
    StoreOp store_op = StoreOp::Store;
    u8 clear_stencil = 0;
    Handle<Texture> resolve_texture;
    ResolveMode resolve_mode = ResolveMode::SampleZero;
};

struct RenderPassDesc
{
    const char* debug_name = nullptr;
    Vec3 debug_color = { 0.0f, 1.0f, 0.0f };
    Span<ColorTarget> color_targets;
    DepthTarget depth_target = {};
    StencilTarget stencil_target = {};
    Span<PassResource> inputs = {};
    Span<PassResource> outputs = {};
};

struct ComputePassDesc
{
    const char* debug_name = nullptr;
    Vec3 debug_color = { 0.0f, 1.0f, 0.0f };
    Span<PassResource> inputs = {};
    Span<PassResource> outputs = {};
    PipelineStage stages = PipelineStage::Compute;
};

struct BlendComponent
{
    BlendFactor src_factor = BlendFactor::One;
    BlendFactor dst_factor = BlendFactor::Zero;
    BlendOp op = BlendOp::Add;
};

struct BlendState
{
    bool enable = false;
    BlendComponent color = {};
    BlendComponent alpha = {};
};

struct ColorTargetDesc
{
    TextureFormat format = TextureFormat::None;
    BlendState blend_state = {};
    ColorWriteMask write_mask = ColorWriteMask::All;
};

struct DepthState
{
    bool test = false;
    bool write = false;
    CompareOp compare = CompareOp::LessOrEqual;
};

struct StencilFace
{
    StencilOp fail = StencilOp::Keep;
    StencilOp pass = StencilOp::Keep;
    StencilOp depth_fail = StencilOp::Keep;
    CompareOp compare = CompareOp::Always;
};

struct StencilState
{
    bool enable = false;
    u8 read_mask = 0xFF;
    u8 write_mask = 0xFF;
    u8 reference = 0;
    StencilFace front = {};
    StencilFace back = {};
};

struct Viewport
{
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 width = 0.0f;
    f32 height = 0.0f;
    f32 min_depth = 0.0f;
    f32 max_depth = 1.0f;
};

struct Scissor
{
    i32 x = 0;
    i32 y = 0;
    u32 width = 0;
    u32 height = 0;
};

struct ShaderBindingTable
{
    Handle<Buffer> buffer;
    VkStridedDeviceAddressRegionKHR ray_gen = {};
    VkStridedDeviceAddressRegionKHR miss = {};
    VkStridedDeviceAddressRegionKHR hit = {};
};

struct Pipeline
{
    Pipeline() = default;
    GFX_NON_COPYABLE(Pipeline);

    VkPipeline vk_pipeline = nullptr;
    VkPipelineLayout vk_pipeline_layout = nullptr;
    Vec3u local_group_size = {};
    ShaderBindingTable shader_binding_table = {};
#if GFX_USE_SPIRV_REFLECT
    std::vector<SpvReflectShaderModule> reflections;
    const void* validated_push_layout = nullptr;
    const void* validated_global_layout = nullptr;
#endif
#if GFX_DEBUG
    u32 num_color_targets = 0;
    TextureFormat color_formats[GFX_MAX_COLOR_TARGETS] = {};
    TextureFormat depth_format = TextureFormat::None;
    TextureFormat stencil_format = TextureFormat::None;
#endif
};

struct SwapchainConfig
{
    TextureFormat format = TextureFormat::BGRA8_sRGB;
    TextureFormat depth_format = TextureFormat::None;
    PresentMode present_mode = PresentMode::Fifo;
};

struct BufferDesc
{
    const char* debug_name = nullptr;
    const void* data = nullptr;
    u64 buffer_size = 0;
    u64 alignment = 1;
    BufferUsage buffer_usage = BufferUsage::VertexBuffer;
    MemoryType memory_type = MemoryType::CPU_GPU;
    SharingMode sharing_mode = SharingMode::Exclusive;
};

struct SamplerDesc
{
    SamplerFilter min_filter = SamplerFilter::Linear;
    SamplerFilter mag_filter = SamplerFilter::Linear;
    SamplerMipMode mipmap_mode = SamplerMipMode::Linear;
    SamplerAddressMode address_mode_u = SamplerAddressMode::ClampToEdge;
    SamplerAddressMode address_mode_v = SamplerAddressMode::ClampToEdge;
    SamplerAddressMode address_mode_w = SamplerAddressMode::ClampToEdge;
    f32 max_anisotropy = 1.0f;
    bool compare_enable = false;
    CompareOp compare_op = CompareOp::LessOrEqual;
    BorderColor border_color = BorderColor::TransparentBlack;
};

enum class DefaultSampler : u32
{
    NearestClamp = 0,
    NearestWrap,
    LinearClamp,
    LinearWrap,
    AnisotropicClamp,
    AnisotropicWrap,
    ShadowLessOrEqual,
    ShadowGreaterOrEqual,
    Count
};

struct TextureDesc
{
    const char* debug_name = nullptr;
    const void* data = nullptr;
    TextureType texture_type = TextureType::Texture2D;
    TextureFormat texture_format = TextureFormat::None;
    TextureUsage texture_usage = TextureUsage::None;
    MemoryType memory_type = MemoryType::GPU;
    Vec3u texture_extent = {};
    u32 num_layers = 1;
    u32 num_samples = 1;
    u32 num_mipmaps = 1;
    Handle<Sampler> sampler;
};

struct TextureViewDesc
{
    const char* debug_name = nullptr;
    Handle<Texture> texture;
    TextureAspect aspect = TextureAspect::Auto;
    u32 start_mip_level = 0;
    u32 num_mipmaps = 1;
    u32 start_array_layer = 0;
    u32 num_layers = 1;
};

struct PipelineDesc
{
    const char* debug_name = nullptr;

    Handle<ShaderModule> VS;
    Handle<ShaderModule> FS;
    Handle<ShaderModule> CS;
    Handle<ShaderModule> GS;
    Handle<ShaderModule> TS;
    Handle<ShaderModule> MS;

    u32 num_color_targets = 0;
    ColorTargetDesc color_targets[GFX_MAX_COLOR_TARGETS] = {};
    TextureFormat depth_format = TextureFormat::None;
    TextureFormat stencil_format = TextureFormat::None;

    Topology topology = Topology::TriangleList;
    bool wireframe = false;
    u32 num_samples = 1;
};

struct HitGroup
{
    Handle<ShaderModule> closest_hit;
    Handle<ShaderModule> any_hit;
    Handle<ShaderModule> intersection;
};

struct RayTracingPipelineDesc
{
    const char* debug_name = nullptr;
    Handle<ShaderModule> ray_gen;
    Span<Handle<ShaderModule>> ray_miss;
    Span<HitGroup> hit_groups;
    u32 max_recursion = 1;
};

enum class GpuPreference : u8
{
    Discrete,
    Integrated,
    Any,
};

enum class Feature : u32
{
    None = 0,
    MeshShader = 1 << 0,         // VK_EXT_mesh_shader
    RayTracing = 1 << 1,         // VK_KHR_ray_tracing_pipeline
    RayQuery = 1 << 2,           // VK_KHR_ray_query
    DrawIndirectCount = 1 << 3,  // drawIndirectCount
    MemoryBudget = 1 << 4,       // VK_EXT_memory_budget
};
GFX_ENUM_FLAGS(Feature);

enum class Validation : u32
{
    None = 0,
    Core = 1 << 0,
    Sync = 1 << 1,
};
GFX_ENUM_FLAGS(Validation);

struct DeviceFeatures
{
    VkPhysicalDeviceFeatures core10 = {};
    VkPhysicalDeviceVulkan11Features core11 = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    VkPhysicalDeviceVulkan12Features core12 = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    VkPhysicalDeviceVulkan13Features core13 = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};

    void* Link(void* next)
    {
        core11.pNext = &core12;
        core12.pNext = &core13;
        core13.pNext = next;
        return &core11;
    }

    template <class T>
    static bool HasAllFeatures(const T& supported_feature, const T& requested_feature, VkBool32 T::*last_feature)
    {
        constexpr size_t offset = std::is_same_v<T, VkPhysicalDeviceFeatures> ? 0 : sizeof(VkBaseOutStructure);
        const VkBool32* supported = reinterpret_cast<const VkBool32*>(reinterpret_cast<const u8*>(&supported_feature) + offset);
        const VkBool32* requested = reinterpret_cast<const VkBool32*>(reinterpret_cast<const u8*>(&requested_feature) + offset);
        const size_t count = &(requested_feature.*last_feature) - requested + 1;

        for (size_t i = 0; i < count; ++i)
        {
            if (requested[i] && !supported[i])
                return false;
        }
        return true;
    }
};

struct ExtensionFeatures
{
    VkPhysicalDeviceMeshShaderFeaturesEXT mesh_shader = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_pipeline = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
    VkPhysicalDeviceRayQueryFeaturesKHR ray_query = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
    VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR ray_tracing_maintenance1 = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MAINTENANCE_1_FEATURES_KHR};
    VkPhysicalDeviceRobustness2FeaturesEXT robustness2 = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT};

    void* Link(Feature features, void* next);
};

struct DeviceConfig
{
    const char* app_name = "gfx";                            // application name
    void* window = nullptr;                                  // hwnd or glfwwindow*, null for no window
    SwapchainConfig swapchain = {};                          // swapchain format
    GpuPreference gpu_preference = GpuPreference::Discrete;  // gpu preference
    Validation validation = Validation::Core;                // validation layer checks
    Span<const SamplerDesc> samplers;                        // custom samplers

    Feature required_features = Feature::None;               // required, fails if not found
    Feature optional_features = Feature::None;               // optional 
    DeviceFeatures device_features = {};                     // extra vulkan core features
    Span<const char* const> extensions;                      // extra device extensions
    void* device_features_next = nullptr;                    // extra feature structs chained to pNext

    u32 max_buffers = 16384;                                 // pool capacities          
    u32 max_textures = 16384;
    u32 max_pipelines = 16384;
    u32 max_shaders = 16384;
    u32 max_acceleration_structures = 16384;
};

enum class BindPoint : u8
{
    Graphics = 0,
    Compute,
    RayTracing,
};

constexpr VkPipelineBindPoint ToVkPipelineBindPoint(BindPoint bind_point)
{
    switch (bind_point)
    {
    case BindPoint::Compute:
        return VK_PIPELINE_BIND_POINT_COMPUTE;
    case BindPoint::RayTracing:
        return VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
    default:
        return VK_PIPELINE_BIND_POINT_GRAPHICS;
    }
}

enum class RenderPassType : u8
{
    None = 0,
    Graphics,
    Compute
};

enum class IndexType : u8
{
    U16 = 0,
    U32
};

constexpr VkIndexType ToVkIndexType(IndexType index_type)
{
    return index_type == IndexType::U32 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
}

enum class AccelerationLevel : u8
{
    Bottom = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
    Top = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
};

enum class VertexFormat : u8
{
    RG32_Float = VK_FORMAT_R32G32_SFLOAT,
    RGB32_Float = VK_FORMAT_R32G32B32_SFLOAT,
    RG16_Float = VK_FORMAT_R16G16_SFLOAT,
    RGBA16_Float = VK_FORMAT_R16G16B16A16_SFLOAT,
    RG16_SNorm = VK_FORMAT_R16G16_SNORM,
    RGBA16_SNorm = VK_FORMAT_R16G16B16A16_SNORM,
};

enum class AccelerationBuild : u8
{
    None = 0,
    Updatable = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR,
    FastTrace = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
    FastBuild = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR,
};
GFX_ENUM_FLAGS(AccelerationBuild);

struct AccelerationStructureGeometry
{
    Handle<Buffer> vertex_buffer;
    u64 vertex_offset = 0;
    VertexFormat vertex_format = VertexFormat::RGB32_Float;
    u32 vertex_stride = 3 * sizeof(f32);
    u32 num_vertices = 0;
    Handle<Buffer> index_buffer;
    u64 index_offset = 0;
    IndexType index_type = IndexType::U32;
    u32 num_triangles = 0;
    Handle<Buffer> transform_buffer;
    u64 transform_offset = 0;
    bool opaque = true;
};

struct AccelerationStructureDesc
{
    const char* debug_name = nullptr;
    AccelerationLevel level = AccelerationLevel::Bottom;
    AccelerationBuild build = AccelerationBuild::FastTrace;
    AccelerationStructureGeometry geometry = {};
    u32 max_instances = 0;
};

struct AccelerationStructureInstance
{
    f32 transform[3][4] = {{ 1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f }};
    u32 custom_index : 24 = 0;
    u32 mask : 8 = 0xFF;
    u32 hit_group : 24 = 0;
    u32 flags : 8 = 0;
    u64 acceleration_structure_address = 0;
};
static_assert(sizeof(AccelerationStructureInstance) == sizeof(VkAccelerationStructureInstanceKHR));

struct AccelerationStructure
{
    AccelerationStructure() = default;
    GFX_NON_COPYABLE(AccelerationStructure);

    VkAccelerationStructureKHR vk_acceleration_structure = nullptr;
    Handle<Buffer> buffer;
    VkDeviceAddress device_address = 0;
    AccelerationLevel level = AccelerationLevel::Bottom;
    AccelerationBuild build = AccelerationBuild::None;
    AccelerationStructureGeometry geometry = {};
    VkAccelerationStructureGeometryKHR vk_geometry = {};
    u32 num_primitives = 0;
    u32 max_instances = 0;
    u64 build_scratch_size = 0;
    u64 update_scratch_size = 0;
    bool built = false;
};

struct TextureRegion
{
    Vec3i offset = {};
    Vec3u extent = {};
    u32 mip_level = 0;
    u32 base_layer = 0;
    u32 num_layers = 1;
};

struct TempAllocation
{
    u8* data = nullptr;
    u64 gpu_address = 0;
    Handle<Buffer> buffer;
    u64 offset = 0;
};

struct MemoryHeapInfo
{
    u64 size = 0;
    u64 budget = 0;
    u64 usage = 0;
    bool device_local = false;
    bool host_visible = false;
};

struct DeviceInfo
{
    const char* name = nullptr;
    const char* driver_name = nullptr;
    const char* driver_info = nullptr;
    VkPhysicalDeviceType type = VK_PHYSICAL_DEVICE_TYPE_OTHER;
    u32 api_version = 0;
    u32 vendor_id = 0;
    u32 device_id = 0;
    f32 timestamp_period_ns = 0.0f;
    u32 graphics_queue_family = 0;
    u32 transfer_queue_family = 0;
    const VkPhysicalDeviceLimits* limits = nullptr;
    u32 num_memory_heaps = 0;
    std::array<MemoryHeapInfo, VK_MAX_MEMORY_HEAPS> memory_heaps = {};
};

struct DebugUtilsFunctions
{
    PFN_vkCreateDebugUtilsMessengerEXT create_messenger = nullptr;
    PFN_vkDestroyDebugUtilsMessengerEXT destroy_messenger = nullptr;
    PFN_vkSetDebugUtilsObjectNameEXT set_object_name = nullptr;
    PFN_vkCmdBeginDebugUtilsLabelEXT cmd_begin_label = nullptr;
    PFN_vkCmdEndDebugUtilsLabelEXT cmd_end_label = nullptr;
    PFN_vkCmdInsertDebugUtilsLabelEXT cmd_insert_label = nullptr;
};

struct MeshShaderFunctions
{
    PFN_vkCmdDrawMeshTasksEXT draw_mesh_tasks = nullptr;
    PFN_vkCmdDrawMeshTasksIndirectEXT draw_mesh_tasks_indirect = nullptr;
    PFN_vkCmdDrawMeshTasksIndirectCountEXT draw_mesh_tasks_indirect_count = nullptr;
};

struct RayTracingFunctions
{
    PFN_vkCreateAccelerationStructureKHR create_acceleration_structure = nullptr;
    PFN_vkDestroyAccelerationStructureKHR destroy_acceleration_structure = nullptr;
    PFN_vkGetAccelerationStructureBuildSizesKHR get_build_sizes = nullptr;
    PFN_vkGetAccelerationStructureDeviceAddressKHR get_device_address = nullptr;
    PFN_vkCmdBuildAccelerationStructuresKHR cmd_build_acceleration_structs = nullptr;
    PFN_vkCreateRayTracingPipelinesKHR create_pipelines = nullptr;
    PFN_vkGetRayTracingShaderGroupHandlesKHR get_shader_group_handles = nullptr;
    PFN_vkCmdTraceRaysKHR cmd_trace_rays = nullptr;
};

struct PendingBarriers
{
    std::array<VkImageMemoryBarrier2, MAX_PENDING_BARRIERS> texture_barriers = {};
    std::array<VkBufferMemoryBarrier2, MAX_PENDING_BARRIERS> buffer_barriers = {};
    u32 num_texture_barriers = 0;
    u32 num_buffer_barriers = 0;
};

#if GFX_DEBUG
template <typename T>
void ValidatePushConstants(const CommandBuffer&, const T&)
{
}

template <typename T>
void ValidateGlobalConstants(CommandBuffer&, const T&)
{
}
#endif

class CommandBuffer
{
  public:
    CommandBuffer() = default;
    explicit CommandBuffer(Device& device) : device(&device) {}

    void PushConstants(const void* data, u32 size) const;

    template <typename T>
    GFX_FORCEINLINE void PushConstants(const T& data) const
    {
#if GFX_DEBUG
        ValidatePushConstants(*this, data);
#endif
        PushConstants(&data, sizeof(T));
    }

    void SetGlobalConstants(const void* data, u32 size) const;

    template <typename T>
    GFX_FORCEINLINE void SetGlobalConstants(const T& data)
    {
#if GFX_DEBUG
        ValidateGlobalConstants(*this, data);
#endif
        SetGlobalConstants(&data, sizeof(T));
    }

    void BindPipeline(Handle<Pipeline> pipeline, BindPoint bind_point);
    void BindIndexBuffer(Handle<Buffer> buffer, IndexType type, u64 offset = 0);

    void Draw(u32 vertex_count, u32 instance_count, u32 first_vertex = 0, u32 first_instance = 0) const;
    void DrawIndexed(u32 index_count, u32 instance_count, u32 first_index = 0, i32 vertex_offset = 0, u32 first_instance = 0) const;
    void DrawIndirect(Handle<Buffer> buffer, u64 offset = 0, u32 draw_count = 1, u32 stride = sizeof(VkDrawIndirectCommand)) const;
    void DrawIndexedIndirect(Handle<Buffer> buffer, u64 offset = 0, u32 draw_count = 1, u32 stride = sizeof(VkDrawIndexedIndirectCommand)) const;
    void DrawIndirectCount(Handle<Buffer> buffer, u64 offset, Handle<Buffer> count_buffer, u64 count_offset, u32 max_draw_count, u32 stride = sizeof(VkDrawIndirectCommand)) const;
    void DrawIndexedIndirectCount(Handle<Buffer> buffer, u64 offset, Handle<Buffer> count_buffer, u64 count_offset, u32 max_draw_count, u32 stride = sizeof(VkDrawIndexedIndirectCommand)) const;

    void DrawMeshTasks(u32 x, u32 y, u32 z) const;
    void DrawMeshTasksIndirect(Handle<Buffer> buffer, u64 offset = 0, u32 draw_count = 1, u32 stride = sizeof(VkDrawMeshTasksIndirectCommandEXT)) const;
    void DrawMeshTasksIndirectCount(Handle<Buffer> buffer, u64 offset, Handle<Buffer> count_buffer, u64 count_offset, u32 max_draw_count, u32 stride = sizeof(VkDrawMeshTasksIndirectCommandEXT)) const;

    void FillBuffer(Handle<Buffer> buffer, u32 value, u64 offset = 0, u64 size = VK_WHOLE_SIZE) const;
    void UpdateBuffer(Handle<Buffer> buffer, const void* data, u64 size, u64 offset = 0) const;
    void CopyBuffer(Handle<Buffer> source, Handle<Buffer> destination, u64 size, u64 source_offset = 0, u64 destination_offset = 0) const;
    void CopyBufferToTexture(Handle<Buffer> src_handle, u64 src_offset, Handle<Texture> dst_handle, const TextureRegion& region, TextureState final_state = TextureState::ShaderRead) const;
    void CopyBufferToTexture(Handle<Texture> dst, const TextureRegion& region, const void* src_data, u64 size);
    void CopyTexture(Handle<Texture> source, Handle<Texture> destination) const;
    void CopyTextureToBuffer(Handle<Texture> src, const TextureRegion& region, Handle<Buffer> dst, u64 offset = 0) const;

    void BeginRenderPass(const RenderPassDesc& desc);
    void EndRenderPass();
    void BeginComputePass(const ComputePassDesc& desc);
    void EndComputePass();

    void SetViewport(const Viewport& viewport) const;
    void SetScissor(const Scissor& scissor) const;
    void SetDepthState(const DepthState& state) const;
    void SetStencilState(const StencilState& state) const;
    void SetDepthBias(f32 constant, f32 slope, f32 clamp = 0.0f) const;
    void SetCullMode(CullMode cull_mode) const;
    void SetFrontFace(FrontFace front_face) const;

    void TransitionTexture(Handle<Texture> texture, TextureState state, PipelineStage stages = PipelineStage::None) const;
    void Barrier(PipelineStage src_stages, StageAccess src_access, PipelineStage dst_stages, StageAccess dst_access) const;

    void Dispatch(Vec3u threads) const;
    void DispatchGroups(u32 x, u32 y, u32 z) const;
    void DispatchIndirect(Handle<Buffer> buffer, u64 offset = 0) const;

    void BuildAccelerationStructures(Span<const Handle<AccelerationStructure>> handles) const;
    void BuildAccelerationStructure(Handle<AccelerationStructure> handle, Handle<Buffer> instances, u64 instances_offset, u32 num_instances, bool update = false) const;
    void TraceRays(u32 width, u32 height, u32 depth = 1) const;

    void BeginDebugLabel(const char* label, const Vec3& color);
    void EndDebugLabel();
    void InsertDebugLabel(const char* label, const Vec3& color);

    Device* device = nullptr;
    VkCommandBuffer vk_cmd_buffer = nullptr;
    u32 cmd_slot_idx = 0;
    Handle<Pipeline> current_pipeline;
    RenderPassType current_render_pass_type = RenderPassType::None;
    bool timed_pass = false;
#if GFX_DEBUG
    void (*global_constants_validator)(CommandBuffer& cmd) = nullptr;
    u32 num_pass_color_targets = 0;
    TextureFormat pass_color_formats[GFX_MAX_COLOR_TARGETS] = {};
    TextureFormat pass_depth_format = TextureFormat::None;
    TextureFormat pass_stencil_format = TextureFormat::None;
#endif

  private:
    friend struct CommandQueue;
    friend struct Device;

    void RequireTexture(Handle<Texture> texture, TextureState state, PipelineStage stages = PipelineStage::None, StageAccess access = StageAccess::None) const;
    void RequireBuffer(Handle<Buffer> buffer, PipelineStage stages, StageAccess access) const;
    void RequireAttachment(Handle<Texture> texture, TextureState state, bool discard, u32 width, u32 height, PipelineStage stages = PipelineStage::None, StageAccess access = StageAccess::None) const;
    void RequireInput(const PassResource& input, PipelineStage default_stages) const;
    void RequireOutput(const PassResource& output, PipelineStage default_stages) const;
    void FlushBarriers() const;

    void BeginPassTiming(const char* name);
    void EndPassTiming();

    void BuildAccelerationStructures(Span<const Handle<AccelerationStructure>> handles, bool update) const;
    PendingBarriers* pending_barriers = nullptr;
    u32 frame_slot = 0;
    u32 timestamp_end_index = 0;
};

struct SyncPoint
{
    VkSemaphore semaphore = nullptr;
    u64 value = 0;
};

struct Frame
{
    CommandBuffer cmd;
    Vec2u swapchain_size = {};
    bool swapchain_valid = false;
    u32 frame_index = 0;
    u64 frame_number = 0;
};

struct CommandQueue
{
    CommandQueue() = default;
    GFX_NON_MOVABLE(CommandQueue);

    void Initialize(Device& device, u32 family, u32 max_slots);
    void Shutdown(Device& device);

    CommandBuffer Acquire(Device& device);
    void End(const CommandBuffer& cmd, u64 timeline_value);
    void Submit(Device& device, const CommandBuffer& cmd, Span<VkSemaphoreSubmitInfo> waits = {}, Span<VkSemaphoreSubmitInfo> signals = {}, VkCommandBuffer upload_cmd = nullptr);

    u32 queue_family = 0;
    u32 slot = 0;
    u32 num_open_command_buffers = 0;
    VkQueue vk_queue = nullptr;
    VkCommandPool vk_command_pool = nullptr;
    std::vector<VkCommandBuffer> vk_cmd_buffers;
    std::vector<u64> slot_timeline_values;
    std::vector<PendingBarriers> pending_barriers;
};

struct UploadCopy
{
    VkBuffer buffer = nullptr;
    VkImage image = nullptr;
    TextureState final_state = TextureState::ShaderRead;
    VkBufferCopy buffer_region = {};
    VkBufferImageCopy texture_region = {};
    bool upload_start = false;
    bool upload_end = false;
};

struct UploadRing
{
    UploadRing() = default;
    GFX_NON_MOVABLE(UploadRing);

    constexpr static u64 BATCH_BYTES = GFX_UPLOAD_RING_SIZE / 4;
    constexpr static u32 MAX_BATCHES = 16;

    void Initialize(Device& device, VkSemaphore semaphore);
    void Shutdown(Device& device);
    void WriteBuffer(Device& device, Handle<Buffer> destination, const void* source);
    void WriteTexture(Device& device, Handle<Texture> destination, const void* source);
    void WriteShaderBindingTable(Device& device, const Pipeline& pipeline);
    void Commit(Device& device, u64 timeline_value);

  private:
    friend struct Device;

    struct Batch
    {
        u64 end = 0;
        u64 value = 0;
    };

    u64 Allocate(Device& device, u64 size, u64 alignment);
    void WaitForSpace(Device& device, u64 required_tail);

    Handle<Buffer> buffer;
    std::vector<UploadCopy> copies;
    u8* data = nullptr;
    u64 head = 0;
    u64 tail = 0;
    u64 open_bytes = 0;
    VkSemaphore timeline = nullptr;

    std::array<Batch, MAX_BATCHES> batches = {};
    u32 first_batch = 0;
    u32 num_batches = 0;
};

enum class DeletionType : u8
{
    Buffer = 0,
    Texture,
    ShaderModule,
    Pipeline,
    AccelerationStructure,
};

struct Deletion
{
    DeletionType type = DeletionType::Buffer;
    HandleBits::SizeType index = 0;
    bool bindless = false;
    GpuAllocation allocation = {};
    VkImage image = nullptr;
    VkImageView image_view = nullptr;
    VkShaderModule shader_module = nullptr;
    VkBuffer buffer = nullptr;
    VkPipeline pipeline = nullptr;
    VkAccelerationStructureKHR acceleration_structure = nullptr;
};

struct TempAllocator
{
    TempAllocator() = default;
    GFX_NON_MOVABLE(TempAllocator);

    void Initialize(Device& device);
    void Shutdown(Device& device);

    void NewFrame(u32 frame);
    void EndFrame();

    TempAllocation Allocate(u64 size, u64 alignment);

    Handle<Buffer> buffer;
    u8* data = nullptr;
    u64 gpu_address = 0;
    u64 uniform_range = 0;
    u64 head = 0;
    u64 end = 0;
    u64 peak = 0;
};

struct alignas(128) Device
{
    GFX_NON_MOVABLE(Device);

    friend struct Texture;
    friend struct Buffer;
    friend struct Swapchain;
    friend struct CommandQueue;
    friend struct BindlessResourcePool;
    friend struct TempAllocator;
    friend struct UploadRing;
    friend struct GpuAllocator;
    friend class CommandBuffer;

    
    friend Device& CreateDevice(const DeviceConfig& config);
    friend void DestroyDevice(Device& device);
    friend void SetCurrentDevice(Device& device);

  private:
    Device(const DeviceConfig& config, u32 device_index);

    void Initialize();
    void Shutdown();

  public:
    Handle<ShaderModule> CreateShader(const ShaderDesc& desc);
    Handle<Pipeline> CreatePipeline(const PipelineDesc& desc);
    Handle<Pipeline> CreateComputePipeline(const PipelineDesc& desc);
    Handle<Pipeline> CreateRayTracingPipeline(const RayTracingPipelineDesc& desc);
    Handle<Buffer> CreateBuffer(const BufferDesc& desc);
    Handle<Texture> CreateTexture(const TextureDesc& desc);
    Handle<Texture> CreateTextureView(const TextureViewDesc& desc);
    Handle<AccelerationStructure> CreateAccelerationStructure(const AccelerationStructureDesc& desc);

    void Destroy(Handle<Buffer> handle);
    void Destroy(Handle<ShaderModule> handle);
    void Destroy(Handle<Pipeline> handle);
    void Destroy(Handle<Texture> handle);
    void Destroy(Handle<AccelerationStructure> handle);

    CommandBuffer AcquireCommandBuffer();

    Frame NewFrame();
    SyncPoint SubmitFrame(Frame& frame);
    SyncPoint Submit(const CommandBuffer& cmd, Span<VkSemaphoreSubmitInfo> extra_waits = {}, Span<VkSemaphoreSubmitInfo> extra_signals = {});

    bool IsComplete(SyncPoint point) const;
    void WaitFor(SyncPoint point) const;
    void WaitForUploads();

    TempAllocation AllocateTemp(u64 size);
    TempAllocation AllocateTemp(u64 size, u64 alignment);

    Handle<Sampler> GetSampler(DefaultSampler sampler) const;
    Handle<Sampler> GetSampler(u32 config_index) const;

    bool IsFormatSupported(TextureFormat format, TextureUsage usage) const;

    bool HasFeature(Feature feature) const;
    Feature GetFeatures() const;

    bool IsExtensionEnabled(const char* name) const;
    bool IsExtensionSupported(const char* name) const;
    const DeviceFeatures& GetEnabledFeatures() const;

    template <class T>
    const T* FindEnabledFeatures() const
    {
        if constexpr (std::is_same_v<T, VkPhysicalDeviceFeatures>)
        {
            return &enabled_features.core10;
        }
        else
        {
            for (const VkBaseInStructure* chain = reinterpret_cast<const VkBaseInStructure*>(&enabled_features.core11); chain; chain = chain->pNext)
            {
                if (chain->sType == vku::GetSType<T>())
                    return reinterpret_cast<const T*>(chain);
            }
            return nullptr;
        }
    }

    template <class T>
    bool IsFeatureEnabled(VkBool32 T::* feature) const
    {
        const T* enabled = FindEnabledFeatures<T>();
        return enabled && enabled->*feature;
    }

    template <class T>
    bool IsFeatureSupported(VkBool32 T::* feature) const
    {
        if constexpr (std::is_same_v<T, VkPhysicalDeviceFeatures>)
        {
            VkPhysicalDeviceFeatures supported = {};
            vkGetPhysicalDeviceFeatures(vk_physical_device, &supported);
            return supported.*feature;
        }
        else
        {
            T supported = {};
            supported.sType = vku::GetSType<T>();
            VkPhysicalDeviceFeatures2 features2 =
            {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                .pNext = &supported,
            };
            vkGetPhysicalDeviceFeatures2(vk_physical_device, &features2);
            return supported.*feature;
        }
    }

    const Swapchain& GetSwapchain() const
    {
        GFX_ASSERT(swapchain.vk_swapchain && "No swapchain");
        return swapchain;
    }

    Handle<Texture> GetSwapchainTexture();
    Handle<Texture> GetSwapchainDepthTexture() const;

    DeviceInfo GetInfo() const;
    MemoryStats GetMemoryStats() const;

    struct GpuTiming
    {
        const char* name = nullptr;
        u64 timestamp_start = 0;
        u64 timestamp_end = 0;
    };

    const std::vector<GpuTiming>& GetGpuTimings() const { return gpu_timings; }

  private:

    void CreateInstance(const DeviceConfig& config);
    void SelectPhysicalDevice(const DeviceConfig& config);
    void CreateLogicalDevice(const DeviceConfig& config);
    void CreateFrameData();
    void CreateAllocator();
    void CreateSamplers(const DeviceConfig& config);

    void DeferDeletion(const Deletion& deletion);
    void DestroyDeletedResources(u32 frame);

    CommandBuffer RecordUploadCommands(u64 timeline_value);
    u64 FlushUploads();

    bool RecreateSwapchain();
    void Present();

    void SetEnabledFeatures(const DeviceConfig& config, Feature optional_features);
    void EnableExtension(const char* name);
    void EnableFeature(Feature feature);
    const char* FindUnsupportedFeature(VkPhysicalDevice gpu) const;

#if GFX_USE_SPIRV_REFLECT
    void ReflectPipelineStage(Pipeline& pipeline, const ShaderModule& shader);
#endif
    void LogDeviceInfo(Feature requested_features) const;

    template <class T>
    void SetDebugName(T handle, const char* name) const
    {
        if (!debug_utils.set_object_name || !name)
            return;

        const VkDebugUtilsObjectNameInfoEXT name_info =
        {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .objectType = vku::GetObjectType<T>(),
            .objectHandle = reinterpret_cast<u64>(handle),
            .pObjectName = name,
        };
        debug_utils.set_object_name(vk_device, &name_info);
    }

    GFX_FORCEINLINE VkImageView CreateImageView(const Texture& texture, TextureAspect aspect = TextureAspect::Auto);
    GFX_FORCEINLINE VkSemaphore CreateBinarySemaphore();
    GFX_FORCEINLINE VkSemaphore CreateTimelineSemaphore();
    GFX_FORCEINLINE VkAccelerationStructureGeometryKHR CreateAccelStructGeometry(const AccelerationStructureGeometry& input);

    u32 device_index = 0;
    VkInstance vk_instance = nullptr;
    VkPhysicalDevice vk_physical_device = nullptr;
    VkDevice vk_device = nullptr;

    BindlessResourcePool bindless_resource_pool;

    CommandQueue graphics_queue;
    CommandQueue upload_queue;
    u32 graphics_queue_family = INVALID_QUEUE_FAMILY;
    u32 transfer_queue_family = INVALID_QUEUE_FAMILY;
    VkSemaphore graphics_timeline = nullptr;
    u64 graphics_timeline_value = 0;

    bool timestamps_supported = false;
    bool ray_tracing_maintenance1_supported = false;
    bool null_descriptor_supported = false;

    VkSurfaceKHR vk_surface = nullptr;
    VkDebugUtilsMessengerEXT vk_debug_utils_messenger = nullptr;
    DebugUtilsFunctions debug_utils;
    MeshShaderFunctions mesh_shader_functions;
    RayTracingFunctions ray_tracing_functions;

    VkPhysicalDeviceProperties device_properties = {};
    VkPhysicalDeviceDriverProperties driver_properties = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES };
    VkPhysicalDeviceMemoryProperties memory_properties = {};
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_properties = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
    VkPhysicalDeviceAccelerationStructurePropertiesKHR acceleration_structure_properties = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
    std::array<VkFormatFeatureFlags, static_cast<size_t>(TextureFormat::MAX)> format_features = {};

    Feature features = Feature::None;
    DeviceFeatures enabled_features = {};
    ExtensionFeatures extension_features = {};
    std::vector<VkExtensionProperties> device_extensions;

    Swapchain swapchain;
    SwapchainConfig swapchain_config = {};

    struct FrameData
    {
        VkQueryPool query_pool = nullptr;
        VkSemaphore acquire_semaphore = nullptr;
        u32 num_timestamps = 0;
        std::vector<const char*> pass_names;
        std::vector<u64> timestamps;
        u64 last_graphics_value = 0;
        std::vector<Deletion> deletions;
    };

    UploadRing upload_ring;
    std::vector<VkImageMemoryBarrier2> upload_image_barriers;
    u64 last_upload_value = 0;

    u32 frame_index = 0;
    u32 last_frame_index = GFX_MAX_FRAMES_IN_FLIGHT - 1;
    u64 frame_number = 0;
    bool swapchain_valid = false;
    
    std::vector<GpuTiming> gpu_timings;
    std::vector<FrameData> frame_data;

    TempAllocator temp_allocator;
    Handle<Buffer> accelstruct_scratch_buffer;

    Pool<Buffer> buffer_pool;
    Pool<Texture> texture_pool;
    Pool<Sampler> sampler_pool;
    Pool<Pipeline> pipeline_pool;
    Pool<ShaderModule> shader_module_pool;
    Pool<AccelerationStructure> acceleration_structure_pool;
    std::vector<Handle<Sampler>> samplers;

    template <class T>
    Pool<T>& GetPool();

    template <class T>
    friend T& Get(Handle<T> handle);
    
    template <class T>
    friend bool IsAlive(Handle<T> handle);

#if GFX_USE_VMA
    VmaAllocator vma_allocator = nullptr;
#else
    GpuAllocator gpu_allocator;
#endif
};

struct alignas(128) DeviceContext
{
    std::unique_ptr<Device> devices[GFX_MAX_GPUS];
    Device* current = nullptr;
};

extern DeviceContext g_device_context;

Device&     CreateDevice(const DeviceConfig& config = {});
void        DestroyDevice(Device& device);
void        SetCurrentDevice(Device& device);
Device&     GetDevice();

template <>
GFX_INLINE Pool<Texture>& Device::GetPool<Texture>()
{
    return texture_pool;
}
template <>
GFX_INLINE Pool<Buffer>& Device::GetPool<Buffer>()
{
    return buffer_pool;
}
template <>
GFX_INLINE Pool<Sampler>& Device::GetPool<Sampler>()
{
    return sampler_pool;
}
template <>
GFX_INLINE Pool<Pipeline>& Device::GetPool<Pipeline>()
{
    return pipeline_pool;
}
template <>
GFX_INLINE Pool<ShaderModule>& Device::GetPool<ShaderModule>()
{
    return shader_module_pool;
}
template <>
GFX_INLINE Pool<AccelerationStructure>& Device::GetPool<AccelerationStructure>()
{
    return acceleration_structure_pool;
}

template <class T>
GFX_FORCEINLINE T& Get(Handle<T> handle)
{
    return g_device_context.devices[handle.DeviceIndex()]->template GetPool<T>().Get(handle);
}

template <class T>
GFX_FORCEINLINE bool IsAlive(Handle<T> handle)
{
    Device* device = g_device_context.devices[handle.DeviceIndex()].get();
    return device != nullptr && device->GetPool<T>().IsValidHandle(handle);
}

template <class T>
GFX_FORCEINLINE void Destroy(Handle<T> handle)
{
    Device* device = g_device_context.devices[handle.DeviceIndex()].get();
    if (device != nullptr)
        device->Destroy(handle);
}

}

#ifdef GFX_IMPLEMENTATION

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <vulkan/vk_enum_string_helper.h>

#if defined(GFX_WINDOW_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#elif defined(GFX_WINDOW_GLFW)
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#endif

#if GFX_USE_VMA
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>
#endif

namespace gfx
{

static constexpr const char* LogLevelText(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Debug:
        return "debug";
    default:
    case LogLevel::Info:
        return "info";
    case LogLevel::Warning:
        return "warn";
    case LogLevel::Error:
        return "error";
    case LogLevel::Fatal:
        return "fatal";
    }
}

static u32 FindMemoryType(const VkPhysicalDeviceMemoryProperties& properties, u32 type_bits, VkMemoryPropertyFlags required, VkMemoryPropertyFlags preferred)
{
    const VkMemoryPropertyFlags searches[] = {preferred, required};
    for (VkMemoryPropertyFlags flags : searches)
    {
        for (u32 i = 0; i < properties.memoryTypeCount; ++i)
        {
            if ((type_bits & (1u << i)) && (properties.memoryTypes[i].propertyFlags & flags) == flags)
                return i;
        }
    }

    GFX_LOG_FATAL("No memory type with flags %s in type bits %u", string_VkMemoryPropertyFlags(required).c_str(), type_bits);
    return 0;
}

static VKAPI_ATTR u32 VKAPI_CALL DebugMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void*)
{
    switch (message_severity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        GFX_LOG("%s %s", callback_data->pMessageIdName, callback_data->pMessage);
        return VK_FALSE;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        GFX_LOG_WARNING("%s %s", callback_data->pMessageIdName, callback_data->pMessage);
        return VK_FALSE;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        GFX_LOG_ERROR("%s %s", callback_data->pMessageIdName, callback_data->pMessage);
        return VK_FALSE;
    default:
        GFX_LOG("%s %s", callback_data->pMessageIdName, callback_data->pMessage);
        return VK_FALSE;
    }

    return VK_FALSE;
}

DeviceContext g_device_context;

#if !GFX_USE_VMA
void GpuAllocator::Initialize(Device& device)
{
    static_assert(GFX_GPU_BLOCK_SIZE <= UINT32_MAX);

    constexpr u64 MIB = 1ull << 20;
    for (u32 i = 0; i < device.memory_properties.memoryHeapCount; ++i)
    {
        const u64 heap_quarter = device.memory_properties.memoryHeaps[i].size / 4 / MIB * MIB;
        block_sizes[i] = std::min<u64>(GFX_GPU_BLOCK_SIZE, heap_quarter);
    }

    const u64 buffer_image_granularity = device.device_properties.limits.bufferImageGranularity;
    separate_image_blocks = buffer_image_granularity > MAX_SHARED_GRANULARITY;
    granularity = separate_image_blocks ? 1 : buffer_image_granularity;
}

void GpuAllocator::Shutdown(Device& device)
{
    for (u32 i = 0; i < blocks.size(); ++i)
    {
        const MemoryBlock& block = blocks[i];
        if (block.vk_memory && block.ranges.GetNumAllocations() == 0)
        {
            DestroyBlock(device, i);
        }
    }

    GFX_LOG("GPU memory: peak %llu MB allocated", peak_allocated_bytes >> 20);
}

GpuAllocation GpuAllocator::AllocateBuffer(Device& device, VkBuffer buffer, MemoryType memory_type, u64 min_alignment)
{
    VkMemoryDedicatedRequirements dedicated_requirements =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS,
    };
    VkMemoryRequirements2 requirements =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
        .pNext = &dedicated_requirements,
    };
    const VkBufferMemoryRequirementsInfo2 requirements_info =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
        .pNext = nullptr,
        .buffer = buffer,
    };
    
    vkGetBufferMemoryRequirements2(device.vk_device, &requirements_info, &requirements);
    requirements.memoryRequirements.alignment = std::max(requirements.memoryRequirements.alignment, min_alignment);

    const VkMemoryDedicatedAllocateInfo dedicated_info =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
        .pNext = nullptr,
        .image = nullptr,
        .buffer = buffer,
    };
    const u32 memory_type_index = FindMemoryType(device.memory_properties, requirements.memoryRequirements.memoryTypeBits, ToVkRequiredMemoryFlags(memory_type), ToVkPreferredMemoryFlags(memory_type));
    const bool dedicated = dedicated_requirements.prefersDedicatedAllocation || dedicated_requirements.requiresDedicatedAllocation;
    const GpuAllocation allocation = Allocate(device, requirements.memoryRequirements, dedicated, memory_type_index, false, dedicated_info);

    VK_CHECK(vkBindBufferMemory(device.vk_device, buffer, allocation.vk_memory, allocation.offset));
    return allocation;
}

GpuAllocation GpuAllocator::AllocateTexture(Device& device, VkImage image, MemoryType memory_type)
{
    VkMemoryDedicatedRequirements dedicated_requirements =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS,
    };
    VkMemoryRequirements2 requirements =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
        .pNext = &dedicated_requirements,
    };
    const VkImageMemoryRequirementsInfo2 requirements_info =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
        .pNext = nullptr,
        .image = image,
    };
    vkGetImageMemoryRequirements2(device.vk_device, &requirements_info, &requirements);

    const VkMemoryDedicatedAllocateInfo dedicated_info =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
        .pNext = nullptr,
        .image = image,
        .buffer = nullptr,
    };

    const u32 memory_type_index = FindMemoryType(device.memory_properties, requirements.memoryRequirements.memoryTypeBits, ToVkRequiredMemoryFlags(memory_type), ToVkPreferredMemoryFlags(memory_type));
    const bool dedicated = dedicated_requirements.prefersDedicatedAllocation || dedicated_requirements.requiresDedicatedAllocation;
    const GpuAllocation allocation = Allocate(device, requirements.memoryRequirements, dedicated, memory_type_index, true, dedicated_info);

    VK_CHECK(vkBindImageMemory(device.vk_device, image, allocation.vk_memory, allocation.offset));
    return allocation;
}

void GpuAllocator::Free(Device& device, const GpuAllocation& allocation)
{
    GFX_ASSERT(allocation.vk_memory);

    --num_allocations;
    used_bytes -= allocation.size;

    if (allocation.IsDedicated())
    {
        vkFreeMemory(device.vk_device, allocation.vk_memory, nullptr);
        allocated_bytes -= allocation.size;
        --num_dedicated;
        return;
    }

    GFX_ASSERT(allocation.block < blocks.size() && blocks[allocation.block].vk_memory == allocation.vk_memory);
    MemoryBlock& block = blocks[allocation.block];

    const OffsetAllocator::Allocation range =
    {
        .offset = static_cast<u32>(allocation.offset),
        .node_idx = allocation.node_index,
    };

    block.ranges.Free(range);
    if (block.ranges.GetNumAllocations() > 0)
        return;

    for (u32 i = 0; i < blocks.size(); ++i)
    {
        const MemoryBlock& other = blocks[i];
        const bool same_kind = other.memory_type == block.memory_type && other.images == block.images;
        if (i != allocation.block && other.vk_memory && same_kind && other.ranges.GetNumAllocations() == 0)
        {
            DestroyBlock(device, allocation.block);
            return;
        }
    }
}

MemoryStats GpuAllocator::GetStats() const
{
    MemoryStats stats =
    {
        .num_memory_objects = num_dedicated,
        .num_allocations = num_allocations,
        .allocated_bytes = allocated_bytes,
        .used_bytes = used_bytes,
    };

    for (const MemoryBlock& block : blocks)
    {
        stats.num_memory_objects += block.vk_memory ? 1 : 0;
    }

    return stats;
}

GpuAllocation GpuAllocator::Allocate(Device& device, const VkMemoryRequirements& requirements, bool dedicated, u32 memory_type_idx, bool images, const VkMemoryDedicatedAllocateInfo& dedicated_info)
{
    GFX_ASSERT(requirements.alignment <= UINT32_MAX);

    const u32 heap = device.memory_properties.memoryTypes[memory_type_idx].heapIndex;
    const u64 block_size = block_sizes[heap];

    ++num_allocations;
    used_bytes += requirements.size;

    if (dedicated || requirements.size > block_size / 2)
    {
        const MemoryBlock memory = AllocateMemory(device, requirements.size, memory_type_idx, images, &dedicated_info);
        ++num_dedicated;
        return
        {
            .vk_memory = memory.vk_memory,
            .mapped_memory = memory.mapped_memory,
            .size = requirements.size,
        };
    }

    const bool image_block = images && separate_image_blocks;
    const u32 size = static_cast<u32>(AlignUpPowerOfTwo(requirements.size, granularity));
    const u32 alignment = static_cast<u32>(std::max(requirements.alignment, granularity));

    u32 block_idx = INDEX_INVALID;
    OffsetAllocator::Allocation range = {};
    for (u32 i = 0; i < blocks.size(); ++i)
    {
        MemoryBlock& block = blocks[i];
        if (!block.vk_memory || block.memory_type != memory_type_idx || block.images != image_block)
            continue;

        range = block.ranges.Allocate(size, alignment);
        if (range.IsValid())
        {
            block_idx = i;
            break;
        }
    }

    if (block_idx == INDEX_INVALID)
    {
        u64 largest_block = 0;
        for (const MemoryBlock& block : blocks)
        {
            if (block.vk_memory && block.memory_type == memory_type_idx && block.images == image_block)
                largest_block = std::max(largest_block, block.size);
        }

        u64 new_block_size = block_size;
        for (u32 i = 0; i < NUM_SMALLER_BLOCKS; ++i)
        {
            const u64 smaller = new_block_size / 2;
            if (smaller <= largest_block || smaller < requirements.size * 2)
                break;
            new_block_size = smaller;
        }

        block_idx = CreateBlock(device, new_block_size, memory_type_idx, image_block);
        range = blocks[block_idx].ranges.Allocate(size, alignment);
        GFX_ASSERT(range.IsValid());
    }

    const MemoryBlock& block = blocks[block_idx];
    return
    {
        .vk_memory = block.vk_memory,
        .mapped_memory = block.mapped_memory ? block.mapped_memory + range.offset : nullptr,
        .offset = range.offset,
        .size = requirements.size,
        .block = block_idx,
        .node_index = range.node_idx,
    };
}

MemoryBlock GpuAllocator::AllocateMemory(Device& device, u64 size, u32 memory_type, bool images, const VkMemoryDedicatedAllocateInfo* dedicated_info)
{
    const VkMemoryAllocateFlagsInfo flags_info =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .pNext = dedicated_info,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        .deviceMask = 0,
    };
    const void* next = images ? static_cast<const void*>(dedicated_info) : static_cast<const void*>(&flags_info);

    const VkMemoryAllocateInfo allocate_info =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = next,
        .allocationSize = size,
        .memoryTypeIndex = memory_type,
    };

    MemoryBlock block =
    {
        .size = size,
        .memory_type = memory_type,
        .images = images,
    };
    VkResult result = vkAllocateMemory(device.vk_device, &allocate_info, nullptr, &block.vk_memory);
    if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY || result == VK_ERROR_OUT_OF_HOST_MEMORY)
    {
        for (u32 i = 0; i < blocks.size(); ++i)
        {
            if (blocks[i].vk_memory && blocks[i].ranges.GetNumAllocations() == 0)
                DestroyBlock(device, i);
        }
        result = vkAllocateMemory(device.vk_device, &allocate_info, nullptr, &block.vk_memory);
    }
    if (result != VK_SUCCESS)
        GFX_LOG_FATAL("Failed to allocate %llu bytes from memory type %u (heap %u): %s", size, memory_type, device.memory_properties.memoryTypes[memory_type].heapIndex, string_VkResult(result));

    const VkMemoryPropertyFlags property_flags = device.memory_properties.memoryTypes[memory_type].propertyFlags;
    if (property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        void* mapped = nullptr;
        VK_CHECK(vkMapMemory(device.vk_device, block.vk_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        block.mapped_memory = static_cast<u8*>(mapped);
    }

    allocated_bytes += size;
    peak_allocated_bytes = std::max(peak_allocated_bytes, allocated_bytes);
    return block;
}

u32 GpuAllocator::CreateBlock(Device& device, u64 size, u32 memory_type, bool images)
{
    u32 index = 0;
    while (index < blocks.size() && blocks[index].vk_memory)
        ++index;

    if (index == blocks.size())
        blocks.emplace_back();

    MemoryBlock& block = blocks[index];
    block = AllocateMemory(device, size, memory_type, images, nullptr);
    block.ranges.Initialize(static_cast<u32>(size), MAX_BLOCK_ALLOCATIONS);
    GFX_LOG_DEBUG("GPU memory block %u: %llu MB, memory type %u", index, size >> 20, memory_type);
    return index;
}

void GpuAllocator::DestroyBlock(Device& device, u32 index)
{
    MemoryBlock& block = blocks[index];
    vkFreeMemory(device.vk_device, block.vk_memory, nullptr);
    allocated_bytes -= block.size;
    block = {};
}
#endif

void GFX_Log(LogLevel log_level, const char* format, ...)
{
    char buffer[4096];

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    fprintf(stdout, "[gfx][%s] %s\n", LogLevelText(log_level), buffer);
    fflush(stdout);

    if (log_level == LogLevel::Fatal)
    {
        abort();
    }
}

void BindlessResourcePool::Initialize(Device& device)
{
    CreateDescriptorSet(device);
    CreateBindlessLayout(device);
    CreateNullResources(device);
    InitializeSlots(device);
}

void BindlessResourcePool::Shutdown(Device& device)
{
    device.Destroy(null_texture);
    device.Destroy(null_acceleration_structure);
    null_texture = {};
    null_acceleration_structure = {};
    null_sampled_info = {};
    null_storage_info = {};
    null_vk_acceleration_structure = nullptr;

    vkDestroyPipelineLayout(device.vk_device, pipeline_layout, nullptr);
    vkDestroyDescriptorPool(device.vk_device, uniform_pool, nullptr);
    vkDestroyDescriptorSetLayout(device.vk_device, uniform_layout, nullptr);
    vkDestroyDescriptorSetLayout(device.vk_device, bindless_layout, nullptr);
    vkDestroyDescriptorPool(device.vk_device, bindless_pool, nullptr);

    pipeline_layout = nullptr;
    uniform_pool = nullptr;
    uniform_layout = nullptr;
    uniform_descriptor_set = nullptr;
    bindless_layout = nullptr;
    bindless_pool = nullptr;
    bindless_descriptor_set = nullptr;
}

void BindlessResourcePool::CreateDescriptorSet(Device& device)
{
    std::array<VkDescriptorSetLayoutBinding, BindingSlot::MAX> bindings {};
    std::array<VkDescriptorBindingFlags, BindingSlot::MAX> flags {};
    const std::array<VkDescriptorType, BindingSlot::MAX> types
    {
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR
    };

    for (u32 i = 0; i < bindings.size(); ++i)
    {
        bindings[i].binding = i;
        bindings[i].descriptorType = types[i];
        bindings[i].descriptorCount = GFX_MAX_BINDLESS_DESCRIPTORS;
        bindings[i].stageFlags = VK_SHADER_STAGE_ALL;
        flags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    }

    u32 num_binding_slots = device.IsExtensionEnabled(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) ? BindingSlot::MAX : BindingSlot::MAX - 1;
    const VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .pNext = nullptr,
        .bindingCount = num_binding_slots,
        .pBindingFlags = flags.data(),
    };

    const VkDescriptorSetLayoutCreateInfo create_info =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &binding_flags,
        .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
        .bindingCount = num_binding_slots,
        .pBindings = bindings.data(),
    };
    VK_CHECK(vkCreateDescriptorSetLayout(device.vk_device, &create_info, nullptr, &bindless_layout));

    std::array<VkDescriptorPoolSize, BindingSlot::MAX> pool_sizes =
    {{
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, GFX_MAX_BINDLESS_DESCRIPTORS},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, GFX_MAX_BINDLESS_DESCRIPTORS},
            {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, GFX_MAX_BINDLESS_DESCRIPTORS},
    }};

    const VkDescriptorPoolCreateInfo pool_create_info =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
        .maxSets = 1,
        .poolSizeCount = num_binding_slots,
        .pPoolSizes = pool_sizes.data(),
    };
    VK_CHECK(vkCreateDescriptorPool(device.vk_device, &pool_create_info, nullptr, &bindless_pool));

    const VkDescriptorSetAllocateInfo allocate_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = bindless_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &bindless_layout,
    };
    VK_CHECK(vkAllocateDescriptorSets(device.vk_device, &allocate_info, &bindless_descriptor_set));
}

void BindlessResourcePool::CreateBindlessLayout(Device& device)
{
    const VkDescriptorSetLayoutBinding uniform_binding =
    {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_ALL,
    };

    const VkDescriptorSetLayoutCreateInfo layout_info =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = 1,
        .pBindings = &uniform_binding,
    };
    VK_CHECK(vkCreateDescriptorSetLayout(device.vk_device, &layout_info, nullptr, &uniform_layout));

    const VkDescriptorPoolSize pool_size =
    {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        .descriptorCount = 1,
    };

    const VkDescriptorPoolCreateInfo pool_info =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &pool_size,
    };
    VK_CHECK(vkCreateDescriptorPool(device.vk_device, &pool_info, nullptr, &uniform_pool));

    const VkDescriptorSetAllocateInfo uniform_allocate_info =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = uniform_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &uniform_layout,
    };
    VK_CHECK(vkAllocateDescriptorSets(device.vk_device, &uniform_allocate_info, &uniform_descriptor_set));

    const VkDescriptorBufferInfo buffer_info =
    {
        .buffer = device.buffer_pool.Get(device.temp_allocator.buffer).vk_buffer,
        .offset = 0,
        .range = device.temp_allocator.uniform_range,
    };

    const VkWriteDescriptorSet uniform_write =
    {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = uniform_descriptor_set,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        .pBufferInfo = &buffer_info,
    };
    vkUpdateDescriptorSets(device.vk_device, 1, &uniform_write, 0, nullptr);

    VkDescriptorSetLayout layouts[2];
    layouts[0] = bindless_layout;
    layouts[1] = uniform_layout;

    VkPushConstantRange push_range =
    {
        .stageFlags = VK_SHADER_STAGE_ALL,
        .offset = 0,
        .size = MAX_PUSH_CONSTANTS_SIZE
    };

    const VkPipelineLayoutCreateInfo pipeline_layout_info =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 2,
        .pSetLayouts = layouts,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_range,
    };

    VK_CHECK(vkCreatePipelineLayout(device.vk_device, &pipeline_layout_info, nullptr, &pipeline_layout));
}

void BindlessResourcePool::CreateNullResources(Device& device)
{
    if (device.null_descriptor_supported)
    {
        const Sampler& sampler = device.sampler_pool.Get(device.GetSampler(DefaultSampler::LinearClamp));
        null_sampled_info =
        {
            .sampler = sampler.vk_sampler,
            .imageView = nullptr,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };
        null_storage_info =
        {
            .sampler = nullptr,
            .imageView = nullptr,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };
        return;
    }

    u32 null_color = 0;
    null_texture = device.CreateTexture(
    {
        .debug_name = "null texture",
        .data = &null_color,
        .texture_type = TextureType::Texture2D,
        .texture_format = TextureFormat::RGBA8_UNorm,
        .texture_usage = TextureUsage::Sample | TextureUsage::Storage,
        .memory_type = MemoryType::GPU,
        .texture_extent = {1, 1, 1},
    });
    GFX_ASSERT(null_texture.IsValid() && null_texture.Index() == 0);

    const Texture& texture = device.texture_pool.Get(null_texture);
    const Sampler& sampler = device.sampler_pool.Get(texture.sampler);
    null_sampled_info =
    {
        .sampler = sampler.vk_sampler,
        .imageView = texture.vk_image_view,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };
    null_storage_info =
    {
        .sampler = nullptr,
        .imageView = texture.vk_image_view,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };

    if (device.features & (Feature::RayTracing | Feature::RayQuery))
    {
        null_acceleration_structure = device.CreateAccelerationStructure(
        {
            .debug_name = "null acceleration structure",
            .level = AccelerationLevel::Top,
        });

        CommandBuffer cmd = device.AcquireCommandBuffer();
        cmd.BuildAccelerationStructure(null_acceleration_structure, {}, 0, 0);
        device.Submit(cmd);
        null_vk_acceleration_structure = null_acceleration_structure->vk_acceleration_structure;
    }
}

void BindlessResourcePool::InitializeSlots(const Device& device)
{
    const std::vector<VkDescriptorImageInfo> sampled_infos(GFX_MAX_BINDLESS_DESCRIPTORS, null_sampled_info);
    const std::vector<VkDescriptorImageInfo> storage_infos(GFX_MAX_BINDLESS_DESCRIPTORS, null_storage_info);

    const VkWriteDescriptorSet writes[2] =
    {
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = bindless_descriptor_set,
            .dstBinding = TextureBinding,
            .descriptorCount = GFX_MAX_BINDLESS_DESCRIPTORS,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = sampled_infos.data()
        },
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = bindless_descriptor_set,
            .dstBinding = StorageBinding,
            .descriptorCount = GFX_MAX_BINDLESS_DESCRIPTORS,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = storage_infos.data()
        }};
    vkUpdateDescriptorSets(device.vk_device, 2, writes, 0, nullptr);

    if (device.features & (Feature::RayTracing | Feature::RayQuery))
    {
        const std::vector<VkAccelerationStructureKHR> structures(GFX_MAX_BINDLESS_DESCRIPTORS, null_vk_acceleration_structure);
        const VkWriteDescriptorSetAccelerationStructureKHR structure_info =
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
            .accelerationStructureCount = static_cast<u32>(structures.size()),
            .pAccelerationStructures = structures.data(),
        };

        const VkWriteDescriptorSet structure_write =
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = &structure_info,
            .dstSet = bindless_descriptor_set,
            .dstBinding = RayAccelerationBinding,
            .descriptorCount = static_cast<u32>(structures.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
        };
        vkUpdateDescriptorSets(device.vk_device, 1, &structure_write, 0, nullptr);
    }
}

void BindlessResourcePool::StoreTexture(Device& device, Handle<Texture> handle)
{
    GFX_ASSERT(handle.IsValid());

    std::array<VkWriteDescriptorSet, 2> writes {};
    std::array<VkDescriptorImageInfo, 2> image_infos {};

    u32 index = 0;

    const Texture* texture_ptr = device.texture_pool.GetPtr(handle);
    GFX_ASSERT(texture_ptr != nullptr);
    const Texture& texture = *texture_ptr;
    GFX_ASSERT(handle.Index() < static_cast<u32>(GFX_MAX_BINDLESS_DESCRIPTORS) && "Bindless resource pool is full");

    if (texture.vk_texture_usage_flags & VK_IMAGE_USAGE_SAMPLED_BIT)
    {
        image_infos[index].imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
        image_infos[index].imageView = texture.vk_image_view;
        const Sampler* sampler = device.sampler_pool.GetPtr(texture.sampler);
        image_infos[index].sampler = sampler != nullptr ? sampler->vk_sampler : nullptr;

        writes[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[index].dstSet = bindless_descriptor_set;
        writes[index].dstBinding = TextureBinding;
        writes[index].dstArrayElement = static_cast<u32>(handle.Index());
        writes[index].descriptorCount = 1;
        writes[index].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[index].pImageInfo = &image_infos[index];
        index++;
    }

    if (texture.vk_texture_usage_flags & VK_IMAGE_USAGE_STORAGE_BIT)
    {
        image_infos[index].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_infos[index].imageView = texture.vk_image_view;
        const Sampler* sampler = device.sampler_pool.GetPtr(texture.sampler);
        image_infos[index].sampler = sampler != nullptr ? sampler->vk_sampler : nullptr;

        writes[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[index].dstSet = bindless_descriptor_set;
        writes[index].dstBinding = StorageBinding;
        writes[index].dstArrayElement = static_cast<u32>(handle.Index());
        writes[index].descriptorCount = 1;
        writes[index].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[index].pImageInfo = &image_infos[index];
        index++;
    }

    if (index > 0)
    {
        vkUpdateDescriptorSets(device.vk_device, index, writes.data(), 0, nullptr);
    }
}

void BindlessResourcePool::StoreAccelerationStructure(Device& device, Handle<AccelerationStructure> handle)
{
    GFX_ASSERT(handle.Index() < static_cast<u32>(GFX_MAX_BINDLESS_DESCRIPTORS));
    const AccelerationStructure& structure = device.acceleration_structure_pool.Get(handle);

    const VkWriteDescriptorSetAccelerationStructureKHR structure_info =
    {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
        .accelerationStructureCount = 1,
        .pAccelerationStructures = &structure.vk_acceleration_structure,
    };

    const VkWriteDescriptorSet write =
    {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = &structure_info,
        .dstSet = bindless_descriptor_set,
        .dstBinding = RayAccelerationBinding,
        .dstArrayElement = static_cast<u32>(handle.Index()),
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
    };
    vkUpdateDescriptorSets(device.vk_device, 1, &write, 0, nullptr);
}

void BindlessResourcePool::ClearTexture(Device& device, u32 slot)
{
    GFX_ASSERT(slot < static_cast<u32>(GFX_MAX_BINDLESS_DESCRIPTORS));

    const VkWriteDescriptorSet writes[2] =
    {
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = bindless_descriptor_set,
            .dstBinding = TextureBinding,
            .dstArrayElement = slot,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &null_sampled_info,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
        },
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = bindless_descriptor_set,
            .dstBinding = StorageBinding,
            .dstArrayElement = slot,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &null_storage_info,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
        }};
    vkUpdateDescriptorSets(device.vk_device, 2, writes, 0, nullptr);
}

void BindlessResourcePool::ClearAccelerationStructure(Device& device, u32 slot)
{
    GFX_ASSERT(slot < static_cast<u32>(GFX_MAX_BINDLESS_DESCRIPTORS));

    const VkWriteDescriptorSetAccelerationStructureKHR structure_info =
    {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
        .accelerationStructureCount = 1,
        .pAccelerationStructures = &null_vk_acceleration_structure,
    };

    const VkWriteDescriptorSet write =
    {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = &structure_info,
        .dstSet = bindless_descriptor_set,
        .dstBinding = RayAccelerationBinding,
        .dstArrayElement = slot,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
    };
    vkUpdateDescriptorSets(device.vk_device, 1, &write, 0, nullptr);
}

void Swapchain::Destroy(Device& device)
{
    for (const Handle<Texture> texture : swapchain_textures)
    {
        const Texture* tex = device.texture_pool.GetPtr(texture);
        if (tex != nullptr)
        {
            vkDestroyImageView(device.vk_device, tex->vk_image_view, nullptr);
            device.texture_pool.Free(texture);
            device.texture_pool.Release(texture.Index());
        }
    }
    swapchain_textures = {};

    if (swapchain_depth_texture.IsValid())
        device.Destroy(swapchain_depth_texture);
    swapchain_depth_texture = {};

    for (const VkSemaphore& semaphore : render_finished_semaphores)
    {
        vkDestroySemaphore(device.vk_device, semaphore, nullptr);
    }
    render_finished_semaphores = {};

    if (vk_swapchain)
        vkDestroySwapchainKHR(device.vk_device, vk_swapchain, nullptr);
    vk_swapchain = nullptr;
}

void OffsetAllocator::Initialize(u32 total_size, u32 capacity)
{
    GFX_ASSERT(total_size > 0 && capacity > 0);
    GFX_ASSERT(static_cast<u64>(capacity) * 2 + 1 <= NODE_USED);

    size = total_size;
    max_allocations = capacity;
    const size_t max_nodes = 2 * static_cast<size_t>(capacity) + 1;
    nodes.resize(max_nodes);
    free_node_indices.resize(max_nodes);
    Reset();
}

void OffsetAllocator::Reset()
{
    GFX_ASSERT(max_allocations > 0);

    num_allocations = 0;
    free_bytes = size;
    top_bin_mask = 0;
    leaf_bin_masks.fill(0);
    bin_head_indices.fill(NODE_UNUSED);

    num_nodes = 0;
    num_free_nodes = 0;

    const NodeIndex root_idx = CreateNode(0, size);
    AddToBin(root_idx);
}

OffsetAllocator::Allocation OffsetAllocator::Allocate(u32 allocation_size, u32 alignment)
{
    GFX_ASSERT(allocation_size > 0);
    GFX_ASSERT(IsPow2(alignment));

    if (num_allocations >= max_allocations)
        return {};

    const NodeIndex node_idx = FindFreeNodeIndex(allocation_size, alignment);
    if (node_idx == NODE_UNUSED)
        return {};

    RemoveFromBin(node_idx);

    Node& node = nodes[node_idx];
    const u32 node_offset = node.offset;
    const u32 node_end = node.offset + node.size;
    const u32 offset = static_cast<u32>(AlignUpPowerOfTwo(node_offset, alignment));
    const u32 end = offset + allocation_size;

    if (offset > node_offset)
    {
        const NodeIndex padding_idx = CreateNode(node_offset, offset - node_offset);
        LinkPrev(node_idx, padding_idx);
        AddToBin(padding_idx);
    }

    if (end < node_end)
    {
        const NodeIndex remainder_idx = CreateNode(end, node_end - end);
        LinkNext(node_idx, remainder_idx);
        AddToBin(remainder_idx);
    }

    node.offset = offset;
    node.size = allocation_size;
    node.bin_prev_idx = NODE_USED;
    ++num_allocations;
    free_bytes -= allocation_size;

    return
    {
        .offset = offset,
        .node_idx = node_idx,
    };
}

void OffsetAllocator::Free(Allocation allocation)
{
    GFX_ASSERT(allocation.node_idx < num_nodes);

    const NodeIndex node_idx = allocation.node_idx;
    Node& node = nodes[node_idx];
    GFX_ASSERT(node.IsUsed() && node.offset == allocation.offset && "double free");

    --num_allocations;
    free_bytes += node.size;

    const NodeIndex prev_idx = node.neighbor_prev_idx;
    if (prev_idx != NODE_UNUSED && !nodes[prev_idx].IsUsed())
    {
        RemoveFromBin(prev_idx);
        node.offset = nodes[prev_idx].offset;
        node.size += nodes[prev_idx].size;
        
        Unlink(prev_idx);
        ReleaseNode(prev_idx);
    }

    const NodeIndex next_idx = node.neighbor_next_idx;
    if (next_idx != NODE_UNUSED && !nodes[next_idx].IsUsed())
    {
        RemoveFromBin(next_idx);
        node.size += nodes[next_idx].size;
        
        Unlink(next_idx);
        ReleaseNode(next_idx);
    }

    AddToBin(node_idx);
}

u32 OffsetAllocator::GetAllocationSize(Allocation allocation) const
{
    GFX_ASSERT(allocation.node_idx < num_nodes && nodes[allocation.node_idx].IsUsed());
    return nodes[allocation.node_idx].size;
}

OffsetAllocator::Stats OffsetAllocator::GetStats() const
{
    Stats report =
    {
        .free_bytes = free_bytes,
        .num_free_ranges = num_nodes - num_free_nodes - num_allocations,
    };

    if (top_bin_mask)
    {
        const u32 top_idx = FindHighestSetBitIndex(top_bin_mask);
        const u32 leaf_idx = FindHighestSetBitIndex(leaf_bin_masks[top_idx]);
        const u32 bin_idx = (top_idx << TOP_BINS_INDEX_SHIFT) | leaf_idx;
        for (NodeIndex node_index = bin_head_indices[bin_idx]; node_index != NODE_UNUSED; node_index = nodes[node_index].bin_next_idx)
            report.largest_free_range = std::max(report.largest_free_range, nodes[node_index].size);
    }

    return report;
}

u32 OffsetAllocator::BinIndexRoundUp(u32 size)
{
    if (size < MANTISSA_VALUE)
        return size;

    const u32 highest_bit = FindHighestSetBitIndex(size);
    const u32 mantissa_start = highest_bit - MANTISSA_BITS;
    const u32 exponent = mantissa_start + 1;
    const u32 low_bits = (1u << mantissa_start) - 1;
    u32 mantissa = (size >> mantissa_start) & MANTISSA_MASK;
    if (size & low_bits)
        ++mantissa;

    return (exponent << MANTISSA_BITS) + mantissa;
}

u32 OffsetAllocator::BinIndexRoundDown(u32 size)
{
    if (size < MANTISSA_VALUE)
        return size;

    const u32 highest_bit = FindHighestSetBitIndex(size);
    const u32 mantissa_start = highest_bit - MANTISSA_BITS;
    const u32 exponent = mantissa_start + 1;
    const u32 mantissa = (size >> mantissa_start) & MANTISSA_MASK;

    return (exponent << MANTISSA_BITS) | mantissa;
}

OffsetAllocator::NodeIndex OffsetAllocator::FindFreeNodeIndex(u32 allocation_size, u32 alignment) const
{
    const u64 worst_case_size = static_cast<u64>(allocation_size) + alignment - 1;
    u32 fallback_end_bin_idx = NUM_LEAF_BINS;

    if (worst_case_size <= size)
    {
        const u32 min_bin_idx = BinIndexRoundUp(static_cast<u32>(worst_case_size));
        u32 top_idx = min_bin_idx >> TOP_BINS_INDEX_SHIFT;
        u32 leaf_idx = INDEX_INVALID;
        if ((top_bin_mask >> top_idx) & 1u)
            leaf_idx = FindLowestSetBitIndexFrom(leaf_bin_masks[top_idx], min_bin_idx & LEAF_BINS_INDEX_MASK);

        if (leaf_idx == INDEX_INVALID)
        {
            top_idx = FindLowestSetBitIndexFrom(top_bin_mask, top_idx + 1);
            if (top_idx != INDEX_INVALID)
                leaf_idx = static_cast<u32>(std::countr_zero(static_cast<u32>(leaf_bin_masks[top_idx])));
        }

        if (leaf_idx != INDEX_INVALID)
            return bin_head_indices[(top_idx << TOP_BINS_INDEX_SHIFT) | leaf_idx];

        fallback_end_bin_idx = std::min(min_bin_idx, NUM_LEAF_BINS);
    }

    for (u32 bin_idx = BinIndexRoundDown(allocation_size); bin_idx < fallback_end_bin_idx; ++bin_idx)
    {
        for (NodeIndex node_idx = bin_head_indices[bin_idx]; node_idx != NODE_UNUSED; node_idx = nodes[node_idx].bin_next_idx)
        {
            const Node& node = nodes[node_idx];
            const u64 offset = AlignUpPowerOfTwo(node.offset, alignment);
            if (offset + allocation_size <= static_cast<u64>(node.offset) + node.size)
                return node_idx;
        }
    }

    return NODE_UNUSED;
}

GFX_FORCEINLINE OffsetAllocator::NodeIndex OffsetAllocator::CreateNode(u32 offset, u32 node_size)
{
    NodeIndex node_idx = 0;
    if (num_free_nodes > 0)
    {
        node_idx = free_node_indices[--num_free_nodes];
    }
    else
    {
        GFX_ASSERT(num_nodes < 2 * max_allocations + 1 && "out of range nodes");
        node_idx = static_cast<NodeIndex>(num_nodes++);
    }

    nodes[node_idx] =
    {
        .offset = offset,
        .size = node_size,
        .bin_prev_idx = NODE_UNUSED,
        .bin_next_idx = NODE_UNUSED,
        .neighbor_prev_idx = NODE_UNUSED,
        .neighbor_next_idx = NODE_UNUSED,
    };
    return node_idx;
}

GFX_FORCEINLINE void OffsetAllocator::ReleaseNode(NodeIndex node_idx)
{
    free_node_indices[num_free_nodes++] = node_idx;
}

GFX_FORCEINLINE void OffsetAllocator::AddToBin(NodeIndex node_idx)
{
    Node& node = nodes[node_idx];
    const u32 bin_idx = BinIndexRoundDown(node.size);
    const u32 top_idx = bin_idx >> TOP_BINS_INDEX_SHIFT;
    const u32 leaf_idx = bin_idx & LEAF_BINS_INDEX_MASK;

    leaf_bin_masks[top_idx] |= static_cast<u8>(1u << leaf_idx);
    top_bin_mask |= 1u << top_idx;

    NodeIndex& head_idx = bin_head_indices[bin_idx];
    node.bin_prev_idx = NODE_UNUSED;
    node.bin_next_idx = head_idx;
    if (head_idx != NODE_UNUSED)
        nodes[head_idx].bin_prev_idx = node_idx;
    head_idx = node_idx;
}

GFX_FORCEINLINE void OffsetAllocator::RemoveFromBin(NodeIndex node_idx)
{
    Node& node = nodes[node_idx];

    if (node.bin_prev_idx != NODE_UNUSED)
    {
        nodes[node.bin_prev_idx].bin_next_idx = node.bin_next_idx;
    }
    else
    {
        const u32 bin_idx = BinIndexRoundDown(node.size);
        NodeIndex& head_idx = bin_head_indices[bin_idx];
        GFX_ASSERT(head_idx == node_idx);
        head_idx = node.bin_next_idx;
        if (head_idx == NODE_UNUSED)
        {
            const u32 top_idx = bin_idx >> TOP_BINS_INDEX_SHIFT;
            const u32 leaf_idx = bin_idx & LEAF_BINS_INDEX_MASK;
            leaf_bin_masks[top_idx] &= static_cast<u8>(~(1u << leaf_idx));
            if (leaf_bin_masks[top_idx] == 0)
                top_bin_mask &= ~(1u << top_idx);
        }
    }
    if (node.bin_next_idx != NODE_UNUSED)
        nodes[node.bin_next_idx].bin_prev_idx = node.bin_prev_idx;

    node.bin_prev_idx = NODE_UNUSED;
    node.bin_next_idx = NODE_UNUSED;
}

GFX_FORCEINLINE void OffsetAllocator::LinkPrev(NodeIndex node_idx, NodeIndex new_idx)
{
    Node& node = nodes[node_idx];
    Node& new_node = nodes[new_idx];

    new_node.neighbor_prev_idx = node.neighbor_prev_idx;
    new_node.neighbor_next_idx = node_idx;
    if (node.neighbor_prev_idx != NODE_UNUSED)
        nodes[node.neighbor_prev_idx].neighbor_next_idx = new_idx;
    node.neighbor_prev_idx = new_idx;
}

GFX_FORCEINLINE void OffsetAllocator::LinkNext(NodeIndex node_idx, NodeIndex new_idx)
{
    Node& node = nodes[node_idx];
    Node& new_node = nodes[new_idx];

    new_node.neighbor_prev_idx = node_idx;
    new_node.neighbor_next_idx = node.neighbor_next_idx;
    if (node.neighbor_next_idx != NODE_UNUSED)
        nodes[node.neighbor_next_idx].neighbor_prev_idx = new_idx;
    node.neighbor_next_idx = new_idx;
}

GFX_FORCEINLINE void OffsetAllocator::Unlink(NodeIndex node_idx)
{
    const Node& node = nodes[node_idx];

    if (node.neighbor_prev_idx != NODE_UNUSED)
        nodes[node.neighbor_prev_idx].neighbor_next_idx = node.neighbor_next_idx;
    if (node.neighbor_next_idx != NODE_UNUSED)
        nodes[node.neighbor_next_idx].neighbor_prev_idx = node.neighbor_prev_idx;
}

u32 OffsetAllocator::FindHighestSetBitIndex(u32 mask)
{
    return static_cast<u32>(std::bit_width(mask)) - 1;
}

u32 OffsetAllocator::FindLowestSetBitIndexFrom(u32 mask, u32 start_bit_index)
{
    if (start_bit_index >= 32)
        return INDEX_INVALID;

    const u32 bits = mask & ~((1u << start_bit_index) - 1);
    return bits ? static_cast<u32>(std::countr_zero(bits)) : INDEX_INVALID;
}

bool Buffer::PrepareBarrier(PipelineStage new_stages, StageAccess new_access, VkBufferMemoryBarrier2& barrier)
{
    const bool only_reads = !(access & StageAccess::AllWrites) && !(new_access & StageAccess::AllWrites);
    const bool new_reader = (static_cast<u64>(new_stages) & ~static_cast<u64>(stages)) != 0 || (static_cast<u64>(new_access) & ~static_cast<u64>(access)) != 0;
    if (stages == PipelineStage::None || (only_reads && !new_reader))
    {
        stages |= new_stages;
        access |= new_access;
        return false;
    }

    barrier =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = static_cast<VkPipelineStageFlags2>(stages),
        .srcAccessMask = static_cast<VkAccessFlags2>(access),
        .dstStageMask = static_cast<VkPipelineStageFlags2>(new_stages),
        .dstAccessMask = static_cast<VkAccessFlags2>(new_access),
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = vk_buffer,
        .offset = 0,
        .size = VK_WHOLE_SIZE,
    };

    if (only_reads)
    {
        stages |= new_stages;
        access |= new_access;
    }
    else
    {
        stages = new_stages;
        access = new_access;
    }
    return true;
}

bool Texture::PrepareBarrier(TextureState new_state, VkImageMemoryBarrier2& barrier, PipelineStage new_stages, StageAccess new_access)
{
    Texture& owner = parent.IsValid() ? parent.Get() : *this;
    const TextureStateInfo info = GetTextureStateInfo(new_state);
    if (new_stages == PipelineStage::None)
    {
        new_stages = info.stages;
    }
    if (new_access == StageAccess::None)
    {
        new_access = info.access;
    }

    const bool only_reads = owner.state == new_state && !(owner.access & StageAccess::AllWrites) && !(new_access & StageAccess::AllWrites);
    const bool new_reader = (static_cast<u64>(new_stages) & ~static_cast<u64>(owner.stages)) != 0 || (static_cast<u64>(new_access) & ~static_cast<u64>(owner.access)) != 0;
    if (only_reads && (!new_reader || owner.stages == PipelineStage::None))
    {
        owner.stages |= new_stages;
        owner.access |= new_access;
        return false;
    }

    barrier =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = static_cast<VkPipelineStageFlags2>(owner.stages),
        .srcAccessMask = static_cast<VkAccessFlags2>(owner.access),
        .dstStageMask = static_cast<VkPipelineStageFlags2>(new_stages),
        .dstAccessMask = static_cast<VkAccessFlags2>(new_access),
        .oldLayout = GetTextureStateInfo(owner.state).layout,
        .newLayout = info.layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = owner.vk_image,
        .subresourceRange =
        {
            .aspectMask = GetImageAspectFlags(owner.texture_format),
            .baseMipLevel = 0,
            .levelCount = VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = 0,
            .layerCount = VK_REMAINING_ARRAY_LAYERS,
        },
    };

    owner.state = new_state;
    if (only_reads)
    {
        owner.stages |= new_stages;
        owner.access |= new_access;
    }
    else
    {
        owner.stages = new_stages;
        owner.access = new_access;
    }
    return true;
}

void* ExtensionFeatures::Link(Feature features, void* next)
{
    if (features & Feature::RayQuery)
    {
        ray_query.pNext = next;
        next = &ray_query;
    }
    if (features & Feature::RayTracing)
    {
        ray_tracing_pipeline.pNext = next;
        next = &ray_tracing_pipeline;
    }
    if (ray_tracing_maintenance1.rayTracingMaintenance1)
    {
        ray_tracing_maintenance1.pNext = next;
        next = &ray_tracing_maintenance1;
    }
    if (robustness2.nullDescriptor)
    {
        robustness2.pNext = next;
        next = &robustness2;
    }
    if (features & (Feature::RayTracing | Feature::RayQuery))
    {
        acceleration_structure.pNext = next;
        next = &acceleration_structure;
    }
    if (features & Feature::MeshShader)
    {
        mesh_shader.pNext = next;
        next = &mesh_shader;
    }
    return next;
}

void CommandBuffer::PushConstants(const void* data, u32 size) const
{
    GFX_ASSERT(size <= MAX_PUSH_CONSTANTS_SIZE && "push constants too big");
    vkCmdPushConstants(vk_cmd_buffer, current_pipeline->vk_pipeline_layout, VK_SHADER_STAGE_ALL, 0, size, data);
}

void CommandBuffer::SetGlobalConstants(const void* data, u32 size) const
{
    GFX_ASSERT(size <= device->temp_allocator.uniform_range);
    const TempAllocation allocation = device->AllocateTemp(size);
    memcpy(allocation.data, data, size);

    const VkDescriptorSet uniform_set = device->bindless_resource_pool.uniform_descriptor_set;
    const u32 offset = static_cast<u32>(allocation.offset);
    vkCmdBindDescriptorSets(vk_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, device->bindless_resource_pool.pipeline_layout, 1, 1, &uniform_set, 1, &offset);
    vkCmdBindDescriptorSets(vk_cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, device->bindless_resource_pool.pipeline_layout, 1, 1, &uniform_set, 1, &offset);
    if (device->features & Feature::RayTracing)
        vkCmdBindDescriptorSets(vk_cmd_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, device->bindless_resource_pool.pipeline_layout, 1, 1, &uniform_set, 1, &offset);
}

void CommandBuffer::BindPipeline(Handle<Pipeline> pipeline, BindPoint bind_point)
{
    if (current_pipeline == pipeline)
        return;

    current_pipeline = pipeline;
    vkCmdBindPipeline(vk_cmd_buffer, ToVkPipelineBindPoint(bind_point), pipeline->vk_pipeline);

#if GFX_DEBUG
    if (bind_point == BindPoint::Graphics && current_render_pass_type == RenderPassType::Graphics)
    {
        const Pipeline& graphics_pipeline = pipeline.Get();
        GFX_ASSERT(graphics_pipeline.num_color_targets == num_pass_color_targets);
        for (u32 i = 0; i < num_pass_color_targets; ++i)
            GFX_ASSERT(graphics_pipeline.color_formats[i] == pass_color_formats[i]);

        GFX_ASSERT(graphics_pipeline.depth_format == pass_depth_format);
        GFX_ASSERT(graphics_pipeline.stencil_format == pass_stencil_format);
    }

    if (global_constants_validator)
        global_constants_validator(*this);
#endif
}

void CommandBuffer::BindIndexBuffer(Handle<Buffer> buffer, IndexType type, u64 offset)
{
    vkCmdBindIndexBuffer(vk_cmd_buffer, buffer->vk_buffer, offset, ToVkIndexType(type));
}

void CommandBuffer::Draw(u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) const
{
    vkCmdDraw(vk_cmd_buffer, vertex_count, instance_count, first_vertex, first_instance);
}

void CommandBuffer::DrawIndexed(u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset, u32 first_instance) const
{
    vkCmdDrawIndexed(vk_cmd_buffer, index_count, instance_count, first_index, vertex_offset, first_instance);
}

void CommandBuffer::DrawIndirect(Handle<Buffer> buffer, u64 offset, u32 draw_count, u32 stride) const
{
    GFX_ASSERT(offset % 4 == 0 && stride % 4 == 0 && (draw_count <= 1 || stride >= sizeof(VkDrawIndirectCommand)));
    vkCmdDrawIndirect(vk_cmd_buffer, buffer->vk_buffer, offset, draw_count, stride);
}

void CommandBuffer::DrawIndexedIndirect(Handle<Buffer> buffer, u64 offset, u32 draw_count, u32 stride) const
{
    GFX_ASSERT(offset % 4 == 0 && stride % 4 == 0 && (draw_count <= 1 || stride >= sizeof(VkDrawIndexedIndirectCommand)));
    vkCmdDrawIndexedIndirect(vk_cmd_buffer, buffer->vk_buffer, offset, draw_count, stride);
}

void CommandBuffer::DrawIndirectCount(Handle<Buffer> buffer, u64 offset, Handle<Buffer> count_buffer, u64 count_offset, u32 max_draw_count, u32 stride) const
{
    GFX_ASSERT(device->HasFeature(Feature::DrawIndirectCount) && "DrawIndirectCount is not enabled");
    vkCmdDrawIndirectCount(vk_cmd_buffer, buffer->vk_buffer, offset, count_buffer->vk_buffer, count_offset, max_draw_count, stride);
}

void CommandBuffer::DrawIndexedIndirectCount(Handle<Buffer> buffer, u64 offset, Handle<Buffer> count_buffer, u64 count_offset, u32 max_draw_count, u32 stride) const
{
    GFX_ASSERT(device->HasFeature(Feature::DrawIndirectCount) && "DrawIndirectCount is not enabled");
    vkCmdDrawIndexedIndirectCount(vk_cmd_buffer, buffer->vk_buffer, offset, count_buffer->vk_buffer, count_offset, max_draw_count, stride);
}

void CommandBuffer::DrawMeshTasks(u32 x, u32 y, u32 z) const
{
    GFX_ASSERT(device->mesh_shader_functions.draw_mesh_tasks && "MeshShader is not enabled");
    device->mesh_shader_functions.draw_mesh_tasks(vk_cmd_buffer, x, y, z);
}

void CommandBuffer::DrawMeshTasksIndirect(Handle<Buffer> buffer, u64 offset, u32 draw_count, u32 stride) const
{
    GFX_ASSERT(device->mesh_shader_functions.draw_mesh_tasks_indirect && "MeshShader is not enabled");
    device->mesh_shader_functions.draw_mesh_tasks_indirect(vk_cmd_buffer, buffer->vk_buffer, offset, draw_count, stride);
}

void CommandBuffer::DrawMeshTasksIndirectCount(Handle<Buffer> buffer, u64 offset, Handle<Buffer> count_buffer, u64 count_offset, u32 max_draw_count, u32 stride) const
{
    GFX_ASSERT(device->mesh_shader_functions.draw_mesh_tasks_indirect_count && "MeshShader is not enabled");
    GFX_ASSERT(device->HasFeature(Feature::DrawIndirectCount) && "DrawIndirectCount is not enabled");
    device->mesh_shader_functions.draw_mesh_tasks_indirect_count(vk_cmd_buffer, buffer->vk_buffer, offset, count_buffer->vk_buffer, count_offset, max_draw_count, stride);
}

void CommandBuffer::FillBuffer(Handle<Buffer> buffer, u32 value, u64 offset, u64 size) const
{
    GFX_ASSERT(current_render_pass_type != RenderPassType::Graphics);
    GFX_ASSERT(offset % 4 == 0 && (size == VK_WHOLE_SIZE || size % 4 == 0));
    RequireBuffer(buffer, PipelineStage::Transfer, StageAccess::TransferWrite);
    FlushBarriers();
    vkCmdFillBuffer(vk_cmd_buffer, buffer->vk_buffer, offset, size, value);
}

void CommandBuffer::UpdateBuffer(Handle<Buffer> buffer, const void* data, u64 size, u64 offset) const
{
    GFX_ASSERT(current_render_pass_type != RenderPassType::Graphics);
    GFX_ASSERT(data || size == 0);
    GFX_ASSERT(offset + size <= buffer->buffer_size);
    if (size == 0)
        return;

    if (size <= MAX_INLINE_BUFFER_UPDATE && size % 4 == 0 && offset % 4 == 0)
    {
        RequireBuffer(buffer, PipelineStage::Transfer, StageAccess::TransferWrite);
        FlushBarriers();
        vkCmdUpdateBuffer(vk_cmd_buffer, buffer->vk_buffer, offset, size, data);
        return;
    }

    const TempAllocation staging = device->AllocateTemp(size, 16);
    memcpy(staging.data, data, size);
    CopyBuffer(staging.buffer, buffer, size, staging.offset, offset);
}

void CommandBuffer::CopyBuffer(Handle<Buffer> source, Handle<Buffer> destination, u64 size, u64 source_offset, u64 destination_offset) const
{
    GFX_ASSERT(current_render_pass_type != RenderPassType::Graphics);
    GFX_ASSERT(source_offset + size <= source->buffer_size && destination_offset + size <= destination->buffer_size);
    if (size == 0)
        return;

    RequireBuffer(source, PipelineStage::Transfer, StageAccess::TransferRead);
    RequireBuffer(destination, PipelineStage::Transfer, StageAccess::TransferWrite);
    FlushBarriers();

    const VkBufferCopy region =
    {
        .srcOffset = source_offset,
        .dstOffset = destination_offset,
        .size = size,
    };
    vkCmdCopyBuffer(vk_cmd_buffer, source->vk_buffer, destination->vk_buffer, 1, &region);
}

void CommandBuffer::CopyBufferToTexture(Handle<Buffer> src_handle, u64 src_offset, Handle<Texture> dst_handle, const TextureRegion& region, TextureState final_state) const
{
    GFX_ASSERT(current_render_pass_type != RenderPassType::Graphics);
    Handle<Buffer> src_buffer = src_handle;
    Handle<Texture> dst_texture = dst_handle;
    GFX_ASSERT(src_buffer.IsValid() && dst_texture.IsValid());

    Buffer& src = src_buffer.Get();
    Texture& dst = dst_texture.Get();
    GFX_ASSERT(!dst.IsView() && "copy into a view");
    GFX_ASSERT((src_offset % 4) == 0);
    GFX_ASSERT(region.extent.x && region.extent.y && region.extent.z);
    GFX_ASSERT(dst.owner_queue_family == device->graphics_queue_family);
    const FormatInfo format_info = GetFormatInfo(static_cast<TextureFormat>(dst.texture_format));
    GFX_ASSERT(!(format_info.depth && format_info.stencil) && "depth/stencil buffer copies are not supported");

    RequireBuffer(src_buffer, PipelineStage::Transfer, StageAccess::TransferRead);
    RequireTexture(dst_texture, TextureState::TransferDst);
    FlushBarriers();

    const VkBufferImageCopy copy =
    {
        .bufferOffset = src_offset,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource =
        {
            .aspectMask = GetImageAspectFlags(dst.texture_format),
            .mipLevel = region.mip_level,
            .baseArrayLayer = region.base_layer,
            .layerCount = region.num_layers,
        },
        .imageOffset = {region.offset.x, region.offset.y, region.offset.z},
        .imageExtent = {region.extent.x, region.extent.y, region.extent.z},
    };
    vkCmdCopyBufferToImage(vk_cmd_buffer, src.vk_buffer, dst.vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

    RequireTexture(dst_texture, final_state);
}

void CommandBuffer::CopyBufferToTexture(Handle<Texture> dst, const TextureRegion& region, const void* src_data, u64 size)
{
    const FormatInfo format_info = GetFormatInfo(static_cast<TextureFormat>(Handle<Texture>(dst)->texture_format));
    GFX_ASSERT(format_info.block_bytes > 0 && "invalid texture format");
    const u64 alignment = 16 % format_info.block_bytes == 0 ? 16 : format_info.block_bytes * 4;
    const TempAllocation staging = device->AllocateTemp(size, alignment);
    memcpy(staging.data, src_data, size);
    CopyBufferToTexture(staging.buffer, staging.offset, dst, region);
}

void CommandBuffer::CopyTexture(Handle<Texture> source, Handle<Texture> destination) const
{
    GFX_ASSERT(current_render_pass_type != RenderPassType::Graphics);
    GFX_ASSERT(!destination->IsView() && "copy into a view");
    GFX_ASSERT(destination->texture_extent.x >= source->texture_extent.x && destination->texture_extent.y >= source->texture_extent.y && destination->texture_extent.z >= source->texture_extent.z && "copy too big");
    GFX_ASSERT(destination->num_mipmaps >= source->num_mipmaps && destination->num_layers >= source->num_layers && "copy has more mips or layers than the destination");
    GFX_ASSERT(GetFormatInfo(static_cast<TextureFormat>(source->texture_format)).block_bytes == GetFormatInfo(static_cast<TextureFormat>(destination->texture_format)).block_bytes && "different copy formats");

    RequireTexture(source, TextureState::TransferSrc);
    RequireTexture(destination, TextureState::TransferDst);
    FlushBarriers();

    std::array<VkImageCopy, MAX_MIP_LEVELS> regions = {};
    for (u32 mip = 0; mip < source->num_mipmaps; ++mip)
    {
        regions[mip] =
        {
            .srcSubresource =
            {
                .aspectMask = GetImageAspectFlags(source->texture_format),
                .mipLevel = source->base_mip_level + mip,
                .baseArrayLayer = source->base_array_layer,
                .layerCount = source->num_layers,
            },
            .dstSubresource =
            {
                .aspectMask = GetImageAspectFlags(destination->texture_format),
                .mipLevel = mip,
                .layerCount = source->num_layers,
            },
            .extent =
            {
                std::max(source->texture_extent.x >> mip, 1u),
                std::max(source->texture_extent.y >> mip, 1u),
                std::max(source->texture_extent.z >> mip, 1u),
            },
        };
    }
    vkCmdCopyImage(vk_cmd_buffer, source->vk_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destination->vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, source->num_mipmaps, regions.data());
}

void CommandBuffer::CopyTextureToBuffer(Handle<Texture> src, const TextureRegion& region, Handle<Buffer> dst, u64 offset) const
{
    GFX_ASSERT(current_render_pass_type != RenderPassType::Graphics);
    GFX_ASSERT(!src->IsView() && "copy from a view");

    const FormatInfo format_info = GetFormatInfo(static_cast<TextureFormat>(src->texture_format));
    GFX_ASSERT((offset % format_info.block_bytes) == 0);
    GFX_ASSERT(!(format_info.depth && format_info.stencil) && "depth/stencil buffer copies are not supported");
    GFX_ASSERT(region.extent.x && region.extent.y && region.extent.z && region.num_layers);

    RequireTexture(src, TextureState::TransferSrc);
    RequireBuffer(dst, PipelineStage::Transfer, StageAccess::TransferWrite);
    FlushBarriers();

    const VkBufferImageCopy copy =
    {
        .bufferOffset = offset,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource =
        {
            .aspectMask = GetImageAspectFlags(src->texture_format),
            .mipLevel = region.mip_level,
            .baseArrayLayer = region.base_layer,
            .layerCount = region.num_layers,
        },
        .imageOffset = {region.offset.x, region.offset.y, region.offset.z},
        .imageExtent = {region.extent.x, region.extent.y, region.extent.z},
    };
    vkCmdCopyImageToBuffer(vk_cmd_buffer, src->vk_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->vk_buffer, 1, &copy);
}

void CommandBuffer::BeginRenderPass(const RenderPassDesc& desc)
{
    GFX_ASSERT(current_render_pass_type == RenderPassType::None);
    GFX_ASSERT(desc.color_targets.size() <= GFX_MAX_COLOR_TARGETS);

    current_render_pass_type = RenderPassType::Graphics;

    const Handle<Texture> depth_texture = desc.depth_target.texture;
    const Handle<Texture> stencil_texture = desc.stencil_target.texture;

    Handle<Texture> target_texture = !desc.color_targets.empty() ? desc.color_targets[0].texture : depth_texture;
    if (!target_texture.IsValid())
    {
        target_texture = stencil_texture;
    }
    
    const u32 width = target_texture->texture_extent.x;
    const u32 height = target_texture->texture_extent.y;

    for (const PassResource& input : desc.inputs)
    {
        RequireInput(input, PipelineStage::Fragment);
    }

    for (const PassResource& output : desc.outputs)
    {
        RequireOutput(output, PipelineStage::Fragment);
    }

    for (const ColorTarget& color : desc.color_targets)
    {
        RequireAttachment(color.texture, TextureState::ColorAttachment, color.load_op != LoadOp::Load, width, height);
        if (color.resolve_texture.IsValid())
            RequireAttachment(color.resolve_texture, TextureState::ColorAttachment, true, width, height);
    }

    const bool shared_depth_stencil = stencil_texture == depth_texture;
    if (depth_texture.IsValid())
    {
        const bool keeps_stencil = depth_texture->IsStencilTexture() && (!shared_depth_stencil || desc.stencil_target.load_op == LoadOp::Load);
        RequireAttachment(depth_texture, TextureState::DepthStencilAttachment, desc.depth_target.load_op != LoadOp::Load && !keeps_stencil, width, height);
    }

    if (stencil_texture.IsValid() && !shared_depth_stencil)
        RequireAttachment(stencil_texture, TextureState::DepthStencilAttachment, desc.stencil_target.load_op != LoadOp::Load && !stencil_texture->IsDepthTexture(), width, height);

    const Handle<Texture> depth_resolve = desc.depth_target.resolve_texture;
    const Handle<Texture> stencil_resolve = desc.stencil_target.resolve_texture;
    if (depth_resolve.IsValid())
        RequireAttachment(depth_resolve, TextureState::DepthStencilAttachment, !depth_resolve->IsStencilTexture() || stencil_resolve == depth_resolve, width, height, PipelineStage::ColorOutput, StageAccess::ColorWrite);

    if (stencil_resolve.IsValid() && stencil_resolve != depth_resolve)
        RequireAttachment(stencil_resolve, TextureState::DepthStencilAttachment, !stencil_resolve->IsDepthTexture(), width, height, PipelineStage::ColorOutput, StageAccess::ColorWrite);

    FlushBarriers();

    if (desc.debug_name)
    {
        BeginPassTiming(desc.debug_name);
        BeginDebugLabel(desc.debug_name, desc.debug_color);
        timed_pass = true;
    }

    static_assert(sizeof(ClearColor) == sizeof(VkClearColorValue));
    VkRenderingAttachmentInfo color_info[GFX_MAX_COLOR_TARGETS];
    for (u32 i = 0; i < desc.color_targets.size(); ++i)
    {
        const ColorTarget& color = desc.color_targets[i];
        color_info[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color_info[i].pNext = nullptr;
        color_info[i].imageView = color.texture->vk_image_view;
        color_info[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_info[i].resolveMode = color.resolve_texture.IsValid() ? static_cast<VkResolveModeFlagBits>(color.resolve_mode) : VK_RESOLVE_MODE_NONE;
        color_info[i].resolveImageView = color.resolve_texture.IsValid() ? color.resolve_texture->vk_image_view : nullptr;
        color_info[i].resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_info[i].loadOp = static_cast<VkAttachmentLoadOp>(color.load_op);
        color_info[i].storeOp = static_cast<VkAttachmentStoreOp>(color.store_op);
        color_info[i].clearValue = {};
        memcpy(&color_info[i].clearValue.color, &color.clear_color, sizeof(VkClearColorValue));
    }

    VkRenderingAttachmentInfo depth_info =
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO
    };
    if (depth_texture.IsValid())
    {
        depth_info.imageView = depth_texture->vk_image_view;
        depth_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth_info.loadOp = static_cast<VkAttachmentLoadOp>(desc.depth_target.load_op);
        depth_info.storeOp = static_cast<VkAttachmentStoreOp>(desc.depth_target.store_op);
        depth_info.clearValue.depthStencil.depth = desc.depth_target.clear_depth;
    }
    if (depth_resolve.IsValid())
    {
        depth_info.resolveMode = static_cast<VkResolveModeFlagBits>(desc.depth_target.resolve_mode);
        depth_info.resolveImageView = depth_resolve->vk_image_view;
        depth_info.resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkRenderingAttachmentInfo stencil_info =
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO
    };
    if (stencil_texture.IsValid())
    {
        stencil_info.imageView = stencil_texture->vk_image_view;
        stencil_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        stencil_info.loadOp = static_cast<VkAttachmentLoadOp>(desc.stencil_target.load_op);
        stencil_info.storeOp = static_cast<VkAttachmentStoreOp>(desc.stencil_target.store_op);
        stencil_info.clearValue.depthStencil.stencil = desc.stencil_target.clear_stencil;
    }
    if (stencil_resolve.IsValid())
    {
        stencil_info.resolveMode = static_cast<VkResolveModeFlagBits>(desc.stencil_target.resolve_mode);
        stencil_info.resolveImageView = stencil_resolve->vk_image_view;
        stencil_info.resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

#if GFX_DEBUG
    num_pass_color_targets = static_cast<u32>(desc.color_targets.size());
    for (u32 i = 0; i < num_pass_color_targets; ++i)
        pass_color_formats[i] = static_cast<TextureFormat>(desc.color_targets[i].texture->texture_format);

    pass_depth_format = depth_texture.IsValid() ? static_cast<TextureFormat>(depth_texture->texture_format) : TextureFormat::None;
    pass_stencil_format = stencil_texture.IsValid() ? static_cast<TextureFormat>(stencil_texture->texture_format) : TextureFormat::None;
#endif

    const VkRenderingInfo rendering_info =
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .flags = 0,
        .renderArea =
        {
            .offset = {0, 0},
            .extent = {width, height}
        },
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = static_cast<u32>(desc.color_targets.size()),
        .pColorAttachments = !desc.color_targets.empty() ? color_info : nullptr,
        .pDepthAttachment = depth_texture.IsValid() ? &depth_info : nullptr,
        .pStencilAttachment = stencil_texture.IsValid() ? &stencil_info : nullptr,
    };
    vkCmdBeginRendering(vk_cmd_buffer, &rendering_info);

    SetScissor(
    {
        .width = width,
        .height = height,
    });

    SetViewport(
    {
        .width = static_cast<f32>(width),
        .height = static_cast<f32>(height),
    });

    SetDepthState({});
    SetStencilState({});
    SetDepthBias(0.0f, 0.0f);
    SetCullMode(CullMode::None);
    SetFrontFace(FrontFace::Clockwise);
}

void CommandBuffer::EndRenderPass()
{
    GFX_ASSERT(current_render_pass_type == RenderPassType::Graphics);
    vkCmdEndRendering(vk_cmd_buffer);

    if (timed_pass)
    {
        EndPassTiming();
        EndDebugLabel();
        timed_pass = false;
    }

    current_render_pass_type = RenderPassType::None;
}

void CommandBuffer::BeginComputePass(const ComputePassDesc& desc)
{
    GFX_ASSERT(current_render_pass_type == RenderPassType::None);
    current_render_pass_type = RenderPassType::Compute;

    for (const PassResource& input : desc.inputs)
        RequireInput(input, desc.stages);

    for (const PassResource& output : desc.outputs)
        RequireOutput(output, desc.stages);

    FlushBarriers();

    if (desc.debug_name)
    {
        BeginPassTiming(desc.debug_name);
        BeginDebugLabel(desc.debug_name, desc.debug_color);
        timed_pass = true;
    }
}

void CommandBuffer::EndComputePass()
{
    GFX_ASSERT(current_render_pass_type == RenderPassType::Compute);
    current_render_pass_type = RenderPassType::None;

    if (timed_pass)
    {
        EndPassTiming();
        EndDebugLabel();
        timed_pass = false;
    }
}

void CommandBuffer::SetViewport(const Viewport& viewport) const
{
    const VkViewport vk_viewport =
    {
        .x = viewport.x,
        .y = viewport.y,
        .width = viewport.width,
        .height = viewport.height,
        .minDepth = viewport.min_depth,
        .maxDepth = viewport.max_depth,
    };
    vkCmdSetViewport(vk_cmd_buffer, 0, 1, &vk_viewport);
}

void CommandBuffer::SetScissor(const Scissor& scissor) const
{
    const VkRect2D rect =
    {
        .offset = {scissor.x, scissor.y},
        .extent = {scissor.width, scissor.height},
    };
    vkCmdSetScissor(vk_cmd_buffer, 0, 1, &rect);
}

void CommandBuffer::SetDepthState(const DepthState& state) const
{
    vkCmdSetDepthTestEnable(vk_cmd_buffer, state.test);
    vkCmdSetDepthWriteEnable(vk_cmd_buffer, state.write);
    vkCmdSetDepthCompareOp(vk_cmd_buffer, static_cast<VkCompareOp>(state.compare));
}

void CommandBuffer::SetStencilState(const StencilState& state) const
{
    vkCmdSetStencilTestEnable(vk_cmd_buffer, state.enable);
    vkCmdSetStencilOp(vk_cmd_buffer, VK_STENCIL_FACE_FRONT_BIT, static_cast<VkStencilOp>(state.front.fail), static_cast<VkStencilOp>(state.front.pass), static_cast<VkStencilOp>(state.front.depth_fail),
                      static_cast<VkCompareOp>(state.front.compare));
    vkCmdSetStencilOp(vk_cmd_buffer, VK_STENCIL_FACE_BACK_BIT, static_cast<VkStencilOp>(state.back.fail), static_cast<VkStencilOp>(state.back.pass), static_cast<VkStencilOp>(state.back.depth_fail),
                      static_cast<VkCompareOp>(state.back.compare));
    vkCmdSetStencilCompareMask(vk_cmd_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, state.read_mask);
    vkCmdSetStencilWriteMask(vk_cmd_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, state.write_mask);
    vkCmdSetStencilReference(vk_cmd_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, state.reference);
}

void CommandBuffer::SetDepthBias(f32 constant, f32 slope, f32 clamp) const
{
    vkCmdSetDepthBiasEnable(vk_cmd_buffer, constant != 0.0f || slope != 0.0f);
    vkCmdSetDepthBias(vk_cmd_buffer, constant, clamp, slope);
}

void CommandBuffer::SetCullMode(CullMode cull_mode) const
{
    vkCmdSetCullMode(vk_cmd_buffer, static_cast<VkCullModeFlags>(cull_mode));
}

void CommandBuffer::SetFrontFace(FrontFace front_face) const
{
    vkCmdSetFrontFace(vk_cmd_buffer, static_cast<VkFrontFace>(front_face));
}

void CommandBuffer::TransitionTexture(Handle<Texture> texture, TextureState state, PipelineStage stages) const
{
    GFX_ASSERT(current_render_pass_type != RenderPassType::Graphics);
    RequireTexture(texture, state, stages);
}

void CommandBuffer::Barrier(PipelineStage src_stages, StageAccess src_access, PipelineStage dst_stages, StageAccess dst_access) const
{
    FlushBarriers();

    const VkMemoryBarrier2 memory_barrier =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = static_cast<VkPipelineStageFlags2>(src_stages),
        .srcAccessMask = static_cast<VkAccessFlags2>(src_access),
        .dstStageMask = static_cast<VkPipelineStageFlags2>(dst_stages),
        .dstAccessMask = static_cast<VkAccessFlags2>(dst_access),
    };

    const VkDependencyInfo dependency_info =
    {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = 0,
        .memoryBarrierCount = 1,
        .pMemoryBarriers = &memory_barrier,
    };

    vkCmdPipelineBarrier2(vk_cmd_buffer, &dependency_info);
}

void CommandBuffer::Dispatch(Vec3u threads) const
{
    GFX_ASSERT(IsAlive(current_pipeline) && "No compute pipeline bound");
    const Pipeline* pipeline = &current_pipeline.Get();

    GFX_ASSERT(pipeline->local_group_size.x && pipeline->local_group_size.y && pipeline->local_group_size.z && "Unknown local group size, define GFX_USE_SPIRV_REFLECT");
    FlushBarriers();

    const Vec3u& local_size = pipeline->local_group_size;
    const Vec3u size = {(threads.x + local_size.x - 1) / local_size.x, (threads.y + local_size.y - 1) / local_size.y, (threads.z + local_size.z - 1) / local_size.z};
    vkCmdDispatch(vk_cmd_buffer, size.x, size.y, size.z);
}

void CommandBuffer::DispatchGroups(u32 x, u32 y, u32 z) const
{
    FlushBarriers();
    vkCmdDispatch(vk_cmd_buffer, x, y, z);
}

void CommandBuffer::DispatchIndirect(Handle<Buffer> buffer, u64 offset) const
{
    GFX_ASSERT(offset % 4 == 0);
    FlushBarriers();
    vkCmdDispatchIndirect(vk_cmd_buffer, buffer->vk_buffer, offset);
}

void CommandBuffer::BuildAccelerationStructures(Span<const Handle<AccelerationStructure>> handles) const
{
    for (const Handle<AccelerationStructure> handle : handles)
    {
        const AccelerationStructure& structure = handle.Get();
        GFX_ASSERT(structure.level == AccelerationLevel::Bottom);

        const AccelerationStructureGeometry& input = structure.geometry;
        RequireBuffer(input.vertex_buffer, PipelineStage::AccelerationStructureBuild, StageAccess::ShaderRead);
        if (input.index_buffer.IsValid())
            RequireBuffer(input.index_buffer, PipelineStage::AccelerationStructureBuild, StageAccess::ShaderRead);
        if (input.transform_buffer.IsValid())
            RequireBuffer(input.transform_buffer, PipelineStage::AccelerationStructureBuild, StageAccess::ShaderRead);
    }

    BuildAccelerationStructures(handles, false);
}

void CommandBuffer::BuildAccelerationStructure(Handle<AccelerationStructure> handle, Handle<Buffer> instances, u64 instances_offset, u32 num_instances, bool update) const
{
    AccelerationStructure& structure = handle.Get();
    GFX_ASSERT(structure.level == AccelerationLevel::Top && num_instances <= structure.max_instances);

    VkDeviceAddress instances_address = 0;
    if (instances.IsValid())
    {
        RequireBuffer(instances, PipelineStage::AccelerationStructureBuild, StageAccess::ShaderRead);
        instances_address = instances->vk_device_address + instances_offset;
    }
    GFX_ASSERT(instances_address % ACCELERATION_STRUCTURE_INPUT_ALIGNMENT == 0 && "instances must be 16 byte aligned");

    GFX_ASSERT((!update || num_instances == structure.num_primitives) && "instance count changed, rebuild");

    structure.vk_geometry.geometry.instances.data.deviceAddress = instances_address;
    structure.num_primitives = num_instances;
    BuildAccelerationStructures(Span<const Handle<AccelerationStructure>>(&handle, 1), update);
}

void CommandBuffer::TraceRays(u32 width, u32 height, u32 depth) const
{
    GFX_ASSERT(IsAlive(current_pipeline) && current_pipeline->shader_binding_table.buffer.IsValid() && "no ray tracing pipeline bound");
    const ShaderBindingTable& table = current_pipeline->shader_binding_table;

    RequireBuffer(table.buffer, PipelineStage::RayTracing, device->ray_tracing_maintenance1_supported ? StageAccess::ShaderBindingTableRead : StageAccess::ShaderRead);
    FlushBarriers();

    const VkStridedDeviceAddressRegionKHR callable = {};
    device->ray_tracing_functions.cmd_trace_rays(vk_cmd_buffer, &table.ray_gen, &table.miss, &table.hit, &callable, width, height, depth);
}

void CommandBuffer::BeginDebugLabel(const char* label, const Vec3& color)
{
    const VkDebugUtilsLabelEXT label_ext =
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pNext = nullptr,
        .pLabelName = label,
        .color = {color.x, color.y, color.z, 1.0f},
    };

    if (device->debug_utils.cmd_begin_label)
        device->debug_utils.cmd_begin_label(vk_cmd_buffer, &label_ext);
}

void CommandBuffer::EndDebugLabel()
{
    if (device->debug_utils.cmd_end_label)
        device->debug_utils.cmd_end_label(vk_cmd_buffer);
}

void CommandBuffer::InsertDebugLabel(const char* label, const Vec3& color)
{
    const VkDebugUtilsLabelEXT label_ext =
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pNext = nullptr,
        .pLabelName = label,
        .color = {color.x, color.y, color.z, 1.0f},
    };

    if (device->debug_utils.cmd_insert_label)
        device->debug_utils.cmd_insert_label(vk_cmd_buffer, &label_ext);
}

void CommandBuffer::RequireTexture(Handle<Texture> texture, TextureState state, PipelineStage stages, StageAccess access) const
{
    PendingBarriers& pending = *pending_barriers;
    for (u32 i = 0; i < pending.num_texture_barriers; ++i)
    {
        if (pending.texture_barriers[i].image == texture->vk_image)
        {
            FlushBarriers();
            break;
        }
    }

    if (pending.num_texture_barriers == pending.texture_barriers.size())
        FlushBarriers();

    if (texture->PrepareBarrier(state, pending.texture_barriers[pending.num_texture_barriers], stages, access))
        ++pending.num_texture_barriers;
}

void CommandBuffer::RequireBuffer(Handle<Buffer> buffer, PipelineStage stages, StageAccess access) const
{
    PendingBarriers& pending = *pending_barriers;
    for (u32 i = 0; i < pending.num_buffer_barriers; ++i)
    {
        if (pending.buffer_barriers[i].buffer == buffer->vk_buffer)
        {
            FlushBarriers();
            break;
        }
    }

    if (pending.num_buffer_barriers == pending.buffer_barriers.size())
        FlushBarriers();

    if (buffer->PrepareBarrier(stages, access, pending.buffer_barriers[pending.num_buffer_barriers]))
        ++pending.num_buffer_barriers;
}

void CommandBuffer::RequireAttachment(Handle<Texture> texture, TextureState state, bool discard, u32 width, u32 height, PipelineStage stages, StageAccess access) const
{
    GFX_ASSERT(texture->texture_extent.x == width && texture->texture_extent.y == height && "different renderpass target sizes");
    RequireTexture(texture, state, stages, access);
    if (discard && !texture->parent.IsValid() && texture->num_mipmaps == 1 && texture->num_layers == 1)
    {
        PendingBarriers& pending = *pending_barriers;
        GFX_ASSERT(pending.num_texture_barriers > 0);
        pending.texture_barriers[pending.num_texture_barriers - 1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

void CommandBuffer::RequireInput(const PassResource& input, PipelineStage default_stages) const
{
    GFX_ASSERT((input.access == PassAccess::Sampled || input.access == PassAccess::Read) && "Inputs must be sampled or read");
    GFX_ASSERT(input.texture.IsValid() != input.buffer.IsValid() && "Input must be texture or a buffer");
    const PipelineStage stages = input.stages != PipelineStage::None ? input.stages : default_stages;

    if (input.texture.IsValid())
    {
        RequireTexture(input.texture, input.access == PassAccess::Sampled ? TextureState::ShaderRead : TextureState::Storage, stages, StageAccess::ShaderRead);
        return;
    }

    GFX_ASSERT(input.access == PassAccess::Read && "Buffers cant be sampled");
    StageAccess access = StageAccess::None;
    if (stages & PipelineStage::Indirect)
        access |= StageAccess::IndirectRead;
    if (stages & PipelineStage::IndexInput)
        access |= StageAccess::IndexRead;
    if (stages & (PipelineStage::AllShaders | PipelineStage::RayTracing))
        access |= StageAccess::ShaderRead;
    RequireBuffer(input.buffer, stages, access);
}

void CommandBuffer::RequireOutput(const PassResource& output, PipelineStage default_stages) const
{
    GFX_ASSERT((output.access == PassAccess::Write || output.access == PassAccess::ReadWrite) && "Output must be written");
    GFX_ASSERT(output.texture.IsValid() != output.buffer.IsValid() && "Output must be texture or a buffer");
    const PipelineStage stages = output.stages != PipelineStage::None ? output.stages : default_stages;
    const StageAccess access = output.access == PassAccess::Write ? StageAccess::ShaderWrite : StageAccess::ShaderRead | StageAccess::ShaderWrite;

    if (output.texture.IsValid())
    {
        RequireTexture(output.texture, TextureState::Storage, stages, access);
        return;
    }

    RequireBuffer(output.buffer, stages, access);
}

void CommandBuffer::FlushBarriers() const
{
    PendingBarriers& pending = *pending_barriers;
    if (pending.num_texture_barriers == 0 && pending.num_buffer_barriers == 0)
        return;

    const VkDependencyInfo dependency_info =
    {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = 0,
        .bufferMemoryBarrierCount = pending.num_buffer_barriers,
        .pBufferMemoryBarriers = pending.buffer_barriers.data(),
        .imageMemoryBarrierCount = pending.num_texture_barriers,
        .pImageMemoryBarriers = pending.texture_barriers.data(),
    };
    vkCmdPipelineBarrier2(vk_cmd_buffer, &dependency_info);
    pending.num_texture_barriers = 0;
    pending.num_buffer_barriers = 0;
}

void CommandBuffer::BeginPassTiming(const char* name)
{
    if (!device->timestamps_supported)
        return;

    Device::FrameData& frame_data = device->frame_data[frame_slot];
    GFX_ASSERT(frame_data.num_timestamps + 2 <= GFX_MAX_QUERY_TIMESTAMPS && "out of timestamps");

    const u32 query = frame_data.num_timestamps;
    frame_data.num_timestamps += 2;
    frame_data.pass_names[query] = name;
    timestamp_end_index = query + 1;
    vkCmdWriteTimestamp2(vk_cmd_buffer, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, frame_data.query_pool, query);
}

void CommandBuffer::EndPassTiming()
{
    if (!device->timestamps_supported)
        return;

    vkCmdWriteTimestamp2(vk_cmd_buffer, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, device->frame_data[frame_slot].query_pool, timestamp_end_index);
}

void CommandBuffer::BuildAccelerationStructures(Span<const Handle<AccelerationStructure>> handles, bool update) const
{
    GFX_ASSERT(current_render_pass_type != RenderPassType::Graphics);

    const VkDeviceAddress scratch_address = device->accelstruct_scratch_buffer->vk_device_address;
    VkAccelerationStructureBuildGeometryInfoKHR build_infos[MAX_ACCELERATION_STRUCTURE_BATCH] = {};
    VkAccelerationStructureBuildRangeInfoKHR ranges[MAX_ACCELERATION_STRUCTURE_BATCH] = {};
    const VkAccelerationStructureBuildRangeInfoKHR* range_pointers[MAX_ACCELERATION_STRUCTURE_BATCH] = {};
    u32 count = 0;
    u64 scratch_offset = 0;

    for (const Handle<AccelerationStructure> handle : handles)
    {
        AccelerationStructure& structure = handle.Get();
        GFX_ASSERT(!update || (structure.built && (structure.build & AccelerationBuild::Updatable)));

        const u64 scratch_size = update ? structure.update_scratch_size : structure.build_scratch_size;
        if (count == MAX_ACCELERATION_STRUCTURE_BATCH || scratch_offset + scratch_size > ACCELERATION_STRUCTURE_SCRATCH_SIZE)
        {
            device->ray_tracing_functions.cmd_build_acceleration_structs(vk_cmd_buffer, count, build_infos, range_pointers);
            count = 0;
            scratch_offset = 0;
        }

        if (count == 0)
        {
            Barrier(PipelineStage::AllCommands, StageAccess::AccelerationStructureWrite, PipelineStage::AccelerationStructureBuild, StageAccess::AccelerationStructureRead | StageAccess::AccelerationStructureWrite);
            FlushBarriers();
        }

        build_infos[count] =
        {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .type = static_cast<VkAccelerationStructureTypeKHR>(structure.level),
            .flags = static_cast<VkBuildAccelerationStructureFlagsKHR>(structure.build),
            .mode = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
            .srcAccelerationStructure = update ? structure.vk_acceleration_structure : nullptr,
            .dstAccelerationStructure = structure.vk_acceleration_structure,
            .geometryCount = 1,
            .pGeometries = &structure.vk_geometry,
            .scratchData = { .deviceAddress = scratch_address + scratch_offset },
        };
        ranges[count] = { .primitiveCount = structure.num_primitives };
        range_pointers[count] = &ranges[count];
        scratch_offset += scratch_size;
        ++count;
        structure.built = true;
    }

    if (count > 0)
        device->ray_tracing_functions.cmd_build_acceleration_structs(vk_cmd_buffer, count, build_infos, range_pointers);

    Barrier(PipelineStage::AccelerationStructureBuild, StageAccess::AccelerationStructureWrite, PipelineStage::AllCommands, StageAccess::AccelerationStructureRead);
}

void CommandQueue::Initialize(Device& device, u32 family, u32 max_slots)
{
    queue_family = family;
    slot = 0;

    vkGetDeviceQueue(device.vk_device, queue_family, 0, &vk_queue);

    const VkCommandPoolCreateInfo pool_info =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue_family,
    };
    VK_CHECK(vkCreateCommandPool(device.vk_device, &pool_info, nullptr, &vk_command_pool));

    vk_cmd_buffers.resize(max_slots);
    const VkCommandBufferAllocateInfo alloc_info =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vk_command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = max_slots,
    };
    VK_CHECK(vkAllocateCommandBuffers(device.vk_device, &alloc_info, vk_cmd_buffers.data()));

    slot_timeline_values.resize(max_slots);
    pending_barriers.resize(max_slots);
    for (u32 i = 0; i < max_slots; ++i)
    {
        device.SetDebugName(vk_cmd_buffers[i], "gfx cmd");
    }
}

void CommandQueue::Shutdown(Device& device)
{
    if (vk_command_pool)
    {
        vkDestroyCommandPool(device.vk_device, vk_command_pool, nullptr);
        vk_command_pool = nullptr;
    }
    vk_cmd_buffers.clear();
    slot_timeline_values.clear();
    pending_barriers.clear();
}

CommandBuffer CommandQueue::Acquire(Device& device)
{
    GFX_ASSERT(slot_timeline_values[slot] != UINT64_MAX && "command buffer slot reused before it was submitted");

    const VkSemaphoreWaitInfo wait_info =
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .pNext = nullptr,
        .flags = 0,
        .semaphoreCount = 1,
        .pSemaphores = &device.graphics_timeline,
        .pValues = &slot_timeline_values[slot],
    };
    VK_CHECK(vkWaitSemaphores(device.vk_device, &wait_info, UINT64_MAX));
    slot_timeline_values[slot] = UINT64_MAX;

    const VkCommandBuffer cmd = vk_cmd_buffers[slot];
    VK_CHECK(vkResetCommandBuffer(cmd, 0));
    ++num_open_command_buffers;

    const VkCommandBufferBeginInfo begin =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };
    VK_CHECK(vkBeginCommandBuffer(cmd, &begin));

    CommandBuffer cmd_buf{};
    cmd_buf.device = &device;
    cmd_buf.vk_cmd_buffer = cmd;
    cmd_buf.cmd_slot_idx = slot;
    cmd_buf.pending_barriers = &pending_barriers[slot];
    pending_barriers[slot].num_texture_barriers = 0;
    pending_barriers[slot].num_buffer_barriers = 0;

    slot = (slot + 1) % static_cast<u32>(vk_cmd_buffers.size());

    return cmd_buf;
}

void CommandQueue::End(const CommandBuffer& cmd, u64 timeline_value)
{
    GFX_ASSERT(cmd.cmd_slot_idx < static_cast<u32>(vk_cmd_buffers.size()));
    GFX_ASSERT(cmd.vk_cmd_buffer == vk_cmd_buffers[cmd.cmd_slot_idx]);
    GFX_ASSERT(num_open_command_buffers > 0 && "double submit");

    cmd.FlushBarriers();
    VK_CHECK(vkEndCommandBuffer(cmd.vk_cmd_buffer));
    --num_open_command_buffers;
    slot_timeline_values[cmd.cmd_slot_idx] = timeline_value;
}

void CommandQueue::Submit(Device& device, const CommandBuffer& cmd, Span<VkSemaphoreSubmitInfo> waits, Span<VkSemaphoreSubmitInfo> signals, VkCommandBuffer upload_cmd)
{
    End(cmd, device.graphics_timeline_value);

    const VkCommandBufferSubmitInfo submit_infos[] =
    {
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .pNext = nullptr,
            .commandBuffer = upload_cmd,
            .deviceMask = 0
        },
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .pNext = nullptr,
            .commandBuffer = cmd.vk_cmd_buffer,
            .deviceMask = 0
        },
    };

    const VkSubmitInfo2 info =
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = static_cast<u32>(waits.size()),
        .pWaitSemaphoreInfos = waits.data(),
        .commandBufferInfoCount = upload_cmd ? 2u : 1u,
        .pCommandBufferInfos = upload_cmd ? &submit_infos[0] : &submit_infos[1],
        .signalSemaphoreInfoCount = static_cast<u32>(signals.size()),
        .pSignalSemaphoreInfos = signals.data(),
    };
    VK_CHECK(vkQueueSubmit2(vk_queue, 1, &info, nullptr));
}

void UploadRing::Initialize(Device& device, VkSemaphore semaphore)
{
    timeline = semaphore;
    buffer = device.CreateBuffer(
    {
        .debug_name = "upload ring",
        .buffer_size = GFX_UPLOAD_RING_SIZE,
        .buffer_usage = BufferUsage::None,
        .memory_type = MemoryType::CPU_GPU,
    });
    data = static_cast<u8*>(device.buffer_pool.Get(buffer).mapped_memory);
}

void UploadRing::Shutdown(Device& device)
{
    device.Destroy(buffer);
}

void UploadRing::WriteBuffer(Device& device, Handle<Buffer> destination, const void* source)
{
    Buffer& target = device.buffer_pool.Get(destination);
    GFX_ASSERT(target.memory_type == MemoryType::GPU && "GPU memory only");
    GFX_ASSERT(source && target.buffer_size > 0);

    const u8* bytes = static_cast<const u8*>(source);
    const u64 alignment = std::max<u64>(16, device.device_properties.limits.optimalBufferCopyOffsetAlignment);
    for (u64 written = 0; written < target.buffer_size; written += BATCH_BYTES)
    {
        const u64 chunk_size = std::min(target.buffer_size - written, BATCH_BYTES);
        const u64 ring_offset = Allocate(device, chunk_size, alignment);
        memcpy(data + ring_offset, bytes + written, chunk_size);

        copies.push_back(
        {
            .buffer = target.vk_buffer,
            .buffer_region =
            {
                .srcOffset = ring_offset,
                .dstOffset = written,
                .size = chunk_size,
            },
        });
    }

    target.stages = PipelineStage::None;
    target.access = StageAccess::None;
}

void UploadRing::WriteTexture(Device& device, Handle<Texture> destination, const void* source)
{
    GFX_ASSERT(source);

    Texture& texture = device.texture_pool.Get(destination);
    GFX_ASSERT(!texture.IsView());

    const FormatInfo format_info = GetFormatInfo(static_cast<TextureFormat>(texture.texture_format));
    GFX_ASSERT(format_info.block_bytes > 0 && "Unknown texture format");
    GFX_ASSERT(!(format_info.depth && format_info.stencil) && "depth/stencil formats are not supported");

    const TextureState final_state = (texture.texture_usage & TextureUsage::Storage) ? TextureState::Storage : TextureState::ShaderRead;
    const u8* bytes = static_cast<const u8*>(source);
    const u64 alignment = 16 % format_info.block_bytes == 0 ? 16 : format_info.block_bytes * 4;
    bool first_tile = true;
    for (u32 mip = 0; mip < texture.num_mipmaps; ++mip)
    {
        const Vec3u extent = {std::max(texture.texture_extent.x >> mip, 1u), std::max(texture.texture_extent.y >> mip, 1u), std::max(texture.texture_extent.z >> mip, 1u)};
        const u32 rows = (extent.y + format_info.block_height - 1) / format_info.block_height;
        const u64 row_bytes = static_cast<u64>((extent.x + format_info.block_width - 1) / format_info.block_width) * format_info.block_bytes;
        const u64 slice_bytes = row_bytes * rows;
        const u64 layer_bytes = slice_bytes * extent.z;
        GFX_ASSERT(row_bytes <= BATCH_BYTES && "texture row too big");

        const u32 tile_layers = static_cast<u32>(std::clamp<u64>(BATCH_BYTES / layer_bytes, 1, texture.num_layers));
        const u32 tile_slices = layer_bytes <= BATCH_BYTES ? extent.z : static_cast<u32>(std::clamp<u64>(BATCH_BYTES / slice_bytes, 1, extent.z));
        const u32 tile_rows = slice_bytes <= BATCH_BYTES ? rows : static_cast<u32>(std::clamp<u64>(BATCH_BYTES / row_bytes, 1, rows));

        for (u32 layer = 0; layer < texture.num_layers; layer += tile_layers)
        {
            for (u32 z = 0; z < extent.z; z += tile_slices)
            {
                for (u32 y = 0; y < rows; y += tile_rows)
                {
                    const u32 num_layers = std::min(tile_layers, texture.num_layers - layer);
                    const u32 num_slices = std::min(tile_slices, extent.z - z);
                    const u32 num_rows = std::min(tile_rows, rows - y);
                    const u64 copy_size = row_bytes * num_rows * num_slices * num_layers;
                    const u64 ring_offset = Allocate(device, copy_size, alignment);
                    memcpy(data + ring_offset, bytes, copy_size);
                    bytes += copy_size;

                    const u32 texel_y = y * format_info.block_height;
                    copies.push_back(
                    {
                        .image = texture.vk_image,
                        .final_state = final_state,
                        .texture_region =
                        {
                            .bufferOffset = ring_offset,
                            .bufferRowLength = 0,
                            .bufferImageHeight = 0,
                            .imageSubresource =
                            {
                                .aspectMask = GetImageAspectFlags(texture.texture_format),
                                .mipLevel = mip,
                                .baseArrayLayer = layer,
                                .layerCount = num_layers,
                            },
                            .imageOffset = {0, static_cast<i32>(texel_y), static_cast<i32>(z)},
                            .imageExtent = {extent.x, std::min(num_rows * format_info.block_height, extent.y - texel_y), num_slices},
                        },
                        .upload_start = first_tile,
                    });
                    first_tile = false;
                }
            }
        }
    }
    copies.back().upload_end = true;

    const TextureStateInfo final_info = GetTextureStateInfo(final_state);
    texture.state = final_state;
    texture.stages = final_info.stages;
    texture.access = final_info.access;
}

void UploadRing::WriteShaderBindingTable(Device& device, const Pipeline& pipeline)
{
    const ShaderBindingTable& binding_table = pipeline.shader_binding_table;
    Buffer& target = device.buffer_pool.Get(binding_table.buffer);
    GFX_ASSERT(target.memory_type == MemoryType::GPU && "GPU memory only");
    GFX_ASSERT(target.buffer_size <= BATCH_BYTES && "shader binding table too large");

    const u64 alignment = std::max<u64>(16, device.device_properties.limits.optimalBufferCopyOffsetAlignment);
    const u64 ring_offset = Allocate(device, target.buffer_size, alignment);
    const u32 handle_size = device.ray_tracing_properties.shaderGroupHandleSize;
    const VkStridedDeviceAddressRegionKHR regions[] = {binding_table.ray_gen, binding_table.miss, binding_table.hit};

    u32 group = 0;
    for (const VkStridedDeviceAddressRegionKHR& region : regions)
    {
        const u64 region_offset = ring_offset + (region.deviceAddress - target.vk_device_address);
        for (u64 offset = 0; offset < region.size; offset += region.stride)
        {
            VK_CHECK(device.ray_tracing_functions.get_shader_group_handles(device.vk_device, pipeline.vk_pipeline, group, 1, handle_size, data + region_offset + offset));
            ++group;
        }
    }

    copies.push_back(
    {
        .buffer = target.vk_buffer,
        .buffer_region =
        {
            .srcOffset = ring_offset,
            .dstOffset = 0,
            .size = target.buffer_size,
        },
    });

    target.stages = PipelineStage::None;
    target.access = StageAccess::None;
}

void UploadRing::Commit(Device& device, u64 timeline_value)
{
    if (num_batches == MAX_BATCHES)
    {
        const Batch& oldest = batches[first_batch];
        WaitForSpace(device, oldest.end);
    }

    const u32 next = (first_batch + num_batches) % MAX_BATCHES;
    batches[next] =
    {
        .end = head,
        .value = timeline_value,
    };
    ++num_batches;

    copies.clear();
    open_bytes = 0;
}

u64 UploadRing::Allocate(Device& device, u64 size, u64 alignment)
{
    GFX_ASSERT(size <= GFX_UPLOAD_RING_SIZE && "Upload copy is too big");

    if (!copies.empty() && open_bytes + size > BATCH_BYTES)
    {
        device.FlushUploads();
    }

    if (head == tail)
    {
        head = 0;
        tail = 0;
    }

    const u64 position = head % GFX_UPLOAD_RING_SIZE;
    u64 offset = AlignUpToMultiple(position, alignment);
    if (offset + size > GFX_UPLOAD_RING_SIZE)
        offset = GFX_UPLOAD_RING_SIZE;

    u64 end = head - position + offset + size;
    if (end - tail > GFX_UPLOAD_RING_SIZE)
    {
        WaitForSpace(device, end - GFX_UPLOAD_RING_SIZE);
        if (head == tail)
        {
            head = 0;
            tail = 0;
            end = size;
        }
    }
    GFX_ASSERT(end - tail <= GFX_UPLOAD_RING_SIZE);

    head = end;
    open_bytes += size;

    const u64 ring_offset = (end - size) % GFX_UPLOAD_RING_SIZE;
    return ring_offset;
}

void UploadRing::WaitForSpace(Device& device, u64 required_tail)
{
    GFX_ASSERT(num_batches > 0);

    u32 count = num_batches;
    for (u32 i = 0; i < num_batches; ++i)
    {
        const Batch& batch = batches[(first_batch + i) % MAX_BATCHES];
        if (batch.end >= required_tail)
        {
            count = i + 1;
            break;
        }
    }

    const Batch& batch = batches[(first_batch + count - 1) % MAX_BATCHES];
    u64 completed = 0;
    VK_CHECK(vkGetSemaphoreCounterValue(device.vk_device, timeline, &completed));
    if (completed < batch.value)
    {
        const VkSemaphoreWaitInfo wait_info =
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .pNext = nullptr,
            .flags = 0,
            .semaphoreCount = 1,
            .pSemaphores = &timeline,
            .pValues = &batch.value,
        };
        VK_CHECK(vkWaitSemaphores(device.vk_device, &wait_info, UINT64_MAX));
    }

    tail = batch.end;
    first_batch = (first_batch + count) % MAX_BATCHES;
    num_batches -= count;
}

void TempAllocator::Initialize(Device& device)
{
    uniform_range = std::min<u64>(device.device_properties.limits.maxUniformBufferRange, 64 * 1024);

    BufferUsage usage = BufferUsage::Uniform | BufferUsage::Storage | BufferUsage::Indirect;
    if (device.features & (Feature::RayTracing | Feature::RayQuery))
        usage |= BufferUsage::AccelerationStructureInput;

    buffer = device.CreateBuffer(
    {
        .debug_name = "temp",
        .buffer_size = GFX_TEMP_FRAME_SIZE * GFX_MAX_FRAMES_IN_FLIGHT + uniform_range,
        .buffer_usage = usage,
        .memory_type = MemoryType::CPU_GPU,
    });

    const Buffer& temp_buffer = buffer.Get();
    data = static_cast<u8*>(temp_buffer.mapped_memory);
    gpu_address = temp_buffer.vk_device_address;

    GFX_LOG("Temp memory: %llu MB x %u frames", static_cast<u64>(GFX_TEMP_FRAME_SIZE) >> 20, GFX_MAX_FRAMES_IN_FLIGHT);
}

void TempAllocator::Shutdown(Device& device)
{
    GFX_LOG("Temp memory: peak %llu KB allocated", peak >> 10);
    device.Destroy(buffer);
}

void TempAllocator::NewFrame(u32 frame)
{
    head = frame * GFX_TEMP_FRAME_SIZE;
    end = head + GFX_TEMP_FRAME_SIZE;
}

void TempAllocator::EndFrame()
{
    head = 0;
    end = 0;
}

TempAllocation TempAllocator::Allocate(u64 size, u64 alignment)
{
    GFX_ASSERT(alignment > 0);
    GFX_ASSERT(end != 0 && "Allocate between NewFrame and SubmitFrame");

    const u64 frame_start = end - GFX_TEMP_FRAME_SIZE;
    const u64 offset = AlignUpToMultiple(head, alignment);
    GFX_ASSERT(offset <= end && size <= end - offset && "Temp memory overflow");

    head = offset + size;
    peak = std::max(peak, head - frame_start);

    return
    {
        .data = data + offset,
        .gpu_address = gpu_address + offset,
        .buffer = buffer,
        .offset = offset,
    };
}

Device::Device(const DeviceConfig& config, u32 device_index) : device_index(device_index)
{
    buffer_pool.Initialize(config.max_buffers, device_index);
    texture_pool.Initialize(config.max_textures, device_index);
    pipeline_pool.Initialize(config.max_pipelines, device_index);
    shader_module_pool.Initialize(config.max_shaders, device_index);
    acceleration_structure_pool.Initialize(config.max_acceleration_structures, device_index);

    CreateInstance(config);

    if (config.window)
    {
#if defined(GFX_WINDOW_WIN32)
        const VkWin32SurfaceCreateInfoKHR surface_info =
        {
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .hinstance = GetModuleHandleW(nullptr),
            .hwnd = static_cast<HWND>(config.window),
        };
        VK_CHECK(vkCreateWin32SurfaceKHR(vk_instance, &surface_info, nullptr, &vk_surface));
#elif defined(GFX_WINDOW_GLFW)
        VK_CHECK(glfwCreateWindowSurface(vk_instance, static_cast<GLFWwindow*>(config.window), nullptr, &vk_surface));
#else
        GFX_LOG_FATAL("Unsupported window system");
#endif
    }

    swapchain_config = config.swapchain;
    Feature enabled = config.required_features;
    SetEnabledFeatures(config, enabled);
    SelectPhysicalDevice(config);

    for (u32 bits = static_cast<u32>(config.optional_features); bits; bits &= bits - 1)
    {
        const Feature feature = static_cast<Feature>(1u << std::countr_zero(bits));
        SetEnabledFeatures(config, enabled | feature);
        if (!FindUnsupportedFeature(vk_physical_device))
            enabled |= feature;
    }
    SetEnabledFeatures(config, enabled);

    CreateLogicalDevice(config);
    CreateFrameData();
    CreateAllocator();
    CreateSamplers(config);
}

void Device::Initialize()
{
    graphics_timeline = CreateTimelineSemaphore();
    graphics_timeline_value = 0;

    upload_ring.Initialize(*this, graphics_timeline);
    temp_allocator.Initialize(*this);
    bindless_resource_pool.Initialize(*this);

    if (vk_surface)
        RecreateSwapchain();

    if (features & (Feature::RayTracing | Feature::RayQuery))
    {
        accelstruct_scratch_buffer = CreateBuffer(
        {
            .debug_name = "accelstruct scratch",
            .buffer_size = ACCELERATION_STRUCTURE_SCRATCH_SIZE,
            .alignment = acceleration_structure_properties.minAccelerationStructureScratchOffsetAlignment,
            .buffer_usage = BufferUsage::Storage,
            .memory_type = MemoryType::GPU,
        });
    }
}

void Device::Shutdown()
{
    if (!vk_device)
        return;

    FlushUploads();
    vkDeviceWaitIdle(vk_device);

    for (i32 i = 0; i < GFX_MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (frame_data[i].query_pool)
        {
            vkDestroyQueryPool(vk_device, frame_data[i].query_pool, nullptr);
            frame_data[i].query_pool = nullptr;
        }
        if (frame_data[i].acquire_semaphore)
        {
            vkDestroySemaphore(vk_device, frame_data[i].acquire_semaphore, nullptr);
            frame_data[i].acquire_semaphore = nullptr;
        }
    }

    swapchain.Destroy(*this);
    Destroy(accelstruct_scratch_buffer);

    temp_allocator.Shutdown(*this);
    upload_ring.Shutdown(*this);
    graphics_queue.Shutdown(*this);
    upload_queue.Shutdown(*this);
    bindless_resource_pool.Shutdown(*this);

    for (u32 i = 0; i < GFX_MAX_FRAMES_IN_FLIGHT; ++i)
        DestroyDeletedResources(i);

    const u32 num_leaked_buffers = static_cast<u32>(buffer_pool.Size());
    const u32 num_leaked_textures = static_cast<u32>(texture_pool.Size());
    const u32 num_leaked_shaders = static_cast<u32>(shader_module_pool.Size());
    const u32 num_leaked_pipelines = static_cast<u32>(pipeline_pool.Size());
    const u32 num_leaked_acceleration_structures = static_cast<u32>(acceleration_structure_pool.Size());
    if (num_leaked_buffers || num_leaked_textures || num_leaked_shaders || num_leaked_pipelines || num_leaked_acceleration_structures)
    {
        GFX_LOG_WARNING("Leaked %u buffers, %u textures, %u shaders, %u pipelines, %u acceleration structures", num_leaked_buffers, num_leaked_textures, num_leaked_shaders, num_leaked_pipelines,
                        num_leaked_acceleration_structures);
    }

    for (const Sampler& sampler : sampler_pool)
    {
        vkDestroySampler(vk_device, sampler.vk_sampler, nullptr);
    }

    samplers.clear();
    buffer_pool.Clear();
    texture_pool.Clear();
    sampler_pool.Clear();
    pipeline_pool.Clear();
    shader_module_pool.Clear();
    acceleration_structure_pool.Clear();

#if GFX_USE_VMA
    vmaDestroyAllocator(vma_allocator);
    vma_allocator = nullptr;
#else
    gpu_allocator.Shutdown(*this);
#endif

    if (graphics_timeline)
    {
        vkDestroySemaphore(vk_device, graphics_timeline, nullptr);
        graphics_timeline = nullptr;
    }

    vkDestroyDevice(vk_device, nullptr);
    if (vk_debug_utils_messenger)
    {
        debug_utils.destroy_messenger(vk_instance, vk_debug_utils_messenger, nullptr);
        vk_debug_utils_messenger = nullptr;
    }
    if (vk_surface)
    {
        vkDestroySurfaceKHR(vk_instance, vk_surface, nullptr);
        vk_surface = nullptr;
    }
    vkDestroyInstance(vk_instance, nullptr);
    vk_device = nullptr;
}

Handle<ShaderModule> Device::CreateShader(const ShaderDesc& desc)
{
    GFX_ASSERT(!desc.byte_code.empty() && desc.byte_code.size() % 4 == 0 && "invalid spirv bytecode");
    const char* entry_name = desc.entry_name ? desc.entry_name : "main";
    GFX_ASSERT(strlen(entry_name) < sizeof(ShaderModule::entry_name) && "entry name too long");

    const VkShaderModuleCreateInfo shader_info =
    {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = static_cast<size_t>(desc.byte_code.size()),
        .pCode = reinterpret_cast<const u32*>(desc.byte_code.data()),
    };

    VkShaderModule vk_module = nullptr;
    const VkResult result = vkCreateShaderModule(vk_device, &shader_info, nullptr, &vk_module);
    if (result != VK_SUCCESS)
    {
        GFX_LOG_ERROR("Failed to create Vulkan shader '%s' (%s)", desc.debug_name, string_VkResult(result));
        return {};
    }

#if GFX_USE_SPIRV_REFLECT
    SpvReflectShaderModule reflection = {};
    const SpvReflectResult reflect_result = spvReflectCreateShaderModule(desc.byte_code.size(), desc.byte_code.data(), &reflection);
    if (reflect_result != SPV_REFLECT_RESULT_SUCCESS)
    {
        GFX_LOG_ERROR("Failed to reflect spirv shader '%s' (%d)", desc.debug_name, reflect_result);
        vkDestroyShaderModule(vk_device, vk_module, nullptr);
        return {};
    }

    const SpvReflectEntryPoint* entry_point = spvReflectGetEntryPoint(&reflection, entry_name);
    GFX_ASSERT(entry_point && "no entry point");
    GFX_ASSERT(static_cast<VkShaderStageFlagBits>(entry_point->shader_stage) == static_cast<VkShaderStageFlagBits>(desc.stage) && "invalid shader stage");
#endif

    Handle<ShaderModule> handle = shader_module_pool.Alloc();
    ShaderModule& shader = handle.Get();
    shader.vk_shader_module = vk_module;
    shader.stage = desc.stage;
    strcpy_s(shader.entry_name, entry_name);

    if (desc.debug_name)
    {
        SetDebugName(shader.vk_shader_module, desc.debug_name);
    }

#if GFX_USE_SPIRV_REFLECT
    shader.shader_meta_data.local_group_size = {entry_point->local_size.x, entry_point->local_size.y, entry_point->local_size.z};
    shader.reflection = reflection;
#endif

    return handle;
}

Handle<Pipeline> Device::CreatePipeline(const PipelineDesc& desc)
{
    Handle<Pipeline> handle = pipeline_pool.Alloc();
    Pipeline& pipeline = handle.Get();
    pipeline.vk_pipeline_layout = bindless_resource_pool.pipeline_layout;

#if GFX_USE_SPIRV_REFLECT
    const Handle<ShaderModule> stage_shaders[] = {desc.VS, desc.FS, desc.GS, desc.TS, desc.MS};
    for (const Handle<ShaderModule> shader : stage_shaders)
    {
        if (shader.IsValid())
            ReflectPipelineStage(pipeline, shader.Get());
    }
#endif

    VkPipelineVertexInputStateCreateInfo vertex_input_state =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = static_cast<VkPrimitiveTopology>(desc.topology),
        .primitiveRestartEnable = false,
    };

    VkPipelineRasterizationStateCreateInfo rasterization_state =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = false,
        .rasterizerDiscardEnable = false,
        .polygonMode = desc.wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = false,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1
    };

    VkPipelineViewportStateCreateInfo pipeline_viewport_state =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = nullptr,
        .scissorCount = 1,
        .pScissors = nullptr,
    };

    const u32 num_samples = desc.num_samples == 0 ? 1u : desc.num_samples;
    const VkSampleCountFlags supported_samples = device_properties.limits.framebufferColorSampleCounts & device_properties.limits.framebufferDepthSampleCounts;
    GFX_ASSERT((supported_samples & num_samples));

    VkPipelineMultisampleStateCreateInfo multisample_state =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = static_cast<VkSampleCountFlagBits>(num_samples),
        .sampleShadingEnable = false,
        .minSampleShading = 0,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = false,
        .alphaToOneEnable = false,
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthTestEnable = false,
        .depthWriteEnable = false,
        .depthCompareOp = VK_COMPARE_OP_ALWAYS,
        .depthBoundsTestEnable = false,
        .stencilTestEnable = false,
        .front = {},
        .back = {},
        .minDepthBounds = 0,
        .maxDepthBounds = 1,
    };

    GFX_ASSERT(desc.num_color_targets <= GFX_MAX_COLOR_TARGETS);
    const u32 num_color_targets = desc.num_color_targets;
    VkFormat color_formats[GFX_MAX_COLOR_TARGETS] = {};
    VkPipelineColorBlendAttachmentState blend_attachments[GFX_MAX_COLOR_TARGETS] = {};
    for (u32 i = 0; i < num_color_targets; i++)
    {
        const ColorTargetDesc& attachment = desc.color_targets[i];
        color_formats[i] = static_cast<VkFormat>(attachment.format);
        blend_attachments[i] =
        {
            .blendEnable = attachment.blend_state.enable,
            .srcColorBlendFactor = static_cast<VkBlendFactor>(attachment.blend_state.color.src_factor),
            .dstColorBlendFactor = static_cast<VkBlendFactor>(attachment.blend_state.color.dst_factor),
            .colorBlendOp = static_cast<VkBlendOp>(attachment.blend_state.color.op),
            .srcAlphaBlendFactor = static_cast<VkBlendFactor>(attachment.blend_state.alpha.src_factor),
            .dstAlphaBlendFactor = static_cast<VkBlendFactor>(attachment.blend_state.alpha.dst_factor),
            .alphaBlendOp = static_cast<VkBlendOp>(attachment.blend_state.alpha.op),
            .colorWriteMask = static_cast<VkColorComponentFlags>(attachment.write_mask),
        };
    }

    VkPipelineColorBlendStateCreateInfo color_blend_state =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = false,
        .logicOp = VK_LOGIC_OP_CLEAR,
        .attachmentCount = num_color_targets,
        .pAttachments = blend_attachments,
    };

    const VkDynamicState dynamic_states[] =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
        VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE,
        VK_DYNAMIC_STATE_DEPTH_COMPARE_OP,
        VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE,
        VK_DYNAMIC_STATE_STENCIL_OP,
        VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
        VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,
        VK_DYNAMIC_STATE_STENCIL_REFERENCE,
        VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE,
        VK_DYNAMIC_STATE_DEPTH_BIAS,
        VK_DYNAMIC_STATE_CULL_MODE,
        VK_DYNAMIC_STATE_FRONT_FACE,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = static_cast<u32>(std::size(dynamic_states)),
        .pDynamicStates = dynamic_states,
    };

    VkPipelineRenderingCreateInfo render_info =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = nullptr,
        .viewMask = 0,
        .colorAttachmentCount = num_color_targets,
        .pColorAttachmentFormats = color_formats,
        .depthAttachmentFormat = static_cast<VkFormat>(desc.depth_format),
        .stencilAttachmentFormat = static_cast<VkFormat>(desc.stencil_format),
    };

#if GFX_DEBUG
    pipeline.num_color_targets = num_color_targets;
    for (u32 i = 0; i < num_color_targets; ++i)
        pipeline.color_formats[i] = desc.color_targets[i].format;

    pipeline.depth_format = desc.depth_format;
    pipeline.stencil_format = desc.stencil_format;
#endif

    u32 num_stage = 0;
    Handle<ShaderModule> shaders[] = {desc.VS, desc.FS, desc.CS, desc.GS, desc.TS, desc.MS};
    VkPipelineShaderStageCreateInfo stages[8] = {};

    for (const Handle<ShaderModule> shader : shaders)
    {
        if (!shader.IsValid())
            continue;

        stages[num_stage++] =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = static_cast<VkShaderStageFlagBits>(shader->stage),
            .module = shader->vk_shader_module,
            .pName = shader->entry_name,
            .pSpecializationInfo = nullptr,
        };
    }

    VkGraphicsPipelineCreateInfo pipeline_info =
    {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &render_info,
        .flags = 0,
        .stageCount = num_stage,
        .pStages = stages,
        .pVertexInputState = &vertex_input_state,
        .pInputAssemblyState = &input_assembly_state,
        .pTessellationState = nullptr,
        .pViewportState = &pipeline_viewport_state,
        .pRasterizationState = &rasterization_state,
        .pMultisampleState = &multisample_state,
        .pDepthStencilState = &depth_stencil_state,
        .pColorBlendState = &color_blend_state,
        .pDynamicState = &dynamic_state,
        .layout = bindless_resource_pool.pipeline_layout,
        .renderPass = nullptr,
        .subpass = 0,
        .basePipelineHandle = nullptr,
        .basePipelineIndex = 0,
    };

    VK_CHECK(vkCreateGraphicsPipelines(vk_device, nullptr, 1, &pipeline_info, nullptr, &pipeline.vk_pipeline));

    if (desc.debug_name)
    {
        SetDebugName(pipeline.vk_pipeline, desc.debug_name);
    }

    return handle;
}

Handle<Pipeline> Device::CreateComputePipeline(const PipelineDesc& desc)
{
    Handle<Pipeline> handle = pipeline_pool.Alloc();
    Pipeline& pipeline = handle.Get();
    pipeline.vk_pipeline_layout = bindless_resource_pool.pipeline_layout;
    pipeline.local_group_size = desc.CS->shader_meta_data.local_group_size;

#if GFX_USE_SPIRV_REFLECT
    ReflectPipelineStage(pipeline, desc.CS.Get());
#endif

    const VkPipelineShaderStageCreateInfo stage =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = desc.CS->vk_shader_module,
        .pName = desc.CS->entry_name,
        .pSpecializationInfo = nullptr,
    };

    const VkComputePipelineCreateInfo create_info =
    {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = stage,
        .layout = bindless_resource_pool.pipeline_layout,
        .basePipelineHandle = nullptr,
        .basePipelineIndex = 0,
    };

    VK_CHECK(vkCreateComputePipelines(vk_device, nullptr, 1, &create_info, nullptr, &pipeline.vk_pipeline));

    if (desc.debug_name)
    {
        SetDebugName(pipeline.vk_pipeline, desc.debug_name);
    }

    return handle;
}

Handle<Pipeline> Device::CreateRayTracingPipeline(const RayTracingPipelineDesc& desc)
{
    GFX_ASSERT(ray_tracing_functions.create_pipelines && "Raytracing is not enabled");
    GFX_ASSERT(desc.max_recursion <= ray_tracing_properties.maxRayRecursionDepth);
    GFX_ASSERT(desc.ray_miss.size() <= MAX_RAY_MISS_SHADERS && "too many miss shaders");
    GFX_ASSERT(desc.hit_groups.size() <= MAX_HIT_GROUPS && "too many hit groups");

    Handle<Pipeline> handle = pipeline_pool.Alloc();
    Pipeline& pipeline = handle.Get();
    pipeline.vk_pipeline_layout = bindless_resource_pool.pipeline_layout;

    const u64 num_miss = desc.ray_miss.size();
    const u64 num_hit = desc.hit_groups.size();

    constexpr u32 MAX_GROUPS = 1 + MAX_RAY_MISS_SHADERS + MAX_HIT_GROUPS;
    constexpr u32 MAX_SHADERS = 1 + MAX_RAY_MISS_SHADERS + 3 * MAX_HIT_GROUPS;
    Handle<ShaderModule> shaders[MAX_SHADERS];
    VkRayTracingShaderGroupCreateInfoKHR groups[MAX_GROUPS];
    u32 num_shaders = 0;
    u32 num_groups = 0;

    for (u64 i = 0; i <= num_miss; ++i)
    {
        groups[num_groups++] =
        {
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
            .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
            .generalShader = num_shaders,
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR,
        };
        shaders[num_shaders++] = i == 0 ? desc.ray_gen : desc.ray_miss[i - 1];
    }

    for (const HitGroup& hit_group : desc.hit_groups)
    {
        VkRayTracingShaderGroupCreateInfoKHR& group = groups[num_groups++];
        group =
        {
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
            .type = hit_group.intersection.IsValid() ? VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR : VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
            .generalShader = VK_SHADER_UNUSED_KHR,
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR,
        };
        if (hit_group.closest_hit.IsValid())
        {
            group.closestHitShader = num_shaders;
            shaders[num_shaders++] = hit_group.closest_hit;
        }
        if (hit_group.any_hit.IsValid())
        {
            group.anyHitShader = num_shaders;
            shaders[num_shaders++] = hit_group.any_hit;
        }
        if (hit_group.intersection.IsValid())
        {
            group.intersectionShader = num_shaders;
            shaders[num_shaders++] = hit_group.intersection;
        }
    }

    VkPipelineShaderStageCreateInfo stages[MAX_SHADERS];
    for (u32 i = 0; i < num_shaders; ++i)
    {
        const ShaderModule& shader = shaders[i].Get();
#if GFX_USE_SPIRV_REFLECT
        ReflectPipelineStage(pipeline, shader);
#endif
        stages[i] =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = static_cast<VkShaderStageFlagBits>(shader.stage),
            .module = shader.vk_shader_module,
            .pName = shader.entry_name,
        };
    }

    const VkRayTracingPipelineCreateInfoKHR create_info =
    {
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
        .stageCount = num_shaders,
        .pStages = stages,
        .groupCount = num_groups,
        .pGroups = groups,
        .maxPipelineRayRecursionDepth = desc.max_recursion,
        .layout = bindless_resource_pool.pipeline_layout,
    };
    VK_CHECK(ray_tracing_functions.create_pipelines(vk_device, nullptr, nullptr, 1, &create_info, nullptr, &pipeline.vk_pipeline));

    const u32 handle_size = ray_tracing_properties.shaderGroupHandleSize;
    const u64 handle_stride = AlignUpToMultiple(handle_size, ray_tracing_properties.shaderGroupHandleAlignment);
    const u64 base_alignment = ray_tracing_properties.shaderGroupBaseAlignment;
    const u64 miss_offset = AlignUpToMultiple(handle_stride, base_alignment);
    const u64 hit_offset = AlignUpToMultiple(miss_offset + num_miss * handle_stride, base_alignment);
    const u64 table_size = hit_offset + num_hit * handle_stride;

    ShaderBindingTable& binding_table = pipeline.shader_binding_table;
    binding_table.buffer = CreateBuffer(
    {
        .debug_name = desc.debug_name,
        .buffer_size = table_size + base_alignment,
        .buffer_usage = BufferUsage::ShaderBindingTable,
        .memory_type = MemoryType::GPU,
    });

    const VkDeviceAddress buffer_address = binding_table.buffer->vk_device_address;
    const VkDeviceAddress address = AlignUpToMultiple(buffer_address, base_alignment);

    binding_table.ray_gen =
    {
        .deviceAddress = address,
        .stride = handle_stride,
        .size = handle_stride
    };

    if (num_miss)
    {
        binding_table.miss =
        {
            .deviceAddress = address + miss_offset,
            .stride = handle_stride,
            .size = num_miss * handle_stride
        };
    }

    if (num_hit)
    {
        binding_table.hit =
        {
            .deviceAddress = address + hit_offset,
            .stride = handle_stride,
            .size = num_hit * handle_stride
        };
    }

    upload_ring.WriteShaderBindingTable(*this, pipeline);

    if (desc.debug_name)
    {
        SetDebugName(pipeline.vk_pipeline, desc.debug_name);
    }

    return handle;
}

Handle<Buffer> Device::CreateBuffer(const BufferDesc& desc)
{
    Handle<Buffer> handle = buffer_pool.Alloc();

    Buffer& buffer = handle.Get();
    buffer.buffer_size = desc.buffer_size;
    buffer.buffer_usage = desc.buffer_usage;
    buffer.memory_type = desc.memory_type;

    // TODO: handle compute queue
    u32 queue_family_indices[] = { graphics_queue_family, transfer_queue_family };

    const bool concurrent_share = desc.sharing_mode == SharingMode::Concurrent && transfer_queue_family != graphics_queue_family;

    VkBufferCreateInfo buffer_create_info =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = desc.buffer_size,
        .usage = static_cast<VkBufferUsageFlags>(desc.buffer_usage),
        .sharingMode = concurrent_share ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = concurrent_share ? 2u : 0u,
        .pQueueFamilyIndices = concurrent_share ? queue_family_indices : nullptr,
    };

    buffer_create_info.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    const bool device_address = desc.buffer_usage & (BufferUsage::Storage | BufferUsage::AccelerationStructureInput | BufferUsage::AccelerationStructureStorage | BufferUsage::ShaderBindingTable);
    if (device_address)
    {
        buffer_create_info.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    }
    GFX_ASSERT(IsPow2(desc.alignment));
    const u64 input_alignment = (desc.buffer_usage & BufferUsage::AccelerationStructureInput) ? ACCELERATION_STRUCTURE_INPUT_ALIGNMENT : 1;
    const u64 min_alignment = std::max(desc.alignment, input_alignment);

#if GFX_USE_VMA
    VmaAllocationCreateInfo vma_alloc_info =
    {
        .requiredFlags = ToVkRequiredMemoryFlags(desc.memory_type),
        .preferredFlags = ToVkPreferredMemoryFlags(desc.memory_type),
    };
    switch (desc.memory_type)
    {
    case MemoryType::CPU_GPU:
        vma_alloc_info.usage = desc.buffer_usage == BufferUsage::None ? VMA_MEMORY_USAGE_AUTO_PREFER_HOST : VMA_MEMORY_USAGE_AUTO;
        vma_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        break;
    case MemoryType::GPU_CPU:
        vma_alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        vma_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        break;
    case MemoryType::GPU:
        vma_alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        break;
    }

    VmaAllocationInfo alloc_info = {};
    VK_CHECK(vmaCreateBufferWithAlignment(vma_allocator, &buffer_create_info, &vma_alloc_info, min_alignment, &buffer.vk_buffer, &buffer.vk_allocation, &alloc_info));
    buffer.mapped_memory = alloc_info.pMappedData;
#else
    VK_CHECK(vkCreateBuffer(vk_device, &buffer_create_info, nullptr, &buffer.vk_buffer));
    buffer.vk_allocation = gpu_allocator.AllocateBuffer(*this, buffer.vk_buffer, desc.memory_type, min_alignment);
    if (desc.memory_type != MemoryType::GPU)
        buffer.mapped_memory = buffer.vk_allocation.mapped_memory;
#endif

    if (device_address)
    {
        const VkBufferDeviceAddressInfo address_info =
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = buffer.vk_buffer,
        };

        buffer.vk_device_address = vkGetBufferDeviceAddress(vk_device, &address_info);
    }

    if (desc.debug_name)
    {
        SetDebugName(buffer.vk_buffer, desc.debug_name);
    }

    if (desc.data)
    {
        GFX_ASSERT(desc.memory_type != MemoryType::GPU_CPU && "not supported");
        if (desc.memory_type == MemoryType::CPU_GPU)
            memcpy(buffer.mapped_memory, desc.data, desc.buffer_size);
        else if (desc.memory_type == MemoryType::GPU)
            upload_ring.WriteBuffer(*this, handle, desc.data);
    }

    return handle;
}

Handle<Texture> Device::CreateTexture(const TextureDesc& desc)
{
    GFX_ASSERT(desc.texture_usage != TextureUsage::None && "Invalid TextureUsage");
    GFX_ASSERT(desc.memory_type == MemoryType::GPU && "Textures must use MemoryType::GPU");
    GFX_ASSERT(IsFormatSupported(desc.texture_format, desc.texture_usage) && "Not supported texture format");
    GFX_ASSERT(desc.texture_extent.x > 0 && desc.texture_extent.y > 0 && desc.texture_extent.z > 0 && "Invalid size");

    u32 num_layers = desc.num_layers;
    if (desc.texture_type == TextureType::TextureCube && num_layers <= 1)
    {
        num_layers = 6;
    }

    u32 num_mipmaps = desc.num_mipmaps;
    if (num_mipmaps == 0)
    {
        const u32 max_dim = std::max({desc.texture_extent.x, desc.texture_extent.y, desc.texture_extent.z});
        num_mipmaps = std::max(1u, static_cast<u32>(std::bit_width(max_dim)));
    }
    GFX_ASSERT(num_mipmaps <= MAX_MIP_LEVELS && "too many mips");

    const u32 num_samples = desc.num_samples == 0 ? 1u : desc.num_samples;
    const VkSampleCountFlags supported_samples = device_properties.limits.framebufferColorSampleCounts & device_properties.limits.framebufferDepthSampleCounts;
    GFX_ASSERT((supported_samples & num_samples));
    GFX_ASSERT(num_samples == 1 || (num_mipmaps == 1 && !desc.data));
    GFX_ASSERT(num_layers == 1 || desc.texture_type == TextureType::Texture2DArray || desc.texture_type == TextureType::TextureCube || desc.texture_type == TextureType::TextureCubeArray);
    GFX_ASSERT((desc.texture_type != TextureType::TextureCube && desc.texture_type != TextureType::TextureCubeArray) || num_layers % 6 == 0);

    Handle<Texture> handle = texture_pool.Alloc();

    Texture& texture = handle.Get();
    texture.texture_extent = desc.texture_extent;
    texture.texture_format = static_cast<VkFormat>(desc.texture_format);
    texture.texture_usage = desc.texture_usage;
    texture.owner_queue_family = graphics_queue_family;

    texture.vk_texture_usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (desc.texture_usage & TextureUsage::Sample)
    {
        texture.vk_texture_usage_flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (desc.texture_usage & TextureUsage::Storage)
    {
        texture.vk_texture_usage_flags |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if (desc.texture_usage & TextureUsage::Attachment)
    {
        const FormatInfo format_info = GetFormatInfo(desc.texture_format);
        if (format_info.depth || format_info.stencil)
            texture.vk_texture_usage_flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        else
            texture.vk_texture_usage_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (desc.texture_usage & TextureUsage::InputAttachment)
    {
        texture.vk_texture_usage_flags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    }

    texture.vk_view_type = static_cast<VkImageViewType>(desc.texture_type);
    texture.num_mipmaps = num_mipmaps;
    texture.num_layers = num_layers;
    texture.num_samples = num_samples;

    const VkImageCreateInfo image_info =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = desc.texture_type == TextureType::TextureCube || desc.texture_type == TextureType::TextureCubeArray ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0u,
        .imageType = ToVkImageType(desc.texture_type),
        .format = static_cast<VkFormat>(desc.texture_format),
        .extent = VkExtent3D(desc.texture_extent.x, desc.texture_extent.y, desc.texture_extent.z),
        .mipLevels = num_mipmaps,
        .arrayLayers = num_layers,
        .samples = static_cast<VkSampleCountFlagBits>(num_samples),
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = texture.vk_texture_usage_flags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

#if GFX_USE_VMA
    const VmaAllocationCreateInfo create_info =
    {
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
    };

    VmaAllocationInfo alloc_info{};
    VK_CHECK(vmaCreateImage(vma_allocator, &image_info, &create_info, &texture.vk_image, &texture.vk_allocation, &alloc_info));
    texture.allocated_size = alloc_info.size;
#else
    VK_CHECK(vkCreateImage(vk_device, &image_info, nullptr, &texture.vk_image));
    texture.vk_allocation = gpu_allocator.AllocateTexture(*this, texture.vk_image, desc.memory_type);
    texture.allocated_size = texture.vk_allocation.size;
#endif

    texture.vk_image_view = CreateImageView(texture);

    texture.sampler = desc.sampler.IsValid() ? desc.sampler : GetSampler(DefaultSampler::LinearClamp);

    texture.bindless = (desc.texture_usage & (TextureUsage::Sample | TextureUsage::Storage)) && !(texture.IsDepthTexture() && texture.IsStencilTexture());
    if (texture.bindless)
    {
        bindless_resource_pool.StoreTexture(*this, handle);
    }

    if (desc.data)
    {
        upload_ring.WriteTexture(*this, handle, desc.data);
    }

    if (desc.debug_name)
    {
        SetDebugName(texture.vk_image, desc.debug_name);
    }

    return handle;
}

Handle<Texture> Device::CreateTextureView(const TextureViewDesc& desc)
{
    const Texture* source = texture_pool.GetPtr(desc.texture);
    GFX_ASSERT(source);
    const Texture& src = *source;
    GFX_ASSERT(desc.num_mipmaps > 0 && desc.start_mip_level + desc.num_mipmaps <= src.num_mipmaps && "view mips out of range");
    GFX_ASSERT(desc.num_layers > 0 && desc.start_array_layer + desc.num_layers <= src.num_layers && "view layers out of range");

    VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    if (desc.num_layers == src.num_layers)
        view_type = src.vk_view_type;
    else if (desc.num_layers == 1)
        view_type = VK_IMAGE_VIEW_TYPE_2D;
    else if ((src.vk_view_type == VK_IMAGE_VIEW_TYPE_CUBE || src.vk_view_type == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) && desc.num_layers % 6 == 0)
        view_type = desc.num_layers == 6 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;

    Handle<Texture> handle = texture_pool.Alloc();
    Texture& texture_view = handle.Get();

    texture_view.vk_image = src.vk_image;
    texture_view.vk_view_type = view_type;
    texture_view.texture_extent = {std::max(src.texture_extent.x >> desc.start_mip_level, 1u), std::max(src.texture_extent.y >> desc.start_mip_level, 1u), std::max(src.texture_extent.z >> desc.start_mip_level, 1u)};
    texture_view.texture_format = src.texture_format;
    texture_view.vk_texture_usage_flags = src.vk_texture_usage_flags;
    texture_view.texture_usage = src.texture_usage;
    texture_view.bindless = (src.texture_usage & (TextureUsage::Sample | TextureUsage::Storage)) && (!(src.IsDepthTexture() && src.IsStencilTexture()) || desc.aspect == TextureAspect::Depth || desc.aspect == TextureAspect::Stencil);
    texture_view.owner_queue_family = src.owner_queue_family;
    texture_view.num_mipmaps = desc.num_mipmaps;
    texture_view.num_layers = desc.num_layers;
    texture_view.num_samples = src.num_samples;
    texture_view.base_mip_level = src.base_mip_level + desc.start_mip_level;
    texture_view.base_array_layer = src.base_array_layer + desc.start_array_layer;
    texture_view.sampler = src.sampler;
    texture_view.parent = src.parent.IsValid() ? src.parent : desc.texture;

    texture_view.vk_image_view = CreateImageView(texture_view, desc.aspect);

    if (texture_view.bindless)
    {
        bindless_resource_pool.StoreTexture(*this, handle);
    }

    if (desc.debug_name)
    {
        SetDebugName(texture_view.vk_image_view, desc.debug_name);
    }

    return handle;
}

Handle<AccelerationStructure> Device::CreateAccelerationStructure(const AccelerationStructureDesc& desc)
{
    GFX_ASSERT(ray_tracing_functions.create_acceleration_structure && "RayTracing or RayQuery is not enabled");
    const bool top_level = desc.level == AccelerationLevel::Top;

    Handle<AccelerationStructure> handle = acceleration_structure_pool.Alloc();
    AccelerationStructure& structure = handle.Get();
    structure.level = desc.level;
    structure.build = desc.build;

    if (top_level)
    {
        GFX_ASSERT(desc.max_instances > 0);
        structure.max_instances = desc.max_instances;
        structure.vk_geometry =
        {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
            .geometry = { .instances = { .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR } },
        };
    }
    else
    {
        structure.geometry = desc.geometry;
        structure.vk_geometry = CreateAccelStructGeometry(desc.geometry);
        structure.num_primitives = desc.geometry.num_triangles;
    }

    const VkAccelerationStructureBuildGeometryInfoKHR size_query =
    {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = static_cast<VkAccelerationStructureTypeKHR>(desc.level),
        .flags = static_cast<VkBuildAccelerationStructureFlagsKHR>(desc.build),
        .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .geometryCount = 1,
        .pGeometries = &structure.vk_geometry,
    };
    
    const u32 max_primitives = top_level ? desc.max_instances : desc.geometry.num_triangles;
    VkAccelerationStructureBuildSizesInfoKHR sizes = { .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
    ray_tracing_functions.get_build_sizes(vk_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &size_query, &max_primitives, &sizes);

    const u64 scratch_alignment = acceleration_structure_properties.minAccelerationStructureScratchOffsetAlignment;
    structure.build_scratch_size = AlignUpToMultiple(sizes.buildScratchSize, scratch_alignment);
    structure.update_scratch_size = AlignUpToMultiple(sizes.updateScratchSize, scratch_alignment);
    GFX_ASSERT(structure.build_scratch_size <= ACCELERATION_STRUCTURE_SCRATCH_SIZE && "build scratch too large");
    GFX_ASSERT(structure.update_scratch_size <= ACCELERATION_STRUCTURE_SCRATCH_SIZE && "update scratch too large");

    structure.buffer = CreateBuffer(
    {
        .debug_name = desc.debug_name,
        .buffer_size = sizes.accelerationStructureSize,
        .buffer_usage = BufferUsage::AccelerationStructureStorage,
        .memory_type = MemoryType::GPU,
    });

    const VkAccelerationStructureCreateInfoKHR create_info =
    {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .buffer = structure.buffer->vk_buffer,
        .size = sizes.accelerationStructureSize,
        .type = static_cast<VkAccelerationStructureTypeKHR>(desc.level),
    };
    VK_CHECK(ray_tracing_functions.create_acceleration_structure(vk_device, &create_info, nullptr, &structure.vk_acceleration_structure));

    const VkAccelerationStructureDeviceAddressInfoKHR address_info =
    {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
        .accelerationStructure = structure.vk_acceleration_structure,
    };
    structure.device_address = ray_tracing_functions.get_device_address(vk_device, &address_info);

    if (top_level)
    {
        bindless_resource_pool.StoreAccelerationStructure(*this, handle);
    }

    if (desc.debug_name)
    {
        SetDebugName(structure.vk_acceleration_structure, desc.debug_name);
    }

    return handle;
}

void Device::Destroy(Handle<Buffer> handle)
{
    const Buffer* buffer = buffer_pool.GetPtr(handle);
    if (buffer == nullptr)
    {
        return;
    }

    DeferDeletion(
    {
        .type = DeletionType::Buffer,
        .index = handle.Index(),
        .allocation = buffer->vk_allocation,
        .buffer = buffer->vk_buffer,
    });

    buffer_pool.Free(handle);
}

void Device::Destroy(Handle<ShaderModule> handle)
{
    ShaderModule* shader_module = shader_module_pool.GetPtr(handle);
    if (shader_module == nullptr)
    {
        return;
    }

#if GFX_USE_SPIRV_REFLECT
    spvReflectDestroyShaderModule(&shader_module->reflection);
#endif

    DeferDeletion(
    {
        .type = DeletionType::ShaderModule,
        .index = handle.Index(),
        .shader_module = shader_module->vk_shader_module,
    });
    shader_module_pool.Free(handle);
}

void Device::Destroy(Handle<Pipeline> handle)
{
    Pipeline* pipeline = pipeline_pool.GetPtr(handle);
    if (pipeline == nullptr)
    {
        return;
    }

#if GFX_USE_SPIRV_REFLECT
    for (SpvReflectShaderModule& module : pipeline->reflections)
        spvReflectDestroyShaderModule(&module);
#endif

    DeferDeletion(
    {
        .type = DeletionType::Pipeline,
        .index = handle.Index(),
        .pipeline = pipeline->vk_pipeline,
    });
    Destroy(pipeline->shader_binding_table.buffer);
    pipeline_pool.Free(handle);
}

void Device::Destroy(Handle<Texture> handle)
{
    const Texture* texture = texture_pool.GetPtr(handle);
    if (texture == nullptr)
    {
        return;
    }

    const bool view_only = texture->IsView();
    DeferDeletion(
    {
        .type = DeletionType::Texture,
        .index = handle.Index(),
        .bindless = texture->bindless,
        .allocation = view_only ? GpuAllocation{} : texture->vk_allocation,
        .image = view_only ? nullptr : texture->vk_image,
        .image_view = texture->vk_image_view,
    });

    texture_pool.Free(handle);
}

void Device::Destroy(Handle<AccelerationStructure> handle)
{
    const AccelerationStructure* structure = acceleration_structure_pool.GetPtr(handle);
    if (structure == nullptr)
    {
        return;
    }

    DeferDeletion(
    {
        .type = DeletionType::AccelerationStructure,
        .index = handle.Index(),
        .bindless = structure->level == AccelerationLevel::Top,
        .acceleration_structure = structure->vk_acceleration_structure,
    });
    Destroy(structure->buffer);

    acceleration_structure_pool.Free(handle);
}

CommandBuffer Device::AcquireCommandBuffer()
{
    CommandBuffer cmd = graphics_queue.Acquire(*this);
    cmd.device = this;
    cmd.frame_slot = last_frame_index;

    const VkDescriptorSet bindless_set = bindless_resource_pool.bindless_descriptor_set;
    vkCmdBindDescriptorSets(cmd.vk_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bindless_resource_pool.pipeline_layout, 0, 1, &bindless_set, 0, nullptr);
    vkCmdBindDescriptorSets(cmd.vk_cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, bindless_resource_pool.pipeline_layout, 0, 1, &bindless_set, 0, nullptr);
    if (features & Feature::RayTracing)
        vkCmdBindDescriptorSets(cmd.vk_cmd_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, bindless_resource_pool.pipeline_layout, 0, 1, &bindless_set, 0, nullptr);

    return cmd;
}

Frame Device::NewFrame()
{
    GFX_ASSERT(graphics_queue.num_open_command_buffers == 0 && "submit other command buffers before NewFrame");
    FlushUploads();

    const VkSemaphoreWaitInfo wait_info =
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .pNext = nullptr,
        .flags = 0,
        .semaphoreCount = 1,
        .pSemaphores = &graphics_timeline,
        .pValues = &frame_data[frame_index].last_graphics_value,
    };

    VK_CHECK(vkWaitSemaphores(vk_device, &wait_info, UINT64_MAX));

    FrameData& frame = frame_data[frame_index];
    DestroyDeletedResources(frame_index);

    if (frame.num_timestamps > 0)
    {
        VK_CHECK(vkGetQueryPoolResults(vk_device, frame.query_pool, 0, frame.num_timestamps, frame.num_timestamps * sizeof(u64), frame.timestamps.data(), sizeof(u64),
                                       VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT));

        gpu_timings.clear();
        for (u32 i = 0; i < frame.num_timestamps; i += 2)
        {
            gpu_timings.push_back(
            {
                .name = frame.pass_names[i],
                .timestamp_start = frame.timestamps[i],
                .timestamp_end = frame.timestamps[i + 1],
            });
        }

        vkResetQueryPool(vk_device, frame.query_pool, 0, frame.num_timestamps);
        frame.num_timestamps = 0;
    }

    temp_allocator.NewFrame(frame_index);
    last_frame_index = frame_index;

    swapchain_valid = vk_surface != nullptr;
    if (swapchain_valid)
    {
        VkSurfaceCapabilitiesKHR capabilities;
        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device, vk_surface, &capabilities));
        const VkExtent2D surface_extent = capabilities.currentExtent;
        const bool resized = !swapchain.vk_swapchain || surface_extent.width != swapchain.extent.x || surface_extent.height != swapchain.extent.y;
        swapchain_valid = surface_extent.width != 0 && surface_extent.height != 0;
        if (swapchain_valid && (resized || swapchain.needs_recreation))
            swapchain_valid = RecreateSwapchain();
    }

    return
    {
        .cmd = AcquireCommandBuffer(),
        .swapchain_size = swapchain_valid ? swapchain.GetSize() : Vec2u{},
        .swapchain_valid = swapchain_valid,
        .frame_index = frame_index,
        .frame_number = frame_number,
    };
}

SyncPoint Device::SubmitFrame(Frame& frame)
{
    GFX_ASSERT(graphics_queue.num_open_command_buffers == 1 && "submit all other command buffers first");
    temp_allocator.EndFrame();

    const bool present = swapchain.acquired;
    if (present)
        frame.cmd.TransitionTexture(swapchain.GetCurrentTexture(), TextureState::Present);

    VkSemaphoreSubmitInfo waits[] =
    {
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext = nullptr,
            .semaphore = frame_data[frame_index].acquire_semaphore,
            .value = 0,
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .deviceIndex = 0,
        },
    };

    VkSemaphoreSubmitInfo signals[] =
    {
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext = nullptr,
            .semaphore = present ? swapchain.GetRenderFinishedSemaphore() : VK_NULL_HANDLE,
            .value = 0,
            .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .deviceIndex = 0,
        },
    };

    const u32 num_present_semaphores = present ? 1 : 0;
    const SyncPoint point = Submit(frame.cmd, Span(waits, num_present_semaphores), Span(signals, num_present_semaphores));
    if (present)
    {
        Present();
        swapchain.acquired = false;
    }

    frame_index = (frame_index + 1) % GFX_MAX_FRAMES_IN_FLIGHT;
    ++frame_number;
    swapchain_valid = false;
    return point;
}

SyncPoint Device::Submit(const CommandBuffer& cmd, Span<VkSemaphoreSubmitInfo> extra_waits, Span<VkSemaphoreSubmitInfo> extra_signals)
{
    constexpr u32 MAX_SIGNALS = 8;
    GFX_ASSERT(extra_signals.size() < MAX_SIGNALS);

    VkSemaphoreSubmitInfo signals[MAX_SIGNALS];
    u32 num_signals = 0;
    for (const VkSemaphoreSubmitInfo& signal : extra_signals)
    {
        signals[num_signals++] = signal;
    }

    signals[num_signals++] =
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext = nullptr,
        .semaphore = graphics_timeline,
        .value = ++graphics_timeline_value,
        .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .deviceIndex = 0,
    };

    VkCommandBuffer upload_cmd = nullptr;
    if (!upload_ring.copies.empty())
    {
        const CommandBuffer recorded = RecordUploadCommands(graphics_timeline_value);
        upload_queue.End(recorded, graphics_timeline_value);
        upload_cmd = recorded.vk_cmd_buffer;
    }

    graphics_queue.Submit(*this, cmd, extra_waits, Span(signals, num_signals), upload_cmd);

    frame_data[last_frame_index].last_graphics_value = graphics_timeline_value;
    return
    {
        .semaphore = graphics_timeline,
        .value = graphics_timeline_value,
    };
}

bool Device::IsComplete(SyncPoint point) const
{
    u64 completed = 0;
    VK_CHECK(vkGetSemaphoreCounterValue(vk_device, point.semaphore, &completed));
    return completed >= point.value;
}

void Device::WaitFor(SyncPoint point) const
{
    const VkSemaphoreWaitInfo wait_info =
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .pNext = nullptr,
        .flags = 0,
        .semaphoreCount = 1,
        .pSemaphores = &point.semaphore,
        .pValues = &point.value,
    };
    VK_CHECK(vkWaitSemaphores(vk_device, &wait_info, UINT64_MAX));
}

void Device::WaitForUploads()
{
    WaitFor(
    {
        .semaphore = graphics_timeline,
        .value = FlushUploads(),
    });
}

TempAllocation Device::AllocateTemp(u64 size)
{
    return AllocateTemp(size, std::max<u64>(16, device_properties.limits.minUniformBufferOffsetAlignment));
}

TempAllocation Device::AllocateTemp(u64 size, u64 alignment)
{
    return temp_allocator.Allocate(size, alignment);
}

Handle<Sampler> Device::GetSampler(DefaultSampler sampler) const
{
    return samplers[static_cast<u32>(sampler)];
}

Handle<Sampler> Device::GetSampler(u32 config_index) const
{
    const u32 index = static_cast<u32>(DefaultSampler::Count) + config_index;
    GFX_ASSERT(index < samplers.size());
    return samplers[index];
}

bool Device::IsFormatSupported(TextureFormat format, TextureUsage usage) const
{
    const FormatInfo info = GetFormatInfo(format);

    VkFormatFeatureFlags required = 0;
    if (usage & TextureUsage::Sample)
        required |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
    if (usage & TextureUsage::Storage)
        required |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
    if (usage & (TextureUsage::Attachment | TextureUsage::InputAttachment))
        required |= info.depth || info.stencil ? VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;

    return (format_features[static_cast<u32>(format)] & required) == required;
}

bool Device::HasFeature(Feature feature) const
{
    return (static_cast<u32>(features) & static_cast<u32>(feature)) == static_cast<u32>(feature);
}

Feature Device::GetFeatures() const
{
    return features;
}

bool Device::IsExtensionEnabled(const char* name) const
{
    for (const VkExtensionProperties& extension : device_extensions)
    {
        if (strcmp(extension.extensionName, name) == 0)
            return true;
    }
    return false;
}

bool Device::IsExtensionSupported(const char* name) const
{
    u32 num_available = 0;
    vkEnumerateDeviceExtensionProperties(vk_physical_device, nullptr, &num_available, nullptr);

    std::vector<VkExtensionProperties> available(num_available);
    vkEnumerateDeviceExtensionProperties(vk_physical_device, nullptr, &num_available, available.data());

    for (const VkExtensionProperties& extension : available)
    {
        if (strcmp(extension.extensionName, name) == 0)
            return true;
    }
    return false;
}

const DeviceFeatures& Device::GetEnabledFeatures() const
{
    return enabled_features;
}

Handle<Texture> Device::GetSwapchainTexture()
{
    if (!swapchain_valid)
        return {};

    if (!swapchain.acquired)
    {
        const VkResult result = vkAcquireNextImageKHR(vk_device, swapchain.vk_swapchain, UINT64_MAX, frame_data[frame_index].acquire_semaphore, nullptr, &swapchain.image_index);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            swapchain.needs_recreation = true;
            swapchain_valid = false;
            return {};
        }

        if (result == VK_SUBOPTIMAL_KHR)
            swapchain.needs_recreation = true;
        else if (result != VK_SUCCESS)
            GFX_LOG_FATAL("vkAcquireNextImageKHR failed: %s", string_VkResult(result));

        swapchain.acquired = true;
        Texture& backbuffer = swapchain.GetCurrentTexture().Get();
        backbuffer.state = TextureState::Undefined;
        backbuffer.stages = PipelineStage::ColorOutput;
    }

    return swapchain.GetCurrentTexture();
}

Handle<Texture> Device::GetSwapchainDepthTexture() const
{
    if (!swapchain_valid)
        return {};

    return swapchain.GetDepthTexture();
}

DeviceInfo Device::GetInfo() const
{
    DeviceInfo info =
    {
        .name = device_properties.deviceName,
        .driver_name = driver_properties.driverName,
        .driver_info = driver_properties.driverInfo,
        .type = device_properties.deviceType,
        .api_version = device_properties.apiVersion,
        .vendor_id = device_properties.vendorID,
        .device_id = device_properties.deviceID,
        .timestamp_period_ns = device_properties.limits.timestampPeriod,
        .graphics_queue_family = graphics_queue_family,
        .transfer_queue_family = transfer_queue_family,
        .limits = &device_properties.limits,
    };

    VkPhysicalDeviceMemoryBudgetPropertiesEXT budget =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT,
    };
    VkPhysicalDeviceMemoryProperties2 memory =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2,
        .pNext = HasFeature(Feature::MemoryBudget) ? &budget : nullptr,
    };
    vkGetPhysicalDeviceMemoryProperties2(vk_physical_device, &memory);

    info.num_memory_heaps = memory.memoryProperties.memoryHeapCount;
    for (u32 i = 0; i < info.num_memory_heaps; ++i)
    {
        info.memory_heaps[i] =
        {
            .size = memory.memoryProperties.memoryHeaps[i].size,
            .budget = budget.heapBudget[i],
            .usage = budget.heapUsage[i],
            .device_local = (memory.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0,
        };
    }

    for (u32 i = 0; i < memory.memoryProperties.memoryTypeCount; ++i)
    {
        const VkMemoryType& type = memory.memoryProperties.memoryTypes[i];
        if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
            info.memory_heaps[type.heapIndex].host_visible = true;
    }

    return info;
}

MemoryStats Device::GetMemoryStats() const
{
#if GFX_USE_VMA
    VmaTotalStatistics statistics = {};
    vmaCalculateStatistics(vma_allocator, &statistics);

    return
    {
        .num_memory_objects = statistics.total.statistics.blockCount,
        .num_allocations = statistics.total.statistics.allocationCount,
        .allocated_bytes = statistics.total.statistics.blockBytes,
        .used_bytes = statistics.total.statistics.allocationBytes,
    };
#else
    return gpu_allocator.GetStats();
#endif
}

void Device::CreateInstance(const DeviceConfig& config)
{
    u32 loader_version = VK_API_VERSION_1_0;
    vkEnumerateInstanceVersion(&loader_version);
    if (loader_version < GFX_API_VERSION)
    {
        GFX_LOG_FATAL("gfx.h requires Vulkan %u.%u, runtime reports %u.%u", VK_API_VERSION_MAJOR(GFX_API_VERSION), VK_API_VERSION_MINOR(GFX_API_VERSION), VK_API_VERSION_MAJOR(loader_version), VK_API_VERSION_MINOR(loader_version));
    }

    std::array<const char*, MAX_INSTANCE_EXTENSIONS> extensions = {};
    u32 num_extensions = 0;

    if (config.window)
    {
#if defined(GFX_WINDOW_WIN32)
        extensions[num_extensions++] = VK_KHR_SURFACE_EXTENSION_NAME;
        extensions[num_extensions++] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
#elif defined(GFX_WINDOW_GLFW)
        u32 num_glfw_extensions = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&num_glfw_extensions);
        GFX_ASSERT(num_glfw_extensions < extensions.size());
        for (u32 i = 0; i < num_glfw_extensions; ++i)
            extensions[num_extensions++] = glfw_extensions[i];
#endif
    }

    const char* validation_layer = "VK_LAYER_KHRONOS_validation";
    u32 num_layers = 0;
    bool use_debug_utils = false;
#if GFX_ENABLE_VALIDATION
    num_layers = config.validation != Validation::None ? 1 : 0;
    extensions[num_extensions++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    use_debug_utils = true;
#endif

    const VkDebugUtilsMessengerCreateInfoEXT messenger_info =
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
        .pfnUserCallback = DebugMessageCallback,
    };

    const VkApplicationInfo app_info =
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = config.app_name,
        .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .pEngineName = "gfx",
        .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .apiVersion = GFX_API_VERSION,
    };

    const VkBool32 validate_core = (config.validation & Validation::Core) ? VK_TRUE : VK_FALSE;
    const VkBool32 validate_sync = (config.validation & Validation::Sync) ? VK_TRUE : VK_FALSE;
    const std::array<VkLayerSettingEXT, 2> layer_settings =
    {{
        {
            .pLayerName = validation_layer,
            .pSettingName = "validate_core",
            .type = VK_LAYER_SETTING_TYPE_BOOL32_EXT,
            .valueCount = 1,
            .pValues = &validate_core,
        },
        {
            .pLayerName = validation_layer,
            .pSettingName = "validate_sync",
            .type = VK_LAYER_SETTING_TYPE_BOOL32_EXT,
            .valueCount = 1,
            .pValues = &validate_sync,
        },
    }};

    const void* messenger_next = use_debug_utils ? &messenger_info : nullptr;
    const VkLayerSettingsCreateInfoEXT layer_settings_info =
    {
        .sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT,
        .pNext = messenger_next,
        .settingCount = static_cast<u32>(layer_settings.size()),
        .pSettings = layer_settings.data(),
    };

    const void* instance_next = messenger_next;
    if (num_layers)
        instance_next = &layer_settings_info;

    VkInstanceCreateInfo instance_info =
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = instance_next,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = num_layers,
        .ppEnabledLayerNames = &validation_layer,
        .enabledExtensionCount = num_extensions,
        .ppEnabledExtensionNames = extensions.data(),
    };

    VkResult result = vkCreateInstance(&instance_info, nullptr, &vk_instance);
    if (result == VK_ERROR_LAYER_NOT_PRESENT)
    {
        GFX_LOG_WARNING("Can't find VK_LAYER_KHRONOS_validation, running no validation");
        instance_info.pNext = messenger_next;
        instance_info.enabledLayerCount = 0;
        result = vkCreateInstance(&instance_info, nullptr, &vk_instance);
    }

    if (result == VK_ERROR_EXTENSION_NOT_PRESENT && use_debug_utils)
    {
        GFX_LOG_WARNING("Can't find VK_EXT_debug_utils, no debug names or labels");
        instance_info.pNext = nullptr;
        instance_info.enabledExtensionCount--;
        use_debug_utils = false;
        result = vkCreateInstance(&instance_info, nullptr, &vk_instance);
    }
    VK_CHECK(result);

    if (use_debug_utils)
    {
        debug_utils.create_messenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(vk_instance, "vkCreateDebugUtilsMessengerEXT"));
        debug_utils.destroy_messenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(vk_instance, "vkDestroyDebugUtilsMessengerEXT"));
        debug_utils.set_object_name = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(vk_instance, "vkSetDebugUtilsObjectNameEXT"));
        debug_utils.cmd_begin_label = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(vk_instance, "vkCmdBeginDebugUtilsLabelEXT"));
        debug_utils.cmd_end_label = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(vk_instance, "vkCmdEndDebugUtilsLabelEXT"));
        debug_utils.cmd_insert_label = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(vkGetInstanceProcAddr(vk_instance, "vkCmdInsertDebugUtilsLabelEXT"));

        VK_CHECK(debug_utils.create_messenger(vk_instance, &messenger_info, nullptr, &vk_debug_utils_messenger));
    }
}

void Device::SelectPhysicalDevice(const DeviceConfig& config)
{
    std::array<VkPhysicalDevice, GFX_MAX_GPUS> gpus = {};
    u32 num_gpus = static_cast<u32>(gpus.size());
    vkEnumeratePhysicalDevices(vk_instance, &num_gpus, gpus.data());

    VkPhysicalDeviceType preferred_type = VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM;
    if (config.gpu_preference == GpuPreference::Discrete)
        preferred_type = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    else if (config.gpu_preference == GpuPreference::Integrated)
        preferred_type = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;

    u32 best_priority = 0;
    for (u32 i = 0; i < num_gpus; ++i)
    {
        const VkPhysicalDevice gpu = gpus[i];

        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(gpu, &properties);
        const char* name = properties.deviceName;

        if (properties.apiVersion < GFX_API_VERSION)
        {
            GFX_LOG("GPU skipped: %s (Vulkan version too low)", name);
            continue;
        }

        std::array<VkQueueFamilyProperties, GFX_MAX_QUEUE_FAMILIES> families = {};
        u32 num_families = static_cast<u32>(families.size());
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &num_families, families.data());

        u32 graphics = INVALID_QUEUE_FAMILY;
        u32 transfer = INVALID_QUEUE_FAMILY;
        for (u32 family = 0; family < num_families; ++family)
        {
            const VkQueueFlags flags = families[family].queueFlags;
            const VkExtent3D transfer_granularity = families[family].minImageTransferGranularity;
            const bool transfer_only = (flags & ~VK_QUEUE_SPARSE_BINDING_BIT) == VK_QUEUE_TRANSFER_BIT;
            const bool full_granularity = transfer_granularity.width == 1 && transfer_granularity.height == 1 && transfer_granularity.depth == 1;

            VkBool32 present = VK_TRUE;
            if (vk_surface)
                vkGetPhysicalDeviceSurfaceSupportKHR(gpu, family, vk_surface, &present);

            if (graphics == INVALID_QUEUE_FAMILY && (flags & VK_QUEUE_GRAPHICS_BIT) && present)
                graphics = family;
            if (transfer == INVALID_QUEUE_FAMILY && transfer_only && full_granularity)
                transfer = family;
        }

        if (graphics == INVALID_QUEUE_FAMILY)
        {
            GFX_LOG("GPU skipped: %s (no graphics queue)", name);
            continue;
        }

        if (const char* unsupported = FindUnsupportedFeature(gpu))
        {
            GFX_LOG("GPU skipped: %s (unsupported %s)", name, unsupported);
            continue;
        }

        u32 priority = 1;
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            priority = 2;
        if (properties.deviceType == preferred_type)
            priority = 3;
        if (priority <= best_priority)
            continue;

        best_priority = priority;
        vk_physical_device = gpu;
        device_properties = properties;
        graphics_queue_family = graphics;
        transfer_queue_family = transfer != INVALID_QUEUE_FAMILY ? transfer : graphics;
        timestamps_supported = families[graphics].timestampValidBits != 0 && properties.limits.timestampPeriod != 0.0f;
    }

    if (!vk_physical_device)
        GFX_LOG_FATAL("No valid Vulkan %u.%u GPU found", VK_API_VERSION_MAJOR(GFX_API_VERSION), VK_API_VERSION_MINOR(GFX_API_VERSION));

    VkPhysicalDeviceProperties2 properties2 =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &driver_properties,
    };
    vkGetPhysicalDeviceProperties2(vk_physical_device, &properties2);
    vkGetPhysicalDeviceMemoryProperties(vk_physical_device, &memory_properties);

    for (u32 i = 0; i < format_features.size(); ++i)
    {
        VkFormatProperties format_properties = {};
        vkGetPhysicalDeviceFormatProperties(vk_physical_device, static_cast<VkFormat>(i), &format_properties);
        format_features[i] = format_properties.optimalTilingFeatures;
    }

    if (!timestamps_supported)
        GFX_LOG_WARNING("GPU timestamps are not supported");
}

void Device::CreateLogicalDevice(const DeviceConfig& config)
{
    const VkPhysicalDeviceFeatures2 features2 =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = enabled_features.Link(extension_features.Link(features, config.device_features_next)),
        .features = enabled_features.core10,
    };

    std::vector<const char*> extension_names;
    for (const VkExtensionProperties& extension : device_extensions)
        extension_names.push_back(extension.extensionName);

    const float priority = 1.0f;
    const VkDeviceQueueCreateInfo queue =
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = graphics_queue_family,
        .queueCount = 1,
        .pQueuePriorities = &priority,
    };

    const VkDeviceCreateInfo create_info =
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &features2,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue,
        .enabledExtensionCount = static_cast<u32>(extension_names.size()),
        .ppEnabledExtensionNames = extension_names.data(),
    };
    const VkResult result = vkCreateDevice(vk_physical_device, &create_info, nullptr, &vk_device);
    if (result == VK_ERROR_FEATURE_NOT_PRESENT)
        GFX_LOG_FATAL("%s doesn't support a requested DeviceConfig::extensions", device_properties.deviceName);
    VK_CHECK(result);

    if (features & Feature::MeshShader)
    {
        mesh_shader_functions.draw_mesh_tasks = reinterpret_cast<PFN_vkCmdDrawMeshTasksEXT>(vkGetDeviceProcAddr(vk_device, "vkCmdDrawMeshTasksEXT"));
        mesh_shader_functions.draw_mesh_tasks_indirect = reinterpret_cast<PFN_vkCmdDrawMeshTasksIndirectEXT>(vkGetDeviceProcAddr(vk_device, "vkCmdDrawMeshTasksIndirectEXT"));
        mesh_shader_functions.draw_mesh_tasks_indirect_count = reinterpret_cast<PFN_vkCmdDrawMeshTasksIndirectCountEXT>(vkGetDeviceProcAddr(vk_device, "vkCmdDrawMeshTasksIndirectCountEXT"));
    }

    if (features & (Feature::RayTracing | Feature::RayQuery))
    {
        ray_tracing_functions.create_acceleration_structure = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(vk_device, "vkCreateAccelerationStructureKHR"));
        ray_tracing_functions.destroy_acceleration_structure = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(vk_device, "vkDestroyAccelerationStructureKHR"));
        ray_tracing_functions.get_build_sizes = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(vk_device, "vkGetAccelerationStructureBuildSizesKHR"));
        ray_tracing_functions.get_device_address = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(vk_device, "vkGetAccelerationStructureDeviceAddressKHR"));
        ray_tracing_functions.cmd_build_acceleration_structs = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(vk_device, "vkCmdBuildAccelerationStructuresKHR"));

        acceleration_structure_properties.pNext = (features & Feature::RayTracing) ? &ray_tracing_properties : nullptr;
        VkPhysicalDeviceProperties2 properties =
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            .pNext = &acceleration_structure_properties,
        };
        vkGetPhysicalDeviceProperties2(vk_physical_device, &properties);
    }

    if (features & Feature::RayTracing)
    {
        ray_tracing_functions.create_pipelines = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(vk_device, "vkCreateRayTracingPipelinesKHR"));
        ray_tracing_functions.get_shader_group_handles = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(vk_device, "vkGetRayTracingShaderGroupHandlesKHR"));
        ray_tracing_functions.cmd_trace_rays = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(vk_device, "vkCmdTraceRaysKHR"));
    }

    enabled_features.core13.pNext = extension_features.Link(features, nullptr);

    graphics_queue.Initialize(*this, graphics_queue_family, GFX_MAX_COMMAND_SLOTS);
    upload_queue.Initialize(*this, graphics_queue_family, GFX_MAX_COMMAND_SLOTS);

    LogDeviceInfo(config.required_features | config.optional_features);
}

void Device::CreateFrameData()
{
    frame_data.resize(GFX_MAX_FRAMES_IN_FLIGHT);
    gpu_timings.reserve(GFX_MAX_QUERY_TIMESTAMPS / 2);

    const VkQueryPoolCreateInfo create_info =
    {
        .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
        .flags = 0,
        .queryType = VK_QUERY_TYPE_TIMESTAMP,
        .queryCount = GFX_MAX_QUERY_TIMESTAMPS
    };

    for (i32 i = 0; i < GFX_MAX_FRAMES_IN_FLIGHT; ++i)
    {
        FrameData& frame = frame_data[i]; 
        frame.acquire_semaphore = CreateBinarySemaphore();
        frame.pass_names.resize(GFX_MAX_QUERY_TIMESTAMPS);
        frame.timestamps.resize(GFX_MAX_QUERY_TIMESTAMPS);
        
        if (timestamps_supported)
        {
            VK_CHECK(vkCreateQueryPool(vk_device, &create_info, nullptr, &frame.query_pool));
            vkResetQueryPool(vk_device, frame.query_pool, 0, GFX_MAX_QUERY_TIMESTAMPS);
        }
    }
}

void Device::CreateAllocator()
{
#if GFX_USE_VMA
    VmaAllocatorCreateFlags flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    if (features & Feature::MemoryBudget)
        flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;

    const VmaAllocatorCreateInfo allocator_info =
    {
        .flags = flags,
        .physicalDevice = vk_physical_device,
        .device = vk_device,
        .instance = vk_instance,
        .vulkanApiVersion = GFX_API_VERSION,
    };
    VK_CHECK(vmaCreateAllocator(&allocator_info, &vma_allocator));
#else
    gpu_allocator.Initialize(*this);
#endif
}

void Device::CreateSamplers(const DeviceConfig& config)
{
    const SamplerDesc default_samplers[] =
    {
        {
            .min_filter = SamplerFilter::Nearest,
            .mag_filter = SamplerFilter::Nearest,
            .mipmap_mode = SamplerMipMode::Nearest,
        },
        {
            .min_filter = SamplerFilter::Nearest,
            .mag_filter = SamplerFilter::Nearest,
            .mipmap_mode = SamplerMipMode::Nearest,
            .address_mode_u = SamplerAddressMode::Repeat,
            .address_mode_v = SamplerAddressMode::Repeat,
            .address_mode_w = SamplerAddressMode::Repeat,
        },
        {
            .min_filter = SamplerFilter::Linear,
            .mag_filter = SamplerFilter::Linear,
            .mipmap_mode = SamplerMipMode::Linear,
            .address_mode_u = SamplerAddressMode::ClampToEdge,
            .address_mode_v = SamplerAddressMode::ClampToEdge,
            .address_mode_w = SamplerAddressMode::ClampToEdge,
        },
        {
            .address_mode_u = SamplerAddressMode::Repeat,
            .address_mode_v = SamplerAddressMode::Repeat,
            .address_mode_w = SamplerAddressMode::Repeat,
        },
        {
            .max_anisotropy = 16.0f,
        },
        {
            .address_mode_u = SamplerAddressMode::Repeat,
            .address_mode_v = SamplerAddressMode::Repeat,
            .address_mode_w = SamplerAddressMode::Repeat,
            .max_anisotropy = 16.0f,
        },
        {
            .mipmap_mode = SamplerMipMode::Nearest,
            .address_mode_u = SamplerAddressMode::ClampToBorder,
            .address_mode_v = SamplerAddressMode::ClampToBorder,
            .address_mode_w = SamplerAddressMode::ClampToBorder,
            .compare_enable = true,
            .compare_op = CompareOp::LessOrEqual,
            .border_color = BorderColor::OpaqueWhite,
        },
        {
            .mipmap_mode = SamplerMipMode::Nearest,
            .address_mode_u = SamplerAddressMode::ClampToBorder,
            .address_mode_v = SamplerAddressMode::ClampToBorder,
            .address_mode_w = SamplerAddressMode::ClampToBorder,
            .compare_enable = true,
            .compare_op = CompareOp::GreaterOrEqual,
            .border_color = BorderColor::OpaqueBlack,
        },
    };
    static_assert(std::size(default_samplers) == static_cast<size_t>(DefaultSampler::Count));

    const u32 num_default_samplers = static_cast<u32>(std::size(default_samplers));
    const u32 num_samplers = num_default_samplers + static_cast<u32>(config.samplers.size());
    sampler_pool.Initialize(num_samplers, device_index);
    samplers.reserve(num_samplers);

    for (u32 i = 0; i < num_samplers; ++i)
    {
        const SamplerDesc& desc = i < num_default_samplers ? default_samplers[i] : config.samplers[i - num_default_samplers];

        const VkSamplerCreateInfo sampler_info =
        {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .magFilter = static_cast<VkFilter>(desc.mag_filter),
            .minFilter = static_cast<VkFilter>(desc.min_filter),
            .mipmapMode = static_cast<VkSamplerMipmapMode>(desc.mipmap_mode),
            .addressModeU = static_cast<VkSamplerAddressMode>(desc.address_mode_u),
            .addressModeV = static_cast<VkSamplerAddressMode>(desc.address_mode_v),
            .addressModeW = static_cast<VkSamplerAddressMode>(desc.address_mode_w),
            .mipLodBias = 0.0f,
            .anisotropyEnable = desc.max_anisotropy > 1.0f,
            .maxAnisotropy = std::min(desc.max_anisotropy, device_properties.limits.maxSamplerAnisotropy),
            .compareEnable = desc.compare_enable,
            .compareOp = static_cast<VkCompareOp>(desc.compare_op),
            .minLod = 0.0f,
            .maxLod = VK_LOD_CLAMP_NONE,
            .borderColor = static_cast<VkBorderColor>(desc.border_color),
            .unnormalizedCoordinates = false
        };

        const Handle<Sampler> handle = sampler_pool.Alloc();
        VK_CHECK(vkCreateSampler(vk_device, &sampler_info, nullptr, &sampler_pool.Get(handle).vk_sampler));
        samplers.push_back(handle);
    }
}

void Device::DeferDeletion(const Deletion& deletion)
{
    FrameData& frame = frame_data[last_frame_index];
    frame.deletions.push_back(deletion);
    frame.last_graphics_value = graphics_timeline_value;
}

void Device::DestroyDeletedResources(u32 frame)
{
    for (const Deletion& deletion : frame_data[frame].deletions)
    {
        switch (deletion.type)
        {
        case DeletionType::Buffer:
#if GFX_USE_VMA
            vmaDestroyBuffer(vma_allocator, deletion.buffer, deletion.allocation);
#else
            vkDestroyBuffer(vk_device, deletion.buffer, nullptr);
            gpu_allocator.Free(*this, deletion.allocation);
#endif
            buffer_pool.Release(deletion.index);
            break;
        case DeletionType::Texture:
            if (deletion.bindless && bindless_resource_pool.bindless_descriptor_set)
                bindless_resource_pool.ClearTexture(*this, static_cast<u32>(deletion.index));

            vkDestroyImageView(vk_device, deletion.image_view, nullptr);
            if (deletion.image)
            {
#if GFX_USE_VMA
                vmaDestroyImage(vma_allocator, deletion.image, deletion.allocation);
#else
                vkDestroyImage(vk_device, deletion.image, nullptr);
                gpu_allocator.Free(*this, deletion.allocation);
#endif
            }
            texture_pool.Release(deletion.index);
            break;
        case DeletionType::ShaderModule:
            vkDestroyShaderModule(vk_device, deletion.shader_module, nullptr);
            shader_module_pool.Release(deletion.index);
            break;
        case DeletionType::Pipeline:
            vkDestroyPipeline(vk_device, deletion.pipeline, nullptr);
            pipeline_pool.Release(deletion.index);
            break;
        case DeletionType::AccelerationStructure:
            if (deletion.bindless && bindless_resource_pool.bindless_descriptor_set)
                bindless_resource_pool.ClearAccelerationStructure(*this, static_cast<u32>(deletion.index));

            ray_tracing_functions.destroy_acceleration_structure(vk_device, deletion.acceleration_structure, nullptr);
            acceleration_structure_pool.Release(deletion.index);
            break;
        }
    }
    frame_data[frame].deletions.clear();
}

CommandBuffer Device::RecordUploadCommands(u64 timeline_value)
{
    CommandBuffer cmd = upload_queue.Acquire(*this);
    cmd.frame_slot = last_frame_index;
    cmd.BeginPassTiming("uploads");

    const VkBuffer source = buffer_pool.Get(upload_ring.buffer).vk_buffer;

    bool has_buffer_copies = false;
    for (const UploadCopy& copy : upload_ring.copies)
    {
        if (copy.buffer)
        {
            vkCmdCopyBuffer(cmd.vk_cmd_buffer, source, copy.buffer, 1, &copy.buffer_region);
            has_buffer_copies = true;
            continue;
        }

        const TextureStateInfo undefined = GetTextureStateInfo(TextureState::Undefined);
        const TextureStateInfo transfer = GetTextureStateInfo(TextureState::TransferDst);
        VkImageMemoryBarrier2 barrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = static_cast<VkPipelineStageFlags2>(undefined.stages),
            .srcAccessMask = static_cast<VkAccessFlags2>(undefined.access),
            .dstStageMask = static_cast<VkPipelineStageFlags2>(transfer.stages),
            .dstAccessMask = static_cast<VkAccessFlags2>(transfer.access),
            .oldLayout = undefined.layout,
            .newLayout = transfer.layout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = copy.image,
            .subresourceRange =
            {
                .aspectMask = copy.texture_region.imageSubresource.aspectMask,
                .baseMipLevel = 0,
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount = VK_REMAINING_ARRAY_LAYERS,
            },
        };

        if (copy.upload_start)
        {
            PendingBarriers& pending = *cmd.pending_barriers;
            pending.texture_barriers[pending.num_texture_barriers++] = barrier;
            cmd.FlushBarriers();
        }

        vkCmdCopyBufferToImage(cmd.vk_cmd_buffer, source, copy.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy.texture_region);

        if (copy.upload_end)
        {
            const TextureStateInfo final_info = GetTextureStateInfo(copy.final_state);
            barrier.srcStageMask = static_cast<VkPipelineStageFlags2>(transfer.stages);
            barrier.srcAccessMask = static_cast<VkAccessFlags2>(transfer.access);
            barrier.dstStageMask = static_cast<VkPipelineStageFlags2>(final_info.stages);
            barrier.dstAccessMask = static_cast<VkAccessFlags2>(final_info.access);
            barrier.oldLayout = transfer.layout;
            barrier.newLayout = final_info.layout;
            upload_image_barriers.push_back(barrier);
        }
    }

    if (has_buffer_copies || !upload_image_barriers.empty())
    {
        const VkMemoryBarrier2 buffer_barrier =
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
        };

        const VkDependencyInfo dependency_info =
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .memoryBarrierCount = has_buffer_copies ? 1u : 0u,
            .pMemoryBarriers = &buffer_barrier,
            .imageMemoryBarrierCount = static_cast<u32>(upload_image_barriers.size()),
            .pImageMemoryBarriers = upload_image_barriers.data(),
        };
        vkCmdPipelineBarrier2(cmd.vk_cmd_buffer, &dependency_info);

        upload_image_barriers.clear();
    }

    cmd.EndPassTiming();

    upload_ring.Commit(*this, timeline_value);
    last_upload_value = timeline_value;
    return cmd;
}

u64 Device::FlushUploads()
{
    if (upload_ring.copies.empty())
        return last_upload_value;

    const CommandBuffer cmd = RecordUploadCommands(++graphics_timeline_value);

    VkSemaphoreSubmitInfo signals[1] =
    {
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext = nullptr,
            .semaphore = graphics_timeline,
            .value = graphics_timeline_value,
            .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .deviceIndex = 0,
        },
    };
    upload_queue.Submit(*this, cmd, {}, signals);

    frame_data[last_frame_index].last_graphics_value = last_upload_value;
    return last_upload_value;
}

bool Device::RecreateSwapchain()
{
    VkSurfaceCapabilitiesKHR capabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device, vk_surface, &capabilities));
    const VkExtent2D extent = capabilities.currentExtent;
    if (extent.width == 0 || extent.height == 0)
        return false;

    const VkSwapchainKHR old_swapchain = swapchain.vk_swapchain;
    if (old_swapchain)
    {
        VK_CHECK(vkQueueWaitIdle(graphics_queue.vk_queue));
        swapchain.vk_swapchain = nullptr;
        swapchain.Destroy(*this);
    }

    u32 num_formats = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device, vk_surface, &num_formats, nullptr));
    std::vector<VkSurfaceFormatKHR> surface_formats(num_formats);
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device, vk_surface, &num_formats, surface_formats.data()));

    const VkFormat requested_format = static_cast<VkFormat>(swapchain_config.format);
    VkSurfaceFormatKHR surface_format = surface_formats[0];
    for (const VkSurfaceFormatKHR& format : surface_formats)
    {
        if (format.format == requested_format && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            surface_format = format;
    }
    if (surface_format.format != requested_format && !old_swapchain)
        GFX_LOG_WARNING("Swapchain format %s not supported, using %s", string_VkFormat(requested_format), string_VkFormat(surface_format.format));

    u32 num_present_modes = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device, vk_surface, &num_present_modes, nullptr));
    std::vector<VkPresentModeKHR> present_modes(num_present_modes);
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device, vk_surface, &num_present_modes, present_modes.data()));

    const VkPresentModeKHR requested_present_mode = static_cast<VkPresentModeKHR>(swapchain_config.present_mode);
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    if (std::find(present_modes.begin(), present_modes.end(), requested_present_mode) != present_modes.end())
        present_mode = requested_present_mode;
    else if (!old_swapchain)
        GFX_LOG_WARNING("Present mode %s not supported, using VK_PRESENT_MODE_FIFO_KHR", string_VkPresentModeKHR(requested_present_mode));

    const u32 max_images = capabilities.maxImageCount != 0 ? capabilities.maxImageCount : UINT32_MAX;
    const u32 num_images = std::clamp<u32>(GFX_MAX_SWAPCHAIN_IMAGES, capabilities.minImageCount, max_images);
    const VkSwapchainCreateInfoKHR swapchain_info =
    {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vk_surface,
        .minImageCount = num_images,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = true,
        .oldSwapchain = old_swapchain,
    };

    swapchain = Swapchain(extent, surface_format.format, present_mode);

    VK_CHECK(vkCreateSwapchainKHR(vk_device, &swapchain_info, nullptr, &swapchain.vk_swapchain));
    if (old_swapchain)
        vkDestroySwapchainKHR(vk_device, old_swapchain, nullptr);

    VK_CHECK(vkGetSwapchainImagesKHR(vk_device, swapchain.vk_swapchain, &swapchain.num_swapchain_textures, nullptr));
    GFX_ASSERT(swapchain.num_swapchain_textures <= GFX_MAX_SWAPCHAIN_IMAGES);

    std::array<VkImage, GFX_MAX_SWAPCHAIN_IMAGES> images = {};
    VK_CHECK(vkGetSwapchainImagesKHR(vk_device, swapchain.vk_swapchain, &swapchain.num_swapchain_textures, images.data()));

    for (u32 i = 0; i < swapchain.num_swapchain_textures; ++i)
    {
        const Handle<Texture> texture_handle = texture_pool.Alloc();
        swapchain.swapchain_textures[i] = texture_handle;
        Texture& texture = *texture_pool.GetPtr(texture_handle);

        texture.texture_extent = swapchain.extent;
        texture.num_layers = 1;
        texture.num_mipmaps = 1;
        texture.vk_view_type = VK_IMAGE_VIEW_TYPE_2D;
        texture.texture_format = surface_format.format;

        texture.vk_image = images[i];
        texture.vk_image_view = CreateImageView(texture);
    }

    if (swapchain_config.depth_format != TextureFormat::None)
    {
        swapchain.swapchain_depth_texture = CreateTexture(
        {
            .debug_name = "swapchain depth",
            .texture_type = TextureType::Texture2D,
            .texture_format = swapchain_config.depth_format,
            .texture_usage = TextureUsage::Sample | TextureUsage::Attachment,
            .memory_type = MemoryType::GPU,
            .texture_extent = swapchain.extent,
        });
    }

    for (u32 i = 0; i < swapchain.num_swapchain_textures; ++i)
        swapchain.render_finished_semaphores[i] = CreateBinarySemaphore();

    GFX_LOG_DEBUG("Swapchain: %ux%u, %s, %s, %u images", extent.width, extent.height, string_VkFormat(surface_format.format), string_VkPresentModeKHR(present_mode), swapchain.num_swapchain_textures);
    swapchain.needs_recreation = false;
    return true;
}

void Device::Present()
{
    VkSemaphore wait_semaphores[] = { swapchain.GetRenderFinishedSemaphore() };

    const VkPresentInfoKHR present_info =
    {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = wait_semaphores,
        .swapchainCount = 1,
        .pSwapchains = &swapchain.vk_swapchain,
        .pImageIndices = &swapchain.image_index,
    };

    const VkResult result = vkQueuePresentKHR(graphics_queue.vk_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        swapchain.needs_recreation = true;
    else if (result != VK_SUCCESS)
        GFX_LOG_FATAL("vkQueuePresentKHR failed: %s", string_VkResult(result));
}

void Device::SetEnabledFeatures(const DeviceConfig& config, Feature optional_features)
{
    features = optional_features;
    enabled_features = config.device_features;
    enabled_features.core11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    enabled_features.core12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    enabled_features.core13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    enabled_features.core10.samplerAnisotropy = VK_TRUE;
    enabled_features.core10.imageCubeArray = VK_TRUE;
    enabled_features.core10.multiDrawIndirect = VK_TRUE;
    enabled_features.core10.drawIndirectFirstInstance = VK_TRUE;
    enabled_features.core10.geometryShader = VK_TRUE;
    enabled_features.core10.depthBiasClamp = VK_TRUE;
    enabled_features.core10.fillModeNonSolid = VK_TRUE;
    enabled_features.core10.shaderStorageImageReadWithoutFormat = VK_TRUE;
    enabled_features.core10.shaderStorageImageWriteWithoutFormat = VK_TRUE;
    enabled_features.core12.bufferDeviceAddress = VK_TRUE;
    enabled_features.core12.timelineSemaphore = VK_TRUE;
    enabled_features.core12.hostQueryReset = VK_TRUE;
    enabled_features.core12.runtimeDescriptorArray = VK_TRUE;
    enabled_features.core12.descriptorBindingPartiallyBound = VK_TRUE;
    enabled_features.core12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    enabled_features.core12.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
    enabled_features.core12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    enabled_features.core12.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
    enabled_features.core13.synchronization2 = VK_TRUE;
    enabled_features.core13.dynamicRendering = VK_TRUE;

    extension_features = {};

    device_extensions.clear();
    if (config.window)
        EnableExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    for (const char* extension : config.extensions)
    {
        GFX_ASSERT(extension && "DeviceConfig::extensions has a null name");
        EnableExtension(extension);
    }

    for (u32 bits = static_cast<u32>(optional_features); bits; bits &= bits - 1)
    {
        EnableFeature(static_cast<Feature>(1u << std::countr_zero(bits)));
    }

    ray_tracing_maintenance1_supported = false;
    null_descriptor_supported = false;
    if (!vk_physical_device)
        return;

    ray_tracing_maintenance1_supported = (optional_features & Feature::RayTracing) && IsExtensionSupported(VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME);
    if (ray_tracing_maintenance1_supported)
    {
        EnableExtension(VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME);
        extension_features.ray_tracing_maintenance1.rayTracingMaintenance1 = VK_TRUE;
    }

    null_descriptor_supported = IsExtensionSupported(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME) && IsFeatureSupported(&VkPhysicalDeviceRobustness2FeaturesEXT::nullDescriptor);
    if (null_descriptor_supported)
    {
        EnableExtension(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME);
        extension_features.robustness2.nullDescriptor = VK_TRUE;
    }
}

void Device::EnableExtension(const char* name)
{
    if (IsExtensionEnabled(name))
        return;

    VkExtensionProperties extension = {};
    strncpy_s(extension.extensionName, name, _TRUNCATE);
    device_extensions.push_back(extension);
}

void Device::EnableFeature(Feature feature)
{
    switch (feature)
    {
    case Feature::MeshShader:
        EnableExtension(VK_EXT_MESH_SHADER_EXTENSION_NAME);
        extension_features.mesh_shader.taskShader = VK_TRUE;
        extension_features.mesh_shader.meshShader = VK_TRUE;
        break;
    case Feature::RayTracing:
        EnableExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        EnableExtension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        EnableExtension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        extension_features.acceleration_structure.accelerationStructure = VK_TRUE;
        extension_features.acceleration_structure.descriptorBindingAccelerationStructureUpdateAfterBind = VK_TRUE;
        extension_features.ray_tracing_pipeline.rayTracingPipeline = VK_TRUE;
        break;
    case Feature::RayQuery:
        EnableExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        EnableExtension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        EnableExtension(VK_KHR_RAY_QUERY_EXTENSION_NAME);
        extension_features.acceleration_structure.accelerationStructure = VK_TRUE;
        extension_features.acceleration_structure.descriptorBindingAccelerationStructureUpdateAfterBind = VK_TRUE;
        extension_features.ray_query.rayQuery = VK_TRUE;
        break;
    case Feature::DrawIndirectCount:
        enabled_features.core12.drawIndirectCount = VK_TRUE;
        break;
    case Feature::MemoryBudget:
        EnableExtension(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
        break;
    default:
        break;
    }
}

const char* Device::FindUnsupportedFeature(VkPhysicalDevice gpu) const
{
    u32 num_available = 0;
    vkEnumerateDeviceExtensionProperties(gpu, nullptr, &num_available, nullptr);
    std::vector<VkExtensionProperties> available(num_available);
    vkEnumerateDeviceExtensionProperties(gpu, nullptr, &num_available, available.data());

    for (const VkExtensionProperties& extension : device_extensions)
    {
        const bool found = std::any_of(available.begin(), available.end(), [&extension](const VkExtensionProperties& other)
        {
            return strcmp(extension.extensionName, other.extensionName) == 0;
        });

        if (!found)
            return extension.extensionName;
    }

    DeviceFeatures supported = {};
    ExtensionFeatures supported_extensions = {};
    VkPhysicalDeviceFeatures2 query =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = supported.Link(supported_extensions.Link(features, nullptr)),
    };
    vkGetPhysicalDeviceFeatures2(gpu, &query);
    supported.core10 = query.features;

    if (!DeviceFeatures::HasAllFeatures(supported.core10, enabled_features.core10, &VkPhysicalDeviceFeatures::inheritedQueries))
        return "VkPhysicalDeviceFeatures";
    if (!DeviceFeatures::HasAllFeatures(supported.core11, enabled_features.core11, &VkPhysicalDeviceVulkan11Features::shaderDrawParameters))
        return "VkPhysicalDeviceVulkan11Features";
    if (!DeviceFeatures::HasAllFeatures(supported.core12, enabled_features.core12, &VkPhysicalDeviceVulkan12Features::subgroupBroadcastDynamicId))
        return "VkPhysicalDeviceVulkan12Features";
    if (!DeviceFeatures::HasAllFeatures(supported.core13, enabled_features.core13, &VkPhysicalDeviceVulkan13Features::maintenance4))
        return "VkPhysicalDeviceVulkan13Features";
    if (!DeviceFeatures::HasAllFeatures(supported_extensions.mesh_shader, extension_features.mesh_shader, &VkPhysicalDeviceMeshShaderFeaturesEXT::meshShaderQueries))
        return "VkPhysicalDeviceMeshShaderFeaturesEXT";
    if (!DeviceFeatures::HasAllFeatures(supported_extensions.acceleration_structure, extension_features.acceleration_structure, &VkPhysicalDeviceAccelerationStructureFeaturesKHR::descriptorBindingAccelerationStructureUpdateAfterBind))
        return "VkPhysicalDeviceAccelerationStructureFeaturesKHR";
    if (!DeviceFeatures::HasAllFeatures(supported_extensions.ray_tracing_pipeline, extension_features.ray_tracing_pipeline, &VkPhysicalDeviceRayTracingPipelineFeaturesKHR::rayTraversalPrimitiveCulling))
        return "VkPhysicalDeviceRayTracingPipelineFeaturesKHR";
    if (!DeviceFeatures::HasAllFeatures(supported_extensions.ray_query, extension_features.ray_query, &VkPhysicalDeviceRayQueryFeaturesKHR::rayQuery))
        return "VkPhysicalDeviceRayQueryFeaturesKHR";
    return nullptr;
}

#if GFX_USE_SPIRV_REFLECT
void Device::ReflectPipelineStage(Pipeline& pipeline, const ShaderModule& shader)
{
    SpvReflectShaderModule& module = pipeline.reflections.emplace_back();
    const SpvReflectResult result = spvReflectCreateShaderModule(spvReflectGetCodeSize(&shader.reflection), spvReflectGetCode(&shader.reflection), &module);
    GFX_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);
}
#endif

void Device::LogDeviceInfo(Feature requested_features) const
{
    const DeviceInfo info = GetInfo();
    const char* type = info.type == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU     ? "discrete"
                       : info.type == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? "integrated"
                       : info.type == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU    ? "virtual"
                       : info.type == VK_PHYSICAL_DEVICE_TYPE_CPU            ? "cpu"
                                                                             : "other";
    GFX_LOG("GPU: %s (%s, Vulkan %u.%u.%u, %s %s)", info.name, type, VK_API_VERSION_MAJOR(info.api_version), VK_API_VERSION_MINOR(info.api_version), VK_API_VERSION_PATCH(info.api_version),
            info.driver_name, info.driver_info);

    std::array<VkQueueFamilyProperties, GFX_MAX_QUEUE_FAMILIES> families = {};
    u32 num_families = static_cast<u32>(families.size());
    vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device, &num_families, families.data());

    GFX_LOG("  available queue families (%u):", num_families);
    for (u32 i = 0; i < num_families; ++i)
    {
        const VkQueueFlags flags = families[i].queueFlags;
        GFX_LOG("    %u: %2u queues%s%s%s%s%s", i, families[i].queueCount,
            (flags & VK_QUEUE_GRAPHICS_BIT) ? " graphics" : "",
            (flags & VK_QUEUE_COMPUTE_BIT) ? " compute" : "",
            (flags & VK_QUEUE_TRANSFER_BIT) ? " transfer" : "",
            (flags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) ? " video decode" : "",
            (flags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) ? " video encode" : "");
    }

    constexpr const char* FEATURE_NAMES[] = {"MeshShader", "RayTracing", "RayQuery", "DrawIndirectCount", "MemoryBudget"};
    static_assert(std::size(FEATURE_NAMES) == std::bit_width(static_cast<u32>(Feature::MemoryBudget)), "FEATURE_NAMES doesn't match Feature");
    const u32 enabled_bits = static_cast<u32>(features);
    const u32 requested_bits = enabled_bits | static_cast<u32>(requested_features);
    GFX_LOG("  requested features (%u):", static_cast<u32>(std::popcount(requested_bits)));
    for (u32 bits = requested_bits; bits; bits &= bits - 1)
    {
        const u32 bit = std::countr_zero(bits);
        GFX_LOG("    %s%s", FEATURE_NAMES[bit], (enabled_bits & (1u << bit)) ? "" : " (not supported)");
    }

    GFX_LOG("  requested extensions (%u):", static_cast<u32>(device_extensions.size()));
    for (const VkExtensionProperties& extension : device_extensions)
        GFX_LOG("    %s", extension.extensionName);

    constexpr double GB = 1024.0 * 1024.0 * 1024.0;
    GFX_LOG("  memory heaps (%u):", info.num_memory_heaps);
    for (u32 i = 0; i < info.num_memory_heaps; ++i)
    {
        const MemoryHeapInfo& heap = info.memory_heaps[i];
        const char* type = "system memory";
        if (heap.device_local)
            type = heap.host_visible ? "device local, host visible" : "device local";

        if (HasFeature(Feature::MemoryBudget))
            GFX_LOG("    %u: %6.2f GB %s, budget %.2f GB, used %.2f GB", i, heap.size / GB, type, heap.budget / GB, heap.usage / GB);
        else
            GFX_LOG("    %u: %6.2f GB %s", i, heap.size / GB, type);
    }
}

VkImageView Device::CreateImageView(const Texture& texture, TextureAspect aspect)
{
    const VkImageViewCreateInfo image_info =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = texture.vk_image,
        .viewType = texture.vk_view_type,
        .format = texture.texture_format,
        .components =
        {
            .r = VK_COMPONENT_SWIZZLE_R,
            .g = VK_COMPONENT_SWIZZLE_G,
            .b = VK_COMPONENT_SWIZZLE_B,
            .a = VK_COMPONENT_SWIZZLE_A
        },
        .subresourceRange =
        {
            .aspectMask = aspect != TextureAspect::Auto ? static_cast<VkImageAspectFlags>(aspect) : GetImageAspectFlags(texture.texture_format),
            .baseMipLevel = texture.base_mip_level,
            .levelCount = texture.num_mipmaps,
            .baseArrayLayer = texture.base_array_layer,
            .layerCount = texture.num_layers
        }
    };

    VkImageView image_view = nullptr;
    VK_CHECK(vkCreateImageView(vk_device, &image_info, nullptr, &image_view));

    return image_view;
}

VkSemaphore Device::CreateBinarySemaphore()
{
    const VkSemaphoreCreateInfo semaphore_info =
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0
    };

    VkSemaphore semaphore = nullptr;
    VK_CHECK(vkCreateSemaphore(vk_device, &semaphore_info, nullptr, &semaphore));

    return semaphore;
}

VkSemaphore Device::CreateTimelineSemaphore()
{
    VkSemaphoreTypeCreateInfo type_info =
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .pNext = nullptr,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue = 0,
    };

    const VkSemaphoreCreateInfo create_info =
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &type_info,
        .flags = 0,
    };
    
    VkSemaphore semaphore = nullptr;
    VK_CHECK(vkCreateSemaphore(vk_device, &create_info, nullptr, &semaphore));
    return semaphore;
}

VkAccelerationStructureGeometryKHR Device::CreateAccelStructGeometry(const AccelerationStructureGeometry& input)
{
    GFX_ASSERT(input.vertex_buffer.IsValid());
    GFX_ASSERT(input.vertex_buffer->buffer_usage & BufferUsage::AccelerationStructureInput);
    GFX_ASSERT(input.num_vertices > 0);
    GFX_ASSERT(input.num_triangles > 0);

    VkIndexType index_type = VK_INDEX_TYPE_NONE_KHR;
    VkDeviceAddress index_address = 0;
    if (input.index_buffer.IsValid())
    {
        GFX_ASSERT(input.index_buffer->buffer_usage & BufferUsage::AccelerationStructureInput);
        index_type = ToVkIndexType(input.index_type);
        index_address = input.index_buffer->vk_device_address + input.index_offset;
    }

    VkDeviceAddress transform_address = 0;
    if (input.transform_buffer.IsValid())
    {
        GFX_ASSERT(input.transform_buffer->buffer_usage & BufferUsage::AccelerationStructureInput);
        transform_address = input.transform_buffer->vk_device_address + input.transform_offset;
    }
    GFX_ASSERT(transform_address % ACCELERATION_STRUCTURE_INPUT_ALIGNMENT == 0 && "transform must be 16 byte aligned");

    return
    {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
        .geometry =
        {
            .triangles =
            {
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                .vertexFormat = static_cast<VkFormat>(input.vertex_format),
                .vertexData = { .deviceAddress = input.vertex_buffer->vk_device_address + input.vertex_offset },
                .vertexStride = input.vertex_stride,
                .maxVertex = input.num_vertices - 1,
                .indexType = index_type,
                .indexData = { .deviceAddress = index_address },
                .transformData = { .deviceAddress = transform_address },
            },
        },
        .flags = input.opaque ? VK_GEOMETRY_OPAQUE_BIT_KHR : 0u,
    };
}

Device& CreateDevice(const DeviceConfig& config)
{
    u32 device_index = 0;
    while (device_index < GFX_MAX_GPUS && g_device_context.devices[device_index])
        ++device_index;
    GFX_ASSERT(device_index < GFX_MAX_GPUS && "Too many devices");

    g_device_context.devices[device_index].reset(new Device(config, device_index));
    Device* device = g_device_context.devices[device_index].get();

    if (!g_device_context.current)
        g_device_context.current = device;

    device->Initialize();
    return *device;
}

void DestroyDevice(Device& device)
{
    const u32 device_index = device.device_index;
    GFX_ASSERT(g_device_context.devices[device_index].get() == &device);
    const bool destroyed_current = g_device_context.current == &device;

    device.Shutdown();
    g_device_context.devices[device_index].reset();

    if (destroyed_current)
    {
        g_device_context.current = nullptr;
        for (const std::unique_ptr<Device>& other : g_device_context.devices)
        {
            if (other && !g_device_context.current)
                g_device_context.current = other.get();
        }
    }
}

void SetCurrentDevice(Device& device)
{
    GFX_ASSERT(g_device_context.devices[device.device_index].get() == &device);
    g_device_context.current = &device;
}

Device& GetDevice()
{
    GFX_ASSERT(g_device_context.current);
    return *g_device_context.current;
}

}

#endif
