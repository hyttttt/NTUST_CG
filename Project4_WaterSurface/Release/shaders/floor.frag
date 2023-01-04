#version 330 core
out vec4 FragColor;

in vec3 texCoords;

uniform sampler2D floor;

void main()
{    
    FragColor = texture(floor, texCoords);
}