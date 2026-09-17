[[vk::binding(0, 0)]] RWTexture2D<float>  pressure;
[[vk::binding(1, 0)]] RWTexture2D<float>  velocityX;
[[vk::binding(5, 0)]] RWTexture2D<float>  oldVelocityX;
[[vk::binding(2, 0)]] RWTexture2D<float>  velocityY;
[[vk::binding(6, 0)]] RWTexture2D<float>  oldVelocityY;
[[vk::binding(4, 0)]] [[vk::image_format("rgba8")]] RWTexture2D<float4> smoke;
[[vk::binding(7, 0)]] [[vk::image_format("rgba8")]] RWTexture2D<float4> oldSmoke;

[[vk::binding(3, 0)]] RWTexture2D<float4> drawImage;

static const uint VISUALIZE_PRESSURE   = 0u;
static const uint VISUALIZE_VELOCITY   = 1u;
static const uint VISUALIZE_DIVERGENCE = 2u;
static const uint VISUALIZE_SMOKE      = 3u;

static const uint BRUSH_SMOKE    = 0u;
static const uint BRUSH_PRESSURE = 1u;

struct Constants {
    float  deltaTime;
    float  density;
    float2 brushPos;
    int    redBlackIdx;
    float  brushSize;
    int2   simBounds;
    bool   brushDown;
    uint   brushType;
    uint   visType;
    float  visScale;
};

[[vk::push_constant]]
ConstantBuffer<Constants> constants;

float getDivergence(int2 coord) {
    float u_right = velocityX[coord + int2(1, 0)];
    float u_left  = velocityX[coord];
    float v_up    = velocityY[coord + int2(0, 1)];
    float v_down  = velocityY[coord];

    return (u_right - u_left) + (v_up - v_down);
}

float sampleVelocityX(RWTexture2D<float> image, float2 pos, float2 offset) {
    pos -= offset;
    
    int2 bottomLeftCoord  = int2(floor(pos));
    int2 bottomRightCoord = int2(bottomLeftCoord.x + 1, bottomLeftCoord.y);
    int2 topLeftCoord     = int2(bottomLeftCoord.x, bottomLeftCoord.y + 1);
    int2 topRightCoord    = int2(topLeftCoord.x + 1, topLeftCoord.y + 1);

    pos = frac(pos);

    float bottomLeftV  = (bottomLeftCoord.x >= 0 && bottomLeftCoord.x <= constants.simBounds.x &&
                          bottomLeftCoord.y >= 0 && bottomLeftCoord.y < constants.simBounds.y) ?
                          image[bottomLeftCoord] : 0.0;
    float bottomRightV = (bottomRightCoord.x >= 0 && bottomRightCoord.x <= constants.simBounds.x &&
                          bottomRightCoord.y >= 0 && bottomRightCoord.y < constants.simBounds.y) ?
                          image[bottomRightCoord] : 0.0;
    float topLeftV     = (topLeftCoord.x >= 0 && topLeftCoord.x <= constants.simBounds.x &&
                          topLeftCoord.y >= 0 && topLeftCoord.y < constants.simBounds.y) ? 
                          image[topLeftCoord] : 0.0;
    float topRightV    = (topRightCoord.x >= 0 && topRightCoord.x <= constants.simBounds.x &&
                          topRightCoord.y >= 0 && topRightCoord.y < constants.simBounds.y) ?
                          image[topRightCoord] : 0.0;

    return lerp(
        lerp(bottomLeftV, bottomRightV, pos.x),
        lerp(topLeftV,    topRightV,    pos.x),
        pos.y
    );
}

