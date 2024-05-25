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

#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>
#include <QtDebug>

#include "qup.h"

QString qup::QUP_VERSION_STRING = "2024.00.00";
char const static *const s_end_of_file = "# End of file. Required comment.";
const int static s_populate_favorites_interval = 250;
const int static s_show_message_interval = 5000;

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
  connect(m_ui.download,
	  &QToolButton::clicked,
	  this,
	  &qup::slot_download);
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

void qup::append(const QString &text)
{
  if(text.trimmed().isEmpty())
    return;

  m_ui.activity->append
    (QString("<u>[%1]</u>: %2").
     arg(QDateTime::currentDateTime().toString(Qt::ISODate)).
     arg(text.trimmed()));
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

void qup::slot_download(void)
{
  auto local_directory(m_ui.local_directory->text().trimmed());

  if(local_directory.isEmpty())
    {
      append
	(tr("<font color='darkred'>Please provide a product directory."
	    "</font>"));
      return;
    }

  auto name(m_ui.favorite_name->text().trimmed());

  if(name.isEmpty())
    {
      append
	(tr("<font color='darkred'>Please provide a product name.</font>"));
      return;
    }

  auto url(QUrl::fromUserInput(m_ui.qup_txt_location->text().trimmed()));

  if(url.isEmpty() || url.isValid() == false)
    {
      append
	(tr("<font color='darkred'>Please provide a valid product URL."
	    "</font>"));
      return;
    }

  auto path(m_ui.temporary_directory->text());

  path.append(QDir::separator());
  path.append("qup-");
  path.append(name);

  QDir directory;
  auto text(tr("<b>Creating %1... </b>").arg(path));

  if(directory.mkpath(path) == false)
    {
      text.append(tr("<font color='darkred'>Failure.</font>"));
      append(text);
      return;
    }
  else
    text.append(tr("<font color='darkgreen'>Created.</font>"));

  append(text);

  // Download the instruction file.

  append(QString("<b>Downloading the file %1.</b>").arg(url.toString()));
  m_instruction_file_reply ?
    m_instruction_file_reply->deleteLater() : (void) 0;
  m_instruction_file_reply = m_network_access_manager.get
    (QNetworkRequest(url));
  m_instruction_file_reply->setProperty
    ("file_name", path + QDir::separator() + url.fileName());
  m_instruction_file_reply_data.clear();
  connect(m_instruction_file_reply,
	  &QNetworkReply::readyRead,
	  this,
	  &qup::slot_write_instruction_file_data);
}

void qup::slot_parse_instruction_file(void)
{
  /*
  ** General section(s).
  */
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
    {
      color = QColor(144, 238, 144); // Light green!
      m_ui.local_directory->setToolTip("");
    }
  else
    m_ui.local_directory->setToolTip(tr("Writable directory, please."));

  palette.setColor(m_ui.local_directory->backgroundRole(), color);
  m_ui.local_directory->setPalette(palette);
}

void qup::slot_write_instruction_file_data(void)
{
  if(!m_instruction_file_reply)
    return;

  while(m_instruction_file_reply->bytesAvailable() > 0)
    {
      m_instruction_file_reply_data.append
	(m_instruction_file_reply->readAll());

      if(m_instruction_file_reply_data.trimmed().endsWith(s_end_of_file))
	break;
    }

  if(m_instruction_file_reply_data.trimmed().endsWith(s_end_of_file))
    {
      QFile file(m_instruction_file_reply->property("file_name").toString());
      QFileInfo file_information
	(m_instruction_file_reply->property("file_name").toString());

      if(file.open(QIODevice::Truncate | QIODevice::WriteOnly))
	{
	  if(file.write(m_instruction_file_reply_data) ==
	     static_cast<qint64> (m_instruction_file_reply_data.length()))
	    {
	      QTimer::singleShot(250, this, &qup::slot_parse_instruction_file);
	      append
		(tr("<font color='darkgreen'>File %1 saved locally.</font>").
		 arg(file_information.fileName()));
	    }
	  else
	    append
	      (tr("<font color='darkred'>Could not write the entire file %1."
		  "</font>").arg(file_information.fileName()));
	}
      else
	append
	  (tr("<font color='darkred'>Could not open a local file %1.</file>").
	   arg(file_information.fileName()));

      m_instruction_file_reply->deleteLater();
      m_instruction_file_reply_data.clear();
    }
}
