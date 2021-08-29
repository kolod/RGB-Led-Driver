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

#include <QDebug>
#include <QSerialPortInfo>
#include <QtWidgets>
#include <QKeySequence>
#include "mainwindow.h"
#include "ui_mainwindow.h"

QString getLastErrorMsg() {
	wchar_t buffer[4096];
	auto err = GetLastError();
	::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, 0, err, 0, buffer, 4096, 0);
	return QString::fromUtf16((const char16_t*) buffer).trimmed();
}

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow),
	mClose(false)
{
	ui->setupUi(this);

	enumeratePorts();

	setWindowFlags(windowFlags() & ~(Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint));

	mColors = new ColorListModel(this);

	ui->listView->setModel(mColors);

	auto actionNext = new QAction(tr("&Next"), this);
	connect(actionNext, &QAction::triggered, this, &MainWindow::nextPreset);

	auto actionPrevious = new QAction(tr("&Previous"), this);
	connect(actionNext, &QAction::triggered, this, &MainWindow::previousPreset);

	auto actionQuit = new QAction(tr("&Quit"), this);
	connect(actionQuit, &QAction::triggered, this, [this] () {
		mClose = true;
		this->close();
	});

	mTrayIconMenu = new QMenu(this);
	mTrayIconMenu->addAction(actionNext);
	mTrayIconMenu->addAction(actionPrevious);
	mTrayIconMenu->addSeparator();
	mTrayIconMenu->addAction(actionQuit);

	mTrayIcon = new QSystemTrayIcon(this);
	mTrayIcon->setContextMenu(mTrayIconMenu);
	mTrayIcon->setIcon(QIcon(":/icons/bulb.svg"));
	mTrayIcon->show();

	connect(mTrayIcon, &QSystemTrayIcon::activated, this, [this] (QSystemTrayIcon::ActivationReason reason) {
		if ((reason == QSystemTrayIcon::Trigger) || (reason == QSystemTrayIcon::DoubleClick)) {
			this->show();
		}
	});

	connect(ui->inputAdd, &QPushButton::clicked, this, [this] (){
		auto dialog = new QColorDialog(this);

		connect(dialog, &QColorDialog::currentColorChanged, this, [this] (const QColor &color) {
			if (mPort.isOpen()) {
				mPort.write(BuildCommand(color));
			}
		});

		if (dialog->exec()) {
			mColors->InsertColor(ui->listView->currentIndex(), dialog->selectedColor());
		}
	});

	connect(ui->inputRemove, &QPushButton::clicked, this, [this] (){
		mColors->RemoveColor(ui->listView->currentIndex());
	});

	connect(ui->inputUpdate  , &QPushButton::clicked, this, &MainWindow::enumeratePorts);
	connect(ui->inputNext    , &QPushButton::clicked, this, &MainWindow::nextPreset);
	connect(ui->inputPrevious, &QPushButton::clicked, this, &MainWindow::previousPreset);
	connect(ui->inputSave    , &QPushButton::clicked, this, &MainWindow::saveSession);
	connect(ui->inputActivate, &QPushButton::clicked, this, &MainWindow::activate);

	connect(ui->listView, &QListView::clicked, this, [this] (){
		if (mPort.isOpen()) {
			mPort.write(BuildCommand(mColors->Color(ui->listView->currentIndex())));
		}
	});

	connect(ui->inputAutostart, &QCheckBox::clicked, this, [this] () {

		auto shortcutPath = QString("\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\%1.lnk")
			.arg(qApp->applicationName());

		auto paths = QStandardPaths::standardLocations(QStandardPaths::GenericConfigLocation);
		foreach (auto path, paths) {
			if (path.contains("ProgramData")) {
				shortcutPath.prepend(path.replace('/', QDir::separator()));

				qDebug() << shortcutPath;

				if (ui->inputAutostart->isChecked()) {
					//TODO: Enable autostart
				} else {
					//TODO: Disable autostart
				}
				return;
			}
		}
	});
}

MainWindow::~MainWindow()
{
	delete ui;
	delete mTrayIcon;
	delete mTrayIconMenu;

	mGlobalShortcut.unsetShortcut();
}

