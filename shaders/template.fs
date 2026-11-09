#version 330 core

uniform sampler2D tex_2d;
uniform vec2 uResolution;
uniform vec2 uViewMin;
uniform vec2 uViewMax;
uniform float uN;
uniform float uTime;
uniform float uRotSpeed;
uniform float uColor;

out vec4 out_col;

const float PI = 3.14159265;

float map(float v, float a, float b, float c, float d) {
    return c + (v - a) / (b - a) * (d - c);
}

vec2 rotate(vec2 p, float a) {
    float c = cos(a);
    float s = sin(a);
    return vec2(c * p.x - s * p.y, s * p.x + c * p.y);
}

vec2 kale(vec2 p, float n) {
    float a = atan(p.y, p.x);
    float r = length(p);
    a = abs(mod(a, 2.0 * PI / n) - PI / n);
    return r * vec2(cos(a), sin(a));
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec2 p;
    p.x = map(uv.x, 0.0, 1.0, uViewMin.x, uViewMax.x);
    p.y = map(uv.y, 0.0, 1.0, uViewMin.y, uViewMax.y);

    p = rotate(p, uTime * uRotSpeed);

    vec2 k = kale(p, uN);

    vec2 texUV = k * 0.5 + 0.5;
    texUV.y = 1.0 - texUV.y;
    vec3 tex = texture(tex_2d, texUV).rgb;

    float hue = atan(k.y, k.x) / (2.0 * PI) + 0.5 + uTime * 0.05;
    vec3 tint = hsv2rgb(vec3(fract(hue), 0.75, 1.0));
    vec3 colored = tex * tint;

    out_col = vec4(mix(tex, colored, uColor), 1.0);
}
