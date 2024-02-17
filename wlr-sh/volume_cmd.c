#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "../lib/cmd_tree/include/cmd_tree.h"
#include "../src/services/ipc_service/ipc_commands.h"
#include "commands.h"

static int volume_root_exec(void *ctx, uint8_t argc, char **argv) {
    printf(
        "Summary:\n"
        "\tControl the volume of the default audio sink for the desktop "
        "session.\n"
        "Commands: \n"
        "\tup   - increase the volume by a step (.05)\n"
        "\tdown - decreate the volume by a step (.05)\n"
        "\tset  - set the volume to a given value (0.0-1.0)");
    return 0;
};

// The volume command for wlr-sh and is displayed when no other arguments are
// provided to the CLI.
//
// A short help blurb is presented along with a list of all available volume
// level commands.
cmd_tree_node_t volume_cmd_root = {.name = "volume", .exec = volume_root_exec};

static int volume_up_exec(void *ctx, uint8_t argc, char **argv) {
    int ret = 0;
    wlr_sh_ctx *wlr_ctx = ctx;

    IPCVolumeUp msg = {
        .header = {.type = IPC_CMD_VOLUME_UP},
    };

    IPC_SEND_MSG(wlr_ctx, msg);

    if (ret == -1) {
        perror("[Error] Failed to send IPCVolumeUp");
        return -1;
    }

    return 0;
};
cmd_tree_node_t volume_cmd_up = {.name = "up", .exec = volume_up_exec};

static int volume_down_exec(void *ctx, uint8_t argc, char **argv) {
    int ret = 0;
    wlr_sh_ctx *wlr_ctx = ctx;

    IPCVolumeDown msg = {
        .header = {.type = IPC_CMD_VOLUME_DOWN},
    };

    IPC_SEND_MSG(wlr_ctx, msg);

    if (ret == -1) {
        perror("[Error] Failed to send IPCVolumeDown");
        return -1;
    }

    return 0;
};
cmd_tree_node_t volume_cmd_down = {.name = "down", .exec = volume_down_exec};

static int volume_set_exec(void *ctx, uint8_t argc, char **argv) {
    int ret = 0;
    char *endptr = NULL;
    wlr_sh_ctx *wlr_ctx = ctx;

    if (argc != 1) {
        printf("Usage: volume set <volume>\n");
        return -1;
    }

    IPCVolumeSet msg = {
        .header = {.type = IPC_CMD_VOLUME_SET},
    };

    msg.volume = strtof(argv[0], &endptr);

    if (msg.volume < 0.0 || msg.volume > 1.0) {
        printf("Volume must be a float between 0.0 and 1.0\n");
        return -1;
    }

    IPC_SEND_MSG(wlr_ctx, msg);

    if (ret == -1) {
        perror("[Error] Failed to send IPCVolumeSet");
        return -1;
    }

    return 0;
};
cmd_tree_node_t volume_cmd_set = {.name = "set", .exec = volume_set_exec};

cmd_tree_node_t *volume_cmd() {
    cmd_tree_node_add_child(&volume_cmd_root, &volume_cmd_up);
    cmd_tree_node_add_child(&volume_cmd_root, &volume_cmd_down);
    cmd_tree_node_add_child(&volume_cmd_root, &volume_cmd_set);

    return &volume_cmd_root;
};