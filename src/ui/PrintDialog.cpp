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

#include "ui/PrintDialog.h"

#include "ui/Theme.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPdfDocument>
#include <QPdfDocumentRenderOptions>
#include <QPrinterInfo>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QStringList>
#include <QVBoxLayout>
#include <QtConcurrent>

namespace {
constexpr int kPreviewW = 320;
constexpr int kPreviewH = 430;
} // namespace

PrintDialog::PrintDialog(QPdfDocument* pdf, QVector<int> order, QVector<int> rotations,
                         const QString& docName, int currentSlot, QWidget* parent)
    : QDialog(parent), m_pdf(pdf), m_order(std::move(order)), m_rot(std::move(rotations)),
      m_pageCount(m_order.size()), m_currentSlot(currentSlot) {
    setWindowTitle(tr("Print"));
    m_rot.resize(m_pageCount);

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Preview (left) ───────────────────────────────────────────────────────
    auto* previewPane = new QFrame(this);
    previewPane->setStyleSheet(
        QStringLiteral("background:%1;").arg(Theme::instance().palette().sunken.name()));
    auto* pv = new QVBoxLayout(previewPane);
    pv->setContentsMargins(24, 24, 24, 16);
    pv->setSpacing(12);

    m_preview = new QLabel(previewPane);
    m_preview->setFixedSize(kPreviewW, kPreviewH);
    m_preview->setAlignment(Qt::AlignCenter);
    pv->addWidget(m_preview, 0, Qt::AlignHCenter);

    auto* navRow = new QHBoxLayout;
    navRow->addStretch(1);
    m_prevPreview = new QPushButton(QStringLiteral("‹"), previewPane);
    m_nextPreview = new QPushButton(QStringLiteral("›"), previewPane);
    m_previewCounter = new QLabel(previewPane);
    m_previewCounter->setAlignment(Qt::AlignCenter);
    m_previewCounter->setMinimumWidth(72);
    for (QPushButton* b : {m_prevPreview, m_nextPreview}) {
        b->setFixedSize(28, 24);
        b->setCursor(Qt::PointingHandCursor);
    }
    navRow->addWidget(m_prevPreview);
    navRow->addWidget(m_previewCounter);
    navRow->addWidget(m_nextPreview);
    navRow->addStretch(1);
    pv->addLayout(navRow);
    root->addWidget(previewPane);

    connect(m_prevPreview, &QPushButton::clicked, this,
            [this] { showPreviewAt(m_previewPos - 1); });
    connect(m_nextPreview, &QPushButton::clicked, this,
            [this] { showPreviewAt(m_previewPos + 1); });

    // ── Options (right) ──────────────────────────────────────────────────────
    auto* opts = new QVBoxLayout;
    opts->setContentsMargins(24, 24, 24, 20);
    opts->setSpacing(10);

    opts->addWidget(new QLabel(tr("Printer"), this));
    m_printer = new QComboBox(this);
    m_printer->addItem(tr("Print to File (PDF)"), QString());
    m_printer->addItem(tr("Searching for printers…"));
    m_printer->setItemData(1, false, Qt::UserRole - 1); // disable the "searching" item
    opts->addWidget(m_printer);

    // Output file row (only for Print to File).
    m_outputRow = new QWidget(this);
    auto* outRow = new QHBoxLayout(m_outputRow);
    outRow->setContentsMargins(0, 0, 0, 0);
    const QString base = QFileInfo(docName).completeBaseName();
    m_outputFile = new QLineEdit(
        QDir(QDir::homePath()).filePath((base.isEmpty() ? tr("document") : base) + ".pdf"),
        m_outputRow);
    auto* browse = new QPushButton(QStringLiteral("…"), m_outputRow);
    browse->setFixedWidth(34);
    outRow->addWidget(m_outputFile);
    outRow->addWidget(browse);
    opts->addWidget(m_outputRow);
    connect(browse, &QPushButton::clicked, this, [this] {
        const QString f = QFileDialog::getSaveFileName(this, tr("Save PDF as"), m_outputFile->text(),
                                                       tr("PDF documents (*.pdf)"));
        if (!f.isEmpty())
            m_outputFile->setText(f);
    });

    opts->addSpacing(6);
    opts->addWidget(new QLabel(tr("Pages"), this));
    m_rangeAll = new QRadioButton(tr("All %1 pages").arg(m_pageCount), this);
    m_rangeCurrent = new QRadioButton(tr("Current page"), this);
    m_rangeCustom = new QRadioButton(tr("Range"), this);
    m_rangeAll->setChecked(true);
    auto* rangeGroup = new QButtonGroup(this);
    for (QRadioButton* r : {m_rangeAll, m_rangeCurrent, m_rangeCustom})
        rangeGroup->addButton(r);
    opts->addWidget(m_rangeAll);
    opts->addWidget(m_rangeCurrent);

    auto* customRow = new QHBoxLayout;
    customRow->addWidget(m_rangeCustom);
    m_rangeText = new QLineEdit(this);
    m_rangeText->setPlaceholderText(tr("e.g. 1-5, 8, 11-13"));
    m_rangeText->setEnabled(false);
    customRow->addWidget(m_rangeText, 1);
    opts->addLayout(customRow);

    // Subset — print all / only odd / only even pages within the range.
    auto* subsetRow = new QHBoxLayout;
    subsetRow->addWidget(new QLabel(tr("Subset"), this));
    m_subset = new QComboBox(this);
    m_subset->addItem(tr("All pages in range"));
    m_subset->addItem(tr("Odd pages only"));
    m_subset->addItem(tr("Even pages only"));
    subsetRow->addWidget(m_subset, 1);
    opts->addLayout(subsetRow);

    opts->addSpacing(10);
    auto* copiesRow = new QHBoxLayout;
    copiesRow->addWidget(new QLabel(tr("Copies"), this));
    m_copies = new QSpinBox(this);
    m_copies->setRange(1, 99);
    copiesRow->addStretch(1);
    copiesRow->addWidget(m_copies);
    opts->addLayout(copiesRow);

    m_grayscale = new QCheckBox(tr("Print in grayscale (black and white)"), this);
    opts->addWidget(m_grayscale);

    opts->addStretch(1);

    auto* buttons = new QDialogButtonBox(this);
    auto* printBtn = buttons->addButton(tr("Print"), QDialogButtonBox::AcceptRole);
    buttons->addButton(QDialogButtonBox::Cancel);
    printBtn->setObjectName("Share"); // reuse the accent-filled primary style
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    opts->addWidget(buttons);

    auto* optsHost = new QWidget(this);
    optsHost->setLayout(opts);
    optsHost->setFixedWidth(300);
    optsHost->setStyleSheet(
        QStringLiteral("background:%1;").arg(Theme::instance().palette().surface.name()));
    root->addWidget(optsHost);

    // React to option changes.
    auto refresh = [this] { rebuildSelection(); };
    connect(m_rangeAll, &QRadioButton::toggled, this, refresh);
    connect(m_rangeCurrent, &QRadioButton::toggled, this, refresh);
    connect(m_rangeCustom, &QRadioButton::toggled, this, [this, refresh](bool on) {
        m_rangeText->setEnabled(on);
        refresh();
    });
    connect(m_rangeText, &QLineEdit::textChanged, this, refresh);
    connect(m_subset, &QComboBox::currentIndexChanged, this, refresh);
    connect(m_printer, &QComboBox::currentIndexChanged, this,
            [this] { updateOutputRowVisibility(); });

    // Discover system printers off the UI thread; add them when ready.
    auto* watcher = new QFutureWatcher<QStringList>(this);
    connect(watcher, &QFutureWatcher<QStringList>::finished, this, [this, watcher] {
        const QStringList names = watcher->result();
        watcher->deleteLater();
        if (m_printer->count() > 1 && !m_printer->itemData(1).isValid())
            m_printer->removeItem(1); // drop the "Searching…" placeholder
        for (const QString& n : names)
            m_printer->addItem(n, n);
    });
    watcher->setFuture(QtConcurrent::run([] {
        QStringList names;
        const QString def = QPrinterInfo::defaultPrinterName();
        for (const QPrinterInfo& pi : QPrinterInfo::availablePrinters())
            names << pi.printerName();
        if (!def.isEmpty() && names.removeAll(def) > 0)
            names.prepend(def); // default first
        return names;
    }));

    updateOutputRowVisibility();
    rebuildSelection();
    resize(660, 520);
}

