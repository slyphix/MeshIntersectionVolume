#version 330

#ifdef GL_ES
precision mediump int;
precision mediump float;
#endif

uniform vec3 color;

out vec4 final_fragment_color;

void main() {
    final_fragment_color = vec4(color, 1.0);
}