void MainWindow::saveSession()
{
	QSettings settings;

	settings.setValue("geometry"     , saveGeometry());
	settings.setValue("state"        , saveState());
	settings.setValue("port"         , ui->inputPort->currentText());
	settings.setValue("colors"       , mColors->toStringList());

	settings.setValue("key"          , ui->inputKey->keySequence().toString());
	settings.setValue("red"          , ui->inputRed->value());
	settings.setValue("green"        , ui->inputGreen->value());
	settings.setValue("blue"         , ui->inputBlue->value());

	settings.setValue("autostart"    , ui->inputAutostart->isChecked());
	settings.setValue("hidden"       , ui->inputStartHidden->isChecked());
	settings.setValue("activate"     , ui->inputActivateOnStart->isChecked());
}

void MainWindow::restoreSession()
{
	QSettings settings;

	restoreGeometry(settings.value("geometry").toByteArray());
	restoreState(settings.value("state").toByteArray());
	ui->inputPort->setCurrentText(settings.value("port", ui->inputPort->currentText()).toString());
	mColors->fromStrngList(settings.value("colors").toStringList());

	ui->inputKey->setKeySequence(QKeySequence(settings.value("key", "").toString(), QKeySequence::PortableText));
	ui->inputRed->setValue(settings.value("red", 255).toInt());
	ui->inputGreen->setValue(settings.value("green", 255).toInt());
	ui->inputBlue->setValue(settings.value("blue", 255).toInt());

	ui->inputAutostart->setChecked(settings.value("autostart", false).toBool());

	if (settings.value("activate", false).toBool()) {
		ui->inputActivateOnStart->setChecked(true);
		ui->inputActivate->setChecked(true);
		activate();
	}

	if (settings.value("hidden", false).toBool()) {
		ui->inputStartHidden->setChecked(true);
	} else {
		show();
	}
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if (mClose) {
		saveSession();
		event->accept();
	} else {
		hide();
		event->ignore();
	}
}

void MainWindow::enumeratePorts()
{
	int id = 0;
	ui->inputPort->clear();

	foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
		QString tooltip = tr("Port: %1\nLocation: %2\nDescription: %3\nManufacturer: %4")
			.arg(info.portName(), info.systemLocation(), info.description(), info.manufacturer());

		if (info.hasVendorIdentifier())
			tooltip.append(tr("\nVendor Identifier: %1")).arg(info.vendorIdentifier());

		if (info.hasProductIdentifier())
			tooltip.append(tr("\nProduct Identifier: %1")).arg(info.productIdentifier());

		ui->inputPort->addItem(info.portName());
		ui->inputPort->setItemData(id, QVariant(tooltip), Qt::ToolTipRole);
		id++;
	}
}

void MainWindow::nextPreset()
{
	if (mPort.isOpen()) {
		mPort.write(BuildCommand(mColors->NextColor()));
		mPort.flush();
	}
}

void MainWindow::previousPreset()
{
	if (mPort.isOpen()) {
		mPort.write(BuildCommand(mColors->PreviousColor()));
		mPort.flush();
	}
}

QByteArray MainWindow::BuildCommand(QColor color)
{
	quint8 red       = color.red();
	quint8 green     = color.green();
	quint8 blue      = color.blue();
	quint8 checksumm = - red - green - blue;

	return QString("U%1%2%3%4")
	  .arg(red      , 2, 16, QChar('0'))
	  .arg(green    , 2, 16, QChar('0'))
	  .arg(blue     , 2, 16, QChar('0'))
	  .arg(checksumm, 2, 16, QChar('0'))
	  .toLatin1();
}

void MainWindow::SetVirtualKeycodes(QComboBox *combo)
{
	for (int i = 0; i < 26; i++) combo->addItem(QString(QChar(i + 'A')), i + 'A');
	for (int i = 0; i < 10; i++) combo->addItem(QString(QChar(i + '0')), i + '0');
	for (int i = 0; i < 10; i++) combo->addItem(tr("Numpad %1").arg(i), i + VK_NUMPAD0);
}

void MainWindow::activate()
{
	if (ui->inputActivate->isChecked()) {
		if (!mPort.isOpen()) {
			mPort.setPortName(ui->inputPort->currentText());
			mPort.setBaudRate(QSerialPort::Baud115200);
			mPort.setDataBits(QSerialPort::Data8);
			mPort.setStopBits(QSerialPort::OneStop);
			mPort.setParity(QSerialPort::NoParity);
			mPort.setFlowControl(QSerialPort::NoFlowControl);
			if (!mPort.open(QIODevice::WriteOnly)) {
				ui->inputActivate->setChecked(false);
			}

			mGlobalShortcut.setShortcut(ui->inputKey->keySequence());
		}
	} else {
		mPort.close();
	}
}
