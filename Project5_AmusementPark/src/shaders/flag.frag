#version 430 core
out vec4 f_color;

in V_OUT
{
   vec3 position;
   vec3 normal;
   vec2 texture_coordinate;
   vec3 mycolor;
} f_in;

uniform sampler2D u_texture;

void main()
{   
    f_color = texture(u_texture, f_in.texture_coordinate);
}