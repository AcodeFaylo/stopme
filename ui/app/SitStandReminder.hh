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

#ifndef SITSTANDREMINDER_HH
#define SITSTANDREMINDER_HH

#include <cstdint>
#include <memory>
#include <vector>

#include <boost/signals2.hpp>

#include "commonui/SitStandTimer.hh"
#include "ui/IApplicationContext.hh"
#include "ui/ISitStandWindow.hh"

//! Reminds the user, on every screen, to stand up or sit down.
class SitStandReminder
{
public:
  explicit SitStandReminder(std::shared_ptr<IApplicationContext> context);
  ~SitStandReminder();

  SitStandReminder(const SitStandReminder &) = delete;
  SitStandReminder &operator=(const SitStandReminder &) = delete;

  //! Must be called once a second.
  void heartbeat();

  //! Whether the reminder is switched on.
  bool is_enabled() const;

  //! The posture the next reminder asks for.
  Posture get_next_posture() const;

  //! Seconds until the next reminder is due.
  int64_t get_remaining() const;

private:
  void show(Posture posture);
  void hide();
  void on_dismissed();
  bool is_break_active() const;

private:
  std::shared_ptr<IApplicationContext> context;
  SitStandTimer timer;
  std::vector<ISitStandWindow::Ptr> windows;
  std::vector<boost::signals2::scoped_connection> connections;
};

#endif // SITSTANDREMINDER_HH
