#version 330 core

uniform vec2 uResolution;
uniform vec2 uViewMin;
uniform vec2 uViewMax;
uniform float uN;
uniform float uTime;
uniform float uRotSpeed;

out vec4 out_col;

void main() {
    float pixelX = gl_FragCoord.x;
    float pixelY = gl_FragCoord.y;
    float uvx = pixelX / uResolution.x;
    float uvy = pixelY / uResolution.y;

    float zr = uViewMin.x + (uvx - 0.0) / (1.0 - 0.0) * (uViewMax.x - uViewMin.x);
    float zi = uViewMin.y + (uvy - 0.0) / (1.0 - 0.0) * (uViewMax.y - uViewMin.y);

    vec2 z1;
    z1.x = zi;
    z1.y = zr;

    vec2 z2;
    z2.x = 0.0;
    z2.y = 0.0;

    float cReal = 0.5667;
    float cImag = 0.0;

    float angleX = uTime * 0.4;
    float angleY = uTime * 0.35;
    float sinVal = sin(angleX);
    float cosVal = cos(angleY);
    float amount = uRotSpeed * 0.15;

    float pReal = -0.5 + amount * sinVal;
    float pImag = 0.0 + amount * cosVal;

    int NUMITR = 200;
    float MAXZ = 4.0;

    int count = 0;

    for (int i = 0; i <= NUMITR; i++) {
        float lengthSquared = z1.x * z1.x + z1.y * z1.y;
        if (lengthSquared >= MAXZ) {
            break;
        }
        float squareReal = z1.x * z1.x - z1.y * z1.y;
        float squareImag = 2.0 * z1.x * z1.y;
        float multReal = pReal * z2.x - pImag * z2.y;
        float multImag = pReal * z2.y + pImag * z2.x;
        vec2 z;
        z.x = squareReal + cReal + multReal;
        z.y = squareImag + cImag + multImag;

        z2 = z1;
        z1 = z;
        count = i + 1;
    }

    if (count > NUMITR) {
        out_col = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        float k = uN;
        if (k < 1.0) {
            k = 1.0;
        }

        float value = float(count) * k * 0.01;
        float t = value - floor(value);

        float red = t;
        float green = t * 0.5;
        float blue = 1.0 - t;

        out_col = vec4(red, green, blue, 1.0);
    }
}