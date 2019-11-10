#include "ms_common.h"

#include "ms_stl.c"
#include "ms_opengl.c"
#include "ms_math.c"
#include "ms_subdiv.c"

s32
main(void)
{
    struct ms_mesh mesh = ms_stl_read_file("res/icosphere.stl");
    struct ms_mesh subdiv = ms_subdiv_catmull_clark(mesh);
    
    (void) subdiv;
    
    GLFWwindow *window = ms_opengl_init(1280, 720);
    
    assert(window);
    
    u32 VAO = ms_opengl_init_buffers(mesh);
    s32 shader_program = ms_opengl_init_shader_program();
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glUseProgram(shader_program);
    glBindVertexArray(VAO);
    
    struct ms_m4 proj = ms_math_projection(1280.0f / 720.0f, M_PI / 2.0f, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "proj"), 1, GL_FALSE, (float *) proj.data);
    
    struct ms_m4 scale = ms_math_scale(0.5f);
    //struct ms_m4 translate = ms_math_translate(0.0f, 0.0f, 0.0f);
    struct ms_m4 model = scale;
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, (float *) model.data);
    
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glDrawArrays(GL_TRIANGLES, 0, mesh.triangles * 3);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    return(0);
}