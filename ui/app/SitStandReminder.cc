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

#include "SitStandReminder.hh"

#include <algorithm>
#include <utility>

#include <spdlog/spdlog.h>

#include "core/ICore.hh"
#include "ui/GUIConfig.hh"

using namespace workrave;

SitStandReminder::SitStandReminder(std::shared_ptr<IApplicationContext> context)
  : context(std::move(context))
{
}

SitStandReminder::~SitStandReminder()
{
  hide();
}

void
SitStandReminder::heartbeat()
{
  auto core = context->get_core();
  OperationMode mode = core->get_active_operation_mode();

  SitStandTimer::Tick now;
  now.enabled = GUIConfig::sit_stand_enabled()();
  now.interval = GUIConfig::sit_stand_interval()();
  now.running = mode != OperationMode::Suspended;
  now.can_remind = mode == OperationMode::Normal && !is_break_active();
  now.user_active = core->is_user_active();

  switch (timer.tick(now))
    {
    case SitStandTimer::Action::Remind:
      show(timer.get_posture());
      break;

    case SitStandTimer::Action::Withdraw:
      hide();
      break;

    case SitStandTimer::Action::Nothing:
      // Windows closed by the user are released here, outside their own
      // event handlers.
      if (!timer.is_reminding() && !windows.empty())
        {
          hide();
        }
      break;
    }
}

bool
SitStandReminder::is_enabled() const
{
  return GUIConfig::sit_stand_enabled()();
}

Posture
SitStandReminder::get_next_posture() const
{
  return timer.get_next_posture();
}

int64_t
SitStandReminder::get_remaining() const
{
  int64_t interval = std::max<int64_t>(GUIConfig::sit_stand_interval()(), SitStandTimer::MIN_INTERVAL);
  return std::max<int64_t>(interval - timer.get_elapsed(), 0);
}

void
SitStandReminder::show(Posture posture)
{
  hide();

  spdlog::info("Reminding to {}", posture == Posture::Standing ? "stand up" : "sit down");

  auto toolkit = context->get_toolkit();
  for (int i = 0; i < toolkit->get_head_count(); i++)
    {
      auto window = toolkit->create_sit_stand_window(i, posture);
      if (window)
        {
          connections.emplace_back(window->signal_dismissed().connect([this]() { on_dismissed(); }));
          windows.push_back(window);
        }
    }

  for (auto &window: windows)
    {
      window->start();
    }
}

void
SitStandReminder::hide()
{
  for (auto &window: windows)
    {
      window->stop();
    }
  connections.clear();
  windows.clear();
}

void
SitStandReminder::on_dismissed()
{
  timer.dismiss();

  // Only hide them: this runs inside the handler of one of these windows.
  for (auto &window: windows)
    {
      window->stop();
    }
}

bool
SitStandReminder::is_break_active() const
{
  auto core = context->get_core();
  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      auto b = core->get_break(BreakId(i));
      if (b != nullptr && b->is_active())
        {
          return true;
        }
    }
  return false;
}
