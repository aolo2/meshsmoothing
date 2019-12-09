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
    
    (void) window;
    (void) mods;
}

static void
update_state(struct ms_mesh *mesh, struct ms_gl_bufs bufs)
{
    /* TODO: Arcball camera */
    
    if (state.keys[GLFW_KEY_ENTER]) {
        /* To prevent accidental spamming */
        state.keys[GLFW_KEY_ENTER] = false;
        
        struct ms_mesh new_mesh = ms_subdiv_catmull_clark(*mesh);
        free(mesh->vertices);
        free(mesh->normals);
        
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
}

static void
print_matrix(struct ms_m4 M)
{
    printf("%f %f %f %f\n", M.data[0][0], M.data[0][1], M.data[0][2], M.data[0][3]);
    printf("%f %f %f %f\n", M.data[1][0], M.data[1][1], M.data[1][2], M.data[1][3]);
    printf("%f %f %f %f\n", M.data[2][0], M.data[2][1], M.data[2][2], M.data[2][3]);
    printf("%f %f %f %f\n", M.data[3][0], M.data[3][1], M.data[3][2], M.data[3][3]);
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
    
    struct ms_v3 *projected_points = malloc(6 * mesh.primitives * sizeof(struct ms_v3));
    struct ms_gl_bufs bufs = ms_opengl_init_buffers(mesh, &state.triangulated_points);
    s32 shader_program = ms_opengl_init_shader_program();
    
    f32 aspect = 1280.0f / 720.0f;
    struct ms_m4 proj = ms_math_perspective(aspect, 75.0f, 0.5f, 100.0f);
    struct ms_m4 rotate = ms_math_unitm4();
    //struct ms_m4 proj = ms_math_ortho(-1.0f * aspect, aspect, -1.0f, 1.0f, 0.1f, 10.0f);
    
    u32 npoints = 3 * mesh.primitives;
    if (mesh.degree == 4) {
        npoints  = 6 * mesh.primitives;
    }
    
    struct ms_v3 axis[] = {
        { 1.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f },
    };
    
    state.cc_step = 0;
    state.frame = 0;
    state.scale_factor = 2.0f;
    state.rot_angle = 0.0f;
    state.translation = 6.4f;
    state.rotation = Y_AXIS;
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glPointSize(8.0f);
    
    
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glfwPollEvents();
        glfwGetCursorPos(window, state.cursor + 0, state.cursor + 1);
        
        u32 cc = state.cc_step;
        update_state(&mesh, bufs);
        if (cc != state.cc_step) {
            npoints = 6 * mesh.primitives;
            projected_points = realloc(projected_points, npoints * sizeof(struct ms_v3));
        }
        
        struct ms_v3 rotation_axis = axis[state.rotation];
        rotate = ms_math_mm(rotate, ms_math_rot(rotation_axis.x, rotation_axis.y, rotation_axis.z, state.rot_angle));
        
        //printf("%f %f %f\n", state.rot_angle, state.translation, state.scale_factor);
        
        struct ms_m4 view = ms_math_translate(0.0f, 0.0f, -1.0f * state.translation);
        struct ms_m4 scale = ms_math_scale(state.scale_factor);
        struct ms_m4 model = ms_math_mm(scale, rotate);
        
        /* TODO: move to state update */
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
        
        if (state.mousedown) {
            ms_opengl_flatquad(qx1, qy1, qx2, qy2);
            for (u32 i = 0; i < npoints; ++i) {
                projected_points[i] = ms_math_mv(model, state.triangulated_points[i]);
                projected_points[i] = ms_math_mv(view, projected_points[i]);
                projected_points[i] = ms_math_mv(proj, projected_points[i]);
            }
        }
        
        glBindVertexArray(bufs.VAO);
        glUseProgram(shader_program);
        
        /* NOTE: OpenGL expects column-major, I use row-major, hence the GL_TRUE for transosition */
        glUniformMatrix4fv(glGetUniformLocation(shader_program, "proj"), 1, GL_TRUE, (float *) proj.data);
        glUniformMatrix4fv(glGetUniformLocation(shader_program, "view"),  1, GL_TRUE, (float *) view.data);
        glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_TRUE, (float *) model.data);
        
        glDrawArrays(GL_TRIANGLES, 0, npoints);
        
        if (state.mousedown) {
            for (u32 i = 0; i < npoints; ++i) {
                f32 px = projected_points[i].x;
                f32 py = projected_points[i].y;
                if (qx1 <= px && px <= qx2 && qy1 <= py && py <= qy2) {
                    glDrawArrays(GL_POINTS, i, 1);
                }
            }
        }
        
        ++state.frame;
        
        glfwSwapBuffers(window);
    }
    
    return(0);
}
