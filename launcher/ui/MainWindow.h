// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Authors: Andrew Okin
 *               Peterix
 *               Orochimarufan <orochimarufan.x3@gmail.com>
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#pragma once

#include <QMainWindow>
#include <QProcess>
#include <QSystemTrayIcon>
#include <QTimer>

#include "minecraft/auth/MinecraftAccount.h"

class LaunchController;
class NewsChecker;
class QToolButton;
class InstanceProxyModel;
class QComboBox;
class QLabel;
class QPushButton;
class QResizeEvent;
class QShowEvent;
class QStackedWidget;
class QWidget;
class QGridLayout;
class MinecraftInstance;
class MinecraftLauncher;
class BaseProfilerFactory;
class InstanceView;
class KonamiCode;
class InstanceTask;

namespace Ui {
class MainWindow;
}
class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

    bool eventFilter(QObject* obj, QEvent* ev) override;
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;

    void checkInstancePathForProblems();

    void updatesAllowedChanged(bool allowed);

    void processURLs(QList<QUrl> urls);
   signals:
    void isClosing();

   protected:
    QMenu* createPopupMenu() override;
    void resizeEvent(QResizeEvent* event) override;
#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
    void showEvent(QShowEvent* event) override;
#endif

   private slots:
    void onCatToggled(bool);

    void onCatChanged(int);

    void on_actionAbout_triggered();

    void on_actionAddInstance_triggered();

    void on_actionREDDIT_triggered();

    void on_actionMATRIX_triggered();

    void on_actionDISCORD_triggered();

    void on_actionCopyInstance_triggered();

    void on_actionChangeInstGroup_triggered();

    void on_actionChangeInstIcon_triggered();

    void on_actionViewLauncherRootFolder_triggered();

    void on_actionViewInstanceFolder_triggered();
    void on_actionViewCentralModsFolder_triggered();

    void on_actionViewIconThemeFolder_triggered();
    void on_actionViewWidgetThemeFolder_triggered();
    void on_actionViewCatPackFolder_triggered();
    void on_actionViewIconsFolder_triggered();
    void on_actionViewLogsFolder_triggered();
    void on_actionViewJavaFolder_triggered();

    void on_actionViewSkinsFolder_triggered();

    void on_actionViewSelectedInstFolder_triggered();

    void refreshInstances();

    void checkForUpdates();

    void on_actionSettings_triggered();

    void on_actionManageSkins_triggered();

    void on_actionManageAccounts_triggered();

    void on_actionReportBug_triggered();

    void on_actionClearMetadata_triggered();

#ifdef Q_OS_MAC
    void on_actionAddToPATH_triggered();
#endif

    void on_actionOpenWiki_triggered();

    void on_actionMoreNews_triggered();

    void newsButtonClicked();

    void on_actionLaunchInstance_triggered();

    void on_actionKillInstance_triggered();

    void on_actionDeleteInstance_triggered();

    void deleteGroup(QString group);
    void renameGroup(QString group);
    void undoTrashInstance();

    inline void on_actionExportInstance_triggered() { on_actionExportInstanceZip_triggered(); }
    void on_actionExportInstanceZip_triggered();
    void on_actionExportInstanceMrPack_triggered();
    void on_actionExportInstanceFlamePack_triggered();

    void on_actionRenameInstance_triggered();

    void on_actionEditInstance_triggered();

    void on_actionCreateInstanceShortcut_triggered();

    void taskEnd();

    /**
     * called when an icon is changed in the icon model.
     */
    void iconUpdated(QString);

    void showInstanceContextMenu(const QPoint&);

    void updateMainToolBar();

    void updateLaunchButton();

    void updateHomeDashboard();

    void updateThemeMenu();

    void instanceActivated(QModelIndex);

    void instanceChanged(const QModelIndex& current, const QModelIndex& previous);

    void instanceSelectRequest(QString id);

    void instanceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

    void selectionBad();

    void startTask(Task* task);

    void defaultAccountChanged();

    void changeActiveAccount();

    void repopulateAccountsMenu();

    void updateNewsLabel();

    void konamiTriggered();

    void globalSettingsClosed();

    void trayActivated(QSystemTrayIcon::ActivationReason reason);
    void toggleWindowVisibility();
    void quitFromTray();

    void setStatusBarVisibility(bool);

    void lockToolbars(bool);

#ifndef Q_OS_MAC
    void keyReleaseEvent(QKeyEvent* event) override;
#endif

    void refreshCurrentInstance();

   private:
    void retranslateUi();

    void updateNameLabel();

    void updateInstanceToolBarOrientation(Qt::Orientation orientation);

    void addInstance(const QString& url = QString(), const QMap<QString, QString>& extra_info = {});
    void setCatBackground(bool enabled);
    void updateCatState();
    void updateInstanceToolIcon(QString new_icon);
    void setSelectedInstanceById(const QString& id);
    void updateStatusCenter();
    void setInstanceActionsEnabled(bool enabled);
    void updateAccountCards();
    void scheduleAccountCardsUpdate();
    void reflowAccountCards();
    void updateLibraryCards();

    void runModalTask(Task* task);
    void instanceFromInstanceTask(InstanceTask* task);

   private:
    Ui::MainWindow* ui;
    // these are managed by Qt's memory management model!
    InstanceView* view = nullptr;
    InstanceProxyModel* proxymodel = nullptr;
    QStackedWidget* m_contentPages = nullptr;
    QStackedWidget* m_libraryContentStack = nullptr;
    QWidget* m_homePage = nullptr;
    QWidget* m_libraryPage = nullptr;
    QLabel* m_homeProfileName = nullptr;
    QLabel* m_homeProfileStatus = nullptr;
    QComboBox* m_homeInstanceSelector = nullptr;
    QComboBox* m_homeVersionSelector = nullptr;
    QComboBox* m_homeLoaderSelector = nullptr;
    QLabel* m_homeGreeting = nullptr;
    QLabel* m_homeGreetingSubtitle = nullptr;
    QLabel* m_homeAvatar = nullptr;
    QPushButton* m_homeAccountButton = nullptr;
    QPushButton* m_homePlayButton = nullptr;
    QPushButton* m_homeManageButton = nullptr;
    bool m_accountCardsUpdatePending = false;
    QToolButton* newsLabel = nullptr;
    QLabel* m_statusLeft = nullptr;
    QLabel* m_statusCenter = nullptr;
    QToolButton* changeIconButton = nullptr;
    QAction* instanceInfoWidget = nullptr;
    QLabel* nameLabel = nullptr;
    QToolButton* helpMenuButton = nullptr;
#ifdef Q_OS_WIN
    QToolButton* m_windowMaximizeButton = nullptr;
    void positionWindowControls();
#endif
    KonamiCode* secretEventFilter = nullptr;

    QSystemTrayIcon* m_trayIcon = nullptr;
    QMenu* m_trayMenu = nullptr;
    QAction* m_trayShowHideAction = nullptr;
    QAction* m_trayQuitAction = nullptr;

    unique_qobject_ptr<NewsChecker> m_newsChecker;

    MinecraftInstance* m_selectedInstance = nullptr;
    QString m_currentInstIcon;

    // managed by the application object
    Task* m_versionLoadTask = nullptr;
};
