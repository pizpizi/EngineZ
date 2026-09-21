[[vk::binding(0, 0)]] RWTexture2D<float> tex_pressure;
[[vk::binding(1, 0)]] RWTexture2D<float> tex_velocityX;
[[vk::binding(5, 0)]] RWTexture2D<float> tex_oldVelocityX;
[[vk::binding(2, 0)]] RWTexture2D<float> tex_velocityY;
[[vk::binding(6, 0)]] RWTexture2D<float> tex_oldVelocityY;
[[vk::binding(8, 0)]] RWTexture2D<float> tex_divergence;
[[vk::binding(9, 0)]] [[vk::image_format("r8ui")]] RWTexture2D<uint> tex_cellType;
[[vk::binding(10, 0)]] [[vk::image_format("rgba16f")]] RWTexture2D<float4> tex_cellData;
[[vk::binding(4, 0)]] [[vk::image_format("rgba16f")]] RWTexture2D<float4> tex_smoke;
[[vk::binding(7, 0)]] [[vk::image_format("rgba16f")]] RWTexture2D<float4> tex_oldSmoke;

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
static const uint CELL_SMOKE = 2u;
static const uint CELL_VELOCITY = 3u;
static const uint CELL_PRESSURE = 4u;

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
    float4 cellData;
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
                    tex_velocityX[coord] += centerFraction * constants.brushDelta.x * 10;
                    tex_velocityX[int2(coord.x + 1, coord.y)] += centerFraction * constants.brushDelta.x * 10;
                    tex_velocityY[coord] += centerFraction * constants.brushDelta.y * 10;
                    tex_velocityY[int2(coord.x, coord.y + 1)] += centerFraction * constants.brushDelta.y * 10;
                    break;
                }
                case BRUSH_SMOKE:
                {
                    tex_smoke[coord] += float4(constants.brushColor.xyz * centerFraction * constants.deltaTime, 0.0);
                    break;
                }
                case BRUSH_SOLID:
                {
                    tex_cellType[coord] = constants.cellType;
                    tex_cellData[coord] = constants.cellData;
                    break;
                }
            }
        }
    }

    uint c_cellType = tex_cellType[coord];
    if (c_cellType == CELL_SMOKE) {
        tex_smoke[coord] = tex_cellData[coord];
    } else if (c_cellType == CELL_PRESSURE) {
        tex_pressure[coord] = tex_cellData[coord].x;
    } else if (c_cellType == CELL_VELOCITY) {
        bool l_exist = (coord.x != 0);
        bool r_exist = (coord.x != constants.simBounds.x - 1);
        bool b_exist = (coord.y != 0);
        bool t_exist = (coord.y != constants.simBounds.y - 1);

        uint l_cellType = (coord.x != 0) ? (tex_cellType[coord + int2(-1, 0)]) : CELL_AIR;
        uint r_cellType = r_exist ? (tex_cellType[coord + int2(1, 0)] ) : CELL_AIR;
        uint b_cellType = b_exist ? (tex_cellType[coord + int2(0, -1)]) : CELL_AIR;
        uint t_cellType = t_exist ? (tex_cellType[coord + int2(0, 1)] ) : CELL_AIR;

        tex_velocityY[coord] = tex_cellData[coord].x;
        tex_velocityY[coord + int2(0, 1)] = tex_cellData[coord].x;
        tex_velocityX[coord + int2(1, 0)] = tex_cellData[coord].y;
        tex_velocityX[coord] = tex_cellData[coord].y;
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


    uint c_cellType = tex_cellType[coord];

    bool l_exist = (coord.x != 0);
    uint l_cellType = l_exist ? tex_cellType[coord + int2(-1, 0)] : CELL_AIR;
    if(c_cellType == CELL_SOLID || l_cellType == CELL_SOLID) {
        tex_velocityX[coord] = 0.0;
    } else if (c_cellType != CELL_VELOCITY && l_cellType != CELL_VELOCITY) {
        float2 l_edgePos = float2(coord.x, coord.y + 0.5) * constants.cellSize;
        float2 l_edgeVelocity = float2(sampleVelocityX(tex_oldVelocityX, l_edgePos), sampleVelocityY(tex_oldVelocityY, l_edgePos));
        float2 l_edgePrevPos = l_edgePos - constants.deltaTime * l_edgeVelocity;
        tex_velocityX[coord] = sampleVelocityX(tex_oldVelocityX, l_edgePrevPos);
    }

    bool b_exist = (coord.y != 0);
    uint b_cellType = b_exist ? tex_cellType[coord + int2(0, -1)] : CELL_AIR;
    if(c_cellType == CELL_SOLID || b_cellType == CELL_SOLID) {
        tex_velocityY[coord] = 0.0;
    } else if (c_cellType != CELL_VELOCITY && b_cellType != CELL_VELOCITY) {
        float2 bottomEdgePos = float2(coord.x + 0.5, coord.y) * constants.cellSize;
        float2 bottomEdgeVelocity = float2(sampleVelocityX(tex_oldVelocityX, bottomEdgePos), sampleVelocityY(tex_oldVelocityY, bottomEdgePos));
        float2 bottomEdgePrevPos = bottomEdgePos - constants.deltaTime * bottomEdgeVelocity;
        tex_velocityY[coord] = sampleVelocityY(tex_oldVelocityY, bottomEdgePrevPos);
    }

    if(coord.x == constants.simBounds.x - 1) {
        bool r_exist = (coord.x != constants.simBounds.x - 1);
        uint r_cellType = r_exist ? tex_cellType[coord + int2(1, 0)] : CELL_AIR;
        if(c_cellType == CELL_SOLID || r_cellType == CELL_SOLID) {
            tex_velocityY[coord + int2(1, 0)] = 0.0;
        } else if (c_cellType != CELL_VELOCITY && r_cellType != CELL_VELOCITY) {
            float2 r_edgePos = float2(coord.x + 1, coord.y + 0.5) * constants.cellSize;
            float2 r_edgeVelocity = float2(sampleVelocityX(tex_oldVelocityX, r_edgePos), sampleVelocityY(tex_oldVelocityY, r_edgePos));
            float2 r_edgePrevPos = r_edgePos - constants.deltaTime * r_edgeVelocity;
            tex_velocityX[coord + int2(1, 0)] = sampleVelocityX(tex_oldVelocityX, r_edgePrevPos);
        }
    }

    if(coord.y == constants.simBounds.y - 1) {
        bool t_exist = (coord.y != constants.simBounds.y - 1);
        uint t_cellType = t_exist ? tex_cellType[coord + int2(0, 1)] : CELL_AIR;
        if(c_cellType == CELL_SOLID || t_cellType == CELL_SOLID) {
            tex_velocityY[coord + int2(0, 1)] = 0.0;
        } else if (c_cellType != CELL_VELOCITY && t_cellType != CELL_VELOCITY) {
            float2 t_edgePos = float2(coord.x + 0.5, coord.y + 1) * constants.cellSize;
            float2 t_edgeVelocity = float2(sampleVelocityX(tex_oldVelocityX, t_edgePos), sampleVelocityY(tex_oldVelocityY, t_edgePos));
            float2 t_edgePrevPos = t_edgePos - constants.deltaTime * t_edgeVelocity;
            tex_velocityY[coord + int2(0, 1)] = sampleVelocityY(tex_oldVelocityY, t_edgePrevPos);
        }
    }

    float2 c_pos = float2(coord.x + 0.5, coord.y + 0.5) * constants.cellSize;
    float2 c_velocity = float2(sampleVelocityX(tex_oldVelocityX, c_pos), sampleVelocityY(tex_oldVelocityY, c_pos));
    float2 c_prevPos = c_pos - constants.deltaTime * c_velocity;
    tex_smoke[coord] = sampleProperty(tex_oldSmoke, c_prevPos);
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
    
    bool l_exist = (coord.x != 0);
    bool r_xist  = (coord.x != constants.simBounds.x - 1);
    bool b_xist  = (coord.y != 0);
    bool t_xist  = (coord.y != constants.simBounds.y - 1);

    uint c_cellType = tex_cellType[coord];
    if(c_cellType == CELL_SOLID) {
        tex_smoke[coord] = float4(0.0, 0.0, 0.0, 0.0);
        return;
    }

    float4 smokeCenter = tex_oldSmoke[coord];
    float4 outsideSmoke = constants.openEdges ? 0.0 : smokeCenter;

    float4 smokeUp = t_xist ? tex_oldSmoke[int2(coord.x, coord.y + 1)] : outsideSmoke;
    float4 smokeDown = b_xist ? tex_oldSmoke[int2(coord.x, coord.y - 1)] : outsideSmoke;
    float4 smokeRight = r_xist ? tex_oldSmoke[int2(coord.x + 1, coord.y)] : outsideSmoke;
    float4 smokeLeft = l_exist ? tex_oldSmoke[int2(coord.x - 1, coord.y)] : outsideSmoke;

    tex_smoke[coord] = smokeCenter + 
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

    bool l_exist = (coord.x != 0);
    bool r_exist = (coord.x != constants.simBounds.x - 1);
    bool b_exist = (coord.y != 0);
    bool t_exist = (coord.y != constants.simBounds.y - 1);

    uint c_cellType = tex_cellType[coord];
    uint l_cellType = l_exist ? (tex_cellType[coord + int2(-1, 0)]) : CELL_AIR;
    uint r_cellType = r_exist ? (tex_cellType[coord + int2(1, 0)] ) : CELL_AIR;
    uint b_cellType = b_exist ? (tex_cellType[coord + int2(0, -1)]) : CELL_AIR;
    uint t_cellType = t_exist ? (tex_cellType[coord + int2(0, 1)] ) : CELL_AIR;

    bool c_solid = (c_cellType == CELL_SOLID) || (c_cellType == CELL_PRESSURE);
    bool l_solid = l_cellType == CELL_SOLID;
    bool r_solid = r_cellType == CELL_SOLID;
    bool b_solid = b_cellType == CELL_SOLID;
    bool t_solid = t_cellType == CELL_SOLID;

    float l_speed = tex_velocityX[coord];
    float r_speed = tex_velocityX[coord + int2(1, 0)];
    float t_speed = tex_velocityY[coord + int2(0, 1)];
    float b_speed = tex_velocityY[coord];

    uint d = asuint(-constants.density * constants.cellSize / constants.deltaTime * (r_speed - l_speed + t_speed - b_speed)) & 0xFFFFFFE0;
    d |= int(c_solid);
    d |= int(l_solid) << 1;
    d |= int(r_solid) << 2;
    d |= int(b_solid) << 3;
    d |= int(t_solid) << 4;

    tex_divergence[coord] = asfloat(d);
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

    uint raw = asuint(tex_divergence[coord]);
    float divergence = asfloat(raw & 0xFFFFFFE0);
    
    bool l_solid = bool(raw & 2);
    bool r_solid = bool(raw & 4);
    bool b_solid = bool(raw & 8);
    bool t_solid = bool(raw & 16);

    if(raw & 1) return;
    
    int neighboursNum = int(!l_solid) + int(!r_solid) + int(!b_solid) + int(!t_solid);
    if(neighboursNum == 0) return;

    bool l_exist = (coord.x != 0);
    bool r_xist  = (coord.x != constants.simBounds.x - 1);
    bool b_exist = (coord.y != 0);
    bool t_exist = (coord.y != constants.simBounds.y - 1);

    float pCenter = tex_pressure[coord];
    float pOutside = -pCenter;

    float b_p = b_solid ? 0.0 : (b_exist   ? tex_pressure[int2(coord.x, coord.y - 1)] : pOutside);
    float t_p = t_solid ? 0.0 : (t_exist   ? tex_pressure[int2(coord.x, coord.y + 1)] : pOutside);
    float r_p = r_solid ? 0.0 : (r_xist    ? tex_pressure[int2(coord.x + 1, coord.y)] : pOutside);
    float l_p = l_solid ? 0.0 : (l_exist   ? tex_pressure[int2(coord.x - 1, coord.y)] : pOutside);

    float newPressure = ((divergence) + b_p + l_p + r_p + t_p) / float(neighboursNum);

    tex_pressure[coord] = lerp(pCenter, newPressure, constants.overRelaxation);
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

    bool c_solid = tex_cellType[coord] != 0;

    float alpha = constants.deltaTime / (constants.density * constants.cellSize);

    float c_pressure = tex_pressure[coord];
    uint c_cellType = tex_cellType[coord];
    float outside_pressure = -c_pressure; 

    bool b_exist = (coord.y != 0);
    uint b_cellType = b_exist ? tex_cellType[coord + int2(0, -1)] : CELL_AIR;
    if(b_cellType == CELL_SOLID || c_cellType == CELL_SOLID) {
        tex_velocityY[coord] = 0.0;
    } else if (c_cellType != CELL_VELOCITY && b_cellType != CELL_VELOCITY) {
        float b_pressure = b_exist ? tex_pressure[coord + int2(0, -1)] : outside_pressure;
        float b_speed    = tex_velocityY[coord];

        tex_velocityY[coord] = b_speed - alpha * (c_pressure - b_pressure);
    }

    bool l_exist    = (coord.x != 0);
    uint l_cellType = l_exist ? tex_cellType[coord + int2(-1, 0)] : CELL_AIR;
    if(l_cellType == CELL_SOLID || c_cellType == CELL_SOLID) {
        tex_velocityX[coord] = 0.0;
    } else if (c_cellType != CELL_VELOCITY && l_cellType != CELL_VELOCITY) {
        float l_pressure = l_exist ? tex_pressure[coord + int2(-1, 0)] : outside_pressure;
        float l_speed = tex_velocityX[coord];

        tex_velocityX[coord] = l_speed - alpha * (c_pressure - l_pressure);
    }

    if(coord.x == constants.simBounds.x - 1) {
        bool r_exist    = (coord.x != constants.simBounds.x - 1);
        uint r_cellType = r_exist ? tex_cellType[coord + int2(1, 0)] : CELL_AIR;
        if(r_cellType == CELL_SOLID || c_cellType == CELL_SOLID) {
            tex_velocityX[coord + int2(1, 0)] = 0.0;
        } else if (c_cellType != CELL_VELOCITY && r_cellType != CELL_VELOCITY) {
            float r_pressure = r_exist ? tex_pressure[coord + int2(1, 0)] : outside_pressure;
            float r_speed = tex_velocityX[coord + int2(1, 0)];

            tex_velocityX[coord + int2(1, 0)] = r_speed - alpha * (r_pressure - c_pressure);
        }
    }
    if(coord.y == constants.simBounds.y - 1) {
        bool t_exist    = (coord.y != constants.simBounds.y - 1);
        uint t_cellType = t_exist ? tex_cellType[coord + int2(0, 1)] : CELL_AIR;
        if(t_cellType == CELL_SOLID || c_cellType == CELL_SOLID) {
            tex_velocityY[coord + int2(0, 1)] = 0.0;
        } else if (c_cellType != CELL_VELOCITY && t_cellType != CELL_VELOCITY) {
            float t_pressure = t_exist ? tex_pressure[coord + int2(0, 1)] : outside_pressure;
            float t_speed = tex_velocityY[coord + int2(0, 1)];

            tex_velocityY[coord + int2(0, 1)] = t_speed - alpha * (t_pressure - c_pressure);
        }
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
            float p = sampleProperty1(tex_pressure, pos) / constants.visScale;
            outColor = (p >= 0.0) ? float4(p, 0.0, 0.0, 1.0) : float4(0.0, 0.0, -p, 1.0);
            break;
        }
        case VISUALIZE_VELOCITY:
        {
            float u = sampleVelocityX(tex_velocityX, pos);
            float v = sampleVelocityY(tex_velocityY, pos);

            float speed = length(float2(u, v));
            float t = speed / max(constants.visScale, 0.0001);

            // outColor = float4(turboColormap(t), 1.0);
            outColor = float4(jetColormap(t), 1.0);   // alternative

            break;
        }
        case VISUALIZE_DIVERGENCE:
        {
            float uLeft  = sampleVelocityX(tex_velocityX, pos);
            float uRight = sampleVelocityX(tex_velocityX, pos + float2(constants.cellSize, 0.0));
            float uDown  = sampleVelocityY(tex_velocityY, pos);
            float uUp    = sampleVelocityY(tex_velocityY, pos + float2(0.0, constants.cellSize));

            float div = (uRight - uLeft) + (uUp - uDown);
            float d = div / max(constants.visScale, 0.0001f);
            outColor = (d >= 0.0f) ? float4(d, 0.0f, 0.0f, 1.0f) : float4(0.0f, 0.0f, -d, 1.0f);
            break;
        }
        case VISUALIZE_SMOKE:
        {
            outColor = float4(sampleProperty(tex_smoke, pos).xyz, 1.0);
            break;
        }
    }

    float cellType = sampleProperty2(tex_cellType, pos);
    if(cellType >= 0.001) {
        outColor = float4(cellType, cellType, cellType, 1.0);
    }

    drawImage[int2(coord.x, constants.drawBound.y - 1 - coord.y)] = outColor;
}
#endif