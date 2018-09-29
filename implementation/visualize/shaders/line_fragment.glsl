#version 330
#extension GL_ARB_conservative_depth : enable

#ifdef GL_ES
precision mediump int;
precision mediump float;
#endif

in vec3 color;

out vec4 final_fragment_color;

layout(depth_less) out float gl_FragDepth;

void main() {
    gl_FragDepth = max(0.0, gl_FragCoord.z - 1e-3);
    final_fragment_color = vec4(color, 1.0);
}
