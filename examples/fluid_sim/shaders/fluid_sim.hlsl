[[vk::binding(0, 0)]] RWTexture2D<float> pressure;
[[vk::binding(1, 0)]] RWTexture2D<float> velocityX;
[[vk::binding(5, 0)]] RWTexture2D<float> oldVelocityX;
[[vk::binding(2, 0)]] RWTexture2D<float> velocityY;
[[vk::binding(6, 0)]] RWTexture2D<float> oldVelocityY;
[[vk::binding(8, 0)]] RWTexture2D<float> divergence;
[[vk::binding(9, 0)]] [[vk::image_format("r8ui")]] RWTexture2D<uint> solidity;
[[vk::binding(4, 0)]] [[vk::image_format("rgba16f")]] RWTexture2D<float4> smoke;
[[vk::binding(7, 0)]] [[vk::image_format("rgba16f")]]     RWTexture2D<float4>   oldSmoke;

[[vk::binding(3, 0)]] RWTexture2D<float4> drawImage;

static const uint VISUALIZE_PRESSURE   = 0u;
static const uint VISUALIZE_VELOCITY   = 1u;
static const uint VISUALIZE_DIVERGENCE = 2u;
static const uint VISUALIZE_SMOKE      = 3u;

static const uint BRUSH_SMOKE    = 0u;
static const uint BRUSH_PRESSURE = 1u;
static const uint BRUSH_SOLID = 2u;

static const uint CELL_AIR = 0u;
static const uint CELL_SOLID = 1u;
static const uint CELL_SMOKE_SOURCE = 2u;
static const uint CELL_VELOCITY_SOURCE = 3u;
static const uint CELL_PRESSURE_SOURCE = 4u;

struct Constants {
    float  deltaTime;
    float  density;
    float2 brushPos;
    float2 brushDelta;
    int    redBlackIdx;
    float  brushSize;
    int2   simBounds;
    bool   brushDown;
    uint   brushType;
    float3 brushColor;
    uint   visType;
    int2   drawBound;
    float  visScale;
    float  overRelaxation;
    float  smokeDiffusion;
    float  cellSize;
    bool   openEdges;
    uint   cellType;
    float4 cellDetails;
};

[[vk::push_constant]]
ConstantBuffer<Constants> constants;


//    +----------------------------------------------------+
//    |                      samplers                      |
//    +----------------------------------------------------+

float sampleVelocityXClamped(RWTexture2D<float> image, float2 pos, float2 offset) {
    pos -= offset;

    int2 boundsTopLeft = int2(0, 0);
    int2 boundsBottomRight = int2(constants.simBounds.x, constants.simBounds.y-1);

    int2 bottomLeftCoord  = int2(floor(pos));
    int2 bottomRightCoord = int2(bottomLeftCoord.x + 1, bottomLeftCoord.y);
    int2 topLeftCoord     = int2(bottomLeftCoord.x, bottomLeftCoord.y + 1);
    int2 topRightCoord    = int2(bottomLeftCoord.x + 1, bottomLeftCoord.y + 1);

    bottomLeftCoord = clamp(bottomLeftCoord, boundsTopLeft, boundsBottomRight);
    bottomRightCoord = clamp(bottomRightCoord, boundsTopLeft, boundsBottomRight);
    topLeftCoord = clamp(topLeftCoord, boundsTopLeft, boundsBottomRight);
    topRightCoord = clamp(topRightCoord, boundsTopLeft, boundsBottomRight);

    pos = frac(pos);

    float topLeftV     = image[topLeftCoord];
    float topRightV    = image[topRightCoord];
    float bottomLeftV  = image[bottomLeftCoord];
    float bottomRightV = image[bottomRightCoord];

    return lerp(
        lerp(bottomLeftV, bottomRightV, pos.x),
        lerp(topLeftV,    topRightV,    pos.x),
        pos.y
    );
}

