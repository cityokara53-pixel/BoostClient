// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023-2024 Trial97 <alexandru.tripon97@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "SkinManageDialog.h"
#include "net/RPCSink.h"
#include "ui/dialogs/skins/draw/SkinOpenGLWindow.h"
#include "ui_SkinManageDialog.h"

#include <FileSystem.h>
#include <QAction>
#include <QBuffer>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QDialog>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QImageReader>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLayoutItem>
#include <QListView>
#include <QMenu>
#include <QMimeDatabase>
#include <QMouseEvent>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPropertyAnimation>
#include <QProgressBar>
#include <QPointer>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QToolButton>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <QSaveFile>
#include <QTemporaryFile>

#include <algorithm>
#include <memory>

#include "Application.h"
#include "DesktopServices.h"
#include "QObjectPtr.h"
#include "settings/SettingsObject.h"

#include "minecraft/auth/Parsers.h"
#include "minecraft/skins/SkinList.h"
#include "minecraft/skins/SkinModel.h"
#include "minecraft/skins/SkinRequests.h"

#include "net/NetJob.h"
#include "net/Request.h"
#include "tasks/Task.h"

#include "Json.h"
#include "archive/ArchiveReader.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProgressDialog.h"

namespace {
constexpr qsizetype MAX_SKIN_API_RESPONSE_SIZE = 1024 * 1024;
constexpr qsizetype MAX_SKIN_ARCHIVE_SIZE = 50 * 1024 * 1024;
constexpr qint64 MAX_SKIN_ARCHIVE_TOTAL_BYTES = 100 * 1024 * 1024;
constexpr int MAX_SKIN_ARCHIVE_ENTRIES = 10000;
constexpr int BUNDLED_SKIN_PAGE_SIZE = 100;
constexpr int BUNDLED_SKIN_COLUMNS = 3;

QImage skinCardPreview(const SkinModel& skin)
{
    const auto preview = skin.getPreview();
    return preview.copy(0, 0, preview.width() / 2, preview.height())
        .scaled(144, 288, Qt::KeepAspectRatio, Qt::FastTransformation);
}

class SkinLibraryProxy final : public QSortFilterProxyModel {
   public:
    enum class View { All, Favorites, Recent };

    explicit SkinLibraryProxy(QObject* parent) : QSortFilterProxyModel(parent)
    {
        setDynamicSortFilter(true);
    }

    void setView(View view)
    {
        m_view = view;
        invalidateFilter();
    }

    void setFavorites(QStringList favorites)
    {
        m_favorites = std::move(favorites);
        invalidateFilter();
    }

    void setRecent(QStringList recent)
    {
        m_recent = std::move(recent);
        invalidateFilter();
    }

    void setSearch(QString search)
    {
        m_search = std::move(search);
        invalidateFilter();
    }

   protected:
    bool filterAcceptsRow(int row, const QModelIndex& parent) const override
    {
        const auto sourceIndex = sourceModel()->index(row, 0, parent);
        const auto name = sourceIndex.data(Qt::UserRole).toString();
        if (m_view == View::Favorites && !m_favorites.contains(name))
            return false;
        if (m_view == View::Recent && !m_recent.contains(name))
            return false;
        return m_search.isEmpty() || name.contains(m_search, Qt::CaseInsensitive);
    }

   private:
    View m_view = View::All;
    QStringList m_favorites;
    QStringList m_recent;
    QString m_search;
};

class SkinLibraryDelegate final : public QStyledItemDelegate {
   public:
    explicit SkinLibraryDelegate(std::function<bool(const QString&)> isFavorite,
                                 std::function<void(const QString&)> toggleFavorite,
                                 QObject* parent)
        : QStyledItemDelegate(parent), m_isFavorite(std::move(isFavorite)), m_toggleFavorite(std::move(toggleFavorite))
    {}

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        painter->save();
        const auto selected = option.state & QStyle::State_Selected;
        const auto hovered = option.state & QStyle::State_MouseOver;
        const auto card = option.rect.adjusted(3, 3, -3, -3);
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(QPen(selected ? QColor("#13c8e8") : hovered ? QColor("#2a6c88") : QColor("#20334c"),
                             selected ? 2 : 1));
        painter->setBrush(hovered ? QColor("#0e1f33") : QColor("#0b1728"));
        painter->drawRoundedRect(card, 9, 9);

        auto icon = qvariant_cast<QImage>(index.data(Qt::DecorationRole));
        if (!icon.isNull())
            icon = icon.copy(0, 0, icon.width() / 2, icon.height());
        const auto imageRect = QRect(card.left() + 10, card.top() + 8, card.width() - 20, card.height() - 48);
        if (!icon.isNull())
            painter->drawImage(imageRect, icon, icon.rect());

        const auto favorite = m_isFavorite(index.data(Qt::UserRole).toString());
        painter->setPen(favorite ? QColor("#12c8e8") : QColor("#a7bfdc"));
        painter->drawText(QRect(card.right() - 31, card.top() + 5, 24, 24), Qt::AlignCenter,
                          favorite ? QStringLiteral("♥") : QStringLiteral("♡"));
        painter->setPen(QColor("#e6efff"));
        painter->drawText(QRect(card.left() + 9, card.bottom() - 32, card.width() - 18, 23),
                          Qt::AlignLeft | Qt::AlignVCenter, painter->fontMetrics().elidedText(
                              index.data(Qt::DisplayRole).toString(), Qt::ElideRight, card.width() - 22));
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override { return QSize(158, 168); }

    bool editorEvent(QEvent* event, QAbstractItemModel*, const QStyleOptionViewItem& option, const QModelIndex& index) override
    {
        if (event->type() != QEvent::MouseButtonRelease)
            return false;
        const auto* mouse = static_cast<QMouseEvent*>(event);
        const auto favoriteRect = QRect(option.rect.right() - 34, option.rect.top() + 5, 28, 28);
        if (!favoriteRect.contains(mouse->position().toPoint()))
            return false;
        m_toggleFavorite(index.data(Qt::UserRole).toString());
        return true;
    }

   private:
    std::function<bool(const QString&)> m_isFavorite;
    std::function<void(const QString&)> m_toggleFavorite;
};

class AnimatedSkinCardButton final : public QToolButton {
   public:
    explicit AnimatedSkinCardButton(QWidget* parent) : QToolButton(parent), m_opacity(new QGraphicsOpacityEffect(this))
    {
        m_opacity->setOpacity(0.96);
        setGraphicsEffect(m_opacity);
        m_animation = new QPropertyAnimation(m_opacity, "opacity", this);
        m_animation->setDuration(130);
        m_animation->setEasingCurve(QEasingCurve::OutCubic);
    }

