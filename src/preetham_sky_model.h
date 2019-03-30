#pragma once

#include "table_sky_model.h"

class PreethamSkyModel : public TableSkyModel
{
public:
	PreethamSkyModel();
	~PreethamSkyModel();

	bool initialize() override;
	void update() override;
	void set_render_uniforms(dw::Program* program) override;

private:
    float m_perez_x[5];
    float m_perez_y[5];
    float m_perez_Y[5];

    glm::vec3 m_zenith = glm::vec3(0.0f);
    glm::vec3 m_perez_inv_den = glm::vec3(1.0f);
};