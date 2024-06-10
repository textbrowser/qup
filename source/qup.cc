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

#include <QDir>
#include <QRegularExpression>
#include <QSettings>

#include "qup.h"
#include "qup_page.h"

QString qup::QUP_VERSION_STRING = "2024.00.00";

qup::qup(void):QMainWindow()
{
  m_ui.setupUi(this);
  connect(m_ui.action_new_page,
	  &QAction::triggered,
	  this,
	  &qup::slot_new_page);
  connect(m_ui.action_quit,
	  &QAction::triggered,
	  this,
	  &qup::slot_quit);
  m_ui.action_new_page->setIcon(QIcon::fromTheme("document-new"));
  m_ui.temporary_directory->setText(QDir::tempPath());
  restoreGeometry(QSettings().value("geometry").toByteArray());
  slot_new_page();
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

void qup::slot_new_page(void)
{
  auto page = new qup_page(this);

  connect(page,
	  &qup_page::populate_favorites,
	  this,
	  &qup::populate_favorites);
  connect(page,
	  SIGNAL(product_name_changed(const QString &)),
	  this,
	  SLOT(slot_product_name_changed(const QString &)));
  connect(this,
	  &qup::populate_favorites,
	  page,
	  &qup_page::slot_populate_favorites);
  m_ui.pages->setCurrentIndex(m_ui.pages->addTab(page, tr("Download")));
}

void qup::slot_product_name_changed(const QString &t)
{
  auto text(t.trimmed());

  if(text.isEmpty())
    text = tr("Download");

  m_ui.pages->setTabText
    (m_ui.pages->indexOf(qobject_cast<qup_page *> (sender())), text);
}

void qup::slot_quit(void)
{
  QSettings settings;

  settings.setValue("geometry", saveGeometry());
  QApplication::exit(0);
}