float sampleVelocityY(RWTexture2D<float> image, float2 pos, float2 offset) {
    pos -= offset;
    
    int2 bottomLeftCoord  = int2(floor(pos));
    int2 bottomRightCoord = int2(bottomLeftCoord.x + 1, bottomLeftCoord.y);
    int2 topLeftCoord     = int2(bottomLeftCoord.x, bottomLeftCoord.y + 1);
    int2 topRightCoord    = int2(topLeftCoord.x + 1, topLeftCoord.y + 1);

    pos = frac(pos);

    float bottomLeftV  = (bottomLeftCoord.x >= 0 && bottomLeftCoord.x < constants.simBounds.x &&
                          bottomLeftCoord.y >= 0 && bottomLeftCoord.y <= constants.simBounds.y) ?
                          image[bottomLeftCoord] : 0.0;
    float bottomRightV = (bottomRightCoord.x >= 0 && bottomRightCoord.x < constants.simBounds.x &&
                          bottomRightCoord.y >= 0 && bottomRightCoord.y <= constants.simBounds.y) ?
                          image[bottomRightCoord] : 0.0;
    float topLeftV     = (topLeftCoord.x >= 0 && topLeftCoord.x < constants.simBounds.x &&
                          topLeftCoord.y >= 0 && topLeftCoord.y <= constants.simBounds.y) ? 
                          image[topLeftCoord] : 0.0;
    float topRightV    = (topRightCoord.x >= 0 && topRightCoord.x < constants.simBounds.x &&
                          topRightCoord.y >= 0 && topRightCoord.y <= constants.simBounds.y) ?
                          image[topRightCoord] : 0.0;

    return lerp(
        lerp(bottomLeftV, bottomRightV, pos.x),
        lerp(topLeftV,    topRightV,    pos.x),
        pos.y
    );
}

float4 sampleProperty(RWTexture2D<float4> image, float2 pos, float2 offset) {
    pos -= offset;
    
    int2 bottomLeftCoord  = int2(floor(pos));
    int2 bottomRightCoord = int2(bottomLeftCoord.x + 1, bottomLeftCoord.y);
    int2 topLeftCoord     = int2(bottomLeftCoord.x, bottomLeftCoord.y + 1);
    int2 topRightCoord    = int2(topLeftCoord.x + 1, topLeftCoord.y + 1);

    pos = frac(pos);

    float4 bottomLeftV  = (bottomLeftCoord.x >= 0 && bottomLeftCoord.x < constants.simBounds.x &&
                           bottomLeftCoord.y >= 0 && bottomLeftCoord.y < constants.simBounds.y) ?
                           image[bottomLeftCoord] : float4(0.0, 0.0, 0.0, 0.0);
    float4 bottomRightV = (bottomRightCoord.x >= 0 && bottomRightCoord.x < constants.simBounds.x &&
                           bottomRightCoord.y >= 0 && bottomRightCoord.y < constants.simBounds.y) ?
                           image[bottomRightCoord] : float4(0.0, 0.0, 0.0, 0.0);
    float4 topLeftV     = (topLeftCoord.x >= 0 && topLeftCoord.x < constants.simBounds.x &&
                           topLeftCoord.y >= 0 && topLeftCoord.y < constants.simBounds.y) ? 
                           image[topLeftCoord] : float4(0.0, 0.0, 0.0, 0.0);
    float4 topRightV    = (topRightCoord.x >= 0 && topRightCoord.x < constants.simBounds.x &&
                           topRightCoord.y >= 0 && topRightCoord.y < constants.simBounds.y) ?
                           image[topRightCoord] : float4(0.0, 0.0, 0.0, 0.0);

    return lerp(
        lerp(bottomLeftV, bottomRightV, pos.x),
        lerp(topLeftV,    topRightV,    pos.x),
        pos.y
    );
}

