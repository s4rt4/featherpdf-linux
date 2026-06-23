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

#include "ui/OutlinePanel.h"

#include "ui/Theme.h"

#include <QLabel>
#include <QPdfBookmarkModel>
#include <QStackedWidget>
#include <QTreeView>
#include <QVBoxLayout>

OutlinePanel::OutlinePanel(QWidget* parent) : QWidget(parent) {
    setObjectName("OutlinePanel");

    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    m_stack = new QStackedWidget(this);

    m_tree = new QTreeView(this);
    m_tree->setObjectName("OutlineTree");
    m_tree->setHeaderHidden(true);
    m_tree->setFrameShape(QFrame::NoFrame);
    m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_tree->setExpandsOnDoubleClick(false);
    m_tree->setAnimated(true);

    m_model = new QPdfBookmarkModel(this);
    m_tree->setModel(m_model);

    m_empty = new QLabel(tr("This document has no outline."), this);
    m_empty->setObjectName("OutlineEmpty");
    m_empty->setAlignment(Qt::AlignCenter);
    m_empty->setWordWrap(true);
    m_empty->setContentsMargins(16, 16, 16, 16);

    m_stack->addWidget(m_tree);  // 0
    m_stack->addWidget(m_empty); // 1
    col->addWidget(m_stack);

    connect(m_tree, &QTreeView::clicked, this, [this](const QModelIndex& index) {
        const int page = m_model->data(index, int(QPdfBookmarkModel::Role::Page)).toInt();
        if (page >= 0)
            emit pageActivated(page);
    });
    connect(m_model, &QAbstractItemModel::modelReset, this, &OutlinePanel::updateEmptyState);
}

void OutlinePanel::setDocument(QPdfDocument* doc) {
    m_model->setDocument(doc);
    m_tree->expandToDepth(0); // top-level chapters open, deeper levels collapsed
    updateEmptyState();
}

void OutlinePanel::clear() {
    m_model->setDocument(nullptr);
    updateEmptyState();
}

void OutlinePanel::updateEmptyState() {
    const bool hasOutline = m_model->rowCount(QModelIndex()) > 0;
    m_stack->setCurrentWidget(hasOutline ? static_cast<QWidget*>(m_tree)
                                         : static_cast<QWidget*>(m_empty));
}
