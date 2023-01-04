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

// refer to: https://learnopengl.com/Lighting/Basic-Lighting
void main()
{   
    vec3 lightColor = vec3(1.0f, 0.9f, 0.6f);
    vec3 lightPos = vec3(u_model * vec4(light_pos, 1.0));

    float ambientStrength = 0.1f;
    vec3 ambient = ambientStrength * lightColor;
    
    vec3 L = normalize(lightPos - f_in.position);
    vec3 N = normalize(vec3(u_model * vec4(f_in.normal, 1.0)));
    float diffFactor = max(dot(L, N), 0.0);
    vec3 diffuse = lightColor * diffFactor;

    f_color = vec4(u_color.xyz * (ambient + diffuse * 2), u_color.w);
}