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

#include <QCloseEvent>
#include <QDir>
#include <QRegularExpression>
#include <QSettings>

#include "qup.h"
#include "qup_page.h"

QString qup::VERSION = "2024.07.04";
static const char * const COMPILED_ON = __DATE__ " @ " __TIME__;

qup::qup(void):QMainWindow()
{
  m_ui.setupUi(this);
  m_about.setIconPixmap
    (QPixmap(":/qup_large.png").scaled(QSize(256, 256),
				       Qt::KeepAspectRatio,
				       Qt::SmoothTransformation));
  m_about.setStandardButtons(QMessageBox::Close);
  m_about.setText
    (tr("<html>"
	"<b>Qup Version %1</b><br><br>"
	"Qup is software management made easy.<br>"
	"Made with love by textbrowser.<br><br>"
	"Architecture: %2.<br>"
	"Compiled On: %3.<br>"
	"Product: %4.<br>"
	"Qt version %5 (runtime version %6).<br><br>"
	"Please visit "
	"<a href=\"https://textbrowser.github.io/qup\">"
	"https://textbrowser.github.io/qup</a> for more details.").
     arg(VERSION).
     arg(QSysInfo::currentCpuArchitecture()).
     arg(COMPILED_ON).
     arg(QSysInfo::prettyProductName()).
     arg(QT_VERSION_STR).
     arg(qVersion()));
  m_about.setTextFormat(Qt::RichText);
  m_about.setWindowIcon(windowIcon());
  m_about.setWindowModality(Qt::NonModal);
  m_about.setWindowTitle(tr("Qup: About"));
  connect(m_ui.action_about,
	  &QAction::triggered,
	  this,
	  &qup::slot_about);
  connect(m_ui.action_close_page,
	  &QAction::triggered,
	  this,
	  &qup::slot_close_page);
  connect(m_ui.action_new_page,
	  &QAction::triggered,
	  this,
	  &qup::slot_new_page);
  connect(m_ui.action_quit,
	  &QAction::triggered,
	  this,
	  &qup::slot_quit);
  connect(m_ui.pages,
	  SIGNAL(tabCloseRequested(int)),
	  this,
	  SLOT(slot_tab_close_requested(int)));
  m_ui.action_close_page->setIcon(QIcon::fromTheme("window-close"));
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
      static auto const r
	(QRegularExpression(QString("[%1%1]+").arg(QDir::separator())));

      home_path.replace(r, QDir::separator());

      if(home_path.endsWith(QDir::separator()))
	home_path = home_path.mid(0, home_path.length() - 1);

      return home_path;
    }
}

void qup::closeEvent(QCloseEvent *event)
{
  if(event && m_ui.pages->count() == 0)
    event->accept();
  else if(event && m_ui.pages->count() > 0)
    {
      QMessageBox message(this);
      QPushButton *yes = nullptr;
      QPushButton *yes_all = nullptr;

      message.addButton(tr("No"), QMessageBox::NoRole);
      message.setIcon(QMessageBox::Question);
      message.setText(tr("Active processes are present. Interrupt?"));
      message.setWindowTitle(tr("Qup: Confirmation"));
      yes = message.addButton(tr("Yes"), QMessageBox::YesRole);
      yes_all = message.addButton(tr("Yes (All)"), QMessageBox::YesRole);

      for(int i = 0; i < m_ui.pages->count(); i++)
	{
	  auto page = qobject_cast<qup_page *> (m_ui.pages->widget(i));

	  if(page && page->active())
	    {
	      auto button = message.clickedButton();

	      if(button == nullptr || button == yes)
		{
		  message.exec();
		  QApplication::processEvents();
		}

	      button = message.clickedButton();

	      if(button == yes || button == yes_all)
		page->interrupt();
	      else if(button != nullptr)
		{
		  event->ignore();
		  return;
		}
	    }
	}

      event->accept();
    }

  QSettings settings;

  settings.setValue("geometry", saveGeometry());
  QMainWindow::closeEvent(event);
  QApplication::exit(0);
}

void qup::close_page(QWidget *widget)
{
  auto page = qobject_cast<qup_page *> (widget);

  if(!page)
    return;

  if(page->active())
    {
      m_ui.pages->setCurrentIndex(m_ui.pages->indexOf(page));

      QMessageBox mb(this);

      mb.setIcon(QMessageBox::Question);
      mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
      mb.setText(tr("Interrupt processes?"));
      mb.setWindowIcon(windowIcon());
      mb.setWindowModality(Qt::ApplicationModal);
      mb.setWindowTitle(tr("Qup: Confirmation"));

      if(mb.exec() == QMessageBox::No)
	{
	  QApplication::processEvents();
	  return;
	}

      page->interrupt();
    }

  m_ui.action_close_page->setEnabled(m_ui.pages->count() - 1 > 0);
  m_ui.pages->removeTab(m_ui.pages->indexOf(page));
  page ? page->deleteLater() : (void) 0;
}

void qup::slot_about(void)
{
  connect(m_about.button(QMessageBox::Close),
	  &QPushButton::clicked,
	  &m_about,
	  &QMessageBox::close,
	  Qt::UniqueConnection);
  m_about.button(QMessageBox::Close)->setShortcut(tr("Ctrl+W"));
  m_about.resize(m_about.sizeHint());
  m_about.showNormal();
  m_about.activateWindow();
  m_about.raise();
}

void qup::slot_close_page(void)
{
  close_page(m_ui.pages->widget(m_ui.pages->currentIndex()));
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
  m_ui.action_close_page->setEnabled(true);
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
  close();
}

void qup::slot_tab_close_requested(int index)
{
  close_page(m_ui.pages->widget(index));
}
