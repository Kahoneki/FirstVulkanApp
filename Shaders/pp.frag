#version 450

layout (location = 0) in vec2 TexCoord;

layout(set = 0, binding = 0, input_attachment_index = 0) uniform subpassInput sceneTexture;

layout(location = 0) out vec4 outColour;

void main()
{
    float vignetteRadius = 0.9;
    float vignetteSoftness = 0.6; //0 is a sharp edge, 1.0 is a very soft fade

    vec4 sceneColour = subpassLoad(sceneTexture);
    float dist = 1-distance(TexCoord, vec2(0.5, 0.5));
    float vignette = smoothstep(vignetteRadius, vignetteRadius - vignetteSoftness, dist);
    
    outColour = vec4(mix(sceneColour.rgb, vec3(0.0), vignette), sceneColour.a);
}