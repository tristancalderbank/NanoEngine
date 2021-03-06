#pragma once

#include <memory>

#include "cubemapframebuffer.h"
#include "shader.h"

/**
 * Computes a diffuse irradiance map from an environment map.
 *
 * The computed map tells you the sum of incoming light from the environment
 * for a particular direction.
 */
class DiffuseIrradianceMap {
public:
    /**
     * Initialize a diffuse irradiance map.
     * @param hdriPath path to an .hdr image in equirectangular projection format
     */
    DiffuseIrradianceMap(const std::string &engineRoot, const unsigned int environmentCubemapId);

    /**
     * Render the diffuse irradiance map.
     */
    void compute();

    /**
     * Get the GL texture ID of the computed cubemap.
     * @return
     */
    unsigned int getCubemapId();

private:
    const unsigned int diffuseIrradianceMapWidth = 32;
    const unsigned int diffuseIrradianceMapHeight = 32;

    const unsigned int environmentCubemapId;

    std::unique_ptr<Shader> diffuseIrradianceShader;
    std::unique_ptr<CubemapFramebuffer> diffuseIrradianceFramebuffer;
};