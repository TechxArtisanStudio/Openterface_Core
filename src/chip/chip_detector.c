#include "openterface/chip.h"

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define OP_ADDR_HDMI_CONNECTION_STATUS 0xFA8Cu
#define OP_ADDR_GPIO0 0xDF00u
#define OP_ADDR_SPDIFOUT 0xDF01u
#define OP_ADDR_FIRMWARE_VERSION_0 0xCBDCu
#define OP_ADDR_FIRMWARE_VERSION_1 0xCBDDu
#define OP_ADDR_FIRMWARE_VERSION_2 0xCBDEu
#define OP_ADDR_FIRMWARE_VERSION_3 0xCBDFu
#define OP_ADDR_INPUT_WIDTH_H 0xC6AFu
#define OP_ADDR_INPUT_WIDTH_L 0xC6B0u
#define OP_ADDR_INPUT_HEIGHT_H 0xC6B1u
#define OP_ADDR_INPUT_HEIGHT_L 0xC6B2u
#define OP_ADDR_INPUT_FPS_H 0xC6B5u
#define OP_ADDR_INPUT_FPS_L 0xC6B6u
#define OP_ADDR_INPUT_PIXELCLK_H 0xC73Cu
#define OP_ADDR_INPUT_PIXELCLK_L 0xC73Du

#define OP_MS2109S_ADDR_INPUT_WIDTH_H 0xC703u
#define OP_MS2109S_ADDR_INPUT_WIDTH_L 0xC704u
#define OP_MS2109S_ADDR_INPUT_HEIGHT_H 0xC705u
#define OP_MS2109S_ADDR_INPUT_HEIGHT_L 0xC706u
#define OP_MS2109S_ADDR_INPUT_FPS_H 0xC617u
#define OP_MS2109S_ADDR_INPUT_FPS_L 0xC618u
#define OP_MS2109S_ADDR_INPUT_PIXELCLK_H 0xC8C5u
#define OP_MS2109S_ADDR_INPUT_PIXELCLK_L 0xC8C6u
#define OP_MS2109S_ADDR_HDMI_CONNECTION_STATUS 0xFD9Cu

#define OP_MS2130S_ADDR_INPUT_WIDTH_H 0x1CFCu
#define OP_MS2130S_ADDR_INPUT_WIDTH_L 0x1CFDu
#define OP_MS2130S_ADDR_INPUT_HEIGHT_H 0x1CFEu
#define OP_MS2130S_ADDR_INPUT_HEIGHT_L 0x1CFFu
#define OP_MS2130S_ADDR_INPUT_FPS_H 0x1D02u
#define OP_MS2130S_ADDR_INPUT_FPS_L 0x1D03u
#define OP_MS2130S_ADDR_INPUT_PIXELCLK_H 0x1D00u
#define OP_MS2130S_ADDR_INPUT_PIXELCLK_L 0x1D01u
#define OP_MS2130S_ADDR_HDMI_CONNECTION_STATUS 0xFA8Du
#define OP_MS2130S_ADDR_FIRMWARE_VERSION_0 0x1FDCu
#define OP_MS2130S_ADDR_FIRMWARE_VERSION_1 0x1FDDu
#define OP_MS2130S_ADDR_FIRMWARE_VERSION_2 0x1FDEu
#define OP_MS2130S_ADDR_FIRMWARE_VERSION_3 0x1FDFu

static void op_video_chip_registers_clear(op_video_chip_register_set_t *registers) {
    if (registers == NULL) {
        return;
    }

    memset(registers, 0, sizeof(*registers));
}

static void op_video_chip_registers_fill_common(op_video_chip_register_set_t *registers) {
    registers->gpio0 = OP_ADDR_GPIO0;
    registers->spdifout = OP_ADDR_SPDIFOUT;
    registers->firmware_version[0] = OP_ADDR_FIRMWARE_VERSION_0;
    registers->firmware_version[1] = OP_ADDR_FIRMWARE_VERSION_1;
    registers->firmware_version[2] = OP_ADDR_FIRMWARE_VERSION_2;
    registers->firmware_version[3] = OP_ADDR_FIRMWARE_VERSION_3;
}

