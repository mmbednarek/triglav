#include "MetaWidget.hpp"

#include "triglav/meta/TypeRegistry.hpp"
#include "triglav/ui_core/widget/GridLayout.hpp"
#include "triglav/ui_core/widget/TextBox.hpp"

#include "DesktopUI.hpp"
#include "DropDownMenu.hpp"
#include "TextInput.hpp"
#include "triglav/NameResolution.hpp"
#include "triglav/ResourcePathMap.hpp"
#include "triglav/project/PathManager.hpp"
#include "triglav/ui_core/widget/Padding.hpp"

namespace triglav::desktop_ui {

using namespace name_literals;

namespace {

bool is_floating_point(const Name type_name)
{
   switch (type_name) {
   case "float"_name:
   case "double"_name:
      return true;
   default:
      break;
   }
   return false;
}

bool is_integer(const Name type_name)
{
   switch (type_name) {
   case "int"_name:
   case "triglav::i8"_name:
   case "triglav::u8"_name:
   case "triglav::i16"_name:
   case "triglav::u16"_name:
   case "triglav::i32"_name:
   case "triglav::u32"_name:
   case "triglav::i64"_name:
   case "triglav::u64"_name:
      return true;
   default:
      break;
   }
   return false;
}

bool int_filter(const Rune r)
{
   return r == '.' || (r >= '0' && r <= '9');
}

bool float_filter(const Rune r)
{
   return int_filter(r) || r == '.';
}

bool is_numeric(const Name type_name)
{
   return is_floating_point(type_name) || is_integer(type_name);
}

[[maybe_unused]] String property_to_string(const meta::PropertyRef& ref)
{
   switch (ref.type()) {
   case "int"_name:
      return to_string(ref.get<int>());
   case "triglav::i8"_name:
      return to_string(ref.get<i8>());
   case "triglav::u8"_name:
      return to_string(ref.get<u8>());
   case "triglav::i16"_name:
      return to_string(ref.get<i16>());
   case "triglav::u16"_name:
      return to_string(ref.get<u16>());
   case "triglav::i32"_name:
      return to_string(ref.get<i32>());
   case "triglav::u32"_name:
      return to_string(ref.get<u32>());
   case "triglav::i64"_name:
      return to_string(ref.get<i64>());
   case "triglav::u64"_name:
      return to_string(ref.get<u64>());
   case "float"_name:
      return to_string(ref.get<float>());
   case "double"_name:
      return to_string(ref.get<double>());
   default:
      break;
   }

   return {};
}

void property_set_string(const meta::PropertyRef& ref, const StringView value)
{
   switch (ref.type()) {
   case "int"_name:
      ref.set(from_string<int>(value));
      break;
   case "triglav::i8"_name:
      ref.set(from_string<i8>(value));
      break;
   case "triglav::u8"_name:
      ref.set(from_string<u8>(value));
      break;
   case "triglav::i16"_name:
      ref.set(from_string<i16>(value));
      break;
   case "triglav::u16"_name:
      ref.set(from_string<u16>(value));
      break;
   case "triglav::i32"_name:
      ref.set(from_string<i32>(value));
      break;
   case "triglav::u32"_name:
      ref.set(from_string<u32>(value));
      break;
   case "triglav::i64"_name:
      ref.set(from_string<i64>(value));
      break;
   case "triglav::u64"_name:
      ref.set(from_string<u64>(value));
      break;
   case "float"_name:
      ref.set(from_string<float>(value));
      break;
   case "double"_name:
      ref.set(from_string<double>(value));
      break;
   default:
      break;
   }
}

ResourceType resource_type_from_type_name(const Name type_name)
{
   switch (type_name) {
#define TG_RESOURCE_TYPE(name, ext, cpp_type, loading_stage)     \
   case make_name_id(TG_STRING(triglav::TG_CONCAT(name, Name))): \
      return ResourceType::name;
      TG_RESOURCE_TYPE_LIST
#undef TG_RESOURCE_TYPE
   }

   assert(false);
   return {};
}

class StringProperty : public DesktopProxyWidget
{
 public:
   struct State
   {
      IMetaProvider* provider;
      Name property_name;
   };

