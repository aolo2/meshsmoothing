#include <glad/glad.h>
#include <GLFW/glfw3.h>

static const char *vs_source = "#version 330 core\n"
"layout (location = 0) in vec3 pos;\n"
"uniform mat4 model;\n"
"uniform mat4 proj;\n"
"void main()\n"
"{\n"
"   gl_Position = proj * model * vec4(pos.x, pos.y, pos.z, 1.0);\n"
"}";

static const char *fs_source = "#version 330 core\n"
"out vec4 frag_color;\n"
"void main()\n"
"{\n"
"   frag_color = vec4(0.7f);\n"
"}\n";


static struct ms_v3 *
_quad_to_triangle(struct ms_mesh mesh, u32 *mesh_bytes)
{
    struct ms_v3 *triangle_verts = malloc(mesh.primitives * 6 * sizeof(struct ms_v3));
    *mesh_bytes = mesh.primitives * 6 * 3 * sizeof(f32);
    
    for (u32 face = 0; face < mesh.primitives; ++face) {
        /* ABCD quad -> two triangles ABC and CDA */
        
        /* ABC */
        triangle_verts[face * 6 + 0] = mesh.vertices[face * 4 + 0];
        triangle_verts[face * 6 + 1] = mesh.vertices[face * 4 + 1];
        triangle_verts[face * 6 + 2] = mesh.vertices[face * 4 + 2];
        
        /* CDA */
        triangle_verts[face * 6 + 3] = mesh.vertices[face * 4 + 2];
        triangle_verts[face * 6 + 4] = mesh.vertices[face * 4 + 3];
        triangle_verts[face * 6 + 5] = mesh.vertices[face * 4 + 0];
    }
    
    return(triangle_verts);
}

static GLFWwindow *
ms_opengl_init(u32 width, u32 height)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 8);
    
    GLFWwindow *window = glfwCreateWindow(width, height, "Mesh smoothing", NULL, NULL);
    
    if (!window) {
        return(NULL);
    }
    
    glfwMakeContextCurrent(window);
    
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        return(NULL);
    }
    
    return(window);
}

static struct ms_gl_bufs
ms_opengl_init_buffers(struct ms_mesh mesh)
{
    struct ms_gl_bufs result;
    
    glGenVertexArrays(1, &result.VAO);
    glGenBuffers(1, &result.VBO);
    
    glBindVertexArray(result.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, result.VBO);
    
    struct ms_v3 *triangle_verts = mesh.vertices;
    u32 mesh_bytes = mesh.primitives * mesh.degree * sizeof(struct ms_v3);
    
    if (mesh.degree == 4) {
        triangle_verts = _quad_to_triangle(mesh, &mesh_bytes);
    }
    
    glBufferData(GL_ARRAY_BUFFER, mesh_bytes, triangle_verts, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), NULL);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0); 
    
    return(result);
}

static void
ms_opengl_update_buffers(struct ms_gl_bufs bufs, struct ms_mesh mesh)
{
    glBindVertexArray(bufs.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, bufs.VBO);
    
    struct ms_v3 *triangle_verts = mesh.vertices;
    u32 mesh_bytes = mesh.primitives * 3 * 3 * sizeof(f32);
    
    if (mesh.degree == 4) {
        triangle_verts = _quad_to_triangle(mesh, &mesh_bytes);
    }
    
    glBufferData(GL_ARRAY_BUFFER, mesh_bytes, triangle_verts, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), NULL);
    glEnableVertexAttribArray(0);
    
    free(triangle_verts);
}

static void
_check_shader_error(GLuint shader, char info_log[512], char message_header[], GLenum stage)
{
    s32 success = 1;
    
    glGetShaderiv(shader, stage, &success);
    
    if (!success) {
        memset(info_log, 0x00, 512);
        glGetShaderInfoLog(shader, 512, NULL, info_log);
        fprintf(stderr, "[ERROR] %s: %s\n", message_header, info_log);
    }
}

static s32
ms_opengl_init_shader_program()
{
    char info_log[512];
    
    u32 vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vs_source, NULL);
    glCompileShader(vertex_shader);
    _check_shader_error(vertex_shader, info_log, "Vertex shader", GL_COMPILE_STATUS);
    
    u32 fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fs_source, NULL);
    glCompileShader(fragment_shader);
    _check_shader_error(fragment_shader, info_log, "Fragment shader", GL_COMPILE_STATUS);
    
    u32 shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    _check_shader_error(shader_program, info_log, "Shader linking", GL_LINK_STATUS);
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    printf("[INFO] Compiled shaders\n");
    
    return(shader_program);
}