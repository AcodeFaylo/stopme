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

#ifndef QMLSITSTANDWINDOW_HH
#define QMLSITSTANDWINDOW_HH

#include <memory>

#include <QObject>
#include <QQuickView>
#include <QScreen>
#include <QString>

#include "ui/IApplicationContext.hh"
#include "ui/ISitStandWindow.hh"

#if defined(HAVE_WAYLAND)
#  include "WaylandWindowManager.hh"
#endif

// Data bridge exposed to SitStandShell.qml / SitStandOverlay.qml / SitStandClassic.qml.
class SitStandBridge : public QObject
{
  Q_OBJECT

  Q_PROPERTY(bool    standing      READ isStanding    CONSTANT)
  Q_PROPERTY(QString heading       READ heading       CONSTANT)
  Q_PROPERTY(QString body          READ body          CONSTANT)
  Q_PROPERTY(QString image         READ image         CONSTANT)
  Q_PROPERTY(bool    fullscreen    READ isFullscreen  NOTIFY layoutChanged)
  // classic == true  → SitStandClassic.qml (Gtk look)
  // classic == false → SitStandOverlay.qml (Sanctuary)
  Q_PROPERTY(bool    classic       READ isClassic     CONSTANT)
  Q_PROPERTY(int     cardW         READ cardW         CONSTANT)
  Q_PROPERTY(int     cardH         READ cardH         CONSTANT)

public:
  explicit SitStandBridge(Posture posture, QObject *parent = nullptr);

  bool    isStanding()   const { return posture == Posture::Standing; }
  QString heading()      const;
  QString body()         const;
  QString image()        const;
  bool    isFullscreen() const { return fullscreen_; }
  bool    isClassic()    const { return classic_; }
  int     cardW()        const { return classic_ ? 420 : 480; }
  int     cardH()        const { return classic_ ? 132 : 148; }

  void setFullscreen(bool v) { fullscreen_ = v; Q_EMIT layoutChanged(); }

  Q_INVOKABLE void dismiss();

Q_SIGNALS:
  void dismissed();
  void layoutChanged();

private:
  Posture posture;
  bool fullscreen_{false};
  bool classic_{false};
};

// ISitStandWindow hosting SitStandShell.qml in a QQuickView at the top of a
// screen, placed like the break warning. It takes clicks but never focus.
class QmlSitStandWindow : public ISitStandWindow
{
public:
  QmlSitStandWindow(std::shared_ptr<IApplicationContext> app, QScreen *screen, Posture posture);
  ~QmlSitStandWindow() override;

  void start() override;
  void stop() override;
  boost::signals2::signal<void()> &signal_dismissed() override;

private:
  static constexpr int MARGIN = 20;

  QRect card_rect() const;
  void  hide();

  QScreen        *screen{nullptr};
  QQuickView     *view{nullptr};
  SitStandBridge *bridge{nullptr};

  bool position_windows{true};
  bool started{false};

  boost::signals2::signal<void()> dismissed_signal;

#if defined(HAVE_WAYLAND)
  std::shared_ptr<WaylandWindowManager> window_manager;
  std::shared_ptr<WaylandLayerSurface> layer_surface;
#endif
};

#endif // QMLSITSTANDWINDOW_HH
