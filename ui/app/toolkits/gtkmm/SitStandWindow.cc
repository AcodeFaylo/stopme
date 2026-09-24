// Copyright (C) 2026 The stopme contributors
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "SitStandWindow.hh"

#include <utility>

#include "commonui/nls.h"
#include "debug.hh"
#include "utils/Platform.hh"

#include "Frame.hh"
#include "GtkUtil.hh"
#include "Hig.hh"

#if defined(HAVE_WAYLAND)
#  include "IToolkitUnixPrivate.hh"
#endif

using namespace workrave::utils;

SitStandWindow::SitStandWindow(std::shared_ptr<IApplicationContext> app, HeadInfo head, Posture posture)
  : Gtk::Window(Gtk::WINDOW_POPUP)
  , app(std::move(app))
  , head(std::move(head))
{
  TRACE_ENTRY();
#if defined(HAVE_WAYLAND)
  if (auto toolkit_priv = std::dynamic_pointer_cast<IToolkitUnixPrivate>(this->app->get_toolkit()); toolkit_priv)
    {
      window_manager = toolkit_priv->get_wayland_window_manager();
    }
#endif

  // On W32, must be *before* realize, otherwise a border is drawn.
  set_resizable(false);
  set_decorated(false);

  Gtk::Window::set_border_width(0);

  if (!Platform::can_position_windows())
    {
      // A transparent window the size of the screen, with the card inside it.
      set_app_paintable(true);
      signal_draw().connect(sigc::mem_fun(*this, &SitStandWindow::on_draw_event), false);
      signal_screen_changed().connect(sigc::mem_fun(*this, &SitStandWindow::on_screen_changed_event));
      on_screen_changed_event(get_screen());
      set_size_request(this->head.get_width(), this->head.get_height());

      // Placing the window on the right monitor is left to arm_unfullscreen(),
      // which runs from start() before the window is mapped.
    }

  realize();

  bool standing = posture == Posture::Standing;

  auto *image = GtkUtil::create_image(standing ? "stopme-panda-standing.png" : "stopme-panda-sitting.png");

  auto *label = Gtk::manage(new Gtk::Label());
  label->set_markup(HigUtil::create_alert_text(standing ? _("Time to stand up") : _("Time to sit down"),
                                               standing
                                                 ? _("Work standing for a while. stopme will tell you when to sit down again.")
                                                 : _("Work sitting for a while. stopme will tell you when to stand up again.")));
  label->set_line_wrap(true);
  label->set_max_width_chars(36);
  label->set_xalign(0.0F);
  label->set_valign(Gtk::ALIGN_CENTER);

  auto *ok_button = Gtk::manage(new Gtk::Button(_("OK")));
  ok_button->set_can_focus(false);
  ok_button->signal_clicked().connect(sigc::mem_fun(*this, &SitStandWindow::on_ok_clicked));

  auto *button_box = Gtk::manage(new Gtk::ButtonBox(Gtk::ORIENTATION_HORIZONTAL));
  button_box->set_layout(Gtk::BUTTONBOX_END);
  button_box->pack_end(*ok_button, false, false, 0);

  auto *vbox = Gtk::manage(new Gtk::VBox(false, 12));
  vbox->pack_start(*label, true, true, 0);
  vbox->pack_start(*button_box, false, false, 0);

  auto *hbox = Gtk::manage(new Gtk::HBox(false, 12));
  if (image != nullptr)
    {
      image->set_valign(Gtk::ALIGN_CENTER);
      hbox->pack_start(*image, false, false, 0);
    }
  hbox->pack_start(*vbox, true, true, 0);
  hbox->set_border_width(12);

  add(*hbox);

  set_can_focus(false);
  set_accept_focus(false);
  set_focus_on_map(false);

  show_all_children();
  stick();
}

void
SitStandWindow::start()
{
  TRACE_ENTRY();
  realize_if_needed();

#if defined(HAVE_WAYLAND)
  if (window_manager)
    {
      layer_surface = window_manager->init_surface(*this, head.get_monitor(), false);
    }
#endif

  set_skip_pager_hint(true);
  set_skip_taskbar_hint(true);

  GtkUtil::set_always_on_top(this, true);

  if (Platform::can_position_windows())
    {
      auto [x, y] = GtkUtil::get_centered_position(*this, head);
      set_position(Gtk::WIN_POS_NONE);
      move(x, head.get_y() + SCREEN_MARGIN);
    }
#if defined(HAVE_WAYLAND)
  arm_unfullscreen();
#endif

  show_all();

  GtkUtil::set_always_on_top(this, true);
}

