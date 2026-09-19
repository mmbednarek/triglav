#pragma once

#include "DesktopContainerWidget.hpp"
#include "triglav/meta/Meta.hpp"

namespace triglav::ui_core {
class GridLayout;
}

namespace triglav::desktop_ui {

class IMetaProvider
{
 public:
   virtual ~IMetaProvider() = default;

   [[nodiscard]] virtual meta::ClassRef get_reference() const = 0;
   virtual void mutate() = 0;
};

class BasicMetaProvider final : public IMetaProvider
{
 public:
   meta::ClassRef reference;

   explicit BasicMetaProvider(const meta::ClassRef& reference) :
       reference(reference)
   {
   }

   [[nodiscard]] meta::ClassRef get_reference() const override
   {
      return reference;
   }

   void mutate() override
   {
      // nothing to do
   }
};

class MetaWidget : public DesktopProxyWidget
{
 public:
   struct State
   {
      Name meta_type;
      std::unique_ptr<IMetaProvider> provider;
   };

   MetaWidget(ui_core::Context& ctx, State&& state, IWidget* parent);

 private:
   void handle_property(const meta::PropertyRef& property_ref);
   void handle_enum(Name prop_name) const;

   State m_state;
   ui_core::GridLayout* m_root_layout{};
};

}// namespace triglav::desktop_ui