float sampleVelocityYClamped(RWTexture2D<float> image, float2 pos, float2 offset) {
    pos -= offset;
    
    int2 boundsTopLeft = int2(0, 0);
    int2 boundsBottomRight = int2(constants.simBounds.x - 1, constants.simBounds.y);

    int2 bottomLeftCoord  = int2(floor(pos));
    int2 bottomRightCoord = int2(bottomLeftCoord.x + 1, bottomLeftCoord.y);
    int2 topLeftCoord     = int2(bottomLeftCoord.x, bottomLeftCoord.y + 1);
    int2 topRightCoord    = int2(bottomLeftCoord.x + 1, bottomLeftCoord.y + 1);

    bottomLeftCoord = clamp(bottomLeftCoord, boundsTopLeft, boundsBottomRight);
    bottomRightCoord = clamp(bottomRightCoord, boundsTopLeft, boundsBottomRight);
    topLeftCoord = clamp(topLeftCoord, boundsTopLeft, boundsBottomRight);
    topRightCoord = clamp(topRightCoord, boundsTopLeft, boundsBottomRight);

    pos = frac(pos);

    float topLeftV     = image[topLeftCoord];
    float topRightV    = image[topRightCoord];
    float bottomLeftV  = image[bottomLeftCoord];
    float bottomRightV = image[bottomRightCoord];

    return lerp(
        lerp(bottomLeftV, bottomRightV, pos.x),
        lerp(topLeftV,    topRightV,    pos.x),
        pos.y
    );
}
float sampleVelocityX(RWTexture2D<float> image, float2 pos) {
    pos /= constants.cellSize;
    pos.y -= 0.5;
    
    int2 bottomLeftCoord  = int2(floor(pos));
    int2 bottomRightCoord = int2(bottomLeftCoord.x + 1, bottomLeftCoord.y);
    int2 topLeftCoord     = int2(bottomLeftCoord.x, bottomLeftCoord.y + 1);
    int2 topRightCoord    = int2(bottomLeftCoord.x + 1, bottomLeftCoord.y + 1);

    pos = frac(pos);

    float topLeftV     = ((topLeftCoord.x >= 0 && topLeftCoord.x <= constants.simBounds.x) &&
                          (topLeftCoord.y >= 0 && topLeftCoord.y < constants.simBounds.y)) ? 
                          image[topLeftCoord] : 0.0;
    float topRightV    = ((topRightCoord.x >= 0 && topRightCoord.x <= constants.simBounds.x) &&
                          (topRightCoord.y >= 0 && topRightCoord.y < constants.simBounds.y)) ?
                          image[topRightCoord] : 0.0;
    float bottomLeftV  = ((bottomLeftCoord.x >= 0 && bottomLeftCoord.x <= constants.simBounds.x) &&
                          (bottomLeftCoord.y >= 0 && bottomLeftCoord.y < constants.simBounds.y)) ?
                          image[bottomLeftCoord] : 0.0;
    float bottomRightV = ((bottomRightCoord.x >= 0 && bottomRightCoord.x <= constants.simBounds.x) &&
                          (bottomRightCoord.y >= 0 && bottomRightCoord.y < constants.simBounds.y)) ?
                          image[bottomRightCoord] : 0.0;

    return lerp(
        lerp(bottomLeftV, bottomRightV, pos.x),
        lerp(topLeftV,    topRightV,    pos.x),
        pos.y
    );
}
float sampleVelocityY(RWTexture2D<float> image, float2 pos) {
    pos /= constants.cellSize;
    pos.x -= 0.5;

    int2 bottomLeftCoord  = int2(floor(pos));
    int2 bottomRightCoord = int2(bottomLeftCoord.x + 1, bottomLeftCoord.y);
    int2 topLeftCoord     = int2(bottomLeftCoord.x, bottomLeftCoord.y + 1);
    int2 topRightCoord    = int2(bottomLeftCoord.x + 1, bottomLeftCoord.y + 1);

    pos = frac(pos);

    float topLeftV     = ((topLeftCoord.x >= 0 && topLeftCoord.x < constants.simBounds.x) &&
                          (topLeftCoord.y >= 0 && topLeftCoord.y <= constants.simBounds.y)) ? 
                          image[topLeftCoord] : 0.0;
    float topRightV    = ((topRightCoord.x >= 0 && topRightCoord.x < constants.simBounds.x) &&
                          (topRightCoord.y >= 0 && topRightCoord.y <= constants.simBounds.y)) ?
                          image[topRightCoord] : 0.0;
    float bottomLeftV  = ((bottomLeftCoord.x >= 0 && bottomLeftCoord.x < constants.simBounds.x) &&
                          (bottomLeftCoord.y >= 0 && bottomLeftCoord.y <= constants.simBounds.y)) ?
                          image[bottomLeftCoord] : 0.0;
    float bottomRightV = ((bottomRightCoord.x >= 0 && bottomRightCoord.x < constants.simBounds.x) &&
                          (bottomRightCoord.y >= 0 && bottomRightCoord.y <= constants.simBounds.y)) ?
                          image[bottomRightCoord] : 0.0;

    return lerp(
        lerp(bottomLeftV, bottomRightV, pos.x),
        lerp(topLeftV,    topRightV,    pos.x),
        pos.y
    );
}
float4 sampleProperty(RWTexture2D<float4> image, float2 pos) {
    pos /= constants.cellSize;
    pos -= float2(0.5, 0.5);
    
    int2 bottomLeftCoord  = int2(floor(pos));
    int2 bottomRightCoord = int2(bottomLeftCoord.x + 1, bottomLeftCoord.y);
    int2 topLeftCoord     = int2(bottomLeftCoord.x, bottomLeftCoord.y + 1);
    int2 topRightCoord    = int2(bottomLeftCoord.x + 1, bottomLeftCoord.y + 1);

    pos = frac(pos);

    float4 topLeftV     = ((topLeftCoord.x >= 0 && topLeftCoord.x < constants.simBounds.x) &&
                          (topLeftCoord.y >= 0 && topLeftCoord.y < constants.simBounds.y)) ? 
                          image[topLeftCoord] : float4(0.0, 0.0, 0.0, 0.0);
    float4 topRightV    = ((topRightCoord.x >= 0 && topRightCoord.x < constants.simBounds.x) &&
                          (topRightCoord.y >= 0 && topRightCoord.y < constants.simBounds.y)) ?
                          image[topRightCoord] : float4(0.0, 0.0, 0.0, 0.0);
    float4 bottomLeftV  = ((bottomLeftCoord.x >= 0 && bottomLeftCoord.x < constants.simBounds.x) &&
                          (bottomLeftCoord.y >= 0 && bottomLeftCoord.y < constants.simBounds.y)) ?
                          image[bottomLeftCoord] : float4(0.0, 0.0, 0.0, 0.0);
    float4 bottomRightV = ((bottomRightCoord.x >= 0 && bottomRightCoord.x < constants.simBounds.x) &&
                          (bottomRightCoord.y >= 0 && bottomRightCoord.y < constants.simBounds.y)) ?
                          image[bottomRightCoord] : float4(0.0, 0.0, 0.0, 0.0);

    return lerp(
        lerp(bottomLeftV, bottomRightV, pos.x),
        lerp(topLeftV,    topRightV,    pos.x),
        pos.y
    );
}
float sampleProperty1(RWTexture2D<float> image, float2 pos) {
    pos /= constants.cellSize;
    pos -= float2(0.5, 0.5);
    
    int2 bottomLeftCoord  = int2(floor(pos));
    int2 bottomRightCoord = int2(bottomLeftCoord.x + 1, bottomLeftCoord.y);
    int2 topLeftCoord     = int2(bottomLeftCoord.x, bottomLeftCoord.y + 1);
    int2 topRightCoord    = int2(bottomLeftCoord.x + 1, bottomLeftCoord.y + 1);

    pos = frac(pos);

    float topLeftV     = ((topLeftCoord.x >= 0 && topLeftCoord.x < constants.simBounds.x) &&
                          (topLeftCoord.y >= 0 && topLeftCoord.y < constants.simBounds.y)) ? 
                          image[topLeftCoord] : 0.0;
    float topRightV    = ((topRightCoord.x >= 0 && topRightCoord.x < constants.simBounds.x) &&
                          (topRightCoord.y >= 0 && topRightCoord.y < constants.simBounds.y)) ?
                          image[topRightCoord] : 0.0;
    float bottomLeftV  = ((bottomLeftCoord.x >= 0 && bottomLeftCoord.x < constants.simBounds.x) &&
                          (bottomLeftCoord.y >= 0 && bottomLeftCoord.y < constants.simBounds.y)) ?
                          image[bottomLeftCoord] : 0.0;
    float bottomRightV = ((bottomRightCoord.x >= 0 && bottomRightCoord.x < constants.simBounds.x) &&
                          (bottomRightCoord.y >= 0 && bottomRightCoord.y < constants.simBounds.y)) ?
                          image[bottomRightCoord] : 0.0;

    return lerp(
        lerp(bottomLeftV, bottomRightV, pos.x),
        lerp(topLeftV,    topRightV,    pos.x),
        pos.y
    );
}
float sampleProperty2(RWTexture2D<uint> image, float2 pos) {
    pos /= constants.cellSize;
    pos -= float2(0.5, 0.5);
    
    int2 bottomLeftCoord  = int2(floor(pos));
    int2 bottomRightCoord = int2(bottomLeftCoord.x + 1, bottomLeftCoord.y);
    int2 topLeftCoord     = int2(bottomLeftCoord.x, bottomLeftCoord.y + 1);
    int2 topRightCoord    = int2(bottomLeftCoord.x + 1, bottomLeftCoord.y + 1);

    pos = frac(pos);

    float topLeftV     = ((topLeftCoord.x >= 0 && topLeftCoord.x < constants.simBounds.x) &&
                          (topLeftCoord.y >= 0 && topLeftCoord.y < constants.simBounds.y)) ? 
                          image[topLeftCoord] : 0.0;
    float topRightV    = ((topRightCoord.x >= 0 && topRightCoord.x < constants.simBounds.x) &&
                          (topRightCoord.y >= 0 && topRightCoord.y < constants.simBounds.y)) ?
                          image[topRightCoord] : 0.0;
    float bottomLeftV  = ((bottomLeftCoord.x >= 0 && bottomLeftCoord.x < constants.simBounds.x) &&
                          (bottomLeftCoord.y >= 0 && bottomLeftCoord.y < constants.simBounds.y)) ?
                          image[bottomLeftCoord] : 0.0;
    float bottomRightV = ((bottomRightCoord.x >= 0 && bottomRightCoord.x < constants.simBounds.x) &&
                          (bottomRightCoord.y >= 0 && bottomRightCoord.y < constants.simBounds.y)) ?
                          image[bottomRightCoord] : 0.0;

    return lerp(
        lerp(bottomLeftV, bottomRightV, pos.x),
        lerp(topLeftV,    topRightV,    pos.x),
        pos.y
    );
}
float sampleNearest(RWTexture2D<uint> image, float2 pos){
    pos /= constants.cellSize;
    pos -= float2(0.5, 0.5);
    
    int2 bottomLeftCoord  = int2(floor(pos));
    int2 bottomRightCoord = int2(bottomLeftCoord.x + 1, bottomLeftCoord.y);
    int2 topLeftCoord     = int2(bottomLeftCoord.x, bottomLeftCoord.y + 1);
    int2 topRightCoord    = int2(bottomLeftCoord.x + 1, bottomLeftCoord.y + 1);
    
    uint topLeftV     = ((topLeftCoord.x >= 0 && topLeftCoord.x < constants.simBounds.x) &&
                          (topLeftCoord.y >= 0 && topLeftCoord.y < constants.simBounds.y)) ? 
                          image[topLeftCoord] : 0.0;
    uint topRightV    = ((topRightCoord.x >= 0 && topRightCoord.x < constants.simBounds.x) &&
                          (topRightCoord.y >= 0 && topRightCoord.y < constants.simBounds.y)) ?
                          image[topRightCoord] : 0.0;
    uint bottomLeftV  = ((bottomLeftCoord.x >= 0 && bottomLeftCoord.x < constants.simBounds.x) &&
                          (bottomLeftCoord.y >= 0 && bottomLeftCoord.y < constants.simBounds.y)) ?
                          image[bottomLeftCoord] : 0.0;
    uint bottomRightV = ((bottomRightCoord.x >= 0 && bottomRightCoord.x < constants.simBounds.x) &&
                          (bottomRightCoord.y >= 0 && bottomRightCoord.y < constants.simBounds.y)) ?
                          image[bottomRightCoord] : 0.0;

    pos = frac(pos);

    return pos.x >= 0.5 ? (pos.y >= 0.5 ? topRightV : bottomRightV) : (pos.y >= 0.5 ? topLeftV : bottomLeftV);
}

