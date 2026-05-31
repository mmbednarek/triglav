#include "TerrainTool.hpp"

#include "DecalRenderingStage.hpp"
#include "LevelEditor.hpp"

namespace triglav::editor {

TerrainTool::TerrainTool(LevelEditor& level_editor) :
    m_level_editor(level_editor),
    m_canvas(m_level_editor.scene())
{
}

bool TerrainTool::on_use_start(const geometry::Ray& ray)
{
   const auto pos = m_canvas.trace_ray(ray);
   if (!pos.has_value()) {
      m_is_being_used = false;
      return true;
   }
   m_is_being_used = true;
   m_pointer_position = m_canvas.world_pos_to_coord(*pos);
   return true;
}

void TerrainTool::on_mouse_moved(const Vector2 position)
{
   auto world_position = m_canvas.trace_ray(m_level_editor.viewport_ray(position));
   if (!world_position.has_value())
      return;

   const auto coord = m_canvas.world_pos_to_coord(*world_position);
   const auto level = m_canvas.sample(coord);

   world_position->z = m_canvas.height_to_world(level);

   const auto brush_world_size = m_canvas.distance_to_world(static_cast<float>(m_brush_size));

   const Matrix4x4 mat =
      glm::scale(glm::translate(Matrix4x4{1}, *world_position), Vector3{brush_world_size, brush_world_size, brush_world_size});
   m_level_editor.decal_rendering_stage().set_matrix(mat);
   m_level_editor.decal_rendering_stage().set_info(*world_position, brush_world_size);

   if (!m_is_being_used)
      return;

   m_pointer_position = m_canvas.world_pos_to_coord(*world_position);
}

void TerrainTool::on_view_updated() {}

void TerrainTool::on_use_end()
{
   m_is_being_used = false;
}

void TerrainTool::on_left_tool()
{
   m_is_being_used = false;
}

}// namespace triglav::editor
