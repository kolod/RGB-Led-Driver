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

#include <QObject>
#include <QtCore>
#include <QColor>
#include <QIcon>

class ColorListModel : public QAbstractListModel
{
public:
	ColorListModel(QObject *parent = nullptr);
	~ColorListModel();

	void fromStrngList(const QStringList &colors);
	QStringList toStringList() const;

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	int rowCount(const QModelIndex &parent) const;

	QColor Color(const QModelIndex &index);
	QColor NextColor();
	QColor PreviousColor();

	void InsertColor(const QModelIndex &index, QColor color);
	void RemoveColor(const QModelIndex &index);

private:
	QList<QColor> mColor;
	QList<QIcon*> mIcon;
	int mCurrentColor;

	QIcon *CreateIcon(const QColor color);
};
