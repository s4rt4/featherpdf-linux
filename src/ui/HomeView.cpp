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

#include "ui/HomeView.h"

#include "ui/Theme.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>
#include <algorithm>

namespace {
constexpr auto kRecentFilesKey = "recentFiles";
constexpr int kMaxCards = 6;

// First local PDF in a drag payload, or empty.
QString pdfFromMime(const QMimeData* mime) {
    if (!mime || !mime->hasUrls())
        return {};
    for (const QUrl& url : mime->urls()) {
        if (!url.isLocalFile())
            continue;
        const QString path = url.toLocalFile();
        if (path.endsWith(".pdf", Qt::CaseInsensitive))
            return path;
    }
    return {};
}
} // namespace

// A single recent-file card: icon chip + name + dim path, clickable.
class RecentCard : public QWidget {
    Q_OBJECT

public:
    RecentCard(const QString& path, QWidget* parent) : QWidget(parent), m_path(path) {
        setObjectName("RecentCard");
        setAttribute(Qt::WA_StyledBackground, true);
        setCursor(Qt::PointingHandCursor);

        auto* row = new QHBoxLayout(this);
        row->setContentsMargins(12, 11, 14, 11);
        row->setSpacing(12);

        m_chip = new QLabel(this);
        m_chip->setObjectName("CardChip");
        m_chip->setFixedSize(36, 36);
        m_chip->setAlignment(Qt::AlignCenter);
        row->addWidget(m_chip);

        const QFileInfo fi(path);
        auto* texts = new QVBoxLayout;
        texts->setContentsMargins(0, 0, 0, 0);
        texts->setSpacing(2);
        m_name = new QLabel(fi.fileName(), this);
        m_name->setObjectName("CardName");
        m_dir = new QLabel(fi.absolutePath(), this);
        m_dir->setObjectName("CardDir");
        m_dir->setToolTip(path);
        texts->addWidget(m_name);
        texts->addWidget(m_dir);
        row->addLayout(texts, 1);

        refresh();
    }

    void refresh() {
        const auto& pal = Theme::instance().palette();
        m_chip->setStyleSheet(
            QStringLiteral("background:%1; border-radius:9px;").arg(pal.accentTint.name()));
        m_chip->setPixmap(Theme::instance().icon("file", pal.accent).pixmap(18, 18));
        m_name->setStyleSheet(
            QStringLiteral("color:%1; font-size:14px; font-weight:600;").arg(pal.text.name()));
        m_dir->setStyleSheet(QStringLiteral("color:%1; font-size:12px;").arg(pal.dim.name()));
    }

signals:
    void activated(const QString& path);

protected:
    void mouseReleaseEvent(QMouseEvent*) override { emit activated(m_path); }

private:
    QString m_path;
    QLabel* m_chip = nullptr;
    QLabel* m_name = nullptr;
    QLabel* m_dir = nullptr;
};

HomeView::HomeView(QWidget* parent) : QWidget(parent) {
    setObjectName("HomeView");
    setAcceptDrops(true);

    // Center a fixed-width content column on the canvas.
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addStretch(1);

    auto* center = new QHBoxLayout;
    center->addStretch(1);

    auto* col = new QVBoxLayout;
    col->setSpacing(0);
    col->setAlignment(Qt::AlignTop);
    auto* colHost = new QWidget(this);
    colHost->setLayout(col);
    colHost->setFixedWidth(560);

    m_logo = new QLabel(this);
    m_logo->setAlignment(Qt::AlignHCenter);
    col->addWidget(m_logo);
    col->addSpacing(14);

    m_title = new QLabel(tr("Feather PDF"), this);
    m_title->setAlignment(Qt::AlignHCenter);
    col->addWidget(m_title);
    col->addSpacing(4);

    m_tagline = new QLabel(tr("Light on the system, full-featured on PDF."), this);
    m_tagline->setAlignment(Qt::AlignHCenter);
    col->addWidget(m_tagline);
    col->addSpacing(24);

    auto* openBtn = new QPushButton(tr("Open a document"), this);
    openBtn->setObjectName("HomePrimary");
    openBtn->setCursor(Qt::PointingHandCursor);
    connect(openBtn, &QPushButton::clicked, this, &HomeView::openRequested);
    auto* openRow = new QHBoxLayout;
    openRow->addStretch(1);
    openRow->addWidget(openBtn);
    openRow->addStretch(1);
    col->addLayout(openRow);
    col->addSpacing(8);

    auto* hint = new QLabel(tr("or drop a PDF anywhere here"), this);
    hint->setObjectName("HomeHint");
    hint->setAlignment(Qt::AlignHCenter);
    col->addWidget(hint);
    col->addSpacing(30);

    m_recentHead = new QLabel(tr("RECENT"), this);
    m_recentHead->setObjectName("HomeRecentHead");
    col->addWidget(m_recentHead);
    col->addSpacing(6);

    m_cards = new QWidget(this);
    m_cardsLayout = new QVBoxLayout(m_cards);
    m_cardsLayout->setContentsMargins(0, 0, 0, 0);
    m_cardsLayout->setSpacing(6);
    col->addWidget(m_cards);

    center->addWidget(colHost);
    center->addStretch(1);
    outer->addLayout(center);
    outer->addStretch(2);

    rebuildIcons();
    refresh();
    connect(&Theme::instance(), &Theme::changed, this, [this] {
        rebuildIcons();
        update();
    });
}