   protected:
    void enterEvent(QEnterEvent* event) override
    {
        m_animation->stop();
        m_animation->setStartValue(m_opacity->opacity());
        m_animation->setEndValue(1.0);
        m_animation->start();
        QToolButton::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override
    {
        m_animation->stop();
        m_animation->setStartValue(m_opacity->opacity());
        m_animation->setEndValue(0.96);
        m_animation->start();
        QToolButton::leaveEvent(event);
    }

   private:
    QGraphicsOpacityEffect* m_opacity;
    QPropertyAnimation* m_animation;
};

QString skinCategory(const QJsonObject& skin)
{
    const auto category = skin.value(QStringLiteral("category")).toString();
    if (!category.isEmpty())
        return category;
    const auto name = skin.value(QStringLiteral("name")).toString().toLower();
    static const QStringList anime = { "anime", "naruto", "gojo", "sukuna", "goku", "luffy", "eren", "nezuko", "tanjiro" };
    static const QStringList pvp = { "pvp", "warrior", "fighter", "assassin", "ninja", "combat" };
    static const QStringList medieval = { "medieval", "knight", "king", "queen", "armor", "armour", "guard", "royal" };
    static const QStringList cute = { "cute", "pastel", "kawaii", "bunny", "cat", "fox", "girl", "bear" };
    static const QStringList mobs = { "creeper", "enderman", "zombie", "skeleton", "spider", "slime", "ghast", "warden", "mob" };
    static const QStringList fantasy = { "fantasy", "dragon", "wizard", "witch", "elf", "mage", "demon", "fairy" };
    const auto matches = [&name](const QStringList& words) {
        return std::any_of(words.cbegin(), words.cend(), [&name](const QString& word) { return name.contains(word); });
    };
    if (matches(anime)) return QStringLiteral("Anime");
    if (matches(pvp)) return QStringLiteral("PvP");
    if (matches(medieval)) return QStringLiteral("Medieval");
    if (matches(cute)) return QStringLiteral("Cute");
    if (matches(mobs)) return QStringLiteral("Mobs");
    if (matches(fantasy)) return QStringLiteral("Fantasy");
    return {};
}

QJsonArray sortSkinCatalog(const QJsonArray& skins)
{
    QList<QJsonObject> sortedSkins;
    sortedSkins.reserve(skins.size());
    for (const auto& skin : skins)
        sortedSkins.append(skin.toObject());
    std::sort(sortedSkins.begin(), sortedSkins.end(), [](const QJsonObject& left, const QJsonObject& right) {
        const auto leftKey = left.value(QStringLiteral("category")).toString() + QLatin1Char('/') +
                             left.value(QStringLiteral("pack")).toString() + QLatin1Char('/') +
                             left.value(QStringLiteral("fileName")).toString();
        const auto rightKey = right.value(QStringLiteral("category")).toString() + QLatin1Char('/') +
                              right.value(QStringLiteral("pack")).toString() + QLatin1Char('/') +
                              right.value(QStringLiteral("fileName")).toString();
        return leftKey.localeAwareCompare(rightKey) < 0;
    });
    QJsonArray result;
    for (const auto& skin : sortedSkins)
        result.append(skin);
    return result;
}

bool isValidSkinPng(const QByteArray& bytes, QImage* decoded = nullptr)
{
    if (bytes.size() < 8 || bytes.size() > MAX_SKIN_API_RESPONSE_SIZE ||
        !bytes.startsWith(QByteArray::fromHex("89504e470d0a1a0a")))
        return false;
    QBuffer buffer;
    buffer.setData(bytes);
    if (!buffer.open(QIODevice::ReadOnly))
        return false;
    QImageReader reader(&buffer, "PNG");
    const auto size = reader.size();
    if (size.width() != 64 || (size.height() != 32 && size.height() != 64))
        return false;
    const auto image = reader.read();
    if (image.isNull())
        return false;
    if (decoded)
        *decoded = image;
    return true;
}

QByteArray skinApiGet(const QUrl& url, QString& error, QString* contentType = nullptr)
{
    QNetworkRequest request(url);
    request.setTransferTimeout(12000);
    auto* reply = APPLICATION->network()->get(request);
    QEventLoop loop;
    QByteArray body;
    QObject::connect(reply, &QNetworkReply::readyRead, &loop, [&] {
        body.append(reply->readAll());
        if (body.size() > MAX_SKIN_API_RESPONSE_SIZE) {
            error = QObject::tr("The skin API response exceeds the 1 MiB limit.");
            reply->abort();
        }
    });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(15000, reply, &QNetworkReply::abort);
    loop.exec();
    body.append(reply->readAll());
    const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (contentType)
        *contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString().section(';', 0, 0).trimmed().toLower();
    if (body.size() > MAX_SKIN_API_RESPONSE_SIZE) {
        error = QObject::tr("The skin API response exceeds the 1 MiB limit.");
        body.clear();
    } else if (reply->error() != QNetworkReply::NoError) {
        error = status == 429 ? QObject::tr("The skin API is rate-limiting requests. Please retry shortly.")
                              : status ? QObject::tr("Skin API returned HTTP %1.").arg(status)
                                       : reply->errorString();
    }
    reply->deleteLater();
    return body;
}
}  // namespace

SkinManageDialog::SkinManageDialog(QWidget* parent, MinecraftAccountPtr acct)
    : QDialog(parent), m_acct(acct), m_ui(new Ui::SkinManageDialog), m_list(this, APPLICATION->settings()->get("SkinsDir").toString(), acct)
{
    m_ui->setupUi(this);
    m_ui->verticalLayout->setSpacing(7);
    m_ui->mainHlLayout->setStretch(0, 7);
    m_ui->mainHlLayout->setStretch(1, 3);
    resize(1280, 720);
    setWindowIcon(APPLICATION->logo());
    setWindowTitle(tr("BoostClient — Skin Studio"));
    setStyleSheet(QStringLiteral(
        "QDialog { background: #06101e; color: #e7f0ff; }"
        "QLabel { color: #dce9fb; }"
        "QGroupBox { border: 1px solid #20334c; border-radius: 9px; margin-top: 11px; padding: 8px; color: #9bb6d6; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
        "QLineEdit, QComboBox { background: #081629; border: 1px solid #243a56; border-radius: 8px; padding: 8px 10px; color: #e7f0ff; }"
        "QLineEdit:focus, QComboBox:focus { border-color: #11c7e8; }"
        "QPushButton { background: #0b1a2d; border: 1px solid #29415f; border-radius: 8px; padding: 8px 14px; color: #dce9fb; }"
        "QPushButton:hover { background: #102541; border-color: #13c8e8; }"
        "QPushButton:pressed { background: #12314a; }"
        "QToolButton { color: #dce9fb; }"
        "QScrollArea { background: transparent; border: 0; }"
        "QListView { background: #071322; border: 1px solid #1c3048; border-radius: 10px; outline: 0; }"
        "QCheckBox, QRadioButton { spacing: 8px; color: #dce9fb; }"
        "QCheckBox::indicator, QRadioButton::indicator { width: 16px; height: 16px; }"));
    m_ui->studioSidebar->setStyleSheet(QStringLiteral(
        "QWidget#studioSidebar { background: #061426; border-right: 1px solid #18304c; }"
        "QToolButton { min-height: 42px; padding: 0 11px; text-align: left; border: 0; border-radius: 4px; background: transparent; "
        "color: #a9bfdc; font-weight: 600; }"
        "QToolButton:hover { background: #0c2138; color: #f1f8ff; }"
        "QToolButton:checked { background: #103251; border-left: 3px solid #10c9ed; color: #f4fbff; }"));
    m_ui->brandNameLabel->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 700; color: #f0f6ff;"));
    m_ui->brandIconLabel->setPixmap(APPLICATION->logo().pixmap(28, 28));
    m_ui->brandIconLabel->setAlignment(Qt::AlignCenter);
    m_ui->brandFooterLabel->setText(tr("BoostClient\nMinecraft Java"));
    m_ui->brandFooterLabel->setStyleSheet(QStringLiteral("font-size: 10px; color: #91a9c8; padding: 8px;"));
    m_ui->titleLabel->setStyleSheet(QStringLiteral("font-size: 25px; font-weight: 700; color: #f0f6ff;"));
    m_ui->titleLabel->setText(tr("Skin <span style=\"color:#11c8e8\">Studio</span>"));
    m_ui->subtitleLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #91a9c8;"));
    m_ui->studioIconLabel->setPixmap(APPLICATION->logo().pixmap(42, 42));
    m_ui->studioIconLabel->setAlignment(Qt::AlignCenter);
    m_ui->fileBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #08bfe2; border: 1px solid #10d8f2; border-radius: 8px; color: #03111b; font-weight: 700; "
        "min-height: 34px; }"
        "QPushButton:hover { background: #35d8ef; }"));
    m_ui->previewPanel->setStyleSheet(QStringLiteral(
        "QFrame#previewPanel { background: #071426; border: 1px solid #1b3552; border-radius: 10px; }"
        "QFrame#previewPanel QLabel { background: transparent; border: 0; }"));
    m_ui->selectedVLayout->setSpacing(6);
    m_ui->previewTitleLabel->setStyleSheet(QStringLiteral("font-size: 11px; font-weight: 700; letter-spacing: 1px; color: #79a4d0;"));
    m_skinNameLabel = m_ui->skinNameLabel;
    m_skinNameLabel->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 600; color: #e8f3ff;"));
    m_ui->modelBox->setTitle(QString());
    m_ui->modelBox->setStyleSheet(QStringLiteral(
        "QGroupBox#modelBox { border: 0; margin: 0; padding: 0; }"
        "QRadioButton { background: #081629; border: 1px solid #243a56; border-radius: 8px; padding: 8px 10px; }"
        "QRadioButton:checked { border-color: #10c9ed; background: #063149; color: #43e2f6; }"));
    m_ui->capeBox->setTitle(tr("Cape"));
    m_ui->capeLayout->setSpacing(6);
    m_ui->capeCombo->setMinimumHeight(34);
    m_ui->elytraCB->setMinimumHeight(22);
    m_ui->capeBox->setMinimumHeight(90);
    m_ui->capeImage->hide();
    m_ui->catalogStatusLabel->setStyleSheet(QStringLiteral("color: #96abc5;"));
    m_ui->catalogStatusLabel->setWordWrap(false);
    m_ui->buttonBox->hide();
    setApplyEnabled(false);
    m_ui->buttonBox->setStyleSheet(QStringLiteral(
        "QPushButton { background: #08bfe2; border: 1px solid #10d8f2; border-radius: 8px; color: #03111b; font-size: 14px; "
        "font-weight: 700; min-height: 38px; }"
        "QPushButton:hover { background: #35d8ef; }"
        "QPushButton:disabled { background: #184255; border-color: #22536a; color: #7894a9; }"));
    m_ui->closeStudioBtn->setStyleSheet(QStringLiteral(
        "QToolButton { font-size: 25px; color: #dce9fb; border: 0; padding: 4px 10px; }"
        "QToolButton:hover { color: #18c9e8; }"));
    m_ui->applySkinBtn->setEnabled(false);
    m_ui->applySkinBtn->setMinimumHeight(40);
    m_ui->applySkinBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #08bfe2; border: 1px solid #10d8f2; border-radius: 8px; color: #03111b; font-size: 14px; "
        "font-weight: 700; min-height: 38px; }"
        "QPushButton:hover { background: #35d8ef; }"
        "QPushButton:disabled { background: #184255; border-color: #22536a; color: #7894a9; }"));
    connect(m_ui->applySkinBtn, &QPushButton::clicked, m_ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::click);
    m_ui->downloadBtn->setMinimumHeight(38);
    m_ui->resetBtn->setText(tr("Reset Account Skin"));
    m_ui->resetBtn->setMaximumHeight(30);
    m_ui->downloadBtn->setEnabled(false);
    setWindowState(Qt::WindowMaximized);
    for (auto* button : { m_ui->navPlayButton, m_ui->navInstancesButton, m_ui->navModsButton, m_ui->navAccountsButton,
                          m_ui->navAiHelpButton, m_ui->navNewsButton, m_ui->navDiscordButton }) {
        button->setCheckable(button == m_ui->navAccountsButton);
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    }
    m_ui->navAccountsButton->setChecked(true);
    connect(m_ui->closeStudioBtn, &QToolButton::clicked, this, &QDialog::reject);
    const QList<QPair<QToolButton*, QString>> navigationTargets = {
        { m_ui->navPlayButton, tr("Play") },       { m_ui->navInstancesButton, tr("Instances") },
        { m_ui->navModsButton, tr("Mods") },       { m_ui->navAccountsButton, tr("Accounts") },
        { m_ui->navAiHelpButton, tr("AI Help") },  { m_ui->navNewsButton, tr("News") },
        { m_ui->navDiscordButton, tr("Discord") },
    };
    for (const auto& [button, targetName] : navigationTargets) {
        for (auto* ancestor = parentWidget(); ancestor && button->icon().isNull(); ancestor = ancestor->parentWidget()) {
            const auto targets = ancestor->findChildren<QToolButton*>(QStringLiteral("boostNavButton"));
            for (auto* target : targets) {
                if (target->text() != targetName)
                    continue;
                button->setIcon(target->icon());
                button->setIconSize(target->iconSize());
                break;
            }
        }
        connect(button, &QToolButton::clicked, this, [this, targetName] {
            for (auto* ancestor = parentWidget(); ancestor; ancestor = ancestor->parentWidget()) {
                const auto targets = ancestor->findChildren<QToolButton*>(QStringLiteral("boostNavButton"));
                for (auto* target : targets) {
                    if (target->text() != targetName)
                        continue;
                    QPointer<QToolButton> guardedTarget(target);
                    QTimer::singleShot(0, target, [guardedTarget] {
                        if (guardedTarget)
                            guardedTarget->click();
                    });
                    reject();
                    return;
                }
            }
            if (targetName == tr("Accounts"))
                reject();
        });
    }

    if (SkinOpenGLWindow::hasOpenGL()) {
        m_skinPreview = new SkinOpenGLWindow(this, QColor(QStringLiteral("#100b1c")));
    } else {
        m_skinPreviewLabel = new QLabel(this);
        m_skinPreviewLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    setWindowModality(Qt::WindowModal);

    auto* contentsWidget = m_ui->listView;
    contentsWidget->installEventFilter(this);

    contentsWidget->setAcceptDrops(true);
    contentsWidget->setDropIndicatorShown(true);
    contentsWidget->viewport()->setAcceptDrops(true);
    contentsWidget->setDragDropMode(QAbstractItemView::DropOnly);
    contentsWidget->setDefaultDropAction(Qt::CopyAction);

    contentsWidget->installEventFilter(this);
    contentsWidget->setModel(&m_list);

    connect(contentsWidget, &QAbstractItemView::doubleClicked, this, &SkinManageDialog::activated);
    connect(&m_list, &SkinList::skinRenamed, this, [this](const QString& oldName, const QString& newName) {
        auto& accountSkin = m_acct->accountData()->minecraftProfile.skin;
        if (m_acct->accountType() == AccountType::Offline &&
            accountSkin.id == QStringLiteral("local:") + oldName) {
            m_acct->setLocalSkin(accountSkin.data, accountSkin.variant, newName);
        }
        for (auto& name : m_favoriteLocalSkins)
            if (name == oldName) name = newName;
        for (auto& name : m_recentLocalSkins)
            if (name == oldName) name = newName;
        if (m_selectedSkinKey == oldName)
            m_selectedSkinKey = newName;
        saveStudioState();
        updateLocalSkinFilter();
        renderCatalogCards();
    });

    connect(m_ui->listView, &QListView::customContextMenuRequested, this, &SkinManageDialog::show_context_menu);
    connect(m_ui->elytraCB, &QCheckBox::checkStateChanged, this, [this]() {
        if (m_skinPreview) {
            m_skinPreview->setElytraVisible(m_ui->elytraCB->isChecked());
        }
        on_capeCombo_currentIndexChanged(0);
    });

    setupCapes();

    loadStudioState();
    setupCatalogView();
    setupStudioTabs();
    connect(m_ui->listView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SkinManageDialog::selectionChanged);
    updateLocalSkinFilter();
    const auto selectedRow = m_list.getSelectedAccountSkin();
    if (selectedRow >= 0)
        m_ui->listView->setCurrentIndex(m_localFilterProxy->mapFromSource(m_list.index(selectedRow)));

    m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    auto* acceptButton = m_ui->buttonBox->button(QDialogButtonBox::Ok);
    if (m_acct->accountType() == AccountType::Offline) {
        acceptButton->setText(tr("Apply Skin"));
        m_ui->applySkinBtn->setText(tr("Apply Skin"));
        acceptButton->setToolTip(tr("Apply this skin locally to the selected offline account."));
        m_ui->applySkinBtn->setToolTip(tr("Apply this skin locally to the selected offline account."));
        m_ui->resetBtn->setEnabled(false);
        m_ui->resetBtn->setToolTip(tr("Skin reset is only available for authenticated Microsoft accounts."));
    } else {
        acceptButton->setText(tr("Apply Skin"));
        m_ui->applySkinBtn->setText(tr("Apply Skin"));
    }
    acceptButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: #08bfe2; border: 1px solid #10d8f2; color: #03111b; font-weight: 700; min-width: 118px; }"
        "QPushButton:hover { background: #35d8ef; }"));
    m_ui->downloadBtn->setStyleSheet(QStringLiteral(
        "QPushButton { border-color: #16b9dc; color: #dff9ff; font-weight: 600; min-width: 118px; }"));
    m_downloadButton = m_ui->downloadBtn;
    connect(m_downloadButton, &QPushButton::clicked, this, &SkinManageDialog::downloadSelectedSkin);

    if (m_skinPreview) {
        auto* previewContainer = QWidget::createWindowContainer(m_skinPreview, this);
        previewContainer->setMinimumHeight(140);
        previewContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_ui->selectedVLayout->setStretch(1, 1);
        m_ui->skinLayout->insertWidget(0, previewContainer, 1);
    } else {
        m_ui->skinLayout->insertWidget(0, m_skinPreviewLabel);
    }
}

SkinManageDialog::~SkinManageDialog()
{
    delete m_ui;

    delete m_skinPreview;
}

void SkinManageDialog::activated(QModelIndex index)
{
    m_selectedSkinKey = index.data(Qt::UserRole).toString();
    accept();
}

void SkinManageDialog::selectionChanged(const QItemSelection& selected, [[maybe_unused]] const QItemSelection& deselected)
{
    if (selected.empty()) {
        return;
    }

    QString key = selected.first().indexes().first().data(Qt::UserRole).toString();
    if (key.isEmpty()) {
        return;
    }
    m_selectedSkinKey = key;
    m_pendingCatalogUuid.clear();
    m_pendingCatalogName.clear();
    m_pendingCatalogVariant.clear();
    m_pendingCatalogData.clear();
    m_catalogPreviewSkin.reset();
    auto* skin = getSelectedSkin();
    if (!skin) {
        return;
    }
    setApplyEnabled(true);
    m_ui->downloadBtn->setEnabled(true);
    m_skinNameLabel->setText(skin->name());

    if (m_skinPreview) {
        m_skinPreview->updateScene(skin);
    } else {
        m_skinPreviewLabel->setPixmap(
            QPixmap::fromImage(skinCardPreview(*skin)).scaled(m_skinPreviewLabel->size(), Qt::KeepAspectRatio, Qt::FastTransformation));
    }
    m_ui->capeCombo->setCurrentIndex(m_capesIdx.value(skin->getCapeId()));
    m_ui->steveBtn->setChecked(skin->getModel() == SkinModel::CLASSIC);
    m_ui->alexBtn->setChecked(skin->getModel() == SkinModel::SLIM);
}

void SkinManageDialog::delayed_scroll(QModelIndex modelIndex)
{
    auto* contentsWidget = m_ui->listView;
    contentsWidget->scrollTo(modelIndex);
}

void SkinManageDialog::on_openDirBtn_clicked()
{
    DesktopServices::openPath(m_list.getDir(), true);
}

void SkinManageDialog::on_fileBtn_clicked()
{
    const auto filter = QMimeDatabase().mimeTypeForName("image/png").filterString();
    const auto path = QFileDialog::getOpenFileName(
        this, tr("Import Minecraft Skin or Skin Pack"), QString(),
        tr("Minecraft skin PNG (%1);;Minecraft skin pack ZIP (*.zip)").arg(filter));
    if (path.isEmpty())
        return;

    if (QFileInfo(path).suffix().compare(QStringLiteral("zip"), Qt::CaseInsensitive) == 0) {
        if (!loadBundledSkinCatalog() || !importSkinArchive(path)) {
            CustomMessageBox::selectable(
                this, tr("Could not import skin pack"),
                tr("The archive could not be indexed or contained no valid Minecraft skin PNGs. Check the launcher log for details."),
                QMessageBox::Warning)
                ->show();
            return;
        }
        m_bundledCatalogReady = true;
        setStudioTab(QStringLiteral("free"));
        showBundledSkinPage(0);
        m_catalogStatus->setText(tr("Skin pack indexed. Valid PNGs are cached for future launches."));
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() <= 0 || file.size() > MAX_SKIN_API_RESPONSE_SIZE) {
        CustomMessageBox::selectable(this, tr("Invalid skin PNG"),
                                     tr("Choose a readable PNG skin no larger than 1 MiB."), QMessageBox::Warning)->show();
        return;
    }
    const auto bytes = file.readAll();
    QImage importedImage;
    if (!isValidSkinPng(bytes, &importedImage)) {
        CustomMessageBox::selectable(this, tr("Invalid skin PNG"),
                                     tr("Skin PNGs must decode correctly and be exactly 64×64 or 64×32 pixels."),
                                     QMessageBox::Warning)
            ->show();
        return;
    }
    QString installedName;
    const auto model = m_ui->alexBtn->isChecked() ? SkinModel::SLIM : SkinModel::CLASSIC;
    const auto error = m_list.installSkinData(bytes, QFileInfo(path).completeBaseName(), model, &installedName);
    if (!error.isEmpty()) {
        CustomMessageBox::selectable(this, tr("Could not import skin"), error, QMessageBox::Warning)->show();
        return;
    }
    m_pendingCatalogData.clear();
    m_catalogPreviewSkin.reset();
    m_selectedSkinKey = installedName;
    setApplyEnabled(true);
    m_ui->downloadBtn->setEnabled(true);
    setStudioTab(QStringLiteral("my"));
    const auto sourceIndex = m_list.index(m_list.getSkinIndex(installedName));
    m_ui->listView->setCurrentIndex(m_localFilterProxy->mapFromSource(sourceIndex));
    m_catalogStatus->setText(importedImage.hasAlphaChannel()
                                 ? tr("%1 imported. PNG transparency was preserved.").arg(installedName)
                                 : tr("%1 imported as an opaque PNG skin.").arg(installedName));
}
namespace {
QPixmap previewCape(const QImage& capeImage, bool elytra = false)
{
    if (elytra) {
        auto wing = capeImage.copy(34, 2, 12, 20);
        QImage mirrored = wing.mirrored(true, false);

        QImage combined((wing.width() * 2) + 1, wing.height() + 14, capeImage.format());
        combined.fill(Qt::transparent);

        QPainter painter(&combined);
        painter.drawImage(0, 7, wing);
        painter.drawImage(wing.width() + 1, 7, mirrored);
        painter.end();
        return QPixmap::fromImage(combined.scaled(84, 128, Qt::KeepAspectRatio, Qt::FastTransformation));
    }
    return QPixmap::fromImage(capeImage.copy(1, 1, 10, 16).scaled(80, 128, Qt::IgnoreAspectRatio, Qt::FastTransformation));
}
}  // namespace