#define OP_MS2130S_VID 0x345Fu
#define OP_MS2130S_PID 0x2132u
#define OP_MS2109S_VID 0x345Fu
#define OP_MS2109S_PID 0x2109u
#define OP_MS2109_VID  0x534Du
#define OP_MS2109_PID  0x2109u

static int op_ascii_equal_ignore_case(char lhs, char rhs) {
    return tolower((unsigned char)lhs) == tolower((unsigned char)rhs);
}

static int op_string_contains_ignore_case(const char *haystack, const char *needle) {
    size_t needle_length;
    size_t index;

    if (haystack == NULL || needle == NULL) {
        return 0;
    }

    needle_length = strlen(needle);
    if (needle_length == 0u) {
        return 1;
    }

    for (index = 0u; haystack[index] != '\0'; ++index) {
        size_t offset;
        for (offset = 0u; offset < needle_length; ++offset) {
            char lhs = haystack[index + offset];
            if (lhs == '\0' || !op_ascii_equal_ignore_case(lhs, needle[offset])) {
                break;
            }
        }
        if (offset == needle_length) {
            return 1;
        }
    }

    return 0;
}

static int op_path_has_vid_pid(const char *path, const char *vid, const char *pid) {
    char vid_tag[16];
    char pid_tag[16];

    if (path == NULL || vid == NULL || pid == NULL) {
        return 0;
    }

    snprintf(vid_tag, sizeof(vid_tag), "vid_%s", vid);
    snprintf(pid_tag, sizeof(pid_tag), "pid_%s", pid);

    return (op_string_contains_ignore_case(path, vid_tag) || op_string_contains_ignore_case(path, vid))
        && (op_string_contains_ignore_case(path, pid_tag) || op_string_contains_ignore_case(path, pid));
}

static op_video_chip_kind_t op_video_chip_detect_from_vid_pid(uint16_t vendor_id, uint16_t product_id) {
    if (vendor_id == OP_MS2130S_VID && product_id == OP_MS2130S_PID) {
        return OP_VIDEO_CHIP_MS2130S;
    }
    if (vendor_id == OP_MS2109S_VID && product_id == OP_MS2109S_PID) {
        return OP_VIDEO_CHIP_MS2109S;
    }
    if (vendor_id == OP_MS2109_VID && product_id == OP_MS2109_PID) {
        return OP_VIDEO_CHIP_MS2109;
    }
    return OP_VIDEO_CHIP_UNKNOWN;
}

static op_video_chip_kind_t op_video_chip_detect_from_path(const char *hid_path) {
    if (hid_path == NULL || hid_path[0] == '\0') {
        return OP_VIDEO_CHIP_UNKNOWN;
    }

    if (op_path_has_vid_pid(hid_path, "345F", "2132")) {
        return OP_VIDEO_CHIP_MS2130S;
    }
    if (op_path_has_vid_pid(hid_path, "345F", "2109")) {
        return OP_VIDEO_CHIP_MS2109S;
    }
    if (op_path_has_vid_pid(hid_path, "534D", "2109")) {
        return OP_VIDEO_CHIP_MS2109;
    }
    return OP_VIDEO_CHIP_UNKNOWN;
}

op_video_chip_kind_t op_video_chip_detect_from_device(const op_device_info_t *device) {
    op_video_chip_kind_t kind;

    if (device == NULL) {
        return OP_VIDEO_CHIP_UNKNOWN;
    }

    switch (device->chip_hint) {
        case OP_VIDEO_CHIP_MS2109:
        case OP_VIDEO_CHIP_MS2109S:
        case OP_VIDEO_CHIP_MS2130S:
            return (op_video_chip_kind_t)device->chip_hint;
        default:
            break;
    }

    kind = op_video_chip_detect_from_vid_pid(device->vendor_id, device->product_id);
    if (kind != OP_VIDEO_CHIP_UNKNOWN) {
        return kind;
    }

    return op_video_chip_detect_from_path(device->hid_path);
}

