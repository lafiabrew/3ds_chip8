#include "chip8.h"
#include "pk_platform.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    char sd_root[512];
    c8_vm_t vm;
    int demo_idx;
    char rom_name[32];
    bool paused;
    char status_msg[128];
} c8_app_t;

static void app_init(c8_app_t *app, const char *sd_root) {
    memset(app, 0, sizeof(*app));
    snprintf(app->sd_root, sizeof(app->sd_root), "%s",
             sd_root != NULL ? sd_root : pk_platform_default_sd_root());
    c8_load_builtin_demo(&app->vm, 0, app->rom_name, sizeof(app->rom_name));
    for (int i = 0; i < 16; i++) {
        c8_step_Instruction(&app->vm);
    }
    snprintf(app->status_msg, sizeof(app->status_msg),
             "ROM: %s | PC:0x%03X | A:STEP X:RUN/PAUSE L/R:ROM",
             app->rom_name, (unsigned)app->vm.pc);
    pk_log_event(app->sd_root, "chip8", "INFO", app->status_msg);
}

static void app_step(c8_app_t *app, const pk_input_t *in) {
    if (in->down & PK_BTN_L || in->down & PK_BTN_R) {
        app->demo_idx = (app->demo_idx + 1) % 3;
        c8_load_builtin_demo(&app->vm, app->demo_idx, app->rom_name, sizeof(app->rom_name));
        for (int i = 0; i < 16; i++) c8_step_Instruction(&app->vm);
        pk_log_event(app->sd_root, "chip8", "INFO", app->rom_name);
    }
    if (in->down & PK_BTN_X) {
        app->paused = !app->paused;
    }
    if (in->down & PK_BTN_A || !app->paused) {
        c8_step_Instruction(&app->vm);
    }
    char dis[32];
    c8_disassemble(app->vm.last_opcode, dis, sizeof(dis));
    snprintf(app->status_msg, sizeof(app->status_msg),
             "PC:0x%03X I:0x%03X OP:%s CYCLES:%u",
             (unsigned)app->vm.pc, (unsigned)app->vm.i_reg, dis, (unsigned)app->vm.cycles);
}

static void app_render(const c8_app_t *app) {
    pk_render_begin_frame();
    pk_render_begin_target(PK_TARGET_TOP_LEFT, PK_RGBA8(10, 18, 14, 255));
    pk_draw_rect(0, 0, 400, 26, PK_RGBA8(22, 54, 42, 255));
    pk_draw_text(8, 7, 0.48f, PK_RGBA8(95, 255, 165, 255),
                 "CHIP-8 STUDIO 3DS - VM, DISASSEMBLER & HEX PAD");

    /* 64x32 display scaled 5x (320x160) at x=40, y=38 */
    pk_draw_rect(38, 36, 324, 164, PK_RGBA8(16, 32, 24, 255));
    pk_draw_rect_outline(38, 36, 324, 164, 1.0f, PK_RGBA8(75, 215, 140, 255));
    for (int y = 0; y < C8_DISP_H; y++) {
        for (int x = 0; x < C8_DISP_W; x++) {
            if (app->vm.display[y * C8_DISP_W + x]) {
                pk_draw_rect((float)(40 + x * 5), (float)(38 + y * 5), 4.0f, 4.0f,
                             PK_RGBA8(110, 255, 175, 255));
            }
        }
    }

    pk_draw_rect(0, 216, 400, 24, PK_RGBA8(18, 42, 32, 255));
    pk_draw_text(8, 222, 0.44f, PK_RGBA8(120, 255, 175, 255), app->status_msg);

    /* Bottom Screen: 4x4 Touch Hex Keypad (left) + Live V0..VF Register Inspector (right) */
    pk_render_begin_target(PK_TARGET_BOTTOM, PK_RGBA8(12, 16, 22, 255));
    static const char *const k_keys[16] = {
        "1", "2", "3", "C",
        "4", "5", "6", "D",
        "7", "8", "9", "E",
        "A", "0", "B", "F"
    };
    for (int k = 0; k < 16; k++) {
        float kx = (float)(10 + (k % 4) * 36);
        float ky = (float)(20 + (k / 4) * 38);
        pk_draw_rect(kx, ky, 32.0f, 32.0f, PK_RGBA8(32, 58, 86, 255));
        pk_draw_text(kx + 11.0f, ky + 10.0f, 0.48f, PK_RGBA8(255, 235, 110, 255), k_keys[k]);
    }

    pk_draw_rect(164, 20, 148, 148, PK_RGBA8(20, 28, 40, 255));
    pk_draw_text(172, 26, 0.44f, PK_RGBA8(130, 230, 255, 255), "REGISTERS V0..VF:");
    for (int r = 0; r < 8; r++) {
        char rline[48];
        snprintf(rline, sizeof(rline), "V%X:%02X  V%X:%02X",
                 (unsigned)r, (unsigned)app->vm.v[r],
                 (unsigned)(r + 8), (unsigned)app->vm.v[r + 8]);
        pk_draw_text(174.0f, (float)(44 + r * 14), 0.42f, PK_RGBA8(220, 235, 250, 255), rline);
    }
    pk_draw_text(10, 216, 0.42f, PK_RGBA8(180, 205, 230, 255),
                 "A:STEP | X:RUN/PAUSE | L/R:DEMO ROM | START:EXIT");
    pk_render_end_frame();
}

int main(int argc, char **argv) {
    const char *sd_root = NULL;
    bool one_shot = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--sd-root") == 0 && i + 1 < argc) sd_root = argv[++i];
        else if (strcmp(argv[i], "--once") == 0) one_shot = true;
    }
    if (!pk_platform_init()) return 1;
    c8_app_t app;
    app_init(&app, sd_root);
    while (pk_platform_main_loop()) {
        pk_input_t in;
        pk_platform_poll_input(&in);
        if (in.down & PK_BTN_START) break;
        app_step(&app, &in);
        app_render(&app);
        if (one_shot) break;
    }
    pk_platform_shutdown();
    return 0;
}