#ifdef KERNEL_BRUSH
[numthreads(16, 16, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    int2 coord = int2(id.xy);
    if (coord.x >= constants.simBounds.x || coord.y >= constants.simBounds.y)
        return;

    if (constants.brushDown && length(float2(coord) - constants.brushPos) <= constants.brushSize) {
        switch (constants.brushType) {
            case BRUSH_PRESSURE:
            {
                velocityX[coord] = 1.0;
                break;
            }
            case BRUSH_SMOKE:
            {
                smoke[coord] = float4(1.0, 1.0, 1.0, 0.0);
                break;
            }
        }
    }
}
#endif

#ifdef KERNEL_ADVECT
[numthreads(16, 16, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    int2 coord = int2(id.xy);
    if (coord.x >= constants.simBounds.x || coord.y >= constants.simBounds.y)
        return;

    // 1. Advect Left-edge Velocity (u)
    float2 leftEdgePos = float2(coord.x, coord.y + 0.5);
    float2 leftEdgeVelocity = float2(
        sampleVelocityX(oldVelocityX, leftEdgePos, float2(0.0, 0.5)),
        sampleVelocityY(oldVelocityY, leftEdgePos, float2(0.5, 0.0))
    );
    float2 leftEdgePrevPos = leftEdgePos - constants.deltaTime * leftEdgeVelocity;
    velocityX[coord] = sampleVelocityX(oldVelocityX, leftEdgePrevPos, float2(0.0, 0.5));

    // 2. Advect Bottom-edge Velocity (v)
    float2 bottomEdgePos = float2(coord.x + 0.5, coord.y);
    float2 bottomEdgeVelocity = float2(
        sampleVelocityX(oldVelocityX, bottomEdgePos, float2(0.0, 0.5)),
        sampleVelocityY(oldVelocityY, bottomEdgePos, float2(0.5, 0.0))
    );
    float2 bottomEdgePrevPos = bottomEdgePos - constants.deltaTime * bottomEdgeVelocity;
    velocityY[coord] = sampleVelocityY(oldVelocityY, bottomEdgePrevPos, float2(0.5, 0.0));

    // Domain edge boundaries
    if (coord.x == constants.simBounds.x - 1) {
        float2 rightEdgePos = float2(coord.x + 1.0, coord.y + 0.5);
        float2 rightEdgeVelocity = float2(
            sampleVelocityX(oldVelocityX, rightEdgePos, float2(0.0, 0.5)),
            sampleVelocityY(oldVelocityY, rightEdgePos, float2(0.5, 0.0))
        );
        float2 rightEdgePrevPos = rightEdgePos - constants.deltaTime * rightEdgeVelocity;
        velocityX[int2(coord.x + 1, coord.y)] = sampleVelocityX(oldVelocityX, rightEdgePrevPos, float2(0.0, 0.5));
    }
    if (coord.y == constants.simBounds.y - 1) {
        float2 topEdgePos = float2(coord.x + 0.5, coord.y + 1.0);
        float2 topEdgeVelocity = float2(
            sampleVelocityX(oldVelocityX, topEdgePos, float2(0.0, 0.5)),
            sampleVelocityY(oldVelocityY, topEdgePos, float2(0.5, 0.0))
        );
        float2 topEdgePrevPos = topEdgePos - constants.deltaTime * topEdgeVelocity;
        velocityY[int2(coord.x, coord.y + 1)] = sampleVelocityY(oldVelocityY, topEdgePrevPos, float2(0.5, 0.0));
    }

    // 3. Advect Smoke (Center)
    float2 centerPos = float2(coord.x + 0.5, coord.y + 0.5);
    float2 centerVelocity = float2(
        sampleVelocityX(oldVelocityX, centerPos, float2(0.0, 0.5)),
        sampleVelocityY(oldVelocityY, centerPos, float2(0.5, 0.0))
    );
    float2 centerPrevPos = centerPos - constants.deltaTime * centerVelocity;
    smoke[coord] = sampleProperty(oldSmoke, centerPrevPos, float2(0.5, 0.5));
}
#endif

#ifdef KERNEL_PROJECT
[numthreads(16, 16, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    int2 coord = int2(id.xy);
    if (coord.x >= constants.simBounds.x || coord.y >= constants.simBounds.y || ((coord.x + coord.y) % 2 != constants.redBlackIdx))
        return;

    bool leftExist  = (coord.x != 0);
    bool rightExist = (coord.x != constants.simBounds.x - 1);
    bool downExist  = (coord.y != 0);
    bool upExist    = (coord.y != constants.simBounds.y - 1);

    int neighbourNum = int(leftExist) + int(rightExist) + int(upExist) + int(downExist);
    if (neighbourNum == 0) return;

    float sumP = (leftExist  ? pressure[int2(coord.x - 1, coord.y)] : 0.0)
               + (rightExist ? pressure[int2(coord.x + 1, coord.y)] : 0.0)
               + (downExist  ? pressure[int2(coord.x, coord.y - 1)] : 0.0)
               + (upExist    ? pressure[int2(coord.x, coord.y + 1)] : 0.0);

    float uLeft  = velocityX[coord];
    float uRight = velocityX[coord + int2(1, 0)];
    float uDown  = velocityY[coord];
    float uUp    = velocityY[coord + int2(0, 1)];

    float div = (uRight - uLeft) + (uUp - uDown);
    float rhs = -(constants.density / constants.deltaTime) * div;

    pressure[coord] = (rhs + sumP) / float(neighbourNum);
}
#endif

#ifdef KERNEL_UPDATE_VELOCITIES
[numthreads(16, 16, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    int2 coord = int2(id.xy);
    if (coord.x >= constants.simBounds.x || coord.y >= constants.simBounds.y)
        return;

    float pCenter = pressure[coord];

    // Update bottom face (uDown)
    bool downExist = (coord.y != 0);
    if (downExist) {
        float pDown = pressure[int2(coord.x, coord.y - 1)];
        velocityY[coord] = velocityY[coord] - (constants.deltaTime / constants.density) * (pCenter - pDown);
    } else {
        velocityY[coord] = 0.0; // Solid bottom wall
    }

    // Update left face (uLeft)
    bool leftExist = (coord.x != 0);
    if (leftExist) {
        float pLeft = pressure[int2(coord.x - 1, coord.y)];
        velocityX[coord] = velocityX[coord] - (constants.deltaTime / constants.density) * (pCenter - pLeft);
    } else {
        velocityX[coord] = 0.0; // Solid left wall
    }

    // Domain outer edges (Right and Top walls)
    if (coord.x == constants.simBounds.x - 1) {
        velocityX[int2(coord.x + 1, coord.y)] = 0.0; // Solid right wall
    }
    if (coord.y == constants.simBounds.y - 1) {
        velocityY[int2(coord.x, coord.y + 1)] = 0.0; // Solid top wall
    }
}
#endif

float3 hsvToRgb(float h, float s, float v) {
    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    float3 p = abs(frac(h + K.xyz) * 6.0 - K.www);
    return v * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), s);
}

#ifdef KERNEL_VISUALIZE
[numthreads(16, 16, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    int2 coord = int2(id.xy);
    if (coord.x >= constants.simBounds.x || coord.y >= constants.simBounds.y)
        return;

    float4 outColor = float4(0.0, 0.0, 0.0, 1.0);

    switch (constants.visType) {
        case VISUALIZE_PRESSURE:
        {
            float p = pressure[coord] / constants.visScale;
            outColor = (p >= 0.0) ? float4(p, 0.0, 0.0, 1.0) : float4(0.0, 0.0, -p, 1.0);
            break;
        }
        case VISUALIZE_VELOCITY:
        {
            float uRight = velocityX[coord + int2(1, 0)];
            float vUp    = velocityY[coord + int2(0, 1)];
            float u = 0.5 * (velocityX[coord] + uRight);
            float v = 0.5 * (velocityY[coord] + vUp);

            float speed = length(float2(u, v));
            float angle = atan2(v, u);
            float hue = frac((angle / 6.283185307) + 1.0);
            float intensity = saturate(speed / max(constants.visScale, 0.0001));

            outColor = float4(hsvToRgb(hue, 1.0, intensity), 1.0);
            break;
        }
        case VISUALIZE_DIVERGENCE:
        {
            float divergence = getDivergence(coord) / constants.visScale;
            outColor = (divergence >= 0.0) ? float4(divergence, 0.0, 0.0, 1.0) : float4(0.0, 0.0, -divergence, 1.0);
            break;
        }
        case VISUALIZE_SMOKE:
        {
            outColor = float4(smoke[coord].xyz / constants.visScale, 1.0);
            break;
        }
    }

    drawImage[int2(coord.x, constants.simBounds.y - 1 - coord.y)] = outColor;
}
#endif