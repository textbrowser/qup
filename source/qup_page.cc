/*
** Copyright (c) 2024, Alexis Megas.
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
#include <QDirIterator>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkReply>
#include <QScrollBar>
#include <QSettings>
#include <QTimer>
#include <QtConcurrent>

#include "qup_page.h"

class PropertyNames
{
 public:
  char const static *AbsoluteFilePath;
  char const static *DestinationDirectory;
  char const static *DestinationFile;
  char const static *Executable;
  char const static *FileName;
  char const static *Read;
};

char const *PropertyNames::AbsoluteFilePath = "absolute_file_path";
char const *PropertyNames::DestinationDirectory = "destination_directory";
char const *PropertyNames::DestinationFile = "destination_file";
char const *PropertyNames::Executable = "executable";
char const *PropertyNames::FileName = "file_name";
char const *PropertyNames::Read = "read";
char const static *const s_end_of_file = "# End of file. Required comment.";
const int static s_populate_favorites_interval = 250;

qup_page::qup_page(QWidget *parent):QWidget(parent)
{
  m_copy_files_timer.setInterval(1500);
  m_copy_files_timer.setSingleShot(true);
  m_ok = true;
  m_ui.setupUi(this);
  QTimer::singleShot
    (s_populate_favorites_interval, this, &qup_page::slot_populate_favorites);
  connect(&m_copy_files_timer,
	  &QTimer::timeout,
	  this,
	  &qup_page::slot_copy_files);
  connect(&m_copy_files_future_watcher,
	  &QFutureWatcher<void>::finished,
	  this,
	  &qup_page::launch_file_gatherer);
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
  connect(m_ui.install,
	  &QToolButton::clicked,
	  this,
	  &qup_page::slot_install);
  connect(m_ui.launch,
	  &QPushButton::clicked,
	  this,
	  &qup_page::slot_launch);
  connect(m_ui.refresh,
	  &QPushButton::clicked,
	  this,
	  &qup_page::launch_file_gatherer);
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
  connect(this,
	  SIGNAL(append_text(const QString &)),
	  this,
	  SLOT(append(const QString &)));
  connect(this,
	  SIGNAL(files_gathered(const QByteArray &,
				const QVector<QVector<QString> > &)),
	  this,
	  SLOT(slot_populate_files_table(const QByteArray &,
					 const QVector<QVector<QString> > &)));
  m_network_access_manager.setRedirectPolicy
    (QNetworkRequest::NoLessSafeRedirectPolicy);
  m_timer.start(2500);
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
  m_ui.refresh->setIcon(QIcon::fromTheme("view-refresh"));
  m_ui.reset->setIcon(QIcon::fromTheme("edit-reset"));
  m_ui.select_local_directory->setIcon(QIcon::fromTheme("document-open"));
  prepare_operating_systems_widget();
}

qup_page::~qup_page()
{
  m_copy_files_future.cancel();
  m_copy_files_future.waitForFinished();
  m_copy_files_timer.stop();
  m_populate_files_table_future.cancel();
  m_populate_files_table_future.waitForFinished();
  m_timer.stop();
}

QString qup_page::executable_suffix(void) const
{
  if(m_operating_system == "Debian 12 AMD64")
    return "_debian_12_amd64";
  else if(m_operating_system == "Debian 13 AMD64")
    return "_debian_13_amd64";
  else if(m_operating_system == "FreeBSD 13 AMD64")
    return "_freebsd_13_amd64";
  else if(m_operating_system == "FreeBSD 14 AMD64")
    return "_freebsd_14_amd64";
  else if(m_operating_system == "MacOS Apple Silicon")
    return "_macos_apple_silicon";
  else if(m_operating_system == "MacOS Intel")
    return "_macos_intel";
  else if(m_operating_system == "PiOS 12 ARM")
    return "_pios_12_arm";
  else if(m_operating_system == "PiOS 12 ARM64")
    return "_pios_12_arm64";
  else if(m_operating_system == "PiOS 13 ARM")
    return "_pios_13_arm";
  else if(m_operating_system == "PiOS 13 ARM64")
    return "_pios_13_arm64";
  else if(m_operating_system == "Ubuntu 24.04 AMD64")
    return "_ubuntu_24_04_amd64";
  else if(m_operating_system == "Ubuntu 16.04 PowerPC")
    return "_ubuntu_16_04_powerpc";
  else if(m_operating_system == "Windows 11")
    return "";
  else
    return "";
}

bool qup_page::active(void) const
{
  return m_copy_files_future.isRunning() ||
    m_network_access_manager.findChildren<QNetworkReply *> ().size() > 0;
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

void qup_page::copy_files
(const QString &destination_path, const QString &local_path)
{
  QDirIterator it
    (local_path,
     QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
     QDirIterator::Subdirectories);

  while(it.hasNext() && m_copy_files_future.isCanceled() == false)
    {
      it.next();

      auto const &file_information(it.fileInfo());

      if(file_information.isDir())
	{
	  auto destination(destination_path);

	  destination.append(QDir::separator());
	  destination.append(file_information.fileName());

	  if(!QFileInfo(destination).exists())
	    {
	      QString text("");

	      text.append(tr("Creating %1... ").arg(destination));

	      if(QDir().mkpath(destination))
		text.append(tr("<font color='darkgreen'>Created.</font>"));
	      else
		text.append(tr("<font color='darkred'>Failure.</font>"));

	      emit append_text(text);
	    }
	}
      else
	{
	  auto destination(destination_path);

	  destination.append(QDir::separator());
	  destination.append(file_information.dir().dirName());

	  if(QFileInfo(destination).isDir())
	    destination.append(QDir::separator());
	  else
	    destination = destination_path + QDir::separator();

	  QString text("");

	  destination.append(file_information.fileName());

	  if(QFileInfo(destination).exists())
	    QFile::remove(destination);

	  text.append
	    (tr("Copying %1 to %2... ").
	     arg(file_information.absoluteFilePath()).
	     arg(destination));

	  if(QFile::copy(file_information.absoluteFilePath(), destination))
	    {
	      text.append(tr("<font color='darkgreen'>Copied.</font>"));
	      emit append_text(text);
	      text.clear();
	      text.append
		(tr("Setting permissions on %1... ").arg(destination));

	      QFile file(destination);

	      if(file.
		 setPermissions(QFileInfo(file_information.
					  absoluteFilePath()).permissions()))
		text.append(tr("<font color='darkgreen'>Success.</font>"));
	      else
		text.append(tr("<font color='darkred'>Failure.</font>"));

	      emit append_text(text);
	    }
	  else
	    {
	      text.append(tr("<font color='darkred'>Failure.</font>"));
	      emit append_text(text);
	    }
	}
    }
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
      auto const &dot = it.value().m_destination == "." ||
	it.value().m_destination.startsWith("./");
      auto remote_file_name(url.toString());

      remote_file_name.append('/');
      remote_file_name.append(it.key());
      append(tr("Downloading %1.").arg(remote_file_name));
      reply = m_network_access_manager.get
	(QNetworkRequest(QUrl::fromUserInput(remote_file_name)));
      reply->ignoreSslErrors();
      reply->setProperty
	(PropertyNames::DestinationDirectory, dot ? "" : directory_destination);
      reply->setProperty
	(PropertyNames::DestinationFile, dot ? it.key() : file_destination);
      reply->setProperty(PropertyNames::Executable, it.value().m_executable);
      reply->setProperty(PropertyNames::FileName, it.key());
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

void qup_page::gather_files
(const QByteArray &super_hash,
 const QString &destination_path,
 const QString &local_path)
{
  QCryptographicHash sha3_512(QCryptographicHash::Sha3_512);
  QDirIterator it
    (destination_path,
     QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
     QDirIterator::Subdirectories);
  QVector<QVector<QString> > data;

  while(it.hasNext() && m_populate_files_table_future.isCanceled() == false)
    {
      it.next();

      auto const &file_information(it.fileInfo());

      if(file_information.isFile())
	{
	  QVector<QString> vector(static_cast<int> (FilesColumns::XYZ));

	  {
	    QCryptographicHash sha3_256(QCryptographicHash::Sha3_256);
	    QFile file(file_information.absoluteFilePath());

	    if(file.open(QIODevice::ReadOnly))
	      {
		sha3_256.addData(&file);
		file.close();
	      }

	    vector[static_cast<int> (FilesColumns::LocalFileDigest)] =
	      sha3_256.result().toHex();
	  }

	  {
	    QCryptographicHash sha3_256(QCryptographicHash::Sha3_256);
	    QFile file;

	    file.setFileName
	      (local_path + QDir::separator() + file_information.fileName());

	    if(file.open(QIODevice::ReadOnly))
	      {
		sha3_256.addData(&file);
		file.close();
	      }

	    vector[static_cast<int> (FilesColumns::TemporaryFileDigest)] =
	      sha3_256.result().toHex();
	  }

	  vector[static_cast<int> (FilesColumns::LocalFileName)] =
	    file_information.absoluteFilePath();
	  vector[static_cast<int> (FilesColumns::LocalFilePermissions)] =
	    QString::number(file_information.permissions());

	  QFileInfo temporary_file_information
	    (local_path + QDir::separator() + file_information.fileName());

	  vector[static_cast<int> (FilesColumns::TemporaryFileName)] =
	    temporary_file_information.absoluteFilePath();
	  vector[static_cast<int> (FilesColumns::TemporaryFilePermissions)] =
	    QString::number(temporary_file_information.permissions());
	  data << vector;

	  foreach(auto const &i, vector)
	    sha3_512.addData(i.toUtf8());
	}
    }

  if(sha3_512.result() != super_hash)
    emit files_gathered(sha3_512.result(), data);
}

void qup_page::launch_file_gatherer(void)
{
  if(m_populate_files_table_future.isFinished())
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    m_populate_files_table_future = QtConcurrent::run
      (this, &qup_page::gather_files, m_super_hash, m_destination, m_path);
#else
    m_populate_files_table_future = QtConcurrent::run
      (&qup_page::gather_files, this, m_super_hash, m_destination, m_path);
#endif
}

void qup_page::prepare_operating_systems_widget(void)
{
  m_ui.operating_system->clear();
  m_ui.operating_system->addItems
    (QStringList() << "Debian 12 AMD64"
                   << "Debian 13 AMD64"
                   << "FreeBSD 13 AMD64"
                   << "FreeBSD 14 AMD64"
                   << "MacOS Apple Silicon"
                   << "MacOS Intel"
                   << "PiOS 12 ARM"
                   << "PiOS 12 ARM64"
                   << "PiOS 13 ARM"
                   << "PiOS 13 ARM64"
                   << "Ubuntu 24.04 AMD64"
                   << "Ubuntu 16.04 PowerPC"
                   << "Windows 11");
}

void qup_page::slot_copy_files(void)
{
  if(m_network_access_manager.findChildren<QNetworkReply *> ().size() > 0 &&
     m_ok)
    {
      m_copy_files_timer.start();
      return;
    }
  else if(m_copy_files_future.isFinished() == false || m_ok == false)
    {
      m_copy_files_timer.stop();
      return;
    }

  append
    (tr("<font color='darkgreen'>You may now install %1!</font>").
     arg(m_product));
  launch_file_gatherer();
  m_ui.install->setEnabled(true);
}

void qup_page::slot_delete_favorite(void)
{
  auto const &name(m_ui.favorite_name->text().trimmed());

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
    {
      QTimer::singleShot(s_populate_favorites_interval,
			 this,
			 &qup_page::slot_populate_favorites);
      emit populate_favorites();
    }
  else
    append(tr("<font color='darkred'>Could not delete %1.</font>").arg(name));
}

void qup_page::slot_download(void)
{
  if(m_copy_files_future.isRunning())
    {
      append
	(tr("<font color='darkred'>Downloaded files are being copied. Please "
	    "wait until the process completes.</font>"));
      return;
    }

  auto const &local_directory(m_ui.local_directory->text().trimmed());

  if(local_directory.isEmpty())
    {
      append
	(tr("<font color='darkred'>Please provide a product directory."
	    "</font>"));
      return;
    }

  auto const &name(m_ui.favorite_name->text().trimmed());

  if(name.isEmpty())
    {
      append
	(tr("<font color='darkred'>Please provide a product name.</font>"));
      return;
    }

  auto const &url(QUrl::fromUserInput(m_ui.qup_txt_location->text().trimmed()));

  if(url.isEmpty() || url.isValid() == false)
    {
      append
	(tr("<font color='darkred'>Please provide a valid product URL."
	    "</font>"));
      return;
    }

  m_destination = m_ui.local_directory->text().trimmed();
  m_operating_system = m_ui.operating_system->currentText();
  m_path = QDir::tempPath();
  m_path.append(QDir::separator());
  m_path.append("qup-");
  m_path.append(name);
  m_product = name;

  if(!QFileInfo(m_path).exists())
    {
      auto text(tr("<b>Creating %1... </b>").arg(m_path));

      if(QDir().mkpath(m_path) == false)
	{
	  text.append(tr("<font color='darkred'>Failure.</font>"));
	  append(text);
	  return;
	}
      else
	text.append(tr("<font color='darkgreen'>Created.</font>"));

      append(text);
    }
  else
    append(tr("The destination path %1 exists.").arg(m_path));

  /*
  ** Download the instructions file.
  */

  append(tr("<b>Downloading the file %1.</b>").arg(url.toString()));
  m_instruction_file_reply = m_network_access_manager.get(QNetworkRequest(url));
  m_instruction_file_reply_data.clear();
  m_ok = true;
  m_qup_txt_file_name = m_path + QDir::separator() + url.fileName();
  m_ui.install->setEnabled(false);
  connect(m_instruction_file_reply,
	  &QNetworkReply::finished,
	  this,
	  &qup_page::slot_instruction_reply_finished);
  connect(m_instruction_file_reply,
	  &QNetworkReply::readyRead,
	  this,
	  &qup_page::slot_write_instruction_file_data);
}

