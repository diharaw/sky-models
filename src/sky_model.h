#pragma once

#include <ogl.h>

class SkyModel
{
public:
	virtual bool initialize() = 0;
	virtual void update() = 0;
	virtual void set_render_uniforms(dw::Program* program) = 0;

	inline glm::vec3 direction() { return m_direction; }
	inline void set_direction(glm::vec3 dir) { m_direction = -dir; }

protected:
	glm::vec3 m_direction;
};