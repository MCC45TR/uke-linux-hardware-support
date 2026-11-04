#version 450
// SPDX-License-Identifier: MIT
layout(push_constant) uniform Frame { uint index; } frame;
layout(location = 0) out vec4 color;
void main() {
    color = ((frame.index & 1u) == 0u) ? vec4(1.0, 0.0, 0.0, 1.0) : vec4(0.0, 1.0, 0.0, 1.0);
}
