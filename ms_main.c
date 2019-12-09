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
mousebutton_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            state.mousedown = true;
            state.cursor_last[0] = state.cursor[0];
            state.cursor_last[1] = state.cursor[1];
        } else if (action == GLFW_RELEASE) {
            state.mousedown = false;
        }
    }
    
    if (mods & GLFW_MOD_SHIFT) {
        state.keys[1024 - GLFW_MOD_SHIFT] = true;
    } else {
        state.keys[1024 - GLFW_MOD_SHIFT] = false;
    }
    
    (void) window;
}

static bool
update_state(struct ms_mesh *mesh, struct ms_gl_bufs bufs, u32 npoints)
{
    bool result = false;
    
    if (state.keys[GLFW_KEY_ENTER]) {
        /* To prevent accidental spamming */
        state.keys[GLFW_KEY_ENTER] = false;
        
        struct ms_v3 *special_points = malloc(4096 * sizeof(struct ms_v3));
        u32 nspecial = 0;
        
        for (u32 i = 0; i < npoints; ++i) {
            if (state.marked[i]) {
                special_points[nspecial++] = state.triangulated_points[i];
            }
        }
        
        struct ms_mesh new_mesh = ms_subdiv_catmull_clark(*mesh, special_points, nspecial);
        free(mesh->vertices);
        free(mesh->normals);
        
        result = true;
        *mesh = new_mesh;
        ms_opengl_update_buffers(bufs, &state.triangulated_points, *mesh);
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
    if (state.keys[GLFW_KEY_LEFT])  { state.rot_angle = 1.0f / 180.0f * M_PI; }
    if (state.keys[GLFW_KEY_RIGHT]) { state.rot_angle = -1.0f / 180.0f * M_PI; }
    if (state.keys[GLFW_KEY_DOWN])  { state.translation += 0.1f; }
    if (state.keys[GLFW_KEY_UP])    { state.translation -= 0.1f; }
    if (state.keys[GLFW_KEY_X])     { state.rotation = X_AXIS; }
    if (state.keys[GLFW_KEY_Y])     { state.rotation = Y_AXIS; }
    if (state.keys[GLFW_KEY_Z])     { state.rotation = Z_AXIS; }
    
    f32 qx1 = (state.cursor_last[0] - 640.0f) / 640.0f;
    f32 qy1 = (state.cursor_last[1] - 360.0f) / -360.0f;
    f32 qx2 = (state.cursor[0] - 640.0f) / 640.0f;
    f32 qy2 = (state.cursor[1] - 360.0f) / -360.0f;
    
    if (qx1 > qx2) {
        f32 tmp = qx1;
        qx1 = qx2;
        qx2 = tmp;
    }
    
    if (qy1 > qy2) {
        f32 tmp = qy1;
        qy1 = qy2;
        qy2 = tmp;
    }
    
    state.qx1 = qx1;
    state.qy1 = qy1;
    state.qx2 = qx2;
    state.qy2 = qy2;
    
    return(result);
}

s32
main(s32 argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "[ERROR] Usage: %s path/to/model.stl\n", argv[0]);
        return(1);
    }
    
    printf("[INFO] Keymap\n");
    printf("\t Zoom in:                             UP ARROW\n");
    printf("\t Zoom out:                            DOWN ARROW\n");
    printf("\t Change rotation axis:                x/y/z\n");
    printf("\t Rotate clockwise around axis:        RIGHT ARROW\n");
    printf("\t Rotate counterclockwise around axis: LEFT ARROW\n");
    printf("\t Subdivide:                           ENTER\n");
    printf("\t Save to disk (interactive):          SPACE\n");
    
    char *filename = argv[1];
    char *extension = filename;
    u32 filename_len = strlen(filename);
    
    for (u32 i = filename_len - 1; i != 0; --i) {
        if (filename[i] == '.') {
            extension = filename + i + 1;
        }
    }
    
    struct ms_mesh mesh = { 0 };
    if (strcmp("stl", extension) == 0) {
        mesh = ms_file_stl_read_file(filename);
    } else if (strcmp("obj", extension) == 0) { 
        mesh = ms_file_obj_read_file(filename);
    } else {
        fprintf(stderr, "[ERROR] Unknown file extension: .%s\n", extension);
        return(1);
    }
    
    GLFWwindow *window = ms_opengl_init(1280, 720);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mousebutton_callback);
    
    assert(window);
    
    struct ms_gl_bufs bufs = ms_opengl_init_buffers(mesh, &state.triangulated_points);
    s32 shader_program = ms_opengl_init_shader_program();
    struct ms_v3 *projected_points = malloc(6 * mesh.primitives * sizeof(struct ms_v3));
    
    f32 aspect = 1280.0f / 720.0f;
    struct ms_m4 proj = ms_math_perspective(aspect, 75.0f, 0.5f, 100.0f);
    struct ms_m4 rotate = ms_math_unitm4();
    //struct ms_m4 proj = ms_math_ortho(-1.0f * aspect, aspect, -1.0f, 1.0f, 0.1f, 10.0f);
    
    u32 npoints = 3 * mesh.primitives;
    if (mesh.degree == 4) {
        npoints = 6 * mesh.primitives;
    }
    
    struct ms_v3 axis[] = {
        { 1.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f },
    };
    
    struct ms_v3 color_white = { 1.0f, 1.0f, 1.0f };
    struct ms_v3 color_yellow = { 1.0f, 1.0f, 0.0f };
    
    state.cc_step = 0;
    state.frame = 0;
    state.scale_factor = 2.0f;
    state.rot_angle = 0.0f;
    state.translation = 6.4f;
    state.rotation = Y_AXIS;
    state.marked = calloc(1, npoints * sizeof(bool));
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glPointSize(8.0f);
    
    
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glfwPollEvents();
        glfwGetCursorPos(window, state.cursor + 0, state.cursor + 1);
        
        bool subdivided = update_state(&mesh, bufs, npoints);
        if (subdivided) {
            npoints = 6 * mesh.primitives;
            projected_points = realloc(projected_points, npoints * sizeof(struct ms_v3));
            state.marked = realloc(state.marked, npoints * sizeof(bool));
            memset(state.marked, false, npoints * sizeof(bool));
        }
        
        struct ms_v3 rotation_axis = axis[state.rotation];
        rotate = ms_math_mm(rotate, ms_math_rot(rotation_axis.x, rotation_axis.y, rotation_axis.z, state.rot_angle));
        
        struct ms_m4 view = ms_math_translate(0.0f, 0.0f, -1.0f * state.translation);
        struct ms_m4 scale = ms_math_scale(state.scale_factor);
        struct ms_m4 model = ms_math_mm(scale, rotate);
        
        for (u32 i = 0; i < npoints; ++i) {
            projected_points[i] = ms_math_mv(model, state.triangulated_points[i]);
            projected_points[i] = ms_math_mv(view, projected_points[i]);
            projected_points[i] = ms_math_mv(proj, projected_points[i]);
        }
        
        if (state.mousedown) {
            ms_opengl_flatquad(state.qx1, state.qy1, state.qx2, state.qy2);
        }
        
        glBindVertexArray(bufs.VAO);
        glUseProgram(shader_program);
        
        /* NOTE: OpenGL expects column-major, I use row-major, hence the GL_TRUE for transosition */
        glUniform3f(glGetUniformLocation(shader_program, "color"), color_white.x, color_white.y, color_white.z);
        glUniformMatrix4fv(glGetUniformLocation(shader_program, "proj"), 1, GL_TRUE, (float *) proj.data);
        glUniformMatrix4fv(glGetUniformLocation(shader_program, "view"),  1, GL_TRUE, (float *) view.data);
        glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_TRUE, (float *) model.data);
        
        glDrawArrays(GL_TRIANGLES, 0, npoints);
        glUniform3f(glGetUniformLocation(shader_program, "color"), color_yellow.x, color_yellow.y, color_yellow.z);
        
        if (state.mousedown) {
            if (!state.keys[1024 - GLFW_MOD_SHIFT]) {
                memset(state.marked, false, npoints * sizeof(bool));
            }
            
            for (u32 i = 0; i < npoints; ++i) {
                f32 px = projected_points[i].x;
                f32 py = projected_points[i].y;
                if (state.qx1 <= px && px <= state.qx2 && state.qy1 <= py && py <= state.qy2) {
                    state.marked[i] = true; 
                }
            }
        }
        
        for (u32 i = 0; i < npoints; ++i) {
            if (state.marked[i]) {
                glDrawArrays(GL_POINTS, i, 1);
            }
        }
        
        ++state.frame;
        
        /* TracyCFrameMark; */
        
        glfwSwapBuffers(window);
    }
    
    return(0);
}