   StringProperty(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
       DesktopProxyWidget(ctx, parent),
       m_state(state)
   {
      const auto reference = m_state.provider->get_reference();
      const auto& text_input = this->create_content<TextInput>({
         .text = String{*reference.property<std::string>(m_state.property_name)},
      });
      m_text_changed_sink = text_input.event_OnTextChanged.connect<&StringProperty::on_text_changed>(*this);
   }

   void on_text_changed(const StringView text) const
   {
      const auto reference = m_state.provider->get_reference();
      reference.property<std::string>(m_state.property_name) = std::string{text.to_std()};
      m_state.provider->mutate();
   }

 private:
   State m_state;
   Sink m_text_changed_sink;
};

class ResourceNameProperty : public DesktopProxyWidget
{
 public:
   struct State
   {
      IMetaProvider* provider;
      Name property_name;
   };

   ResourceNameProperty(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
       DesktopProxyWidget(ctx, parent),
       m_state(state)
   {
      const auto reference = m_state.provider->get_reference();
      const auto property = reference.property_ref(m_state.property_name);

      // This is UB, but should work
      const auto name = property.get<Name>();
      const auto resource_type = resource_type_from_type_name(property.type());
      m_resource_name = ResourceName(resource_type, name);

      const auto& text_input = this->create_content<TextInput>({
         .text = ResourcePathMap::the().resolve(m_resource_name),
      });
      m_text_changed_sink = text_input.event_OnTextChanged.connect<&ResourceNameProperty::on_text_changed>(*this);
   }

   void on_text_changed(const StringView text) const
   {
      const auto path = name_from_path(text);
      const auto reference = m_state.provider->get_reference();
      reference.property<Name>(m_state.property_name) = path.name();
      m_state.provider->mutate();
   }

 private:
   State m_state;
   Sink m_text_changed_sink;
   ResourceName m_resource_name;
};

class NameProperty : public DesktopProxyWidget
{
 public:
   struct State
   {
      IMetaProvider* provider;
      Name property_name;
   };

   NameProperty(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
       DesktopProxyWidget(ctx, parent),
       m_state(state)
   {
      const auto reference = m_state.provider->get_reference();
      const auto property = reference.property_ref(m_state.property_name);

      const auto name = property.get<Name>();
      std::string resolved = std::string{resolve_name(name)};
      if (resolved.empty()) {
         resolved = std::to_string(name);
      }

      const auto& text_input = this->create_content<TextInput>({
         .text = String{resolved},
      });
      m_text_changed_sink = text_input.event_OnTextChanged.connect<&NameProperty::on_text_changed>(*this);
   }

   void on_text_changed(const StringView text) const
   {
      const auto reference = m_state.provider->get_reference();
      reference.property<Name>(m_state.property_name) = make_name_id(text.to_std());
      m_state.provider->mutate();
   }

 private:
   State m_state;
   Sink m_text_changed_sink;
};

class NumericProperty : public DesktopProxyWidget
{
 public:
   struct State
   {
      IMetaProvider* provider;
      Name property_name;
   };

   NumericProperty(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
       DesktopProxyWidget(ctx, parent),
       m_state(state)
   {
      const auto reference = m_state.provider->get_reference();
      const auto property = reference.property_ref(m_state.property_name);

      auto func = is_floating_point(property.type()) ? float_filter : int_filter;

      const auto& text_input = this->create_content<TextInput>({
         .text = property_to_string(property),
         .filter_func = func,
      });
      m_text_changed_sink = text_input.event_OnTextChanged.connect<&NumericProperty::on_text_changed>(*this);
   }

   void on_text_changed(const StringView text) const
   {
      const auto reference = m_state.provider->get_reference();
      property_set_string(reference.property_ref(m_state.property_name), text);
      m_state.provider->mutate();
   }

 private:
   State m_state;
   Sink m_text_changed_sink;
};

constexpr Color RED_OUTLINE{0.62f, 0.34f, 0.33f, 1.0f};
constexpr Color GREEN_OUTLINE{0.33f, 0.63f, 0.33f, 1.0f};
constexpr Color BLUE_OUTLINE{0.32f, 0.43f, 0.62f, 1.0f};

struct AxisInfo
{
   u32 index;
   Axis axis;
   Color outline_color;
};

std::array AXIS_INFOS = {
   AxisInfo{
      .index = 0,
      .axis = Axis::X,
      .outline_color = RED_OUTLINE,
   },
   AxisInfo{
      .index = 1,
      .axis = Axis::Y,
      .outline_color = GREEN_OUTLINE,
   },
   AxisInfo{
      .index = 2,
      .axis = Axis::Z,
      .outline_color = BLUE_OUTLINE,
   },
};

class VectorProperty : public DesktopProxyWidget
{
 public:
   struct State
   {
      IMetaProvider* provider;
      Name property_name;
   };

