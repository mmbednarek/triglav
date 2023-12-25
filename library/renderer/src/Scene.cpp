#include "Scene.h"

#include <glm/gtx/quaternion.hpp>

#include "Context3D.h"
#include "Renderer.h"

namespace renderer {

constexpr auto g_upVector = glm::vec3{0.0f, 0.0f, 1.0f};

Scene::Scene(Renderer &renderer, Context3D &context3D, ShadowMap &shadowMap) :
    m_renderer(renderer),
    m_context3D(context3D),
    m_shadowMap(shadowMap)
{
}

void Scene::update() const
{
   const auto [width, height] = m_renderer.screen_resolution();

   const auto view = glm::lookAt(m_camera.position, m_camera.position + m_camera.lookDirection, g_upVector);
   const auto projection = glm::perspective(
           glm::radians(45.0f), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);

   const auto shadowMapView =
           glm::lookAt(m_shadowMapCamera.position,
                       m_shadowMapCamera.position + m_shadowMapCamera.lookDirection, g_upVector);
   const auto shadowMapProjection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);

   for (const auto &obj : m_instancedObjects) {
      obj.ubo->view          = view;
      obj.ubo->proj          = projection;
      obj.shadowMap.ubo->mvp = shadowMapProjection * shadowMapView * obj.ubo->model;
      obj.ubo->shadowMapMVP  = obj.shadowMap.ubo->mvp;
   }
}

void Scene::add_object(SceneObject object)
{
   m_objects.emplace_back(std::move(object));
}

void Scene::compile_scene()
{
   m_instancedObjects.clear();

   float angle = 0.0f;

   for (const auto &obj : m_objects) {
      auto instance       = m_context3D.instance_model(obj.model, m_shadowMap);
      const auto modelMat = glm::translate(glm::scale(glm::mat4(1), obj.scale), obj.position) *
                            glm::mat4_cast(obj.rotation);
      instance.ubo->model  = modelMat;
      instance.ubo->normal = glm::transpose(glm::inverse(glm::mat3(modelMat)));
      m_instancedObjects.push_back(std::move(instance));
      angle += 45.0f;
   }
}

void Scene::render() const
{
   // TODO: Render only objects visiable by camera.

   for (const auto &obj : m_instancedObjects) {
      m_context3D.draw_model(obj);
   }
}

void Scene::render_shadow_map() const
{
   m_context3D.set_light_position(m_shadowMapCamera.position);
   // TODO: Render only objects visiable by camera.

   for (const auto &obj : m_instancedObjects) {
      m_shadowMap.draw_model(m_context3D, obj);
   }
}

void Scene::set_camera(Camera camera)
{
   // std::cout << "cam: " << camera.position.x << ", " << camera.position.y << ", " << camera.position.y
   //           << ", look: " << camera.lookDirection.x << ", " << camera.lookDirection.y << ", "
   //           << camera.lookDirection.y << '\n';
   m_camera = std::move(camera);
}

void Scene::set_shadow_x(const float x)
{
   m_shadowMapCamera.position.x = x;
}

}// namespace renderer