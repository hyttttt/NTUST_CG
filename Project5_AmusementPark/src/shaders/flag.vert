// refer to: https://jayconrod.com/posts/34/water-simulation-in-glsl

#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texture_coordinate;
layout (location = 3) in vec3 mycolor;

uniform mat4 u_model;
uniform float time;

const float pi = 3.14159f;
float wavelength = 2.0f * pi;
float speed = 3.0f;
float amplitude = 0.5f;
float angle = fract(sin(pi)*100000.0); // simulate random
vec2 direction = vec2(cos(angle), sin(angle));

layout (std140, binding = 0) uniform commom_matrices
{
    mat4 u_projection;
    mat4 u_view;
};

out V_OUT
{
   vec3 position;
   vec3 normal;
   vec2 texture_coordinate;
   vec3 mycolor;
} v_out;

float wave(float x, float y) {
    float frequency = 2.0 * pi / wavelength;
    float phase = speed * frequency;
    float theta = dot(direction, vec2(x, y));

    return amplitude * sin(theta * frequency + time * phase);
}

vec3 waveNormal(float x, float y) {
    float frequency = 2.0 * pi / wavelength;
    float phase = speed * frequency;
    float theta = dot(direction, vec2(x, y));

    float dx = amplitude * direction.x * frequency * cos(theta * frequency + time * phase);
    float dy = amplitude * direction.y * frequency * cos(theta * frequency + time * phase);

    vec3 n = vec3(-dx, 1.0, -dy);
    return normalize(n);
}

void main()
{
    vec4 mywave = vec4(position.x, position.y, wave(position.x, position.y), 1.0f);
    vec3 mynormal = waveNormal(position.x, position.z);

    float angle = radians(90);
    mat4 rotate_z = mat4(
        cos(angle),  sin(angle), 0.0, 0.0,
        -sin(angle), cos(angle), 0.0, 0.0,
        0.0,         0.0,        1.0, 0.0,
        0.0,         0.0,        0.0, 1.0
    );

    angle = radians(30);
    mat4 rotate_x = mat4(
        1.0, 0.0,         0.0,        0.0,
        0.0, cos(angle),  sin(angle), 0.0,
        0.0, -sin(angle), cos(angle), 0.0,
        0.0, 0.0,         0.0,        1.0
    );

    float dx = -9.1;
    float dy = 5.0;
    float dz = -10.0;
    mat4 translation = mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        dx,  dy,  dz,  1.0
    );

    vec4 pos = translation * rotate_x * rotate_z * mywave;

    gl_Position = u_projection * u_view * u_model * pos;
    v_out.position = vec3(u_model * pos);
    v_out.normal = mat3(transpose(inverse(u_model))) * mynormal;
    v_out.texture_coordinate = vec2(texture_coordinate.x, 1.0f - texture_coordinate.y);
    v_out.mycolor = mycolor;
}