   struct AxisContext
   {
      VectorProperty* property;
      Axis axis;
   };

   VectorProperty(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
       DesktopProxyWidget(ctx, parent),
       m_state(state)
   {
      const auto reference = m_state.provider->get_reference();

      m_values = *reference.property<Vector3>(m_state.property_name);

      auto& grid = this->create_content<ui_core::GridLayout>({
         .column_ratios = {0.333f, 0.333f, 0.333f},
         .row_ratios = {1.0f},
         .horizontal_spacing = 5.0f,
         .vertical_spacing = 0.0f,
      });

      for (const auto& info : AXIS_INFOS) {
         const auto& input = grid.create_child<TextInput>({
            .text = to_string<float>(vector3_component(m_values, info.axis)),
            .filter_func = float_filter,
            .border_color = info.outline_color,
         });

         m_axis_contexts[info.index] = AxisContext{
            .property = this,
            .axis = info.axis,
         };

         m_sinks[info.index] =
            input.event_OnTextChanged.connect_raw(&m_axis_contexts[info.index], [](void* handle, const StringView value) {
               const auto* axis_context = static_cast<AxisContext*>(handle);
               axis_context->property->update_value(from_string<float>(value), axis_context->axis);
            });
      }
   }

   void update_value(const float value, const Axis axis)
   {
      const auto reference = m_state.provider->get_reference();
      vector3_component(m_values, axis) = value;
      reference.property<Vector3>(m_state.property_name) = Vector3{m_values};
      m_state.provider->mutate();
   }

 private:
   State m_state;
   std::array<Sink, AXIS_INFOS.size()> m_sinks{};
   std::array<AxisContext, AXIS_INFOS.size()> m_axis_contexts{};
   Vector3 m_values{};
};

class QuaternionProperty : public DesktopProxyWidget
{
 public:
   struct State
   {
      IMetaProvider* provider;
      Name property_name;
   };

   struct AxisContext
   {
      QuaternionProperty* property;
      Axis axis;
   };

   QuaternionProperty(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
       DesktopProxyWidget(ctx, parent),
       m_state(state)
   {
      const auto reference = m_state.provider->get_reference();

      const auto& quat = *reference.property<Quaternion>(m_state.property_name);
      m_euler_degrees = glm::degrees(glm::eulerAngles(quat));

      auto& grid = this->create_content<ui_core::GridLayout>({
         .column_ratios = {0.333f, 0.333f, 0.333f},
         .row_ratios = {1.0f},
         .horizontal_spacing = 5.0f,
         .vertical_spacing = 0.0f,
      });

      for (const auto& info : AXIS_INFOS) {
         const auto& input = grid.create_child<TextInput>({
            .text = to_string<float>(vector3_component(m_euler_degrees, info.axis)),
            .filter_func = float_filter,
            .border_color = info.outline_color,
         });

         m_axis_contexts[info.index] = AxisContext{
            .property = this,
            .axis = info.axis,
         };

         m_sinks[info.index] =
            input.event_OnTextChanged.connect_raw(&m_axis_contexts[info.index], [](void* handle, const StringView value) {
               const auto* axis_context = static_cast<AxisContext*>(handle);
               axis_context->property->update_value(from_string<float>(value), axis_context->axis);
            });
      }
   }

   void update_value(const float value, const Axis axis)
   {
      vector3_component(m_euler_degrees, axis) = value;
      const auto reference = m_state.provider->get_reference();
      reference.property<Quaternion>(m_state.property_name) = Quaternion{glm::radians(m_euler_degrees)};
      m_state.provider->mutate();
   }

 private:
   State m_state;
   std::array<Sink, AXIS_INFOS.size()> m_sinks{};
   std::array<AxisContext, AXIS_INFOS.size()> m_axis_contexts{};
   Vector3 m_euler_degrees{};
};


class EnumProperty : public DesktopProxyWidget
{
 public:
   struct State
   {
      IMetaProvider* provider;
      Name property_name;
   };

