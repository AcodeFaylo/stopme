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
  // One second is expected. The clock can also be set back: count nothing then.
  int64_t seconds = last_time != 0 ? std::max<int64_t>(now.time - last_time, 0) : 1;
  last_time = now.time;

  if (!now.enabled)
    {
      bool was_reminding = reminding;
      reset();
      return was_reminding ? Action::Withdraw : Action::Nothing;
    }

  // Seconds without a tick mean the computer slept, or stopme could not run.
  // Nobody saw what the user did meanwhile, so they count as time away.
  bool away = seconds > 1 && count(seconds - 1, false, now.running);

  // Suspended counts as away too, so a long suspension starts the countdown over.
  away = count(std::min<int64_t>(seconds, 1), now.running && now.user_active, now.running) || away;

  if (away || !now.running || !now.can_remind)
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

//! Counts `seconds` spent at the computer or not; returns whether the user
//! has now been away long enough for the countdown to start over.
bool
SitStandTimer::count(int64_t seconds, bool present, bool running)
{
  idle = present ? 0 : idle + seconds;
  if (idle >= AWAY_SECONDS)
    {
      elapsed = 0;
      return true;
    }

  if (running)
    {
      // Short pauses, such as reading, still count.
      elapsed += seconds;
    }
  return false;
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
