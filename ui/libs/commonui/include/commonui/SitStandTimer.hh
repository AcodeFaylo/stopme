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

#ifndef WORKRAVE_UI_SITSTANDTIMER_HH
#define WORKRAVE_UI_SITSTANDTIMER_HH

#include <cstdint>

enum class Posture
{
  Sitting,
  Standing
};

//! Decides when to remind the user to switch between sitting and standing.
/*!
 * Driven once a second by tick(). It knows nothing about windows or settings:
 * the caller says what it knows about the current second, and shows or takes
 * down the reminder when tick() asks for it.
 *
 * The user is assumed to sit when the countdown starts, so the first reminder
 * is to stand up; after that the reminders alternate. Time spent away from the
 * computer does not count: whoever is away for AWAY_SECONDS or longer has
 * moved already, so the countdown starts over when they are back. A gap
 * between ticks, such as a computer that slept, counts as time away.
 */
class SitStandTimer
{
public:
  //! What the caller knows about the current second.
  struct Tick
  {
    //! Wall-clock time in seconds. Not a monotonic clock: those stop while
    //! the computer sleeps, and a sleep has to count as time away.
    int64_t time{0};

    //! The reminder is switched on.
    bool enabled{false};

    //! Seconds from one reminder to the next.
    int64_t interval{0};

    //! Time counts: stopme is not suspended.
    bool running{true};

    //! A reminder may appear now: not in quiet mode, and no break under way.
    bool can_remind{true};

    //! There was keyboard or mouse activity in the last few seconds.
    bool user_active{false};
  };

  enum class Action
  {
    Nothing,

    //! Show a reminder to take get_posture().
    Remind,

    //! Take down the reminder that is showing.
    Withdraw
  };

  //! Time away from the computer after which the countdown starts over.
  static constexpr int64_t AWAY_SECONDS = 5 * 60;

  //! Shortest interval honoured, so a bad setting cannot remind every second.
  static constexpr int64_t MIN_INTERVAL = 60;

  Action tick(const Tick &now);

  //! The user closed the reminder.
  void dismiss();

  //! Starts over: sitting, a full interval before the next reminder.
  void reset();

  //! The posture the user was last reminded to take; sitting before that.
  Posture get_posture() const;

  //! The posture the next reminder asks for.
  Posture get_next_posture() const;

  //! Seconds counted towards the next reminder.
  int64_t get_elapsed() const;

  //! Whether a reminder is showing.
  bool is_reminding() const;

private:
  bool count(int64_t seconds, bool present, bool running);
  Action withdraw();

private:
  Posture posture{Posture::Sitting};
  int64_t elapsed{0};
  int64_t idle{0};
  int64_t last_time{0};
  bool reminding{false};
};

#endif // WORKRAVE_UI_SITSTANDTIMER_HH
