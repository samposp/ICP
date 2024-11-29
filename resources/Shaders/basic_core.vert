#version 460 core
in vec3 aPos;
in vec3 aNormal;
in vec2 aTex;

uniform mat4 uP_m = mat4(1.0);
uniform mat4 uM_m = mat4(1.0);
uniform mat4 uV_m = mat4(1.0);

out vec3 color;

void main()
{
    // Outputs the positions/coordinates of all vertices
    gl_Position = uP_m * uV_m * uM_m * vec4(aPos, 1.0f);
    color = aNormal * vec3(aTex, 1.0f);
}
