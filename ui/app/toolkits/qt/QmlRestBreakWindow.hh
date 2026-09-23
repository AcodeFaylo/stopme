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

#ifndef QMLRESTBREAKWINDOW_HH
#define QMLRESTBREAKWINDOW_HH

#include <functional>
#include <memory>

#include <QObject>
#include <QScreen>
#include <QQuickView>
#include <QString>
#include <QTimer>

#include "ui/IBreakWindow.hh"
#include "ui/GUIConfig.hh"
#include "ui/UiTypes.hh"
#include "ui/IApplicationContext.hh"
#include "core/CoreTypes.hh"
#include "utils/Signals.hh"

#if defined(HAVE_WAYLAND)
#  include "WaylandWindowManager.hh"
#endif

// Data bridge exposed to the QML scene as the "bridge" context property.
class RestBreakBridge
  : public QObject
  , public workrave::utils::Trackable
{
  Q_OBJECT

  // CONSTANT properties
  Q_PROPERTY(int blockMode READ blockMode CONSTANT)
  Q_PROPERTY(bool lockable READ lockable CONSTANT)
  Q_PROPERTY(bool shutdownable READ shutdownable CONSTANT)
  Q_PROPERTY(bool sleepable READ sleepable CONSTANT)
  Q_PROPERTY(bool isNatural READ isNatural CONSTANT)

  // Notified via lockStateChanged
  Q_PROPERTY(bool canPostpone READ canPostpone NOTIFY lockStateChanged)
  Q_PROPERTY(bool canSkip READ canSkip NOTIFY lockStateChanged)
  Q_PROPERTY(double lockProgress READ lockProgress NOTIFY lockStateChanged)
  Q_PROPERTY(bool isLocked READ isLocked NOTIFY lockStateChanged)

  // Notified via breakProgressChanged
  Q_PROPERTY(double breakProgress READ breakProgress NOTIFY breakProgressChanged)
  Q_PROPERTY(QString breakTime READ breakTime NOTIFY breakProgressChanged)
  Q_PROPERTY(QString breakTimeShort READ breakTimeShort NOTIFY breakProgressChanged)
  Q_PROPERTY(QString breakMaxStr READ breakMaxStr NOTIFY breakProgressChanged)

  // Notified via userActivityChanged
  Q_PROPERTY(bool userActive READ userActive NOTIFY userActivityChanged)

  // Notified via classicChanged
  Q_PROPERTY(bool classic READ isClassic NOTIFY classicChanged)

public:
  explicit RestBreakBridge(std::shared_ptr<IApplicationContext> app,
                           BlockMode block_mode,
                           BreakFlags break_flags,
                           QObject *parent = nullptr);

  // CONSTANT accessors
  int blockMode() const;
  bool lockable() const;
  bool shutdownable() const;
  bool sleepable() const;
  bool isNatural() const;

  // Lock state
  bool canPostpone() const;
  bool canSkip() const;
  double lockProgress() const;
  bool isLocked() const;

  // Break total time
  QString breakTimeShort() const;
  QString breakMaxStr() const;

  // Break progress
  double breakProgress() const;
  QString breakTime() const;

  // User activity
  bool userActive() const
  {
    return user_active_;
  }
  bool isClassic() const
  {
    return classic_;
  }

  // Called from QmlRestBreakWindow
  void setProgress(int value, int max_value);
  void setBreakButtonState(const BreakButtonState &state);
  void setDismissHandler(std::function<void()> fn)
  {
    on_dismiss_ = std::move(fn);
  }
  void updateUserActivity();

Q_SIGNALS:
  void lockStateChanged();
  void breakProgressChanged();
  void userActivityChanged();
  void classicChanged();

public Q_SLOTS:
  void requestPostpone();
  void requestSkip();
  void requestLock();
  void requestShutdown();
  void requestSleep();

private:
  std::shared_ptr<IApplicationContext> app;
  BlockMode block_mode;
  BreakFlags break_flags;

  std::function<void()> on_dismiss_;

  int break_value{0};
  int break_max{1};
  bool postpone_locked{false};
  bool skip_locked{false};
  double lock_progress_val{0.0};

  bool user_active_{false};
  bool classic_{false};
};

// IBreakWindow implementation that hosts a QQuickView with RestBreakOverlay.qml.
class QmlRestBreakWindow : public IBreakWindow
{
public:
  QmlRestBreakWindow(std::shared_ptr<IApplicationContext> app, QScreen *screen, BreakFlags break_flags);
  ~QmlRestBreakWindow() override;

  void init() override;
  void start() override;
  void stop() override;
  void refresh() override;
  void set_progress(int value, int max_value) override;
  void set_break_button_state(const BreakButtonState &state) override;

private:
  void configure_view_for_block_mode();
  void refresh_topmost_state();

  std::shared_ptr<IApplicationContext> app;
  QScreen *screen;
  BreakFlags break_flags;
  BlockMode block_mode;

  QQuickView *view{nullptr};
  RestBreakBridge *bridge{nullptr};
  std::shared_ptr<bool> alive_{std::make_shared<bool>(true)};
  bool topmost_enabled_{true};
  QTimer *topmost_timer_{nullptr};

#if defined(HAVE_WAYLAND)
  std::shared_ptr<WaylandWindowManager> window_manager;
  std::shared_ptr<WaylandLayerSurface> layer_surface;
#endif
};

#endif // QMLRESTBREAKWINDOW_HH
