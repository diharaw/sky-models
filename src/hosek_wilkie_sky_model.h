#pragma once

#include "table_sky_model.h"

class HosekWilkieSkyModel : public TableSkyModel
{
public:
	HosekWilkieSkyModel();
	~HosekWilkieSkyModel();

	bool initialize() override;
	void update() override;
	void set_render_uniforms(dw::Program* program) override;

	inline glm::vec3 rgb_albedo() { return m_rgb_albedo; }
	inline void set_rgb_albedo(glm::vec3 a) { m_rgb_albedo = a; }

private:
	glm::vec3 m_rgb_albedo = glm::vec3(1.0f);
	glm::vec3 m_to_sun;
    float m_coeffs_XYZ[3][9]; // Hosek 9-term distribution coefficients
	glm::vec3 m_coeffs_XYZ_GPU[9];
    glm::vec3 m_rad_XYZ; // Overall average radiance
};