#include <application.h>
#include <mesh.h>
#include <camera.h>
#include <material.h>
#include <memory>
#include <iostream>
#include <stack>
#include <random>
#include <chrono>
#include "bruneton_sky_model.h"
#include "preetham_sky_model.h"
#include "hosek_wilkie_sky_model.h"

// Uniform buffer data structure.
struct ObjectUniforms
{
	DW_ALIGNED(16) glm::mat4 model;
};

struct GlobalUniforms
{
    DW_ALIGNED(16) glm::mat4 view;
    DW_ALIGNED(16) glm::mat4 projection;
	DW_ALIGNED(16) glm::mat4 inv_view;
	DW_ALIGNED(16) glm::mat4 inv_projection;
	DW_ALIGNED(16) glm::mat4 inv_view_projection;
	DW_ALIGNED(16) glm::vec4 view_pos;
};

#define CAMERA_FAR_PLANE 10000.0f

class SkyModels : public dw::Application
{
protected:
    
    // -----------------------------------------------------------------------------------------------------------------------------------
    
	bool init(int argc, const char* argv[]) override
	{
		// Create GPU resources.
		if (!create_shaders())
			return false;

		if (!create_uniform_buffer())
			return false;

		// Load mesh.
		if (!load_mesh())
			return false;

		if (!create_framebuffer())
			return false;

		// Create camera.
		create_camera();

		m_mesh_transforms.model = glm::mat4(1.0f);

		m_sun_angle = glm::radians(-180.0f);

		return m_bruneton_model.initialize() && m_preetham_model.initialize() && m_hosek_wilkie_model.initialize();
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void update(double delta) override
	{
		// Update camera.
		update_camera();

		update_global_uniforms(m_global_uniforms);
		update_object_uniforms(m_mesh_transforms);

		if (m_show_gui)
			ui();

		m_bruneton_model.set_direction(m_direction);
		m_preetham_model.set_direction(m_direction);
		m_hosek_wilkie_model.set_direction(m_direction);

		if (m_sky_model == 0)
			m_bruneton_model.update();
		else if (m_sky_model == 1)
			m_preetham_model.update();
		else if (m_sky_model == 2)
			m_hosek_wilkie_model.update();

		render_meshes();

		render_cubemap();

		render_fullscreen_triangle();

		if (m_debug_mode)
			m_debug_draw.frustum(m_main_camera->m_view_projection, glm::vec3(0.0f, 1.0f, 0.0f));

        // Render debug draw.
        m_debug_draw.render(nullptr, m_width, m_height, m_debug_mode ? m_debug_camera->m_view_projection : m_main_camera->m_view_projection);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void shutdown() override
	{
		dw::Mesh::unload(m_mesh);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void window_resized(int width, int height) override
	{
		// Override window resized method to update camera projection.
		m_main_camera->update_projection(60.0f, 0.1f, CAMERA_FAR_PLANE, float(m_width) / float(m_height));
        m_debug_camera->update_projection(60.0f, 0.1f, CAMERA_FAR_PLANE * 2.0f, float(m_width) / float(m_height));

		create_framebuffer();
	}

	// -----------------------------------------------------------------------------------------------------------------------------------
    
    void key_pressed(int code) override
    {
        // Handle forward movement.
        if(code == GLFW_KEY_W)
            m_heading_speed = m_camera_speed;
        else if(code == GLFW_KEY_S)
            m_heading_speed = -m_camera_speed;
        
        // Handle sideways movement.
        if(code == GLFW_KEY_A)
            m_sideways_speed = -m_camera_speed;
        else if(code == GLFW_KEY_D)
            m_sideways_speed = m_camera_speed;

		if (code == GLFW_KEY_K)
			m_debug_mode = !m_debug_mode;
    }
    
    // -----------------------------------------------------------------------------------------------------------------------------------
    
    void key_released(int code) override
    {
        // Handle forward movement.
        if(code == GLFW_KEY_W || code == GLFW_KEY_S)
            m_heading_speed = 0.0f;
        
        // Handle sideways movement.
        if(code == GLFW_KEY_A || code == GLFW_KEY_D)
            m_sideways_speed = 0.0f;

		if (code == GLFW_KEY_G)
			m_show_gui = !m_show_gui;
    }
    
    // -----------------------------------------------------------------------------------------------------------------------------------

	void mouse_pressed(int code) override
	{
		// Enable mouse look.
		if (code == GLFW_MOUSE_BUTTON_RIGHT)
			m_mouse_look = true;
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void mouse_released(int code) override
	{
		// Disable mouse look.
		if (code == GLFW_MOUSE_BUTTON_RIGHT)
			m_mouse_look = false;
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

protected:

	// -----------------------------------------------------------------------------------------------------------------------------------

	dw::AppSettings intial_app_settings() override
	{
		dw::AppSettings settings;

		settings.resizable = true;
		settings.maximized = false;
		settings.refresh_rate = 60;
		settings.major_ver = 4;
		settings.width = 1280;
		settings.height = 720;
		settings.title = "Sky Models";

		return settings;
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

private:

	// -----------------------------------------------------------------------------------------------------------------------------------
	
	void ui()
	{
		ImGui::InputFloat("Exposure", &m_exposure);
		ImGui::SliderAngle("Sun Angle", &m_sun_angle, 0.0f, -180.0f);

		const char* sky_models[] = { "Bruneton", "Preetham", "Hosek-Wilkie" };
		ImGui::Combo("Sky Model", &m_sky_model, sky_models, IM_ARRAYSIZE(sky_models));

		float turbidity = 0.0f;

		if (m_sky_model > 0)
		{
			if (m_sky_model == 1)
				turbidity = m_preetham_model.turbidity();
			if (m_sky_model == 2)
				turbidity = m_hosek_wilkie_model.turbidity();

			ImGui::SliderFloat("Turbidity", &turbidity, 2.0f, 30.0f);

			if (m_sky_model == 1)
				m_preetham_model.set_turbidity(turbidity);
			if (m_sky_model == 2)
				m_hosek_wilkie_model.set_turbidity(turbidity);
		}

		m_direction = glm::normalize(glm::vec3(0.0f, sin(m_sun_angle), cos(m_sun_angle)));

		ImGui::Text("Sun Direction = [ %f, %f, %f ]", m_direction.x, m_direction.y, m_direction.z);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------
	
	bool create_shaders()
	{
		{
			// Create general shaders
			m_mesh_vs = std::unique_ptr<dw::Shader>(dw::Shader::create_from_file(GL_VERTEX_SHADER, "shader/mesh_vs.glsl"));
			m_mesh_fs = std::unique_ptr<dw::Shader>(dw::Shader::create_from_file(GL_FRAGMENT_SHADER, "shader/mesh_fs.glsl"));

			if (!m_mesh_vs || !m_mesh_fs)
			{
				DW_LOG_FATAL("Failed to create Shaders");
				return false;
			}

			// Create general shader program
			dw::Shader* shaders[] = { m_mesh_vs.get(), m_mesh_fs.get() };
			m_mesh_program = std::make_unique<dw::Program>(2, shaders);

			if (!m_mesh_program)
			{
				DW_LOG_FATAL("Failed to create Shader Program");
				return false;
			}

			m_mesh_program->uniform_block_binding("u_GlobalUBO", 0);
			m_mesh_program->uniform_block_binding("u_ObjectUBO", 1);
		}

		{
			m_fullscreen_vs = std::unique_ptr<dw::Shader>(dw::Shader::create_from_file(GL_VERTEX_SHADER, "shader/fullscreen_vs.glsl"));
			m_fullscreen_fs = std::unique_ptr<dw::Shader>(dw::Shader::create_from_file(GL_FRAGMENT_SHADER, "shader/fullscreen_fs.glsl"));

			if (!m_fullscreen_vs || !m_fullscreen_fs)
			{
				DW_LOG_FATAL("Failed to create Shaders");
				return false;
			}

			// Create general shader program
			dw::Shader* shaders[] = { m_fullscreen_vs.get(), m_fullscreen_fs.get() };
			m_fullscreen_program = std::make_unique<dw::Program>(2, shaders);

			if (!m_fullscreen_program)
			{
				DW_LOG_FATAL("Failed to create Shader Program");
				return false;
			}
		}

		{
			m_sky_vs = std::unique_ptr<dw::Shader>(dw::Shader::create_from_file(GL_VERTEX_SHADER, "shader/sky_vs.glsl"));
			m_sky_fs = std::unique_ptr<dw::Shader>(dw::Shader::create_from_file(GL_FRAGMENT_SHADER, "shader/sky_fs.glsl"));

			if (!m_sky_vs || !m_sky_fs)
			{
				DW_LOG_FATAL("Failed to create Shaders");
				return false;
			}

			// Create general shader program
			dw::Shader* shaders[] = { m_sky_vs.get(), m_sky_fs.get() };
			m_sky_program = std::make_unique<dw::Program>(2, shaders);

			if (!m_sky_program)
			{
				DW_LOG_FATAL("Failed to create Shader Program");
				return false;
			}

			m_sky_program->uniform_block_binding("u_GlobalUBO", 0);
		}

		return true;
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	bool create_uniform_buffer()
	{
		// Create uniform buffer for object matrix data
        m_object_ubo = std::make_unique<dw::UniformBuffer>(GL_DYNAMIC_DRAW, sizeof(ObjectUniforms));
        
        // Create uniform buffer for global data
		m_global_ubo = std::make_unique<dw::UniformBuffer>(GL_DYNAMIC_DRAW, sizeof(GlobalUniforms));
        
		return true;
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	bool create_framebuffer()
	{
		if (m_fbo)
			m_fbo.reset();

		if (m_color_rt)
			m_color_rt.reset();

		if (m_depth_rt)
			m_depth_rt.reset();

		m_color_rt = std::make_unique<dw::Texture2D>(m_width, m_height, 1, 1, 1, GL_RGB16F, GL_RGB, GL_HALF_FLOAT);
		m_color_rt->set_min_filter(GL_LINEAR);
		m_color_rt->set_wrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		m_depth_rt = std::make_unique<dw::Texture2D>(m_width, m_height, 1, 1, 1, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);
		m_depth_rt->set_min_filter(GL_LINEAR);

		m_fbo = std::make_unique<dw::Framebuffer>();

		m_fbo->attach_render_target(0, m_color_rt.get(), 0, 0);
		m_fbo->attach_depth_stencil_target(m_depth_rt.get(), 0, 0);
		
		return true;
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	bool load_mesh()
	{
		m_mesh = dw::Mesh::load("mesh/teapot_smooth.obj");

		if (!m_mesh)
		{
			DW_LOG_FATAL("Failed to load mesh!");
			return false;
		}

		return true;
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void create_camera()
	{
		m_main_camera = std::make_unique<dw::Camera>(60.0f, 0.1f, CAMERA_FAR_PLANE, float(m_width) / float(m_height), glm::vec3(0.0f, 5.0f, 150.0f), glm::vec3(0.0f, 0.0, -1.0f));
        m_debug_camera = std::make_unique<dw::Camera>(60.0f, 0.1f, CAMERA_FAR_PLANE * 2.0f, float(m_width) / float(m_height), glm::vec3(0.0f, 5.0f, 150.0f), glm::vec3(0.0f, 0.0, -1.0f));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void render_mesh(dw::Mesh* mesh)
	{
		// Bind uniform buffers.
		m_object_ubo->bind_base(1);

		// Bind vertex array.
		mesh->mesh_vertex_array()->bind();

		dw::SubMesh* submeshes = mesh->sub_meshes();

		for (uint32_t i = 0; i < mesh->sub_mesh_count(); i++)
		{
			dw::SubMesh& submesh = submeshes[i];
			// Issue draw call.
			glDrawElementsBaseVertex(GL_TRIANGLES, submesh.index_count, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * submesh.base_index), submesh.base_vertex);
		}
	}
    
	// -----------------------------------------------------------------------------------------------------------------------------------

	void render_meshes()
	{
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		m_fbo->bind();
		glViewport(0, 0, m_width, m_height);

		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClearDepth(1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// Bind shader program.
		m_mesh_program->use();

		// Bind uniform buffers.
		m_global_ubo->bind_base(0);

		m_mesh_program->set_uniform("direction", m_direction);

		// Draw meshes.
		render_mesh(m_mesh);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void render_cubemap()
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDisable(GL_CULL_FACE);

		m_sky_program->use();

		m_global_ubo->bind_base(0);

		m_fbo->bind();
		glViewport(0, 0, m_width, m_height);

		/*m_cubemap_program->set_uniform("s_Skybox", 0);
		m_cubemap->bind(0);*/

		m_sky_program->set_uniform("sky_model", m_sky_model);

		if (m_sky_model == 0)
			m_bruneton_model.set_render_uniforms(m_sky_program.get());
		else if (m_sky_model == 1)
			m_preetham_model.set_render_uniforms(m_sky_program.get());
		else if (m_sky_model == 2)
			m_hosek_wilkie_model.set_render_uniforms(m_sky_program.get());

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glDepthFunc(GL_LESS);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void render_fullscreen_triangle()
	{
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		m_fullscreen_program->use();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, m_width, m_height);

		glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		m_fullscreen_program->set_uniform("exposure", m_exposure);

		m_fullscreen_program->set_uniform("s_Texture", 0);
		m_color_rt->bind(0);

		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
    
	// -----------------------------------------------------------------------------------------------------------------------------------

	void update_object_uniforms(const ObjectUniforms& transform)
	{
        void* ptr = m_object_ubo->map(GL_WRITE_ONLY);
		memcpy(ptr, &transform, sizeof(ObjectUniforms));
        m_object_ubo->unmap();
	}
    
    // -----------------------------------------------------------------------------------------------------------------------------------
    
    void update_global_uniforms(const GlobalUniforms& global)
    {
        void* ptr = m_global_ubo->map(GL_WRITE_ONLY);
        memcpy(ptr, &global, sizeof(GlobalUniforms));
        m_global_ubo->unmap();
    }

    // -----------------------------------------------------------------------------------------------------------------------------------
    
    void update_transforms(dw::Camera* camera)
    {
        // Update camera matrices.
		m_global_uniforms.view = camera->m_view;
        m_global_uniforms.projection = camera->m_projection;
		
		glm::mat4 cubemap_view = glm::mat4(glm::mat3(m_global_uniforms.view));
		glm::mat4 view_proj = camera->m_projection * cubemap_view;

		m_global_uniforms.inv_view = glm::inverse(cubemap_view);
		m_global_uniforms.inv_projection = glm::inverse(camera->m_projection);
		m_global_uniforms.inv_view_projection = glm::inverse(view_proj);
		m_global_uniforms.view_pos = glm::vec4(camera->m_position, 0.0f);
    }

    // -----------------------------------------------------------------------------------------------------------------------------------
    
    void update_camera()
    {
       dw::Camera* current = m_main_camera.get();
        
        if (m_debug_mode)
            current = m_debug_camera.get();
        
        float forward_delta = m_heading_speed * m_delta;
        float right_delta = m_sideways_speed * m_delta;
        
        current->set_translation_delta(current->m_forward, forward_delta);
        current->set_translation_delta(current->m_right, right_delta);

		m_camera_x = m_mouse_delta_x * m_camera_sensitivity;
		m_camera_y = m_mouse_delta_y * m_camera_sensitivity;
        
        if (m_mouse_look)
        {
            // Activate Mouse Look
            current->set_rotatation_delta(glm::vec3((float)(m_camera_y),
                                                    (float)(m_camera_x),
                                                    (float)(0.0f)));
        }
        else
        {
            current->set_rotatation_delta(glm::vec3((float)(0),
                                                    (float)(0),
                                                    (float)(0)));
        }
        
        current->update();

		update_transforms(current);
    }
    
    // -----------------------------------------------------------------------------------------------------------------------------------
    
private:
	// General GPU resources.
    std::unique_ptr<dw::Shader> m_mesh_vs;
	std::unique_ptr<dw::Shader> m_mesh_fs;
	std::unique_ptr<dw::Program> m_mesh_program;
	std::unique_ptr<dw::UniformBuffer> m_object_ubo;
    std::unique_ptr<dw::UniformBuffer> m_global_ubo;

	std::unique_ptr<dw::Texture2D> m_color_rt;
	std::unique_ptr<dw::Texture2D> m_depth_rt;
	std::unique_ptr<dw::Framebuffer> m_fbo;

	std::unique_ptr<dw::Shader> m_fullscreen_vs;
	std::unique_ptr<dw::Shader> m_fullscreen_fs;
	std::unique_ptr<dw::Program> m_fullscreen_program;

	std::unique_ptr<dw::Shader>  m_sky_vs;
	std::unique_ptr<dw::Shader>  m_sky_fs;
	std::unique_ptr<dw::Program> m_sky_program;

    // Camera.
	std::unique_ptr<dw::Camera> m_main_camera;
    std::unique_ptr<dw::Camera> m_debug_camera;
    
	// Uniforms.
	ObjectUniforms m_mesh_transforms;
    GlobalUniforms m_global_uniforms;

	// Mesh
	dw::Mesh* m_mesh;

    // Camera controls.
	bool m_show_gui = true;
    bool m_mouse_look = false;
    bool m_debug_mode = false;
    float m_heading_speed = 0.0f;
    float m_sideways_speed = 0.0f;
    float m_camera_sensitivity = 0.05f;
    float m_camera_speed = 0.06f;
	float m_camera_x = 0.0f;
	float m_camera_y = 0.0f;
	float m_exposure = 1.0f;
	float m_sun_angle = 0.0f;
	int m_sky_model = 0;
	glm::vec3 m_direction = glm::vec3(0.0f, 0.0f, 1.0f);

	BrunetonSkyModel m_bruneton_model;
	PreethamSkyModel m_preetham_model;
	HosekWilkieSkyModel m_hosek_wilkie_model;
};

DW_DECLARE_MAIN(SkyModels)