void qup_page::slot_install(void)
{
  if(m_copy_files_future.isRunning())
    {
      append
	(tr("<font color='darkred'>Downloaded files are being copied. Please "
	    "wait until the process completes.</font>"));
      return;
    }

  append
    (tr("<b>Copying files from %1 to %2.</b>").arg(m_path).arg(m_destination));

  if(!QFileInfo(m_destination).exists())
    {
      QString text("");

      text.append(tr("<b>Creating %1... </b>").arg(m_destination));

      if(QDir().mkpath(m_destination))
	text.append(tr("<font color='darkgreen'>Created.</font>"));
      else
	text.append(tr("<font color='darkred'>Failure.</font>"));

      append(text);
    }

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  m_copy_files_future = QtConcurrent::run
    (this, &qup_page::copy_files, m_destination, m_path);
#else
  m_copy_files_future = QtConcurrent::run
    (&qup_page::copy_files, this, m_destination, m_path);
#endif
  m_copy_files_future_watcher.setFuture(m_copy_files_future);
}

void qup_page::slot_instruction_reply_finished(void)
{
  if(m_instruction_file_reply)
    {
      if(m_instruction_file_reply->error() != QNetworkReply::NoError)
	append
	  (tr("<font color='darkred'>Could not download %1. Perhaps "
	      "the file does not exist.</font>").
	   arg(m_instruction_file_reply->url().toString()));

      m_instruction_file_reply->deleteLater();
    }
}