void
SitStandWindow::stop()
{
  TRACE_ENTRY();
#if defined(HAVE_WAYLAND)
  layer_surface.reset();
  unfullscreen_connection.disconnect();
  unfullscreen_pending = false;
#endif

  hide();
}

#if defined(HAVE_WAYLAND)
//! Maps the window fullscreen on its own monitor, and leaves fullscreen again.
/*!
 *  The same fallback as PreludeWindow::arm_unfullscreen(): without the layer
 *  shell protocol (Mutter), a Wayland client cannot pick the output it
 *  appears on, so each screen-sized window is briefly made fullscreen on the
 *  monitor of its head.
 */
void
SitStandWindow::arm_unfullscreen()
{
  TRACE_ENTRY();
  if (window_manager || !Platform::running_on_wayland() || Platform::can_position_windows())
    {
      return;
    }

  if (!app || (app->get_toolkit()->get_head_count() <= 1))
    {
      return;
    }

  const auto screen = get_screen();
  if (!screen)
    {
      return;
    }

  const int monitor_index = head.get_monitor_index(screen);
  if (monitor_index < 0)
    {
      TRACE_MSG("no monitor index for head");
      return;
    }

  unfullscreen_connection.disconnect();
  unfullscreen_pending = true;
  fullscreen_on_monitor(screen, monitor_index);
}

bool
SitStandWindow::on_window_state_event(GdkEventWindowState *event)
{
  if (unfullscreen_pending && ((event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) != 0)
      && ((event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0))
    {
      unfullscreen_pending = false;

      // Leave the fullscreen state only after the compositor has presented a
      // frame. Unsetting it right away allows the compositor to coalesce both
      // requests, in which case the window never moves to the right monitor.
      unfullscreen_connection = Glib::signal_timeout().connect(
        [this]() {
          unfullscreen();
          return false;
        },
        100);
    }

  return Gtk::Window::on_window_state_event(event);
}
#endif

boost::signals2::signal<void()> &
SitStandWindow::signal_dismissed()
{
  return dismissed_signal;
}

void
SitStandWindow::add(Gtk::Widget &widget)
{
  if (window_frame == nullptr)
    {
      window_frame = Gtk::manage(new Frame());
      window_frame->set_border_width(0);
      window_frame->set_frame_style(Frame::STYLE_BREAK_WINDOW);

      if (!Platform::can_position_windows())
        {
          align = Gtk::manage(new Gtk::Alignment(0.5, 0.0, 0.0, 0.0));
          align->set_padding(SCREEN_MARGIN, 0, 0, 0);
          align->add(*window_frame);
          Gtk::Window::add(*align);

          window_frame->signal_size_allocate().connect(sigc::mem_fun(*this, &SitStandWindow::on_size_allocate_event));
        }
      else
        {
          Gtk::Window::add(*window_frame);
        }
    }

  window_frame->add(widget);
}

void
SitStandWindow::on_ok_clicked()
{
  hide();
  dismissed_signal();
}

bool
SitStandWindow::on_draw_event(const Cairo::RefPtr<Cairo::Context> &cr)
{
  cr->save();
  cr->set_source_rgba(0.0, 0.0, 0.0, 0.0);
#if CAIROMM_CHECK_VERSION(1, 15, 4)
  cr->set_operator(Cairo::Context::Operator::SOURCE);
#else
  cr->set_operator(Cairo::OPERATOR_SOURCE);
#endif
  cr->paint();
  cr->restore();

  return Gtk::Window::on_draw(cr);
}

void
SitStandWindow::on_screen_changed_event(const Glib::RefPtr<Gdk::Screen> &previous_screen)
{
  (void)previous_screen;

  const Glib::RefPtr<Gdk::Screen> screen = get_screen();
  const Glib::RefPtr<Gdk::Visual> visual = screen->get_rgba_visual();

  if (visual)
    {
      gtk_widget_set_visual(GTK_WIDGET(gobj()), visual->gobj());
    }
}

void
SitStandWindow::on_size_allocate_event(Gtk::Allocation &allocation)
{
  // Only the card takes clicks; the rest of the screen-sized window lets them through.
  Glib::RefPtr<Gdk::Window> window = get_window();
  if (window)
    {
      Cairo::RectangleInt rect = {allocation.get_x(), allocation.get_y(), allocation.get_width(), allocation.get_height()};
      window->input_shape_combine_region(Cairo::Region::create(rect), 0, 0);
    }
}
