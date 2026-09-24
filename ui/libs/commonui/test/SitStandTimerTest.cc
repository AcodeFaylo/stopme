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
} // namespace

class SitStandTimerTest : public ::testing::Test
{
protected:
  //! Ticks one second after the previous tick.
  Action tick(SitStandTimer::Tick now)
  {
    now.time = ++clock;
    return timer.tick(now);
  }

  //! Ticks once a second for `seconds` seconds; returns the actions other than Nothing.
  std::vector<Action> run(const SitStandTimer::Tick &now, int64_t seconds)
  {
    std::vector<Action> actions;
    for (int64_t i = 0; i < seconds; i++)
      {
        Action action = tick(now);
        if (action != Action::Nothing)
          {
            actions.push_back(action);
          }
      }
    return actions;
  }

  SitStandTimer timer;
  int64_t clock{1000000};
};

TEST_F(SitStandTimerTest, FirstReminderIsToStandUp)
{
  EXPECT_TRUE(run(active(), INTERVAL - 1).empty());
  EXPECT_EQ(timer.get_next_posture(), Posture::Standing);

  EXPECT_EQ(tick(active()), Action::Remind);
  EXPECT_EQ(timer.get_posture(), Posture::Standing);
  EXPECT_TRUE(timer.is_reminding());
  EXPECT_EQ(timer.get_elapsed(), 0);
}

TEST_F(SitStandTimerTest, RemindersAlternate)
{
  run(active(), INTERVAL);
  EXPECT_EQ(timer.get_posture(), Posture::Standing);
  timer.dismiss();

  EXPECT_EQ(run(active(), INTERVAL), std::vector<Action>{Action::Remind});
  EXPECT_EQ(timer.get_posture(), Posture::Sitting);

  // An unanswered reminder is replaced by the next one.
  EXPECT_EQ(run(active(), INTERVAL), std::vector<Action>{Action::Remind});
  EXPECT_EQ(timer.get_posture(), Posture::Standing);
}

TEST_F(SitStandTimerTest, ShortPausesCount)
{
  run(active(), INTERVAL - 120);
  run(idle(), 119);
  EXPECT_EQ(timer.get_elapsed(), INTERVAL - 1);

  EXPECT_EQ(tick(active()), Action::Remind);
}

TEST_F(SitStandTimerTest, DueReminderWaitsForActivity)
{
  run(active(), INTERVAL - 10);
  EXPECT_TRUE(run(idle(), 60).empty());

  EXPECT_EQ(tick(active()), Action::Remind);
}

TEST_F(SitStandTimerTest, BeingAwayStartsOver)
{
  run(active(), INTERVAL);
  EXPECT_TRUE(timer.is_reminding());

  // The reminder that is showing is taken down, and the countdown starts over.
  EXPECT_EQ(run(idle(), SitStandTimer::AWAY_SECONDS), std::vector<Action>{Action::Withdraw});
  EXPECT_EQ(timer.get_elapsed(), 0);

  // The last posture is kept: a full interval later it is time to sit down.
  EXPECT_TRUE(run(active(), INTERVAL - 1).empty());
  EXPECT_EQ(tick(active()), Action::Remind);
  EXPECT_EQ(timer.get_posture(), Posture::Sitting);
}

TEST_F(SitStandTimerTest, SuspendedPausesAndCountsAsAway)
{
  SitStandTimer::Tick suspended = active();
  suspended.running = false;
  suspended.user_active = false;

  run(active(), 100);
  run(suspended, SitStandTimer::AWAY_SECONDS - 1);
  EXPECT_EQ(timer.get_elapsed(), 100);

  tick(suspended);
  EXPECT_EQ(timer.get_elapsed(), 0);
}

TEST_F(SitStandTimerTest, QuietModeHoldsTheReminderBack)
{
  SitStandTimer::Tick quiet = active();
  quiet.can_remind = false;

  EXPECT_TRUE(run(quiet, 2 * INTERVAL).empty());
  EXPECT_EQ(tick(active()), Action::Remind);

  // A break or quiet mode takes a showing reminder down.
  EXPECT_EQ(tick(quiet), Action::Withdraw);
  EXPECT_FALSE(timer.is_reminding());
}

TEST_F(SitStandTimerTest, DisablingStartsOver)
{
  SitStandTimer::Tick disabled = active();
  disabled.enabled = false;

  run(active(), INTERVAL);
  EXPECT_EQ(tick(disabled), Action::Withdraw);
  EXPECT_EQ(timer.get_posture(), Posture::Sitting);
  EXPECT_EQ(timer.get_elapsed(), 0);
  EXPECT_TRUE(run(disabled, 2 * INTERVAL).empty());
}

TEST_F(SitStandTimerTest, ShorterIntervalTakesEffectRightAway)
{
  SitStandTimer::Tick shorter = active();
  shorter.interval = 10 * 60;

  run(active(), 20 * 60);
  EXPECT_EQ(tick(shorter), Action::Remind);
}

TEST_F(SitStandTimerTest, TinyIntervalIsRaisedToTheMinimum)
{
  SitStandTimer::Tick tiny = active();
  tiny.interval = 0;

  EXPECT_TRUE(run(tiny, SitStandTimer::MIN_INTERVAL - 1).empty());
  EXPECT_EQ(tick(tiny), Action::Remind);
}

TEST_F(SitStandTimerTest, SleepCountsAsTimeAway)
{
  run(active(), INTERVAL);
  EXPECT_TRUE(timer.is_reminding());

  // The computer sleeps for an hour, and is in use as soon as it wakes up.
  clock += 60 * 60;
  EXPECT_EQ(tick(active()), Action::Withdraw);
  EXPECT_EQ(timer.get_elapsed(), 1);
}

TEST_F(SitStandTimerTest, MissedTicksStillCount)
{
  run(active(), 100);

  clock += 2;
  tick(active());
  EXPECT_EQ(timer.get_elapsed(), 103);
}

TEST_F(SitStandTimerTest, ClockSetBackCountsNothing)
{
  run(active(), 100);

  clock -= 60 * 60;
  tick(active());
  EXPECT_EQ(timer.get_elapsed(), 100);
}
