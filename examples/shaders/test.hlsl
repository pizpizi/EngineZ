#version 450

layout(location = 0) out vec4 fragColor;

vec3 posisitions[3] = vec3[](
        vec3(0.0, -0.4, 0.0),
        vec3(0.4, 0.4, 0.0),
        vec3(-0.4, 0.4, 0.0)
    );

vec3 colors[3] = vec3[](
        vec3(0.0, 1.0, 0.0),
        vec3(1.0, 0.0, 0.0),
        vec3(0.0, 0.0, 1.0)
    );
void main() {
    gl_Position = vec4(posisitions[gl_VertexIndex], 1.0);
    fragColor = vec4(colors[gl_VertexIndex], 1.0);
    
}
