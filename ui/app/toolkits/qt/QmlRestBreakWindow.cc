// Copyright (C) 2024 Rob Caelers <robc@krandor.nl>
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "QmlRestBreakWindow.hh"

#include <QQmlContext>
#include <QScreen>
#include <QGuiApplication>
#include <QTimer>
#include <QUrl>

#include "core/ICore.hh"
#include "core/IBreak.hh"
#include "core/CoreTypes.hh"
#include "session/System.hh"
#include "debug.hh"
#include "UiUtil.hh"
#include <fmt/format.h>

#if defined(HAVE_WAYLAND)
#  include "IToolkitUnixPrivate.hh"
#  include "WaylandScreenPlacement.hh"
#endif

#if defined(PLATFORM_OS_WINDOWS)
#  include "TaskManagerWatcher.hh"
#  include "ui/windows/WindowsCompat.hh"
#endif

#if defined(PLATFORM_OS_MACOS)
#  include "MacOSOverlayWindow.hh"
#endif

using namespace workrave;
using namespace workrave::utils;

// ── RestBreakBridge ───────────────────────────────────────────────────────────

RestBreakBridge::RestBreakBridge(std::shared_ptr<IApplicationContext> app,
                                 BlockMode block_mode,
                                 BreakFlags break_flags,
                                 QObject *parent)
  : QObject(parent)
  , app(std::move(app))
  , block_mode(block_mode)
  , break_flags(break_flags)
  , classic_(!GUIConfig::sanctuary_ui_enabled()())
{
  GUIConfig::sanctuary_ui_enabled().connect(this, [this](bool enabled) {
    bool new_classic = !enabled;
    if (new_classic != classic_)
      {
        classic_ = new_classic;
        Q_EMIT classicChanged();
      }
  });
}

int
RestBreakBridge::blockMode() const
{
  return static_cast<int>(block_mode);
}

bool
RestBreakBridge::lockable() const
{
  return System::is_lockable();
}

bool
RestBreakBridge::shutdownable() const
{
  return GUIConfig::break_enable_shutdown(BREAK_ID_REST_BREAK)() && System::is_shutdownable();
}

bool
RestBreakBridge::sleepable() const
{
  return GUIConfig::break_enable_shutdown(BREAK_ID_REST_BREAK)() && System::is_sleepable();
}

bool
RestBreakBridge::isNatural() const
{
  return (break_flags & BREAK_FLAGS_NATURAL) != 0;
}

bool
RestBreakBridge::canPostpone() const
{
  return (break_flags & BREAK_FLAGS_POSTPONABLE) != 0 && !postpone_locked;
}

bool
RestBreakBridge::canSkip() const
{
  return (break_flags & BREAK_FLAGS_SKIPPABLE) != 0 && !skip_locked;
}

double
RestBreakBridge::lockProgress() const
{
  return lock_progress_val;
}

bool
RestBreakBridge::isLocked() const
{
  return postpone_locked || skip_locked;
}

double
RestBreakBridge::breakProgress() const
{
  if (break_max <= 0)
    {
      return 1.0;
    }
  double remaining = static_cast<double>(break_max - break_value) / break_max;
  return qBound(0.0, remaining, 1.0);
}

QString
RestBreakBridge::breakTime() const
{
  time_t t = static_cast<time_t>(std::max(0, break_max - break_value));
  return QString::fromStdString(
    fmt::format(fmt::runtime(tr("Rest break for {}").toStdString()), UiUtil::time_to_string(t).toStdString()));
}

QString
RestBreakBridge::breakTimeShort() const
{
  time_t t = static_cast<time_t>(std::max(0, break_max - break_value));
  return UiUtil::time_to_string(t);
}

QString
RestBreakBridge::breakMaxStr() const
{
  return UiUtil::time_to_string(static_cast<time_t>(break_max));
}

void
RestBreakBridge::setProgress(int value, int max_value)
{
  break_value = value;
  break_max = max_value;
  Q_EMIT breakProgressChanged();
}

void
RestBreakBridge::setBreakButtonState(const BreakButtonState &state)
{
  bool changed = (state.can_postpone == postpone_locked) || (state.can_skip == skip_locked)
                 || (state.lock_progress() != lock_progress_val);
  postpone_locked = !state.can_postpone;
  skip_locked = !state.can_skip;
  lock_progress_val = state.lock_progress();
  if (changed)
    {
      Q_EMIT lockStateChanged();
    }
}

void
RestBreakBridge::updateUserActivity()
{
  bool active = app->get_core()->is_user_active();
  if (active != user_active_)
    {
      user_active_ = active;
      Q_EMIT userActivityChanged();
    }
}

void
RestBreakBridge::requestPostpone()
{
  QTimer::singleShot(0, this, [this]() {
    app->get_core()->get_break(BREAK_ID_REST_BREAK)->postpone_break();
    if (on_dismiss_)
      on_dismiss_();
  });
}

void
RestBreakBridge::requestSkip()
{
  QTimer::singleShot(0, this, [this]() {
    app->get_core()->get_break(BREAK_ID_REST_BREAK)->skip_break();
    if (on_dismiss_)
      on_dismiss_();
  });
}

void
RestBreakBridge::requestLock()
{
  if (System::is_lockable())
    {
      auto locker = app->get_toolkit()->get_locker();
      locker->unlock();
      locker->lock();
      System::lock_screen_by_id(GUIConfig::preferred_lock_method()());
    }
}

void
RestBreakBridge::requestShutdown()
{
  System::execute(System::SystemOperation::SYSTEM_OPERATION_SHUTDOWN);
}

