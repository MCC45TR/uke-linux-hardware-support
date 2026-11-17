/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "label-bridge.h"
#include <ply-label.h>
uke_label_t* uke_label_new(void) { return ply_label_new(); }
void uke_label_free(uke_label_t* label) { ply_label_free(label); }
void uke_label_hide(uke_label_t* label) { ply_label_hide(label); }
void uke_label_draw_area(uke_label_t* label, ply_pixel_buffer_t* buffer, long x, long y,
                         unsigned long width, unsigned long height) {
    ply_label_draw_area(label, buffer, x, y, width, height);
}
void uke_label_set_text(uke_label_t* label, const char* text) { ply_label_set_text(label, text); }
void uke_label_center(uke_label_t* label) { ply_label_set_alignment(label, PLY_LABEL_ALIGN_CENTER); }
void uke_label_set_width(uke_label_t* label, long width) { ply_label_set_width(label, width); }
void uke_label_set_font(uke_label_t* label, const char* font) { ply_label_set_font(label, font); }
void uke_label_set_color(uke_label_t* label, float r, float g, float b, float a) { ply_label_set_color(label,r,g,b,a); }
bool uke_label_show(uke_label_t* label, ply_pixel_display_t* display, long x, long y) { return ply_label_show(label,display,x,y); }
long uke_label_get_width(uke_label_t* label) { return ply_label_get_width(label); }
long uke_label_get_height(uke_label_t* label) { return ply_label_get_height(label); }
