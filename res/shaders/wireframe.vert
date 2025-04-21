#version 330

layout(location = 0) in vec3 aPosition;

uniform mat4 matModelView;
uniform mat4 matProjection;

out vec3 vWorldPos;

void main() {
    vec4 worldPos = matModelView * vec4(aPosition, 1.0);
    vWorldPos = worldPos.xyz;
    gl_Position = matProjection * worldPos;
}
