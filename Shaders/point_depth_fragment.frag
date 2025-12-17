#version 410 core

in vec3 FragPos;

uniform vec3 lightPos;   // Point light position (world space)
uniform float farPlane;  // Same farPlane used in matrix generation

void main()
{
    // Compute the distance from fragment to light
    float dist = length(FragPos - lightPos);

    // Write normalized depth (0..1)
    gl_FragDepth = dist / farPlane;
}
