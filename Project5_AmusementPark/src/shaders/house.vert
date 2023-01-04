#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texture_coordinate;
layout (location = 3) in vec3 mycolor;

uniform mat4 u_model;
uniform mat4 projection;
uniform mat4 view;

out V_OUT
{
   vec3 position;
   vec3 normal;
   vec2 texture_coordinate;
   vec3 mycolor;
} v_out;


void main()
{
    float angle = radians(90);
    mat4 rotate = mat4(
        cos(angle), 0.0, -sin(angle), 0.0,
        0.0,        1.0, 0.0        , 0.0,
        sin(angle), 0.0, cos(angle) , 0.0,
        0.0,        0.0, 0.0        , 1.0
        );

    vec4 pos =  rotate * vec4(position, 1.0f);

    gl_Position = projection * view * u_model * pos;
    v_out.position = vec3(u_model * pos);
    v_out.normal = mat3(transpose(inverse(u_model))) * normal;
    v_out.texture_coordinate = vec2(texture_coordinate.x, 1.0f - texture_coordinate.y);
    v_out.mycolor = mycolor;
}