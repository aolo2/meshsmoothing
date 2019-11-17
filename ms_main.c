#include "ms_common.h"

#include "ms_file.c"
#include "ms_opengl.c"
#include "ms_math.c"
#include "ms_subdiv.c"

static struct ms_state state;

static void
key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS) {
        state.keys[key] = true;
    } else if (action == GLFW_RELEASE) {
        state.keys[key] = false;
    }
    
    (void) window;
    (void) scancode;
    (void) mods;
}

static void
update_state(struct ms_mesh *mesh, struct ms_gl_bufs bufs)
{
    if (state.keys[GLFW_KEY_ENTER]) {
        /* To prevent accidental spamming */
        state.keys[GLFW_KEY_ENTER] = false;
        
        struct ms_mesh new_mesh = ms_subdiv_catmull_clark(*mesh);
        free(mesh->vertices);
        free(mesh->normals);
        
        *mesh = new_mesh;
        ms_opengl_update_buffers(bufs, *mesh);
        printf("[INFO] Finished Catmull-Clark [%d]\n", ++state.cc_step);
    }
    
    if (state.keys[GLFW_KEY_SPACE]) {
        /* To prevent accidental spamming */
        state.keys[GLFW_KEY_SPACE] = false;
        
        char filename[512];
        printf("[SAVE] Filename: ");
        scanf("%s", filename);
        
        ms_file_stl_write_file(*mesh, filename);
    }
    
    if (!state.keys[GLFW_KEY_LEFT] && !state.keys[GLFW_KEY_RIGHT]) { state.rot_angle = 0.0f; }
    if (state.keys[GLFW_KEY_LEFT])  { state.rot_angle = -1.0f / 180.0f * M_PI; }
    if (state.keys[GLFW_KEY_RIGHT]) { state.rot_angle = 1.0f / 180.0f * M_PI; }
    if (state.keys[GLFW_KEY_DOWN])  { state.translation += 0.1f; }
    if (state.keys[GLFW_KEY_UP])    { state.translation -= 0.1f; }
    if (state.keys[GLFW_KEY_X])     { state.rotation = X_AXIS; }
    if (state.keys[GLFW_KEY_Y])     { state.rotation = Y_AXIS; }
    if (state.keys[GLFW_KEY_Z])     { state.rotation = Z_AXIS; }
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
    
    char *filename = argv[1];
    char *extension = "stl";
    u32 filename_len = strlen(filename);
    
    for (u32 i = filename_len - 1; i != 0; --i) {
        if (filename[i] == '.') {
            extension = filename + i + 1;
        }
    }
    
    struct ms_mesh mesh;
    if (strcmp("stl", extension) == 0) {
        mesh = ms_file_stl_read_file(filename);
    } else if (strcmp("obj", extension) == 0) { 
        mesh = ms_file_obj_read_file(filename);
    }
    
    GLFWwindow *window = ms_opengl_init(1280, 720);
    glfwSetKeyCallback(window, key_callback);
    
    assert(window);
    
    struct ms_gl_bufs bufs = ms_opengl_init_buffers(mesh);
    s32 shader_program = ms_opengl_init_shader_program();
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glUseProgram(shader_program);
    glBindVertexArray(bufs.VAO);
    
    struct ms_m4 proj = ms_math_ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 10.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "proj"), 1, GL_FALSE, (float *) proj.data);
    
    struct ms_v3 axis[] = {
        { 1.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f },
    };
    
    state.cc_step = 0;
    state.frame = 0;
    state.scale_factor = 2.0f;
    state.rot_angle = 0.0f;
    state.translation = 0.0f;
    state.rotation = Y_AXIS;
    
    struct ms_m4 rotate = ms_math_unitm4();
    
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glfwPollEvents();
        
        update_state(&mesh, bufs);
        
        struct ms_v3 rotation_axis = axis[state.rotation];
        struct ms_m4 view = ms_math_translate(0.0f, 0.0f, -1.0f * state.translation);
        rotate = ms_math_mm(rotate, ms_math_rot(rotation_axis.x, rotation_axis.y, rotation_axis.z, state.rot_angle));
        struct ms_m4 scale = ms_math_scale(state.scale_factor);
        struct ms_m4 model = ms_math_mm(scale, rotate);
        
        glUniformMatrix4fv(glGetUniformLocation(shader_program, "view"), 1, GL_FALSE, (float *) view.data);
        glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, (float *) model.data);
        
        if (mesh.degree == 3) {
            glDrawArrays(GL_TRIANGLES, 0, mesh.primitives * 3);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, mesh.primitives * 6);
        }
        
        ++state.frame;
        
        glfwSwapBuffers(window);
    }
    
    return(0);
}
