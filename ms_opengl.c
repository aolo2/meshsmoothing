#include <glad/glad.h>
#include <GLFW/glfw3.h>

static const char *vs_source = "#version 330 core\n"
"layout (location = 0) in vec3 pos;\n"
"layout (location = 1) in vec3 normal;\n"
"uniform mat4 model;\n"
"uniform mat4 proj;\n"
"void main()\n"
"{\n"
"   gl_Position = proj * model * vec4(pos, 1.0);\n"
"}";

static const char *fs_source = "#version 330 core\n"
"out vec4 frag_color;\n"
"void main()\n"
"{\n"
"   frag_color = vec4(0.7f);\n"
"}\n";


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

static GLuint
ms_opengl_init_buffers(struct ms_mesh mesh)
{
    GLuint VAO, VBO;
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    
    glBufferData(GL_ARRAY_BUFFER, mesh.triangles * 12 * sizeof(f32), NULL, GL_STATIC_DRAW);
    
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    mesh.triangles * 9 * sizeof(f32),
                    mesh.vertices);
    
    glBufferSubData(GL_ARRAY_BUFFER,
                    mesh.triangles * 9 * sizeof(f32), 
                    mesh.triangles * 3 * sizeof(f32),
                    mesh.normals);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), NULL);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), (void *) (mesh.triangles * 9 * sizeof(f32)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0); 
    
    return(VAO);
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