//    +----------------------------------------------------+
//    |                       brush                        |
//    +----------------------------------------------------+

#ifdef KERNEL_BRUSH
[numthreads(16, 16, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    int2 coord = int2(id.xy);
    if (coord.x >= constants.simBounds.x || coord.y >= constants.simBounds.y)
        return;

    if(constants.brushDown) {
        float2 brushPrevPos = constants.brushPos - constants.brushDelta;
        float2 centerPos    = float2(coord.x + 0.5, coord.y + 0.5);

        bool outsideLine = dot(constants.brushDelta, centerPos - brushPrevPos) < 0 ||
                        dot(constants.brushDelta, centerPos - constants.brushPos) > 0;

        float centerDist = outsideLine
            ? min(length(centerPos - brushPrevPos), length(centerPos - constants.brushPos))
            : abs(constants.brushDelta.x * (centerPos.y - brushPrevPos.y) -
                constants.brushDelta.y * (centerPos.x - brushPrevPos.x))
            / max(length(constants.brushDelta), 0.0001f);
        float centerFraction = 1 - centerDist / constants.brushSize;
        centerFraction = centerFraction > 0.25 ? 1 : centerFraction * 4;
        
        if(centerFraction >= 0.0){
            switch (constants.brushType) {
                case BRUSH_PRESSURE:
                {
                    velocityX[coord] += centerFraction * constants.brushDelta.x * 10;
                    velocityX[int2(coord.x + 1, coord.y)] += centerFraction * constants.brushDelta.x * 10;
                    velocityY[coord] += centerFraction * constants.brushDelta.y * 10;
                    velocityY[int2(coord.x, coord.y + 1)] += centerFraction * constants.brushDelta.y * 10;
                    break;
                }
                case BRUSH_SMOKE:
                {
                    smoke[coord] += float4(constants.brushColor.xyz * centerFraction * constants.deltaTime, 0.0);
                    break;
                }
                case BRUSH_SOLID:
                {
                    solidity[coord] = 1;
                    break;
                }
            }
        }
    }

    if(coord.x <= 10) {
        velocityX[coord] = 100;
        velocityX[int2(coord.x + 1, coord.y)] = 100;
    }

    // if(length(coord - float2(100 , constants.simBounds.y / 2)) < 20) {
    //     solidity[coord] = 1;
    // }

    if(coord.y == 0 || coord.y == constants.simBounds.y - 1) {
        solidity[coord] = 1;
    }

    if( abs(coord.x - 10) < 5) {
        smoke[coord] = float4(1.0, 1.0, 1.0, 1.0);
    }
}
#endif

//    +----------------------------------------------------+
//    |                       advect                       |
//    +----------------------------------------------------+

#ifdef KERNEL_ADVECT
[numthreads(16, 16, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    int2 coord = int2(id.xy);
    if (coord.x >= constants.simBounds.x || coord.y >= constants.simBounds.y)
        return;

    float2 leftEdgePos = float2(coord.x, coord.y + 0.5) * constants.cellSize;
    float2 leftEdgeVelocity = float2(sampleVelocityX(oldVelocityX, leftEdgePos), sampleVelocityY(oldVelocityY, leftEdgePos));
    float2 leftEdgePrevPos = leftEdgePos - constants.deltaTime * leftEdgeVelocity;
    velocityX[coord] = sampleVelocityX(oldVelocityX, leftEdgePrevPos);

    float2 bottomEdgePos = float2(coord.x + 0.5, coord.y) * constants.cellSize;
    float2 bottomEdgeVelocity = float2(sampleVelocityX(oldVelocityX, bottomEdgePos), sampleVelocityY(oldVelocityY, bottomEdgePos));
    float2 bottomEdgePrevPos = bottomEdgePos - constants.deltaTime * bottomEdgeVelocity;
    velocityY[coord] = sampleVelocityY(oldVelocityY, bottomEdgePrevPos);

    if(coord.x == constants.simBounds.x - 1) {
        float2 rightEdgePos = float2(coord.x + 1, coord.y + 0.5) * constants.cellSize;
        float2 rightEdgeVelocity = float2(sampleVelocityX(oldVelocityX, rightEdgePos), sampleVelocityY(oldVelocityY, rightEdgePos));
        float2 rightEdgePrevPos = rightEdgePos - constants.deltaTime * rightEdgeVelocity;
        velocityX[int2(coord.x + 1, coord.y)] = sampleVelocityX(oldVelocityX, rightEdgePrevPos);
    }
    if(coord.y == constants.simBounds.y - 1) {
        float2 topEdgePos = float2(coord.x + 0.5, coord.y + 1) * constants.cellSize;
        float2 topEdgeVelocity = float2(sampleVelocityX(oldVelocityX, topEdgePos), sampleVelocityY(oldVelocityY, topEdgePos));
        float2 topEdgePrevPos = topEdgePos - constants.deltaTime * topEdgeVelocity;
        velocityY[int2(coord.x, coord.y + 1)] = sampleVelocityY(oldVelocityY, topEdgePrevPos);
    }

    float2 centerPos = float2(coord.x + 0.5, coord.y + 0.5) * constants.cellSize;
    float2 centerVelocity = float2(sampleVelocityX(oldVelocityX, centerPos), sampleVelocityY(oldVelocityY, centerPos));
    float2 centerPrevPos = centerPos - constants.deltaTime * centerVelocity;
    smoke[coord] = sampleProperty(oldSmoke, centerPrevPos);
}
#endif

//    +----------------------------------------------------+
//    |                       diffuse                      |
//    +----------------------------------------------------+

#ifdef KERNEL_DIFFUSE
[numthreads(16, 16, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    int2 coord = int2(id.xy);
    if (coord.x >= constants.simBounds.x || coord.y >= constants.simBounds.y)
        return;
    
    bool leftExist  = (coord.x != 0);
    bool rightExist = (coord.x != constants.simBounds.x - 1);
    bool downExist  = (coord.y != 0);
    bool upExist    = (coord.y != constants.simBounds.y - 1);

    float4 smokeCenter = oldSmoke[coord];
    float4 outsideSmoke = constants.openEdges ? 0.0 : smokeCenter;

    float4 smokeUp = upExist ? oldSmoke[int2(coord.x, coord.y + 1)] : outsideSmoke;
    float4 smokeDown = downExist ? oldSmoke[int2(coord.x, coord.y - 1)] : outsideSmoke;
    float4 smokeRight = rightExist ? oldSmoke[int2(coord.x + 1, coord.y)] : outsideSmoke;
    float4 smokeLeft = leftExist ? oldSmoke[int2(coord.x - 1, coord.y)] : outsideSmoke;

    smoke[coord] = smokeCenter + 
        (constants.deltaTime * constants.smokeDiffusion) * 
        (smokeUp + smokeDown + smokeRight + smokeLeft - 4 * smokeCenter)
        / (constants.cellSize * constants.cellSize);
}
#endif

//    +----------------------------------------------------+
//    |                     pre process                    |
//    +----------------------------------------------------+

#ifdef KERNEL_PRE_PROCESS
[numthreads(16, 16, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    int2 coord = int2(id.xy);
    if (coord.x >= constants.simBounds.x || coord.y >= constants.simBounds.y)
        return;

    bool leftExist  = (coord.x != 0);
    bool rightExist = (coord.x != constants.simBounds.x - 1);
    bool downExist  = (coord.y != 0);
    bool upExist    = (coord.y != constants.simBounds.y - 1);

    bool centerSolid  = solidity[coord];

    bool leftSolid  = leftExist  ? solidity[coord + int2(-1, 0)] : !(constants.openEdges || (coord.x != 0));
    bool rightSolid = rightExist ? solidity[coord + int2(1, 0)]  : !(constants.openEdges || (coord.x != constants.simBounds.x - 1));
    bool downSolid  = downExist  ? solidity[coord + int2(0, -1)] : !(constants.openEdges || (coord.y != 0));
    bool upSolid    = upExist    ? solidity[coord + int2(0, 1)]  : !(constants.openEdges || (coord.y != constants.simBounds.y - 1));

    float uLeft   = !leftSolid  ? velocityX[coord]                 : 0.0;
    float uRight  = !rightSolid ? velocityX[coord + int2(1, 0)]    : 0.0;
    float uUp     = !upSolid    ? velocityY[coord + int2(0, 1)]    : 0.0;
    float uDown   = !downSolid  ? velocityY[coord]                 : 0.0;

    uint d = asuint(-constants.density * constants.cellSize / constants.deltaTime * (uRight - uLeft + uUp - uDown)) & 0xFFFFFFE0;
    d |= int(centerSolid);
    d |= int(leftSolid) << 1;
    d |= int(rightSolid) << 2;
    d |= int(downSolid) << 3;
    d |= int(upSolid) << 4;

    divergence[coord] = asfloat(d);
}
#endif

//    +----------------------------------------------------+
//    |                      project                       |
//    +----------------------------------------------------+

#ifdef KERNEL_PROJECT
[numthreads(16, 16, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    int2 coord = int2(id.x * 2 + ((id.y + constants.redBlackIdx) & 1), id.y);
    if (coord.x >= constants.simBounds.x || coord.y >= constants.simBounds.y)
        return;

    uint raw = asuint(divergence[coord]);
    float div = asfloat(raw & 0xFFFFFFE0);
    
    bool leftSolid  = bool(raw & 2);
    bool rightSolid = bool(raw & 4);
    bool downSolid  = bool(raw & 8);
    bool upSolid    = bool(raw & 16);

    if(raw & 1) return;
    
    int neighboursNum = int(!leftSolid) + int(!rightSolid) + int(!downSolid) + int(!upSolid);
    if(neighboursNum == 0) return;

    bool leftExist  = (coord.x != 0);
    bool rightExist = (coord.x != constants.simBounds.x - 1);
    bool downExist  = (coord.y != 0);
    bool upExist    = (coord.y != constants.simBounds.y - 1);

    float pCenter = pressure[coord];
    float pOutside = -pCenter;

    float pDown  = downSolid  ? 0.0 : (downExist  ? pressure[int2(coord.x, coord.y - 1)] : pOutside);
    float pUp    = upSolid    ? 0.0 : (upExist    ? pressure[int2(coord.x, coord.y + 1)] : pOutside);
    float pRight = rightSolid ? 0.0 : (rightExist ? pressure[int2(coord.x + 1, coord.y)] : pOutside);
    float pLeft  = leftSolid  ? 0.0 : (leftExist  ? pressure[int2(coord.x - 1, coord.y)] : pOutside);

    float newPressure = ((div) + pDown + pLeft + pRight + pUp) / float(neighboursNum);

    pressure[coord] = lerp(pCenter, newPressure, constants.overRelaxation);
}
#endif

//    +----------------------------------------------------+
//    |                  update velocities                 |
//    +----------------------------------------------------+

#ifdef KERNEL_UPDATE_VELOCITIES
[numthreads(16, 16, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    int2 coord = int2(id.xy);
    if (coord.x >= constants.simBounds.x || coord.y >= constants.simBounds.y)
        return;

    bool centerSolid = solidity[coord] != 0;

    float alpha = constants.deltaTime / (constants.density * constants.cellSize);

    float pCenter = pressure[coord];
    float pOutside = constants.openEdges ? -pCenter : pCenter; 

    bool downExist   = (coord.y != 0);
    bool downSolid   = downExist ? solidity[coord + int2(0, -1)] : !constants.openEdges;
    float pDown      = downExist ? pressure[int2(coord.x, coord.y - 1)] : pOutside;
    float uDown      = velocityY[coord];
    float uDownNew   = (!downSolid && !centerSolid) ? uDown - alpha * (pCenter - pDown) : 0.0;
    velocityY[coord] = uDownNew;

    bool leftExist   = (coord.x != 0);
    bool leftSolid   = leftExist ? solidity[coord + int2(-1, 0)] : !constants.openEdges;
    float pLeft      = leftExist ? pressure[int2(coord.x - 1, coord.y)] : pOutside;
    float uLeft      = velocityX[coord];
    float uLeftNew   = (!leftSolid && !centerSolid) ? uLeft - alpha * (pCenter - pLeft) : 0;
    velocityX[coord] = uLeftNew;

    if(coord.x == constants.simBounds.x - 1) {
        bool rightExist = (coord.x != constants.simBounds.x - 1);
        bool rightSolid = rightExist ? solidity[coord + int2(1, 0)]  : !constants.openEdges;
        float pRight    = rightExist ? pressure[int2(coord.x + 1, coord.y)] : pOutside;
        float uRight    = velocityX[coord + int2(1, 0)];
        float uRightNew = (!rightSolid && !centerSolid) ? uRight - alpha * (pRight - pCenter) : 0;
        velocityX[int2(coord.x + 1, coord.y)] = uRightNew;
    }
    if(coord.y == constants.simBounds.y - 1) {
        bool upExist = (coord.y != constants.simBounds.y - 1);
        bool upSolid = upExist    ? solidity[coord + int2(0, 1)]  : !constants.openEdges;
        float pUp    = upExist ? pressure[int2(coord.x, coord.y + 1)] : pOutside;
        float uUp    = velocityY[coord + int2(0, 1)];
        float uUpNew = (!upSolid && !centerSolid) ? uUp - alpha * (pUp - pCenter) : 0.0;
        velocityY[int2(coord.x, coord.y + 1)] = uUpNew;
    }
}
#endif

//    +----------------------------------------------------+
//    |                   visualization                    |
//    +----------------------------------------------------+

float3 hsvToRgb(float h, float s, float v) {
    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    float3 p = abs(frac(h + K.xyz) * 6.0 - K.www);
    return v * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), s);
}

float3 turboColormap(float x) {
    const float4 kRedVec4   = float4( 0.13572138,   4.61539260, -42.66032258,  132.13108234);
    const float4 kGreenVec4 = float4( 0.09140261,   2.19418839,   4.84296658,  -14.18503333);
    const float4 kBlueVec4  = float4( 0.10667330,  12.64194608, -60.58204836,  110.36276771);
    const float2 kRedVec2   = float2(-152.94239396, 59.28637943);
    const float2 kGreenVec2 = float2(   4.27729857,  2.82956604);
    const float2 kBlueVec2  = float2( -89.90310912, 27.34824973);

    x = saturate(x);
    float4 v4 = float4(1.0, x, x * x, x * x * x);
    float2 v2 = v4.zw * v4.z;   // x^4, x^5

    return saturate(float3(
        dot(v4, kRedVec4)   + dot(v2, kRedVec2),
        dot(v4, kGreenVec4) + dot(v2, kGreenVec2),
        dot(v4, kBlueVec4)  + dot(v2, kBlueVec2)
    ));
}

float3 jetColormap(float x) {
    x = saturate(x);
    return saturate(float3(
        1.5 - abs(4.0 * x - 3.0),
        1.5 - abs(4.0 * x - 2.0),
        1.5 - abs(4.0 * x - 1.0)
    ));
}

#ifdef KERNEL_VISUALIZE
[numthreads(16, 16, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    int2 coord = int2(id.xy);
    if (coord.x >= constants.drawBound.x || coord.y >= constants.drawBound.y)
        return;

    float2 pos = (coord + float2(0.5, 0.5));

    float4 outColor = float4(0.0, 0.0, 0.0, 1.0);

    switch (constants.visType) {
        case VISUALIZE_PRESSURE:
        {
            float p = sampleProperty1(pressure, pos) / constants.visScale;
            outColor = (p >= 0.0) ? float4(p, 0.0, 0.0, 1.0) : float4(0.0, 0.0, -p, 1.0);
            break;
        }
        case VISUALIZE_VELOCITY:
        {
            float u = sampleVelocityX(velocityX, pos);
            float v = sampleVelocityY(velocityY, pos);

            float speed = length(float2(u, v));
            float t = speed / max(constants.visScale, 0.0001);

            // outColor = float4(turboColormap(t), 1.0);
            outColor = float4(jetColormap(t), 1.0);   // alternative

            break;
        }
        case VISUALIZE_DIVERGENCE:
        {
            float uLeft  = sampleVelocityX(velocityX, pos);
            float uRight = sampleVelocityX(velocityX, pos + float2(constants.cellSize, 0.0));
            float uDown  = sampleVelocityY(velocityY, pos);
            float uUp    = sampleVelocityY(velocityY, pos + float2(0.0, constants.cellSize));

            float div = (uRight - uLeft) + (uUp - uDown);
            float d = div / max(constants.visScale, 0.0001f);
            outColor = (d >= 0.0f) ? float4(d, 0.0f, 0.0f, 1.0f) : float4(0.0f, 0.0f, -d, 1.0f);
            break;
        }
        case VISUALIZE_SMOKE:
        {
            outColor = float4(sampleProperty(smoke, pos).xyz, 0.0);
            break;
        }
    }

    float cellType = sampleProperty2(solidity, pos);
    if(cellType >= 0.001) {
        outColor = float4(cellType, cellType, cellType, 1.0);
    }

    drawImage[int2(coord.x, constants.drawBound.y - 1 - coord.y)] = outColor;
}
#endif