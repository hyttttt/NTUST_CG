#version 430 core
out vec4 f_color;

in V_OUT
{
   vec3 position;
   vec3 normal;
   vec2 texture_coordinate;
   vec3 mycolor;
} f_in;

uniform vec4 u_color;
uniform vec3 light_pos;
uniform mat4 u_model;

void main()
{   
    f_color = u_color;
}