#pragma once
#include "io.hpp"

// If you want to change OpenGL Core version in order to
// limit your codebase on IDE level (e.g. Context Menu show-up), then
// `#define IO_GL_API_CORE 200` (for smallest "2.0.0") once in source file before including `hi.hpp`
#ifndef IO_GL_API_CORE
#   define IO_GL_API_CORE 460 // max version 4.6.0 by default
#endif

#ifndef assert
#   ifdef _DEBUG
#       include <assert.h>
#   else
#       define assert // empty macro to do nothing
#   endif
#endif

// ------------------- OS-dependent Includes -------------------------
#ifdef IO_IMPLEMENTATION
#   if defined(_WIN32)
#      ifndef WIN32_LEAN_AND_MEAN
#          define WIN32_LEAN_AND_MEAN
#      endif
#      ifndef NOMINMAX
#          define NOMINMAX
#      endif
#      include <Windows.h>
#      include <gl/GL.h>
#   elif defined(__linux__)
    // gl
#   else
#       error "OS isn't specified"
#   endif
#endif // IO_IMPLEMENTATION

#pragma region macros
#if !defined(NDEBUG) && !defined(_DEBUG)
#   define _DEBUG
#endif
// ======================= OpenGL Macros ======================================
#if defined(_WIN32)
#   define GL_APIENTRY __stdcall
#else
#   define GL_APIENTRY
#endif
#pragma endregion // macros

namespace gl {
    using LoadProc = void* (*)(const char* name);
    static LoadProc loader = nullptr;
    static bool loaded = false;
    static io::u8 loaded_major = 0;
    static io::u8 loaded_minor = 0;

    static inline bool ver_ge(int maj, int min, int req_maj, int req_min) noexcept {
        return (maj > req_maj) || (maj == req_maj && min >= req_min);
    }

    // DefaultReturnImpl
    namespace native {
        template <typename T> inline T DefaultReturnImpl() noexcept { return T{}; }
        template <> inline void DefaultReturnImpl<void>() noexcept {}
    } // namespace native
} // namespace gl

