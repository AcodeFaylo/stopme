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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "QmlSitStandWindow.hh"

#include <QGuiApplication>
#include <QQmlContext>
#include <QRegion>

#include "ui/GUIConfig.hh"
#include "utils/Platform.hh"

#if defined(HAVE_WAYLAND)
#  include "IToolkitUnixPrivate.hh"
#  include "WaylandScreenPlacement.hh"
#endif
#if defined(PLATFORM_OS_MACOS)
#  include "MacOSOverlayWindow.hh"
#endif

// ── SitStandBridge ────────────────────────────────────────────────────────────

SitStandBridge::SitStandBridge(Posture posture, QObject *parent)
  : QObject(parent)
  , posture(posture)
  , classic_(!GUIConfig::sanctuary_ui_enabled()())
{
}

QString
SitStandBridge::heading() const
{
  return isStanding() ? tr("Time to stand up") : tr("Time to sit down");
}

QString
SitStandBridge::body() const
{
  return isStanding() ? tr("Work standing for a while. stopme will tell you when to sit down again.")
                      : tr("Work sitting for a while. stopme will tell you when to stand up again.");
}

QString
SitStandBridge::image() const
{
  return isStanding() ? QStringLiteral("qrc:/sanctuary/stopme-panda-standing.svg")
                      : QStringLiteral("qrc:/sanctuary/stopme-panda-sitting.svg");
}

void
SitStandBridge::dismiss()
{
  Q_EMIT dismissed();
}

// ── QmlSitStandWindow ─────────────────────────────────────────────────────────

QmlSitStandWindow::QmlSitStandWindow(std::shared_ptr<IApplicationContext> app, QScreen *screen, Posture posture)
  : screen(screen)
  , position_windows(workrave::utils::Platform::can_position_windows())
{
#if defined(HAVE_WAYLAND)
  window_manager = std::dynamic_pointer_cast<IToolkitUnixPrivate>(app->get_toolkit())->get_wayland_window_manager();
#else
  (void)app;
#endif

  bridge = new SitStandBridge(posture);
  if (!position_windows)
    {
      bridge->setFullscreen(true);
    }

  view = new QQuickView();
  view->setResizeMode(QQuickView::SizeRootObjectToView);
  // The OK button takes clicks, but keyboard input stays with the window
  // that had it before the card appeared.
  Qt::WindowFlags window_flags = Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus;
#if defined(PLATFORM_OS_MACOS)
  // Qt::Tool creates the NSPanel required for overlays in another
  // application's full-screen Space. The Cocoa helper makes it non-activating.
  window_flags |= Qt::Tool;
#else
  window_flags |= Qt::SplashScreen;
#endif
  view->setFlags(window_flags);
  view->setColor(Qt::transparent);
  view->rootContext()->setContextProperty("bridge", bridge);
  view->setSource(QUrl("qrc:/sanctuary/SitStandShell.qml"));

  QObject::connect(bridge, &SitStandBridge::dismissed, view, [this]() {
    hide();
    dismissed_signal();
  });
}

QmlSitStandWindow::~QmlSitStandWindow()
{
  hide();
  delete view;
  delete bridge;
}

QRect
QmlSitStandWindow::card_rect() const
{
  QRect geo = screen ? screen->availableGeometry() : QGuiApplication::primaryScreen()->availableGeometry();
  int x = geo.left() + (geo.width() - bridge->cardW()) / 2;
  int y = geo.top() + MARGIN;
  return {x, y, bridge->cardW(), bridge->cardH()};
}

void
QmlSitStandWindow::start()
{
  if (started)
    {
      return;
    }

#if defined(HAVE_WAYLAND)
  if (window_manager)
    {
      layer_surface = window_manager->init_surface(view, screen, false);
    }
#endif

  QRect geo = screen ? screen->availableGeometry() : QGuiApplication::primaryScreen()->availableGeometry();
  if (position_windows)
    {
      view->setGeometry(card_rect());
    }
  else
    {
      // Wayland: a transparent window covering the screen, with the card at
      // the top. Only the card takes clicks; the rest lets them through.
      view->setGeometry(geo);
      view->setMask(QRegion((geo.width() - bridge->cardW()) / 2, MARGIN, bridge->cardW(), bridge->cardH()));
    }

#if defined(PLATFORM_OS_MACOS)
  begin_macos_overlay(view);
  view->show();
  order_macos_overlay_front(view);
#else
  bool shown = false;
#  if defined(HAVE_WAYLAND)
  shown = WaylandScreenPlacement::arm(view, screen, layer_surface != nullptr);
#  endif
  if (!shown)
    {
      view->show();
    }
#endif
  started = true;
}

void
QmlSitStandWindow::stop()
{
  hide();

#if defined(HAVE_WAYLAND)
  layer_surface.reset();
#endif
}

boost::signals2::signal<void()> &
QmlSitStandWindow::signal_dismissed()
{
  return dismissed_signal;
}

void
QmlSitStandWindow::hide()
{
  if (!started)
    {
      return;
    }

  view->hide();
#if defined(PLATFORM_OS_MACOS)
  end_macos_overlay(view);
#endif
  started = false;
}