void SkinManageDialog::setupCapes()
{
    // FIXME: add a model for this, download/refresh the capes on demand
    auto& accountData = *m_acct->accountData();
    int index = 0;
    m_ui->capeCombo->addItem(tr("No Cape"), QVariant());
    auto currentCape = accountData.minecraftProfile.currentCape;
    if (currentCape.isEmpty()) {
        m_ui->capeCombo->setCurrentIndex(index);
    }

    auto capesDir = FS::PathCombine(m_list.getDir(), "capes");
    NetJob::Ptr job{ new NetJob(tr("Download capes"), APPLICATION->network()) };
    bool needsToDownload = false;
    for (auto& cape : accountData.minecraftProfile.capes) {
        auto path = FS::PathCombine(capesDir, cape.id + ".png");
        if (!cape.data.isEmpty()) {
            QImage capeImage;
            if (capeImage.loadFromData(cape.data, "PNG") && capeImage.save(path)) {
                m_capes[cape.id] = capeImage;
                continue;
            }
        }
        if (QFileInfo(path).exists()) {
            continue;
        }
        if (!cape.url.isEmpty()) {
            needsToDownload = true;
            job->addNetAction(Net::Request::makeFile(cape.url, path));
        }
    }
    if (needsToDownload) {
        ProgressDialog dlg(this);
        dlg.execWithTask(job.get());
    }
    for (auto& cape : accountData.minecraftProfile.capes) {
        index++;
        QImage capeImage;
        if (!m_capes.contains(cape.id)) {
            auto path = FS::PathCombine(capesDir, cape.id + ".png");
            if (QFileInfo(path).exists() && capeImage.load(path)) {
                m_capes[cape.id] = capeImage;
            }
        }
        if (!capeImage.isNull()) {
            m_ui->capeCombo->addItem(previewCape(capeImage, m_ui->elytraCB->isChecked()), cape.alias, cape.id);
        } else {
            m_ui->capeCombo->addItem(cape.alias, cape.id);
        }

        m_capesIdx[cape.id] = index;
    }
}

void SkinManageDialog::on_capeCombo_currentIndexChanged(int /*index*/)
{
    auto id = m_ui->capeCombo->currentData();
    auto cape = m_capes.value(id.toString(), {});
    if (!cape.isNull()) {
        m_ui->capeImage->setPixmap(
            previewCape(cape, m_ui->elytraCB->isChecked()).scaled(size() * (1. / 3), Qt::KeepAspectRatio, Qt::FastTransformation));
    } else {
        m_ui->capeImage->clear();
    }
    if (m_skinPreview) {
        m_skinPreview->updateCape(cape);
    }
    if (auto* skin = getSelectedSkin(); skin) {
        skin->setCapeId(id.toString());
        if (m_skinPreview) {
            m_skinPreview->updateScene(skin);
        } else {
            m_skinPreviewLabel->setPixmap(
                QPixmap::fromImage(skinCardPreview(*skin)).scaled(m_skinPreviewLabel->size(), Qt::KeepAspectRatio, Qt::FastTransformation));
        }
    }
}

void SkinManageDialog::on_steveBtn_toggled(bool checked)
{
    if (auto* skin = getSelectedSkin(); skin) {
        skin->setModel(checked ? SkinModel::CLASSIC : SkinModel::SLIM);
        if (!m_pendingCatalogData.isEmpty())
            m_pendingCatalogVariant = checked ? QStringLiteral("classic") : QStringLiteral("slim");
        if (m_skinPreview) {
            m_skinPreview->updateScene(skin);
        } else {
            m_skinPreviewLabel->setPixmap(
                QPixmap::fromImage(skinCardPreview(*skin)).scaled(m_skinPreviewLabel->size(), Qt::KeepAspectRatio, Qt::FastTransformation));
        }
    }
}

void SkinManageDialog::accept()
{
    if (!m_pendingCatalogData.isEmpty()) {
        const auto selectedCape = m_catalogPreviewSkin ? m_catalogPreviewSkin->getCapeId() : QString();
        QString installedName;
        const auto installName = m_pendingCatalogUuid.size() == 64
                                     ? m_pendingCatalogName
                                     : tr("MineSkin %1").arg(m_pendingCatalogUuid.left(8));
        const auto error = m_list.installSkinData(m_pendingCatalogData, installName,
                                                   m_pendingCatalogVariant.compare(QStringLiteral("slim"), Qt::CaseInsensitive) == 0
                                                       ? SkinModel::SLIM
                                                       : SkinModel::CLASSIC,
                                                   &installedName);
        if (!error.isEmpty()) {
            CustomMessageBox::selectable(this, tr("Could not add skin to library"), error, QMessageBox::Warning)->exec();
            return;
        }
        m_selectedSkinKey = installedName;
        if (auto* installedSkin = m_list.skin(installedName)) {
            installedSkin->setCapeId(selectedCape);
            m_list.save();
        }
        setApplyEnabled(true);
        m_ui->downloadBtn->setEnabled(true);
        m_pendingCatalogData.clear();
        m_catalogPreviewSkin.reset();
        m_localFilterProxy->invalidate();
        const auto sourceIndex = m_list.index(m_list.getSkinIndex(installedName));
        m_ui->listView->setCurrentIndex(m_localFilterProxy->mapFromSource(sourceIndex));
    }
    auto* skin = m_list.skin(m_selectedSkinKey);
    if (!skin) {
        m_catalogStatus->setText(tr("Select a skin before applying."));
        return;
    }
    if (m_acct->accountType() == AccountType::Offline) {
        QFile skinFile(skin->getPath());
        if (!skinFile.open(QIODevice::ReadOnly)) {
            CustomMessageBox::selectable(this, tr("Skin Apply"), tr("Could not read the selected skin: %1").arg(skinFile.errorString()),
                                         QMessageBox::Warning)
                ->exec();
            return;
        }
        const auto skinData = skinFile.readAll();
        if (!m_acct->setLocalSkin(skinData, skin->getModelString(), skin->name())) {
            CustomMessageBox::selectable(this, tr("Skin Apply"),
                                         tr("Could not apply this skin to the offline account. Please select a valid skin and try again."),
                                         QMessageBox::Warning)
                ->exec();
            return;
        }
        m_recentLocalSkins.removeAll(skin->name());
        m_recentLocalSkins.prepend(skin->name());
        while (m_recentLocalSkins.size() > 20)
            m_recentLocalSkins.removeLast();
        saveStudioState();
        updateLocalSkinFilter();
        CustomMessageBox::selectable(this, tr("Skin Applied"),
                                     tr("The skin was applied to this offline account and saved locally. It was not uploaded to Minecraft services."),
                                     QMessageBox::Information)
            ->exec();
        QDialog::accept();
        return;
    }
    auto path = skin->getPath();

    ProgressDialog prog(this);
    NetJob::Ptr skinUpload{ new NetJob(tr("Change skin"), APPLICATION->network(), 1) };

    if (!QFile::exists(path)) {
        CustomMessageBox::selectable(this, tr("Skin Upload"), tr("Skin file does not exist!"), QMessageBox::Warning)->exec();
        reject();
        return;
    }

    skinUpload->addNetAction(makeSkinUploadRequest(m_acct->accessToken(), skin->getPath(), skin->getModelString()));

    auto selectedCape = skin->getCapeId();
    if (selectedCape != m_acct->accountData()->minecraftProfile.currentCape) {
        skinUpload->addNetAction(makeCapeChangeRequest(m_acct->accessToken(), selectedCape));
    }

    skinUpload->addTask(m_acct->refresh().staticCast<Task>());
    if (prog.execWithTask(skinUpload.get()) != QDialog::Accepted) {
        CustomMessageBox::selectable(this, tr("Skin Upload"), tr("Failed to upload skin!"), QMessageBox::Warning)->exec();
        reject();
        return;
    }
    skin->setURL(m_acct->accountData()->minecraftProfile.skin.url);
    m_recentLocalSkins.removeAll(skin->name());
    m_recentLocalSkins.prepend(skin->name());
    while (m_recentLocalSkins.size() > 20)
        m_recentLocalSkins.removeLast();
    saveStudioState();
    updateLocalSkinFilter();
    QDialog::accept();
}

void SkinManageDialog::on_resetBtn_clicked()
{
    ProgressDialog prog(this);
    NetJob::Ptr skinReset{ new NetJob(tr("Reset skin"), APPLICATION->network(), 1) };
    skinReset->addNetAction(makeSkinDeleteRequest(m_acct->accessToken()));
    skinReset->addTask(m_acct->refresh().staticCast<Task>());
    if (prog.execWithTask(skinReset.get()) != QDialog::Accepted) {
        CustomMessageBox::selectable(this, tr("Skin Delete"), tr("Failed to delete current skin!"), QMessageBox::Warning)->exec();
        reject();
        return;
    }
    QDialog::accept();
}

void SkinManageDialog::show_context_menu(const QPoint& pos)
{
    QMenu myMenu(tr("Context menu"), this);
    myMenu.addAction(m_ui->action_Rename_Skin);
    myMenu.addAction(m_ui->action_Delete_Skin);

    myMenu.exec(m_ui->listView->mapToGlobal(pos));
}

bool SkinManageDialog::eventFilter(QObject* obj, QEvent* ev)
{
    if (obj == m_ui->listView) {
        if (ev->type() == QEvent::KeyPress) {
            auto* keyEvent = static_cast<QKeyEvent*>(ev);
            switch (keyEvent->key()) {
                case Qt::Key_Delete:
                    on_action_Delete_Skin_triggered(false);
                    return true;
                case Qt::Key_F2:
                    on_action_Rename_Skin_triggered(false);
                    return true;
                default:
                    break;
            }
        }
    }
    if (m_catalogScrollArea && obj == m_catalogScrollArea->viewport() && ev->type() == QEvent::Resize) {
        if (m_catalogLoadingOverlay) {
            m_catalogLoadingOverlay->setGeometry(m_catalogScrollArea->viewport()->rect());
            m_catalogLoadingOverlay->raise();
        }
        const auto cardWidth = qBound(160, (m_catalogScrollArea->viewport()->width() - 32) / BUNDLED_SKIN_COLUMNS, 260);
        const auto cardHeight = qBound(180, (m_catalogScrollArea->viewport()->height() - 20) / 2, 230);
        if (cardWidth != m_catalogCardWidth || cardHeight != m_catalogCardHeight) {
            m_catalogColumns = BUNDLED_SKIN_COLUMNS;
            m_catalogCardWidth = cardWidth;
            m_catalogCardHeight = cardHeight;
            QTimer::singleShot(0, this, [this] { renderCatalogCards(); });
        }
    }
    return QDialog::eventFilter(obj, ev);
}

void SkinManageDialog::on_action_Rename_Skin_triggered(bool /*unused*/)
{
    if (!m_selectedSkinKey.isEmpty()) {
        m_ui->listView->edit(m_ui->listView->currentIndex());
    }
}

