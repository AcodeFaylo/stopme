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

#include "commonui/SitStandTimer.hh"

#include <algorithm>

SitStandTimer::Action
SitStandTimer::tick(const Tick &now)
{
  if (!now.enabled)
    {
      bool was_reminding = reminding;
      reset();
      return was_reminding ? Action::Withdraw : Action::Nothing;
    }

  // Suspended counts as away too, so a long suspension starts the countdown over.
  idle = (now.running && now.user_active) ? 0 : idle + 1;
  if (idle >= AWAY_SECONDS)
    {
      elapsed = 0;
      return withdraw();
    }

  if (!now.running)
    {
      return withdraw();
    }

  // Short pauses, such as reading, still count.
  elapsed++;

  if (!now.can_remind)
    {
      return withdraw();
    }

  // Wait for activity, so the reminder appears when someone is there to see it.
  if (elapsed >= std::max(now.interval, MIN_INTERVAL) && now.user_active)
    {
      posture = get_next_posture();
      elapsed = 0;
      reminding = true;
      return Action::Remind;
    }

  return Action::Nothing;
}

void
SitStandTimer::dismiss()
{
  reminding = false;
}

void
SitStandTimer::reset()
{
  posture = Posture::Sitting;
  elapsed = 0;
  idle = 0;
  reminding = false;
}

Posture
SitStandTimer::get_posture() const
{
  return posture;
}

Posture
SitStandTimer::get_next_posture() const
{
  return posture == Posture::Sitting ? Posture::Standing : Posture::Sitting;
}

int64_t
SitStandTimer::get_elapsed() const
{
  return elapsed;
}

bool
SitStandTimer::is_reminding() const
{
  return reminding;
}

SitStandTimer::Action
SitStandTimer::withdraw()
{
  if (!reminding)
    {
      return Action::Nothing;
    }
  reminding = false;
  return Action::Withdraw;
}