void qup_page::slot_launch(void)
{
  auto executable(m_destination);
  auto result = false;

  executable.append(QDir::separator());
  executable.append(m_product);
#ifdef Q_OS_MACOS
  executable.append(".app");

  if(QFileInfo(executable).isBundle())
    {
      QStringList list;

      list << "-a" << executable << "-g";
      result = QProcess::startDetached("open", list, m_destination);
    }
  else
    append(tr("<font color='darkred'>The executable %1 is not a bundle. "
	      "Cannot launch.</font>").arg(executable));
#elif defined(Q_OS_OS2)
  executable.append(".exe");

  if(QFileInfo(executable).isExecutable())
    result = QProcess::startDetached
      (QString("\"%1\"").arg(executable), QStringList(), m_destination);
  else
    append(tr("<font color='darkred'>The file %1 is not an executable. "
	      "Cannot launch.</font>").arg(executable));
#elif defined(Q_OS_WINDOWS)
  executable.append(".exe");

  if(QFileInfo(executable).isExecutable())
    result = QProcess::startDetached
      (QString("\"%1\"").arg(executable), QStringList(), m_destination);
  else
    append(tr("<font color='darkred'>The file %1 is not an executable. "
	      "Cannot launch.</font>").arg(executable));
#else
  if(QFileInfo(executable).isExecutable())
    result = QProcess::startDetached(executable, QStringList(), m_destination);
  else
    append(tr("<font color='darkred'>The file %1 is not an executable. "
	      "Cannot launch.</font>").arg(executable));
#endif

  if(result)
    append(tr("<font color='darkgreen'>The program %1 was launched.</font>").
	   arg(executable));
  else
    append
      (tr("<font color='darkred'>The program %1 was not launched.</font>").
       arg(executable));
}

