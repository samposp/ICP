#version 460 core

// (interpolated) input from previous pipeline stage
in VS_OUT {
    vec2 texcoord;
    vec3 N;
    vec3 L;
    vec3 V;
} fs_in;

// uniform variables
uniform sampler2D tex0; // texture unit from C++
uniform vec4 u_diffuse_color = vec4(1.0f); // object color for ambient and diffuse light
vec4 specular_material = vec4(1.0f);
// lights
uniform vec3 ambient_intensity, diffuse_intensity = vec3(0.0f), specular_intensity = vec3(1.0f); 
uniform float specular_shinines = 10;
// spotlight
uniform float cut_off;
uniform vec3 spotlight_direction;



// mandatory: final output color
out vec4 FragColor; 

// fog
vec4 fog_color = vec4(vec3(0.0f), 0.5f); // black, non-transparent = night
float near = 0.1f;
float far = 20.0f;

float log_depth(float depth, float steepness, float offset)
{
     float linear_depth = (2.0 * near * far) / (far + near - (depth * 2.0 - 1.0) * (far - near));
     return (1 / (1 + exp(-steepness * (linear_depth - offset))));
}

void main() {
    // Normalize the incoming N, L and V vectors
    vec3 N = normalize(fs_in.N);
    vec3 L = normalize(fs_in.L);
    vec3 V = normalize(fs_in.V);

    // Calculate R by reflecting -L around the plane defined by N
    vec3 R = reflect(-L, N);

    // calculate lights
    vec4 ambient = vec4(ambient_intensity, 1.0f) * u_diffuse_color;
    vec4 diffuse = max(dot(N, L), 0.0) * u_diffuse_color * vec4(diffuse_intensity, 1.0f);
    vec4 specular = pow(max(dot(R, V), 0.0), specular_shinines) * specular_material * vec4(specular_intensity, 1.0f);

    // calculate spotlight
    float theta = dot(normalize(spotlight_direction), - V);
    float spotlight_effect = smoothstep(cut_off, cut_off + 0.01, theta);
    vec4 spotlight_color = vec4(1.0, 0.9, 0.7, 1.0);

    if (theta > cut_off) {
        vec4 spotlight_diffuse = max(dot(N, V), 0.0) * u_diffuse_color * spotlight_color;
        diffuse += spotlight_diffuse * spotlight_effect;
    }



    // modulate texture with material color, including transparency
     vec4 color = (ambient + diffuse) * texture(tex0, fs_in.texcoord) + specular;
     float depth = log_depth(gl_FragCoord.z, 15.0f, 19.8f);
     FragColor = mix(color, fog_color, depth); //linear interpolation
}
