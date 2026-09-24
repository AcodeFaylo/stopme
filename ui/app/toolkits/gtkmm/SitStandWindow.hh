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

#ifndef SITSTANDWINDOW_HH
#define SITSTANDWINDOW_HH

#include <memory>

#include <gtkmm.h>

#include "HeadInfo.hh"
#include "ui/IApplicationContext.hh"
#include "ui/ISitStandWindow.hh"

#if defined(HAVE_WAYLAND)
#  include "WaylandWindowManager.hh"
#endif

class Frame;

//! A card at the top of the screen that asks the user to stand up or sit down.
/*!
 *  It is placed like the break warning, but stays until the user presses OK.
 */
class SitStandWindow
  : public Gtk::Window
  , public ISitStandWindow
{
public:
  SitStandWindow(std::shared_ptr<IApplicationContext> app, HeadInfo head, Posture posture);
  ~SitStandWindow() override = default;

  void start() override;
  void stop() override;
  boost::signals2::signal<void()> &signal_dismissed() override;

private:
  void add(Gtk::Widget &widget) override;

  void on_ok_clicked();
  bool on_draw_event(const ::Cairo::RefPtr<::Cairo::Context> &cr);
  void on_screen_changed_event(const Glib::RefPtr<Gdk::Screen> &previous_screen);
  void on_size_allocate_event(Gtk::Allocation &allocation);

private:
  std::shared_ptr<IApplicationContext> app;

  //! Distance from the top of the screen.
  const int SCREEN_MARGIN{20};

  HeadInfo head;

  //! Border of the card.
  Frame *window_frame{nullptr};

  //! Places the card in a screen-sized window where windows cannot be moved.
  Gtk::Alignment *align{nullptr};

  boost::signals2::signal<void()> dismissed_signal;

#if defined(HAVE_WAYLAND)
  std::shared_ptr<WaylandWindowManager> window_manager;
  std::shared_ptr<WaylandLayerSurface> layer_surface;
#endif
};

#endif // SITSTANDWINDOW_HH
