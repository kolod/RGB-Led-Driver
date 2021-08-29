//    RGB Driver
//    Copyright (C) 2015-2021  Alexandr Kolodkin <alexandr.kolodkin@gmail.com>
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <QMainWindow>
#include <QtCore>
#include <QtWidgets>
#include <QSerialPort>
#include "qglobalshortcut.h"
#include "colorlistmodel.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	void saveSession();
	void restoreSession();

private slots:
	void enumeratePorts();
	void nextPreset();
	void previousPreset();
	void activate();

private:
	bool mClose;
	Ui::MainWindow *ui;
	QSerialPort mPort;
	QSystemTrayIcon *mTrayIcon;
	QMenu *mTrayIconMenu;
	ColorListModel *mColors;
	QPoint mPosition;
	QGlobalShortcut mGlobalShortcut;

	void closeEvent(QCloseEvent *event);
	QByteArray BuildCommand(QColor color);

	void SetVirtualKeycodes(QComboBox *combo);
};
