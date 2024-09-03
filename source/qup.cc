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
#include <QColorDialog>
#include <QDir>
#include <QPainter>
#include <QRegularExpression>
#include <QSettings>

#include "qup.h"
#include "qup_page.h"
#include "qup_swifty.h"

QColor qup::INVALID_PROCESS_COLOR = QColor(255, 114, 118);
QColor qup::VALID_PROCESS_COLOR = QColor(144, 238, 144);
QString qup::VERSION = "2024.09.15";
static const char * const COMPILED_ON = __DATE__ " @ " __TIME__;

qup::qup(void):QMainWindow()
{
  m_ui.setupUi(this);
  m_about.setIconPixmap
    (QPixmap(":/qup_large.png").scaled(QSize(256, 256),
				       Qt::KeepAspectRatio,
				       Qt::SmoothTransformation));
  m_about.setStandardButtons(QMessageBox::Close);
  m_about.setTextFormat(Qt::RichText);
  m_about.setWindowIcon(windowIcon());
  m_about.setWindowModality(Qt::NonModal);
  m_about.setWindowTitle(tr("Qup: About"));
  m_swifty = new swifty
    (VERSION,
     "QString qup::VERSION = ",
     QUrl::fromUserInput("https://raw.githubusercontent.com/"
			 "textbrowser/qup/master/source/qup.cc"),
     this);
  m_swifty->download();
  assign_image(m_ui.process_invalid_color, INVALID_PROCESS_COLOR);
  assign_image(m_ui.process_valid_color, VALID_PROCESS_COLOR);
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
  connect(m_ui.process_invalid_color,
	  &QPushButton::clicked,
	  this,
	  &qup::slot_select_color);
  connect(m_ui.process_valid_color,
	  &QPushButton::clicked,
	  this,
	  &qup::slot_select_color);
  m_ui.action_close_page->setIcon(QIcon::fromTheme("window-close"));
  m_ui.action_new_page->setIcon(QIcon::fromTheme("document-new"));
  m_ui.menu_tabs->setStyleSheet("QMenu {menu-scrollable: 1;}");
  m_ui.process_invalid_color->setText
    (INVALID_PROCESS_COLOR.name(QColor::HexArgb));
  m_ui.temporary_directory->setText(QDir::tempPath());
  m_ui.process_valid_color->setText(VALID_PROCESS_COLOR.name(QColor::HexArgb));
  restore_settings();
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

void qup::assign_image(QPushButton *button, const QColor &color)
{
  if(!button)
    return;

  QImage image(QSize(16, 16), QImage::Format_ARGB32);
  QPainter painter(&image);

  image.fill(color);
  button->setIcon(QPixmap::fromImage(image));
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

  QSettings().setValue("geometry", saveGeometry());
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
  prepare_tabs_menu();
}

void qup::restore_settings(void)
{
  QColor color;
  QSettings settings;

  color = QColor(settings.value("invalid-process-color").toString().trimmed());
  INVALID_PROCESS_COLOR = color.isValid() ? color : INVALID_PROCESS_COLOR;
  color = QColor(settings.value("valid-process-color").toString().trimmed());
  VALID_PROCESS_COLOR = color.isValid() ? color : VALID_PROCESS_COLOR;
  restoreGeometry(settings.value("geometry").toByteArray());
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
  m_about.setText
    (tr("<html>"
	"<b>Qup Version %1</b><br>"
	"The official version is <b>%2</b>.<br><br>"
	"Qup is software management made easy.<br>"
	"Made with love by textbrowser.<br><br>"
	"Architecture: %3.<br>"
	"Compiled On: %4.<br>"
	"Product: %5.<br>"
	"Qt version %6 (runtime version %7).<br><br>"
	"Please visit "
	"<a href=\"https://textbrowser.github.io/qup\">"
	"https://textbrowser.github.io/qup</a> for more information.").
     arg(VERSION).
     arg(m_swifty->newest_version()).
     arg(QSysInfo::currentCpuArchitecture()).
     arg(COMPILED_ON).
     arg(QSysInfo::prettyProductName()).
     arg(QT_VERSION_STR).
     arg(qVersion()));
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
  connect(page->tabs_menu_action(),
	  &QAction::triggered,
	  this,
	  &qup::slot_select_page);
  connect(this,
	  &qup::populate_favorites,
	  page,
	  &qup_page::slot_populate_favorites);
  connect(this,
	  &qup::settings_applied,
	  page,
	  &qup_page::slot_settings_applied);
  m_ui.action_close_page->setEnabled(true);
  m_ui.pages->setCurrentIndex(m_ui.pages->addTab(page, tr("Download")));
  prepare_tabs_menu();
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

void qup::slot_select_color(void)
{
  if(!(m_ui.process_invalid_color == sender() ||
       m_ui.process_valid_color == sender()))
    return;

  QColorDialog dialog(this);
  auto button = qobject_cast<QPushButton *> (sender());

  dialog.setCurrentColor(QColor(button->text().remove('&')));
  dialog.setOption(QColorDialog::ShowAlphaChannel, true);
  dialog.setWindowIcon(windowIcon());
  QApplication::processEvents();

  if(dialog.exec() == QDialog::Accepted)
    {
      QApplication::processEvents();
      assign_image(button, dialog.selectedColor());
      button->setText(dialog.selectedColor().name(QColor::HexArgb));

      QSettings settings;

      if(button == m_ui.process_invalid_color)
	{
	  INVALID_PROCESS_COLOR = dialog.selectedColor();
	  settings.setValue
	    ("invalid-process-color", button->text().remove('&'));
	}
      else
	{
	  VALID_PROCESS_COLOR = dialog.selectedColor();
	  settings.setValue("valid-process-color", button->text().remove('&'));
	}

      emit settings_applied();
    }
}

void qup::slot_select_page(void)
{
  auto action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
  m_ui.pages->setCurrentWidget(qobject_cast<QWidget *> (action->parent()));
#else
  m_ui.pages->setCurrentWidget(action->parentWidget());
#endif
}

void qup::slot_tab_close_requested(int index)
{
  close_page(m_ui.pages->widget(index));
}

void qup::prepare_tabs_menu(void)
{
  m_ui.menu_tabs->clear();

  if(m_ui.pages->count() == 0)
    {
      m_ui.menu_tabs->setEnabled(false);
      return;
    }
  else
    m_ui.menu_tabs->setEnabled(true);

  auto group = m_ui.menu_tabs->findChild<QActionGroup *> ();

  if(!group)
    group = new QActionGroup(m_ui.menu_tabs);

  for(int i = 0; i < m_ui.pages->count(); i++)
    {
      auto page = qobject_cast<qup_page *> (m_ui.pages->widget(i));

      if(page)
	{
	  auto action = page->tabs_menu_action();

	  if(action)
	    {
	      action->setCheckable(true);
	      m_ui.menu_tabs->addAction(action);

	      if(i != m_ui.pages->currentIndex())
		action->setChecked(false);
	      else
		action->setChecked(true);

	      group->addAction(action);
	    }
	}
    }
}
