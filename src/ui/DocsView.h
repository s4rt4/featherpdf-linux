// Feather PDF — light on the system, full-featured on PDF.
// Copyright (C) 2026 Feather PDF contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <QString>
#include <QWidget>

class QTreeWidget;
class QTreeWidgetItem;
class QTextBrowser;
class QPushButton;

// The full in-app documentation, shown as its own workspace tab: a table-of-
// contents sidebar (grouped topics) beside the article for the selected topic.
// Every topic covers what it is, its purpose, how it works, how to use it, and
// troubleshooting - in English and Indonesian (toggle at the top of the TOC).
class DocsView : public QWidget {
    Q_OBJECT

public:
    explicit DocsView(QWidget* parent = nullptr);

private:
    void rebuildToc();
    void setLanguage(const QString& lang);

    QTreeWidget* m_toc = nullptr;
    QTextBrowser* m_browser = nullptr;
    QPushButton* m_en = nullptr;
    QPushButton* m_id = nullptr;
    QString m_lang;        // "en" or "id"
    QString m_currentId;   // currently shown topic id
};