const char *op_video_chip_kind_label(op_video_chip_kind_t kind) {
    switch (kind) {
        case OP_VIDEO_CHIP_MS2109:
            return "MS2109";
        case OP_VIDEO_CHIP_MS2109S:
            return "MS2109S";
        case OP_VIDEO_CHIP_MS2130S:
            return "MS2130S";
        default:
            return "UNKNOWN";
    }
}

int op_video_chip_get_register_set(op_video_chip_kind_t kind, op_video_chip_register_set_t *out_registers) {
    if (out_registers == NULL) {
        return 0;
    }

    op_video_chip_registers_clear(out_registers);
    op_video_chip_registers_fill_common(out_registers);

    switch (kind) {
        case OP_VIDEO_CHIP_MS2109:
            out_registers->hdmi_status = OP_ADDR_HDMI_CONNECTION_STATUS;
            out_registers->width_h = OP_ADDR_INPUT_WIDTH_H;
            out_registers->width_l = OP_ADDR_INPUT_WIDTH_L;
            out_registers->height_h = OP_ADDR_INPUT_HEIGHT_H;
            out_registers->height_l = OP_ADDR_INPUT_HEIGHT_L;
            out_registers->fps_h = OP_ADDR_INPUT_FPS_H;
            out_registers->fps_l = OP_ADDR_INPUT_FPS_L;
            out_registers->pixel_clock_h = OP_ADDR_INPUT_PIXELCLK_H;
            out_registers->pixel_clock_l = OP_ADDR_INPUT_PIXELCLK_L;
            return 1;
        case OP_VIDEO_CHIP_MS2109S:
            out_registers->hdmi_status = OP_MS2109S_ADDR_HDMI_CONNECTION_STATUS;
            out_registers->gpio0 = 0u;
            out_registers->spdifout = 0u;
            out_registers->width_h = OP_MS2109S_ADDR_INPUT_WIDTH_H;
            out_registers->width_l = OP_MS2109S_ADDR_INPUT_WIDTH_L;
            out_registers->height_h = OP_MS2109S_ADDR_INPUT_HEIGHT_H;
            out_registers->height_l = OP_MS2109S_ADDR_INPUT_HEIGHT_L;
            out_registers->fps_h = OP_MS2109S_ADDR_INPUT_FPS_H;
            out_registers->fps_l = OP_MS2109S_ADDR_INPUT_FPS_L;
            out_registers->pixel_clock_h = OP_MS2109S_ADDR_INPUT_PIXELCLK_H;
            out_registers->pixel_clock_l = OP_MS2109S_ADDR_INPUT_PIXELCLK_L;
            return 1;
        case OP_VIDEO_CHIP_MS2130S:
            out_registers->hdmi_status = OP_MS2130S_ADDR_HDMI_CONNECTION_STATUS;
            out_registers->firmware_version[0] = OP_MS2130S_ADDR_FIRMWARE_VERSION_0;
            out_registers->firmware_version[1] = OP_MS2130S_ADDR_FIRMWARE_VERSION_1;
            out_registers->firmware_version[2] = OP_MS2130S_ADDR_FIRMWARE_VERSION_2;
            out_registers->firmware_version[3] = OP_MS2130S_ADDR_FIRMWARE_VERSION_3;
            out_registers->width_h = OP_MS2130S_ADDR_INPUT_WIDTH_H;
            out_registers->width_l = OP_MS2130S_ADDR_INPUT_WIDTH_L;
            out_registers->height_h = OP_MS2130S_ADDR_INPUT_HEIGHT_H;
            out_registers->height_l = OP_MS2130S_ADDR_INPUT_HEIGHT_L;
            out_registers->fps_h = OP_MS2130S_ADDR_INPUT_FPS_H;
            out_registers->fps_l = OP_MS2130S_ADDR_INPUT_FPS_L;
            out_registers->pixel_clock_h = OP_MS2130S_ADDR_INPUT_PIXELCLK_H;
            out_registers->pixel_clock_l = OP_MS2130S_ADDR_INPUT_PIXELCLK_L;
            return 1;
        default:
            op_video_chip_registers_clear(out_registers);
            return 0;
    }
}