void PrintDialog::updateOutputRowVisibility() {
    const bool toFile = m_printer->currentData().toString().isEmpty();
    m_outputRow->setVisible(toFile);
}

QList<int> PrintDialog::parseRange(const QString& text) const {
    QList<int> pages;
    const QStringList parts = text.split(',', Qt::SkipEmptyParts);
    for (QString part : parts) {
        part = part.trimmed();
        if (part.contains('-')) {
            const QStringList ends = part.split('-');
            if (ends.size() != 2)
                continue;
            bool ok1 = false, ok2 = false;
            int a = ends[0].trimmed().toInt(&ok1);
            int b = ends[1].trimmed().toInt(&ok2);
            if (!ok1 || !ok2)
                continue;
            if (a > b)
                std::swap(a, b);
            for (int p = a; p <= b; ++p)
                if (p >= 1 && p <= m_pageCount)
                    pages << (p - 1);
        } else {
            bool ok = false;
            const int p = part.toInt(&ok);
            if (ok && p >= 1 && p <= m_pageCount)
                pages << (p - 1);
        }
    }
    return pages;
}

void PrintDialog::rebuildSelection() {
    m_selection.clear();
    if (m_rangeCurrent->isChecked()) {
        if (m_currentSlot >= 0 && m_currentSlot < m_pageCount)
            m_selection << m_currentSlot;
    } else if (m_rangeCustom->isChecked()) {
        m_selection = parseRange(m_rangeText->text());
    } else {
        for (int i = 0; i < m_pageCount; ++i)
            m_selection << i;
    }

    // Subset filter: keep only odd / even page numbers (1-based) within the range.
    const int subset = m_subset ? m_subset->currentIndex() : 0;
    if (subset == 1 || subset == 2) {
        const bool wantOdd = (subset == 1);
        QList<int> filtered;
        for (int slot : std::as_const(m_selection)) {
            const bool isOdd = ((slot + 1) % 2) == 1;
            if (isOdd == wantOdd)
                filtered << slot;
        }
        m_selection = filtered;
    }

    // The Print button needs a non-empty selection.
    if (auto* box = findChild<QDialogButtonBox*>()) {
        for (QAbstractButton* b : box->buttons()) {
            if (box->buttonRole(b) == QDialogButtonBox::AcceptRole)
                b->setEnabled(!m_selection.isEmpty());
        }
    }
    showPreviewAt(0);
}

