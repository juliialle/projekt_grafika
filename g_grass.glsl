#version 330 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 128) out;

uniform mat4 V, P;

in vec3 worldPos[];
in vec2 TexCoord[];

out vec2 geomTexCoord;

const int numBlades = 10;

uniform float time;

void generateGrass(vec3 base, vec2 baseUV, int i) {
    float bladeHeight = 0.2;
    float bladeWidth = 0.02;

    float windStrength = 0.05; // jak bardzo siê buja
    float windSpeed = 2.0;     // jak szybko siê buja

    float phase = float(i) * 10.0; // aby ka¿de ŸdŸb³o mia³o inn¹ fazê
    float sway = sin(time * windSpeed + phase) * windStrength;

    vec3 tip = base + vec3(sway, bladeHeight, 0.0); // czubek siê przesuwa
    vec3 left = base + vec3(-bladeWidth, 0.0, 0.0);
    vec3 right = base + vec3(bladeWidth, 0.0, 0.0);

    vec2 uvLeft = baseUV + vec2(-0.1, 0.0);
    vec2 uvRight = baseUV + vec2(0.1, 0.0);
    vec2 uvTip = baseUV + vec2(0.0, 1.0);

    geomTexCoord = uvLeft;
    gl_Position = P * V * vec4(left, 1.0);
    EmitVertex();

    geomTexCoord = uvRight;
    gl_Position = P * V * vec4(right, 1.0);
    EmitVertex();

    geomTexCoord = uvTip;
    gl_Position = P * V * vec4(tip, 1.0);
    EmitVertex();

    EndPrimitive();
}


void main() {
    // Rysujemy oryginalny trójk¹t siatki (¿eby by³a oteksturowana ca³a siatka)
    for (int i = 0; i < 3; ++i) {
        geomTexCoord = TexCoord[i] *10;
        gl_Position = P * V * vec4(worldPos[i], 1.0);
        EmitVertex();
    }
    EndPrimitive();

    // Teraz generujemy kêpki trawy na powierzchni
    for (int i = 0; i < numBlades; ++i) {
        float rand1 = fract(sin(dot(vec2(i, i + 1), vec2(12.9898, 78.233))) * 43758.5453);
        float rand2 = fract(sin(dot(vec2(i + 2, i + 3), vec2(93.9898, 67.345))) * 12345.6789);

        float sqrt_r1 = sqrt(rand1);
        float u = 1.0 - sqrt_r1;
        float v = rand2 * sqrt_r1;
        float w = 1.0 - u - v;

        vec3 base = u * worldPos[0] + v * worldPos[1] + w * worldPos[2];
        vec2 baseUV = u * TexCoord[0] + v * TexCoord[1] + w * TexCoord[2];
        baseUV *= 10.0; // Powtarzamy teksturê 10x na kêpce

        generateGrass(base, baseUV, i);
    }
}
