#version 330

#ifdef GL_ES
precision mediump int;
precision mediump float;
#endif

in vec3 a_position;

uniform mat4 modelview;
uniform mat4 projection;

void main() {
    gl_Position = projection * modelview * vec4(a_position, 1.0);
}