void SkinManageDialog::on_action_Delete_Skin_triggered(bool /*unused*/)
{
    if (m_selectedSkinKey.isEmpty()) {
        return;
    }

    if (m_list.getSkinIndex(m_selectedSkinKey) == m_list.getSelectedAccountSkin()) {
        CustomMessageBox::selectable(this, tr("Delete error"), tr("Can not delete skin that is in use."), QMessageBox::Warning)->exec();
        return;
    }

    auto* skin = m_list.skin(m_selectedSkinKey);
    if (!skin) {
        return;
    }

    auto response = CustomMessageBox::selectable(this, tr("Confirm Deletion"),
                                                 tr("You are about to delete \"%1\".\n"
                                                    "Are you sure?")
                                                     .arg(skin->name()),
                                                 QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                        ->exec();

    if (response == QMessageBox::Yes) {
        const auto deleted = m_list.deleteSkin(m_selectedSkinKey, true) || m_list.deleteSkin(m_selectedSkinKey, false);
        if (!deleted)
            return;
        m_favoriteLocalSkins.removeAll(m_selectedSkinKey);
        m_recentLocalSkins.removeAll(m_selectedSkinKey);
        m_selectedSkinKey.clear();
        saveStudioState();
        updateLocalSkinFilter();
        renderCatalogCards();
    }
}

void SkinManageDialog::on_urlBtn_clicked()
{
    auto url = QUrl(m_ui->urlLine->text());
    if (!url.isValid()) {
        CustomMessageBox::selectable(this, tr("Invalid url"), tr("Invalid url"), QMessageBox::Critical)->show();
        return;
    }

    NetJob::Ptr job{ new NetJob(tr("Download skin"), APPLICATION->network()) };
    job->setAskRetry(false);

    auto path = FS::PathCombine(m_list.getDir(), url.fileName());
    job->addNetAction(Net::Request::makeFile(url, path));
    ProgressDialog dlg(this);
    dlg.execWithTask(job.get());
    SkinModel s(path);
    if (!s.isValid()) {
        CustomMessageBox::selectable(this, tr("URL is not a valid skin"),
                                     QFileInfo::exists(path) ? tr("Skin images must be 64x64 or 64x32 pixel PNG files.")
                                                             : tr("Unable to download the skin: '%1'.").arg(m_ui->urlLine->text()),
                                     QMessageBox::Critical)
            ->show();
        QFile::remove(path);
        return;
    }
    m_ui->urlLine->setText("");
    if (QFileInfo(path).suffix().isEmpty()) {
        QFile::rename(path, path + ".png");
    }
}

void SkinManageDialog::on_userBtn_clicked()
{
    auto user = m_ui->urlLine->text();
    if (user.isEmpty()) {
        return;
    }
    MinecraftProfile mcProfile;
    auto path = FS::PathCombine(m_list.getDir(), user + ".png");

    NetJob::Ptr job{ new NetJob(tr("Download user skin"), APPLICATION->network(), 1) };
    job->setAskRetry(false);

    auto downloadSkin = Net::Request::makeFile(QUrl(), path);
    // use a valid url to trick the make to actually create the task
    // the actual url will be set in the getUUID parse function
    auto [getProfile, a] = Net::RPC::make<bool>({ { .url = { "https://sessionserver.mojang.com/session/minecraft/profile/" } },
                                                  [downloadSkin, &mcProfile](const auto& v) -> Result<bool> {
                                                      TRY(Parsers::parseMinecraftProfileMojang(v, mcProfile))
                                                      downloadSkin->setUrl(mcProfile.skin.url);
                                                      return {};
                                                  } });
    auto [getUUID, b] = Net::RPC::make<bool>({ { .url = "https://api.minecraftservices.com/minecraft/profile/lookup/name/" + user },
                                               [&getProfile](const auto& v) -> Result<bool> {
                                                   TRY_INTO(auto root, Json::requireObject(v, "Minecraft profile lookup"))
                                                   auto id = root["id"].toString();
                                                   if (id.isEmpty()) {
                                                       return std::unexpected(tr("user id is empty"));
                                                   }
                                                   getProfile->setUrl("https://sessionserver.mojang.com/session/minecraft/profile/" + id);
                                                   return {};
                                               } });

    QString failReason;

    connect(getUUID.get(), &Task::failed, this, [&failReason](const QString& reason) {
        qCritical() << "Couldn't get user UUID:" << reason;
        failReason = tr("failed to get user UUID");
    });
    connect(getProfile.get(), &Task::failed, this, [&failReason](const QString& reason) {
        qCritical() << "Couldn't get user profile:" << reason;
        failReason = tr("failed to get user profile");
    });
    connect(downloadSkin.get(), &Task::failed, this, [&failReason](const QString& reason) {
        qCritical() << "Couldn't download skin:" << reason;
        failReason = tr("failed to download skin");
    });

    job->addNetAction(getUUID);
    job->addNetAction(getProfile);
    job->addNetAction(downloadSkin);
    ProgressDialog dlg(this);
    dlg.execWithTask(job.get());

    SkinModel s(path);
    if (!s.isValid()) {
        if (failReason.isEmpty()) {
            failReason = tr("the skin is invalid");
        }
        CustomMessageBox::selectable(this, tr("Username not found"),
                                     tr("Unable to find the skin for '%1'\n because: %2.").arg(user, failReason), QMessageBox::Critical)
            ->show();
        QFile::remove(path);
        return;
    }
    m_ui->urlLine->setText("");
    s.setModel(mcProfile.skin.variant.toUpper() == "SLIM" ? SkinModel::SLIM : SkinModel::CLASSIC);
    s.setURL(mcProfile.skin.url);
    if (m_capes.contains(mcProfile.currentCape)) {
        s.setCapeId(mcProfile.currentCape);
    }
    m_list.updateSkin(&s);
}

void SkinManageDialog::on_browseSkinsBtn_clicked()
{
    auto baseUrl = QUrl(APPLICATION->settings()->get("SkinApiBaseUrl").toString().trimmed());
    if (baseUrl.isEmpty() || baseUrl.host().compare(QStringLiteral("mccatalog.com"), Qt::CaseInsensitive) == 0) {
        bool accepted = false;
        const auto origin = QInputDialog::getText(
                                this, tr("Connect Skin API"), tr("Your Vercel HTTPS origin (no path or API key):"),
                                QLineEdit::Normal, m_catalogApiUrl->text(), &accepted)
                                .trimmed();
        if (!accepted || origin.isEmpty()) return;
        baseUrl = QUrl(origin);
    }

    const bool localHttp = baseUrl.scheme() == "http" &&
                           (baseUrl.host() == "localhost" || baseUrl.host() == "127.0.0.1" || baseUrl.host() == "::1");
    if ((!localHttp && baseUrl.scheme() != "https") || baseUrl.host().isEmpty() || !baseUrl.userInfo().isEmpty() ||
        (!baseUrl.path().isEmpty() && baseUrl.path() != "/") || !baseUrl.query().isEmpty() || !baseUrl.fragment().isEmpty()) {
        CustomMessageBox::selectable(this, tr("Invalid Skin API URL"),
                                     tr("Use an HTTPS origin without a path, query, or credentials. HTTP is allowed only for localhost."),
                                     QMessageBox::Warning)
            ->show();
        return;
    }
    baseUrl.setPath(QString());
    APPLICATION->settings()->set("SkinApiBaseUrl", baseUrl.toString());
    m_catalogApiUrl->setText(baseUrl.toString());

    bool accepted = false;
    const auto queryText = QInputDialog::getText(
        this, tr("Online Skin Lookup"), tr("Minecraft username or UUID:"), QLineEdit::Normal, m_ui->urlLine->text(), &accepted).trimmed();
    if (!accepted || queryText.isEmpty())
        return;

    const QRegularExpression uuidPattern(QStringLiteral("^[0-9a-fA-F]{8}-?[0-9a-fA-F]{4}-?[0-9a-fA-F]{4}-?[0-9a-fA-F]{4}-?[0-9a-fA-F]{12}$"));
    const QRegularExpression usernamePattern(QStringLiteral("^[A-Za-z0-9_]{3,16}$"));
    const bool isUuid = uuidPattern.match(queryText).hasMatch();
    if (!isUuid && !usernamePattern.match(queryText).hasMatch()) {
        CustomMessageBox::selectable(this, tr("Invalid lookup"), tr("Enter a valid Minecraft username or UUID."), QMessageBox::Warning)->show();
        return;
    }

    QUrl lookupUrl = baseUrl;
    lookupUrl.setPath("/api/skins/search");
    QUrlQuery lookupQuery;
    lookupQuery.addQueryItem(isUuid ? "uuid" : "username", isUuid ? queryText.toLower() : queryText);
    lookupUrl.setQuery(lookupQuery);
    QString error;
    QString contentType;
    const auto profileBytes = skinApiGet(lookupUrl, error, &contentType);
    if (!error.isEmpty() || contentType != "application/json") {
        CustomMessageBox::selectable(this, tr("Online Skin Lookup"),
                                     error.isEmpty() ? tr("The skin API returned an unexpected response.") : error, QMessageBox::Warning)
            ->show();
        return;
    }

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(profileBytes, &parseError);
    QJsonObject profile;
    if (parseError.error == QJsonParseError::NoError) {
        const auto profiles = document.object().value("skins").toArray();
        if (!profiles.isEmpty())
            profile = profiles.first().toObject();
    }
    if (profile.isEmpty()) {
        CustomMessageBox::selectable(this, tr("Online Skin Lookup"), tr("No skin was found for that username or UUID."),
                                     QMessageBox::Information)
            ->show();
        return;
    }
    const auto username = profile.value("username").toString();
    auto uuid = profile.value("uuid").toString().remove('-').toLower();
    if (!usernamePattern.match(username).hasMatch() || !QRegularExpression(QStringLiteral("^[0-9a-f]{32}$")).match(uuid).hasMatch()) {
        CustomMessageBox::selectable(this, tr("Online Skin Lookup"), tr("The skin API returned an invalid profile."),
                                     QMessageBox::Warning)
            ->show();
        return;
    }

    QUrl skinUrl = baseUrl;
    skinUrl.setPath("/api/skins/skin");
    QUrlQuery skinQuery;
    skinQuery.addQueryItem("uuid", uuid);
    skinUrl.setQuery(skinQuery);
    error.clear();
    contentType.clear();
    const auto skinBytes = skinApiGet(skinUrl, error, &contentType);
    if (!error.isEmpty() || contentType != "image/png") {
        CustomMessageBox::selectable(this, tr("Online Skin Lookup"),
                                     error.isEmpty() ? tr("The skin API did not return a PNG image.") : error, QMessageBox::Warning)
            ->show();
        return;
    }

    QString installedName;
    const auto variant = profile.value("variant").toString().compare("slim", Qt::CaseInsensitive) == 0
                             ? SkinModel::SLIM
                             : SkinModel::CLASSIC;
    error = m_list.installSkinData(skinBytes, username, variant, &installedName);
    if (!error.isEmpty()) {
        CustomMessageBox::selectable(this, tr("Online Skin Lookup"), error, QMessageBox::Warning)->show();
        return;
    }

    m_selectedSkinKey = installedName;
    setStudioTab(QStringLiteral("my"));
    const auto row = m_list.getSkinIndex(installedName);
    if (row >= 0)
        m_ui->listView->setCurrentIndex(m_localFilterProxy->mapFromSource(m_list.index(row)));
    m_ui->urlLine->clear();
}

void SkinManageDialog::on_browseCatalogBtn_clicked()
{
    setStudioTab(QStringLiteral("free"));
}

void SkinManageDialog::setupCatalogView()
{
    m_studioStatePath = QDir(m_list.getDir()).filePath(QStringLiteral("skin-studio.json"));
    m_catalogCacheDir = QDir(m_list.getDir()).filePath(QStringLiteral("catalog-cache"));
    if (!QDir().mkpath(m_catalogCacheDir))
        qWarning() << "Unable to create skin catalog cache directory:" << m_catalogCacheDir;

    auto* proxy = new SkinLibraryProxy(this);
    proxy->setSourceModel(&m_list);
    m_localFilterProxy = proxy;
    m_ui->listView->setModel(m_localFilterProxy);
    m_ui->listView->setViewMode(QListView::IconMode);
    m_ui->listView->setFlow(QListView::LeftToRight);
    m_ui->listView->setWrapping(true);
    m_ui->listView->setResizeMode(QListView::Adjust);
    m_ui->listView->setMovement(QListView::Static);
    m_ui->listView->setSpacing(10);
    m_ui->listView->setIconSize(QSize(104, 112));
    m_ui->listView->setGridSize(QSize(158, 168));
    m_ui->listView->setUniformItemSizes(true);
    m_ui->listView->setMouseTracking(true);
    m_ui->listView->setItemDelegate(new SkinLibraryDelegate(
        [this](const QString& name) { return m_favoriteLocalSkins.contains(name); },
        [this](const QString& name) {
            if (name.isEmpty()) return;
            if (m_favoriteLocalSkins.contains(name))
                m_favoriteLocalSkins.removeAll(name);
            else
                m_favoriteLocalSkins.append(name);
            saveStudioState();
            updateLocalSkinFilter();
            renderCatalogCards();
        },
        m_ui->listView));

    m_skinViews = new QStackedWidget(this);
    auto* oldItem = m_ui->mainHlLayout->replaceWidget(m_ui->listView, m_skinViews);
    delete oldItem;
    m_skinViews->addWidget(m_ui->listView);

    m_catalogPage = new QWidget(m_skinViews);
    auto* pageLayout = new QVBoxLayout(m_catalogPage);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    m_catalogApiRow = new QWidget(this);
    auto* toolbar = new QHBoxLayout(m_catalogApiRow);
    toolbar->setContentsMargins(0, 0, 0, 0);
    m_catalogApiUrl = new QLineEdit(m_catalogPage);
    m_catalogApiUrl->setObjectName(QStringLiteral("catalogApiUrl"));
    m_catalogApiUrl->setPlaceholderText(tr("Vercel API origin · https://your-project.vercel.app"));
    const auto configuredApi = QUrl(APPLICATION->settings()->get("SkinApiBaseUrl").toString().trimmed());
    if (configuredApi.host().compare(QStringLiteral("mccatalog.com"), Qt::CaseInsensitive) != 0)
        m_catalogApiUrl->setText(configuredApi.toString(QUrl::RemovePath | QUrl::RemoveQuery | QUrl::RemoveFragment));
    m_catalogConnectButton = new QPushButton(tr("Connect"), m_catalogPage);
    m_catalogConnectButton->setObjectName(QStringLiteral("catalogConnectButton"));
    toolbar->addWidget(m_catalogApiUrl, 1);
    toolbar->addWidget(m_catalogConnectButton);
    m_ui->catalogApiLayout->addWidget(m_catalogApiRow);

    m_catalogSearch = new QLineEdit(m_catalogPage);
    m_catalogSearch->setObjectName(QStringLiteral("catalogSearch"));
    m_catalogSearch->setPlaceholderText(tr("Search skins by name or ID"));
    m_catalogSort = new QComboBox(m_catalogPage);
    m_catalogSort->setObjectName(QStringLiteral("catalogSort"));
    m_catalogSort->addItem(tr("Newest"), QStringLiteral("newest"));
    m_catalogSort->addItem(tr("Most viewed"), QStringLiteral("views"));
    m_ui->catalogSearchLayout->addWidget(m_catalogSearch, 1);
    m_ui->catalogSearchLayout->addWidget(m_catalogSort);
    m_catalogSort->hide();
    m_catalogApiRow->hide();

    const QStringList categories = { "All", "Popular", "Anime", "PvP", "Medieval", "Cute", "Mobs", "Fantasy" };
    for (const auto& category : categories) {
        auto* button = new QToolButton(this);
        button->setText(category);
        button->setObjectName(QStringLiteral("skinCategory_%1").arg(category.toLower()));
        button->setCheckable(true);
        button->setAutoRaise(true);
        button->setToolTip(category == QStringLiteral("Popular")
                               ? tr("Only skins with real popularity data can appear here.")
                               : tr("Category filters use the folders and metadata in the loaded skin packs."));
        button->setStyleSheet(QStringLiteral(
            "QToolButton { background: #0a192b; border: 1px solid #243b57; border-radius: 14px; padding: 6px 12px; color: #b8cce5; }"
            "QToolButton:hover { border-color: #13c8e8; color: #e7faff; }"
            "QToolButton:checked { background: #0caccb; border-color: #12d4ee; color: #04131d; font-weight: 700; }"));
        m_catalogCategoryButtons.append(button);
        m_ui->catalogCategoryLayout->addWidget(button);
        connect(button, &QToolButton::clicked, this, [this, category] {
            m_catalogCategory = category;
            m_bundledPage = 0;
            m_catalogScrollArea->verticalScrollBar()->setValue(0);
            for (auto* item : m_catalogCategoryButtons)
                item->setChecked(item->text() == category);
            renderCatalogCards();
        });
    }
    m_catalogCategoryButtons.first()->setChecked(true);

    auto* scrollArea = new QScrollArea(m_catalogPage);
    m_catalogScrollArea = scrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_catalogLoadingOverlay = new QWidget(scrollArea->viewport());
    m_catalogLoadingOverlay->setObjectName(QStringLiteral("catalogLoadingOverlay"));
    m_catalogLoadingOverlay->setStyleSheet(QStringLiteral(
        "QWidget#catalogLoadingOverlay { background: rgba(5, 15, 28, 245); border: 1px solid #1b3552; border-radius: 10px; }"
        "QLabel { color: #dce9fb; font-size: 14px; font-weight: 600; }"
        "QProgressBar { background: #0a192b; border: 1px solid #243b57; border-radius: 4px; }"
        "QProgressBar::chunk { background: #12c8e8; border-radius: 3px; }"));
    auto* loadingLayout = new QVBoxLayout(m_catalogLoadingOverlay);
    loadingLayout->setContentsMargins(24, 24, 24, 24);
    loadingLayout->addStretch(1);
    m_catalogLoadingLabel = new QLabel(m_catalogLoadingOverlay);
    m_catalogLoadingLabel->setObjectName(QStringLiteral("catalogLoadingLabel"));
    m_catalogLoadingLabel->setAlignment(Qt::AlignCenter);
    loadingLayout->addWidget(m_catalogLoadingLabel);
    m_catalogLoadingProgress = new QProgressBar(m_catalogLoadingOverlay);
    m_catalogLoadingProgress->setObjectName(QStringLiteral("catalogLoadingProgress"));
    m_catalogLoadingProgress->setRange(0, 0);
    m_catalogLoadingProgress->setTextVisible(false);
    m_catalogLoadingProgress->setFixedHeight(8);
    loadingLayout->addWidget(m_catalogLoadingProgress);
    m_catalogLoadingRetryButton = new QPushButton(tr("Retry"), m_catalogLoadingOverlay);
    m_catalogLoadingRetryButton->setObjectName(QStringLiteral("catalogLoadingRetryButton"));
    m_catalogLoadingRetryButton->hide();
    loadingLayout->addWidget(m_catalogLoadingRetryButton, 0, Qt::AlignHCenter);
    loadingLayout->addStretch(1);
    m_catalogLoadingOverlay->hide();
    connect(m_catalogLoadingRetryButton, &QPushButton::clicked, this, [this] {
        setStudioTab(QStringLiteral("free"));
    });
    auto* catalogContents = new QWidget(scrollArea);
    m_catalogGrid = new QGridLayout(catalogContents);
    m_catalogGrid->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_catalogGrid->setContentsMargins(4, 4, 4, 4);
    m_catalogGrid->setHorizontalSpacing(10);
    m_catalogGrid->setVerticalSpacing(8);
    scrollArea->setWidget(catalogContents);
    pageLayout->addWidget(scrollArea, 1);
    m_catalogScrollSnapTimer = new QTimer(this);
    m_catalogScrollSnapTimer->setSingleShot(true);
    m_catalogScrollSnapTimer->setInterval(140);
    auto* catalogScrollBar = scrollArea->verticalScrollBar();
    connect(catalogScrollBar, &QScrollBar::sliderPressed, this, [this] {
        m_catalogScrollDragging = true;
        m_catalogScrollSnapTimer->stop();
    });
    connect(catalogScrollBar, &QScrollBar::sliderReleased, this, [this] {
        m_catalogScrollDragging = false;
        snapCatalogScrollPosition();
    });
    connect(catalogScrollBar, &QScrollBar::valueChanged, this, [this] {
        if (!m_catalogScrollDragging)
            m_catalogScrollSnapTimer->start();
    });
    connect(m_catalogScrollSnapTimer, &QTimer::timeout, this, &SkinManageDialog::snapCatalogScrollPosition);

    m_catalogPreviousButton = new QPushButton(tr("Previous"), m_catalogPage);
    m_catalogNextButton = new QPushButton(tr("Next"), m_catalogPage);
    m_catalogPreviousButton->setEnabled(false);
    m_catalogNextButton->setEnabled(false);
    m_ui->catalogCategoryLayout->addStretch(1);
    m_ui->catalogPaginationLayout->addWidget(m_catalogPreviousButton);
    m_ui->catalogPaginationLayout->addWidget(m_catalogNextButton);
    m_catalogStatus = m_ui->catalogStatusLabel;
    m_skinViews->addWidget(m_catalogPage);

    connect(m_catalogConnectButton, &QPushButton::clicked, this, [this] {
        const auto baseUrl = QUrl(m_catalogApiUrl->text().trimmed());
        if (!baseUrl.isValid() || baseUrl.scheme() != QStringLiteral("https") || baseUrl.host().isEmpty() ||
            !baseUrl.userInfo().isEmpty() || !baseUrl.query().isEmpty() || !baseUrl.fragment().isEmpty() ||
            (baseUrl.path() != QString() && baseUrl.path() != QStringLiteral("/"))) {
            m_catalogStatus->setText(tr("Enter your Vercel HTTPS site origin only, for example https://your-project.vercel.app."));
            return;
        }
        auto origin = baseUrl;
        origin.setPath(QString());
        APPLICATION->settings()->set("SkinApiBaseUrl", origin.toString());
        m_catalogAfter.clear();
        m_catalogNextAfter.clear();
        m_catalogCursorHistory.clear();
        m_catalogSkins = {};
        m_catalogImages.clear();
        loadCatalogPage();
    });
    connect(m_catalogSearch, &QLineEdit::textChanged, this, [this] {
        m_bundledPage = 0;
        m_catalogScrollArea->verticalScrollBar()->setValue(0);
        updateLocalSkinFilter();
        renderCatalogCards();
    });
    connect(m_catalogSort, &QComboBox::currentIndexChanged, this, [this] {
        m_bundledPage = 0;
        renderCatalogCards();
    });
    connect(m_catalogPreviousButton, &QPushButton::clicked, this, [this] {
        if (m_bundledPage > 0)
            showBundledSkinPage(m_bundledPage - 1);
    });
    connect(m_catalogNextButton, &QPushButton::clicked, this, [this] {
        if (!m_catalogNextAfter.isEmpty())
            showBundledSkinPage(m_bundledPage + 1);
    });
    m_catalogScrollArea->viewport()->installEventFilter(this);
    m_catalogApiRow->setMinimumWidth(0);
    setStudioTab(QStringLiteral("my"));
}

void SkinManageDialog::setupStudioTabs()
{
    const QList<QPair<QString, QString>> tabs = {
        { QStringLiteral("free"), tr("🔥  Free Skins") },
        { QStringLiteral("my"), tr("♟  My Skins") },
        { QStringLiteral("favorites"), tr("♥  Favorites") },
        { QStringLiteral("recent"), tr("◷  Recently Used") },
    };
    for (const auto& [id, label] : tabs) {
        auto* button = new QToolButton(this);
        button->setText(label);
        button->setObjectName(QStringLiteral("skinStudioTab_%1").arg(id));
        button->setProperty("studioTab", id);
        button->setCheckable(true);
        button->setAutoRaise(true);
        button->setStyleSheet(QStringLiteral(
            "QToolButton { background: transparent; border: 0; border-bottom: 2px solid transparent; padding: 9px 14px; color: #8fa8c7; font-weight: 600; }"
            "QToolButton:hover { color: #e6f6ff; }"
            "QToolButton:checked { color: #e8f9ff; border-bottom-color: #12c8e8; background: #0b1a2a; }"));
        m_studioTabButtons.append(button);
        m_ui->studioTabsLayout->addWidget(button);
        connect(button, &QToolButton::clicked, this, [this, id] { setStudioTab(id); });
    }
    m_ui->studioTabsLayout->addStretch(1);
    setStudioTab(QStringLiteral("free"));
}

void SkinManageDialog::setStudioTab(const QString& tab)
{
    m_studioTab = tab;
    for (auto* button : m_studioTabButtons)
        button->setChecked(button->property("studioTab").toString() == tab);

    const bool catalogTab = tab == QStringLiteral("free") || tab == QStringLiteral("favorites");
    m_skinViews->setCurrentWidget(catalogTab ? m_catalogPage : m_ui->listView);
    m_catalogApiRow->hide();
    m_catalogSort->hide();
    for (auto* button : m_catalogCategoryButtons)
        button->setVisible(tab == QStringLiteral("free") || tab == QStringLiteral("favorites"));
    m_catalogPreviousButton->setVisible(tab == QStringLiteral("free"));
    m_catalogNextButton->setVisible(tab == QStringLiteral("free"));
    m_catalogSearch->setPlaceholderText(tab == QStringLiteral("free")
                                            ? tr("Search all skins by filename, category, or pack")
                                            : tab == QStringLiteral("favorites") ? tr("Search your favorites") : tr("Search your skin library"));
    const bool showLegacyImportTools = tab == QStringLiteral("my");
    m_ui->resetBtn->setVisible(showLegacyImportTools);
    m_ui->previewTitleLabel->setVisible(tab != QStringLiteral("free"));
    m_skinNameLabel->setVisible(tab != QStringLiteral("free"));
    m_ui->urlLine->setVisible(showLegacyImportTools);
    m_ui->urlBtn->setVisible(showLegacyImportTools);
    m_ui->userBtn->setVisible(showLegacyImportTools);
    m_ui->browseSkinsBtn->setVisible(showLegacyImportTools);

    if (tab == QStringLiteral("favorites"))
        m_catalogCategory = QStringLiteral("All");
    for (auto* button : m_catalogCategoryButtons)
        button->setChecked(button->text() == m_catalogCategory);
    updateLocalSkinFilter();
    if (tab == QStringLiteral("free")) {
        if (m_bundledCatalogReady) {
            hideCatalogLoadingState();
            showBundledSkinPage(0);
        } else if (!m_bundledCatalogLoading) {
            m_bundledCatalogLoading = true;
            showCatalogLoadingState(tr("Loading the bundled skin library…"));
            QTimer::singleShot(50, this, [this] {
                if (!m_bundledCatalogLoading)
                    return;
                const bool loaded = loadBundledSkinCatalog();
                m_bundledCatalogLoading = false;
                if (m_studioTab != QStringLiteral("free")) {
                    hideCatalogLoadingState();
                    return;
                }
                if (loaded) {
                    hideCatalogLoadingState();
                    showBundledSkinPage(0);
                } else {
                    showCatalogLoadingState(m_catalogStatus->text(), true);
                }
            });
        }
    } else if (tab == QStringLiteral("favorites")) {
        renderCatalogCards();
    } else if (tab == QStringLiteral("recent")) {
        m_catalogStatus->setText(m_recentLocalSkins.isEmpty()
                                     ? tr("Skins you apply or save will appear here.")
                                     : m_localFilterProxy->rowCount() == 0
                                           ? tr("No recently used skins match this search.")
                                           : tr("%1 recently used skin(s).").arg(m_localFilterProxy->rowCount()));
    } else if (m_list.rowCount() == 0) {
        m_catalogStatus->setText(tr("Your skin library is empty. Import a PNG or browse Free Skins."));
    } else {
        m_catalogStatus->setText(m_localFilterProxy->rowCount() == 0
                                     ? tr("No library skins match this search.")
                                     : tr("%1 skin(s) in your local library.").arg(m_localFilterProxy->rowCount()));
    }
}

void SkinManageDialog::setApplyEnabled(bool enabled)
{
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enabled);
    m_ui->applySkinBtn->setEnabled(enabled);
}

