#include "TerrainTool.hpp"

#include "DecalRenderingStage.hpp"
#include "LevelEditor.hpp"

namespace triglav::editor {

TerrainTool::TerrainTool(LevelEditor& level_editor) :
    m_level_editor(level_editor)
{
}

bool TerrainTool::on_use_start(const geometry::Ray& ray)
{
   if (m_canvas == nullptr) {
      if (!m_level_editor.level().has_component<world::TerrainComponent>(m_level_editor.selected_object_id()))
         return false;

      m_canvas = std::make_unique<TerrainCanvas>(m_level_editor.level(), m_level_editor.selected_object_id());
   }

   const auto pos = m_canvas->trace_ray(ray);
   if (!pos.has_value()) {
      m_is_being_used = false;
      return true;
   }
   m_is_being_used = true;
   m_pointer_position = *pos;
   return true;
}

void TerrainTool::on_mouse_moved(const Vector2 position)
{
   if (m_canvas == nullptr)
      return;

   auto terrain_coord = m_canvas->trace_ray(m_level_editor.viewport_ray(position, 1000.0f));
   if (!terrain_coord.has_value())
      return;

   const auto world_pos = m_canvas->coord_to_world(*terrain_coord);

   const auto brush_world_size = m_canvas->distance_to_world(m_canvas->brush_size());

   const Matrix4x4 mat =
      Matrix4x4::translation(world_pos) * Matrix3x3::scale(Vector3{brush_world_size, brush_world_size, brush_world_size}).extend();
   m_level_editor.decal_rendering_stage().set_matrix(mat);
   m_level_editor.decal_rendering_stage().set_info(world_pos, brush_world_size);

   if (!m_is_being_used)
      return;

   m_pointer_position = *terrain_coord;
}

void TerrainTool::on_view_updated() {}

void TerrainTool::on_use_end()
{
   m_is_being_used = false;
}

void TerrainTool::on_left_tool()
{
   m_canvas.reset();
   m_is_being_used = false;
}

void TerrainTool::set_brush_size(const float size) const
{
   m_canvas->set_brush_size(size);
}

void TerrainTool::set_strength(const float strength)
{
   m_strength = strength;
}

}// namespace triglav::editor
