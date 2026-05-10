#pragma once

#include "triglav/ui_core/IWidget.hpp"

namespace triglav::desktop_ui {

class DesktopContext;

class DesktopContainerWidget : public ui_core::ContainerWidget
{
 public:
   DesktopContainerWidget(ui_core::Context& context, IWidget* parent);

   [[nodiscard]] DesktopContext& desktop_context();
   [[nodiscard]] const DesktopContext& desktop_context() const;
};

class DesktopProxyWidget : public ui_core::ProxyWidget
{
 public:
   DesktopProxyWidget(ui_core::Context& context, IWidget* parent);

   [[nodiscard]] DesktopContext& desktop_context();
   [[nodiscard]] const DesktopContext& desktop_context() const;
};

}// namespace triglav::desktop_ui