void SkinManageDialog::updateLocalSkinFilter()
{
    if (!m_localFilterProxy)
        return;
    auto* proxy = static_cast<SkinLibraryProxy*>(m_localFilterProxy);
    proxy->setFavorites(m_favoriteLocalSkins);
    proxy->setRecent(m_recentLocalSkins);
    proxy->setSearch(m_catalogSearch ? m_catalogSearch->text().trimmed() : QString());
    proxy->setView(m_studioTab == QStringLiteral("favorites") ? SkinLibraryProxy::View::Favorites
                    : m_studioTab == QStringLiteral("recent") ? SkinLibraryProxy::View::Recent
                                                              : SkinLibraryProxy::View::All);
}

void SkinManageDialog::loadStudioState()
{
    m_studioStatePath = QDir(m_list.getDir()).filePath(QStringLiteral("skin-studio.json"));
    QFile stateFile(m_studioStatePath);
    if (!stateFile.exists())
        return;
    if (!stateFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Unable to read skin studio state:" << stateFile.errorString();
        return;
    }
    const auto document = QJsonDocument::fromJson(stateFile.readAll());
    if (!document.isObject()) {
        qWarning() << "Skin studio state is not a valid JSON object:" << m_studioStatePath;
        return;
    }
    const auto root = document.object();
    for (const auto& item : root.value(QStringLiteral("favoriteLocal")).toArray())
        if (item.isString()) m_favoriteLocalSkins.append(item.toString());
    for (const auto& item : root.value(QStringLiteral("recentLocal")).toArray())
        if (item.isString()) m_recentLocalSkins.append(item.toString());
    m_favoriteCatalogSkins = root.value(QStringLiteral("favoriteCatalog")).toArray();
}

void SkinManageDialog::saveStudioState() const
{
    QJsonArray favoriteLocal;
    for (const auto& name : m_favoriteLocalSkins)
        favoriteLocal.append(name);
    QJsonArray recentLocal;
    for (const auto& name : m_recentLocalSkins)
        recentLocal.append(name);
    QJsonObject root;
    root.insert(QStringLiteral("favoriteLocal"), favoriteLocal);
    root.insert(QStringLiteral("recentLocal"), recentLocal);
    root.insert(QStringLiteral("favoriteCatalog"), m_favoriteCatalogSkins);
    QSaveFile file(m_studioStatePath);
    if (!file.open(QIODevice::WriteOnly) ||
        file.write(QJsonDocument(root).toJson(QJsonDocument::Compact)) < 0 ||
        !file.commit()) {
        qCritical() << "Unable to save skin studio state:" << file.errorString();
    }
}

