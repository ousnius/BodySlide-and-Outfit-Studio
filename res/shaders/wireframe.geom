#version 330

layout(triangles) in;
layout(line_strip, max_vertices = 6) out;

out vec3 gWorldPos;

void EmitEdge(int i0, int i1) {
    vec4 p0 = gl_in[i0].gl_Position;
    vec4 p1 = gl_in[i1].gl_Position;

    // Apply a slight depth offset toward camera (in clip space)
    vec3 dir = normalize((p0.xyz / p0.w) - (p1.xyz / p1.w));
    float offset = 0.0005;

    p0.xyz += dir * offset * p0.w;
    p1.xyz += dir * offset * p1.w;

    gl_Position = p0;
    gWorldPos = (gl_in[i0].gl_Position).xyz;
    EmitVertex();

    gl_Position = p1;
    gWorldPos = (gl_in[i1].gl_Position).xyz;
    EmitVertex();

    EndPrimitive();
}

void main() {
	// Cull back-facing triangles here if needed, depending on mesh settings
	// ...

    EmitEdge(0, 1);
    EmitEdge(1, 2);
    EmitEdge(2, 0);
}
