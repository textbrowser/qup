/*
** Copyright (c) 2023, Alexis Megas.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from Glitch without specific prior written permission.
**
** QUP IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** QUP, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>
#include <QtDebug>

#include "qup.h"

QString qup::QUP_VERSION_STRING = "2024.00.00";
static int s_populate_favorites_interval = 250;
static int s_show_message_interval = 5000;

qup::qup(void):QMainWindow()
{
  m_ui.setupUi(this);
  QTimer::singleShot
    (s_populate_favorites_interval, this, &qup::slot_populate_favorites);
  connect(&m_timer,
	  &QTimer::timeout,
	  this,
	  &qup::slot_timeout);
  connect(m_ui.action_quit,
	  &QAction::triggered,
	  this,
	  &qup::slot_quit);
  connect(m_ui.delete_favorite,
	  &QToolButton::clicked,
	  this,
	  &qup::slot_delete_favorite);
  connect(m_ui.favorites,
	  &QToolButton::clicked,
	  m_ui.favorites,
	  &QToolButton::showMenu);
  connect(m_ui.save_favorite,
	  &QPushButton::clicked,
	  this,
	  &qup::slot_save_favorite);
  connect(m_ui.select_local_directory,
	  &QPushButton::clicked,
	  this,
	  &qup::slot_select_local_directory);
  m_timer.start(1500);
  m_ui.favorites->setArrowType(Qt::NoArrow);
  m_ui.favorites->setMenu(new QMenu(this));
#ifdef Q_OS_MACOS
#else
  m_ui.favorites->setPopupMode(QToolButton::MenuButtonPopup);
#endif
#ifdef Q_OS_MACOS
  m_ui.favorites->setStyleSheet
    ("QToolButton {border: none;}"
     "QToolButton::menu-button {border: none;}"
     "QToolButton::menu-indicator {image: none;}");
#endif
  m_ui.install->setEnabled(false);
  m_ui.temporary_directory->setText(QDir::tempPath());
  restoreGeometry(QSettings().value("geometry").toByteArray());
}

qup::~qup()
{
}

QString qup::home_path(void)
{
  QString home_path(qgetenv("QUP_HOME").trimmed());

  if(home_path.isEmpty())
#ifdef Q_OS_WIN32
    return QDir::currentPath() + QDir::separator() + ".qup";
#else
    return QDir::homePath() + QDir::separator() + ".qup";
#endif
  else
    {
      static auto r
	(QRegularExpression(QString("[%1%1]+").arg(QDir::separator())));

      home_path.replace(r, QDir::separator());

      if(home_path.endsWith(QDir::separator()))
	home_path = home_path.mid(0, home_path.length() - 1);

      return home_path;
    }
}

void qup::closeEvent(QCloseEvent *event)
{
  QMainWindow::closeEvent(event);
  slot_quit();
}

void qup::slot_delete_favorite(void)
{
  auto name(m_ui.favorite_name->text().trimmed());

  if(name.isEmpty())
    return;

  QMessageBox mb(this);

  mb.setIcon(QMessageBox::Question);
  mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
  mb.setText(tr("Delete %1? Are you sure?").arg(name));
  mb.setWindowIcon(windowIcon());
  mb.setWindowModality(Qt::ApplicationModal);
  mb.setWindowTitle(tr("Qup: Confirmation"));

  if(mb.exec() == QMessageBox::No)
    {
      QApplication::processEvents();
      return;
    }

  QApplication::processEvents();

  QSettings settings;

  settings.beginGroup(QString("favorite-%1").arg(name));
  settings.remove("");
  settings.endGroup(); // Optional.

  if(settings.status() == QSettings::NoError)
    slot_populate_favorites();
  else
    statusBar()->showMessage
      (tr("Could not delete %1.").arg(name), s_show_message_interval);
}

void qup::slot_populate_favorite(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  QSettings settings;

  settings.beginGroup(QString("favorite-%1").arg(action->text()));
  m_ui.favorite_name->setText(settings.value("name").toString());
  m_ui.local_directory->setText(settings.value("local-directory").toString());
  m_ui.qup_txt_location->setText(settings.value("url").toString());
  settings.endGroup(); // Optional.
}

void qup::slot_populate_favorites(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QMap<QString, char> groups;
  QSettings settings;

  foreach(auto const &group, settings.childGroups())
    {
      settings.beginGroup(group);
      groups[settings.value("name").toString().trimmed()] = 0;
      settings.endGroup();
    }

  m_ui.favorites->menu()->clear();

  foreach(auto const &key, groups.keys())
    if(!key.isEmpty())
      m_ui.favorites->menu()->addAction
	(key, this, &qup::slot_populate_favorite);

  m_ui.favorites->setEnabled(!groups.isEmpty());
  QApplication::restoreOverrideCursor();
}

void qup::slot_quit(void)
{
  QSettings settings;

  settings.setValue("geometry", saveGeometry());
  QApplication::exit(0);
}

void qup::slot_save_favorite(void)
{
  auto local_directory(m_ui.local_directory->text().trimmed());
  auto name(m_ui.favorite_name->text().trimmed());
  auto url(m_ui.qup_txt_location->text().trimmed());

  if(local_directory.trimmed().isEmpty() || name.isEmpty() || url.isEmpty())
    {
      statusBar()->showMessage
	(tr("Please complete the required fields."), s_show_message_interval);
      return;
    }

  QSettings settings;

  settings.beginGroup(QString("favorite-%1").arg(name));
  settings.setValue("local-directory", local_directory);
  settings.setValue("name", name);
  settings.setValue("url", url);
  settings.endGroup();

  if(settings.status() == QSettings::NoError)
    statusBar()->showMessage
      (tr("The favorite %1 has been saved in the Qup INI file.").arg(name),
       s_show_message_interval);
  else
    statusBar()->showMessage
      (tr("The favorite %1 cannot be saved in the Qup INI file!").arg(name),
       s_show_message_interval);

  QTimer::singleShot
    (s_populate_favorites_interval, this, &qup::slot_populate_favorites);
}

void qup::slot_select_local_directory(void)
{
  QFileDialog dialog(this);

  dialog.selectFile(m_ui.local_directory->text());
  dialog.setDirectory(QDir::homePath());
  dialog.setFileMode(QFileDialog::Directory);
  dialog.setLabelText(QFileDialog::Accept, tr("Select"));
  dialog.setWindowTitle(tr("Qup: Select Download Path"));

  if(dialog.exec() == QDialog::Accepted)
    {
      QApplication::processEvents();
      m_ui.local_directory->setText(dialog.selectedFiles().value(0));
    }
  else
    QApplication::processEvents();
}

void qup::slot_timeout(void)
{
  QColor color(240, 128, 128); // Light coral!
  auto palette(m_ui.local_directory->palette());

  if(QFileInfo(m_ui.local_directory->text().trimmed()).isWritable())
    color = QColor(144, 238, 144); // Light green!

  palette.setColor(m_ui.local_directory->backgroundRole(), color);
  m_ui.local_directory->setPalette(palette);
}
