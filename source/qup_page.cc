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
#include <QMenu>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSettings>
#include <QTimer>

#include "qup_page.h"

char const static *const s_end_of_file = "# End of file. Required comment.";
const int static s_populate_favorites_interval = 250;

qup_page::qup_page(QWidget *parent):QWidget(parent)
{
  m_ok = true;
  m_ui.setupUi(this);
  QTimer::singleShot
    (s_populate_favorites_interval, this, &qup_page::slot_populate_favorites);
  connect(&m_timer,
	  &QTimer::timeout,
	  this,
	  &qup_page::slot_timeout);
  connect(m_ui.delete_favorite,
	  &QToolButton::clicked,
	  this,
	  &qup_page::slot_delete_favorite);
  connect(m_ui.download,
	  &QToolButton::clicked,
	  this,
	  &qup_page::slot_download);
  connect(m_ui.favorites,
	  &QToolButton::clicked,
	  m_ui.favorites,
	  &QToolButton::showMenu);
  connect(m_ui.reset,
	  &QPushButton::clicked,
	  m_ui.activity,
	  &QTextEdit::clear);
  connect(m_ui.save_favorite,
	  &QPushButton::clicked,
	  this,
	  &qup_page::slot_save_favorite);
  connect(m_ui.select_local_directory,
	  &QPushButton::clicked,
	  this,
	  &qup_page::slot_select_local_directory);
  m_network_access_manager.setRedirectPolicy
    (QNetworkRequest::NoLessSafeRedirectPolicy);
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
}

qup_page::~qup_page()
{
}

QString qup_page::executable_suffix(void) const
{
  return "";
}

void qup_page::append(const QString &text)
{
  if(text.trimmed().isEmpty())
    return;

  m_ui.activity->append
    (QString("<u>[%1]</u>: %2").
     arg(QDateTime::currentDateTime().toString(Qt::ISODate)).
     arg(text.trimmed()));
}

void qup_page::closeEvent(QCloseEvent *event)
{
  QWidget::closeEvent(event);
}

void qup_page::download_files(const QHash<QString, FileInformation> &files,
			      const QString &directory_destination,
			      const QString &file_destination,
			      const QUrl &url)
{
  if(files.isEmpty() || url.isEmpty() || url.isValid() == false)
    return;

  QHashIterator<QString, FileInformation> it(files);

  while(it.hasNext())
    {
      it.next();

      if(it.key().trimmed().isEmpty())
	continue;

      QNetworkReply *reply = nullptr;
      auto dot = it.value().m_destination == "." ||
	it.value().m_destination.startsWith("./");
      auto remote_file_name(url.toString());

      remote_file_name.append('/');
      remote_file_name.append(it.key());
      append(tr("Downloading %1.").arg(remote_file_name));
      reply = m_network_access_manager.get
	(QNetworkRequest(QUrl::fromUserInput(remote_file_name)));
      reply->ignoreSslErrors();
      reply->setProperty
	("destination_directory", dot ? "" : directory_destination);
      reply->setProperty
	("destination_file", dot ? it.key() : file_destination);
      reply->setProperty("file_name", it.key());
      connect(reply,
	      &QNetworkReply::finished,
	      this,
	      &qup_page::slot_reply_finished);
      connect(reply,
	      &QNetworkReply::readyRead,
	      this,
	      &qup_page::slot_write_file);
    }
}

void qup_page::slot_delete_favorite(void)
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
  mb.setWindowTitle(tr("Qup_Page: Confirmation"));

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
    {
      QTimer::singleShot(s_populate_favorites_interval,
			 this,
			 &qup_page::slot_populate_favorites);
      emit populate_favorites();
    }
  else
    append(tr("Could not delete %1.").arg(name));
}

void qup_page::slot_download(void)
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

  m_path = QDir::tempPath();
  m_path.append(QDir::separator());
  m_path.append("qup-");
  m_path.append(name);

  QDir directory;
  auto text(tr("<b>Creating %1... </b>").arg(m_path));

  if(directory.mkpath(m_path) == false)
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
  m_instruction_file_reply_data.clear();
  m_qup_txt_file_name = m_path + QDir::separator() + url.fileName();
  connect(m_instruction_file_reply,
	  &QNetworkReply::readyRead,
	  this,
	  &qup_page::slot_write_instruction_file_data);
}

