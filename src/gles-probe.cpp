// SPDX-License-Identifier: MIT
#include <EGL/egl.h>
#include <GLES3/gl31.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

static void require(bool condition, const char *operation)
{
    if (!condition) {
        std::fprintf(stderr, "%s failed (EGL=0x%x GL=0x%x)\n", operation,
                     eglGetError(), glGetError());
        std::exit(1);
    }
}

static GLuint compile(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        char log[4096]{};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::fputs(log, stderr);
    }
    require(compiled, "shader compilation");
    return shader;
}

static void link(GLuint program)
{
    glLinkProgram(program);
    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[4096]{};
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::fputs(log, stderr);
    }
    require(linked, "program link");
    glUseProgram(program);
}

static void render(const char *label, const char *source, bool verify_color)
{
    constexpr const char *vertex_source = R"(#version 300 es
void main() {
    vec2 positions[3] = vec2[3](vec2(-1,-1), vec2(3,-1), vec2(-1,3));
    gl_Position = vec4(positions[gl_VertexID], 0, 1);
})";
    GLuint vertex = compile(GL_VERTEX_SHADER, vertex_source);
    GLuint fragment = compile(GL_FRAGMENT_SHADER, source);
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    link(program);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    unsigned char pixel[4]{};
    glReadPixels(293, 89, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    require(glGetError() == GL_NO_ERROR, "rasterization/readback");
    std::printf("%s pixel=%u,%u,%u,%u\n", label, pixel[0], pixel[1], pixel[2], pixel[3]);
    if (verify_color) {
        constexpr int expected[] = {64, 128, 191, 255};
        for (unsigned i = 0; i < 4; ++i)
            require(std::abs(int(pixel[i]) - expected[i]) <= 1, "color pixel");
    }
    glUseProgram(0);
    glDeleteProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

int main()
{
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    require(display != EGL_NO_DISPLAY && eglInitialize(display, nullptr, nullptr), "EGL initialize");
    require(eglBindAPI(EGL_OPENGL_ES_API), "bind GLES");
    const EGLint attributes[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_NONE};
    EGLConfig config{};
    EGLint count = 0;
    require(eglChooseConfig(display, attributes, &config, 1, &count) && count == 1, "EGL config");
    const EGLint dimensions[] = {EGL_WIDTH, 800, EGL_HEIGHT, 600, EGL_NONE};
    EGLSurface surface = eglCreatePbufferSurface(display, config, dimensions);
    const EGLint version[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, version);
    require(surface != EGL_NO_SURFACE && context != EGL_NO_CONTEXT &&
            eglMakeCurrent(display, surface, surface, context), "GLES context");
    const char *renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    require(renderer != nullptr, "renderer query");
    std::printf("renderer=%s version=%s\n", renderer, glGetString(GL_VERSION));
    if (std::strstr(renderer, "llvmpipe") || std::strstr(renderer, "softpipe") ||
        std::strstr(renderer, "SwiftShader")) {
        std::fputs("software renderer cannot qualify hardware\n", stderr);
        return 2;
    }
    GLint major = 0, minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    require(major > 3 || (major == 3 && minor >= 1), "GLES 3.1 compute availability");
    for (GLenum precision : {GL_MEDIUM_FLOAT, GL_HIGH_FLOAT}) {
        GLint range[2]{}, bits = 0;
        glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, precision, range, &bits);
        std::printf("fragment_precision=0x%x range=%d,%d bits=%d\n", precision, range[0], range[1], bits);
    }
    glViewport(0, 0, 800, 600);
    render("constant-color", "#version 300 es\nprecision mediump float;\nout vec4 color;\n"
           "void main(){ color=vec4(0.25,0.5,0.75,1.0); }", true);
    for (const char *precision : {"mediump", "highp"}) {
        for (bool conditional : {false, true}) {
            std::string source = "#version 300 es\nprecision ";
            source += precision;
            source += " float;\nout vec4 color;\nvoid main(){\n"
                "highp vec2 coord=gl_FragCoord.xy;\n"
                "float d=fract(coord.x*coord.y*0.0001);\nfor(int i=0;i<5;++i){\n";
            source += conditional ? "d=fract((d>=0.5?2.0:3.0)*d);" : "d=fract(3.0*d);";
            source += "}\ncolor=vec4(d,d,d,1.0);}";
            std::string label = std::string(precision) + (conditional ? "-conditional" : "-loop");
            render(label.c_str(), source.c_str(), false);
        }
    }
    constexpr const char *compute_source = R"(#version 310 es
layout(local_size_x=64) in;
layout(std430,binding=0) buffer Result { uint words[]; };
void main(){ uint i=gl_GlobalInvocationID.x; words[i]=i*3u+7u; }
)";
    GLuint compute = compile(GL_COMPUTE_SHADER, compute_source);
    GLuint program = glCreateProgram();
    glAttachShader(program, compute);
    link(program);
    uint32_t zeros[1024]{};
    GLuint buffer = 0;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(zeros), zeros, GL_DYNAMIC_READ);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
    glDispatchCompute(16, 1, 1);
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
    GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    require(fence != nullptr, "compute fence");
    GLenum waited = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 5000000000ULL);
    require(waited == GL_ALREADY_SIGNALED || waited == GL_CONDITION_SATISFIED, "compute wait (5 seconds)");
    auto *words = static_cast<const uint32_t *>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(zeros), GL_MAP_READ_BIT));
    require(words != nullptr, "compute mapping");
    for (uint32_t i = 0; i < 1024; ++i)
        require(words[i] == i * 3 + 7, "compute word");
    require(glUnmapBuffer(GL_SHADER_STORAGE_BUFFER) && glGetError() == GL_NO_ERROR, "compute unmap");
    glDeleteSync(fence);
    glDeleteBuffers(1, &buffer);
    glUseProgram(0);
    glDeleteProgram(program);
    glDeleteShader(compute);
    require(eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT), "release context");
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);
    std::puts("PASS: color rasterization and 1024-word GLES compute readback; precision values are diagnostic, not conformance");
}
