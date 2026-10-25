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

#pragma once

#include <QDialog>
#include <QItemSelection>
#include <QLabel>
#include <QPixmap>
#include <QStackedWidget>
#include <QWidget>
#include <QByteArray>
#include <QHash>
#include <QJsonArray>
#include <QList>
#include <QByteArray>
#include <QSet>
#include <QStringList>
#include <memory>
#include "minecraft/auth/MinecraftAccount.h"
#include "minecraft/skins/SkinList.h"
#include "minecraft/skins/SkinModel.h"
#include "ui/dialogs/skins/draw/SkinOpenGLWindow.h"

namespace Ui {
class SkinManageDialog;
}
class QComboBox;
class QGridLayout;
class QLineEdit;
class QProgressBar;
class QSortFilterProxyModel;
class QPushButton;
class QScrollArea;
class QTimer;
class QToolButton;
class SkinManageDialog : public QDialog, public SkinProvider {
    Q_OBJECT
   public:
    explicit SkinManageDialog(QWidget* parent, MinecraftAccountPtr acct);
    ~SkinManageDialog() override;
    void resizeEvent(QResizeEvent* event) override;

    SkinModel* getSelectedSkin() override;
    QHash<QString, QImage> capes() override;

   public slots:
    void selectionChanged(const QItemSelection&, const QItemSelection&);
    void activated(QModelIndex);
    void delayed_scroll(QModelIndex);
    void on_openDirBtn_clicked();
    void on_fileBtn_clicked();
    void on_urlBtn_clicked();
    void on_userBtn_clicked();
    void on_browseSkinsBtn_clicked();
    void on_browseCatalogBtn_clicked();
    void accept() override;
    void on_capeCombo_currentIndexChanged(int index);
    void on_steveBtn_toggled(bool checked);
    void on_resetBtn_clicked();
    void show_context_menu(const QPoint& pos);
    bool eventFilter(QObject* obj, QEvent* ev) override;
    void on_action_Rename_Skin_triggered(bool checked);
    void on_action_Delete_Skin_triggered(bool checked);

   private:
    void setupCapes();
    void setupCatalogView();
    void setupStudioTabs();
    void setStudioTab(const QString& tab);
    void setApplyEnabled(bool enabled);
    void updateLocalSkinFilter();
    void loadStudioState();
    void saveStudioState() const;
    void toggleCatalogFavorite(const QJsonObject& skin);
    bool loadBundledSkinCatalog();
    bool importSkinArchive(const QString& archivePath, const QByteArray& archiveData = {});
    void showBundledSkinPage(int page);
    void selectCatalogSkin(const QString& uuid, const QString& name, const QString& variant, const QByteArray& skinData);
    void downloadSelectedSkin();
    void loadCatalogPage();
    void renderCatalogCards();
    void snapCatalogScrollPosition();
    void showCatalogLoadingState(const QString& message, bool error = false);
    void hideCatalogLoadingState();
    void loadCatalogThumbnail(const QString& uuid, const QString& variant, QToolButton* button);

   private:
    MinecraftAccountPtr m_acct;
    Ui::SkinManageDialog* m_ui;
    SkinList m_list;
    QString m_selectedSkinKey;
    QHash<QString, QImage> m_capes;
    QHash<QString, int> m_capesIdx;
    SkinOpenGLWindow* m_skinPreview = nullptr;
    QLabel* m_skinPreviewLabel = nullptr;
    QStackedWidget* m_skinViews = nullptr;
    QWidget* m_catalogPage = nullptr;
    QLabel* m_catalogStatus = nullptr;
    QLabel* m_skinNameLabel = nullptr;
    QLineEdit* m_catalogApiUrl = nullptr;
    QLineEdit* m_catalogSearch = nullptr;
    QComboBox* m_catalogSort = nullptr;
    QGridLayout* m_catalogGrid = nullptr;
    QScrollArea* m_catalogScrollArea = nullptr;
    QWidget* m_catalogLoadingOverlay = nullptr;
    QLabel* m_catalogLoadingLabel = nullptr;
    QProgressBar* m_catalogLoadingProgress = nullptr;
    QTimer* m_catalogScrollSnapTimer = nullptr;
    QPushButton* m_catalogConnectButton = nullptr;
    QPushButton* m_catalogLoadingRetryButton = nullptr;
    QPushButton* m_catalogPreviousButton = nullptr;
    QPushButton* m_catalogNextButton = nullptr;
    QPushButton* m_downloadButton = nullptr;
    QWidget* m_catalogApiRow = nullptr;
    QSortFilterProxyModel* m_localFilterProxy = nullptr;
    QList<QToolButton*> m_catalogCategoryButtons;
    QList<QToolButton*> m_studioTabButtons;
    QJsonArray m_catalogSkins;
    QJsonArray m_bundledSkinCatalog;
    QJsonArray m_bundledSkinArchives;
    QJsonArray m_favoriteCatalogSkins;
    QHash<QString, QByteArray> m_catalogImages;
    QSet<QString> m_catalogImagesLoading;
    QStringList m_favoriteLocalSkins;
    QStringList m_recentLocalSkins;
    QString m_catalogCacheDir;
    QString m_studioStatePath;
    QString m_studioTab = QStringLiteral("free");
    QString m_catalogCategory = QStringLiteral("All");
    QString m_pendingCatalogUuid;
    QString m_pendingCatalogName;
    QString m_pendingCatalogVariant;
    QByteArray m_pendingCatalogData;
    std::unique_ptr<SkinModel> m_catalogPreviewSkin;
    QString m_catalogAfter;
    QString m_catalogNextAfter;
    QStringList m_catalogCursorHistory;
    bool m_catalogLoading = false;
    bool m_bundledCatalogLoading = false;
    int m_catalogColumns = 0;
    int m_catalogCardWidth = 0;
    int m_catalogCardHeight = 0;
    int m_bundledPage = 0;
    bool m_bundledCatalogReady = false;
    bool m_catalogScrollDragging = false;
};
