#include "ms_common.h"

#include "ms_file.c"
#include "ms_opengl.c"
#include "ms_math.c"
#include "ms_subdiv.c"

static bool ms_update_mesh = false;
static bool ms_save_mesh   = false;
static bool ms_dec_scale   = false;
static bool ms_inc_scale   = false;

static void
key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ENTER) { ms_update_mesh = true; }
        if (key == GLFW_KEY_SPACE) { ms_save_mesh = true; }
        if (key == GLFW_KEY_DOWN)  { ms_dec_scale = true; }
        if (key == GLFW_KEY_UP)    { ms_inc_scale = true; }
    }
    
    (void) window;
    (void) scancode;
    (void) mods;
}

s32
main(s32 argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "[ERROR] Usage: %s path/to/model.stl\n", argv[0]);
        return(1);
    }
    
    printf("[INFO] Keymap\n");
    printf("\t Zoom in: UP ARROW\n");
    printf("\t Zoom out: DOWN ARROW\n");
    printf("\t Subdivide: ENTER\n");
    printf("\t Save to disk (interactive): SPACE\n");
    
    struct ms_mesh mesh = ms_file_obj_read_file(argv[1]);
    //struct ms_mesh mesh = {
    //.primitives = 6,
    //.degree = 4,
    //.vertices = malloc(6 * 4 * sizeof(struct ms_v3))
    //};
    
#if 0
    mesh.vertices[0] = (struct ms_v3) { -1, -1, -1 };
    mesh.vertices[1] = (struct ms_v3) { -1, -1,  1 };
    mesh.vertices[2] = (struct ms_v3) { -1,  1,  1 };
    mesh.vertices[3] = (struct ms_v3) { -1,  1, -1 };
    
    mesh.vertices[4] = (struct ms_v3) { 1, -1, -1 };
    mesh.vertices[5] = (struct ms_v3) { 1,  1, -1 };
    mesh.vertices[6] = (struct ms_v3) { 1,  1,  1 };
    mesh.vertices[7] = (struct ms_v3) { 1, -1,  1 };
    
    mesh.vertices[8]  = (struct ms_v3) { -1, -1, -1 };
    mesh.vertices[9]  = (struct ms_v3) {  1, -1, -1 };
    mesh.vertices[10] = (struct ms_v3) {  1, -1,  1 };
    mesh.vertices[11] = (struct ms_v3) { -1, -1,  1 };
    
    mesh.vertices[12] = (struct ms_v3) { -1, 1, -1 };
    mesh.vertices[13] = (struct ms_v3) { -1, 1,  1 };
    mesh.vertices[14] = (struct ms_v3) {  1, 1,  1 };
    mesh.vertices[15] = (struct ms_v3) {  1, 1, -1 };
    
    mesh.vertices[16] = (struct ms_v3) { -1, -1, -1 };
    mesh.vertices[17] = (struct ms_v3) { -1,  1, -1 };
    mesh.vertices[18] = (struct ms_v3) {  1,  1, -1 };
    mesh.vertices[19] = (struct ms_v3) {  1, -1, -1 };
    
    mesh.vertices[20] = (struct ms_v3) { -1, -1, 1 };
    mesh.vertices[21] = (struct ms_v3) {  1, -1, 1 };
    mesh.vertices[22] = (struct ms_v3) {  1,  1, 1 };
    mesh.vertices[23] = (struct ms_v3) { -1,  1, 1 };
#endif
    
    GLFWwindow *window = ms_opengl_init(1280, 720);
    glfwSetKeyCallback(window, key_callback);
    
    assert(window);
    
    struct ms_gl_bufs bufs = ms_opengl_init_buffers(mesh);
    s32 shader_program = ms_opengl_init_shader_program();
    
    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glUseProgram(shader_program);
    //    glLineWidth(2.0f);
    glCullFace(GL_FRONT);
    glPointSize(2.0f);
    glBindVertexArray(bufs.VAO);
    
    struct ms_m4 proj = ms_math_ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 10.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "proj"), 1, GL_FALSE, (float *) proj.data);
    
    u32 cc_step = 0;
    u32 frame = 0;
    f32 scale_factor = 1.0f;
    
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        if (ms_update_mesh) {
            struct ms_mesh new_mesh = ms_subdiv_catmull_clark(mesh);
            free(mesh.vertices);
            free(mesh.normals);
            mesh = new_mesh;
            ms_opengl_update_buffers(bufs, mesh);
            printf("[INFO] Finished Catmull-Clark [%d]\n", ++cc_step);
            ms_update_mesh = false;
        }
        
        if (ms_save_mesh) {
            char filename[512];
            printf("[SAVE] Filename: ");
            scanf("%s", filename);
            ms_file_stl_write_file(mesh, filename);
            ms_save_mesh = false;
        }
        
        if (ms_dec_scale) {
            scale_factor *= 0.9f;
            if (scale_factor < 0.1f) { scale_factor = 0.1f; }
            ms_dec_scale = false;
        }
        
        if (ms_inc_scale) {
            scale_factor *= 1.1f;
            if (scale_factor > 4.0f) { scale_factor = 4.0f; }
            ms_inc_scale = false;
        }
        
        struct ms_m4 rotate = ms_math_rot(0.0f, 1.0f, 0.0f, (f32) frame / 180.0f * M_PI);
        struct ms_m4 scale = ms_math_scale(scale_factor);
        struct ms_m4 model = ms_math_mm(scale, rotate);
        
        glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, (float *) model.data);
        
        if (mesh.degree == 3) {
            glDrawArrays(GL_TRIANGLES, 0, mesh.primitives * 3);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, mesh.primitives * 6);
        }
        
        ++frame;
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    return(0);
}
