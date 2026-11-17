/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once
#include <stdbool.h>
#include <ply-pixel-display.h>
#include <ply-pixel-buffer.h>
#ifdef __cplusplus
extern "C" {
#endif
/* The public label header contains a C enum forward declaration that C++ rejects.
 * Keep that header in this small C translation unit; the theme remains C++.
 * This opaque public type is never inspected. */
typedef struct _ply_label uke_label_t;
uke_label_t* uke_label_new(void);
void uke_label_free(uke_label_t* label);
void uke_label_hide(uke_label_t* label);
void uke_label_draw_area(uke_label_t* label, ply_pixel_buffer_t* buffer, long x, long y,
                         unsigned long width, unsigned long height);
void uke_label_set_text(uke_label_t* label, const char* text);
void uke_label_center(uke_label_t* label);
void uke_label_set_width(uke_label_t* label, long width);
void uke_label_set_font(uke_label_t* label, const char* font);
void uke_label_set_color(uke_label_t* label, float r, float g, float b, float a);
bool uke_label_show(uke_label_t* label, ply_pixel_display_t* display, long x, long y);
long uke_label_get_width(uke_label_t* label);
long uke_label_get_height(uke_label_t* label);
#ifdef __cplusplus
}
#endif