void
RestBreakBridge::requestSleep()
{
  const std::string &pref = GUIConfig::preferred_sleep_operation()();
  System::SystemOperation::SystemOperationType type = System::SystemOperation::SYSTEM_OPERATION_SUSPEND;
  if (pref == "hibernate")
    {
      type = System::SystemOperation::SYSTEM_OPERATION_HIBERNATE;
    }
  else if (pref == "suspend_hybrid")
    {
      type = System::SystemOperation::SYSTEM_OPERATION_SUSPEND_HYBRID;
    }
  if (!System::execute(type))
    {
      for (const auto &op: System::get_sleep_operations())
        {
          if (System::execute(op.type))
            break;
        }
    }
}

// ── QmlRestBreakWindow ────────────────────────────────────────────────────────

QmlRestBreakWindow::QmlRestBreakWindow(std::shared_ptr<IApplicationContext> app, QScreen *screen, BreakFlags break_flags)
  : app(app)
  , screen(screen)
  , break_flags(break_flags)
  , block_mode(GUIConfig::block_mode()())
{
#if defined(HAVE_WAYLAND)
  window_manager = std::dynamic_pointer_cast<IToolkitUnixPrivate>(app->get_toolkit())->get_wayland_window_manager();
#endif
}

QmlRestBreakWindow::~QmlRestBreakWindow()
{
  *alive_ = false;
  delete topmost_timer_;
#if defined(PLATFORM_OS_MACOS)
  if (view != nullptr)
    {
      end_macos_overlay(view);
    }
#endif
  delete view;
}

void
QmlRestBreakWindow::init()
{
  TRACE_ENTRY();

  bridge = new RestBreakBridge(app, block_mode, break_flags);
  bridge->setDismissHandler([this, alive = alive_]() {
    if (*alive)
      stop();
  });
  // User activity halts the break until the user rests again.
  app->get_core()->set_insist_policy(InsistPolicy::Halt);

  topmost_timer_ = new QTimer();
  topmost_timer_->setInterval(100);
  QObject::connect(topmost_timer_, &QTimer::timeout, [this]() { refresh_topmost_state(); });

  view = new QQuickView();
  view->setResizeMode(QQuickView::SizeRootObjectToView);
  view->rootContext()->setContextProperty("bridge", bridge);

  QObject::connect(view, &QQuickView::statusChanged, view, [this](QQuickView::Status status) {
    if (status == QQuickView::Error)
      {
        for (const auto &err: view->errors())
          {
            spdlog::error("RestBreakOverlay QML error: {}", err.toString().toStdString());
          }
      }
  });

  view->setSource(QUrl("qrc:/sanctuary/RestBreakShell.qml"));

  configure_view_for_block_mode();
}

void
QmlRestBreakWindow::configure_view_for_block_mode()
{
  Qt::WindowFlags window_flags = Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus;
#if defined(PLATFORM_OS_MACOS)
  window_flags |= Qt::Tool;
#else
  window_flags |= Qt::SplashScreen;
#endif
  view->setFlags(window_flags);
  view->setColor(Qt::transparent);
}

void
QmlRestBreakWindow::start()
{
  TRACE_ENTRY();

  if (screen != nullptr)
    {
      view->setScreen(screen);
    }

#if defined(HAVE_WAYLAND)
  if (window_manager)
    layer_surface = window_manager->init_surface(view, screen, true);
#endif

#if defined(PLATFORM_OS_MACOS)
  begin_macos_overlay(view);
  QRect geo = (screen != nullptr) ? screen->geometry() : QGuiApplication::primaryScreen()->geometry();
  view->setGeometry(geo);
  view->show();
  order_macos_overlay_front(view);
#else
  if (block_mode == BlockMode::All)
    {
      view->showFullScreen();
    }
  else
    {
      // Input and Off: full-screen transparent view; QML draws a centered floating card.
      QRect geo = (screen != nullptr) ? screen->geometry() : QGuiApplication::primaryScreen()->geometry();
      view->setGeometry(geo);

      bool shown = false;
#if defined(HAVE_WAYLAND)
      shown = WaylandScreenPlacement::arm(view, screen, layer_surface != nullptr);
#endif
      if (!shown)
        {
          view->show();
        }
    }

  view->raise();
#endif
  refresh_topmost_state();
  if (topmost_timer_ != nullptr)
    {
      topmost_timer_->start();
    }
}

void
QmlRestBreakWindow::stop()
{
  TRACE_ENTRY();
  if (topmost_timer_ != nullptr)
    {
      topmost_timer_->stop();
    }
  view->hide();
#if defined(PLATFORM_OS_MACOS)
  end_macos_overlay(view);
#endif

#if defined(HAVE_WAYLAND)
  layer_surface.reset();
#endif
}

void
QmlRestBreakWindow::refresh()
{
  refresh_topmost_state();
  bridge->updateUserActivity();
}

void
QmlRestBreakWindow::refresh_topmost_state()
{
#if defined(PLATFORM_OS_WINDOWS)
  if (view == nullptr)
    {
      return;
    }

  bool should_be_topmost = !TaskManagerWatcher::is_running();
  if (topmost_enabled_ != should_be_topmost)
    {
      WindowsCompat::SetWindowOnTop(reinterpret_cast<HWND>(view->winId()), should_be_topmost ? TRUE : FALSE);
      topmost_enabled_ = should_be_topmost;
    }
#else
  (void)topmost_enabled_;
#endif
}

void
QmlRestBreakWindow::set_progress(int value, int max_value)
{
  bridge->setProgress(value, max_value);
}

void
QmlRestBreakWindow::set_break_button_state(const BreakButtonState &state)
{
  bridge->setBreakButtonState(state);
}
