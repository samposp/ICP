#version 460 core

// (interpolated) input from previous pipeline stage
in VS_OUT {
    vec2 texcoord;
} fs_in;

// uniform variables
uniform sampler2D tex0; // texture unit from C++
uniform vec4 u_diffuse_color = vec4(1.0f);

// mandatory: final output color
out vec4 FragColor; 

// fog
uniform vec4 fog_color = vec4(vec3(0.0f), 0.5f); // black, non-transparent = night
uniform float near = 0.1f;
uniform float far = 20.0f;

float log_depth(float depth, float steepness, float offset)
{
     float linear_depth = (2.0 * near * far) / (far + near - (depth * 2.0 - 1.0) * (far - near));
     return (1 / (1 + exp(-steepness * (linear_depth - offset))));
}

void main() {
    // modulate texture with material color, including transparency
     vec4 color = u_diffuse_color * texture(tex0, fs_in.texcoord);

     float depth = log_depth(gl_FragCoord.z, 15.0f, 19.8f);
     FragColor = mix(color, fog_color, depth); //linear interpolation
}
