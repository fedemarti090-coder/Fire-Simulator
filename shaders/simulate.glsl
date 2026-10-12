#version 430 core

layout (local_size_x = 16, local_size_y = 16) in;
layout (binding = 0) uniform sampler2D u_readTex;
layout (rgba16f, binding = 0) uniform image2D imgOutput;

uniform float u_time;

void main () {
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 dims = textureSize(u_readTex, 0);

    if(pixelCoord.x < dims.x && pixelCoord.y < dims.y){

        vec2 uv = vec2(pixelCoord) / vec2(dims);

        vec4 currentData = texelFetch(u_readTex, pixelCoord, 0);
        vec2 velocity = currentData.rg;
        float temperature = currentData.b;
        float density = currentData.a;

        velocity.x = 0.0;

        vec2 backUv = uv - velocity * 0.001; // PASSO TEMPORALE
        vec4 oldData = texture(u_readTex, backUv);

        temperature = oldData.b;
        density = oldData.a;
        vec2 advecVel = oldData.rg;
        advecVel.x = 0.0;

        // SORGENTE
        
        vec2 sourcePoint = vec2(0.5, 0.05);
        float dist = distance(uv, sourcePoint);
        if (dist < 0.04) {
            temperature = 1.0;
            density = 1.0;
            advecVel.y = 0.02;
        }

        // FISICA
        temperature -= 0.001;
        temperature = clamp(temperature, 0.0, 1.0);

        density -= 0.003;
        density = clamp(density, 0.0, 1.0);

        advecVel.y += temperature * 0.005;
        advecVel *= 0.98;

        vec4 newData = vec4(advecVel, temperature, density);
        imageStore(imgOutput, pixelCoord, newData);
    }
}