#define GL_CALL_CUSTOM(_Name, _RetType, _ProcT, _Args_Decl, _Args_Pass)       \
namespace gl {                                                                \
namespace native {                                                            \
    using PFN##_Name = _RetType (GL_APIENTRY*) _ProcT;                        \
    static PFN##_Name p##_Name = nullptr;                                     \
} /* namespace native */                                                      \
static inline _RetType _Name _Args_Decl noexcept {                            \
    assert(::gl::loaded && "gl::" #_Name " called before gl::load()");        \
    assert(::gl::native::p##_Name && "gl::" #_Name " missing");               \
    return ::gl::native::p##_Name _Args_Pass;                                 \
} /* glFunc */                                                                \
} // namespace gl

#define GL_CALL_OVERLOAD(_Name, _RetType, _ProcT, _Args_Decl, _Args_Pass)     \
namespace gl {                                                                \
static inline _RetType _Name _Args_Decl noexcept {                            \
    assert(::gl::loaded && "gl::" #_Name " called before gl::load()");        \
    assert(::gl::native::p##_Name && "gl::" #_Name " missing");               \
    return ::gl::native::p##_Name _Args_Pass;                                 \
} /* glFunc */                                                                \
} // namespace gl

// ========================== opengl enums ====================================

namespace gl {
    IO_CONSTEXPR_VAR static int UNPACK_ALIGNMENT = 0x0CF5;
    // These values correspond to the standard OpenGL flags for glClear().  
    // They can be combined using the `|` operator, for example:
    //     gl::Clear(gl::buffer_bit.Color | gl::buffer_bit.Depth);
    //
    // Each flag indicates which buffer should be cleared before rendering a new frame:
    //   - Color   — clears the color buffer where the final scene pixels are stored.
    //   - Depth   — clears the depth buffer that contains per-pixel depth values.
    //   - Stencil — clears the stencil buffer if stencil testing is used.
    IO_CONSTEXPR_VAR struct BufferBit {
        IO_CONSTEXPR_VAR static int Depth   = 0x00000100;
        IO_CONSTEXPR_VAR static int Stencil = 0x00000400;
        IO_CONSTEXPR_VAR static int Color   = 0x00004000;
    } buffer_bit;

    struct GlVersionParam {
        IO_CONSTEXPR_VAR static int major = 0x821B;
        IO_CONSTEXPR_VAR static int minor = 0x821C;
    };

    enum class Face : io::u32 {
        Front = 0x404,
        Back = 0x405,
        FrontAndBack = 0x408,
    };

    enum class Polygon : io::u32 {
        Point = 0x1B00,
        Line = 0x1B01,
        Fill = 0x1B02,
    };

    enum class Capability : io::u32 {
        Blend = 0x0BE2,
        CullFace = 0x0B44,
        DepthTest = 0x0B71,
        // others
    };

    enum class BlendFactor : io::u32 {
        Zero = 0,
        One = 1,
        SrcAlpha = 0x0302,
        OneMinusSrcAlpha = 0x0303,
        // others
    };

    // -------------------------------------------------------------------------
    // TexTarget: what kind of texture object is being specified/bound.
    // This is not "format"; it selects the texture *type* and (for cubemaps) the face.
    // -------------------------------------------------------------------------
    enum class TexTarget : io::u32 {
        // Regular 2D texture: width × height, UV coordinates normalized [0..1].
        // Supports mipmaps (levels 0..N).
        Tex2D = 0x0DE1,

        // 1D array texture: (width × layers). In glTexImage2D call, "height" parameter is the layer count.
        // In shaders: sampler1DArray / isampler1DArray / usampler1DArray, sampled with (u, layer).
        Array1D = 0x8C18,

        // Rectangle texture: (width × height) but texture coordinates are *unnormalized* (pixel-space).
        // Mipmaps are not allowed, so `level` must be 0. Rare in modern renderers; historically used for some FBO/video paths.
        TexRectangle = 0x84F5,

        // Cubemap faces. A cubemap is conceptually one texture with 6 square faces.
        // When using glTexImage2D, you specify each face with one of these targets.
        // Sampling is via samplerCube with a direction vector (vec3).
        CubeMapPositiveX = 0x8515,
        CubeMapNegativeX = 0x8516,
        CubeMapPositiveY = 0x8517,
        CubeMapNegativeY = 0x8518,
        CubeMapPositiveZ = 0x8519,
        CubeMapNegativeZ = 0x851A,
    };

    enum class TexUnit : io::u32 {
         _0 = 0x84C0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31,
    };

    enum class TexParam : io::u32 {
        MinFilter = 0x2801,
        MagFilter = 0x2800,
        WrapS = 0x2802,
        WrapT = 0x2803,
        // others
    };

    // Initially is set to `Repeat`
    enum class TexWrap : int {
        ClampToEdge = 0x812F,
        ClampToBorder = 0x812D,
        MirroredRepeat = 0x8370,
        Repeat = 0x2901
    };

    // -------------------------------------------------------------------------
    // TexFormat: CPU-side channel order for pixel transfers (upload/download).
    // This describes how the incoming bytes should be interpreted as channels.
    // It does NOT define GPU VRAM storage (that's gl::InternalFormat).
    // -------------------------------------------------------------------------
    enum class TexFormat : int {
        R  = 0x1903,  // One channel (red). Often used for masks, heightfields, single-channel data.
        RG = 0x8227,  // Two channels (red, green). Common for two-component data (e.g., motion vectors, RG packed normals).
        
        // Red-Green-Blue is common as an upload format, but many
        // engines avoid RGB GPU storage due to `alignment`/`bandwidth pitfalls`,
        RGB = 0x1907,
        RGBA = 0x1908,

        // BGR/BGRA describe *byte order* in CPU memory. Useful when loading images from APIs that output BGR(A).
        BGR = 0x80E0, // blue-green-red
        BGRA = 0x80E1, // blue-green-red-alpha
    };

    // -------------------------------------------------------------------------
    // InternalFormat: GPU storage layout in VRAM (glTexImage2D 'internalFormat').
    // This defines:
    //   - component count (R / RG / RGB / RGBA / depth / depth+stencil)
    //   - bit widths (8/16/32 etc)
    //   - numeric interpretation (UNORM, SNORM, FLOAT, INT, UINT, SRGB, compressed)
    //
    // Suffix meaning (sampling):
    //   (none)         UNORM (or special packed float/srgb) -> sampler* returns normalized float
    //   _SNORM         SNORM -> sampler* returns float in [-1, 1]
    //   _F             float -> sampler* returns float (real float storage)
    //   _I / _UI       integer -> isampler*/usampler* returns int/uint vectors (no normalization)
    //
    // IMPORTANT:
    //   InternalFormat is about VRAM. It does not need to match the upload 'DataType'.
    //   OpenGL converts CPU data into this storage on upload.
    // -------------------------------------------------------------------------
    enum class InternalFormat : int {
        // TODO: gl core 4 formats

        // ----------------------------- RGBA -----------------------------
        RGBA32_F  = 0x8814, // 4×32-bit float (128 bpp). Highest precision color/storage; expensive in bandwidth.
        RGBA32_I  = 0x8D82, // 4×32-bit signed integer (128 bpp). For integer textures (no normalization). Use `isampler2D`.
        RGBA32_UI = 0x8D70, // 4×32-bit unsigned integer (128 bpp). For integer textures. Use usampler2D.
        RGBA16    = 0x805B, // 4×16-bit UNORM (64 bpp). Stored as integers; sampled as float in [0..1].
        RGBA16_F  = 0x881A, // 4×16-bit float (64 bpp). Common HDR intermediate format (lighting/bloom).
        RGBA16_I  = 0x8D88, // 4×16-bit signed integer (64 bpp). Integer texture. isampler2D.
        RGBA16_UI = 0x8D76, // 4×16-bit unsigned integer (64 bpp). Integer texture. usampler2D.
        RGBA8     = 0x8058, // 4×8-bit UNORM (32 bpp). Default "color texture" format.
        RGBA8_UI  = 0x8D7C, // 4×8-bit unsigned integer (32 bpp). Integer texture for IDs/masks/etc. usampler2D.

        // 4×8-bit sRGB color + 8-bit alpha. Stored in sRGB space; on sampling converts to linear float.
        // Use for albedo/UI authored in sRGB. Do NOT use for data/normal maps.
        SRGB8_A8  = 0x8C43,

        // 10/10/10/2 packed UNORM (32 bpp). Sampled as float [0..1].
        // Good for compact HDR-ish buffers or normals (with care).
        RGB10_A2  = 0x8059,

        // Same bit layout as RGB10_A2 but *UNSIGNED INTEGER* interpretation.
        // Sample with usampler* and you get raw integer ranges: RGB ∈ [0..1023], A ∈ [0..3].
        RGB10_A2_UI = 0x906F,

        // Packed float format: R=11f, G=11f, B=10f (no alpha), 32 bpp.
        // Great HDR color buffer when alpha not needed; higher quality than 10:10:10:2 for lighting.
        R11_G11_B10_F = 0x8C3A,
        RGBA16_SNORM = 0x8F9B, // Signed normalized variant (sampled to [-1,1]).
        RGBA8_SNORM  = 0x8F97, // Signed normalized variant (sampled to [-1,1]).

        // ----------------------------- RGB -----------------------------

        RGB32_F  = 0x8815,
        RGB32_I  = 0x8D83,
        RGB32_UI = 0x8D71,

        RGB16    = 0x8054, // GL_RGB16 (UNORM)
        RGB16_F  = 0x881B,
        RGB16_I  = 0x8D89,
        RGB16_UI = 0x8D77,
        RGB16_SNORM = 0x8F9A,

        RGB8     = 0x8051, // GL_RGB8 (UNORM)
        RGB8_I   = 0x8D8F,
        RGB8_UI  = 0x8D7D,
        RGB8_SNORM = 0x8F96,

        SRGB8 = 0x8C41, // sRGB without alpha (for color-only textures).

        // Packed HDR with shared exponent: RGB mantissa 9 bits each + shared exponent 5 bits (32 bpp).
        // Compact HDR-ish storage; used sometimes for environment/light probes.
        RGB9_E5 = 0x8C3D,

        // ----------------------------- RG -----------------------------

        RG32_F  = 0x8230,
        RG32_I  = 0x823B,
        RG32_UI = 0x823C,

        RG16    = 0x822C, // GL_RG16 (UNORM)
        RG16_F  = 0x822F,
        RG16_I  = 0x8239,
        RG16_UI = 0x823A,
        RG16_SNORM = 0x8F99,

        RG8     = 0x822B, // GL_RG8 (UNORM)
        RG8_I   = 0x8237,
        RG8_UI  = 0x8238,
        RG8_SNORM = 0x8F95,


        // ----------------------------- R -----------------------------

        R32_F  = 0x822E,
        R32_I  = 0x8235,
        R32_UI = 0x8236,

        R16_F  = 0x822D,
        R16_I  = 0x8233,
        R16_UI = 0x8234,
        R16_SNORM = 0x8F98,

        R8     = 0x8229, // GL_R8 (UNORM)
        R8_I   = 0x8231,
        R8_UI  = 0x8232,
        R8_SNORM = 0x8F94,


        // -------------------------- Compressed (RGTC / BC4/BC5) --------------------------
        //
        // These formats are block-compressed. You typically upload via glCompressedTexImage*,
        // not glTexImage2D, and DataType/TexFormat do not apply in the same way.
        //
        // RGTC1 == BC4 (single-channel), RGTC2 == BC5 (two-channel).
        // Common use:
        //   - BC4: single channel masks/height.
        //   - BC5: two-channel normal maps (XY), reconstruct Z in shader.
        //
        // Signed variants decode to signed normalized values.
        COMPRESSED_RED_RGTC1         = 0x8DBB,
        COMPRESSED_SIGNED_RED_RGTC1  = 0x8DBC,
        COMPRESSED_RG_RGTC2          = 0x8DBD,
        COMPRESSED_SIGNED_RG_RGTC2   = 0x8DBE,


        // ----------------------------- Depth / Depth-Stencil -----------------------------

        DEPTH_COMPONENT32_F = 0x8CAC, // 32-bit float depth. High precision; useful for large ranges, sometimes slower/bigger.        
        DEPTH_COMPONENT24   = 0x81A6, // 24-bit UNORM depth. Most common depth buffer format.
        DEPTH_COMPONENT16   = 0x81A5, // 16-bit UNORM depth. Smaller/faster, lower precision.
        DEPTH32F_STENCIL8   = 0x8CAD, // 32f depth + 8-bit stencil. Precision depth with stencil.
        DEPTH24_STENCIL8    = 0x88F0, // 24-bit depth + 8-bit stencil. Most common depth-stencil format.
    };

    // -------------------------------------------------------------------------
    // DataType: CPU-side representation/packing for pixel transfer (glTexImage2D 'type').
    // This tells GL how to parse your RAM bytes into components described by TexFormat.
    // It does NOT define the GPU storage; GL will convert into InternalFormat.
    // -------------------------------------------------------------------------
    enum class DataType : io::u32 {
        // Plain scalar element types (one element per component).
        UnsignedByte  = 0x1401, // GL_UNSIGNED_BYTE
        Byte          = 0x1400, // GL_BYTE
        UnsignedShort = 0x1403, // GL_UNSIGNED_SHORT
        Short         = 0x1402, // GL_SHORT
        UnsignedInt   = 0x1405, // GL_UNSIGNED_INT
        Int           = 0x1404, // GL_INT
        Float         = 0x1406, // GL_FLOAT

        // Packed pixel layouts (multiple components packed into a single integer).
        // Use when your CPU source is already in a packed layout (common in some image/codec outputs).
        UByte_3_3_2        = 0x8032, // GL_UNSIGNED_BYTE_3_3_2
        UByte_2_3_3_Rev    = 0x8362, // GL_UNSIGNED_BYTE_2_3_3_REV

        UShort_5_6_5       = 0x8363, // GL_UNSIGNED_SHORT_5_6_5
        UShort_5_6_5_Rev   = 0x8364, // GL_UNSIGNED_SHORT_5_6_5_REV

        UShort_4_4_4_4     = 0x8033, // GL_UNSIGNED_SHORT_4_4_4_4
        UShort_4_4_4_4_Rev = 0x8365, // GL_UNSIGNED_SHORT_4_4_4_4_REV

        UShort_5_5_5_1     = 0x8034, // GL_UNSIGNED_SHORT_5_5_5_1
        UShort_1_5_5_5_Rev = 0x8366, // GL_UNSIGNED_SHORT_1_5_5_5_REV

        UInt_8_8_8_8       = 0x8035, // GL_UNSIGNED_INT_8_8_8_8
        UInt_8_8_8_8_Rev   = 0x8367, // GL_UNSIGNED_INT_8_8_8_8_REV

        UInt_10_10_10_2    = 0x8036, // GL_UNSIGNED_INT_10_10_10_2
        UInt_2_10_10_10_Rev= 0x8368, // GL_UNSIGNED_INT_2_10_10_10_REV
    };


    enum class GetParam : io::u32 {
        Viewport = 0x0BA2,
        // others
    };

    // The texture minifying function. Used whenever the level-of-detail function used
    // when sampling from the texture determines that the texture should be minified. 
    enum class MinifyingFilter : io::u32 {
        // Returns the weighted average of the four texture elements that are closest to the specified texture coordinates.
        // These can include items wrapped or repeated from other parts of a texture, depending on the values of `gl::TexParam::WrapS` and `gl::TexParam::WrapT`, and on the exact mapping. 
        Nearest = 0x2600,

        // Returns the weighted average of the four texture elements that are closest to the specified texture coordinates.
        // These can include items wrapped or repeated from other parts of a texture, depending on the values of `gl::TexParam::WrapS` and `gl::TexParam::WrapT`, and on the exact mapping. 
        Linear = 0x2601,

        // Chooses the mipmap that most closely matches the size of the pixel being textured and
        // uses the `gl::MinifyingFilter::Nearest` criterion (the texture element closest to the specified texture coordinates) to produce a texture value. 
        NearestMipmapNearest = 0x2700,
        
        // Chooses the mipmap that most closely matches the size of the pixel being textured and
        // uses the `gl::MinifyingFilter::Linear` criterion (a weighted average of the four texture elements that are closest to the specified texture coordinates) to produce a texture value. 
        LinearMipmapNearest  = 0x2701,
        
        // Chooses the two mipmaps that most closely match the size of the pixel being textured and
        // uses the `gl::MinifyingFilter::Nearest` criterion (the texture element closest to the specified texture coordinates ) to produce a texture value from each mipmap. The final texture value is a weighted average of those two values. 
        NearestMipmapLinear  = 0x2702,
        
        // Chooses the two mipmaps that most closely match the size of the pixel being textured and
        // uses the `gl::MinifyingFilter::Linear` criterion (a weighted average of the texture elements that are closest to the specified texture coordinates) to produce a texture value from each mipmap. The final texture value is a weighted average of those two values. 
        LinearMipmapLinear   = 0x2703,
    };

    // The texture magnification function. Used whenever the level-of-detail function used when
    // sampling from the texture determines that the texture should be magified.
    // It sets the texture magnification function to either `gl::MagnifyingFilter::Nearest` or `gl::MagnifyingFilter::Linear` (see below).
    // `gl::MagnifyingFilter::Nearest` is generally faster than `gl::MagnifyingFilter::Linear`,
    // but it can produce textured images with sharper edges because the transition between texture elements is not as smooth.
    // The initial value is `Linear`. 
    enum class MagnifyingFilter : io::u32 {
        // Returns the value of the texture element that is nearest (in Manhattan distance) to the specified texture coordinates. 
        Nearest = 0x2600,

        // Returns the weighted average of the texture elements that are closest to the specified texture coordinates.
        // These can include items wrapped or repeated from other parts of a texture, depending on the values of `gl::TexParam::WrapS` and `gl::TexParam::WrapT`, and on the exact mapping. 
        Linear = 0x2601,
    };

    enum class PrimitiveMode : io::u32 {
        Triangles = 0x0004,
        TriangleStrip = 0x0005,
        Lines = 0x0001,
        LineStrip = 0x0003,
        Points = 0x0000,
        // others
    };

    enum class DrawElementsType : io::u32 {
        Byte = 0x1400,
        UnsignedByte = 0x1401,
        Short = 0x1402,
        UnsignedShort = 0x1403,
        Int = 0x1404,
        UnsignedInt = 0x1405,
        Float = 0x1406,
    };
    IO_NODISCARD IO_CONSTEXPR
        io::u32 getDrawElementsTypeSize(DrawElementsType type) noexcept {
        switch (type) {
        case DrawElementsType::Byte:          IO_FALLTHROUGH;
        case DrawElementsType::UnsignedByte:  return 1;
        case DrawElementsType::Short:         IO_FALLTHROUGH;
        case DrawElementsType::UnsignedShort: return 2;
        case DrawElementsType::Int:           IO_FALLTHROUGH;
        case DrawElementsType::UnsignedInt:   IO_FALLTHROUGH;
        case DrawElementsType::Float:         IO_FALLTHROUGH;
        default:                              return 4;
        }
    }

    enum class BufferTarget : io::u32 {
        ArrayBuffer = 0x8892,
        ElementArrayBuffer = 0x8893,
        UniformBuffer = 0x8A11,
        // others
    };

    enum class BufferUsage : io::u32 {
        StaticDraw = 0x88E4,
        DynamicDraw = 0x88E8,
        StreamDraw = 0x88E0
    };

    enum class ShaderType : io::u32 {
        VertexShader = 0x8B31,
        FragmentShader = 0x8B30,
        // others
    };

    enum class ProgramProperty : io::u32 {
        LinkStatus = 0x8B82,
        InfoLogLength = 0x8B84
    };

    enum class ShaderProperty : io::u32 {
        CompileStatus = 0x8B81,
        InfoLogLength = 0x8B84
    };
} // namespace gl

#if IO_GL_API_CORE >= 200
// ----------------------------------------------------------------------------
// GL_VERSION_1_1    // Core state
// ----------------------------------------------------------------------------
    GL_CALL_CUSTOM(GetError, int, (), (), ())
    GL_CALL_CUSTOM(GetIntegerv, void, (io::u32, int*), (io::u32 pname, int* data), (pname, data))
    GL_CALL_CUSTOM(CullFace,    void, (io::u32),       (Face face),                (static_cast<io::u32>(face)))
    
    // NOT SUPPORTED by OpenGL ES
    GL_CALL_CUSTOM(PolygonMode, void,
        /* proc   T */ (io::u32, io::u32),
        /*     decl */ (Face face, Polygon mode),
        /*     pass */ (static_cast<io::u32>(face), static_cast<io::u32>(mode)))

    GL_CALL_CUSTOM(TexParameterf, void,
        /* proc   T */ (io::u32, io::u32, float),
        /*     decl */ (TexTarget target, TexParam pname, float param),
        /*     pass */ (static_cast<io::u32>(target), static_cast<io::u32>(pname), param))

    
    /* 1a */ GL_CALL_CUSTOM(TexParameteri, void, (io::u32, io::u32, int),
        /*     decl */ (TexTarget target, TexParam pname, int param),
        /*     pass */ (static_cast<io::u32>(target), static_cast<io::u32>(pname), param))
    /* 1b */ GL_CALL_OVERLOAD(TexParameteri, void, (io::u32, io::u32, int),
        /*     decl */ (TexTarget target, TexParam pname, MagnifyingFilter mag_mode),
        /*     pass */ (static_cast<io::u32>(target), static_cast<io::u32>(pname), static_cast<int>(mag_mode)))
    /* 1c */ GL_CALL_OVERLOAD(TexParameteri, void, (io::u32, io::u32, int),
        /*     decl */ (TexTarget target, TexParam pname, MinifyingFilter min_mode),
        /*     pass */ (static_cast<io::u32>(target), static_cast<io::u32>(pname), static_cast<int>(min_mode)))
    /* 1d */ GL_CALL_OVERLOAD(TexParameteri, void, (io::u32, io::u32, int),
        /*     decl */ (TexTarget target, TexParam pname, TexWrap wrap_mode),
        /*     pass */ (static_cast<io::u32>(target), static_cast<io::u32>(pname), static_cast<int>(wrap_mode)))

    GL_CALL_CUSTOM(PixelStorei, void, (io::u32, int), (io::u32 pname, int param), (pname, param))

    /* 1a */ GL_CALL_CUSTOM(TexImage2D, void,
        /* proc   T */ (io::u32, int,int,int,int,int, io::u32, io::u32, const void*),
        /*     decl */ (TexTarget target,
                        int level, InternalFormat internalformat, int width, int height, int border,
                        TexFormat format, DataType type, const void* pixels),
        /*     pass */ (static_cast<io::u32>(target),
                        level, static_cast<int>(internalformat), width, height, border,
                        static_cast<io::u32>(format), static_cast<io::u32>(type), pixels))
    /* 1b */ GL_CALL_OVERLOAD(TexImage2D, void,
        /* proc   T */ (io::u32, int,int,int,int,int, io::u32, io::u32, const void*),
        /*     decl */ (TexTarget target,
                        int level, int internalformat, int width, int height, int border,
                        TexFormat format, DataType type, const void* pixels),
        /*     pass */ (static_cast<io::u32>(target),
                        level, internalformat, width, height, border,
                        static_cast<io::u32>(format), static_cast<io::u32>(type), pixels))


    // ----------------------------------------------------------------------------
    // Usage in the render loop:
    //
    //   // 1. set background color
    //   gl::ClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    //   // 2. clear color and depth buffers
    //   gl::Clear(gl::BufferBit.Color | gl::BufferBit.Depth);
    //   // 3. after this you can draw your scene geometry
    //
    // The function takes a bit-mask (e.g. Color | Depth)
    // and clears the corresponding buffers before rendering.
    //
    // In the rendering pipeline, clearing is the first step
    // before drawing a new frame. If you don't clear the buffers:
    //
    //   • The color buffer may hold pixels from the previous frame;
    //   • The depth buffer may contain old depth data, causing new objects to be
    //     incorrectly hidden or mis-sorted by Z;
    //   • The stencil buffer may contain leftover stencil masks.
    // ----------------------------------------------------------------------------
    GL_CALL_CUSTOM(Clear, void, (io::u32), (io::u32 mask), (mask))
    GL_CALL_CUSTOM(ClearColor, void, (float,float,float,float), (float r,float g,float b,float a),  (r, g, b, a))
    GL_CALL_CUSTOM(Disable, void, (io::u32), (Capability cap), (static_cast<io::u32>(cap)))
    GL_CALL_CUSTOM(Enable,  void, (io::u32), (Capability cap), (static_cast<io::u32>(cap)))
    GL_CALL_CUSTOM(BlendFunc, void, (io::u32, io::u32), (BlendFactor sfactor, BlendFactor dfactor),
        /*     pass */ (static_cast<io::u32>(sfactor), static_cast<io::u32>(dfactor)))

    GL_CALL_CUSTOM(GetFloatv, void, (io::u32, float*), (GetParam pname, float* data), (static_cast<io::u32>(pname), data))
    GL_CALL_CUSTOM(GetString, const unsigned char*, (io::u32), (io::u32 name), (name))
    GL_CALL_CUSTOM(Viewport, void, (int,int,int,int), (int x, int y, int width, int height), (x, y, width, height))
    GL_CALL_CUSTOM(DrawArrays, void, (io::u32, int, int), (PrimitiveMode mode, int first, int count), (static_cast<io::u32>(mode), first, count))
    GL_CALL_CUSTOM(DrawElements, void, (io::u32, int, io::u32, const void*),
        /*     decl */ (PrimitiveMode mode,            int count,
                        DrawElementsType type, const void* indices),
        /*     pass */ (static_cast<io::u32>(mode), count, static_cast<io::u32>(type), indices))

    GL_CALL_CUSTOM(BindTexture, void, (io::u32, io::u32), (TexTarget target, io::u32 texture), (static_cast<io::u32>(target), texture))
    GL_CALL_CUSTOM(DeleteTextures, void, (int, const io::u32*), (int n, const io::u32* textures), (n, textures))
    GL_CALL_CUSTOM(GenTextures, void, (int, io::u32*), (int n, io::u32* textures), (n, textures))

// ----------------------------------------------------------------------------
// GL_VERSION_1_3
// ----------------------------------------------------------------------------
    GL_CALL_CUSTOM(ActiveTexture, void, (io::u32), (TexUnit unit), (static_cast<io::u32>(unit)))

// ----------------------------------------------------------------------------
// GL_VERSION_1_5
// ----------------------------------------------------------------------------
    GL_CALL_CUSTOM(BindBuffer, void, (io::u32, io::u32), (BufferTarget target, io::u32 buffer), (static_cast<io::u32>(target), buffer))
    GL_CALL_CUSTOM(DeleteBuffers, void, (int, const io::u32*), (int n, const io::u32* buffers), (n, buffers))
    GL_CALL_CUSTOM(GenBuffers, void, (int, io::u32*), (int n, io::u32* buffers), (n, buffers))
    GL_CALL_CUSTOM(BufferData, void, /* proc   T */ (io::u32, intptr_t, const void*, io::u32),
        /*     decl */ (BufferTarget target, intptr_t size, const void* data, BufferUsage usage),
        /*     pass */ (static_cast<io::u32>(target), size, data, static_cast<io::u32>(usage)))

    GL_CALL_CUSTOM(BufferSubData, void, /* proc   T */ (io::u32, intptr_t, intptr_t, const void*),
        /*     decl */ (BufferTarget target, intptr_t offset, intptr_t size, const void* data),
        /*     pass */ (static_cast<io::u32>(target), offset, size, data))

// ----------------------------------------------------------------------------
// GL_VERSION_2_0
// ----------------------------------------------------------------------------
    GL_CALL_CUSTOM(AttachShader, void, (io::u32, io::u32), (io::u32 program, io::u32 shader), (program, shader))
    GL_CALL_CUSTOM(CompileShader, void, (io::u32), (io::u32 shader), (shader))
    GL_CALL_CUSTOM(CreateProgram, io::u32, (void), (), ())
    GL_CALL_CUSTOM(CreateShader, io::u32, (io::u32), (ShaderType type), (static_cast<io::u32>(type)))
    GL_CALL_CUSTOM(DeleteProgram, void, (io::u32), (io::u32 program), (program))
    GL_CALL_CUSTOM(DeleteShader, void, (io::u32), (io::u32 shader), (shader))
    GL_CALL_CUSTOM(EnableVertexAttribArray, void, (io::u32), (io::u32 index), (index))
    GL_CALL_CUSTOM(GetProgramiv, void, (io::u32, io::u32, int*),
        /*     decl */ (io::u32 program, ProgramProperty pname, int* params),
        /*     pass */ (program, static_cast<io::u32>(pname), params))
    
    GL_CALL_CUSTOM(GetProgramInfoLog, void, (io::u32, int, int*, char*),
        /*     decl */ (io::u32 program, int bufSize, int* length, char* infoLog),
        /*     pass */ (program, bufSize, length, infoLog))
    
    GL_CALL_CUSTOM(GetShaderiv, void, (io::u32, io::u32, int*),
        /*     decl */ (io::u32 shader, ShaderProperty pname, int* params),
        /*     pass */ (shader, static_cast<io::u32>(pname), params))
    
    GL_CALL_CUSTOM(GetShaderInfoLog, void, (io::u32, int, int*, char*),
        /*     decl */ (io::u32 shader, int bufSize, int* length, char* infoLog),
        /*     pass */ (shader, bufSize, length, infoLog))
    
    GL_CALL_CUSTOM(GetUniformLocation, int, (io::u32, const char*), (io::u32 program, const char* name), (program, name))
    GL_CALL_CUSTOM(LinkProgram, void, (io::u32), (io::u32 program), (program))
    GL_CALL_CUSTOM(ShaderSource, void, (io::u32, int, const char* const*, const int*),
        /*     decl */ (io::u32 shader, int count, const char* const* strings, const int* lengths),
        /*     pass */ (shader, count, strings, lengths))
    
    GL_CALL_CUSTOM(UseProgram, void, (io::u32), (io::u32 program), (program))
    
    GL_CALL_CUSTOM(Uniform1i, void, (int, int), (int location, int v0), (location, v0))
    GL_CALL_CUSTOM(Uniform1f, void, (int, float), (int location, float v0), (location, v0))
    GL_CALL_CUSTOM(Uniform2f, void, (int, float, float), (int location, float v0, float v1), (location, v0, v1))
    GL_CALL_CUSTOM(UniformMatrix4fv, void, (int, int, unsigned char, const float*),
        /*     decl */ (int location, int count, bool transpose, const float* value),
        /*     pass */ (location, count, static_cast<unsigned char>(transpose), value))
    
    GL_CALL_CUSTOM(VertexAttribPointer, void, (io::u32, int, io::u32, unsigned char, int, const void*),
        /*     decl */ (io::u32 index, int size, DrawElementsType type, bool normalized, int stride, const void* pointer),
        /*     pass */ (index, size, static_cast<io::u32>(type), static_cast<unsigned char>(normalized), stride, pointer))
#endif // Core 200

#if IO_GL_API_CORE >= 300
// ----------------------------------------------------------------------------
// GL_VERSION_3_0_0
// ----------------------------------------------------------------------------

    GL_CALL_CUSTOM(BindBufferBase, void, (io::u32, io::u32, io::u32),
        /*     decl */ (BufferTarget target, io::u32 index, io::u32 buffer),
        /*     pass */ (static_cast<io::u32>(target), index, buffer))
    GL_CALL_CUSTOM(VertexAttribIPointer, void, (io::u32, int, io::u32, int, const void*),
        /*     decl */ (io::u32 index, int size, DrawElementsType type, int stride, const void* pointer),
        /*     pass */ (index, size, static_cast<io::u32>(type), stride, pointer))
    GL_CALL_CUSTOM(BindVertexArray, void, (io::u32), (io::u32 _array), (_array))
    GL_CALL_CUSTOM(DeleteVertexArrays, void, (int, const io::u32*), (int n, const io::u32* arrays), (n, arrays))
    GL_CALL_CUSTOM(GenVertexArrays, void, (int, io::u32*), (int n, io::u32* arrays), (n, arrays))
#endif // Core 3.0.0

namespace gl {

    // returns 0,0 if query unsupported (old GL), then fallback to GL_VERSION string parse (optional)
    static inline void query_core_version(int& maj, int& min) noexcept {
        maj = 0; min = 0;
        // safe even if pointers missing? -> require load of GetIntegerv first
        gl::GetIntegerv(GlVersionParam::major, &maj);
        gl::GetIntegerv(GlVersionParam::minor, &min);
    } // query_core_version

    // Load minimum set depending on *real* desktop GL core version.
    IO_NODISCARD static inline io::char_view load_core(int core_major, int core_minor) noexcept {
        (void)core_minor;
        if (!loader) return io::char_view{"loader(null)"};

        auto must_load = [](auto& out, const char* name) noexcept -> bool {
            using FnT = decltype(out);
            void* p = ::gl::loader(name);
            out = reinterpret_cast<FnT>(p);
            return out != nullptr;
        };

#define IO_GL_LOAD(Name) \
        do { if (!must_load(native::p##Name, "gl" #Name)) return io::char_view{"gl" #Name}; } while(0)

        // ---- always needed (GL 2.x+) ----
        IO_GL_LOAD(GetError);
        IO_GL_LOAD(GetIntegerv);
        IO_GL_LOAD(GetString);
        IO_GL_LOAD(Clear);
        IO_GL_LOAD(ClearColor);
        IO_GL_LOAD(Viewport);

        IO_GL_LOAD(Enable);
        IO_GL_LOAD(Disable);
        IO_GL_LOAD(BlendFunc);
        IO_GL_LOAD(CullFace);
        IO_GL_LOAD(PolygonMode);

        IO_GL_LOAD(GenBuffers);
        IO_GL_LOAD(BindBuffer);
        IO_GL_LOAD(BufferData);
        IO_GL_LOAD(BufferSubData);
        IO_GL_LOAD(DeleteBuffers);

        IO_GL_LOAD(ActiveTexture);
        IO_GL_LOAD(GenTextures);
        IO_GL_LOAD(BindTexture);
        IO_GL_LOAD(TexParameteri);
        IO_GL_LOAD(PixelStorei);
        IO_GL_LOAD(TexParameterf);
        IO_GL_LOAD(TexImage2D);
        IO_GL_LOAD(DeleteTextures);

        IO_GL_LOAD(CreateShader);
        IO_GL_LOAD(ShaderSource);
        IO_GL_LOAD(CompileShader);
        IO_GL_LOAD(GetShaderiv);
        IO_GL_LOAD(GetShaderInfoLog);
        IO_GL_LOAD(DeleteShader);

        IO_GL_LOAD(CreateProgram);
        IO_GL_LOAD(AttachShader);
        IO_GL_LOAD(LinkProgram);
        IO_GL_LOAD(GetProgramiv);
        IO_GL_LOAD(GetProgramInfoLog);
        IO_GL_LOAD(UseProgram);
        IO_GL_LOAD(DeleteProgram);

        IO_GL_LOAD(GetUniformLocation);
        IO_GL_LOAD(Uniform1i);
        IO_GL_LOAD(Uniform1f);
        IO_GL_LOAD(Uniform2f);
        IO_GL_LOAD(UniformMatrix4fv);

        IO_GL_LOAD(EnableVertexAttribArray);
        IO_GL_LOAD(VertexAttribPointer);
        IO_GL_LOAD(DrawArrays);
        IO_GL_LOAD(DrawElements);

        // ---- only if core >= 3 ----
        if (core_major >= 3) {
            IO_GL_LOAD(GenVertexArrays);
            IO_GL_LOAD(BindVertexArray);
            IO_GL_LOAD(DeleteVertexArrays);
            IO_GL_LOAD(BindBufferBase);
            IO_GL_LOAD(VertexAttribIPointer);
        }

#undef IO_GL_LOAD
        loaded = true;
        loaded_major = (io::u8)core_major;
        loaded_minor = (io::u8)core_minor;
        return {};
    } // load_core

// ----------------------------------------------------------------------------
//                          Higl-level GL API
// ----------------------------------------------------------------------------


#if IO_GL_API_CORE >= 200
    struct Shader {
    private:
        io::u32 _id_program{ 0 };

    public:
        // Check compile status with `.failed()`
        // When `#define _DEBUG` automatically prints errors via glGetShaderInfoLog
        inline bool Compile(const char* vert, const char* frag) noexcept {
            int success;
            char info[512];
            using namespace gl;
            io::u32 V; // 1. Vertex shader
            io::u32 F; // 2. Fragment shader
            io::u32 P; // 3. shader Program
            // 1. vertex shader
            V = CreateShader(ShaderType::VertexShader);
            ShaderSource(V, 1, &vert, nullptr);
            CompileShader(V);
            GetShaderiv(V, ShaderProperty::CompileStatus, &success);
            if (!success) {
                GetShaderInfoLog(V, 512, nullptr, info);
                io::out << "shader compile failed: " << info << '\n';
                DeleteShader(V);
                return false;
            }
            // 2. fragment shader
            F = CreateShader(ShaderType::FragmentShader);
            ShaderSource(F, 1, &frag, nullptr);
            CompileShader(F);
            GetShaderiv(F, ShaderProperty::CompileStatus, &success);
            if (!success) {
                GetShaderInfoLog(F, 512, nullptr, info);
                io::out << "fshader compile failed: " << info << '\n';
                DeleteShader(V);
                DeleteShader(F);
                return false;
            }
            // 3. shader program
            P = CreateProgram();
            AttachShader(P, V);
            AttachShader(P, F);
            LinkProgram(P);
            GetProgramiv(P, ProgramProperty::LinkStatus, &success);
            if (!success) {
                GetProgramInfoLog(P, 512, nullptr, info);
                ::io::out << "shader link failed: " << info << '\n';
                DeleteShader(V);
                DeleteShader(F);
                DeleteProgram(P);
                return false;
            }
            // We no longer need shader objects after linking
            DeleteShader(V);
            DeleteShader(F);
            _id_program = P;
            // Freeing the shader program, using RAII, in the destructor.
            return true;
        }

        inline explicit Shader(io::u32 ready_program) noexcept : _id_program{ ready_program } {}
        inline explicit Shader() noexcept = default;
        inline explicit Shader(const char* vert, const char* frag) noexcept { Compile(vert, frag); }
        inline ~Shader() noexcept { gl::DeleteProgram(_id_program); }

        // Non-copyable, non-movable
        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;
        Shader(Shader&&) = delete;
        Shader& operator=(Shader&&) = delete;

        inline io::u32 id() const noexcept { return _id_program; }
        inline bool failed() const noexcept { return _id_program == 0; }

        inline void Use() const noexcept { gl::UseProgram(_id_program); }

        // caller must gl::DeleteShader
        static inline bool compile_shader(io::u32& out_sh, gl::ShaderType type, const char* src) noexcept {
            out_sh = gl::CreateShader(type);
            if (!out_sh) return false;
            gl::ShaderSource(out_sh, 1, &src, nullptr);
            gl::CompileShader(out_sh);

            int ok = 0;
            gl::GetShaderiv(out_sh, gl::ShaderProperty::CompileStatus, &ok);
            if (!ok) {
                char log[512]{};
                gl::GetShaderInfoLog(out_sh, (int)sizeof(log)-1, nullptr, log);
                io::out << "shader compile failed: " << log << '\n';
                gl::DeleteShader(out_sh);
                out_sh = 0;
                return false;
            }
            return true;
        }

        static inline bool link_program(io::u32& out_prog, io::u32 vs, io::u32 fs) noexcept {
            out_prog = gl::CreateProgram();
            if (!out_prog) return false;
            gl::AttachShader(out_prog, vs);
            gl::AttachShader(out_prog, fs);
            gl::LinkProgram(out_prog);
        
            int ok = 0;
            gl::GetProgramiv(out_prog, gl::ProgramProperty::LinkStatus, &ok);
            if (!ok) {
                char log[512]{};
                gl::GetProgramInfoLog(out_prog, (int)sizeof(log)-1, nullptr, log);
                io::out << "program link failed: " << log << '\n';
                gl::DeleteProgram(out_prog);
                out_prog = 0;
                return false;
            }
            return true;
        }
}; // struct Shader

struct Buffer {
private:
    unsigned _id{ 0 };
    gl::BufferTarget _target;

public:
    IO_CONSTEXPR Buffer() noexcept = delete;
    inline explicit Buffer(gl::BufferTarget target) noexcept : _target(target) { gl::GenBuffers(1, &_id); }
    inline ~Buffer() noexcept { if (_id) gl::DeleteBuffers(1, &_id); }
    inline void bind() const noexcept { BindBuffer(_target, _id); }
    inline void data(const void* ptr, io::usize bytes, gl::BufferUsage usage) const noexcept { BufferData(_target, bytes, ptr, usage); }
    inline unsigned id() const noexcept { return _id; }

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&& o) noexcept { _id=o._id; _target=o._target; o._id=0; }
    Buffer& operator=(Buffer&& o) noexcept {
        if (this != &o) {
            if (_id) gl::DeleteBuffers(1, &_id);
            _id=o._id; _target=o._target; o._id=0;
        }
        return *this;
    }
};
#endif // Core 2.0.0

#if IO_GL_API_CORE >= 300
struct VertexArray {
private:
    unsigned _id{ 0 };

public:
    inline VertexArray() noexcept { GenVertexArrays(1, &_id); }
    inline ~VertexArray() noexcept { if (_id) DeleteVertexArrays(1, &_id); }

    inline void bind() const noexcept { BindVertexArray(_id); }
    inline unsigned id() const noexcept { return _id; }

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    VertexArray(VertexArray&& o) noexcept {
        _id = o._id;
        o._id = 0;
    }

    VertexArray& operator=(VertexArray&& o) noexcept {
        if (this != &o) {
            if (_id) DeleteVertexArrays(1, &_id);
            _id = o._id;
            o._id = 0;
        }
        return *this;
    }
};

template<typename T> struct DrawElementsTypeOf;
template<> struct DrawElementsTypeOf<io::i8> { static IO_CONSTEXPR_VAR DrawElementsType value = DrawElementsType::Byte; };
template<> struct DrawElementsTypeOf<io::u8> { static IO_CONSTEXPR_VAR DrawElementsType value = DrawElementsType::UnsignedByte; };
template<> struct DrawElementsTypeOf<io::i16> { static IO_CONSTEXPR_VAR DrawElementsType value = DrawElementsType::Short; };
template<> struct DrawElementsTypeOf<io::u16> { static IO_CONSTEXPR_VAR DrawElementsType value = DrawElementsType::UnsignedShort; };
template<> struct DrawElementsTypeOf<io::i32> { static IO_CONSTEXPR_VAR DrawElementsType value = DrawElementsType::Int; };
template<> struct DrawElementsTypeOf<io::u32> { static IO_CONSTEXPR_VAR DrawElementsType value = DrawElementsType::UnsignedInt; };
template<> struct DrawElementsTypeOf<float> { static IO_CONSTEXPR_VAR DrawElementsType value = DrawElementsType::Float; };

struct Attr {
private:
    io::u32 _amount;
    DrawElementsType _type;
    bool _normalize;

public:
    // amount_of_components e.g. 3 for vec3
    // gl_type: Byte, UnsignedByte, Short, UnsignedShort, Int, UnsignedInt, Float.    
    IO_CONSTEXPR Attr(io::u32 amount_of_components, DrawElementsType gl_type = DrawElementsType::Float, bool normalize = false) noexcept
        : _amount{ amount_of_components }, _type{ gl_type }, _normalize{ normalize } {}

    // amount of components, e.g. 3 for vec3
    IO_NODISCARD IO_CONSTEXPR io::u32 amount() const noexcept { return _amount; }
    IO_NODISCARD IO_CONSTEXPR DrawElementsType type() const noexcept { return _type; }
    IO_NODISCARD IO_CONSTEXPR bool normalized() const noexcept { return _normalize; }
    IO_NODISCARD IO_CONSTEXPR int size() const noexcept {
        return amount() * getDrawElementsTypeSize(type()); // e.g. `3 * sizeof(float)`
    }
};

template<typename T>
IO_CONSTEXPR const Attr AttrOf(io::u32 amount) noexcept {
    return Attr(amount, DrawElementsTypeOf<T>::value);
}
template<typename T>
IO_CONSTEXPR const Attr AttrOf(io::u32 amount, bool normalize) noexcept {
    return Attr(amount, DrawElementsTypeOf<T>::value, normalize);
}

struct MeshBinder {
    inline static void setup(
        const VertexArray& vao,
        const Buffer& vbo,
        const io::view<const Attr>& attrs
    ) noexcept {
        io::u32 index, stride;
        io::usize offset = 0;
        index = stride = 0;

        vao.bind();
        vbo.bind();

        // Calculate stride, by iterating all elements
        for (auto it = attrs.begin(); it != attrs.end(); ++it)
            stride += it->size();

        // Call glFunctions for each Attr
        for (auto it = attrs.begin(); it != attrs.end(); ++it, ++index) {
            VertexAttribPointer(
                index,        // location
                it->amount(), // amount of comps
                it->type(),   // gl type
                it->normalized(),
                stride,       // stride
                reinterpret_cast<void*>(offset) // attr offset
            );
            EnableVertexAttribArray(index);
            offset += it->size();
        }

        BindVertexArray(0);
        BindBuffer(BufferTarget::ArrayBuffer, 0);
    }
}; // struct MeshBinder
#endif // Core 3.0.0



} // namespace gl