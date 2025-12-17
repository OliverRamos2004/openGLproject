#version 410 core

layout (location = 0) in vec3 aPos;

// The model matrix transforms object to world space
uniform mat4 model;

// This is the *single* shadow matrix for THIS PASS
// (main.cpp will update this 6 times)
uniform mat4 shadowMatrix;

// World-space position (for distance calculation in fragment shader)
out vec3 FragPos;

void main()
{
    // World-space fragment position
    FragPos = vec3(model * vec4(aPos, 1.0));

    // Transform into the current cubemap face's projection-view space
    gl_Position = shadowMatrix * vec4(FragPos, 1.0);
}
