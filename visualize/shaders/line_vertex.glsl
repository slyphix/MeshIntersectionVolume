#version 330

#ifdef GL_ES
precision mediump int;
precision mediump float;
#endif

in vec3 a_position;
in vec3 a_tangent;
in vec3 a_surface;
in vec3 a_normal;

out vec3 position;
out vec3 tangent;
out vec3 surface;
out vec3 normal;

void main() {
    position = a_position;
    tangent = a_tangent;
    surface = a_surface;
    normal = a_normal;
}
