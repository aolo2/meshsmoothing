#include "ms_common.h"

#include "ms_stl.c"
#include "ms_opengl.c"
#include "ms_math.c"
#include "ms_subdiv.c"

s32
main(void)
{
    struct ms_mesh mesh = ms_stl_read_file("res/cube.stl");
    struct ms_mesh subdiv_1 = ms_subdiv_catmull_clark(mesh);
    struct ms_mesh subdiv_2 = ms_subdiv_catmull_clark(subdiv_1);
    struct ms_mesh subdiv_3 = ms_subdiv_catmull_clark(subdiv_2);
    struct ms_mesh subdiv_4 = ms_subdiv_catmull_clark(subdiv_3);
    struct ms_mesh subdiv_5 = ms_subdiv_catmull_clark(subdiv_4);
    
    ms_stl_write_file(subdiv_1, "res/cube_sd_1.stl");
    ms_stl_write_file(subdiv_2, "res/cube_sd_2.stl");
    ms_stl_write_file(subdiv_3, "res/cube_sd_3.stl");
    ms_stl_write_file(subdiv_4, "res/cube_sd_4.stl");
    ms_stl_write_file(subdiv_5, "res/cube_sd_5.stl");
    
    GLFWwindow *window = ms_opengl_init(1280, 720);
    
    assert(window);
    
    u32 VAO = ms_opengl_init_buffers(mesh);
    s32 shader_program = ms_opengl_init_shader_program();
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glUseProgram(shader_program);
    glLineWidth(2.0f);
    glPointSize(2.0f);
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
        
        glDrawArrays(GL_POINTS, 0, mesh.primitives * mesh.degree);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    return(0);
}