bool SkinManageDialog::loadBundledSkinCatalog()
{
    if (m_bundledCatalogReady)
        return true;
    if (!QDir().mkpath(m_catalogCacheDir)) {
        m_catalogStatus->setText(tr("Unable to create the persistent skin cache: %1").arg(m_catalogCacheDir));
        return false;
    }

    const auto indexPath = QDir(m_catalogCacheDir).filePath(QStringLiteral("bundled-skins.json"));
    QFile indexFile(indexPath);
    if (indexFile.exists()) {
        if (!indexFile.open(QIODevice::ReadOnly)) {
            qWarning() << "Unable to read bundled skin index:" << indexFile.errorString();
        } else {
            QJsonParseError parseError;
            const auto indexDocument = QJsonDocument::fromJson(indexFile.readAll(), &parseError);
            if (parseError.error == QJsonParseError::NoError && indexDocument.isObject() &&
                indexDocument.object().value(QStringLiteral("archives")).isArray()) {
                m_bundledSkinArchives = indexDocument.object().value(QStringLiteral("archives")).toArray();
                for (const auto& source : m_bundledSkinArchives) {
                    for (const auto& entry : source.toObject().value(QStringLiteral("skins")).toArray()) {
                        const auto skin = entry.toObject();
                        const auto id = skin.value(QStringLiteral("uuid")).toString();
                        if (QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(id).hasMatch() &&
                            QFileInfo::exists(QDir(m_catalogCacheDir).filePath(id + QStringLiteral(".png")))) {
                            m_bundledSkinCatalog.append(skin);
                        }
                    }
                }
            } else {
                qWarning() << "Ignoring invalid bundled skin index:" << indexPath << parseError.errorString();
            }
        }
    }

    QFile bundledArchive(QStringLiteral(":/skinpacks/BoostClient_Skins.zip"));
    if (!bundledArchive.open(QIODevice::ReadOnly) || bundledArchive.size() <= 0 ||
        bundledArchive.size() > MAX_SKIN_ARCHIVE_SIZE) {
        m_catalogStatus->setText(tr("The bundled skin archive is missing or exceeds the safe size limit."));
        return false;
    }
    const auto bundledData = bundledArchive.readAll();
    if (bundledData.size() != bundledArchive.size() || !importSkinArchive({}, bundledData)) {
        m_catalogStatus->setText(tr("Unable to load the bundled Minecraft skin pack. Check the launcher log for details."));
        return false;
    }

    auto configuredArchive = qEnvironmentVariable("BOOSTCLIENT_SKIN_ARCHIVE").trimmed();
    if (configuredArchive.isEmpty())
        configuredArchive = APPLICATION->settings()->get("SkinArchivePath").toString().trimmed();
    if (!configuredArchive.isEmpty() && !importSkinArchive(configuredArchive)) {
        m_catalogStatus->setText(tr("The bundled skins are available, but the configured skin archive could not be imported."));
    }

    m_bundledSkinCatalog = sortSkinCatalog(m_bundledSkinCatalog);
    m_catalogSkins = m_bundledSkinCatalog;
    m_bundledCatalogReady = true;
    return true;
}

bool SkinManageDialog::importSkinArchive(const QString& archivePath, const QByteArray& archiveData)
{
    QByteArray archiveBytesData = archiveData;
    QString sourceName = tr("bundled skin pack");
    if (archiveBytesData.isEmpty()) {
        QFile sourceFile(archivePath);
        if (!sourceFile.open(QIODevice::ReadOnly)) {
            qWarning() << "Unable to open skin archive:" << archivePath << sourceFile.errorString();
            return false;
        }
        if (sourceFile.size() <= 0 || sourceFile.size() > MAX_SKIN_ARCHIVE_SIZE) {
            qWarning() << "Skin archive is empty or exceeds the 50 MiB limit:" << archivePath;
            return false;
        }
        archiveBytesData = sourceFile.readAll();
        if (archiveBytesData.size() != sourceFile.size()) {
            qWarning() << "Unable to read complete skin archive:" << archivePath << sourceFile.errorString();
            return false;
        }
        sourceName = QFileInfo(archivePath).fileName();
    }

    const auto archiveHash =
        QString::fromLatin1(QCryptographicHash::hash(archiveBytesData, QCryptographicHash::Sha256).toHex());
    auto existingSource = std::find_if(m_bundledSkinArchives.begin(), m_bundledSkinArchives.end(),
                                       [&archiveHash](const QJsonValue& source) {
                                           return source.toObject().value(QStringLiteral("sha256")).toString() == archiveHash;
                                       });
    if (existingSource != m_bundledSkinArchives.cend()) {
        const auto existingSkins = existingSource->toObject().value(QStringLiteral("skins")).toArray();
        const bool cacheComplete = std::all_of(existingSkins.cbegin(), existingSkins.cend(), [this](const QJsonValue& item) {
            const auto id = item.toObject().value(QStringLiteral("uuid")).toString();
            return QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(id).hasMatch() &&
                   QFileInfo::exists(QDir(m_catalogCacheDir).filePath(id + QStringLiteral(".png")));
        });
        if (cacheComplete)
            return true;
        QSet<QString> removedIds;
        for (const auto& item : existingSkins)
            removedIds.insert(item.toObject().value(QStringLiteral("uuid")).toString());
        QJsonArray remainingSkins;
        for (const auto& entry : m_bundledSkinCatalog)
            if (!removedIds.contains(entry.toObject().value(QStringLiteral("uuid")).toString()))
                remainingSkins.append(entry);
        m_bundledSkinCatalog = remainingSkins;
        m_bundledSkinArchives.erase(existingSource);
    }

    QTemporaryFile temporaryArchive;
    if (!temporaryArchive.open() || temporaryArchive.write(archiveBytesData) != archiveBytesData.size() ||
        !temporaryArchive.flush()) {
        qWarning() << "Unable to stage skin archive:" << temporaryArchive.errorString();
        return false;
    }
    const auto stagedPath = temporaryArchive.fileName();
    temporaryArchive.close();

    MMCZip::ArchiveReader reader(stagedPath);
    QJsonArray importedSkins;
    qint64 archiveBytes = 0;
    int invalidPngs = 0;
    int validPngs = 0;
    auto result = reader.parse([&](MMCZip::ArchiveReader::File* file) -> Result<> {
        if (!file->isFile())
            return file->skip();

        const auto entrySize = file->size();
        if (entrySize < 0 || entrySize > MAX_SKIN_ARCHIVE_TOTAL_BYTES - archiveBytes)
            return std::unexpected{ QStringLiteral("The archive exceeds the 100 MiB expanded size limit.") };
        archiveBytes += entrySize;

        auto entryPath = file->filename();
        entryPath.replace(QLatin1Char('\\'), QLatin1Char('/'));
        const auto cleanEntryPath = QDir::cleanPath(entryPath);
        if (cleanEntryPath.isEmpty() || cleanEntryPath == QStringLiteral(".") ||
            cleanEntryPath.startsWith(QStringLiteral("../")) || cleanEntryPath == QStringLiteral("..") ||
            QDir::isAbsolutePath(cleanEntryPath)) {
            ++invalidPngs;
            return file->skip();
        }
        if (!cleanEntryPath.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive))
            return file->skip();
        if (entrySize < 8 || entrySize > MAX_SKIN_API_RESPONSE_SIZE) {
            ++invalidPngs;
            return file->skip();
        }
        if (importedSkins.size() >= MAX_SKIN_ARCHIVE_ENTRIES)
            return std::unexpected{ QStringLiteral("The archive exceeds the 10,000-skin limit.") };

        auto imageData = file->readAll();
        if (!imageData)
            return std::unexpected{ imageData.error() };
        if (!isValidSkinPng(*imageData)) {
            ++invalidPngs;
            return {};
        }
        ++validPngs;

        const auto identity = cleanEntryPath.toUtf8() + QByteArray(1, '\0') + *imageData;
        const auto id = QString::fromLatin1(QCryptographicHash::hash(identity, QCryptographicHash::Sha256).toHex());
        if (std::any_of(m_bundledSkinCatalog.cbegin(), m_bundledSkinCatalog.cend(), [&id](const QJsonValue& item) {
                return item.toObject().value(QStringLiteral("uuid")).toString() == id;
            }))
            return {};

        const auto imagePath = QDir(m_catalogCacheDir).filePath(id + QStringLiteral(".png"));
        QSaveFile imageFile(imagePath);
        if (!imageFile.open(QIODevice::WriteOnly) || imageFile.write(*imageData) != imageData->size() || !imageFile.commit())
            return std::unexpected{ QStringLiteral("Unable to cache skin %1: %2").arg(entryPath, imageFile.errorString()) };

        const auto pathParts = cleanEntryPath.split(QLatin1Char('/'), Qt::SkipEmptyParts);
        QJsonObject skin;
        skin.insert(QStringLiteral("uuid"), id);
        skin.insert(QStringLiteral("name"), QFileInfo(cleanEntryPath).completeBaseName());
        skin.insert(QStringLiteral("fileName"), QFileInfo(cleanEntryPath).fileName());
        skin.insert(QStringLiteral("category"), pathParts.size() > 1 ? pathParts.first() : tr("Uncategorized"));
        skin.insert(QStringLiteral("pack"), pathParts.size() > 2 ? pathParts.at(1) : QString());
        skin.insert(QStringLiteral("variant"), QStringLiteral("classic"));
        skin.insert(QStringLiteral("source"), sourceName);
        importedSkins.append(skin);
        m_bundledSkinCatalog.append(skin);
        return {};
    });
    if (!result) {
        qWarning() << "Unable to process skin archive" << sourceName << ":" << result.error();
        return false;
    }
    if (validPngs == 0) {
        qWarning() << "Skin archive contains no valid 64x64 or 64x32 PNG skins:" << sourceName
                   << "invalid PNGs:" << invalidPngs;
        return false;
    }

    QJsonObject archiveRecord;
    archiveRecord.insert(QStringLiteral("sha256"), archiveHash);
    archiveRecord.insert(QStringLiteral("source"), sourceName);
    archiveRecord.insert(QStringLiteral("skins"), importedSkins);
    m_bundledSkinArchives.append(archiveRecord);
    QJsonObject index;
    index.insert(QStringLiteral("version"), 1);
    index.insert(QStringLiteral("archives"), m_bundledSkinArchives);
    QSaveFile indexFile(QDir(m_catalogCacheDir).filePath(QStringLiteral("bundled-skins.json")));
    if (!indexFile.open(QIODevice::WriteOnly) ||
        indexFile.write(QJsonDocument(index).toJson(QJsonDocument::Compact)) < 0 || !indexFile.commit()) {
        qWarning() << "Unable to save persistent bundled skin index:" << indexFile.errorString();
        return false;
    }
    qInfo() << "Indexed" << validPngs << "valid skins from" << sourceName << "; added" << importedSkins.size()
            << "new skins and skipped invalid PNGs:" << invalidPngs;
    return true;
}

void SkinManageDialog::showBundledSkinPage(int page)
{
    m_bundledSkinCatalog = sortSkinCatalog(m_bundledSkinCatalog);
    m_catalogSkins = m_bundledSkinCatalog;
    m_catalogScrollArea->verticalScrollBar()->setValue(0);
    const auto pageCount = (m_catalogSkins.size() + BUNDLED_SKIN_PAGE_SIZE - 1) / BUNDLED_SKIN_PAGE_SIZE;
    m_bundledPage = qBound(0, page, qMax(0, pageCount - 1));
    m_catalogAfter = QString::number(m_bundledPage);
    m_catalogNextAfter = m_bundledPage + 1 < pageCount ? QString::number(m_bundledPage + 1) : QString();
    m_catalogPreviousButton->setEnabled(m_bundledPage > 0);
    m_catalogNextButton->setEnabled(!m_catalogNextAfter.isEmpty());
    renderCatalogCards();
    if (m_pendingCatalogUuid.isEmpty() && m_selectedSkinKey.isEmpty() && !m_catalogSkins.isEmpty()) {
        const auto firstSkin = m_catalogSkins.at(m_bundledPage * BUNDLED_SKIN_PAGE_SIZE).toObject();
        const auto uuid = firstSkin.value(QStringLiteral("uuid")).toString();
        QFile skinFile(QDir(m_catalogCacheDir).filePath(uuid + QStringLiteral(".png")));
        if (skinFile.open(QIODevice::ReadOnly)) {
            const auto skinData = skinFile.readAll();
            selectCatalogSkin(uuid, firstSkin.value(QStringLiteral("name")).toString(),
                              firstSkin.value(QStringLiteral("variant")).toString(), skinData);
        } else {
            qWarning() << "Unable to open the first bundled skin for preview:" << skinFile.fileName() << skinFile.errorString();
        }
    }
}

void SkinManageDialog::toggleCatalogFavorite(const QJsonObject& skin)
{
    const auto uuid = skin.value(QStringLiteral("uuid")).toString();
    if (uuid.isEmpty())
        return;
    QJsonArray updated;
    bool removed = false;
    for (const auto& entry : m_favoriteCatalogSkins) {
        const auto object = entry.toObject();
        if (object.value(QStringLiteral("uuid")).toString() == uuid) {
            removed = true;
            continue;
        }
        updated.append(object);
    }
    if (!removed)
        updated.append(skin);
    m_favoriteCatalogSkins = updated;
    saveStudioState();
    renderCatalogCards();
}

