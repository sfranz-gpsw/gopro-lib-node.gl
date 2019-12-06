#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2018 GoPro Inc.
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#

from fractions import Fraction

from PySide2 import QtCore, QtWidgets, QtGui
from PySide2.QtCore import Qt
from PySide2.QtCore import QEvent

from .seekbar import Seekbar

from pynodegl_utils import player
from pynodegl_utils import export

MIN_RESIZE_INTERVAL = 100


class _PlayerWidget(QtWidgets.QWidget):

    onPlayerAvailable = QtCore.Signal()

    def __init__(self, parent, config):
        super(_PlayerWidget, self).__init__(parent)

        self.setAttribute(Qt.WA_DontCreateNativeAncestors)
        self.setAttribute(Qt.WA_NativeWindow)
        self.setAttribute(Qt.WA_PaintOnScreen)

        self._player = None
        self._last_frame_time = 0.0
        self._config = config

        self._req_width = None
        self._req_height = None

        self._timer = QtCore.QTimer()
        self._timer.setSingleShot(True)
        self._timer.setInterval(MIN_RESIZE_INTERVAL)
        self._timer.timeout.connect(self._resize)

    def paintEngine(self):
        return None

    @QtCore.Slot()
    def _resize(self):
        assert self._req_width is not None
        assert self._req_height is not None
        width = int(self._req_width * self.devicePixelRatioF())
        height = int(self._req_height * self.devicePixelRatioF())
        self._player.resize(width, height)
        self._player.draw()

    def resizeEvent(self, event):
        if not self._player:
            return

        size = event.size()
        self._req_width = size.width()
        self._req_height = size.height()
        self._timer.start()

        super(_PlayerWidget, self).resizeEvent(event)

    def event(self, event):
        if event.type() == QEvent.Paint:
            if not self._player:
                self._player = player.Player(
                    self.winId(),
                    self.width() * self.devicePixelRatioF(),
                    self.height() * self.devicePixelRatioF(),
                    self._config,
                )
                self._player.start()
                self._player.onFrame.connect(self._set_last_frame_time)
                self.onPlayerAvailable.emit()
            else:
                self._player.draw()
        elif event.type() == QEvent.Close:
            if self._player:
                self._player.stop()
                self._player.wait()
        return super(_PlayerWidget, self).event(event)

    @QtCore.Slot(int, float)
    def _set_last_frame_time(self, frame_index, frame_time):
        self._last_frame_time = frame_time

    def get_last_frame_time(self):
        return self._last_frame_time

    def get_player(self):
        return self._player


class ConstrainedARWidget(QtWidgets.QWidget):

    def __init__(self, parent, config):
        super(ConstrainedARWidget, self).__init__(parent)

        size_policy = QtWidgets.QSizePolicy()
        size_policy.setVerticalPolicy(QtWidgets.QSizePolicy.MinimumExpanding)
        size_policy.setHorizontalPolicy(QtWidgets.QSizePolicy.MinimumExpanding)
        self.setSizePolicy(size_policy)

        self._aspect_ratio = Fraction(1, 1)
        self._player_widget = _PlayerWidget(self, config)

        self.onPlayerAvailable = self._player_widget.onPlayerAvailable
        self.get_last_frame_time = self._player_widget.get_last_frame_time
        self.get_player = self._player_widget.get_player

    def resizeEvent(self, event):
        size = event.size()
        ar = Fraction(size.width(), size.height())
        print self._aspect_ratio
        if ar <= self._aspect_ratio:
            width = size.width()
            height = int(round(width / self._aspect_ratio))
        else:
            height = size.height()
            width = int(round(height * self._aspect_ratio))
        self._player_widget.setGeometry((size.width() - width) / 2, (size.height() - height) / 2, width, height)

    def event(self, event):
        if event.type() == QtCore.QEvent.Paint:
            print 'paint'
            self.resizeEvent(QtGui.QResizeEvent(self.size(), self.size()))
        return super(ConstrainedARWidget, self).event(event)

    def set_aspect_ratio(self, value):
        self._aspect_ratio = Fraction(*value)
        #self.update()

    def closeEvent(self, event):
        self._player_widget.close()
        super(ConstrainedARWidget, self).closeEvent(event)


class PlayerView(QtWidgets.QWidget):

    def __init__(self, get_scene_func, config):
        super(PlayerView, self).__init__()

        self._get_scene_func = get_scene_func
        self._cfg = None

        self._seekbar = Seekbar(config)
        self._player_widget = ConstrainedARWidget(self, config)
        self._player_widget.onPlayerAvailable.connect(self._connect_seekbar)

        screenshot_btn = QtWidgets.QToolButton()
        screenshot_btn.setText(u'📷')

        toolbar = QtWidgets.QHBoxLayout()
        toolbar.addWidget(self._seekbar)
        toolbar.addWidget(screenshot_btn)

        self._gl_layout = QtWidgets.QVBoxLayout(self)
        self._gl_layout.addWidget(self._player_widget, strech=1)
        self._gl_layout.addLayout(toolbar)

        screenshot_btn.clicked.connect(self._screenshot)

    @QtCore.Slot()
    def _connect_seekbar(self):
        player = self._player_widget.get_player()
        player.onPlay.connect(self._seekbar.set_play_state)
        player.onPause.connect(self._seekbar.set_pause_state)
        player.onSceneMetadata.connect(self._seekbar.set_scene_metadata)
        player.onFrame.connect(self._seekbar.set_frame_time)

        self._seekbar.seek.connect(player.seek)
        self._seekbar.play.connect(player.play)
        self._seekbar.pause.connect(player.pause)
        self._seekbar.step.connect(player.step)
        self._seekbar.stop.connect(player.reset_scene)

        if self._cfg:
            self._player_widget.set_aspect_ratio(self._cfg['aspect_ratio'])
            player.set_scene(self._cfg)

    @QtCore.Slot()
    def _screenshot(self):
        filenames = QtWidgets.QFileDialog.getSaveFileName(self, 'Save screenshot file')
        if not filenames[0]:
            return
        exporter = export.Exporter(
            self._get_scene_func,
            filenames[0],
            self._player_widget.width(),
            self._player_widget.height(),
            ['-frames:v', '1'],
            self._player_widget.get_last_frame_time()
        )
        exporter.start()
        exporter.wait()

    def enter(self):
        self._cfg = self._get_scene_func()
        if not self._cfg:
            return
        self._player_widget.set_aspect_ratio(self._cfg['aspect_ratio'])

        player = self._player_widget.get_player()
        if not player:
            return
        player.set_scene(self._cfg)


    def leave(self):
        player = self._player_widget.get_player()
        if not player:
            return
        player.pause()

    def closeEvent(self, close_event):
        self._player_widget.close()
        self._seekbar.close()
        super(PlayerView, self).closeEvent(close_event)
