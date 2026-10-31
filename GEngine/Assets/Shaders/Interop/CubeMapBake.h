#ifndef SHADER_INTEROP_CUBEMAP_BAKE_H
#define SHADER_INTEROP_CUBEMAP_BAKE_H

#include "Interop.h"

struct CubeMapBakeConstants {
    uint32_t InputIndex;
    uint32_t CameraIndex;
};

struct CubeFaceCameraData {
    row_major float4x4 ViewProjection[6];
};

#endif // SHADER_INTEROP_CUBEMAP_BAKE_H
