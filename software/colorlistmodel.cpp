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

#include "colorlistmodel.h"
#include <QSvgRenderer>
#include <QPainter>

ColorListModel::ColorListModel(QObject *parent)
	: QAbstractListModel(parent)
{
	mCurrentColor = 0;
}

ColorListModel::~ColorListModel()
{
	qDeleteAll(mIcon);
}

void ColorListModel::fromStrngList(const QStringList &colors)
{
	mColor.clear();
	qDeleteAll(mIcon);

	mColor.reserve(colors.count());
	mIcon.reserve(colors.count());

	foreach (QString color, colors) {
		mColor.append(QColor(color));
		mIcon.append(CreateIcon(mColor.last()));
	}
}

QStringList ColorListModel::toStringList() const
{
	QStringList temp;
	foreach (auto color, mColor) temp.append(color.name());
	return QStringList(temp);
}

QIcon *ColorListModel::CreateIcon(const QColor color)
{
	QFile svgfile(":/icons/color.svg");

	if (svgfile.open(QIODevice::ReadOnly)) {
		QByteArray rawsvg = svgfile.readAll();
		svgfile.close();

		rawsvg.replace("$color", color.name().toLatin1());

		// create svg renderer with edited contents
		QSvgRenderer svgRenderer(rawsvg);

		// create pixmap target (could be a QImage)
		QPixmap pix(svgRenderer.defaultSize());
		pix.fill(Qt::transparent);

		// create painter to act over pixmap
		QPainter pixPainter(&pix);

		// use renderer to render over painter which paints on pixmap
		svgRenderer.render(&pixPainter);
		return new QIcon(pix);
	}

	return nullptr;
}


QVariant ColorListModel::data(const QModelIndex &index, int role) const
{
	if ((index.row() <0) || (index.row() >= mColor.count())) return QVariant();

	switch (role) {
	case Qt::DisplayRole:
	case Qt::EditRole:
		return mColor.at(index.row()).name();

	case Qt::DecorationRole:
		return QVariant::fromValue(*(mIcon.at(index.row())));

	default:
		return QVariant();
	}
}

int ColorListModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return mColor.count();
}


QColor ColorListModel::NextColor()
{
	if (++mCurrentColor >= mColor.count()) mCurrentColor = 0;
	return mColor.at(mCurrentColor);
}

QColor ColorListModel::PreviousColor()
{
	if (--mCurrentColor < 0) mCurrentColor = mColor.count() - 1;
	return mColor.at(mCurrentColor);
}

QColor ColorListModel::Color(const QModelIndex &index)
{
	if (index.isValid()) {
		mCurrentColor = index.row();
		return mColor.at(mCurrentColor);
	}
	return QColor();
}

void ColorListModel::InsertColor(const QModelIndex &index, QColor color)
{
	if (index.isValid()) {
		beginInsertRows(index.parent(), index.row(), index.row() + 1);
		mColor.insert(index.row(), color);
		mIcon.insert(index.row(), CreateIcon(color));
		endInsertRows();
	} else {
		beginInsertRows(QModelIndex(), mColor.count() - 1, mColor.count());
		mColor.append(color);
		mIcon.append(CreateIcon(color));
		endInsertRows();
	}
}

void ColorListModel::RemoveColor(const QModelIndex &index)
{
	if (index.isValid()) {
		beginRemoveRows(index.parent(), index.row(), index.row());
		mColor.removeAt(index.row());
		delete mIcon.at(index.row());
		mIcon.removeAt(index.row());
		endRemoveRows();
	}
}
