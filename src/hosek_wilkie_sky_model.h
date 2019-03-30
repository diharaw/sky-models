#pragma once

#include "sky_model.h"

// An Analytic Model for Full Spectral Sky-Dome Radiance (Lukas Hosek, Alexander Wilkie)
class HosekWilkieSkyModel : public SkyModel
{
public:
	HosekWilkieSkyModel();
	~HosekWilkieSkyModel();

	bool initialize() override;
	void update() override;
	void set_render_uniforms(dw::Program* program) override;

private:
	glm::vec3 A, B, C, D, E, F, G, H, I;
    glm::vec3 Z;
};