   EnumProperty(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
       DesktopProxyWidget(ctx, parent),
       m_state(state)
   {
      const auto reference = m_state.provider->get_reference();
      const auto property = reference.property_ref(m_state.property_name);
      const auto enum_ref = property.to_enum_ref();

      const auto& enum_type = meta::TypeRegistry::the().type_info(property.type());

      std::vector<String> strings;
      for (const auto& member : enum_type.members) {
         if (member.role_flags & meta::MemberRole::EnumValue) {
            strings.emplace_back(member.identifier);
         }
      }

      [[maybe_unused]] const auto& drop_down = this->create_content<DropDownMenu>({
         .items = std::move(strings),
         .selected_item = static_cast<u32>(enum_ref.value()),
      });
      m_selected_sink = drop_down.event_OnSelected.connect<&EnumProperty::on_selected>(*this);
   }

   void on_selected(const u32 index) const
   {
      const auto reference = m_state.provider->get_reference();
      const auto property = reference.property_ref(m_state.property_name);
      const auto enum_ref = property.to_enum_ref();
      enum_ref.set(index);

      m_state.provider->mutate();
   }

 private:
   State m_state;
   Sink m_selected_sink;
};

String make_property_label(const StringView view)
{
   String result;
   bool is_new_word = true;
   for (const auto ch : view) {
      if (ch == '_') {
         result.append_rune(' ');
         is_new_word = true;
         continue;
      }
      if (is_new_word) {
         result.append_rune(std::toupper(static_cast<int>(ch)));
         is_new_word = false;
         continue;
      }
      result.append_rune(ch);
   }

   return result;
}

}// namespace

MetaWidget::MetaWidget(ui_core::Context& ctx, State&& state, ui_core::IWidget* parent) :
    DesktopProxyWidget(ctx, parent),
    m_state(std::move(state))
{
   const meta::ClassRef class_ref(nullptr, state.meta_type);
   auto range = class_ref.properties();
   const auto prop_count = std::distance(range.begin(), range.end());

   const float single_width = 1.0f / static_cast<float>(prop_count);

   m_root_layout = &this->create_content<ui_core::Padding>({4.0f, 4.0f, 4.0f, 4.0f})
                       .create_content<ui_core::GridLayout>({
                          .column_ratios = {0.3, 0.7f},
                          .row_ratios = std::vector<float>(prop_count, single_width),
                          .horizontal_spacing = 10.0f,
                          .vertical_spacing = 10.0f,
                       });

   for (auto property : class_ref.properties()) {
      m_root_layout->create_child<ui_core::TextBox>({
         .font_size = TG_THEME_VAL(base_font_size),
         .typeface = TG_THEME_VAL(base_typeface),
         .content = make_property_label(StringView{property.identifier()}),
         .color = TG_THEME_VAL(foreground_color),
         .horizontal_alignment = ui_core::HorizontalAlignment::Left,
         .vertical_alignment = ui_core::VerticalAlignment::Center,
      });

      switch (property.ref_kind()) {
      case meta::RefKind::Primitive:
         this->handle_property(property);
         break;
      case meta::RefKind::Enum:
         this->handle_enum(property.name());
         break;
      default:
         break;
      }
   }
}

void MetaWidget::handle_property(const meta::PropertyRef& property_ref)
{
   if (is_numeric(property_ref.type())) {
      m_root_layout->create_child<NumericProperty>({
         .provider = m_state.provider.get(),
         .property_name = property_ref.name(),
      });
      return;
   }

   switch (property_ref.type()) {
   case "std::string"_name:
      m_root_layout->create_child<StringProperty>({
         .provider = m_state.provider.get(),
         .property_name = property_ref.name(),
      });
      break;
   case "triglav::Vector3"_name:
      m_root_layout->create_child<VectorProperty>({
         .provider = m_state.provider.get(),
         .property_name = property_ref.name(),
      });
      break;
   case "triglav::Quaternion"_name:
      m_root_layout->create_child<QuaternionProperty>({
         .provider = m_state.provider.get(),
         .property_name = property_ref.name(),
      });
      break;
   case "triglav::Name"_name:
      m_root_layout->create_child<NameProperty>({
         .provider = m_state.provider.get(),
         .property_name = property_ref.name(),
      });
      break;
#define TG_RESOURCE_TYPE(res_name, ext, cpp_type, loading_stage)     \
   case make_name_id(TG_STRING(triglav::TG_CONCAT(res_name, Name))): \
      m_root_layout->create_child<ResourceNameProperty>({            \
         .provider = m_state.provider.get(),                         \
         .property_name = property_ref.name(),                       \
      });                                                            \
      break;
      TG_RESOURCE_TYPE_LIST
#undef TG_RESOURCE_TYPE
   default:
      assert(false);
      break;
   }
}

void MetaWidget::handle_enum(const Name prop_name) const
{
   m_root_layout->create_child<EnumProperty>({
      .provider = m_state.provider.get(),
      .property_name = prop_name,
   });
}

}// namespace triglav::desktop_ui