void SkinManageDialog::loadCatalogPage()
{
    if (m_catalogLoading)
        return;
    m_catalogSkins = {};
    m_catalogNextAfter.clear();
    m_catalogImages.clear();
    const auto baseUrl = QUrl(APPLICATION->settings()->get("SkinApiBaseUrl").toString().trimmed());
    const auto cacheIdentity = m_catalogAfter.isEmpty() ? QByteArray("first") : m_catalogAfter.toUtf8();
    const auto cacheName = QString::fromLatin1(QCryptographicHash::hash(cacheIdentity, QCryptographicHash::Sha256).toHex()) +
                           QStringLiteral(".json");
    const auto cachePath = QDir(m_catalogCacheDir).filePath(cacheName);
    QFile cachedPage(cachePath);
    if (cachedPage.open(QIODevice::ReadOnly)) {
        QJsonParseError cachedError;
        const auto cachedDocument = QJsonDocument::fromJson(cachedPage.readAll(), &cachedError);
        if (cachedError.error == QJsonParseError::NoError && cachedDocument.isObject() &&
            cachedDocument.object().value(QStringLiteral("skins")).isArray()) {
            QJsonArray validSkins;
            const auto cachedSkins = cachedDocument.object().value(QStringLiteral("skins")).toArray();
            for (const auto& entry : cachedSkins) {
                const auto skin = entry.toObject();
                if (QRegularExpression(QStringLiteral("^[0-9a-f]{32}$"))
                        .match(skin.value(QStringLiteral("uuid")).toString())
                        .hasMatch())
                    validSkins.append(skin);
            }
            m_catalogSkins = validSkins;
            m_catalogNextAfter = cachedDocument.object().value(QStringLiteral("nextAfter")).toString();
            m_catalogNextButton->setEnabled(!m_catalogNextAfter.isEmpty());
            m_catalogPreviousButton->setEnabled(!m_catalogCursorHistory.isEmpty());
            m_catalogStatus->setText(tr("Showing your cached MineSkin page while checking for updates…"));
            renderCatalogCards();
        } else {
            qWarning() << "Ignoring invalid cached MineSkin page:" << cachePath;
        }
    }

    if (!baseUrl.isValid() || baseUrl.scheme() != QStringLiteral("https") || baseUrl.host().isEmpty() ||
        baseUrl.host().compare(QStringLiteral("mccatalog.com"), Qt::CaseInsensitive) == 0 ||
        !baseUrl.userInfo().isEmpty() || !baseUrl.query().isEmpty() || !baseUrl.fragment().isEmpty() ||
        (baseUrl.path() != QString() && baseUrl.path() != QStringLiteral("/"))) {
        if (m_catalogSkins.isEmpty()) {
            m_catalogStatus->setText(
                tr("Connect your Vercel API to browse real MineSkin downloads. No sample or placeholder skins are shown."));
        } else {
            m_catalogStatus->setText(tr("Showing cached MineSkin skins. Connect your Vercel API to refresh this page."));
            renderCatalogCards();
        }
        return;
    }

    QUrl url = baseUrl;
    url.setPath(QStringLiteral("/api/skins/catalog"));
    if (!m_catalogAfter.isEmpty()) {
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("after"), m_catalogAfter);
        url.setQuery(query);
    }
    m_catalogLoading = true;
    m_catalogConnectButton->setEnabled(false);
    m_catalogPreviousButton->setEnabled(false);
    m_catalogNextButton->setEnabled(false);
    if (m_catalogSkins.isEmpty())
        m_catalogStatus->setText(tr("Loading public MineSkin catalog…"));
    QNetworkRequest request(url);
    request.setTransferTimeout(12000);
    request.setRawHeader("Accept", "application/json");
    auto* reply = APPLICATION->network()->get(request);
    auto response = std::make_shared<QByteArray>();
    connect(reply, &QNetworkReply::readyRead, this, [reply, response] {
        response->append(reply->readAll());
        if (response->size() > MAX_SKIN_API_RESPONSE_SIZE) reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, response, cacheName] {
        response->append(reply->readAll());
        m_catalogLoading = false;
        m_catalogConnectButton->setEnabled(true);
        const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError) {
            const auto error = status == 429
                                         ? tr("MineSkin is rate-limiting the catalog. Please wait and try again.")
                                         : status == 503
                                               ? tr("Vercel does not have MINESKIN_API_KEY configured yet.")
                                               : tr("Could not load the catalog (HTTP %1). Check your Vercel URL and deployment.")
                                                     .arg(status);
            if (m_catalogSkins.isEmpty())
                m_catalogStatus->setText(error);
            else
                m_catalogStatus->setText(tr("Using cached skins. Update failed: %1").arg(error));
            m_catalogPreviousButton->setEnabled(!m_catalogCursorHistory.isEmpty());
            m_catalogNextButton->setEnabled(!m_catalogNextAfter.isEmpty());
            reply->deleteLater();
            return;
        }

        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(*response, &parseError);
        const auto object = document.object();
        const auto skins = object.value(QStringLiteral("skins")).toArray();
        if (parseError.error != QJsonParseError::NoError || !document.isObject() || !object.value(QStringLiteral("skins")).isArray()) {
            m_catalogStatus->setText(tr("Vercel returned an invalid catalog response."));
            reply->deleteLater();
            return;
        }
        QJsonArray validSkins;
        for (const auto& entry : skins) {
            const auto skin = entry.toObject();
            const auto uuid = skin.value(QStringLiteral("uuid")).toString();
            if (QRegularExpression(QStringLiteral("^[0-9a-f]{32}$")).match(uuid).hasMatch())
                validSkins.append(skin);
        }
        m_catalogSkins = validSkins;
        m_catalogNextAfter = object.value(QStringLiteral("nextAfter")).toString();
        m_catalogImages.clear();
        QSaveFile pageFile(QDir(m_catalogCacheDir).filePath(cacheName));
        if (pageFile.open(QIODevice::WriteOnly)) {
            if (pageFile.write(*response) < 0 || !pageFile.commit())
                qWarning() << "Unable to cache MineSkin catalog page:" << pageFile.errorString();
        } else {
            qWarning() << "Unable to create MineSkin catalog page cache:" << pageFile.errorString();
        }
        m_catalogNextButton->setEnabled(!m_catalogNextAfter.isEmpty());
        m_catalogPreviousButton->setEnabled(!m_catalogCursorHistory.isEmpty());
        renderCatalogCards();
        reply->deleteLater();
    });
}

void SkinManageDialog::renderCatalogCards()
{
    if (!m_catalogGrid || !m_catalogSearch || !m_catalogScrollArea)
        return;
    const auto viewportSize = m_catalogScrollArea->viewport()->size();
    m_catalogColumns = BUNDLED_SKIN_COLUMNS;
    const int cardWidth = qBound(160, (viewportSize.width() - 32) / BUNDLED_SKIN_COLUMNS, 260);
    const int cardHeight = qBound(180, (viewportSize.height() - 20) / 2, 230);
    m_catalogCardWidth = cardWidth;
    m_catalogCardHeight = cardHeight;
    const int rowStride = cardHeight + m_catalogGrid->verticalSpacing();
    m_catalogScrollArea->verticalScrollBar()->setSingleStep(rowStride);
    for (int row = 0; row < m_catalogGrid->rowCount(); ++row)
        m_catalogGrid->setRowMinimumHeight(row, 0);
    for (int column = 0; column < m_catalogGrid->columnCount(); ++column)
        m_catalogGrid->setColumnMinimumWidth(column, 0);
    while (auto* item = m_catalogGrid->takeAt(0)) {
        if (auto* widget = item->widget()) widget->deleteLater();
        delete item;
    }
    QList<QJsonObject> skins;
    if (m_studioTab == QStringLiteral("favorites")) {
        skins.reserve(m_favoriteCatalogSkins.size() + m_favoriteLocalSkins.size());
        for (const auto& entry : m_favoriteCatalogSkins)
            skins.append(entry.toObject());
        for (const auto& name : m_favoriteLocalSkins) {
            const auto* localSkin = m_list.skin(name);
            if (!localSkin)
                continue;
            QJsonObject entry;
            entry.insert(QStringLiteral("localName"), name);
            entry.insert(QStringLiteral("name"), localSkin->name());
            entry.insert(QStringLiteral("variant"), localSkin->getModelString().toLower());
            skins.append(entry);
        }
    } else {
        skins.reserve(m_catalogSkins.size());
        for (const auto& entry : m_catalogSkins)
            skins.append(entry.toObject());
    }
    if (m_catalogSort->currentData().toString() == QStringLiteral("views") ||
        m_catalogCategory == QStringLiteral("Popular")) {
        std::sort(skins.begin(), skins.end(), [](const QJsonObject& left, const QJsonObject& right) {
            return left.value(QStringLiteral("views")).toInt() > right.value(QStringLiteral("views")).toInt();
        });
    }
    const auto filter = m_catalogSearch->text().trimmed();
    const QRegularExpression searchPattern(QRegularExpression::escape(filter), QRegularExpression::CaseInsensitiveOption);
    QList<QJsonObject> filteredSkins;
    for (const auto& skin : skins) {
        if (m_catalogCategory == QStringLiteral("Popular")) {
            if (!skin.contains(QStringLiteral("views")))
                continue;
        } else if (m_catalogCategory != QStringLiteral("All") && skinCategory(skin) != m_catalogCategory) {
            continue;
        }
        const auto searchable = skin.value(QStringLiteral("name")).toString() + QLatin1Char(' ') +
                                skin.value(QStringLiteral("uuid")).toString() + QLatin1Char(' ') +
                                skin.value(QStringLiteral("fileName")).toString() + QLatin1Char(' ') +
                                skin.value(QStringLiteral("category")).toString() + QLatin1Char(' ') +
                                skin.value(QStringLiteral("pack")).toString() + QLatin1Char(' ') +
                                skin.value(QStringLiteral("localName")).toString();
        if (searchPattern.match(searchable).hasMatch())
            filteredSkins.append(skin);
    }

    const bool freeTab = m_studioTab == QStringLiteral("free");
    const int firstResult = freeTab ? m_bundledPage * BUNDLED_SKIN_PAGE_SIZE : 0;
    const int lastResult = freeTab ? qMin(firstResult + BUNDLED_SKIN_PAGE_SIZE, filteredSkins.size()) : filteredSkins.size();
    m_catalogPreviousButton->setEnabled(freeTab && m_bundledPage > 0);
    m_catalogNextButton->setEnabled(freeTab && lastResult < filteredSkins.size());
    int displayed = 0;
    for (int skinIndex = firstResult; skinIndex < lastResult; ++skinIndex) {
        const auto skin = filteredSkins.at(skinIndex);
        const auto uuid = skin.value(QStringLiteral("uuid")).toString();
        const auto localName = skin.value(QStringLiteral("localName")).toString();
        const auto fileName = skin.value(QStringLiteral("fileName")).toString();
        const auto name = skin.value(QStringLiteral("name")).toString();
        const auto variant = skin.value(QStringLiteral("variant")).toString();
        const bool local = !localName.isEmpty();
        const auto identity = skin.value(QStringLiteral("source")).toString().isEmpty() || fileName.isEmpty()
                                  ? (name.isEmpty() ? (local ? localName : uuid.left(8)) : name)
                                  : QFileInfo(fileName).completeBaseName().left(8);

        auto* card = new QWidget(m_catalogPage);
        card->setObjectName(QStringLiteral("skinCard_%1").arg(local ? localName : uuid));
        card->setFixedSize(cardWidth, cardHeight);
        auto* cardLayout = new QGridLayout(card);
        cardLayout->setContentsMargins(0, 0, 0, 0);
        auto* selectButton = new AnimatedSkinCardButton(card);
        selectButton->setObjectName(QStringLiteral("skinCardSelect_%1").arg(local ? localName : uuid));
        selectButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        const auto modelText = variant.compare(QStringLiteral("slim"), Qt::CaseInsensitive) == 0 ? tr("Slim") : tr("Classic");
        selectButton->setText(tr("%1\n%2").arg(identity, modelText));
        const auto sourcePath = skin.value(QStringLiteral("category")).toString() + QLatin1Char('/') +
                                skin.value(QStringLiteral("pack")).toString() + QLatin1Char('/') + fileName;
        selectButton->setToolTip(local ? tr("Preview this skin from your local library.")
                                       : skin.value(QStringLiteral("source")).toString().isEmpty()
                                             ? tr("Preview this public MineSkin skin. Select Apply to add it to your library.")
                                             : tr("%1 · %2").arg(sourcePath, skin.value(QStringLiteral("source")).toString()));
        selectButton->setIconSize(QSize(qMin(176, cardWidth - 32), cardHeight - 70));
        selectButton->setFixedSize(cardWidth, cardHeight);
        selectButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        selectButton->setStyleSheet(QStringLiteral(
            "QToolButton { background: #0a1728; border: 1px solid %1; border-radius: 9px; padding: 7px; color: #e6efff; text-align: center; }"
            "QToolButton:hover { background: #10233a; border-color: #15c8e8; }")
                                        .arg((local && localName == m_selectedSkinKey) ||
                                                     (!local && uuid == m_pendingCatalogUuid)
                                                 ? QStringLiteral("#12c8e8")
                                                 : QStringLiteral("#203852")));
        if (local) {
            if (const auto* localSkin = m_list.skin(localName))
                selectButton->setIcon(QPixmap::fromImage(skinCardPreview(*localSkin)));
        } else {
            const auto cachedImage = m_catalogImages.constFind(uuid);
            if (cachedImage != m_catalogImages.cend()) {
                SkinModel thumbnail(QDir(m_catalogCacheDir).filePath(uuid + QStringLiteral(".png")));
                if (thumbnail.isValid()) {
                    thumbnail.setModel(variant.compare(QStringLiteral("slim"), Qt::CaseInsensitive) == 0
                                           ? SkinModel::SLIM
                                           : SkinModel::CLASSIC);
                    selectButton->setIcon(QPixmap::fromImage(skinCardPreview(thumbnail)));
                }
            } else {
                selectButton->setText(tr("%1\nLoading preview…").arg(identity));
            }
        }
        cardLayout->addWidget(selectButton, 0, 0);
        auto* favoriteButton = new QToolButton(card);
        favoriteButton->setText(local ? (m_favoriteLocalSkins.contains(localName) ? QStringLiteral("♥") : QStringLiteral("♡"))
                                      : (std::any_of(m_favoriteCatalogSkins.cbegin(), m_favoriteCatalogSkins.cend(),
                                                     [&uuid](const QJsonValue& entry) {
                                                         return entry.toObject().value(QStringLiteral("uuid")).toString() == uuid;
                                                     })
                                             ? QStringLiteral("♥")
                                             : QStringLiteral("♡")));
        favoriteButton->setToolTip(tr("Toggle favorite"));
        favoriteButton->setFixedSize(28, 28);
        favoriteButton->setStyleSheet(QStringLiteral(
            "QToolButton { background: #071322; border: 1px solid #314966; border-radius: 14px; color: #19c8e8; font-size: 15px; }"
            "QToolButton:hover { background: #12304a; border-color: #19c8e8; }"));
        cardLayout->addWidget(favoriteButton, 0, 0, Qt::AlignTop | Qt::AlignRight);
        const auto row = displayed / qMax(1, m_catalogColumns);
        const auto column = displayed % qMax(1, m_catalogColumns);
        m_catalogGrid->setRowMinimumHeight(row, cardHeight);
        m_catalogGrid->setColumnMinimumWidth(column, cardWidth);
        m_catalogGrid->addWidget(card, row, column, Qt::AlignLeft | Qt::AlignTop);
        ++displayed;
        if (local) {
            connect(selectButton, &QToolButton::clicked, this, [this, localName] {
                setStudioTab(QStringLiteral("my"));
                const auto index = m_localFilterProxy->mapFromSource(m_list.index(m_list.getSkinIndex(localName)));
                m_ui->listView->setCurrentIndex(index);
            });
            connect(favoriteButton, &QToolButton::clicked, this, [this, localName] {
                if (m_favoriteLocalSkins.contains(localName))
                    m_favoriteLocalSkins.removeAll(localName);
                else
                    m_favoriteLocalSkins.append(localName);
                saveStudioState();
                updateLocalSkinFilter();
                renderCatalogCards();
            });
        } else {
            connect(selectButton, &QToolButton::clicked, this, [this, uuid, name, variant] {
                m_pendingCatalogUuid = uuid;
                m_pendingCatalogName = name;
                m_pendingCatalogVariant = variant;
                m_pendingCatalogData.clear();
                m_selectedSkinKey.clear();
                m_catalogPreviewSkin.reset();
                m_ui->listView->clearSelection();
                setApplyEnabled(false);
                m_ui->downloadBtn->setEnabled(false);
                m_skinNameLabel->setText(tr("Loading %1…").arg(name.isEmpty() ? uuid.left(8) : name));
                const auto cached = m_catalogImages.constFind(uuid);
                if (cached != m_catalogImages.cend()) {
                    selectCatalogSkin(uuid, name, variant, cached.value());
                } else {
                    m_catalogStatus->setText(tr("Loading %1 for preview…").arg(name.isEmpty() ? uuid.left(8) : name));
                    loadCatalogThumbnail(uuid, variant, nullptr);
                }
            });
            connect(favoriteButton, &QToolButton::clicked, this, [this, skin] { toggleCatalogFavorite(skin); });
            loadCatalogThumbnail(uuid, variant, selectButton);
        }
    }
    m_catalogGrid->activate();
    if (auto* contents = m_catalogScrollArea->widget()) {
        const int gridRowStride = m_catalogCardHeight + m_catalogGrid->verticalSpacing();
        const int naturalHeight = m_catalogGrid->sizeHint().height();
        const int scrollRange = naturalHeight - m_catalogScrollArea->viewport()->height();
        const int bottomPadding =
            gridRowStride > 0 && scrollRange > 0 ? (gridRowStride - scrollRange % gridRowStride) % gridRowStride : 0;
        contents->setMinimumHeight(naturalHeight + bottomPadding);
    }
    if (m_studioTab == QStringLiteral("favorites") && displayed == 0) {
        m_catalogStatus->setText(m_favoriteCatalogSkins.isEmpty() && m_favoriteLocalSkins.isEmpty()
                                     ? tr("Tap the heart on a skin card to add it to Favorites.")
                                     : tr("No favorites match these filters."));
    } else if (displayed == 0) {
        if (m_catalogCategory == QStringLiteral("Popular") && filteredSkins.isEmpty())
            m_catalogStatus->setText(tr("This skin pack does not include popularity ratings."));
        else
            m_catalogStatus->setText(m_catalogSkins.isEmpty()
                                         ? tr("No real skin PNGs are available in the configured skin archive.")
                                         : tr("No skins match these filters."));
    } else {
        if (freeTab) {
            m_catalogStatus->setText(tr("%1–%2 of %3 skins")
                                         .arg(firstResult + 1)
                                         .arg(firstResult + displayed)
                                         .arg(filteredSkins.size()));
        } else {
            m_catalogStatus->setText(tr("%1 skins shown · your favorites are saved locally.").arg(displayed));
        }
    }
}

