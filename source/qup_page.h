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

#ifndef _qup_page_h_
#define _qup_page_h_

#include <QFuture>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QTimer>

#include "ui_qup_page.h"

class qup_page: public QWidget
{
  Q_OBJECT

 public:
  qup_page(QWidget *parent);
  ~qup_page();

 public slots:
  void slot_populate_favorites(void);

 private:
  class FileInformation
  {
  public:
    QString m_destination;
    bool m_executable;
  };

  QByteArray m_instruction_file_reply_data;
  QFuture<void> m_copy_files_future;
  QNetworkAccessManager m_network_access_manager;
  QPointer<QNetworkReply> m_instruction_file_reply;
  QString m_destination;
  QString m_operating_system;
  QString m_path;
  QString m_qup_txt_file_name;
  QTimer m_copy_files_timer;
  QTimer m_timer;
  Ui_qup_page m_ui;
  bool m_ok;
  QString executable_suffix(void) const;
  void closeEvent(QCloseEvent *event);
  void copy_files
    (const QString &destination_path, const QString &local_path);
  void download_files
    (const QHash<QString, FileInformation> &files,
     const QString &directory_destination,
     const QString &file_destination,
     const QUrl &url);
 private slots:
  void append(const QString &text);
  void slot_copy_files(void);
  void slot_delete_favorite(void);
  void slot_download(void);
  void slot_install(void);
  void slot_parse_instruction_file(void);
  void slot_populate_favorite(void);
  void slot_reply_finished(void);
  void slot_save_favorite(void);
  void slot_select_local_directory(void);
  void slot_timeout(void);
  void slot_write_file(void);
  void slot_write_instruction_file_data(void);

 signals:
  void append_text(const QString &text);
  void populate_favorites(void);
};

#endif