void qup_page::slot_parse_instruction_file(void)
{
  if(m_qup_txt_file_name.trimmed().isEmpty())
    return;

  QFile file(m_qup_txt_file_name);

  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
      QHash<QString, qup_page::FileInformation> files;
      QString executable_suffix(this->executable_suffix());
      QString file_destination("");
      QTextStream stream(&file);
      auto general = false;
      auto unix = false;

      while(!stream.atEnd())
	{
	  auto line(stream.readLine().trimmed());
	  auto position = line.indexOf('#');

	  if(position > 0)
	    line = line.mid(0, position);

	  if(line == "[General]")
	    {
	      general = true;
	      continue;
	    }
	  else if(line == "[Unix]")
	    {
#ifdef Q_OS_MACOS
	      unix = false;
#elif defined(Q_OS_UNIX)
	      unix = true;
#endif
	      continue;
	    }
	  else if(line.startsWith('#'))
	    continue;

	  if(general)
	    {
	      auto list(line.split('='));
	      auto p
		(qMakePair(list.value(0).trimmed(), list.value(1).trimmed()));

	      if(p.first.isEmpty() || p.second.isEmpty())
		continue;

	      if(p.first == "file")
		{
		  FileInformation file_information;

		  file_information.m_executable = false;
		  files[p.second] = file_information;
		}
	      else if(p.first == "file_destination")
		file_destination = p.second;
	      else if(p.first == "url")
		{
		  // Begin the download(s).

		  download_files
		    (files,
		     file_destination, // Directory.
		     "",
		     QUrl::fromUserInput(p.second));
		  file_destination.clear();
		  files.clear();
		  general = false;
		}
	    }
	  else if(unix)
	    {
	      auto list(line.split('='));
	      auto p
		(qMakePair(list.value(0).trimmed(), list.value(1).trimmed()));

	      if(p.first.isEmpty() || p.second.isEmpty())
		continue;

	      auto executable
		(QString("executable:%1").
		 arg(QSysInfo::currentCpuArchitecture().toLower()));

	      if(executable == p.first)
		{
		  FileInformation file_information;

		  file_information.m_executable = false;
		  file_information.m_destination = "";
		  files[p.second] = file_information;
		}
	      else if(p.first == "file")
		{
		  FileInformation file_information;

		  file_information.m_executable = false;
		  file_information.m_destination = p.second;
		  files[p.second] = file_information;
		}
	      else if(p.first == "local_executable")
		file_destination = p.second;
	      else if(p.first == "shell")
		{
		  FileInformation file_information;

		  file_information.m_executable = true;
		  file_information.m_destination = p.second;
		  files[p.second] = file_information;
		}
	      else if(p.first == "url")
		{
		  // Begin the download(s).

		  download_files
		    (files,
		     "", // Directory.
		     file_destination,
		     QUrl::fromUserInput(p.second));
		  file_destination.clear();
		  files.clear();
		  unix = false;
		}
	    }
	}
    }
  else
    {
      append(tr("Cannot open %1 for processing.").arg(m_qup_txt_file_name));
      return;
    }
}

void qup_page::slot_populate_favorite(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  QSettings settings;

  settings.beginGroup(QString("favorite-%1").arg(action->text()));
  m_ui.favorite_name->setText(settings.value("name").toString().trimmed());
  m_ui.local_directory->setText
    (settings.value("local-directory").toString().trimmed());
  m_ui.operating_system->setCurrentIndex
    (m_ui.operating_system->
     findText(settings.value("operating-system").toString().trimmed()));
  m_ui.operating_system->setCurrentIndex
    (m_ui.operating_system->currentIndex() < 0 ?
     0 : m_ui.operating_system->currentIndex());
  m_ui.qup_txt_location->setText(settings.value("url").toString().trimmed());
  settings.endGroup(); // Optional.
}

void qup_page::slot_populate_favorites(void)
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
	(key, this, &qup_page::slot_populate_favorite);

  m_ui.favorites->setEnabled(!groups.isEmpty());
  QApplication::restoreOverrideCursor();
}

