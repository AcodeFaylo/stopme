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

#ifndef WORKRAVE_UI_ISITSTANDWINDOW_HH
#define WORKRAVE_UI_ISITSTANDWINDOW_HH

#include <memory>
#include <boost/signals2.hpp>

#include "commonui/SitStandTimer.hh"

//! The card asking the user to stand up or sit down.
class ISitStandWindow
{
public:
  using Ptr = std::shared_ptr<ISitStandWindow>;

  virtual ~ISitStandWindow() = default;

  //! Shows the window.
  virtual void start() = 0;

  //! Hides the window.
  virtual void stop() = 0;

  //! The user closed the reminder in this window. The window hides itself
  //! first; it must not be destroyed from a handler of this signal.
  virtual boost::signals2::signal<void()> &signal_dismissed() = 0;
};

#endif // WORKRAVE_UI_ISITSTANDWINDOW_HH
