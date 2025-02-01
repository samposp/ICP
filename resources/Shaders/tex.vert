#version 460 core
in vec3 aPos;
in vec3 aNorm;
in vec2 aTex;

uniform mat4 uP_m = mat4(1.0f);
uniform mat4 uM_m = mat4(1.0f);
uniform mat4 uV_m = mat4(1.0f);
uniform vec3 camPos;

// Light properties
uniform vec3 light_position = vec3(10000.0f, 10000.0f, 0.0f);

uniform vec3 spotlight_direction;

out VS_OUT {
    vec2 texcoord;
    vec3 N;
    vec3 L;
    vec3 V;
    vec3 spotlight_direction;
} vs_out;

void main() {
    // Create Model-View matrix
    mat4 mv_m = uV_m * uM_m;

    // Calculate view-space coordinate - in P point 
    // we are computing the color
    vec4 P = mv_m * vec4(aPos, 1.0f);

    // Calculate normal in view space
    //vs_out.N = mat3(mv_m) * aNorm;
    vs_out.N = mat3(uM_m) * aNorm;

     // Calculate view-space light vector
    // vs_out.L = light_position - P.xyz;
    vs_out.L = (vec4(light_position, 0.0f) - ( uM_m * vec4(aPos, 1.0f))).xyz;

    vs_out.spotlight_direction = mat3(uM_m) * spotlight_direction;

    // Calculate view vector (negative of the view-space position)
    // vs_out.V = -P.xyz;

    vs_out.V = (vec4(camPos, 1.0f) - (uM_m * vec4(aPos, 1.0f))).xyz;


    vs_out.texcoord = aTex;

    // Outputs the positions/coordinates of all vertices
    gl_Position = uP_m * P;


}