void HomeView::rebuildIcons() {
    const auto& pal = Theme::instance().palette();
    m_logo->setPixmap(Theme::instance().brandLogo(76));
    m_title->setStyleSheet(
        QStringLiteral("color:%1; font-size:24px; font-weight:700;").arg(pal.text.name()));
    m_tagline->setStyleSheet(QStringLiteral("color:%1; font-size:13px;").arg(pal.dim.name()));
    m_recentHead->setStyleSheet(QStringLiteral("color:%1; font-size:11px; font-weight:700;"
                                               " letter-spacing:1px;")
                                    .arg(pal.dim.name()));
    if (auto* hint = findChild<QLabel*>("HomeHint"))
        hint->setStyleSheet(QStringLiteral("color:%1; font-size:12px;").arg(pal.dim.name()));
}

void HomeView::refresh() {
    // Clear existing cards.
    while (QLayoutItem* item = m_cardsLayout->takeAt(0)) {
        if (QWidget* w = item->widget())
            w->deleteLater();
        delete item;
    }

    QSettings settings;
    QStringList files = settings.value(kRecentFilesKey).toStringList();
    // Only offer files that still exist.
    files.erase(std::remove_if(files.begin(), files.end(),
                               [](const QString& p) { return !QFileInfo::exists(p); }),
                files.end());

    const bool hasRecent = !files.isEmpty();
    m_recentHead->setVisible(hasRecent);
    m_cards->setVisible(hasRecent);

    int shown = 0;
    for (const QString& path : files) {
        if (shown++ >= kMaxCards)
            break;
        auto* card = new RecentCard(path, m_cards);
        connect(card, &RecentCard::activated, this, &HomeView::openPathRequested);
        m_cardsLayout->addWidget(card);
    }
}

void HomeView::dragEnterEvent(QDragEnterEvent* event) {
    if (!pdfFromMime(event->mimeData()).isEmpty()) {
        m_dragActive = true;
        update();
        event->acceptProposedAction();
    }
}

void HomeView::dragLeaveEvent(QDragLeaveEvent*) {
    m_dragActive = false;
    update();
}

void HomeView::dropEvent(QDropEvent* event) {
    m_dragActive = false;
    update();
    const QString path = pdfFromMime(event->mimeData());
    if (!path.isEmpty()) {
        event->acceptProposedAction();
        emit openPathRequested(path);
    }
}

void HomeView::paintEvent(QPaintEvent*) {
    const auto& pal = Theme::instance().palette();
    QPainter p(this);
    p.fillRect(rect(), pal.canvas);

    // While a PDF is dragged over, show an accent dashed invitation frame.
    if (m_dragActive) {
        p.setRenderHint(QPainter::Antialiasing, true);
        QPen pen(pal.accent, 2, Qt::DashLine);
        p.setPen(pen);
        QPainterPath path;
        path.addRoundedRect(QRectF(rect()).adjusted(14, 14, -14, -14), 16, 16);
        p.drawPath(path);
    }
}

#include "HomeView.moc"
