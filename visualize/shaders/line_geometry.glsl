#version 330

#ifdef GL_ES
precision mediump int;
precision mediump float;
#endif

layout(points) in;
layout(line_strip, max_vertices = 6) out;

uniform mat4 modelview;
uniform mat4 projection;
uniform vec3 tangent_color;
uniform vec3 surface_color;
uniform vec3 normal_color;

uniform float tangent_scale = 1.0;
uniform float surface_scale = 0.3;
uniform float normal_scale = 0.1;

in vec3 position[];
in vec3 tangent[];
in vec3 surface[];
in vec3 normal[];

out vec3 color;

void emit_line(vec3 start, vec3 end, vec3 line_color) {
    color = line_color;
    gl_Position = projection * modelview * vec4(start, 1.0);
    EmitVertex();
    gl_Position = projection * modelview * vec4(end, 1.0);
    EmitVertex();
    EndPrimitive();
}

void main() {
    vec3 base = position[0];
    vec3 with_tangent = base + tangent[0] * tangent_scale;
    vec3 with_surface = with_tangent + surface[0] * surface_scale;
    vec3 with_normal = with_surface + normal[0] * normal_scale;

    emit_line(base, with_tangent, tangent_color);
    emit_line(with_tangent, with_surface, surface_color);
    emit_line(with_surface, with_normal, normal_color);
}