void qup_page::slot_reply_finished(void)
{
  auto reply = qobject_cast<QNetworkReply *> (sender());

  if(!reply)
    {
      append("<font color='red'>Cannot discover QNetworkReply object. "
	     "Serious problem!</font>");
      return;
    }

  if(reply->error() != QNetworkReply::NoError)
    {
      append
	(tr("<font color='red'>An error occurred while downloading %1."
	    "</font>").arg(reply->property("file_name").toString()));
      m_ok = false;
    }
  else
    append
      (tr("<font color='green'>Completed downloading %1.</font>").
       arg(reply->property("file_name").toString()));

  reply->deleteLater();
}

void qup_page::slot_save_favorite(void)
{
  auto local_directory(m_ui.local_directory->text().trimmed());
  auto name(m_ui.favorite_name->text().trimmed());
  auto url(m_ui.qup_txt_location->text().trimmed());

  if(local_directory.trimmed().isEmpty() || name.isEmpty() || url.isEmpty())
    {
      append(tr("Please complete the required fields."));
      return;
    }

  QSettings settings;

  settings.beginGroup(QString("favorite-%1").arg(name));
  settings.setValue("local-directory", local_directory);
  settings.setValue("name", name);
  settings.setValue("operating-system", m_ui.operating_system->currentText());
  settings.setValue("url", url);
  settings.endGroup();

  if(settings.status() == QSettings::NoError)
    {
      QTimer::singleShot(s_populate_favorites_interval,
			 this,
			 &qup_page::slot_populate_favorites);
      append
	(tr("The favorite %1 has been saved in the Qup_Page INI file.").
	 arg(name));
      emit populate_favorites();
    }
  else
    append
      (tr("The favorite %1 cannot be saved in the Qup_Page INI file!").
       arg(name));
}

void qup_page::slot_select_local_directory(void)
{
  QFileDialog dialog(this);

  dialog.selectFile(m_ui.local_directory->text());
  dialog.setDirectory(QDir::homePath());
  dialog.setFileMode(QFileDialog::Directory);
  dialog.setLabelText(QFileDialog::Accept, tr("Select"));
  dialog.setWindowTitle(tr("Qup_Page: Select Download Path"));

  if(dialog.exec() == QDialog::Accepted)
    {
      QApplication::processEvents();
      m_ui.local_directory->setText(dialog.selectedFiles().value(0));
    }
  else
    QApplication::processEvents();
}

void qup_page::slot_timeout(void)
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

void qup_page::slot_write_file(void)
{
  auto reply = qobject_cast<QNetworkReply *> (sender());

  if(!reply)
    return;

  if(!reply->property("destination_directory").toString().isEmpty())
    {
      QDir directory;

      directory.mkpath
	(m_path +
	 QDir::separator() +
	 reply->property("destination_directory").toString());
    }

  QFile file;

  if(!reply->property("destination_directory").toString().isEmpty())
    file.setFileName
      (m_path +
       QDir::separator() +
       reply->property("destination_directory").toString() +
       QDir::separator() +
       reply->property("file_name").toString());
  else
    file.setFileName
      (m_path +
       QDir::separator() +
       reply->property("destination_file").toString());

  QIODevice::OpenMode flags = QIODevice::NotOpen;

  if(reply->property("read").toBool())
    flags = QIODevice::Append | QIODevice::WriteOnly;
  else
    flags = QIODevice::Truncate | QIODevice::WriteOnly;

  reply->setProperty("read", true);

  if(file.open(flags))
    while(reply->bytesAvailable() > 0)
      {
	append(tr("Writing data into %1.").arg(file.fileName()));
	file.write(reply->readAll());
      }
}

void qup_page::slot_write_instruction_file_data(void)
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
      QFile file(m_qup_txt_file_name);
      QFileInfo file_information(m_qup_txt_file_name);

      if(file.open(QIODevice::Text |
		   QIODevice::Truncate |
		   QIODevice::WriteOnly))
	{
	  if(file.write(m_instruction_file_reply_data) ==
	     static_cast<qint64> (m_instruction_file_reply_data.length()))
	    {
	      QTimer::singleShot
		(250, this, &qup_page::slot_parse_instruction_file);
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
