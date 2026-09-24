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

#ifndef SITSTANDPREFERENCEPANEL_HH
#define SITSTANDPREFERENCEPANEL_HH

#include <memory>

#include <gtkmm/box.h>

#include "ui/IApplicationContext.hh"

class TimeEntry;
class DataConnector;

namespace Gtk
{
  class CheckButton;
} // namespace Gtk

//! Settings of the reminder to switch between sitting and standing.
class SitStandPreferencePanel : public Gtk::VBox
{
public:
  explicit SitStandPreferencePanel(std::shared_ptr<IApplicationContext> app);
  ~SitStandPreferencePanel() override = default;

private:
  void on_enabled_toggled();

  std::shared_ptr<DataConnector> connector;

  Gtk::CheckButton *enabled_cb{nullptr};
  TimeEntry *interval_tim{nullptr};
};

#endif // SITSTANDPREFERENCEPANEL_HH
