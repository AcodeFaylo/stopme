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

#include <vector>

#include <gtest/gtest.h>

#include "commonui/SitStandTimer.hh"

namespace
{
  constexpr int64_t INTERVAL = 30 * 60;

  using Action = SitStandTimer::Action;

  SitStandTimer::Tick active()
  {
    SitStandTimer::Tick tick;
    tick.enabled = true;
    tick.interval = INTERVAL;
    tick.running = true;
    tick.can_remind = true;
    tick.user_active = true;
    return tick;
  }

  SitStandTimer::Tick idle()
  {
    SitStandTimer::Tick tick = active();
    tick.user_active = false;
    return tick;
  }

  //! Ticks `seconds` times and returns the actions other than Nothing.
  std::vector<Action> run(SitStandTimer &timer, const SitStandTimer::Tick &tick, int64_t seconds)
  {
    std::vector<Action> actions;
    for (int64_t i = 0; i < seconds; i++)
      {
        Action action = timer.tick(tick);
        if (action != Action::Nothing)
          {
            actions.push_back(action);
          }
      }
    return actions;
  }
} // namespace

TEST(SitStandTimerTest, FirstReminderIsToStandUp)
{
  SitStandTimer timer;

  EXPECT_TRUE(run(timer, active(), INTERVAL - 1).empty());
  EXPECT_EQ(timer.get_next_posture(), Posture::Standing);

  EXPECT_EQ(timer.tick(active()), Action::Remind);
  EXPECT_EQ(timer.get_posture(), Posture::Standing);
  EXPECT_TRUE(timer.is_reminding());
  EXPECT_EQ(timer.get_elapsed(), 0);
}

TEST(SitStandTimerTest, RemindersAlternate)
{
  SitStandTimer timer;

  run(timer, active(), INTERVAL);
  EXPECT_EQ(timer.get_posture(), Posture::Standing);
  timer.dismiss();

  EXPECT_EQ(run(timer, active(), INTERVAL), std::vector<Action>{Action::Remind});
  EXPECT_EQ(timer.get_posture(), Posture::Sitting);

  // An unanswered reminder is replaced by the next one.
  EXPECT_EQ(run(timer, active(), INTERVAL), std::vector<Action>{Action::Remind});
  EXPECT_EQ(timer.get_posture(), Posture::Standing);
}

TEST(SitStandTimerTest, ShortPausesCount)
{
  SitStandTimer timer;

  run(timer, active(), INTERVAL - 120);
  run(timer, idle(), 119);
  EXPECT_EQ(timer.get_elapsed(), INTERVAL - 1);

  EXPECT_EQ(timer.tick(active()), Action::Remind);
}

TEST(SitStandTimerTest, DueReminderWaitsForActivity)
{
  SitStandTimer timer;

  run(timer, active(), INTERVAL - 10);
  EXPECT_TRUE(run(timer, idle(), 60).empty());

  EXPECT_EQ(timer.tick(active()), Action::Remind);
}

TEST(SitStandTimerTest, BeingAwayStartsOver)
{
  SitStandTimer timer;

  run(timer, active(), INTERVAL);
  EXPECT_TRUE(timer.is_reminding());

  // The reminder that is showing is taken down, and the countdown starts over.
  EXPECT_EQ(run(timer, idle(), SitStandTimer::AWAY_SECONDS), std::vector<Action>{Action::Withdraw});
  EXPECT_EQ(timer.get_elapsed(), 0);

  // The last posture is kept: a full interval later it is time to sit down.
  EXPECT_TRUE(run(timer, active(), INTERVAL - 1).empty());
  EXPECT_EQ(timer.tick(active()), Action::Remind);
  EXPECT_EQ(timer.get_posture(), Posture::Sitting);
}

TEST(SitStandTimerTest, SuspendedPausesAndCountsAsAway)
{
  SitStandTimer timer;
  SitStandTimer::Tick suspended = active();
  suspended.running = false;
  suspended.user_active = false;

  run(timer, active(), 100);
  run(timer, suspended, SitStandTimer::AWAY_SECONDS - 1);
  EXPECT_EQ(timer.get_elapsed(), 100);

  timer.tick(suspended);
  EXPECT_EQ(timer.get_elapsed(), 0);
}

TEST(SitStandTimerTest, QuietModeHoldsTheReminderBack)
{
  SitStandTimer timer;
  SitStandTimer::Tick quiet = active();
  quiet.can_remind = false;

  EXPECT_TRUE(run(timer, quiet, 2 * INTERVAL).empty());
  EXPECT_EQ(timer.tick(active()), Action::Remind);

  // A break or quiet mode takes a showing reminder down.
  EXPECT_EQ(timer.tick(quiet), Action::Withdraw);
  EXPECT_FALSE(timer.is_reminding());
}

TEST(SitStandTimerTest, DisablingStartsOver)
{
  SitStandTimer timer;
  SitStandTimer::Tick disabled = active();
  disabled.enabled = false;

  run(timer, active(), INTERVAL);
  EXPECT_EQ(timer.tick(disabled), Action::Withdraw);
  EXPECT_EQ(timer.get_posture(), Posture::Sitting);
  EXPECT_EQ(timer.get_elapsed(), 0);
  EXPECT_TRUE(run(timer, disabled, 2 * INTERVAL).empty());
}

TEST(SitStandTimerTest, ShorterIntervalTakesEffectRightAway)
{
  SitStandTimer timer;
  SitStandTimer::Tick shorter = active();
  shorter.interval = 10 * 60;

  run(timer, active(), 20 * 60);
  EXPECT_EQ(timer.tick(shorter), Action::Remind);
}

TEST(SitStandTimerTest, TinyIntervalIsRaisedToTheMinimum)
{
  SitStandTimer timer;
  SitStandTimer::Tick tiny = active();
  tiny.interval = 0;

  EXPECT_TRUE(run(timer, tiny, SitStandTimer::MIN_INTERVAL - 1).empty());
  EXPECT_EQ(timer.tick(tiny), Action::Remind);
}
