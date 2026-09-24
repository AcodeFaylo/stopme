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

#include "SitStandPreferencePanel.hh"

#include <gtkmm/checkbutton.h>
#include <gtkmm/label.h>

#include "commonui/nls.h"
#include "ui/GUIConfig.hh"

#include "DataConnector.hh"
#include "GtkUtil.hh"
#include "Hig.hh"
#include "TimeEntry.hh"

SitStandPreferencePanel::SitStandPreferencePanel(std::shared_ptr<IApplicationContext> app)
  : Gtk::VBox(false, 6)
  , connector(std::make_shared<DataConnector>(app))
{
  Gtk::Label *enabled_lab = Gtk::manage(GtkUtil::create_label(_("Remind me to switch between sitting and standing"), true));
  enabled_cb = Gtk::manage(new Gtk::CheckButton());
  enabled_cb->add(*enabled_lab);
  enabled_cb->signal_toggled().connect(sigc::mem_fun(*this, &SitStandPreferencePanel::on_enabled_toggled));

  auto *hig = Gtk::manage(new HigCategoryPanel(_("Reminder")));

  interval_tim = Gtk::manage(new TimeEntry());
  hig->add_label(std::string(_("Time between reminders")) + ":", *interval_tim);

  auto *hint = Gtk::manage(
    new Gtk::Label(_("The first reminder asks you to stand up; after that they alternate. "
                     "Time away from the computer does not count: after five minutes "
                     "away, the countdown starts over.")));
  hint->set_line_wrap(true);
  hint->set_max_width_chars(60);
  hint->set_xalign(0.0F);
  hig->add_widget(*hint);

  auto *categories = Gtk::manage(new HigCategoriesPanel());
  categories->add(*hig);

  pack_start(*enabled_cb, false, false, 0);
  pack_start(*categories, false, false, 0);

  connector->connect(GUIConfig::sit_stand_enabled(), dc::wrap(enabled_cb));
  connector->connect(GUIConfig::sit_stand_interval(), dc::wrap(interval_tim));

  on_enabled_toggled();

  set_border_width(12);
}

void
SitStandPreferencePanel::on_enabled_toggled()
{
  interval_tim->set_sensitive(enabled_cb->get_active());
}
