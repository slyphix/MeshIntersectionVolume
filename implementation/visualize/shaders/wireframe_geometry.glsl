#version 330

#ifdef GL_ES
precision mediump int;
precision mediump float;
#endif

layout(triangles) in;
layout(line_strip, max_vertices = 4) out;

void forward_vertex(int triangle_index) {
    gl_Position = gl_in[triangle_index].gl_Position;
    EmitVertex();
}

void main() {
    forward_vertex(0);
    forward_vertex(1);
    forward_vertex(2);
    forward_vertex(0);

    EndPrimitive();
}
