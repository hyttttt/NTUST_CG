// refer to: https://jayconrod.com/posts/34/water-simulation-in-glsl

#version 430 core
out vec4 f_color;

in V_OUT
{
   vec3 position;
   vec3 normal;
   vec2 texture_coordinate;
   vec3 mycolor;
} f_in;

uniform vec3 camera_pos;
uniform samplerCube skybox;
//uniform sampler2D tub;

void main()
{   
    vec3 I = normalize(f_in.position - camera_pos);
    vec3 reflect_ray = reflect(I, normalize(f_in.normal));

    float ratio = 1.00 / 1.52;
    vec3 refract_ray = refract(I, normalize(f_in.normal), ratio);

    vec4 reflect_color = vec4(texture(skybox, reflect_ray).rgb, 1.0);
    //vec4 refract_color = vec4(texture(tub, refract_ray).rgb, 1.0);
    vec4 water_color = vec4(f_in.mycolor, 0.5f);

    f_color = reflect_color * water_color;
    //f_color = reflect_color * 0.5 + water_color * 0.5;
}