void qup_page::slot_parse_instruction_file(void)
{
  if(m_qup_txt_file_name.trimmed().isEmpty())
    return;

  QFile file(m_qup_txt_file_name);

  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
      QHash<QString, qup_page::FileInformation> files;
      QString file_destination("");
      QTextStream stream(&file);
      auto general = false;
      auto unix = false;

      while(!stream.atEnd())
	{
	  auto line(stream.readLine().trimmed());
	  auto const &position = line.indexOf('#');

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
	      auto const &list(line.split('='));
	      auto const &p
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
		  /*
		  ** Begin the download(s).
		  */

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
	      auto const &list(line.split('='));
	      auto const &p
		(qMakePair(list.value(0).trimmed(), list.value(1).trimmed()));

	      if(p.first.isEmpty() || p.second.isEmpty())
		continue;

	      if(p.first == "executable" &&
		 p.second.toLower().endsWith(executable_suffix()))
		{
		  FileInformation file_information;

		  file_information.m_executable = true;
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
		  /*
		  ** Begin the download(s).
		  */

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
  m_destination = settings.value("local-directory").toString().trimmed();
  m_path = QDir::tempPath();
  m_path.append(QDir::separator());
  m_path.append("qup-");
  m_path.append(settings.value("name").toString().trimmed());
  m_product = action->text();
  m_super_hash.clear();
  m_ui.favorite_name->setText(settings.value("name").toString().trimmed());
  m_ui.files->setRowCount(0);
  m_ui.files->sortByColumn(0, Qt::AscendingOrder);
  m_ui.install->setEnabled(false);
  m_ui.local_directory->setText
    (settings.value("local-directory").toString().trimmed());
  m_ui.operating_system->setCurrentIndex
    (m_ui.operating_system->
     findText(settings.value("operating-system").toString().trimmed()));
  m_ui.operating_system->setCurrentIndex
    (m_ui.operating_system->currentIndex() < 0 ?
     0 : m_ui.operating_system->currentIndex());
  m_ui.qup_txt_location->setText(settings.value("url").toString().trimmed());
  launch_file_gatherer();
  emit product_name_changed(m_ui.favorite_name->text());
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

void qup_page::slot_populate_files_table
(const QByteArray &hash, const QVector<QVector<QString> > &data)
{
  m_super_hash = hash;

  auto const &h = m_ui.files->horizontalScrollBar()->value();
  auto const &v = m_ui.files->verticalScrollBar()->value();

  m_ui.files->setRowCount(data.size());
  m_ui.files->setSortingEnabled(false);

  for(int i = 0; i < data.size(); i++)
    {
      auto const &file(data.at(i));

      for(int j = 0; j < m_ui.files->columnCount(); j++)
	{
	  auto item = new QTableWidgetItem(file.value(j));

	  item->setFlags(Qt::ItemIsEnabled);
	  item->setToolTip(item->text());
	  m_ui.files->setItem(i, j, item);
	}
    }

  m_ui.files->horizontalScrollBar()->setValue(h);
  m_ui.files->resizeColumnsToContents();
  m_ui.files->setSortingEnabled(true);
  m_ui.files->sortByColumn
    (m_ui.files->horizontalHeader()->sortIndicatorSection(),
     m_ui.files->horizontalHeader()->sortIndicatorOrder());
  m_ui.files->verticalScrollBar()->setValue(v);
}

void qup_page::slot_reply_finished(void)
{
  auto reply = qobject_cast<QNetworkReply *> (sender());

  if(!reply)
    {
      append(tr("<font color='darkred'>Cannot discover QNetworkReply object. "
		"Serious problem!</font>"));
      return;
    }

  if(reply->error() != QNetworkReply::NoError)
    {
      QFile::remove
	(reply->property(PropertyNames::AbsoluteFilePath).toString());
      append
	(tr("<font color='darkred'>An error occurred while downloading %1."
	    "</font>").arg(reply->property("file_name").toString()));
      m_ok = false;
    }
  else
    {
      append
	(tr("<font color='darkgreen'>Completed downloading %1.</font>").
	 arg(reply->property(PropertyNames::FileName).toString()));

      if(reply->property(PropertyNames::Executable).toBool())
	{
	  QFile file
	    (reply->property(PropertyNames::AbsoluteFilePath).toString());

	  file.setPermissions(QFileDevice::ExeOwner | file.permissions());
	}
    }

  reply->deleteLater();

  if(m_ok)
    /*
    ** Downloads completed!
    */

    m_copy_files_timer.start();
}

void qup_page::slot_save_favorite(void)
{
  auto const &local_directory
    (QDir::cleanPath(m_ui.local_directory->text().trimmed()));
  auto const &name(m_ui.favorite_name->text().trimmed());
  auto const &url(m_ui.qup_txt_location->text().trimmed());

  if(local_directory.trimmed().isEmpty() || name.isEmpty() || url.isEmpty())
    {
      append(tr("<font color='darkred'>Please complete the "
		"required fields.</font>"));
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
	(tr("<font color='darkgreen'>The favorite %1 has been saved "
	    "in the Qup INI file.</font>").arg(name));
      m_super_hash.clear();
      m_ui.install->setEnabled(false);
      m_ui.local_directory->setText(local_directory);
      emit populate_favorites();
      emit product_name_changed(m_ui.favorite_name->text());
    }
  else
    append
      (tr("<font color='darkred'>The favorite %1 cannot be saved in the "
	  "Qup INI file!</font>").arg(name));
}

void qup_page::slot_select_local_directory(void)
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

  if(!reply->property(PropertyNames::DestinationDirectory).toString().isEmpty())
    QDir().mkpath
      (m_path +
       QDir::separator() +
       reply->property(PropertyNames::DestinationDirectory).toString());

  QFile file;

  if(!reply->property(PropertyNames::DestinationDirectory).toString().isEmpty())
    file.setFileName
      (m_path +
       QDir::separator() +
       reply->property(PropertyNames::DestinationDirectory).toString() +
       QDir::separator() +
       reply->property(PropertyNames::FileName).toString());
  else
    file.setFileName
      (m_path +
       QDir::separator() +
       reply->property(PropertyNames::DestinationFile).toString());

  QIODevice::OpenMode flags = QIODevice::NotOpen;

  if(reply->property(PropertyNames::Read).toBool())
    flags = QIODevice::Append | QIODevice::WriteOnly;
  else
    flags = QIODevice::Truncate | QIODevice::WriteOnly;

  reply->setProperty(PropertyNames::AbsoluteFilePath, file.fileName());
  reply->setProperty(PropertyNames::Read, true);

  if(file.open(flags) && reply->bytesAvailable() > 0)
    {
      append(tr("Writing data into %1.").arg(file.fileName()));

      while(reply->bytesAvailable() > 0)
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

	  launch_file_gatherer();
	}
      else
	append
	  (tr("<font color='darkred'>Could not open a local file %1.</file>").
	   arg(file_information.fileName()));

      m_instruction_file_reply_data.clear();
    }
}