void SkinManageDialog::snapCatalogScrollPosition()
{
    if (!m_catalogScrollArea || !m_catalogGrid)
        return;

    const int rowStride = m_catalogCardHeight + m_catalogGrid->verticalSpacing();
    if (rowStride <= 0)
        return;

    auto* scrollBar = m_catalogScrollArea->verticalScrollBar();
    const int maximumAlignedValue = scrollBar->maximum() - scrollBar->maximum() % rowStride;
    const int nearestRow = qRound(static_cast<double>(scrollBar->value()) / rowStride);
    scrollBar->setValue(qBound(0, qMin(nearestRow * rowStride, maximumAlignedValue), scrollBar->maximum()));
}

void SkinManageDialog::showCatalogLoadingState(const QString& message, bool error)
{
    if (!m_catalogLoadingOverlay)
        return;
    m_catalogLoadingLabel->setText(message);
    m_catalogLoadingProgress->setVisible(!error);
    m_catalogLoadingRetryButton->setVisible(error);
    m_catalogLoadingOverlay->setGeometry(m_catalogScrollArea->viewport()->rect());
    m_catalogLoadingOverlay->show();
    m_catalogLoadingOverlay->raise();
}

void SkinManageDialog::hideCatalogLoadingState()
{
    if (m_catalogLoadingOverlay)
        m_catalogLoadingOverlay->hide();
}

void SkinManageDialog::loadCatalogThumbnail(const QString& uuid, const QString& variant, QToolButton* button)
{
    const auto cachePath = QDir(m_catalogCacheDir).filePath(uuid + QStringLiteral(".png"));
    const auto cached = m_catalogImages.constFind(uuid);
    if (cached != m_catalogImages.cend()) {
        if (button) {
            SkinModel thumbnail(cachePath);
            if (thumbnail.isValid()) {
                thumbnail.setModel(variant.compare(QStringLiteral("slim"), Qt::CaseInsensitive) == 0
                                       ? SkinModel::SLIM
                                       : SkinModel::CLASSIC);
                button->setIcon(QPixmap::fromImage(skinCardPreview(thumbnail)));
                button->setText(button->text().replace(tr("Loading preview…"), variant == QStringLiteral("slim") ? tr("Slim") : tr("Classic")));
            }
        }
        return;
    }
    QFile cachedFile(cachePath);
    if (cachedFile.open(QIODevice::ReadOnly)) {
        const auto cachedData = cachedFile.readAll();
        if (isValidSkinPng(cachedData)) {
            m_catalogImages.insert(uuid, cachedData);
            if (button) {
                SkinModel thumbnail(cachePath);
                thumbnail.setModel(variant.compare(QStringLiteral("slim"), Qt::CaseInsensitive) == 0
                                       ? SkinModel::SLIM
                                       : SkinModel::CLASSIC);
                button->setIcon(QPixmap::fromImage(skinCardPreview(thumbnail)));
                button->setText(button->text().replace(tr("Loading preview…"), variant == QStringLiteral("slim") ? tr("Slim") : tr("Classic")));
            } else if (m_pendingCatalogUuid == uuid) {
                selectCatalogSkin(uuid, m_pendingCatalogName, variant, cachedData);
            }
            return;
        }
        qWarning() << "Ignoring invalid cached MineSkin PNG:" << cachePath;
    }

    if (uuid.size() == 64) {
        if (button)
            button->setToolTip(tr("The cached bundled skin PNG is unavailable. Re-import its skin pack."));
        if (m_pendingCatalogUuid == uuid)
            m_catalogStatus->setText(tr("The cached bundled skin PNG is unavailable. Re-import its skin pack."));
        return;
    }

    if (m_catalogImagesLoading.contains(uuid))
        return;
    m_catalogImagesLoading.insert(uuid);
    const auto baseUrl = QUrl(APPLICATION->settings()->get("SkinApiBaseUrl").toString().trimmed());
    QUrl url = baseUrl;
    url.setPath(QStringLiteral("/api/skins/catalog-skin"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("uuid"), uuid);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setTransferTimeout(12000);
    request.setRawHeader("Accept", "image/png");
    auto* reply = APPLICATION->network()->get(request);
    auto response = std::make_shared<QByteArray>();
    QPointer<QToolButton> guardedButton(button);
    connect(reply, &QNetworkReply::readyRead, this, [reply, response] {
        response->append(reply->readAll());
        if (response->size() > MAX_SKIN_API_RESPONSE_SIZE) reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, uuid, variant, response, guardedButton, reply, cachePath] {
        response->append(reply->readAll());
        m_catalogImagesLoading.remove(uuid);
        const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString().section(';', 0, 0).trimmed().toLower();
        const auto skinVariant = QString::fromLatin1(reply->rawHeader("X-Skin-Variant")).toLower();
        const auto resolvedVariant = skinVariant == QStringLiteral("slim") ? QStringLiteral("slim") : variant;
        if (reply->error() == QNetworkReply::NoError && contentType == QStringLiteral("image/png") &&
            isValidSkinPng(*response)) {
            m_catalogImages.insert(uuid, *response);
            QSaveFile imageFile(cachePath);
            if (imageFile.open(QIODevice::WriteOnly) && imageFile.write(*response) == response->size() && imageFile.commit()) {
                SkinModel thumbnail(cachePath);
                thumbnail.setModel(resolvedVariant == QStringLiteral("slim") ? SkinModel::SLIM : SkinModel::CLASSIC);
                auto* previewButton = guardedButton ? guardedButton.data()
                                                    : m_catalogPage->findChild<QToolButton*>(QStringLiteral("skinCardSelect_%1").arg(uuid));
                if (previewButton) {
                    previewButton->setIcon(QPixmap::fromImage(skinCardPreview(thumbnail)));
                    previewButton->setText(previewButton->text().replace(
                        tr("Loading preview…"), resolvedVariant == QStringLiteral("slim") ? tr("Slim") : tr("Classic")));
                }
                if (m_pendingCatalogUuid == uuid)
                    selectCatalogSkin(uuid, m_pendingCatalogName, resolvedVariant, *response);
            } else {
                m_catalogStatus->setText(tr("Unable to cache this skin preview: %1").arg(imageFile.errorString()));
            }
        } else if (guardedButton) {
            guardedButton->setToolTip(status == 503 ? tr("MineSkin API is not configured on Vercel.")
                                                     : tr("Could not load valid skin preview (HTTP %1).").arg(status));
        } else if (m_pendingCatalogUuid == uuid) {
            m_catalogStatus->setText(status == 503
                                         ? tr("Vercel needs the MINESKIN_API_KEY environment variable.")
                                         : tr("Could not load this skin preview (HTTP %1).").arg(status));
        }
        reply->deleteLater();
    });
}

void SkinManageDialog::selectCatalogSkin(const QString& uuid, const QString& name, const QString& variant, const QByteArray& skinData)
{
    const auto cachePath = QDir(m_catalogCacheDir).filePath(uuid + QStringLiteral(".png"));
    if (!isValidSkinPng(skinData)) {
        m_catalogStatus->setText(tr("This catalog entry did not contain a valid Minecraft skin PNG."));
        return;
    }
    if (!QFileInfo::exists(cachePath)) {
        QSaveFile imageFile(cachePath);
        if (!imageFile.open(QIODevice::WriteOnly) || imageFile.write(skinData) != skinData.size() || !imageFile.commit()) {
            m_catalogStatus->setText(tr("Unable to cache this skin preview: %1").arg(imageFile.errorString()));
            return;
        }
    }
    m_catalogPreviewSkin = std::make_unique<SkinModel>(cachePath);
    if (!m_catalogPreviewSkin->isValid()) {
        m_catalogPreviewSkin.reset();
        m_catalogStatus->setText(tr("The downloaded PNG is not a valid Minecraft skin."));
        return;
    }
    const auto resolvedVariant = variant.compare(QStringLiteral("slim"), Qt::CaseInsensitive) == 0
                                     ? QStringLiteral("slim")
                                     : QStringLiteral("classic");
    m_catalogPreviewSkin->setModel(resolvedVariant == QStringLiteral("slim") ? SkinModel::SLIM : SkinModel::CLASSIC);
    m_catalogPreviewSkin->setCapeId(m_ui->capeCombo->currentData().toString());
    m_pendingCatalogUuid = uuid;
    m_pendingCatalogName = name;
    m_pendingCatalogVariant = resolvedVariant;
    m_pendingCatalogData = skinData;
    m_selectedSkinKey.clear();
    setApplyEnabled(true);
    m_ui->downloadBtn->setEnabled(true);
    m_ui->listView->clearSelection();
    m_skinNameLabel->setText(name.isEmpty() ? tr("MineSkin %1").arg(uuid.left(8)) : name);
    m_ui->steveBtn->setChecked(resolvedVariant == QStringLiteral("classic"));
    m_ui->alexBtn->setChecked(resolvedVariant == QStringLiteral("slim"));
    if (m_skinPreview)
        m_skinPreview->updateScene(m_catalogPreviewSkin.get());
    else
        m_skinPreviewLabel->setPixmap(QPixmap::fromImage(skinCardPreview(*m_catalogPreviewSkin))
                                          .scaled(m_skinPreviewLabel->size(), Qt::KeepAspectRatio, Qt::FastTransformation));
    renderCatalogCards();
    m_catalogStatus->setText(tr("Previewing %1. Choose Apply Skin to save and apply it.")
                                 .arg(m_pendingCatalogName.isEmpty() ? uuid.left(8) : m_pendingCatalogName));
}

void SkinManageDialog::downloadSelectedSkin()
{
    const auto* skin = getSelectedSkin();
    if (!skin) {
        m_catalogStatus->setText(tr("Select a skin before downloading."));
        return;
    }
    QByteArray bytes = m_pendingCatalogData;
    if (bytes.isEmpty()) {
        QFile file(skin->getPath());
        if (!file.open(QIODevice::ReadOnly) || file.size() <= 0 || file.size() > MAX_SKIN_API_RESPONSE_SIZE) {
            m_catalogStatus->setText(tr("Unable to read the selected skin PNG."));
            return;
        }
        bytes = file.readAll();
    }
    if (!isValidSkinPng(bytes)) {
        m_catalogStatus->setText(tr("The selected skin is not a valid 64×64 or 64×32 PNG."));
        return;
    }
    const auto path = QFileDialog::getSaveFileName(this, tr("Download Minecraft Skin"),
                                                   skin->name() + QStringLiteral(".png"),
                                                   QMimeDatabase().mimeTypeForName("image/png").filterString());
    if (path.isEmpty())
        return;
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        CustomMessageBox::selectable(this, tr("Skin download failed"), file.errorString(), QMessageBox::Warning)->show();
        return;
    }
    m_catalogStatus->setText(tr("PNG saved to %1").arg(path));
}

SkinModel* SkinManageDialog::getSelectedSkin()
{
    if (m_catalogPreviewSkin && !m_pendingCatalogData.isEmpty())
        return m_catalogPreviewSkin.get();
    if (auto* skin = m_list.skin(m_selectedSkinKey); skin && skin->isValid())
        return skin;
    return nullptr;
}

void SkinManageDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    QSize s = size() * (1. / 3);

    auto id = m_ui->capeCombo->currentData();
    auto cape = m_capes.value(id.toString(), {});
    if (!cape.isNull()) {
        m_ui->capeImage->setPixmap(previewCape(cape, m_ui->elytraCB->isChecked()).scaled(s, Qt::KeepAspectRatio, Qt::FastTransformation));
    } else {
        m_ui->capeImage->clear();
    }
    if (auto* skin = getSelectedSkin(); skin && !m_skinPreview) {
        m_skinPreviewLabel->setPixmap(
            QPixmap::fromImage(skinCardPreview(*skin)).scaled(m_skinPreviewLabel->size(), Qt::KeepAspectRatio, Qt::FastTransformation));
    }
}

QHash<QString, QImage> SkinManageDialog::capes()
{
    return m_capes;
}