void PrintDialog::showPreviewAt(int positionInSelection) {
    if (m_selection.isEmpty()) {
        m_preview->setPixmap({});
        m_preview->setText(tr("No pages selected"));
        m_previewCounter->clear();
        m_prevPreview->setEnabled(false);
        m_nextPreview->setEnabled(false);
        return;
    }
    m_previewPos = qBound(0, positionInSelection, m_selection.size() - 1);
    const int slot = m_selection[m_previewPos];
    m_preview->setPixmap(renderSlot(slot, kPreviewW, kPreviewH));
    m_previewCounter->setText(tr("%1 / %2  ·  page %3")
                                  .arg(m_previewPos + 1)
                                  .arg(m_selection.size())
                                  .arg(slot + 1));
    m_prevPreview->setEnabled(m_previewPos > 0);
    m_nextPreview->setEnabled(m_previewPos < m_selection.size() - 1);
}

QPixmap PrintDialog::renderSlot(int slot, int maxW, int maxH) const {
    if (!m_pdf || slot < 0 || slot >= m_order.size())
        return {};
    const int orig = m_order[slot];
    const int rot = m_rot.value(slot, 0);
    QSizeF pt = m_pdf->pagePointSize(orig);
    if (rot == 90 || rot == 270)
        pt.transpose();
    if (pt.width() <= 0 || pt.height() <= 0)
        return {};

    const qreal dpr = devicePixelRatioF();
    const double scale = std::min(maxW / pt.width(), maxH / pt.height());
    const QSize px(qRound(pt.width() * scale * dpr), qRound(pt.height() * scale * dpr));

    QPdfDocumentRenderOptions opts;
    switch (rot) {
    case 90: opts.setRotation(QPdfDocumentRenderOptions::Rotation::Clockwise90); break;
    case 180: opts.setRotation(QPdfDocumentRenderOptions::Rotation::Clockwise180); break;
    case 270: opts.setRotation(QPdfDocumentRenderOptions::Rotation::Clockwise270); break;
    default: break;
    }
    QPixmap pm = QPixmap::fromImage(m_pdf->render(orig, px, opts));
    pm.setDevicePixelRatio(dpr);
    return pm;
}

QString PrintDialog::selectedPrinter() const {
    return m_printer->currentData().toString();
}

QString PrintDialog::outputFile() const {
    return m_outputFile->text();
}

QList<int> PrintDialog::selectedSlots() const {
    return m_selection;
}

int PrintDialog::copies() const {
    return m_copies->value();
}

bool PrintDialog::grayscale() const {
    return m_grayscale->isChecked();
}
