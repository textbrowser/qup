/*
** Copyright (c) 2024, Alexis Megas.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
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

#ifndef _qup_h_
#define _qup_h_

#include <QMessageBox>

#include "ui_qup.h"

class swifty;

class qup: public QMainWindow
{
  Q_OBJECT

 public:
  qup(void);
  ~qup();
  static QColor INVALID_PROCESS_COLOR;
  static QColor VALID_PROCESS_COLOR;
  static QString VERSION;
  static QString VERSION_LTS;
  static QString home_path(void);
  static void assign_image(QPushButton *button, const QColor &color);

 private:
  QMessageBox m_about;
  Ui_qup m_ui;
  swifty *m_swifty;
  void closeEvent(QCloseEvent *event);
  void close_page(QWidget *widget);
  void prepare_tabs_menu(void);
  void release_notes(void);
  void restore_settings(void);
  void set_proxy(void);

 private slots:
  void slot_about(void);
  void slot_close_page(void);
  void slot_new_page(void);
  void slot_product_name_changed(const QString &t);
  void slot_proxy_changed(const QString &text);
  void slot_quit(void);
  void slot_save_proxy(void);
  void slot_save_proxy_type(int index);
  void slot_select_color(void);
  void slot_select_page(void);
  void slot_tab_close_requested(int index);

 signals:
  void populate_favorites(void);
  void settings_applied(void);
};

#endif
