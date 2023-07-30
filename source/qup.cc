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
#include <QRegularExpression>
#include <QSettings>

#include "qup.h"

QString qup::QUP_VERSION_STRING = "2023.00.00";

qup::qup(void):QMainWindow()
{
  m_ui.setupUi(this);
  connect(m_ui.action_quit,
	  &QAction::triggered,
	  this,
	  &qup::slot_quit);
  m_ui.temporary_directory->setText(QDir::tempPath());

  QSettings settings;

  restoreGeometry(settings.value("geometry").toByteArray());
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

void qup::slot_quit(void)
{
  QSettings settings;

  settings.setValue("geometry", saveGeometry());
  QApplication::exit(0);
}
