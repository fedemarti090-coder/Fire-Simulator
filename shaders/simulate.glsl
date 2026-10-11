#version 430 core

layout (local_size_x = 16, local_size_y = 16) in;
layout (rgba8, binding = 0) uniform image2D imgOutput;

uniform float u_time;

void main (){
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 dims = imageSize(imgOutput);

    if (pixelCoord.x < dims.x && pixelCoord.y < dims.y){
        vec2 uv = vec2(pixelCoord) / vec2(dims);
        float x = uv.x;
        float y = uv.y;

        vec2 point = vec2(0.5, 0.05);

        float noise = sin(x * 12.0 + u_time * 5.0) * 0.03 
                    + cos(y * 20.0 - u_time * 8.0) * 0.03;

        vec2 distortedPoint = point + vec2(0.0, noise);

        float rawDist = sqrt(pow(x - distortedPoint.x, 2) * 1.5 + pow(y - distortedPoint.y, 2) * 0.5);

        float radius = 0.25;
        float intensity = 1.0 - (rawDist / radius);
        intensity = clamp(intensity, 0.0, 1.0);
        
        intensity = pow(intensity, 2.0);

        // COLORI
        vec3 colBlack  = vec3(0.0, 0.0, 0.0);
        vec3 colRed    = vec3(0.9, 0.1, 0.0);
        vec3 colYellow = vec3(1.0, 0.5, 0.05);
        vec3 colWhite  = vec3(1.0, 0.95, 0.8); 

        vec3 finalColor = colBlack;

        if (intensity > 0.0) {
            if (intensity < 0.35) {
                finalColor = mix(colBlack, colRed, intensity / 0.35);
            } else if (intensity < 0.75) {
                finalColor = mix(colRed, colYellow, (intensity - 0.35) / 0.4);
            } else {
                finalColor = mix(colYellow, colWhite, (intensity - 0.75) / 0.25);
            }
        }

        imageStore(imgOutput, pixelCoord, vec4(finalColor, 1.0f));
    }
}