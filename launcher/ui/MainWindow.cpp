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

#include "Application.h"
#include "BuildConfig.h"
#include "FileSystem.h"

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QUrlQuery>
#include <QVariant>

#include <QAction>
#include <QActionGroup>
#include <QAbstractButton>
#include <QApplication>
#include <QButtonGroup>
#include <QComboBox>
#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QImage>
#include <QInputDialog>
#include <QIcon>
#include <QLinearGradient>
#include <QListWidget>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLocale>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProgressDialog>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QRegularExpression>
#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QResizeEvent>
#include <QScreen>
#include <QScrollArea>
#include <QShortcut>
#include <QStackedLayout>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QSignalBlocker>
#include <QTextDocument>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QWidgetAction>
#include <QWindow>
#include <cmath>
#include <functional>
#include <memory>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#undef OPTIONAL
#endif

#include <BaseInstance.h>
#include <BuildConfig.h>
#include <DesktopServices.h>
#include <InstanceList.h>
#include <MMCZip.h>
#include <icons/IconList.h>
#include <java/JavaInstallList.h>
#include <java/JavaUtils.h>
#include <launch/LaunchTask.h>
#include <minecraft/MinecraftInstance.h>
#include <minecraft/auth/AccountList.h>
#include <net/ApiRequest.h>
#include <net/NetJob.h>
#include <news/NewsChecker.h>
#include <tools/BaseProfiler.h>
#include <updater/ExternalUpdater.h>
#include "InstanceWindow.h"

#include "ui/GuiUtil.h"
#include "ui/ViewLogWindow.h"
#include "ui/dialogs/AboutDialog.h"
#include "ui/dialogs/CopyInstanceDialog.h"
#include "ui/dialogs/CreateShortcutDialog.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ExportInstanceDialog.h"
#include "ui/dialogs/ExportPackDialog.h"
#include "ui/dialogs/IconPickerDialog.h"
#include "ui/dialogs/ImportResourceDialog.h"
#include "ui/dialogs/NewInstanceDialog.h"
#include "ui/dialogs/NewsDialog.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui/dialogs/skins/SkinManageDialog.h"
#include "ui/instanceview/InstanceDelegate.h"
#include "ui/instanceview/InstanceProxyModel.h"
#include "ui/instanceview/InstanceView.h"
#include "ui/themes/ITheme.h"
#include "ui/themes/ThemeManager.h"

#include "minecraft/PackProfile.h"
#include "minecraft/VersionFile.h"
#include "minecraft/WorldList.h"
#include "minecraft/skins/SkinModel.h"
#include "minecraft/mod/ModFolderModel.h"
#include "minecraft/mod/ResourcePackFolderModel.h"
#include "minecraft/mod/ShaderPackFolderModel.h"
#include "minecraft/mod/TexturePackFolderModel.h"
#include "minecraft/mod/tasks/LocalResourceParse.h"

#include "modplatform/ModIndex.h"
#include "modplatform/flame/FlameAPI.h"
#include "modplatform/flame/FlameModIndex.h"
#include "modplatform/modrinth/ModrinthAPI.h"

#include "KonamiCode.h"

#include "InstanceCopyTask.h"
#include "InstanceDirUpdate.h"

#include "Json.h"

#include "MMCTime.h"

namespace {
QPixmap navigationGlyph(const QString& name, const QColor& color)
{
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);

    if (name == QStringLiteral("launch")) {
        QPainterPath gamepad;
        gamepad.moveTo(9, 8);
        gamepad.lineTo(23, 8);
        gamepad.cubicTo(26, 8, 27, 10, 28, 14);
        gamepad.lineTo(30, 22);
        gamepad.cubicTo(30.8, 25, 27.6, 27, 25.2, 24.6);
        gamepad.lineTo(21.3, 20.8);
        gamepad.lineTo(10.7, 20.8);
        gamepad.lineTo(6.8, 24.6);
        gamepad.cubicTo(4.4, 27, 1.2, 25, 2, 22);
        gamepad.lineTo(4, 14);
        gamepad.cubicTo(5, 10, 6, 8, 9, 8);
        gamepad.closeSubpath();
        painter.drawPath(gamepad);
        painter.setBrush(QColor("#111111"));
        painter.drawRoundedRect(QRectF(7, 12, 2.4, 8), 1, 1);
        painter.drawRoundedRect(QRectF(4.2, 14.8, 8, 2.4), 1, 1);
        painter.drawEllipse(QPointF(22.5, 14.7), 1.4, 1.4);
        painter.drawEllipse(QPointF(25.7, 17.5), 1.4, 1.4);
    } else if (name == QStringLiteral("minecraft")) {
        auto drawLayer = [&painter, &color](qreal y, qreal inset, qreal alpha) {
            QColor layerColor(color);
            layerColor.setAlpha(qRound(255 * alpha));
            QPainterPath layer;
            layer.moveTo(16, y);
            layer.lineTo(30 - inset, y + 6);
            layer.lineTo(16, y + 13);
            layer.lineTo(2 + inset, y + 6);
            layer.closeSubpath();
            painter.fillPath(layer, layerColor);
        };
        drawLayer(4, 0, 1.0);
        drawLayer(12, 3, 0.9);
        drawLayer(20, 6, 0.78);
    } else if (name == QStringLiteral("loadermods")) {
        QPainterPath puzzle;
        puzzle.moveTo(5, 6);
        puzzle.lineTo(11, 6);
        puzzle.cubicTo(10, 2, 13, 1, 16, 1);
        puzzle.cubicTo(19, 1, 22, 2, 21, 6);
        puzzle.lineTo(27, 6);
        puzzle.lineTo(27, 12);
        puzzle.cubicTo(31, 11, 32, 14, 32, 16);
        puzzle.cubicTo(32, 19, 31, 21, 27, 20);
        puzzle.lineTo(27, 27);
        puzzle.lineTo(20, 27);
        puzzle.cubicTo(21, 31, 18, 32, 16, 32);
        puzzle.cubicTo(13, 32, 11, 31, 12, 27);
        puzzle.lineTo(5, 27);
        puzzle.lineTo(5, 20);
        puzzle.cubicTo(1, 21, 0, 18, 0, 16);
        puzzle.cubicTo(0, 13, 1, 11, 5, 12);
        puzzle.closeSubpath();
        painter.drawPath(puzzle);
    } else if (name == QStringLiteral("accounts")) {
        painter.drawEllipse(QRectF(10, 3, 12, 13));
        QPainterPath shoulders;
        shoulders.moveTo(4, 29);
        shoulders.cubicTo(4, 21, 8, 18, 16, 18);
        shoulders.cubicTo(24, 18, 28, 21, 28, 29);
        shoulders.cubicTo(28, 31, 26, 32, 24, 32);
        shoulders.lineTo(8, 32);
        shoulders.cubicTo(6, 32, 4, 31, 4, 29);
        painter.drawPath(shoulders);
    } else if (name == QStringLiteral("help")) {
        auto star = [&painter](QPointF center, qreal outer, qreal inner) {
            QPainterPath path;
            for (int i = 0; i < 8; ++i) {
                const qreal angle = -1.57079632679 + i * 0.78539816339;
                const qreal radius = (i % 2 == 0) ? outer : inner;
                const QPointF point(center.x() + std::cos(angle) * radius, center.y() + std::sin(angle) * radius);
                if (i == 0)
                    path.moveTo(point);
                else
                    path.lineTo(point);
            }
            path.closeSubpath();
            painter.drawPath(path);
        };
        star(QPointF(16, 15), 12, 4.2);
        star(QPointF(27, 6), 3.2, 1.2);
        star(QPointF(6, 27), 3.1, 1.1);
    } else if (name == QStringLiteral("news")) {
        QPainterPath page;
        page.addRoundedRect(QRectF(5, 3, 21, 27), 3, 3);
        painter.drawPath(page);
        painter.setBrush(QColor("#111111"));
        painter.drawRoundedRect(QRectF(9, 10, 12, 2), 1, 1);
        painter.drawRoundedRect(QRectF(9, 15, 12, 2), 1, 1);
        painter.drawRoundedRect(QRectF(9, 20, 9, 2), 1, 1);
        painter.drawRoundedRect(QRectF(21, 4, 2, 4), 1, 1);
    } else if (name == QStringLiteral("settings")) {
        QPainterPath gear;
        for (int i = 0; i < 24; ++i) {
            const qreal angle = -1.57079632679 + i * 0.2617993878;
            const qreal radius = i % 3 == 1 ? 10.5 : 14.0;
            const QPointF point(16 + std::cos(angle) * radius, 16 + std::sin(angle) * radius);
            if (i == 0)
                gear.moveTo(point);
            else
                gear.lineTo(point);
        }
        gear.closeSubpath();
        gear.addEllipse(QRectF(12, 12, 8, 8));
        gear.setFillRule(Qt::OddEvenFill);
        painter.drawPath(gear);
    } else if (name == QStringLiteral("discord")) {
        QPainterPath discord;
        discord.moveTo(9, 5);
        discord.cubicTo(12, 3, 20, 3, 23, 5);
        discord.cubicTo(27, 7, 29, 13, 30, 22);
        discord.cubicTo(27, 26, 24, 28, 20, 29);
        discord.lineTo(18, 26);
        discord.cubicTo(21, 25, 23, 24, 25, 22);
        discord.cubicTo(19, 25, 13, 25, 7, 22);
        discord.cubicTo(9, 24, 11, 25, 14, 26);
        discord.lineTo(12, 29);
        discord.cubicTo(8, 28, 5, 26, 2, 22);
        discord.cubicTo(3, 13, 5, 7, 9, 5);
        discord.closeSubpath();
        painter.drawPath(discord);
        painter.setBrush(QColor("#111111"));
        painter.drawEllipse(QPointF(11.5, 16), 2, 2.4);
        painter.drawEllipse(QPointF(20.5, 16), 2, 2.4);
    }

    painter.end();
    return pixmap;
}

QIcon navigationIcon(const QString& name)
{
    if (name == QStringLiteral("minecraft")) {
        return QIcon(QStringLiteral(":/icons/pe_colored/scalable/minecraft.svg"));
    }

    const QColor normalColor = QColor("#E7E7E8");
    const QColor selectedColor = QColor("#F28A1D");
    QIcon icon;
    icon.addPixmap(navigationGlyph(name, normalColor), QIcon::Normal, QIcon::Off);
    icon.addPixmap(navigationGlyph(name, selectedColor), QIcon::Normal, QIcon::On);
    return icon;
}

QPixmap orangeTintedPixmap(const QPixmap& pixmap, int saturationLimit, int valueLimit)
{
    if (pixmap.isNull()) {
        return {};
    }

    QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < image.height(); ++y) {
        auto* row = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            QColor pixel = QColor::fromRgba(row[x]);
            int hue = 0;
            int saturation = 0;
            int value = 0;
            int alpha = 0;
            pixel.getHsv(&hue, &saturation, &value, &alpha);
            if (alpha > 0 && saturation > 20) {
                pixel.setHsv(28, qMin(saturation, saturationLimit), qMin(value, valueLimit), alpha);
                row[x] = pixel.rgba();
            }
        }
    }
    return QPixmap::fromImage(image);
}

class BackgroundFrame final : public QFrame
{
public:
    explicit BackgroundFrame(QWidget* parent = nullptr) : QFrame(parent) {}

    void setBackgroundPixmap(QPixmap pixmap)
    {
        m_background = orangeTintedPixmap(pixmap, 170, 175);
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        const QRectF bounds = rect().adjusted(1, 1, -1, -1);
        QPainterPath clipPath;
        clipPath.addRoundedRect(bounds, 12, 12);
        painter.setClipPath(clipPath);

        if (m_background.isNull() || size().isEmpty()) {
            painter.fillRect(rect(), QColor("#111B2C"));
        } else {
            QRect source = m_background.rect();
            const qreal frameAspectRatio = qreal(width()) / height();
            const qreal imageAspectRatio = qreal(m_background.width()) / m_background.height();
            if (imageAspectRatio > frameAspectRatio) {
                const int sourceWidth = qRound(m_background.height() * frameAspectRatio);
                source.setLeft((m_background.width() - sourceWidth) / 2);
                source.setWidth(sourceWidth);
            } else {
                const int sourceHeight = qRound(m_background.width() / frameAspectRatio);
                source.setTop((m_background.height() - sourceHeight) / 2);
                source.setHeight(sourceHeight);
            }
            painter.drawPixmap(rect(), m_background, source);
            QLinearGradient shade(rect().topLeft(), rect().topRight());
            shade.setColorAt(0.0, QColor(5, 10, 20, 140));
            shade.setColorAt(0.55, QColor(5, 10, 20, 80));
            shade.setColorAt(1.0, QColor(5, 10, 20, 18));
            painter.fillRect(rect(), shade);
            QLinearGradient lowerShade(rect().topLeft(), rect().bottomLeft());
            lowerShade.setColorAt(0.0, QColor(3, 9, 22, 8));
            lowerShade.setColorAt(0.7, QColor(3, 9, 22, 24));
            lowerShade.setColorAt(1.0, QColor(3, 9, 22, 48));
            painter.fillRect(rect(), lowerShade);
        }

        painter.setClipping(false);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor("#26354C"), 1));
        painter.drawRoundedRect(bounds, 12, 12);
    }

private:
    QPixmap m_background;
};

class PlayerProfileBanner final : public QFrame
{
public:
    explicit PlayerProfileBanner(QWidget* parent = nullptr) : QFrame(parent) {}

    void setBackgroundPixmap(QPixmap pixmap)
    {
        m_background = std::move(pixmap);
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        const QRectF bounds = rect().adjusted(1, 1, -1, -1);
        QPainterPath clipPath;
        clipPath.addRoundedRect(bounds, 12, 12);
        painter.setClipPath(clipPath);

        if (!m_background.isNull() && !size().isEmpty()) {
            QRect source = m_background.rect();
            const qreal frameAspectRatio = qreal(width()) / height();
            const qreal imageAspectRatio = qreal(m_background.width()) / m_background.height();
            if (imageAspectRatio > frameAspectRatio) {
                const int sourceWidth = qRound(m_background.height() * frameAspectRatio);
                source.setLeft((m_background.width() - sourceWidth) / 2);
                source.setWidth(sourceWidth);
            } else {
                const int sourceHeight = qRound(m_background.width() / frameAspectRatio);
                source.setTop((m_background.height() - sourceHeight) / 2);
                source.setHeight(sourceHeight);
            }
            painter.drawPixmap(rect(), m_background, source);
        } else {
            painter.fillRect(rect(), QColor("#151515"));
        }

        QLinearGradient shade(rect().topLeft(), rect().topRight());
        shade.setColorAt(0.0, QColor(5, 5, 5, 200));
        shade.setColorAt(0.42, QColor(5, 5, 5, 132));
        shade.setColorAt(1.0, QColor(5, 5, 5, 38));
        painter.fillRect(rect(), shade);

        painter.setClipping(false);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor("#353535"), 1));
        painter.drawRoundedRect(bounds, 12, 12);
    }

private:
    QPixmap m_background;
};

class HomeDashboardScrollArea final : public QScrollArea
{
public:
    using QScrollArea::QScrollArea;

    void setDashboardSections(QWidget* content, QWidget* news)
    {
        m_content = content;
        m_news = news;
        updateContentHeight();
    }

protected:
    void resizeEvent(QResizeEvent* event) override
    {
        QScrollArea::resizeEvent(event);
        updateContentHeight();
    }

private:
    void updateContentHeight()
    {
        if (!m_content || !m_news) {
            return;
        }

        const int contentHeight = viewport()->height() + m_news->sizeHint().height();
        if (m_content->minimumHeight() != contentHeight) {
            m_content->setMinimumHeight(contentHeight);
        }
    }

    QWidget* m_content = nullptr;
    QWidget* m_news = nullptr;
};

class ClickableInstanceFrame final : public QFrame
{
public:
    explicit ClickableInstanceFrame(QWidget* parent = nullptr) : QFrame(parent)
    {
        setCursor(Qt::PointingHandCursor);
    }

    void setClickHandler(std::function<void()> handler)
    {
        m_clickHandler = std::move(handler);
    }

protected:
    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && rect().contains(event->pos()) && m_clickHandler) {
            m_clickHandler();
        }
        QFrame::mouseReleaseEvent(event);
    }

    void paintEvent(QPaintEvent* event) override
    {
        QFrame::paintEvent(event);

        static const QPixmap artwork(QStringLiteral(":/backgrounds/boostclient-instance-grass"));
        if (artwork.isNull() || size().isEmpty()) {
            return;
        }

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        const QRectF bounds = rect().adjusted(1, 1, -1, -1);
        QPainterPath clipPath;
        clipPath.addRoundedRect(bounds, 10, 10);
        painter.setClipPath(clipPath);

        const int artworkSize = qBound(96, height() * 3 / 5, 150);
        const QRect artworkRect(width() - artworkSize - 4, height() - artworkSize + 12, artworkSize, artworkSize);
        painter.setOpacity(0.42);
        painter.drawPixmap(artworkRect, artwork);
        painter.setOpacity(1.0);

        QLinearGradient shade(rect().topLeft(), rect().topRight());
        shade.setColorAt(0.0, QColor(0, 0, 0, 8));
        shade.setColorAt(0.58, QColor(0, 0, 0, 20));
        shade.setColorAt(1.0, QColor(0, 0, 0, 55));
        painter.fillRect(rect(), shade);

        painter.setClipping(false);
        const bool selected = property("selected").toBool();
        painter.setPen(QPen(selected ? QColor("#F28A1D") : (underMouse() ? QColor("#5A5B5D") : QColor("#303030")),
                            selected ? 2 : 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(bounds, 10, 10);
    }

private:
    std::function<void()> m_clickHandler;
};

QPixmap grassBlockIcon()
{
    QPixmap pixmap(80, 88);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#78D94E"));
    painter.drawPolygon(QPolygon{ QPoint(40, 2), QPoint(78, 20), QPoint(40, 39), QPoint(2, 20) });
    painter.setBrush(QColor("#8BE45C"));
    painter.drawPolygon(QPolygon{ QPoint(40, 2), QPoint(60, 12), QPoint(40, 22), QPoint(20, 12) });
    painter.setBrush(QColor("#55B83B"));
    painter.drawPolygon(QPolygon{ QPoint(2, 20), QPoint(40, 39), QPoint(40, 86), QPoint(2, 66) });
    painter.setBrush(QColor("#38932F"));
    painter.drawPolygon(QPolygon{ QPoint(40, 39), QPoint(78, 20), QPoint(78, 66), QPoint(40, 86) });
    painter.setBrush(QColor("#71523D"));
    painter.drawPolygon(QPolygon{ QPoint(2, 27), QPoint(40, 46), QPoint(40, 53), QPoint(2, 34) });
    painter.drawPolygon(QPolygon{ QPoint(40, 46), QPoint(78, 27), QPoint(78, 34), QPoint(40, 53) });
    painter.setBrush(QColor("#467F32"));
    painter.drawRect(QRect(10, 27, 7, 4));
    painter.drawRect(QRect(28, 37, 5, 4));
    painter.drawRect(QRect(55, 30, 6, 4));
    painter.setPen(QPen(QColor(202, 251, 168, 180), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawPolygon(QPolygon{ QPoint(40, 2), QPoint(78, 20), QPoint(78, 66), QPoint(40, 86), QPoint(2, 66), QPoint(2, 20) });
    return pixmap;
}

QIcon launchButtonIcon()
{
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#03203A"));
    painter.drawPolygon(QPolygon{ QPoint(7, 4), QPoint(20, 12), QPoint(7, 20) });
    return QIcon(pixmap);
}

QString profileInUseFilter(const QString& profile, bool used)
{
    if (used) {
        return QObject::tr("%1 (in use)").arg(profile);
    } else {
        return profile;
    }
}

QString installedMinecraftVersion(MinecraftInstance* instance)
{
    const auto version = instance->getPackProfile()->getComponentVersion(QStringLiteral("net.minecraft"));
    return version.isEmpty() ? QObject::tr("Unknown version") : version;
}

QString installedLoaderLabel(MinecraftInstance* instance)
{
    const auto loaders = instance->getPackProfile()->getModLoadersList();
    if (loaders.isEmpty()) {
        return QObject::tr("Vanilla");
    }

    QStringList names;
    for (const auto loader : loaders) {
        const auto name = ModPlatform::getModLoaderAsString(loader);
        if (!names.contains(name)) {
            names.append(name);
        }
    }
    return names.join(QStringLiteral(" + "));
}

QIcon instanceTypeIcon(MinecraftInstance* instance)
{
    const auto loaders = instance->getPackProfile()->getModLoadersList();
    if (loaders.contains(ModPlatform::ModLoaderType::NeoForge)) {
        return QIcon(QStringLiteral(":/icons/multimc/scalable/instances/neoforged.svg"));
    }
    if (loaders.contains(ModPlatform::ModLoaderType::Forge) || loaders.contains(ModPlatform::ModLoaderType::Cauldron) ||
        loaders.contains(ModPlatform::ModLoaderType::Rift)) {
        return QIcon(QStringLiteral(":/icons/multimc/128x128/instances/forge.png"));
    }
    if (loaders.contains(ModPlatform::ModLoaderType::Quilt)) {
        return QIcon(QStringLiteral(":/icons/multimc/scalable/instances/quiltmc.svg"));
    }
    if (loaders.contains(ModPlatform::ModLoaderType::Fabric) || loaders.contains(ModPlatform::ModLoaderType::Babric) ||
        loaders.contains(ModPlatform::ModLoaderType::LegacyFabric) || loaders.contains(ModPlatform::ModLoaderType::Ornithe) ||
        loaders.contains(ModPlatform::ModLoaderType::BTA)) {
        return QIcon(QStringLiteral(":/icons/multimc/scalable/instances/fabricmc.svg"));
    }
    if (loaders.contains(ModPlatform::ModLoaderType::LiteLoader)) {
        return QIcon(QStringLiteral(":/icons/multimc/128x128/instances/liteloader.png"));
    }
    if (loaders.isEmpty()) {
        return QIcon(QStringLiteral(":/backgrounds/boostclient-grasss"));
    }
    return {};
}

class NavigationButtonAnimation final : public QObject
{
public:
    explicit NavigationButtonAnimation(QToolButton* button)
        : QObject(button), m_button(button), m_opacity(new QGraphicsOpacityEffect(button)),
          m_animation(new QPropertyAnimation(m_opacity, "opacity", this))
    {
        button->setGraphicsEffect(m_opacity);
        m_opacity->setOpacity(button->isChecked() ? 1.0 : 0.72);
        m_animation->setDuration(160);
        m_animation->setEasingCurve(QEasingCurve::OutCubic);
        button->installEventFilter(this);
        QObject::connect(button, &QToolButton::toggled, this, [this](bool checked) {
            animateTo(checked || m_button->underMouse() ? 1.0 : 0.72);
        });
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == m_button && (event->type() == QEvent::Enter || event->type() == QEvent::Leave)) {
            animateTo(event->type() == QEvent::Enter || m_button->isChecked() ? 1.0 : 0.72);
        }
        return QObject::eventFilter(watched, event);
    }

private:
    void animateTo(qreal opacity)
    {
        m_animation->stop();
        m_animation->setStartValue(m_opacity->opacity());
        m_animation->setEndValue(opacity);
        m_animation->start();
    }

    QToolButton* m_button;
    QGraphicsOpacityEffect* m_opacity;
    QPropertyAnimation* m_animation;
};

class WindowControlAnimation final : public QObject
{
public:
    explicit WindowControlAnimation(QToolButton* button)
        : QObject(button), m_button(button), m_opacity(new QGraphicsOpacityEffect(button)),
          m_animation(new QPropertyAnimation(m_opacity, "opacity", this))
    {
        button->setGraphicsEffect(m_opacity);
        m_opacity->setOpacity(0.82);
        m_animation->setDuration(140);
        m_animation->setEasingCurve(QEasingCurve::OutCubic);
        button->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == m_button && (event->type() == QEvent::Enter || event->type() == QEvent::Leave)) {
            m_animation->stop();
            m_animation->setStartValue(m_opacity->opacity());
            m_animation->setEndValue(event->type() == QEvent::Enter ? 1.0 : 0.82);
            m_animation->start();
        }
        return QObject::eventFilter(watched, event);
    }

private:
    QToolButton* m_button;
    QGraphicsOpacityEffect* m_opacity;
    QPropertyAnimation* m_animation;
};

#ifdef Q_OS_WIN
class WindowDragBar final : public QWidget
{
   public:
    using QWidget::QWidget;

   protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            if (QWindow* handle = window()->windowHandle()) {
                handle->startSystemMove();
            }
        }
        QWidget::mousePressEvent(event);
    }

    void mouseDoubleClickEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            if (QWidget* topLevel = window()) {
                topLevel->isMaximized() ? topLevel->showNormal() : topLevel->showMaximized();
            }
        }
        QWidget::mouseDoubleClickEvent(event);
    }
};
#endif
}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
#ifdef Q_OS_WIN
    setWindowFlag(Qt::WindowTitleHint, false);
    setWindowFlag(Qt::FramelessWindowHint);
#endif

    m_trayIcon = new QSystemTrayIcon(APPLICATION->logo(), this);
    m_trayMenu = new QMenu(this);

    m_trayShowHideAction = new QAction(tr("Show / Hide"), this);
    connect(m_trayShowHideAction, &QAction::triggered, this, &MainWindow::toggleWindowVisibility);
    m_trayMenu->addAction(m_trayShowHideAction);

    m_trayQuitAction = new QAction(tr("Exit"), this);
    connect(m_trayQuitAction, &QAction::triggered, this, &MainWindow::quitFromTray);
    m_trayMenu->addAction(m_trayQuitAction);

    m_trayIcon->setContextMenu(m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::trayActivated);
    m_trayIcon->show();

    setStyleSheet(QString::fromUtf8(R"BOOSTQSS(
        QMainWindow#MainWindow, QWidget#centralWidget, QStackedWidget {
            background: #080D18;
            color: #FFFFFF;
            font-family: "Segoe UI";
            font-size: 9pt;
        }
        QWidget { color: #F2F6FF; background-color: #080D18; }
        QLabel { background: transparent; }
        QScrollArea, QScrollArea QWidget { background: transparent; border: 0; }
        QWidget#boostSidebar {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                        stop:0 #080F1C, stop:0.52 #060B14, stop:1 #03070E);
            border-right: 1px solid #12344C;
        }
        QLabel#boostBrandMark {
            background: #111B2C;
            color: #FFFFFF;
            border: 1px solid #26354C;
            border-radius: 9px;
            font-size: 12pt;
            font-weight: 800;
        }
        QLabel#boostBrand {
            color: #F2F6FF;
            font-size: 11pt;
            font-weight: 700;
            letter-spacing: 0.06em;
        }
        QLabel#boostBrandCaption, QLabel#boostNavGroup, QLabel#boostKicker,
        QLabel#boostHomeIntro, QLabel#boostProfileStatus, QLabel#boostPanelCaption {
            color: #F2F6FF;
        }
        QLabel#boostBrandCaption, QLabel#boostNavGroup, QLabel#boostKicker {
            font-size: 8pt;
            font-weight: 700;
            letter-spacing: 1px;
        }
        QToolButton#boostNavButton {
            background: transparent;
            color: #C0D7EF;
            border: 1px solid rgba(54, 223, 255, 0);
            border-left: 3px solid transparent;
            border-radius: 2px;
            min-height: 32px;
            padding: 0 16px;
            font-size: 9pt;
            font-weight: 700;
            text-align: left;
        }
        QToolButton#boostNavButton:hover {
            background: rgba(30, 125, 164, 0.11);
            border-color: rgba(54, 223, 255, 0.18);
            border-left-color: rgba(54, 223, 255, 0.55);
            color: #E5F8FF;
        }
        QToolButton#boostNavButton:checked {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 rgba(19, 142, 220, 0.52),
                                        stop:0.48 rgba(10, 81, 143, 0.38),
                                        stop:1 rgba(7, 27, 48, 0.12));
            border-color: rgba(54, 223, 255, 0.42);
            border-left-color: #32DFFF;
            color: #F3FCFF;
        }
        QToolButton#boostNavButton:checked:hover {
            border-color: rgba(87, 232, 255, 0.6);
        }
        QWidget#boostTopBar {
            background: transparent;
            border: 0;
        }
        QToolButton#boostWindowMinimize, QToolButton#boostWindowMaximize, QToolButton#boostWindowClose {
            background: transparent;
            color: #B8B8B8;
            border: 1px solid transparent;
            border-radius: 7px;
            font-family: "Segoe UI Symbol";
            font-size: 12pt;
            font-weight: 500;
        }
        QToolButton#boostWindowMinimize:hover, QToolButton#boostWindowMaximize:hover {
            background: #242424;
            border-color: #343434;
            color: #FFFFFF;
        }
        QToolButton#boostWindowClose:hover {
            background: #C94136;
            border-color: #E05A4E;
            color: #FFFFFF;
        }
        QToolButton#boostWindowMinimize:pressed, QToolButton#boostWindowMaximize:pressed {
            background: #303030;
        }
        QToolButton#boostWindowClose:pressed { background: #A83229; }
        QToolBar#mainToolBar {
            background: #101010;
            border: 0;
            border-bottom: 1px solid #303030;
            spacing: 8px;
            min-height: 54px;
            padding: 4px 14px;
        }
        QToolBar::handle { image: none; width: 0px; }
        QToolBar#instanceToolBar {
            background: #101010;
            border: 0;
            border-left: 1px solid #303030;
            spacing: 5px;
            padding: 8px 5px;
        }
        QToolBar QToolButton {
            background: transparent;
            color: #FFFFFF;
            border: 1px solid transparent;
            border-radius: 6px;
            padding: 6px;
        }
        QToolBar QToolButton:hover {
            background: #1E1E1E;
            border-color: #303030;
        }
        QPushButton#boostAccountMenuButton {
            background: #141414;
            color: #FFFFFF;
            border: 1px solid #303030;
            border-radius: 8px;
            min-height: 38px;
            padding: 0 12px;
            font-weight: 700;
        }
        QPushButton#boostAccountMenuButton:hover {
            background: #1E1E1E;
            border-color: #35D9FF;
        }
        QFrame#boostHero {
            background: transparent;
            border: 0;
        }
    )BOOSTQSS") + QString::fromUtf8(R"BOOSTQSS(
        QFrame#boostDashboardPanel {
            background: #141414;
            border: 1px solid #303030;
            border-radius: 12px;
        }
        QFrame#boostAccountCard {
            background: #141414;
            border: 1px solid #303030;
            border-radius: 12px;
        }
        QFrame#boostAccountCard[selected="true"] {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                        stop:0 #21180F, stop:1 #15120F);
            border: 2px solid #F28A1D;
        }
        QScrollArea#boostAccountScrollArea, QWidget#boostAccountCards {
            background: transparent;
            border: 0;
        }
        QFrame#boostAccountProfileHero {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                       stop:0 #171717, stop:0.72 #21170F, stop:1 #17120D);
            border: 1px solid #49331F;
            border-radius: 12px;
        }
        QLabel#boostAccountProfileAvatar {
            background: #101010;
            border: 1px solid #5C3B1D;
            border-radius: 10px;
        }
        QLabel#boostAccountProfileKicker {
            color: #F28A1D;
            background: transparent;
            font-size: 9pt;
            font-weight: 700;
        }
        QLabel#boostAccountProfileName {
            color: #FFFFFF;
            background: transparent;
            font-size: 16pt;
            font-weight: 700;
        }
        QLabel#boostAccountProfileStatus {
            color: #A8A8A8;
            background: transparent;
        }
        QPushButton#boostAccountManageButton {
            color: #F5F5F5;
            background: #1D1D1D;
            border: 1px solid #38332E;
            border-radius: 7px;
            min-width: 148px;
            max-width: 148px;
            min-height: 36px;
            max-height: 36px;
            padding: 0 12px;
            font-weight: 600;
            text-align: center;
        }
        QPushButton#boostAccountManageButton:hover {
            color: #FFFFFF;
            background: #29221A;
            border-color: #F28A1D;
        }
        QLabel#boostAccountHeadingIcon {
            background: #21170F;
            border: 1px solid #49331F;
            border-radius: 12px;
        }
        QLabel#boostAccountCardName {
            color: #F5F5F5;
            font-size: 14pt;
            font-weight: 700;
        }
        QLabel#boostAccountFooter {
            color: #A8A8A8;
            padding: 3px 0;
        }
        QFrame#boostAccountFooterDivider {
            background: #303030;
            border: 0;
            max-height: 1px;
        }
        QLabel#boostAccountAvatar {
            background: #101010;
            border: 1px solid #49331F;
            border-radius: 9px;
        }
        QLabel#boostAccountStatus, QLabel#boostAccountEdition {
            color: #F28A1D;
        }
        QLabel#boostAccountDivider {
            background: #303030;
            border: 0;
            max-height: 1px;
        }
        QLabel#boostSelectedAccountBadge {
            color: #17120D;
            background: #F28A1D;
            border-radius: 8px;
            padding: 4px 9px;
            font-weight: 700;
        }
        QToolButton#boostAccountCardAction {
            background: #1B1B1B;
            color: #D6D6D6;
            border: 1px solid #303030;
            border-radius: 7px;
            padding: 5px;
            font-weight: 600;
        }
        QToolButton#boostAccountCardAction:hover {
            border-color: #F28A1D;
            background: #29221A;
        }
        QToolButton#boostAccountCardAction:disabled {
            color: #777777;
            background: #171717;
            border-color: #292929;
        }
        QToolButton#boostAccountCardAction[selected="true"] {
            color: #17120D;
            background: #F28A1D;
            border-color: #F28A1D;
        }
    )BOOSTQSS") + QString::fromUtf8(R"BOOSTQSS(
        QLabel#boostProfileAvatar {
            background: #1E1E1E;
            border: 1px solid #303030;
            border-radius: 10px;
        }
        QLabel#boostPlayerProfileName {
            color: #FFFFFF;
            font-size: 16pt;
            font-weight: 700;
        }
        QLabel#boostPlayerProfileDescription { color: #FFFFFF; }
        QLabel#boostHomeTitle, QLabel#boostLibraryTitle {
            color: #FFFFFF;
            font-size: 25pt;
            font-weight: 800;
        }
        QLabel#boostProfileName, QLabel#boostPanelMetric {
            color: #FFFFFF;
            font-size: 17pt;
            font-weight: 700;
        }
        QLabel#boostHeroArtwork {
            background: #1E1E1E;
            border: 1px solid #303030;
            border-radius: 12px;
        }
        QPushButton#boostPlayButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #77ECFF, stop:1 #39C8FF);
            color: #07131F;
            border: 1px solid rgba(126, 234, 255, 0.85);
            border-radius: 12px;
            min-height: 62px;
            padding: 0 24px;
            font-size: 18pt;
            font-weight: 800;
            letter-spacing: 0.04em;
        }
        QPushButton#boostPlayButton:hover { background: #A5F4FF; }
        QPushButton#boostPlayButton:pressed { background: #1BB8EA; }
        QPushButton#boostPlayButton:disabled {
            background: #303030;
            color: #FFFFFF;
        }
        QPushButton#boostSecondaryButton, QPushButton {
            background: #1E1E1E;
            color: #FFFFFF;
            border: 1px solid #303030;
            border-radius: 7px;
            padding: 7px 12px;
        }
        QPushButton#boostSecondaryButton:hover, QPushButton:hover {
            border-color: #35D9FF;
        }
        QPushButton#boostSecondaryButton:disabled, QPushButton:disabled {
            color: #FFFFFF;
            background: #141414;
            border-color: #303030;
        }
        QComboBox, QLineEdit, QSpinBox, QDoubleSpinBox {
            background: #141414;
            color: #FFFFFF;
            border: 1px solid #303030;
            border-radius: 6px;
            min-height: 32px;
            padding: 0 9px;
        }
        QComboBox QAbstractItemView {
            background: #141414;
            color: #FFFFFF;
            selection-background-color: #1E1E1E;
            selection-color: #FFFFFF;
        }
        QMenu {
            background: #141414;
            color: #FFFFFF;
            border: 1px solid #303030;
            padding: 5px;
        }
        QMenu::item { color: #FFFFFF; padding: 7px 22px 7px 10px; }
        QMenu::item:selected { background: #1E1E1E; }
        QMenu::separator { height: 1px; background: #303030; margin: 4px 6px; }
        QToolTip {
            background: #141414;
            color: #FFFFFF;
            border: 1px solid #303030;
            padding: 5px;
        }
        QDialog, QMessageBox { background: #080808; color: #FFFFFF; }
        QCheckBox, QRadioButton { color: #FFFFFF; spacing: 7px; }
        QCheckBox::indicator, QRadioButton::indicator {
            width: 15px;
            height: 15px;
            border: 1px solid #92A1B6;
            background: #141414;
        }
        QCheckBox::indicator { border-radius: 3px; }
        QRadioButton::indicator { border-radius: 8px; }
        QCheckBox::indicator:checked, QRadioButton::indicator:checked {
            background: #35D9FF;
            border-color: #35D9FF;
        }
        QGroupBox {
            color: #FFFFFF;
            border: 1px solid #303030;
            border-radius: 8px;
            margin-top: 12px;
            padding: 12px 8px 8px;
        }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }
        QTabWidget::pane { background: #101010; border: 1px solid #303030; }
        QTabBar::tab {
            background: #141414;
            color: #FFFFFF;
            border: 1px solid #303030;
            padding: 8px 12px;
        }
        QTabBar::tab:selected { background: #1E1E1E; border-bottom-color: #35D9FF; }
        QProgressBar {
            background: #141414;
            color: #FFFFFF;
            border: 1px solid #303030;
            border-radius: 5px;
            text-align: center;
        }
        QProgressBar::chunk { background: #35D9FF; border-radius: 4px; }
        QListWidget#boostNewsList {
            background: #101010;
            color: #FFFFFF;
            border: 1px solid #303030;
            border-radius: 8px;
            padding: 6px;
        }
        QListWidget#boostNewsList::item {
            color: #FFFFFF;
            padding: 12px 10px;
            border-bottom: 1px solid #303030;
        }
        QListWidget#boostNewsList::item:hover,
        QListWidget#boostNewsList::item:selected {
            background: #1E1E1E;
            color: #FFFFFF;
        }
        QToolButton#boostTopbarAction {
            background: transparent;
            color: #FFFFFF;
            border: 1px solid transparent;
            border-radius: 7px;
            min-width: 36px;
            min-height: 36px;
        }
        QToolButton#boostTopbarAction:hover {
            background: #1E1E1E;
            border-color: #303030;
        }
        QTreeView, QListView, QTableView {
            background: #101010;
            alternate-background-color: #141414;
            color: #FFFFFF;
            border: 1px solid #303030;
            selection-background-color: #1E1E1E;
            selection-color: #FFFFFF;
        }
        QHeaderView::section {
            background: #141414;
            color: #FFFFFF;
            border: 0;
            border-bottom: 1px solid #303030;
            padding: 7px;
            font-weight: 700;
        }
        QStatusBar {
            background: #101010;
            color: #FFFFFF;
            border: 0;
        }
        QScrollBar:vertical { background: #101010; width: 10px; margin: 0; }
        QScrollBar::handle:vertical { background: #303030; min-height: 28px; border-radius: 5px; }
        QScrollBar::handle:vertical:hover { background: #35D9FF; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QWidget#boostBrandBlock { background: transparent; }
        QLabel#boostBrandCaption, QLabel#boostNavGroup, QLabel#boostPanelCaption,
        QLabel#boostHomeNewsStatus {
            color: #8FA2BD;
            background: transparent;
        }
        QLabel#boostBrandCaption, QLabel#boostNavGroup { letter-spacing: 1px; }
        QToolButton#boostNavButton {
            color: #C0D7EF;
            min-height: 32px;
            text-transform: none;
            letter-spacing: 0;
            border-radius: 2px;
            padding: 0 16px;
        }
        QToolButton#boostNavButton:hover {
            background: rgba(30, 125, 164, 0.11);
            border-color: rgba(54, 223, 255, 0.18);
            border-left-color: rgba(54, 223, 255, 0.55);
            color: #E5F8FF;
        }
        QToolButton#boostNavButton:checked {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 rgba(19, 142, 220, 0.52),
                                        stop:0.48 rgba(10, 81, 143, 0.38),
                                        stop:1 rgba(7, 27, 48, 0.12));
            border-color: rgba(54, 223, 255, 0.42);
            border-left-color: #32DFFF;
            color: #F3FCFF;
        }
        QToolButton#boostNavButton:checked:hover { border-color: rgba(87, 232, 255, 0.6); }
        QWidget#boostTopBar { background: transparent; }
        QPushButton#boostAccountMenuButton {
            background: #121D2F;
            border-color: #2B3B55;
            color: #F2F6FF;
            min-height: 42px;
            border-radius: 10px;
            padding: 0 12px;
            text-align: left;
        }
        QPushButton#boostAccountMenuButton:hover {
            background: #17263A;
            border-color: #36DFFF;
        }
        QFrame#boostDashboardPanel {
            background: #101A2B;
            border-color: #26354C;
        }
        QFrame#boostAccountCard { background: #141414; border-color: #303030; }
        QFrame#boostAccountCard[selected="true"] {
            background: #21180F;
            border: 2px solid #F28A1D;
        }
        QToolButton#boostAccountCardAction:disabled {
            color: #777777;
            background: #171717;
            border-color: #292929;
        }
        QToolButton#boostAccountCardAction[selected="true"] {
            color: #17120D;
            background: #F28A1D;
            border-color: #F28A1D;
        }
        QFrame#boostHero {
            border: 1px solid rgba(27, 74, 114, 0.9);
            border-radius: 18px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 rgba(17, 27, 45, 0.88),
                                        stop:0.5 rgba(15, 30, 48, 0.86),
                                        stop:1 rgba(7, 16, 30, 0.9));
        }
        QLabel#boostHeroArtwork {
            color: #EAFBFF;
            background: rgba(7, 15, 29, 0.72);
            border: 1px solid rgba(54, 223, 255, 0.52);
            border-radius: 12px;
            font-size: 18pt;
            font-weight: 800;
            letter-spacing: 1px;
        }
        QLabel#boostHomeGreeting {
            color: #F2F6FF;
            font-size: 10pt;
            font-weight: 600;
        }
        QLabel#boostHomeGreetingSubtitle {
            color: #AAC1DB;
            font-size: 9pt;
        }
        QLabel#boostGameTitle {
            color: #F3F7FF;
            font-size: 27pt;
            font-weight: 800;
        }
        QLabel#boostGameEdition {
            color: #7FB8F5;
            font-size: 14pt;
            font-weight: 600;
        }
        QLabel#boostGameTagline {
            color: #78AFE3;
            font-size: 9pt;
        }
        QLabel#boostHomeAvatar, QLabel#boostProfileAvatar {
            background: #14243A;
            border: 1px solid #36DFFF;
            border-radius: 21px;
        }
        QLabel#boostKicker { color: #61DFF4; }
        QLabel#boostProfileName { font-size: 8pt; }
        QComboBox#boostBuildSelector {
            min-height: 48px;
            border-radius: 10px;
            padding: 0 10px;
            font-size: 11pt;
            font-weight: 700;
        }
        QLabel#boostDashboardPanel {
            color: #B4C1D5;
            background: #101A2B;
            border: 1px solid #26354C;
            border-radius: 10px;
            padding: 14px;
        }
        QLabel#boostProfileName, QLabel#boostPanelMetric, QLabel#boostLibraryTitle,
        QLabel#boostPlayerProfileName, QLabel#boostSectionTitle {
            color: #F2F6FF;
        }
        QLabel#boostProfileStatus, QLabel#boostPanelCaption, QLabel#boostPlayerProfileDescription {
            color: #B4C1D5;
        }
        QPushButton#boostPlayButton {
            color: #06131D;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #56EAFF, stop:1 #28BFEA);
            border: 1px solid rgba(207, 249, 255, 0.55);
            border-radius: 10px;
            font-size: 12pt;
        }
        QPushButton#boostPlayButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #91F1FF, stop:1 #42D6F6);
        }
        QPushButton#boostPlayButton:disabled {
            color: #91A3B8;
            background: #1C2A3F;
            border-color: #2B3B55;
        }
        QPushButton#boostSecondaryButton, QPushButton {
            background: #142036;
            border-color: #2B3B55;
            color: #E7EEFC;
        }
        QPushButton#boostSecondaryButton:hover, QPushButton:hover {
            background: #1A2B45;
            border-color: #36DFFF;
        }
        QPushButton#boostLinkButton {
            color: #60DFF5;
            background: transparent;
            border: 0;
            padding: 5px 7px;
        }
        QPushButton#boostLinkButton:hover { color: #A5F2FF; }
        QLabel#boostSectionTitle { font-size: 14pt; font-weight: 700; }
        QFrame#boostInstanceCard, QFrame#boostHomeNewsCard {
            background: #101A2B;
            border: 1px solid #26354C;
            border-radius: 10px;
        }
        QFrame#boostContentPanel {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                        stop:0 rgba(5, 16, 33, 0.95), stop:1 rgba(4, 13, 27, 0.96));
            border: 1px solid #153454;
            border-radius: 13px;
        }
        QFrame#boostInstanceCard:hover, QFrame#boostHomeNewsCard:hover {
            border-color: #36DFFF;
        }
        QFrame#boostInstanceCard[selected="true"] {
            border-color: rgba(54, 223, 255, 0.65);
            background: #101D31;
        }
        QLabel#boostInstanceCardIcon, QLabel#boostNewsCardIcon {
            background: #0A172A;
            border: 1px solid #193B5C;
            border-radius: 8px;
        }
        QLabel#boostInstanceCardBadge {
            color: #B9D9F7;
            background: #112744;
            border: 1px solid #1B4266;
            border-radius: 9px;
            padding: 2px 8px;
            font-size: 8pt;
        }
        QLabel#boostInstanceCardStatus { color: #2CE88A; }
        QToolButton#boostCardLaunchButton {
            color: #36DFFF;
            background: #07182B;
            border: 1px solid #14517B;
            border-radius: 17px;
            font-size: 11pt;
            font-weight: 700;
        }
        QToolButton#boostCardLaunchButton:hover {
            color: #06131D;
            background: #36DFFF;
            border-color: #89F3FF;
        }
        QToolButton#boostCardLaunchButton:disabled {
            color: #7186A2;
            background: #111B2B;
            border-color: #26354C;
        }
        QLabel#boostInstanceCardName, QLabel#boostNewsCardTitle {
            color: #F2F6FF;
            font-weight: 700;
        }
        QLabel#boostInstanceCardMeta, QLabel#boostNewsCardExcerpt {
            color: #91A2BB;
        }
        QComboBox#boostProfileSelector, QComboBox#boostBuildSelector {
            background: rgba(9, 19, 35, 0.88);
            border: 1px solid rgba(99, 187, 219, 0.52);
            color: #F2F6FF;
        }
        QListWidget#boostNewsList {
            background: #0D1524;
            border-color: #26354C;
        }
    )BOOSTQSS") + QString::fromUtf8(R"OGQSS(
        QMainWindow#MainWindow, QWidget#centralWidget, QStackedWidget {
            background: #0A0A0A;
            color: #F5F5F5;
        }
        QWidget { color: #F0F0F0; background-color: #0A0A0A; }
        QLabel { background: transparent; }
        QWidget#boostSidebar {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #111214, stop:1 #17181A);
            border-right: 1px solid #2A2B2D;
        }
        QWidget#boostBrandBlock, QWidget#boostSidebarNavigation { background: transparent; }
        QLabel#boostBrandMark {
            background: #1C1D1F;
            color: #FFFFFF;
            border: 1px solid #343537;
            border-radius: 10px;
        }
        QLabel#boostBrand, QLabel#boostBrandCaption {
            background: transparent;
            color: #F0F0F0;
        }
        QLabel#boostBrand { font-size: 10pt; font-weight: 800; letter-spacing: 1px; }
        QLabel#boostBrandCaption { color: #B1B2B4; font-size: 7pt; font-weight: 700; letter-spacing: 1px; }
        QLabel#boostSidebarVersion { color: #96979A; font-size: 8pt; background: transparent; }
        QWidget#boostSidebarArtwork, QLabel#boostSidebarGrassImage { background: transparent; }
        QWidget#boostSidebarArtworkOverlay { background-color: rgba(0, 0, 0, 26); }
        QToolButton#boostNavButton {
            background: transparent;
            color: #E7E7E8;
            border: 1px solid transparent;
            border-left: 3px solid transparent;
            border-radius: 8px;
            min-height: 54px;
            padding: 4px 7px;
            font-size: 9pt;
            font-weight: 600;
            text-align: center;
        }
        QToolButton#boostNavButton:hover {
            background: #232426;
            color: #FFFFFF;
            border-color: #303133;
            border-left-color: #5A5B5D;
        }
        QToolButton#boostNavButton:checked {
            background: #292A2C;
            color: #FFFFFF;
            border-color: #3B3C3E;
            border-left-color: #F28A1D;
            font-weight: 700;
        }
        QToolButton#boostNavButton:checked:hover { background: #303133; }
        QToolButton#boostSidebarSettings {
            background: transparent;
            color: #E7E7E8;
            border: 1px solid transparent;
            border-radius: 8px;
            min-height: 54px;
            padding: 4px 9px;
            font-size: 9pt;
            font-weight: 600;
            text-align: center;
        }
        QToolButton#boostSidebarSettings:hover {
            background: #232426;
            color: #FFFFFF;
            border-color: #303133;
        }
        QWidget#boostTopBar {
            background: transparent;
            border: 0;
        }
        QToolButton#boostWindowMinimize, QToolButton#boostWindowMaximize, QToolButton#boostWindowClose {
            color: #B8B8B8;
            border: 1px solid transparent;
            border-radius: 7px;
        }
        QToolButton#boostWindowMinimize:hover, QToolButton#boostWindowMaximize:hover {
            background: #242424;
            border-color: #343434;
            color: #FFFFFF;
        }
        QToolButton#boostWindowClose:hover { background: #C94136; border-color: #E05A4E; color: #FFFFFF; }
        QToolBar#mainToolBar, QToolBar#instanceToolBar {
            background: #101010;
            border-color: #303030;
        }
        QToolBar#instanceToolBar {
            background: transparent;
            border: 0;
            border-left: 1px solid #303030;
            border-radius: 0;
            spacing: 3px;
            padding: 120px 8px 8px;
            margin: 0;
        }
        QToolBar#instanceToolBar::separator {
            background: #303030;
            height: 1px;
            margin: 3px 7px;
        }
        QToolBar#instanceToolBar ActionButton#boostInstanceToolbarAction,
        QToolBar#instanceToolBar ActionButton#boostInstanceToolbarDanger,
        QToolBar#instanceToolBar ActionButton#boostInstanceToolbarLaunch {
            color: #E8E5E1;
            background: #202020;
            border: 1px solid #333333;
            border-radius: 8px;
            min-height: 26px;
            max-height: 28px;
            padding: 2px 8px;
            font-size: 9pt;
            text-align: left;
        }
        QToolBar#instanceToolBar ActionButton#boostInstanceToolbarAction:hover,
        QToolBar#instanceToolBar ActionButton#boostInstanceToolbarDanger:hover {
            color: #FFFFFF;
            background: #24201B;
            border-color: #5B3A1F;
        }
        QToolBar#instanceToolBar ActionButton#boostInstanceToolbarAction:disabled,
        QToolBar#instanceToolBar ActionButton#boostInstanceToolbarDanger:disabled { color: #737373; }
        QToolBar#instanceToolBar ActionButton#boostInstanceToolbarLaunch {
            color: #17120D;
            background: #F28A1D;
            border: 1px solid #F28A1D;
            border-radius: 7px;
            font-weight: 700;
        }
        QToolBar#instanceToolBar ActionButton#boostInstanceToolbarLaunch:hover {
            color: #17120D;
            background: #FFA13B;
            border-color: #FFA13B;
        }
        QToolBar#instanceToolBar ActionButton#boostInstanceToolbarLaunch::menu-button {
            width: 16px;
            border-left: 1px solid rgba(23, 18, 13, 0.25);
            border-top-right-radius: 6px;
            border-bottom-right-radius: 6px;
        }
        QToolBar#instanceToolBar ActionButton#boostInstanceToolbarDanger {
            color: #E36B5D;
            background: #241615;
            border-color: #492724;
        }
        QToolBar#instanceToolBar ActionButton#boostInstanceToolbarDanger:disabled {
            color: #777777;
        }
        QToolBar#instanceToolBar QWidget#boostInstanceToolbarHeader {
            background: #0D0D0D;
            border: 1px solid #292929;
            border-radius: 10px;
            margin: 2px 0 6px;
        }
        QToolBar#instanceToolBar QToolButton#changeIconButton {
            background: #141414;
            border: 1px solid #353535;
            border-radius: 10px;
            padding: 1px;
        }
        QToolBar#instanceToolBar QToolButton#changeIconButton:hover {
            background: #21180F;
            border-color: #F28A1D;
        }
        QToolBar#instanceToolBar QLabel#nameLabel {
            color: #F5F2EE;
            font-size: 9pt;
            font-weight: 700;
            padding: 2px 0 3px;
        }
        QToolBar QToolButton:hover { background: #222222; border-color: #383838; }
        QPushButton#boostAccountMenuButton {
            background: #141414;
            border-color: #303030;
            color: #F5F5F5;
        }
        QPushButton#boostAccountMenuButton:hover { background: #202020; border-color: #F28A1D; }
        QFrame#boostDashboardPanel, QFrame#boostLibraryProfileCard, QFrame#boostAccountCard, QFrame#boostInstanceCard,
        QFrame#boostHomeNewsCard {
            background: #141414;
            border: 1px solid #303030;
        }
        QFrame#boostLibraryProfileCard {
            border-radius: 13px;
            background: transparent;
            border-color: #353535;
        }
        QLabel#boostProfileAvatar { border-radius: 12px; }
        QLabel#boostLibraryHeadingIcon {
            background: transparent;
        }
        QComboBox#boostLibraryFolderSelector {
            color: #EEEEEE;
            background: #171717;
            border: 1px solid #383838;
            border-radius: 8px;
            min-height: 36px;
            padding: 0 10px;
        }
        QComboBox#boostLibraryFolderSelector:hover { border-color: #F28A1D; }
        QComboBox#boostLibraryFolderSelector::drop-down { border: 0; width: 24px; }
        QFrame#boostAccountCard[selected="true"], QFrame#boostInstanceCard[selected="true"] {
            background: #202020;
            border: 2px solid #F28A1D;
        }
        QFrame#boostInstanceCard {
            background: #171717;
            border: 1px solid #343434;
            border-radius: 12px;
        }
        QFrame#boostInstanceCard:hover { border-color: #7A4A1D; }
        QFrame#boostContentPanel { background: #101010; border: 1px solid #303030; }
        QFrame#boostHero { border-color: #303030; }
        QLabel#boostHeroArtwork {
            color: #F5F5F5;
            background: rgba(10, 10, 10, 0.78);
            border-color: rgba(242, 138, 29, 0.55);
        }
        QLabel#boostHomeGreeting, QLabel#boostGameTitle, QLabel#boostProfileName,
        QLabel#boostPanelMetric, QLabel#boostLibraryTitle, QLabel#boostPlayerProfileName,
        QLabel#boostSectionTitle, QLabel#boostInstanceCardName, QLabel#boostNewsCardTitle {
            color: #F5F5F5;
        }
        QLabel#boostHomeGreetingSubtitle, QLabel#boostGameTagline, QLabel#boostGameEdition,
        QLabel#boostHomeIntro, QLabel#boostProfileStatus, QLabel#boostPanelCaption,
        QLabel#boostPlayerProfileDescription, QLabel#boostInstanceCardMeta,
        QLabel#boostNewsCardExcerpt, QLabel#boostBrandCaption, QLabel#boostNavGroup {
            color: #A8A8A8;
        }
        QLabel#boostKicker, QPushButton#boostLinkButton { color: #F28A1D; }
        QLabel#boostHomeAvatar, QLabel#boostProfileAvatar {
            background: #171717;
            border-color: #383838;
        }
        QComboBox, QLineEdit, QAbstractSpinBox, QTextEdit, QPlainTextEdit,
        QListWidget, QTreeView, QTableView {
            color: #F0F0F0;
            background: #111111;
            border-color: #333333;
            selection-background-color: #333333;
            selection-color: #FFFFFF;
        }
        QComboBox#boostBuildSelector, QComboBox#boostProfileSelector {
            color: #F0F0F0;
            background: #111111;
            border-color: #333333;
        }
        QComboBox#boostBuildSelector:focus, QComboBox#boostProfileSelector:focus,
        QComboBox:focus, QLineEdit:focus, QAbstractSpinBox:focus,
        QTextEdit:focus, QPlainTextEdit:focus { border-color: #F28A1D; }
        QPushButton#boostPlayButton {
            color: #111111;
            background: #F28A1D;
            border: 0;
        }
        QPushButton#boostPlayButton:hover { background: #FFA13B; }
        QPushButton#boostLibraryAddButton {
            color: #111111;
            background: #F28A1D;
            border: 0;
            border-radius: 7px;
            padding: 0 10px;
            font-size: 8pt;
            font-weight: 700;
        }
        QPushButton#boostLibraryAddButton:hover { background: #FFA13B; }
        QPushButton#boostLibraryViewButton {
            min-height: 30px;
            padding: 0 11px;
            border-radius: 7px;
        }
        QPushButton#boostLibraryViewButton:checked {
            color: #F28A1D;
            background: #24201B;
            border-color: #6A421F;
        }
        QPushButton#boostLibraryLaunchButton {
            color: #111111;
            background: #F28A1D;
            border: 0;
            border-radius: 7px;
            padding: 5px 18px;
            font-weight: 700;
        }
        QPushButton#boostLibraryLaunchButton:hover { background: #FFA13B; }
        QPushButton#boostLibraryLaunchButton:disabled {
            color: #888888;
            background: #292929;
        }
        QLabel#boostLibraryEmptyState { color: #A8A8A8; }
        QPushButton#boostPlayButton:disabled {
            color: #888888;
            background: #292929;
        }
        QPushButton#boostSecondaryButton, QPushButton {
            color: #EAEAEA;
            background: #171717;
            border-color: #333333;
        }
        QPushButton#boostSecondaryButton:hover, QPushButton:hover {
            background: #252525;
            border-color: #F28A1D;
        }
        QPushButton#boostLinkButton { background: transparent; border: 0; }
        QToolButton#boostCardLaunchButton {
            color: #F28A1D;
            background: #151515;
            border-color: #5A3518;
        }
        QToolButton#boostCardLaunchButton:hover {
            color: #111111;
            background: #F28A1D;
            border-color: #FFB347;
        }
        QLabel#boostInstanceCardBadge {
            color: #D0D0D0;
            background: #202020;
            border-color: #383838;
        }
        QHeaderView::section { background: #161616; color: #F0F0F0; border-color: #303030; }
        QStatusBar { background: #101010; color: #E0E0E0; border-color: #303030; }
        QStatusBar { border: 0; }
        QScrollBar:vertical { background: #111111; }
        QScrollBar::handle:vertical { background: #353535; }
        QScrollBar::handle:vertical:hover { background: #F28A1D; }
        QTabWidget::pane { background: #111111; border-color: #303030; }
        QTabBar::tab { color: #A8A8A8; background: #141414; border-color: #303030; }
        QTabBar::tab:selected { color: #F28A1D; background: #222222; }
    )OGQSS"));
    ui->mainToolBar->setIconSize({ 18, 18 });
    ui->instanceToolBar->setIconSize({ 18, 18 });
    ui->newsToolBar->setIconSize({ 18, 18 });

    setWindowIcon(APPLICATION->logo());
    setWindowTitle(APPLICATION->applicationDisplayName());
#ifndef QT_NO_ACCESSIBILITY
    setAccessibleName(BuildConfig.LAUNCHER_DISPLAYNAME);
#endif

    QSize minimumWindowSize(900, 560);
    QSize initialWindowSize(1280, 720);
    if (auto* screen = QApplication::primaryScreen()) {
        const QSize availableSize = screen->availableGeometry().size();
        initialWindowSize.setWidth(qMin(initialWindowSize.width(), int(availableSize.width() * 0.88)));
        initialWindowSize.setHeight(qMin(initialWindowSize.height(), int(availableSize.height() * 0.88)));
        if (availableSize.width() < minimumWindowSize.width()) {
            minimumWindowSize.setWidth(qMin(minimumWindowSize.width(), qMax(720, int(availableSize.width() * 0.84))));
        }
        if (availableSize.height() < minimumWindowSize.height()) {
            minimumWindowSize.setHeight(qMin(minimumWindowSize.height(), qMax(400, int(availableSize.height() * 0.82))));
        }
        initialWindowSize.setWidth(qMax(minimumWindowSize.width(), initialWindowSize.width()));
        initialWindowSize.setHeight(qMax(minimumWindowSize.height(), initialWindowSize.height()));
    }
    setMinimumSize(minimumWindowSize);
    resize(initialWindowSize);
    ui->mainToolBar->setMovable(false);
    removeToolBar(ui->mainToolBar);
    ui->mainToolBar->setVisible(false);

    auto* sidebar = new QWidget(ui->centralWidget);
    sidebar->setObjectName(QStringLiteral("boostSidebar"));
    sidebar->setFixedWidth(140);
    sidebar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    auto* sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(4, 8, 4, 8);
    sidebarLayout->setSpacing(1);

    auto* brand = new QWidget(sidebar);
    brand->setObjectName(QStringLiteral("boostBrandBlock"));
    brand->setFixedHeight(52);
    auto* brandLayout = new QHBoxLayout(brand);
    brandLayout->setContentsMargins(6, 0, 5, 0);
    brandLayout->setSpacing(7);
    auto* brandMark = new QLabel(brand);
    brandMark->setObjectName(QStringLiteral("boostBrandMark"));
    brandMark->setFixedSize(34, 34);
    brandMark->setAlignment(Qt::AlignCenter);
    brandMark->setPixmap(APPLICATION->logo().pixmap(26, 26));
    brandLayout->addWidget(brandMark);
    auto* brandCopy = new QVBoxLayout();
    brandCopy->setContentsMargins(0, 0, 0, 0);
    brandCopy->setSpacing(1);
    auto* brandName = new QLabel(tr("OG CLIENT"), brand);
    brandName->setObjectName(QStringLiteral("boostBrand"));
    auto* brandCaption = new QLabel(tr("MINECRAFT JAVA"), brand);
    brandCaption->setObjectName(QStringLiteral("boostBrandCaption"));
    brandCopy->addWidget(brandName);
    brandCopy->addWidget(brandCaption);
    brandLayout->addLayout(brandCopy, 1);
    sidebarLayout->addWidget(brand);

    auto* navigationWidget = new QWidget(sidebar);
    navigationWidget->setObjectName(QStringLiteral("boostSidebarNavigation"));
    auto* navigationLayout = new QVBoxLayout(navigationWidget);
    navigationLayout->setContentsMargins(3, 7, 3, 7);
    navigationLayout->setSpacing(4);
    sidebarLayout->addWidget(navigationWidget, 1);

    auto* pageNavigation = new QButtonGroup(this);
    pageNavigation->setExclusive(true);

    auto makeNavigationButton = [navigationWidget, pageNavigation](const QString& text, const QString& accessibleName,
                                                                    const QString& iconName) {
        auto* button = new QToolButton(navigationWidget);
        button->setObjectName(QStringLiteral("boostNavButton"));
        button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        button->setText(text);
        button->setIcon(navigationIcon(iconName));
        button->setIconSize(QSize(20, 20));
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        button->setCheckable(true);
        button->setAccessibleName(accessibleName);
        pageNavigation->addButton(button);
        new NavigationButtonAnimation(button);
        return button;
    };

    auto* homeButton = makeNavigationButton(tr("Play"), tr("Play"), QStringLiteral("launch"));
    homeButton->setChecked(true);
    homeButton->setShortcut(QKeySequence(QStringLiteral("Alt+1")));
    navigationLayout->addWidget(homeButton);

    auto* libraryButton = makeNavigationButton(tr("Instances"), tr("Manage Minecraft instances"), QStringLiteral("minecraft"));
    navigationLayout->addWidget(libraryButton);

    auto* modsNavigationButton =
        makeNavigationButton(tr("Mods"), tr("Manage mods for the selected instance"), QStringLiteral("loadermods"));
    navigationLayout->addWidget(modsNavigationButton);

    auto* accountsButton = makeNavigationButton(tr("Accounts"), tr("Manage Minecraft accounts"), QStringLiteral("accounts"));
    navigationLayout->addWidget(accountsButton);

    navigationLayout->addStretch(1);

    auto* settingsButton = new QToolButton(sidebar);
    settingsButton->setObjectName(QStringLiteral("boostSidebarSettings"));
    settingsButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    settingsButton->setText(tr("Settings"));
    settingsButton->setIcon(navigationIcon(QStringLiteral("settings")));
    settingsButton->setIconSize(QSize(20, 20));
    settingsButton->setAccessibleName(tr("Launcher settings"));
    settingsButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    sidebarLayout->addWidget(settingsButton);
    connect(settingsButton, &QAbstractButton::clicked, ui->actionSettings, &QAction::trigger);

    auto* sidebarArtwork = new QWidget(sidebar);
    sidebarArtwork->setObjectName(QStringLiteral("boostSidebarArtwork"));
    sidebarArtwork->setFixedHeight(220);
    auto* sidebarArtworkLayout = new QStackedLayout(sidebarArtwork);
    sidebarArtworkLayout->setContentsMargins(0, 0, 0, 0);
    sidebarArtworkLayout->setStackingMode(QStackedLayout::StackAll);
    auto* sidebarGrassImage = new QLabel(sidebarArtwork);
    sidebarGrassImage->setObjectName(QStringLiteral("boostSidebarGrassImage"));
    sidebarGrassImage->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    sidebarGrassImage->setContentsMargins(8, 0, 0, 0);
    sidebarGrassImage->setPixmap(QPixmap(QStringLiteral(":/backgrounds/boostclient-sidebar-mascot"))
                                     .scaled(QSize(168, 175), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    sidebarArtworkLayout->addWidget(sidebarGrassImage);
    auto* sidebarArtworkOverlay = new QWidget(sidebarArtwork);
    sidebarArtworkOverlay->setObjectName(QStringLiteral("boostSidebarArtworkOverlay"));
    sidebarArtworkOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    sidebarArtworkLayout->addWidget(sidebarArtworkOverlay);

    auto* sidebarVersion = new QLabel(BuildConfig.printableVersionString(), sidebar);
    sidebarVersion->setObjectName(QStringLiteral("boostSidebarVersion"));
    sidebarVersion->setAlignment(Qt::AlignCenter);
    sidebarLayout->addWidget(sidebarVersion);
    sidebarLayout->addWidget(sidebarArtwork);

    auto* contentColumn = new QWidget(ui->centralWidget);
    contentColumn->setObjectName(QStringLiteral("boostContentColumn"));
    contentColumn->installEventFilter(this);
    auto* contentColumnLayout = new QVBoxLayout(contentColumn);
    contentColumnLayout->setContentsMargins(0, 0, 0, 0);
    contentColumnLayout->setSpacing(0);
#ifdef Q_OS_WIN
    auto* titleBar = new WindowDragBar(contentColumn);
    titleBar->setObjectName(QStringLiteral("boostTopBar"));
    titleBar->setFixedHeight(40);
    titleBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto* titleBarLayout = new QHBoxLayout(titleBar);
    titleBarLayout->setContentsMargins(0, 3, 8, 0);
    titleBarLayout->setSpacing(3);
    titleBarLayout->addStretch(1);

    auto makeWindowControl = [titleBar, titleBarLayout](const QString& objectName, const QString& text,
                                                        const QString& accessibleName) {
        auto* button = new QToolButton(titleBar);
        button->setObjectName(objectName);
        button->setText(text);
        button->setToolTip(accessibleName);
        button->setAccessibleName(accessibleName);
        button->setFocusPolicy(Qt::NoFocus);
        button->setAutoRaise(true);
        button->setCursor(Qt::ArrowCursor);
        button->setFixedSize(objectName == QStringLiteral("boostWindowClose") ? QSize(48, 32) : QSize(42, 32));
        titleBarLayout->addWidget(button);
        new WindowControlAnimation(button);
        return button;
    };

    auto* minimizeButton = makeWindowControl(QStringLiteral("boostWindowMinimize"), QStringLiteral("−"), tr("Minimize"));
    connect(minimizeButton, &QToolButton::clicked, this, &QWidget::showMinimized);
    m_windowMaximizeButton =
        makeWindowControl(QStringLiteral("boostWindowMaximize"), QStringLiteral("□"), tr("Maximize"));
    connect(m_windowMaximizeButton, &QToolButton::clicked, this, [this] {
        isMaximized() ? showNormal() : showMaximized();
    });
    auto* closeButton = makeWindowControl(QStringLiteral("boostWindowClose"), QStringLiteral("×"), tr("Close"));
    connect(closeButton, &QToolButton::clicked, this, &QWidget::close);
#endif
    m_contentPages = new QStackedWidget(contentColumn);
    m_homePage = new QWidget(m_contentPages);
    auto* homePageLayout = new QVBoxLayout(m_homePage);
    homePageLayout->setContentsMargins(0, 0, 0, 0);
    homePageLayout->setSpacing(0);

    auto* homeScroll = new HomeDashboardScrollArea(m_homePage);
    homeScroll->setObjectName(QStringLiteral("boostHomeScroll"));
    homeScroll->setWidgetResizable(true);
    homeScroll->setFrameShape(QFrame::NoFrame);
    auto* homeContent = new QWidget(homeScroll);
    auto* homeContentLayout = new QVBoxLayout(homeContent);
    homeContentLayout->setContentsMargins(0, 0, 0, 0);
    homeContentLayout->setSpacing(0);

    auto* hero = new BackgroundFrame(homeContent);
    hero->setObjectName(QStringLiteral("boostHero"));
    hero->setMinimumHeight(350);
    hero->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    const QString wallpaperPath = QStringLiteral(":/backgrounds/boostclient-hero");
    QPixmap wallpaper(wallpaperPath);
    if (wallpaper.isNull()) {
        qWarning() << "Unable to load BoostClient startup wallpaper:" << wallpaperPath;
    } else {
        hero->setBackgroundPixmap(std::move(wallpaper));
    }
    auto* heroLayout = new QGridLayout(hero);
    heroLayout->setContentsMargins(24, 20, 22, 18);
    heroLayout->setHorizontalSpacing(16);
    heroLayout->setVerticalSpacing(8);
    heroLayout->setColumnStretch(0, 7);
    heroLayout->setColumnStretch(1, 5);
    auto* greetingRow = new QHBoxLayout();
    greetingRow->setContentsMargins(0, 0, 0, 0);
    greetingRow->setSpacing(11);
    m_homeAvatar = new QLabel(hero);
    m_homeAvatar->setObjectName(QStringLiteral("boostHomeAvatar"));
    m_homeAvatar->setFixedSize(42, 42);
    m_homeAvatar->setAlignment(Qt::AlignCenter);
    greetingRow->addWidget(m_homeAvatar);
    auto* greetingCopy = new QVBoxLayout();
    greetingCopy->setContentsMargins(0, 0, 0, 0);
    greetingCopy->setSpacing(2);
    m_homeGreeting = new QLabel(hero);
    m_homeGreeting->setObjectName(QStringLiteral("boostHomeGreeting"));
    greetingCopy->addWidget(m_homeGreeting);
    m_homeGreetingSubtitle = new QLabel(tr("Ready for your next adventure?"), hero);
    m_homeGreetingSubtitle->setObjectName(QStringLiteral("boostHomeGreetingSubtitle"));
    greetingCopy->addWidget(m_homeGreetingSubtitle);
    greetingRow->addLayout(greetingCopy);
    heroLayout->addLayout(greetingRow, 0, 0, 1, 2, Qt::AlignLeft | Qt::AlignVCenter);

    auto* selectedLabel = new QLabel(tr("Selected Profile"), hero);
    selectedLabel->setObjectName(QStringLiteral("boostKicker"));
    m_homeInstanceSelector = new QComboBox(hero);
    m_homeInstanceSelector->setObjectName(QStringLiteral("boostBuildSelector"));
    m_homeInstanceSelector->setMinimumWidth(170);
    m_homeInstanceSelector->setIconSize({ 24, 24 });
    m_homeInstanceSelector->setAccessibleName(tr("Select a Minecraft installation"));
    auto* profileField = new QVBoxLayout();
    profileField->setSpacing(3);
    profileField->addWidget(selectedLabel);
    profileField->addWidget(m_homeInstanceSelector);

    auto* identity = new QWidget(hero);
    identity->setObjectName(QStringLiteral("boostGameIdentity"));
    auto* identityLayout = new QHBoxLayout(identity);
    identityLayout->setContentsMargins(0, 0, 0, 0);
    identityLayout->setSpacing(12);
    auto* grassIcon = new QLabel(identity);
    grassIcon->setFixedSize(60, 60);
    grassIcon->setAlignment(Qt::AlignCenter);
    grassIcon->setPixmap(QPixmap(QStringLiteral(":/backgrounds/boostclient-grasss"))
                             .scaled(60, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    identityLayout->addWidget(grassIcon);
    auto* titleCopy = new QVBoxLayout();
    titleCopy->setSpacing(0);
    auto* gameTitle = new QLabel(tr("Minecraft"), identity);
    gameTitle->setObjectName(QStringLiteral("boostGameTitle"));
    auto* editionTitle = new QLabel(tr("Java Edition"), identity);
    editionTitle->setObjectName(QStringLiteral("boostGameEdition"));
    auto* tagline = new QLabel(tr("Build   ·   Explore   ·   Survive"), identity);
    tagline->setObjectName(QStringLiteral("boostGameTagline"));
    titleCopy->addWidget(gameTitle);
    titleCopy->addWidget(editionTitle);
    titleCopy->addWidget(tagline);
    identityLayout->addLayout(titleCopy);
    heroLayout->addWidget(identity, 1, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);

    auto* buildSelectors = new QHBoxLayout();
    buildSelectors->setSpacing(8);
    buildSelectors->addLayout(profileField);
    auto* versionField = new QVBoxLayout();
    versionField->setSpacing(3);
    auto* versionLabel = new QLabel(tr("Minecraft Version"), hero);
    versionLabel->setObjectName(QStringLiteral("boostKicker"));
    versionField->addWidget(versionLabel);
    m_homeVersionSelector = new QComboBox(hero);
    m_homeVersionSelector->setObjectName(QStringLiteral("boostBuildSelector"));
    m_homeVersionSelector->setMinimumWidth(140);
    m_homeVersionSelector->setIconSize({ 24, 24 });
    m_homeVersionSelector->setAccessibleName(tr("Select an installed Minecraft version"));
    versionField->addWidget(m_homeVersionSelector);
    buildSelectors->addLayout(versionField);
    auto* loaderField = new QVBoxLayout();
    loaderField->setSpacing(3);
    auto* loaderLabel = new QLabel(tr("Mod Loader"), hero);
    loaderLabel->setObjectName(QStringLiteral("boostKicker"));
    loaderField->addWidget(loaderLabel);
    m_homeLoaderSelector = new QComboBox(hero);
    m_homeLoaderSelector->setObjectName(QStringLiteral("boostBuildSelector"));
    m_homeLoaderSelector->setMinimumWidth(130);
    m_homeLoaderSelector->setIconSize({ 24, 24 });
    m_homeLoaderSelector->setAccessibleName(tr("Select an installed mod loader"));
    loaderField->addWidget(m_homeLoaderSelector);
    buildSelectors->addLayout(loaderField);
    buildSelectors->addStretch(1);
    heroLayout->addLayout(buildSelectors, 2, 0, 1, 1);

    m_homeProfileName = new QLabel(hero);
    m_homeProfileName->setObjectName(QStringLiteral("boostKicker"));
    m_homeProfileName->setWordWrap(true);
    m_homeProfileStatus = new QLabel(hero);
    m_homeProfileStatus->setObjectName(QStringLiteral("boostProfileStatus"));
    m_homeProfileStatus->setWordWrap(true);
    m_homeProfileStatus->setMaximumHeight(36);
    auto* historyCopy = new QVBoxLayout();
    historyCopy->setSpacing(2);
    historyCopy->addWidget(m_homeProfileName);
    historyCopy->addWidget(m_homeProfileStatus);
    auto* heroActions = new QHBoxLayout();
    heroActions->setSpacing(12);
    m_homePlayButton = new QPushButton(tr("Play"), hero);
    m_homePlayButton->setObjectName(QStringLiteral("boostPlayButton"));
    m_homePlayButton->setIcon(launchButtonIcon());
    m_homePlayButton->setIconSize({ 22, 22 });
    m_homePlayButton->setMinimumSize(220, 58);
    m_homePlayButton->setAccessibleName(tr("Launch or stop the selected installation"));
    m_homeManageButton = new QPushButton(tr("Configure"), hero);
    m_homeManageButton->setObjectName(QStringLiteral("boostSecondaryButton"));
    m_homeManageButton->setIcon(QIcon(QStringLiteral(":/icons/breeze_dark/scalable/instance-settings.svg")));
    m_homeManageButton->setMinimumSize(132, 42);
    m_homeManageButton->setAccessibleName(tr("Configure the selected installation"));
    heroActions->addLayout(historyCopy, 1);
    heroActions->addWidget(m_homePlayButton);
    heroActions->addWidget(m_homeManageButton);
    heroLayout->addLayout(heroActions, 3, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);

    homeContentLayout->addWidget(hero, 1);

    auto* instancesPanel = new QFrame(homeContent);
    instancesPanel->setObjectName(QStringLiteral("boostContentPanel"));
    auto* instancesPanelLayout = new QVBoxLayout(instancesPanel);
    instancesPanelLayout->setContentsMargins(12, 9, 12, 10);
    instancesPanelLayout->setSpacing(7);
    auto* instancesHeading = new QHBoxLayout();
    instancesHeading->setSpacing(10);
    auto* instancesIcon = new QLabel(instancesPanel);
    instancesIcon->setFixedSize(30, 30);
    instancesIcon->setAlignment(Qt::AlignCenter);
    instancesIcon->setPixmap(QIcon(QStringLiteral(":/icons/breeze_dark/scalable/minecraft.svg")).pixmap(26, 26));
    instancesHeading->addWidget(instancesIcon);
    auto* instancesHeadingCopy = new QVBoxLayout();
    instancesHeadingCopy->setContentsMargins(0, 0, 0, 0);
    instancesHeadingCopy->setSpacing(1);
    auto* instancesTitle = new QLabel(tr("Instances"), instancesPanel);
    instancesTitle->setObjectName(QStringLiteral("boostSectionTitle"));
    auto* instancesSubtitle = new QLabel(tr("Manage your Minecraft installations"), instancesPanel);
    instancesSubtitle->setObjectName(QStringLiteral("boostPanelCaption"));
    instancesHeadingCopy->addWidget(instancesTitle);
    instancesHeadingCopy->addWidget(instancesSubtitle);
    instancesHeading->addLayout(instancesHeadingCopy);
    instancesHeading->addStretch(1);
    auto* browseInstancesButton = new QPushButton(tr("View all  ›"), instancesPanel);
    browseInstancesButton->setObjectName(QStringLiteral("boostLinkButton"));
    browseInstancesButton->setFlat(true);
    instancesHeading->addWidget(browseInstancesButton);
    instancesPanelLayout->addLayout(instancesHeading);

    auto* instancesScroll = new QScrollArea(instancesPanel);
    instancesScroll->setObjectName(QStringLiteral("boostInstanceCardsScroll"));
    instancesScroll->setWidgetResizable(true);
    instancesScroll->setFrameShape(QFrame::NoFrame);
    instancesScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    instancesScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    instancesScroll->setMinimumHeight(108);
    instancesScroll->setMaximumHeight(108);
    auto* instanceCards = new QWidget(instancesScroll);
    auto* instanceCardsLayout = new QHBoxLayout(instanceCards);
    instanceCardsLayout->setObjectName(QStringLiteral("boostHomeInstanceCards"));
    instanceCardsLayout->setContentsMargins(0, 2, 2, 8);
    instanceCardsLayout->setSpacing(10);
    instancesScroll->setWidget(instanceCards);
    instancesPanelLayout->addWidget(instancesScroll);
    auto* instancesMargin = new QWidget(homeContent);
    auto* instancesMarginLayout = new QVBoxLayout(instancesMargin);
    instancesMarginLayout->setContentsMargins(16, 0, 16, 0);
    instancesMarginLayout->addWidget(instancesPanel);
    homeContentLayout->addWidget(instancesMargin);

    connect(browseInstancesButton, &QPushButton::clicked, libraryButton, &QPushButton::click);

    auto* newsPanel = new QFrame(homeContent);
    newsPanel->setObjectName(QStringLiteral("boostContentPanel"));
    auto* newsPanelLayout = new QVBoxLayout(newsPanel);
    newsPanelLayout->setContentsMargins(12, 9, 12, 10);
    newsPanelLayout->setSpacing(7);
    auto* latestNewsHeading = new QHBoxLayout();
    latestNewsHeading->setSpacing(10);
    auto* newsIcon = new QLabel(newsPanel);
    newsIcon->setFixedSize(30, 30);
    newsIcon->setAlignment(Qt::AlignCenter);
    newsIcon->setPixmap(QIcon(QStringLiteral(":/icons/breeze_dark/scalable/news.svg")).pixmap(25, 25));
    latestNewsHeading->addWidget(newsIcon);
    auto* newsHeadingCopy = new QVBoxLayout();
    newsHeadingCopy->setContentsMargins(0, 0, 0, 0);
    newsHeadingCopy->setSpacing(1);
    auto* latestNewsTitle = new QLabel(tr("Latest News"), newsPanel);
    latestNewsTitle->setObjectName(QStringLiteral("boostSectionTitle"));
    auto* newsSubtitle = new QLabel(tr("Stay updated with the latest Minecraft news and updates"), newsPanel);
    newsSubtitle->setObjectName(QStringLiteral("boostPanelCaption"));
    newsHeadingCopy->addWidget(latestNewsTitle);
    newsHeadingCopy->addWidget(newsSubtitle);
    latestNewsHeading->addLayout(newsHeadingCopy);
    latestNewsHeading->addStretch(1);
    auto* allNewsButton = new QPushButton(tr("View all  ›"), newsPanel);
    allNewsButton->setObjectName(QStringLiteral("boostLinkButton"));
    allNewsButton->setFlat(true);
    latestNewsHeading->addWidget(allNewsButton);
    newsPanelLayout->addLayout(latestNewsHeading);
    auto* homeNewsStatus = new QLabel(tr("Loading Minecraft news…"), newsPanel);
    homeNewsStatus->setObjectName(QStringLiteral("boostHomeNewsStatus"));
    homeNewsStatus->setWordWrap(true);
    newsPanelLayout->addWidget(homeNewsStatus);
    auto* homeNewsContainer = new QWidget(newsPanel);
    homeNewsContainer->setObjectName(QStringLiteral("boostHomeNewsContainer"));
    auto* homeNewsLayout = new QHBoxLayout(homeNewsContainer);
    homeNewsLayout->setObjectName(QStringLiteral("boostHomeNewsItems"));
    homeNewsLayout->setContentsMargins(0, 0, 0, 0);
    homeNewsLayout->setSpacing(10);
    homeNewsContainer->setMinimumHeight(108);
    homeNewsContainer->setMaximumHeight(108);
    newsPanelLayout->addWidget(homeNewsContainer);
    auto* newsMargin = new QWidget(homeContent);
    auto* newsMarginLayout = new QVBoxLayout(newsMargin);
    newsMarginLayout->setContentsMargins(16, 0, 16, 0);
    newsMarginLayout->addWidget(newsPanel);
    homeContentLayout->addWidget(newsMargin);
    homeScroll->setWidget(homeContent);
    homeScroll->setDashboardSections(homeContent, newsMargin);
    homePageLayout->addWidget(homeScroll);

    m_libraryPage = new QWidget(m_contentPages);
    auto* libraryLayout = new QVBoxLayout(m_libraryPage);
    libraryLayout->setContentsMargins(6, 0, 8, 10);
    libraryLayout->setSpacing(6);
    auto* profileCard = new PlayerProfileBanner(m_libraryPage);
    profileCard->setObjectName(QStringLiteral("boostLibraryProfileCard"));
    profileCard->setMinimumHeight(80);
    profileCard->setBackgroundPixmap(QPixmap(QStringLiteral(":/backgrounds/boostclient-hero")));
    auto* profileCardLayout = new QHBoxLayout(profileCard);
    profileCardLayout->setContentsMargins(16, 8, 16, 8);
    profileCardLayout->setSpacing(14);
    auto* profileAvatar = new QLabel(profileCard);
    profileAvatar->setObjectName(QStringLiteral("boostProfileAvatar"));
    profileAvatar->setFixedSize(56, 56);
    profileAvatar->setAlignment(Qt::AlignCenter);
    profileCardLayout->addWidget(profileAvatar);
    auto* profileDetails = new QVBoxLayout();
    profileDetails->setContentsMargins(0, 0, 0, 0);
    profileDetails->setSpacing(3);
    auto* profileHeading = new QLabel(tr("PLAYER PROFILE"), profileCard);
    profileHeading->setObjectName(QStringLiteral("boostKicker"));
    profileDetails->addWidget(profileHeading);
    auto* profileName = new QLabel(profileCard);
    profileName->setObjectName(QStringLiteral("boostPlayerProfileName"));
    profileName->setWordWrap(true);
    profileDetails->addWidget(profileName);
    auto* profileDescription = new QLabel(profileCard);
    profileDescription->setObjectName(QStringLiteral("boostPlayerProfileDescription"));
    profileDescription->setTextFormat(Qt::RichText);
    profileDescription->setWordWrap(true);
    profileDetails->addWidget(profileDescription);
    profileCardLayout->addLayout(profileDetails, 1);
    auto* manageProfileButton = new QPushButton(tr("Manage account"), profileCard);
    manageProfileButton->setObjectName(QStringLiteral("boostSecondaryButton"));
    profileCardLayout->addWidget(manageProfileButton);
    libraryLayout->addWidget(profileCard);

    auto* libraryHeading = new QHBoxLayout();
    libraryHeading->setSpacing(14);
    auto* libraryHeadingIcon = new QLabel(m_libraryPage);
    libraryHeadingIcon->setObjectName(QStringLiteral("boostLibraryHeadingIcon"));
    libraryHeadingIcon->setFixedSize(58, 58);
    libraryHeadingIcon->setAlignment(Qt::AlignCenter);
    libraryHeadingIcon->setPixmap(QPixmap(QStringLiteral(":/backgrounds/boostclient-grasss"))
                                      .scaled(56, 56, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    libraryHeading->addWidget(libraryHeadingIcon);
    auto* libraryHeadingText = new QVBoxLayout();
    libraryHeadingText->setSpacing(3);
    auto* libraryTitle = new QLabel(tr("Multiple Instances"), m_libraryPage);
    libraryTitle->setObjectName(QStringLiteral("boostLibraryTitle"));
    libraryHeadingText->addWidget(libraryTitle);
    auto* libraryDescription =
        new QLabel(tr("Keep separate Minecraft setups for different versions, modpacks, and play styles."), m_libraryPage);
    libraryDescription->setObjectName(QStringLiteral("boostPanelCaption"));
    libraryDescription->setWordWrap(true);
    libraryHeadingText->addWidget(libraryDescription);
    libraryHeading->addLayout(libraryHeadingText, 1);
    libraryLayout->addLayout(libraryHeading);

    auto* libraryActions = new QHBoxLayout();
    libraryActions->setSpacing(8);
    auto* libraryCardsButton = new QPushButton(tr("Cards"), m_libraryPage);
    libraryCardsButton->setCheckable(true);
    libraryCardsButton->setChecked(true);
    libraryCardsButton->setObjectName(QStringLiteral("boostLibraryViewButton"));
    auto* libraryListButton = new QPushButton(tr("List"), m_libraryPage);
    libraryListButton->setCheckable(true);
    libraryListButton->setObjectName(QStringLiteral("boostLibraryViewButton"));
    auto* libraryViewButtons = new QButtonGroup(m_libraryPage);
    libraryViewButtons->setExclusive(true);
    libraryViewButtons->addButton(libraryCardsButton);
    libraryViewButtons->addButton(libraryListButton);
    libraryActions->addWidget(libraryCardsButton);
    libraryActions->addWidget(libraryListButton);
    libraryActions->addStretch(1);
    auto* libraryFolderSelector = new QComboBox(m_libraryPage);
    libraryFolderSelector->setObjectName(QStringLiteral("boostLibraryFolderSelector"));
    libraryFolderSelector->setMinimumWidth(140);
    libraryFolderSelector->setIconSize(QSize(18, 18));
    libraryFolderSelector->setAccessibleName(tr("Filter instances by folder"));
    libraryFolderSelector->setToolTip(tr("Show instances from a folder"));
    libraryActions->addWidget(libraryFolderSelector);
    auto* addLibraryInstanceButton = new QPushButton(tr("Add New Instance"), m_libraryPage);
    addLibraryInstanceButton->setObjectName(QStringLiteral("boostLibraryAddButton"));
    addLibraryInstanceButton->setIcon(QIcon::fromTheme("list-add", QApplication::style()->standardIcon(QStyle::SP_FileDialogNewFolder)));
    addLibraryInstanceButton->setMinimumHeight(32);
    connect(addLibraryInstanceButton, &QPushButton::clicked, ui->actionAddInstance, &QAction::trigger);
    libraryActions->addWidget(addLibraryInstanceButton);
    libraryLayout->addLayout(libraryActions);

    m_libraryContentStack = new QStackedWidget(m_libraryPage);
    m_libraryContentStack->setObjectName(QStringLiteral("boostLibraryContentStack"));
    auto* libraryCardsScroll = new QScrollArea(m_libraryContentStack);
    libraryCardsScroll->setObjectName(QStringLiteral("boostLibraryCardsScroll"));
    libraryCardsScroll->setFrameShape(QFrame::NoFrame);
    libraryCardsScroll->setWidgetResizable(true);
    libraryCardsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* libraryCardsContent = new QWidget(libraryCardsScroll);
    libraryCardsContent->setObjectName(QStringLiteral("boostLibraryCardsContent"));
    auto* libraryCardsGrid = new QGridLayout(libraryCardsContent);
    libraryCardsGrid->setObjectName(QStringLiteral("boostLibraryCardsGrid"));
    libraryCardsGrid->setContentsMargins(0, 4, 8, 10);
    libraryCardsGrid->setHorizontalSpacing(12);
    libraryCardsGrid->setVerticalSpacing(12);
    libraryCardsScroll->setWidget(libraryCardsContent);
    m_libraryContentStack->addWidget(libraryCardsScroll);
    libraryLayout->addWidget(m_libraryContentStack, 1);

    auto* accountPage = new QWidget(m_contentPages);
    accountPage->setObjectName(QStringLiteral("boostAccountPage"));
    auto* accountPageLayout = new QVBoxLayout(accountPage);
    accountPageLayout->setContentsMargins(32, 26, 32, 22);
    accountPageLayout->setSpacing(16);

    auto* accountProfileHero = new QFrame(accountPage);
    accountProfileHero->setObjectName(QStringLiteral("boostAccountProfileHero"));
    auto* accountProfileLayout = new QHBoxLayout(accountProfileHero);
    accountProfileLayout->setContentsMargins(16, 13, 16, 13);
    accountProfileLayout->setSpacing(14);
    auto* accountProfileAvatar = new QLabel(accountProfileHero);
    accountProfileAvatar->setObjectName(QStringLiteral("boostAccountProfileAvatar"));
    accountProfileAvatar->setFixedSize(58, 58);
    accountProfileAvatar->setAlignment(Qt::AlignCenter);
    accountProfileLayout->addWidget(accountProfileAvatar);
    auto* accountProfileDetails = new QVBoxLayout();
    accountProfileDetails->setSpacing(1);
    auto* accountProfileKicker = new QLabel(tr("PLAYER PROFILE"), accountProfileHero);
    accountProfileKicker->setObjectName(QStringLiteral("boostAccountProfileKicker"));
    auto* accountProfileName = new QLabel(tr("No account selected"), accountProfileHero);
    accountProfileName->setObjectName(QStringLiteral("boostAccountProfileName"));
    auto* accountProfileStatus = new QLabel(accountProfileHero);
    accountProfileStatus->setObjectName(QStringLiteral("boostAccountProfileStatus"));
    accountProfileDetails->addWidget(accountProfileKicker);
    accountProfileDetails->addWidget(accountProfileName);
    accountProfileDetails->addWidget(accountProfileStatus);
    accountProfileLayout->addLayout(accountProfileDetails, 1);
    auto* manageProfileAccountButton = new QPushButton(tr("Manage account"), accountProfileHero);
    manageProfileAccountButton->setObjectName(QStringLiteral("boostAccountManageButton"));
    manageProfileAccountButton->setIcon(
        QIcon::fromTheme("preferences-system", QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView)));
    manageProfileAccountButton->setFixedSize(148, 36);
    accountProfileLayout->addWidget(manageProfileAccountButton, 0, Qt::AlignVCenter);
    accountPageLayout->addWidget(accountProfileHero);

    auto* accountHeading = new QHBoxLayout();
    accountHeading->setSpacing(16);
    auto* accountHeadingIcon = new QLabel(accountPage);
    accountHeadingIcon->setObjectName(QStringLiteral("boostAccountHeadingIcon"));
    accountHeadingIcon->setFixedSize(54, 54);
    accountHeadingIcon->setAlignment(Qt::AlignCenter);
    accountHeadingIcon->setPixmap(QIcon(QStringLiteral(":/icons/breeze_dark/scalable/accounts.svg"))
                                      .pixmap(48, 48));
    auto* accountTitle = new QLabel(tr("Saved accounts"), accountPage);
    accountTitle->setObjectName(QStringLiteral("boostLibraryTitle"));
    auto* accountDescription = new QLabel(tr("Choose the account you want to use for Minecraft."), accountPage);
    accountDescription->setObjectName(QStringLiteral("boostPanelCaption"));
    accountDescription->setWordWrap(true);
    auto* accountHeadingText = new QVBoxLayout();
    accountHeadingText->setSpacing(2);
    accountHeadingText->addWidget(accountTitle);
    accountHeadingText->addWidget(accountDescription);
    accountHeading->addWidget(accountHeadingIcon);
    accountHeading->addLayout(accountHeadingText, 1);
    accountPageLayout->addLayout(accountHeading);

    auto* accountScrollArea = new QScrollArea(accountPage);
    accountScrollArea->setObjectName(QStringLiteral("boostAccountScrollArea"));
    accountScrollArea->setFrameShape(QFrame::NoFrame);
    accountScrollArea->setWidgetResizable(true);
    accountScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* accountCards = new QWidget(accountScrollArea);
    accountCards->setObjectName(QStringLiteral("boostAccountCards"));
    accountCards->installEventFilter(this);
    auto* accountRowsLayout = new QGridLayout(accountCards);
    accountRowsLayout->setObjectName(QStringLiteral("boostAccountRows"));
    accountRowsLayout->setContentsMargins(0, 8, 0, 8);
    accountRowsLayout->setHorizontalSpacing(16);
    accountRowsLayout->setVerticalSpacing(16);
    accountScrollArea->setWidget(accountCards);
    accountPageLayout->addWidget(accountScrollArea, 1);

    auto* accountActions = new QHBoxLayout();
    accountActions->setSpacing(14);
    auto* addAccountButton = new QPushButton(tr("Add Account"), accountPage);
    addAccountButton->setObjectName(QStringLiteral("boostPlayButton"));
    addAccountButton->setIcon(QIcon::fromTheme("list-add", QApplication::style()->standardIcon(QStyle::SP_FileDialogNewFolder)));
    addAccountButton->setMinimumSize(216, 58);
    auto* manageAccountsButton = new QPushButton(tr("Manage accounts"), accountPage);
    manageAccountsButton->setObjectName(QStringLiteral("boostSecondaryButton"));
    manageAccountsButton->setIcon(QIcon::fromTheme("preferences-system", QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView)));
    manageAccountsButton->setMinimumSize(200, 54);
    accountActions->addWidget(addAccountButton);
    accountActions->addWidget(manageAccountsButton);
    accountActions->addStretch(1);
    accountPageLayout->addLayout(accountActions);

    auto* accountFooterDivider = new QFrame(accountPage);
    accountFooterDivider->setObjectName(QStringLiteral("boostAccountFooterDivider"));
    accountFooterDivider->setFrameShape(QFrame::HLine);
    accountFooterDivider->setFrameShadow(QFrame::Plain);
    accountPageLayout->addWidget(accountFooterDivider);
    auto* accountFooter = new QLabel(tr("ⓘ  You can add multiple accounts, change skins, or remove accounts from here."), accountPage);
    accountFooter->setObjectName(QStringLiteral("boostAccountFooter"));
    accountFooter->setWordWrap(true);
    accountPageLayout->addWidget(accountFooter);

    auto* helpPage = new QWidget(m_contentPages);
    auto* helpPageLayout = new QVBoxLayout(helpPage);
    helpPageLayout->setContentsMargins(28, 24, 28, 24);
    helpPageLayout->setSpacing(14);
    auto* helpTitle = new QLabel(tr("AI Help"), helpPage);
    helpTitle->setObjectName(QStringLiteral("boostLibraryTitle"));
    helpPageLayout->addWidget(helpTitle);
    auto* helpNotice = new QLabel(
        tr("AI chat is not currently integrated in this build. Use the existing support resources below for verified help."),
        helpPage);
    helpNotice->setObjectName(QStringLiteral("boostDashboardPanel"));
    helpNotice->setWordWrap(true);
    helpPageLayout->addWidget(helpNotice);
    auto* helpActions = new QHBoxLayout();
    auto* documentationButton = new QPushButton(tr("Open documentation"), helpPage);
    documentationButton->setObjectName(QStringLiteral("boostSecondaryButton"));
    auto* logsButton = new QPushButton(tr("View launcher logs"), helpPage);
    logsButton->setObjectName(QStringLiteral("boostSecondaryButton"));
    auto* reportButton = new QPushButton(tr("Report a problem"), helpPage);
    reportButton->setObjectName(QStringLiteral("boostSecondaryButton"));
    reportButton->setVisible(!BuildConfig.BUG_TRACKER_URL.isEmpty());
    documentationButton->setVisible(!BuildConfig.WIKI_URL.isEmpty());
    helpActions->addWidget(documentationButton);
    helpActions->addWidget(logsButton);
    helpActions->addWidget(reportButton);
    helpActions->addStretch(1);
    helpPageLayout->addLayout(helpActions);
    helpPageLayout->addStretch(1);

    auto* newsPage = new QWidget(m_contentPages);
    auto* newsPageLayout = new QVBoxLayout(newsPage);
    newsPageLayout->setContentsMargins(28, 24, 28, 24);
    newsPageLayout->setSpacing(12);
    auto* newsTitle = new QLabel(tr("News & updates"), newsPage);
    newsTitle->setObjectName(QStringLiteral("boostLibraryTitle"));
    newsPageLayout->addWidget(newsTitle);
    auto* newsStatus = new QLabel(tr("Loading launcher news…"), newsPage);
    newsStatus->setObjectName(QStringLiteral("boostPanelCaption"));
    newsStatus->setWordWrap(true);
    newsPageLayout->addWidget(newsStatus);
    auto* newsList = new QListWidget(newsPage);
    newsList->setObjectName(QStringLiteral("boostNewsList"));
    newsPageLayout->addWidget(newsList, 1);
    auto* refreshNewsButton = new QPushButton(tr("Refresh news"), newsPage);
    refreshNewsButton->setObjectName(QStringLiteral("boostSecondaryButton"));
    newsPageLayout->addWidget(refreshNewsButton, 0, Qt::AlignLeft);

    m_contentPages->addWidget(m_homePage);
    m_contentPages->addWidget(m_libraryPage);
    m_contentPages->addWidget(accountPage);
    m_contentPages->addWidget(helpPage);
    m_contentPages->addWidget(newsPage);
#ifdef Q_OS_WIN
    auto updateWindowControlsPlacement = [this, titleBar](int index) {
        const bool onHomePage = m_contentPages->widget(index) == m_homePage;
        m_contentPages->setContentsMargins(0, onHomePage ? 0 : titleBar->height(), 0, 0);
        titleBar->raise();
    };
    connect(m_contentPages, &QStackedWidget::currentChanged, this, updateWindowControlsPlacement);
    updateWindowControlsPlacement(m_contentPages->currentIndex());
#endif
    ui->horizontalLayout->addWidget(sidebar);
    contentColumnLayout->addWidget(m_contentPages, 1);
#ifdef Q_OS_WIN
    titleBar->setGeometry(0, 0, 0, titleBar->height());
    titleBar->show();
#endif
    ui->horizontalLayout->addWidget(contentColumn, 1);
#ifdef Q_OS_WIN
    positionWindowControls();
#endif
    ui->instanceToolBar->setVisible(false);

    connect(homeButton, &QAbstractButton::clicked, this, [this] {
        m_contentPages->setCurrentWidget(m_homePage);
        ui->instanceToolBar->setVisible(false);
    });
    connect(libraryButton, &QAbstractButton::clicked, this, [this] {
        m_contentPages->setCurrentWidget(m_libraryPage);
        ui->instanceToolBar->setVisible(true);
    });
    connect(modsNavigationButton, &QAbstractButton::clicked, this, [this, libraryButton] {
        libraryButton->setChecked(true);
        m_contentPages->setCurrentWidget(m_libraryPage);
        ui->instanceToolBar->setVisible(true);
        if (m_selectedInstance) {
            ui->actionEditInstance->trigger();
        } else {
            statusBar()->showMessage(tr("Select an instance to manage its mods."), 5000);
        }
    });
    connect(accountsButton, &QAbstractButton::clicked, this, [this, accountPage] {
        m_contentPages->setCurrentWidget(accountPage);
        ui->instanceToolBar->setVisible(false);
    });
    connect(allNewsButton, &QPushButton::clicked, this, [this, newsPage] {
        m_contentPages->setCurrentWidget(newsPage);
        ui->instanceToolBar->setVisible(false);
    });
    connect(addAccountButton, &QPushButton::clicked, this, &MainWindow::on_actionManageAccounts_triggered);
    connect(manageAccountsButton, &QPushButton::clicked, this, &MainWindow::on_actionManageAccounts_triggered);
    connect(manageProfileAccountButton, &QPushButton::clicked, this, &MainWindow::on_actionManageAccounts_triggered);
    connect(manageProfileButton, &QPushButton::clicked, this, &MainWindow::on_actionManageAccounts_triggered);
    connect(documentationButton, &QPushButton::clicked, ui->actionOpenWiki, &QAction::trigger);
    connect(logsButton, &QPushButton::clicked, ui->actionViewLog, &QAction::trigger);
    connect(reportButton, &QPushButton::clicked, ui->actionReportBug, &QAction::trigger);
    connect(refreshNewsButton, &QPushButton::clicked, this, [this, newsStatus, homeNewsStatus] {
        const auto loadingMessage = tr("Loading launcher news…");
        newsStatus->setText(loadingMessage);
        homeNewsStatus->setText(loadingMessage);
        homeNewsStatus->setVisible(true);
        m_newsChecker->reloadNews();
    });
    connect(newsList, &QListWidget::itemActivated, this, [this](QListWidgetItem*) {
        if (m_newsChecker && !m_newsChecker->getNewsEntries().isEmpty()) {
            on_actionMoreNews_triggered();
        }
    });
    connect(m_homeInstanceSelector, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        const auto id = m_homeInstanceSelector->currentData().toString();
        if (index >= 0 && !id.isEmpty()) {
            setSelectedInstanceById(id);
        }
    });
    auto selectDashboardInstallation = [this](const QString& version, const QString& loader, bool requireLoader) {
        auto* instances = APPLICATION->instances();
        for (int row = 0; row < instances->count(); ++row) {
            auto* instance = instances->at(row);
            if (installedMinecraftVersion(instance) == version &&
                (!requireLoader || installedLoaderLabel(instance) == loader)) {
                setSelectedInstanceById(instance->id());
                return;
            }
        }
    };
    connect(m_homeVersionSelector, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this, selectDashboardInstallation](int index) {
                if (index < 0)
                    return;
                selectDashboardInstallation(m_homeVersionSelector->currentData().toString(),
                                            m_homeLoaderSelector->currentData().toString(), false);
            });
    connect(m_homeLoaderSelector, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this, selectDashboardInstallation](int index) {
                if (index < 0)
                    return;
                selectDashboardInstallation(m_homeVersionSelector->currentData().toString(),
                                            m_homeLoaderSelector->currentData().toString(), true);
            });
    connect(m_homePlayButton, &QPushButton::clicked, this, [this] {
        if (m_selectedInstance && m_selectedInstance->isRunning()) {
            ui->actionKillInstance->trigger();
        } else {
            ui->actionLaunchInstance->trigger();
        }
    });
    connect(m_homeManageButton, &QPushButton::clicked, ui->actionEditInstance, &QAction::trigger);

    // instance toolbar stuff
    {
        connect(ui->instanceToolBar, &QToolBar::orientationChanged, this, &MainWindow::updateInstanceToolBarOrientation);

        // FIXME: why doesn't this work when done in the UI file?
        ui->instanceToolBar->setMinimumWidth(220);
        ui->instanceToolBar->setMaximumWidth(220);
        const QList<QAction*> instanceActions = {
            ui->actionRenameInstance,       ui->actionLaunchInstance, ui->actionLaunchOfflineCustom, ui->actionKillInstance,
            ui->actionEditInstance,         ui->actionChangeInstGroup, ui->actionViewSelectedInstFolder,
            ui->actionExportInstance,       ui->actionCopyInstance, ui->actionDeleteInstance, ui->actionCreateInstanceShortcut,
        };
        for (auto* action : instanceActions) {
            auto* actionButton = ui->instanceToolBar->widgetForAction(action);
            if (!actionButton) {
                continue;
            }
            const auto objectName =
                action == ui->actionLaunchInstance
                    ? QStringLiteral("boostInstanceToolbarLaunch")
                    : ((action == ui->actionKillInstance || action == ui->actionDeleteInstance)
                           ? QStringLiteral("boostInstanceToolbarDanger")
                           : QStringLiteral("boostInstanceToolbarAction"));
            actionButton->setObjectName(objectName);
            actionButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            actionButton->setFixedHeight(28);
            const QColor iconColor =
                action == ui->actionLaunchInstance
                    ? QColor("#17120D")
                    : ((action == ui->actionDeleteInstance || action == ui->actionCreateInstanceShortcut)
                           ? QColor("#F28A1D")
                           : QColor());
            QPixmap actionIcon;
            if (iconColor.isValid()) {
                actionIcon = action->icon().pixmap(QSize(18, 18));
            }
            if (!actionIcon.isNull()) {
                QImage tintedIcon = actionIcon.toImage().convertToFormat(QImage::Format_ARGB32);
                for (int y = 0; y < tintedIcon.height(); ++y) {
                    auto* pixels = reinterpret_cast<QRgb*>(tintedIcon.scanLine(y));
                    for (int x = 0; x < tintedIcon.width(); ++x) {
                        const int alpha = qAlpha(pixels[x]);
                        pixels[x] = qRgba(iconColor.red(), iconColor.green(), iconColor.blue(), alpha);
                    }
                }
                action->setIcon(QIcon(QPixmap::fromImage(tintedIcon)));
            }
            if (auto* toolButton = qobject_cast<QToolButton*>(actionButton)) {
                toolButton->setAutoRaise(false);
            }
        }

        // if you try to add a widget to a toolbar in a .ui file
        // qt designer will delete it when you save the file >:(
        // HACK: the toolbar interferes with the sizing for some reason
        auto* infoWidget = new QWidget(this);
        infoWidget->setObjectName(QStringLiteral("boostInstanceToolbarHeader"));
        auto* infoLayout = new QVBoxLayout(infoWidget);
        infoLayout->setContentsMargins(8, 8, 8, 9);
        infoLayout->setSpacing(5);

        changeIconButton = new QToolButton(infoWidget);
        changeIconButton->setObjectName(QStringLiteral("changeIconButton"));
        changeIconButton->setFixedSize(56, 56);
        changeIconButton->setIconSize({ 48, 48 });
        changeIconButton->setAutoRaise(true);
        updateInstanceToolIcon("default");
        changeIconButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        connect(changeIconButton, &QToolButton::clicked, this, &MainWindow::on_actionChangeInstIcon_triggered);
        infoLayout->addWidget(changeIconButton, 0, Qt::AlignHCenter);

        nameLabel = new QLabel(infoWidget);
        nameLabel->setObjectName(QStringLiteral("nameLabel"));
        nameLabel->installEventFilter(this);
        nameLabel->setAlignment(Qt::AlignCenter);
        // QSizePolicy::Ignored allows the button to shrink or expand as necessary (ignoring the size hint)
        nameLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        nameLabel->setMinimumHeight(20);
        nameLabel->setTextFormat(Qt::PlainText);
        infoLayout->addWidget(nameLabel, 0, Qt::AlignHCenter);

        instanceInfoWidget = ui->instanceToolBar->insertWidgetBefore(ui->actionRenameInstance, infoWidget);
        Q_ASSERT(instanceInfoWidget != nullptr);
        ui->instanceToolBar->insertSpacer(ui->actionCreateInstanceShortcut);
    }

    // set the menu for the folders help, accounts, and export tool buttons
    {
        auto foldersMenuButton = dynamic_cast<QToolButton*>(ui->mainToolBar->widgetForAction(ui->actionFoldersButton));
        ui->actionFoldersButton->setMenu(ui->foldersMenu);
        foldersMenuButton->setPopupMode(QToolButton::InstantPopup);

        helpMenuButton = dynamic_cast<QToolButton*>(ui->mainToolBar->widgetForAction(ui->actionHelpButton));
        ui->actionHelpButton->setMenu(new QMenu(this));
        ui->actionHelpButton->menu()->addActions(ui->helpMenu->actions());
        ui->actionHelpButton->menu()->removeAction(ui->actionCheckUpdate);
        helpMenuButton->setPopupMode(QToolButton::InstantPopup);

        auto accountMenuButton = dynamic_cast<QToolButton*>(ui->mainToolBar->widgetForAction(ui->actionAccountsButton));
        ui->actionAccountsButton->setVisible(true);
        accountMenuButton->setPopupMode(QToolButton::InstantPopup);
        accountMenuButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        m_homeAccountButton = new QPushButton(hero);
        m_homeAccountButton->setObjectName(QStringLiteral("boostAccountMenuButton"));
        m_homeAccountButton->setIconSize({ 24, 24 });
        m_homeAccountButton->setMinimumSize(180, 42);
        m_homeAccountButton->setCursor(Qt::PointingHandCursor);
        m_homeAccountButton->setText(tr("Accounts"));
        m_homeAccountButton->setIcon(QIcon::fromTheme("accounts", QApplication::style()->standardIcon(QStyle::SP_DirHomeIcon)));
        m_homeAccountButton->setMenu(ui->actionAccountsButton->menu());
        connect(ui->actionAccountsButton, &QAction::changed, m_homeAccountButton, [action = ui->actionAccountsButton, this] {
            m_homeAccountButton->setText(action->text().isEmpty() ? QObject::tr("Accounts") : action->text());
            m_homeAccountButton->setIcon(action->icon().isNull()
                                             ? QIcon::fromTheme("accounts", QApplication::style()->standardIcon(QStyle::SP_DirHomeIcon))
                                             : action->icon());
            m_homeAccountButton->setMenu(action->menu());
            m_homeAccountButton->setEnabled(action->isEnabled());
        });
        m_homeAccountButton->setVisible(true);
        greetingRow->addStretch(1);
        greetingRow->addWidget(m_homeAccountButton);

        for (QAction* action : ui->mainToolBar->actions()) {
            if (action != ui->actionAccountsButton) {
                ui->mainToolBar->removeAction(action);
            }
        }

        auto exportInstanceMenu = new QMenu(this);
        exportInstanceMenu->addAction(ui->actionExportInstanceZip);
        exportInstanceMenu->addAction(ui->actionExportInstanceMrPack);
        exportInstanceMenu->addAction(ui->actionExportInstanceFlamePack);
        ui->actionExportInstance->setMenu(exportInstanceMenu);
    }

    // hide, disable and show stuff
    {
        ui->actionReportBug->setVisible(!BuildConfig.BUG_TRACKER_URL.isEmpty());
        ui->actionMATRIX->setVisible(!BuildConfig.MATRIX_URL.isEmpty());
        ui->actionDISCORD->setVisible(!BuildConfig.DISCORD_URL.isEmpty());
        ui->actionREDDIT->setVisible(!BuildConfig.SUBREDDIT_URL.isEmpty());

        ui->actionCheckUpdate->setVisible(APPLICATION->updaterEnabled());

#ifndef Q_OS_MAC
        ui->actionAddToPATH->setVisible(false);
#endif

        // disabled until we have an instance selected
        ui->instanceToolBar->setEnabled(false);
        setInstanceActionsEnabled(false);

        // add a close button at the end of the main toolbar when running on gamescope / steam deck
        // this is only needed on gamescope because it defaults to an X11/XWayland session and
        // does not implement decorations
        if (qgetenv("XDG_CURRENT_DESKTOP") == "gamescope") {
            ui->mainToolBar->addAction(ui->actionCloseWindow);
        }

        ui->actionViewJavaFolder->setEnabled(BuildConfig.JAVA_DOWNLOADER_ENABLED);
    }

    {  // logs viewing
        connect(ui->actionViewLog, &QAction::triggered, this, [] { APPLICATION->showLogWindow(); });
    }

    // add the toolbar toggles to the view menu
    ui->viewMenu->addAction(ui->instanceToolBar->toggleViewAction());
    ui->viewMenu->addAction(ui->newsToolBar->toggleViewAction());

    updateThemeMenu();
    updateMainToolBar();
    // OSX magic.
    setUnifiedTitleAndToolBarOnMac(true);

    // Global shortcuts
    {
        // you can't set QKeySequence::StandardKey shortcuts in qt designer >:(
        ui->actionAddInstance->setShortcut(QKeySequence::New);
        ui->actionSettings->setShortcut(QKeySequence::Preferences);
        ui->actionUndoTrashInstance->setShortcut(QKeySequence::Undo);
        ui->actionDeleteInstance->setShortcuts({ QKeySequence(tr("Backspace")), QKeySequence::Delete });
        ui->actionCloseWindow->setShortcut(QKeySequence::Close);
        connect(ui->actionCloseWindow, &QAction::triggered, APPLICATION, &Application::closeCurrentWindow);

        // FIXME: This is kinda weird. and bad. We need some kind of managed shutdown.
        auto q = new QShortcut(QKeySequence::Quit, this);
        connect(q, &QShortcut::activated, APPLICATION, &Application::quit);
    }

    // Konami Code
    {
        secretEventFilter = new KonamiCode(this);
        connect(secretEventFilter, &KonamiCode::triggered, this, &MainWindow::konamiTriggered);
    }

    // Add the news label to the news toolbar.
    {
        const auto newsFeedUrl = BuildConfig.NEWS_RSS_URL.isEmpty()
                                     ? QStringLiteral("https://www.minecraft.net/en-us/feeds/community-content/rss")
                                     : BuildConfig.NEWS_RSS_URL;
        m_newsChecker.reset(new NewsChecker(APPLICATION->network(), newsFeedUrl));
        newsLabel = new QToolButton();
        newsLabel->setIcon(QIcon::fromTheme("news"));
        newsLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        newsLabel->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        newsLabel->setFocusPolicy(Qt::NoFocus);
        ui->newsToolBar->insertWidget(ui->actionMoreNews, newsLabel);
        ui->newsToolBar->hide();

        connect(newsLabel, &QAbstractButton::clicked, this, &MainWindow::newsButtonClicked);
        connect(m_newsChecker.get(), &NewsChecker::newsLoaded, this, &MainWindow::updateNewsLabel);
        connect(m_newsChecker.get(), &NewsChecker::newsLoaded, this, [this, newsList, newsStatus, homeNewsStatus] {
            newsList->clear();
            auto* homeNewsItems = findChild<QVBoxLayout*>(QStringLiteral("boostHomeNewsItems"));
            if (homeNewsItems) {
                while (auto* item = homeNewsItems->takeAt(0)) {
                    delete item->widget();
                    delete item;
                }
            }
            const auto entries = m_newsChecker->getNewsEntries();
            for (const auto& entry : entries) {
                QTextDocument document;
                document.setHtml(entry->content);
                const auto excerpt = document.toPlainText().simplified();

                auto* item = new QListWidgetItem(entry->title, newsList);
                item->setToolTip(excerpt);
                item->setData(Qt::UserRole, entry->link);

                if (homeNewsItems && homeNewsItems->count() < 3) {
                    auto* card = new QFrame(this);
                    card->setObjectName(QStringLiteral("boostHomeNewsCard"));
                    card->setMinimumWidth(320);
                    card->setMaximumWidth(460);
                    auto* cardLayout = new QHBoxLayout(card);
                    cardLayout->setContentsMargins(14, 10, 14, 10);
                    cardLayout->setSpacing(12);
                    auto* articleIcon = new QLabel(card);
                    articleIcon->setObjectName(QStringLiteral("boostNewsCardIcon"));
                    articleIcon->setFixedSize(100, 76);
                    articleIcon->setAlignment(Qt::AlignCenter);
                    articleIcon->setPixmap(QIcon(QStringLiteral(":/icons/breeze_dark/scalable/news.svg")).pixmap(36, 36));
                    cardLayout->addWidget(articleIcon);
                    auto* text = new QVBoxLayout();
                    text->setSpacing(3);
                    auto* title = new QLabel(entry->title, card);
                    title->setObjectName(QStringLiteral("boostNewsCardTitle"));
                    title->setWordWrap(true);
                    text->addWidget(title);
                    auto* summary = new QLabel(excerpt, card);
                    summary->setObjectName(QStringLiteral("boostNewsCardExcerpt"));
                    summary->setWordWrap(true);
                    summary->setMaximumHeight(40);
                    text->addWidget(summary);
                    cardLayout->addLayout(text, 1);

                    if (QUrl(entry->link).isValid() && !entry->link.isEmpty()) {
                        auto* readButton = new QPushButton(tr("Read"), card);
                        readButton->setObjectName(QStringLiteral("boostSecondaryButton"));
                        connect(readButton, &QPushButton::clicked, this, [link = entry->link] {
                            DesktopServices::openUrl(QUrl(link));
                        });
                        cardLayout->addWidget(readButton);
                    }
                    homeNewsItems->addWidget(card);

                    static const QRegularExpression imageSource(
                        QStringLiteral("<img\\b[^>]*\\bsrc\\s*=\\s*[\"']([^\"']+)[\"']"),
                        QRegularExpression::CaseInsensitiveOption);
                    const auto imageMatch = imageSource.match(entry->content);
                    if (imageMatch.hasMatch()) {
                        const QUrl imageUrl(imageMatch.captured(1));
                        if (imageUrl.scheme() == QStringLiteral("https") || imageUrl.scheme() == QStringLiteral("http")) {
                            auto* reply = APPLICATION->network()->get(QNetworkRequest(imageUrl));
                            connect(reply, &QNetworkReply::downloadProgress, reply, [reply](qint64 received, qint64 total) {
                                if (received > 2 * 1024 * 1024 || total > 2 * 1024 * 1024) {
                                    reply->abort();
                                }
                            });
                            connect(reply, &QNetworkReply::finished, card, [reply, articleIcon] {
                                const auto imageData = reply->error() == QNetworkReply::NoError ? reply->readAll() : QByteArray();
                                QPixmap thumbnail;
                                if (!imageData.isEmpty() && thumbnail.loadFromData(imageData)) {
                                    articleIcon->setPixmap(thumbnail.scaled(articleIcon->size(), Qt::KeepAspectRatioByExpanding,
                                                                            Qt::SmoothTransformation));
                                }
                                reply->deleteLater();
                            });
                        }
                    }
                }
            }
            const auto status = entries.isEmpty() ? tr("No news articles are available right now.")
                                                  : tr("Latest Minecraft launcher announcements");
            newsStatus->setText(status);
            homeNewsStatus->setText(entries.isEmpty() ? status : QString());
            homeNewsStatus->setVisible(entries.isEmpty());
        });
        connect(m_newsChecker.get(), &NewsChecker::newsLoadingFailed, this, [newsStatus, homeNewsStatus](const QString& error) {
            qWarning() << "Unable to load launcher news:" << error;
            const auto message = QObject::tr("Could not load Minecraft news. Check your connection and try again.");
            newsStatus->setText(message);
            homeNewsStatus->setText(message);
            homeNewsStatus->setVisible(true);
        });
        updateNewsLabel();
    }

    // Create the instance list widget
    {
        view = new InstanceView(m_libraryContentStack);

        view->setSelectionMode(QAbstractItemView::SingleSelection);
        // FIXME: leaks ListViewDelegate
        auto delegate = new ListViewDelegate(this);
        view->setItemDelegate(delegate);
        view->setFrameShape(QFrame::NoFrame);
        // do not show ugly blue border on the mac
        view->setAttribute(Qt::WA_MacShowFocusRect, false);
        connect(delegate, &ListViewDelegate::textChanged, this, [this](QString before, QString after) {
            if (auto newRoot = askToUpdateInstanceDirName(m_selectedInstance, before, after, this); !newRoot.isEmpty()) {
                auto oldID = m_selectedInstance->id();
                auto newID = QFileInfo(newRoot).fileName();
                QString origGroup(APPLICATION->instances()->getInstanceGroup(oldID));
                bool syncGroup = origGroup != GroupId() && oldID != newID;
                if (syncGroup)
                    APPLICATION->instances()->setInstanceGroup(oldID, GroupId());

                refreshInstances();
                setSelectedInstanceById(newID);

                if (syncGroup)
                    APPLICATION->instances()->setInstanceGroup(newID, origGroup);
            }
        });

        view->installEventFilter(this);
        view->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(view, &QWidget::customContextMenuRequested, this, &MainWindow::showInstanceContextMenu);
        connect(view, &InstanceView::droppedURLs, this, &MainWindow::processURLs, Qt::QueuedConnection);

        proxymodel = new InstanceProxyModel(this);
        proxymodel->setSourceModel(APPLICATION->instances());
        proxymodel->sort(0);
        proxymodel->setFilterRole(InstanceViewRoles::GroupRole);
        proxymodel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        connect(proxymodel, &InstanceProxyModel::dataChanged, this, &MainWindow::instanceDataChanged);

        view->setModel(proxymodel);
        view->setSourceOfGroupCollapseStatus(
            [](const QString& groupName) -> bool { return APPLICATION->instances()->isGroupCollapsed(groupName); });
        connect(view, &InstanceView::groupStateChanged, APPLICATION->instances(), &InstanceList::on_GroupStateChanged);
        m_libraryContentStack->addWidget(view);
        auto* folderSelector = findChild<QComboBox*>(QStringLiteral("boostLibraryFolderSelector"));
        connect(folderSelector, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
            updateLibraryCards();
        });
        connect(libraryCardsButton, &QPushButton::clicked, this, [this] { m_libraryContentStack->setCurrentIndex(0); });
        connect(libraryListButton, &QPushButton::clicked, this, [this] { m_libraryContentStack->setCurrentWidget(view); });
        auto updateLibraryInstances = [this] { updateLibraryCards(); };
        connect(APPLICATION->instances(), &QAbstractItemModel::rowsInserted, this, updateLibraryInstances);
        connect(APPLICATION->instances(), &QAbstractItemModel::rowsRemoved, this, updateLibraryInstances);
        connect(APPLICATION->instances(), &QAbstractItemModel::modelReset, this, updateLibraryInstances);
        connect(APPLICATION->instances(), &QAbstractItemModel::dataChanged, this, updateLibraryInstances);
    }
    // The cat background
    {
        // set the cat action priority here so you can still see the action in qt designer
        ui->actionCAT->setPriority(QAction::LowPriority);
        updateCatState();
        connect(ui->actionCAT, &QAction::toggled, this, &MainWindow::onCatToggled);
        connect(APPLICATION, &Application::currentCatChanged, this, &MainWindow::onCatChanged);
    }

    // Togglable status bar
    {
        bool statusBarVisible = APPLICATION->settings()->get("StatusBarVisible").toBool();
        ui->actionToggleStatusBar->setChecked(statusBarVisible);
        connect(ui->actionToggleStatusBar, &QAction::toggled, this, &MainWindow::setStatusBarVisibility);
        setStatusBarVisibility(statusBarVisible);
    }

    // Lock toolbars
    {
        bool toolbarsLocked = APPLICATION->settings()->get("ToolbarsLocked").toBool();
        ui->actionLockToolbars->setChecked(toolbarsLocked);
        connect(ui->actionLockToolbars, &QAction::toggled, this, &MainWindow::lockToolbars);
        lockToolbars(toolbarsLocked);
    }
    // start instance when double-clicked
    connect(view, &InstanceView::activated, this, &MainWindow::instanceActivated);

    connect(ui->actionLaunchOfflineCustom, &QAction::triggered, this, [this] {
        if (m_selectedInstance && !m_selectedInstance->isRunning()) {
            APPLICATION->launch(m_selectedInstance, LaunchMode::Offline);
        }
    });

    // track the selection -- update the instance toolbar
    connect(view->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::instanceChanged);

    // track icon changes and update the toolbar!
    connect(APPLICATION->icons(), &IconList::iconUpdated, this, &MainWindow::iconUpdated);

    // model reset -> selection is invalid. All the instance pointers are wrong.
    connect(APPLICATION->instances(), &InstanceList::dataIsInvalid, this, &MainWindow::selectionBad);

    // handle newly added instances
    connect(APPLICATION->instances(), &InstanceList::instanceSelectRequest, this, &MainWindow::instanceSelectRequest);

    // When the global settings page closes, we want to know about it and update our state
    connect(APPLICATION, &Application::globalSettingsApplied, this, &MainWindow::globalSettingsClosed);

    m_statusLeft = new QLabel(tr("No instance selected"), this);
    m_statusCenter = new QLabel(tr("Total playtime: 0s"), this);
    statusBar()->addPermanentWidget(m_statusLeft, 1);
    statusBar()->addPermanentWidget(m_statusCenter, 0);

    // Use undocumented property... https://stackoverflow.com/questions/7121718/create-a-scrollbar-in-a-submenu-qt
    ui->accountsMenu->setStyleSheet("QMenu { menu-scrollable: 1; }");

    repopulateAccountsMenu();
    // Update the menu when the active account changes.
    // Shouldn't have to use lambdas here like this, but if I don't, the compiler throws a fit.
    // Template hell sucks...
    connect(APPLICATION->accounts(), &AccountList::defaultAccountChanged, this, [this] { defaultAccountChanged(); });
    connect(APPLICATION->accounts(), &AccountList::listActivityChanged, this, [this] { defaultAccountChanged(); });
    connect(APPLICATION->accounts(), &AccountList::listChanged, this, [this] { defaultAccountChanged(); });

    // Show initial account
    defaultAccountChanged();

    // TODO: refresh accounts here?
    // auto accounts = APPLICATION->accounts();

    // load the news
    {
        m_newsChecker->reloadNews();
        updateNewsLabel();
    }

    if (APPLICATION->updaterEnabled()) {
        bool updatesAllowed = APPLICATION->updatesAreAllowed();
        updatesAllowedChanged(updatesAllowed);

        connect(ui->actionCheckUpdate, &QAction::triggered, this, &MainWindow::checkForUpdates);

        // set up the updater object.
        auto updater = APPLICATION->updater();

        if (updater) {
            connect(updater, &ExternalUpdater::canCheckForUpdatesChanged, this, &MainWindow::updatesAllowedChanged);
        }
    }

    connect(ui->actionUndoTrashInstance, &QAction::triggered, this, &MainWindow::undoTrashInstance);

    setSelectedInstanceById(APPLICATION->settings()->get("SelectedInstance").toString());

    // removing this looks stupid
    view->setFocus();

    QTimer::singleShot(0, this, [this] {
        if (QScreen* activeScreen = screen()) {
            const QRect availableGeometry = activeScreen->availableGeometry();
            const QSize startupSize(qMin(1280, int(availableGeometry.width() * 0.88)),
                                    qMin(720, int(availableGeometry.height() * 0.88)));
            if (width() > startupSize.width() || height() > startupSize.height()) {
                setWindowState(windowState() & ~Qt::WindowMaximized);
                resize(startupSize);
                move(availableGeometry.center() - rect().center());
            }
        }
        ui->mainToolBar->setVisible(false);
        ui->menuBar->hide();
        ui->newsToolBar->hide();
        addToolBar(Qt::RightToolBarArea, ui->instanceToolBar);
        ui->instanceToolBar->setVisible(m_contentPages->currentWidget() == m_libraryPage);
    });
    retranslateUi();
}

// macOS always has a native menu bar, so these fixes are not applicable
// Other systems may or may not have a native menu bar (most do not - it seems like only Ubuntu Unity does)
#ifndef Q_OS_MAC
void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Alt) {
        ui->menuBar->setVisible(false);
        return;
    }
    QMainWindow::keyReleaseEvent(event);
}
#endif

void MainWindow::retranslateUi()
{
    if (m_selectedInstance) {
        m_statusLeft->setText(m_selectedInstance->getStatusbarDescription());
    } else {
        m_statusLeft->setText(tr("No instance selected"));
    }

    ui->retranslateUi(this);

    MinecraftAccountPtr defaultAccount = APPLICATION->accounts()->defaultAccount();
    if (defaultAccount) {
        auto profileLabel = profileInUseFilter(defaultAccount->displayName(), defaultAccount->isInUse());
        ui->actionAccountsButton->setText(profileLabel);
    }

    changeIconButton->setToolTip(ui->actionChangeInstIcon->toolTip());

    // replace the %1 with the launcher display name in some actions
    if (helpMenuButton->toolTip().contains("%1"))
        helpMenuButton->setToolTip(helpMenuButton->toolTip().arg(BuildConfig.LAUNCHER_DISPLAYNAME));

    for (auto action : ui->helpMenu->actions()) {
        if (action->text().contains("%1"))
            action->setText(action->text().arg(BuildConfig.LAUNCHER_DISPLAYNAME));
        if (action->toolTip().contains("%1"))
            action->setToolTip(action->toolTip().arg(BuildConfig.LAUNCHER_DISPLAYNAME));
    }
}

MainWindow::~MainWindow()
{
    if (m_trayIcon) {
        m_trayIcon->hide();
        m_trayIcon->deleteLater();
    }
}

QMenu* MainWindow::createPopupMenu()
{
    QMenu* filteredMenu = QMainWindow::createPopupMenu();
    filteredMenu->removeAction(ui->mainToolBar->toggleViewAction());

    filteredMenu->addAction(ui->actionToggleStatusBar);
    filteredMenu->addAction(ui->actionLockToolbars);

    return filteredMenu;
}
void MainWindow::setStatusBarVisibility(bool state)
{
    statusBar()->setVisible(state);
    APPLICATION->settings()->set("StatusBarVisible", state);
}
void MainWindow::lockToolbars(bool state)
{
    ui->mainToolBar->setMovable(!state);
    ui->instanceToolBar->setMovable(!state);
    ui->newsToolBar->setMovable(!state);
    APPLICATION->settings()->set("ToolbarsLocked", state);
}

void MainWindow::konamiTriggered()
{
    QString gradient =
        " stop:0 rgba(125, 0, 0, 255), stop:0.166 rgba(125, 125, 0, 255), stop:0.333 rgba(0, 125, 0, 255), stop:0.5 rgba(0, 125, 125, "
        "255), stop:0.666 rgba(0, 0, 125, 255), stop:0.833 rgba(125, 0, 125, 255), stop:1 rgba(125, 0, 0, 255));";
    QString stylesheet = "background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0," + gradient;
    if (ui->mainToolBar->styleSheet() == stylesheet) {
        ui->mainToolBar->setStyleSheet("");
        ui->instanceToolBar->setStyleSheet("");
        ui->centralWidget->setStyleSheet("");
        ui->newsToolBar->setStyleSheet("");
        ui->statusBar->setStyleSheet("");
        qDebug() << "Super Secret Mode DEACTIVATED!";
    } else {
        ui->mainToolBar->setStyleSheet(stylesheet);
        ui->instanceToolBar->setStyleSheet("background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1," + gradient);
        ui->centralWidget->setStyleSheet("background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:1," + gradient);
        ui->newsToolBar->setStyleSheet(stylesheet);
        ui->statusBar->setStyleSheet(stylesheet);
        qDebug() << "Super Secret Mode ACTIVATED!";
    }
}

void MainWindow::showInstanceContextMenu(const QPoint& pos)
{
    QList<QAction*> actions;

    QAction* actionSep = new QAction("", this);
    actionSep->setSeparator(true);

    bool onInstance = view->indexAt(pos).isValid();
    if (onInstance) {
        // reuse the file menu actions
        actions = ui->fileMenu->actions();

        // remove the add instance action, launcher settings action and close action
        actions.removeFirst();
        actions.removeLast();
        actions.removeLast();

        actions.prepend(ui->actionChangeInstIcon);
        actions.prepend(ui->actionRenameInstance);

        // add header
        actions.prepend(actionSep);
        QAction* actionVoid = new QAction(m_selectedInstance->name(), this);
        actionVoid->setEnabled(false);
        actions.prepend(actionVoid);
    } else {
        auto group = view->groupNameAt(pos);

        QAction* actionVoid = new QAction(group.isNull() ? BuildConfig.LAUNCHER_DISPLAYNAME : group, this);
        actionVoid->setEnabled(false);

        QAction* actionCreateInstance = new QAction(tr("&Create instance"), this);
        actionCreateInstance->setToolTip(ui->actionAddInstance->toolTip());
        if (!group.isNull()) {
            QVariantMap instance_action_data;
            instance_action_data["group"] = group;
            actionCreateInstance->setData(instance_action_data);
        }

        connect(actionCreateInstance, &QAction::triggered, this, &MainWindow::on_actionAddInstance_triggered);

        actions.prepend(actionSep);
        actions.prepend(actionVoid);
        actions.append(actionCreateInstance);
        if (!group.isNull()) {
            QAction* actionDeleteGroup = new QAction(tr("&Delete group"), this);
            connect(actionDeleteGroup, &QAction::triggered, this, [this, group] { deleteGroup(group); });
            actions.append(actionDeleteGroup);

            QAction* actionRenameGroup = new QAction(tr("&Rename group"), this);
            connect(actionRenameGroup, &QAction::triggered, this, [this, group] { renameGroup(group); });
            actions.append(actionRenameGroup);
        }
    }
    QMenu myMenu;
    myMenu.addActions(actions);
    /*
    if (onInstance)
        myMenu.setEnabled(m_selectedInstance->canLaunch());
    */
    myMenu.exec(view->mapToGlobal(pos));
}

void MainWindow::updateMainToolBar()
{
    ui->menuBar->setVisible(false);
    ui->mainToolBar->setVisible(false);
}

void MainWindow::updateLaunchButton()
{
    updateHomeDashboard();

    QMenu* launchMenu = ui->actionLaunchInstance->menu();
    if (launchMenu)
        launchMenu->clear();
    else
        launchMenu = new QMenu(this);
    if (m_selectedInstance)
        m_selectedInstance->populateLaunchMenu(launchMenu);
    ui->actionLaunchInstance->setMenu(launchMenu);
}

void MainWindow::updateLibraryCards()
{
    auto* cardsGrid = findChild<QGridLayout*>(QStringLiteral("boostLibraryCardsGrid"));
    auto* cardsContent = findChild<QWidget*>(QStringLiteral("boostLibraryCardsContent"));
    if (!cardsGrid || !cardsContent || !proxymodel) {
        return;
    }

    auto* folderSelector = findChild<QComboBox*>(QStringLiteral("boostLibraryFolderSelector"));
    if (folderSelector) {
        const QString currentGroup = folderSelector->currentData().toString();
        const QSignalBlocker blocker(folderSelector);
        folderSelector->clear();
        folderSelector->addItem(QIcon::fromTheme("folder-open"), tr("All Folders"), QString());
        for (const auto& group : APPLICATION->instances()->getGroups()) {
            folderSelector->addItem(QIcon::fromTheme("folder"), group, group);
        }
        const int selectedIndex = folderSelector->findData(currentGroup);
        folderSelector->setCurrentIndex(selectedIndex >= 0 ? selectedIndex : 0);
        proxymodel->setFilterFixedString(folderSelector->currentData().toString());
    } else {
        proxymodel->setFilterFixedString(QString());
    }

    while (auto* item = cardsGrid->takeAt(0)) {
        if (auto* widget = item->widget()) {
            delete widget;
        }
        delete item;
    }

    auto* instances = APPLICATION->instances();
    if (proxymodel->rowCount() == 0) {
        auto* emptyState = new QLabel(tr("No Minecraft installations yet. Create an instance to get started."), cardsContent);
        emptyState->setObjectName(QStringLiteral("boostLibraryEmptyState"));
        emptyState->setAlignment(Qt::AlignCenter);
        emptyState->setMinimumHeight(120);
        cardsGrid->addWidget(emptyState, 0, 0, 1, 2);
        return;
    }

    constexpr int cardMinimumWidth = 280;
    constexpr int cardSpacing = 12;
    const int columnCount = qBound(1, (cardsContent->width() + cardSpacing) / (cardMinimumWidth + cardSpacing), 3);
    for (int column = 0; column < 3; ++column) {
        cardsGrid->setColumnStretch(column, column < columnCount ? 1 : 0);
    }

    for (int row = 0; row < proxymodel->rowCount(); ++row) {
        const auto proxyIndex = proxymodel->index(row, 0);
        const auto sourceIndex = proxymodel->mapToSource(proxyIndex);
        auto* instance = instances->at(sourceIndex.row());
        const auto id = instance->id();

        auto icon = instanceTypeIcon(instance);
        if (icon.isNull()) {
            icon = qvariant_cast<QIcon>(sourceIndex.data(Qt::DecorationRole));
        }
        if (icon.isNull()) {
            icon = QIcon(QStringLiteral(":/backgrounds/boostclient-grasss"));
        }

        auto* card = new ClickableInstanceFrame(cardsContent);
        card->setObjectName(QStringLiteral("boostInstanceCard"));
        card->setMinimumSize(cardMinimumWidth, 122);
        card->setMaximumWidth(300);
        card->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        card->setProperty("selected", instance == m_selectedInstance);
        card->setAccessibleName(tr("%1, Minecraft %2, %3")
                                    .arg(instance->name(), installedMinecraftVersion(instance), installedLoaderLabel(instance)));
        card->setClickHandler([this, id] { setSelectedInstanceById(id); });

        auto* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(10, 9, 10, 9);
        cardLayout->setSpacing(7);
        auto* identityRow = new QHBoxLayout();
        identityRow->setSpacing(10);
        auto* iconLabel = new QLabel(card);
        iconLabel->setObjectName(QStringLiteral("boostInstanceCardIcon"));
        iconLabel->setFixedSize(56, 56);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setPixmap(icon.pixmap(48, 48));
        identityRow->addWidget(iconLabel);

        auto* details = new QVBoxLayout();
        details->setSpacing(5);
        auto* titleRow = new QHBoxLayout();
        auto* name = new QLabel(instance->name(), card);
        name->setObjectName(QStringLiteral("boostInstanceCardName"));
        name->setTextInteractionFlags(Qt::TextSelectableByMouse);
        titleRow->addWidget(name, 1);
        auto* moreButton = new QToolButton(card);
        moreButton->setText(QStringLiteral("..."));
        moreButton->setAccessibleName(tr("More actions for %1").arg(instance->name()));
        moreButton->setPopupMode(QToolButton::InstantPopup);
        auto* moreMenu = new QMenu(moreButton);
        auto* configureAction = moreMenu->addAction(tr("Configure"));
        auto* duplicateAction = moreMenu->addAction(tr("Duplicate"));
        auto* deleteAction = moreMenu->addAction(tr("Delete"));
        connect(configureAction, &QAction::triggered, this, [this, id] {
            setSelectedInstanceById(id);
            ui->actionEditInstance->trigger();
        });
        connect(duplicateAction, &QAction::triggered, this, [this, id] {
            setSelectedInstanceById(id);
            ui->actionCopyInstance->trigger();
        });
        connect(deleteAction, &QAction::triggered, this, [this, id] {
            setSelectedInstanceById(id);
            ui->actionDeleteInstance->trigger();
        });
        moreButton->setMenu(moreMenu);
        titleRow->addWidget(moreButton);
        details->addLayout(titleRow);

        auto* metadataRow = new QHBoxLayout();
        metadataRow->setSpacing(7);
        const auto minecraftVersion = installedMinecraftVersion(instance);
        if (minecraftVersion.compare(instance->name(), Qt::CaseInsensitive) != 0) {
            auto* version = new QLabel(minecraftVersion, card);
            version->setObjectName(QStringLiteral("boostInstanceCardMeta"));
            metadataRow->addWidget(version);
        }
        auto* loader = new QLabel(installedLoaderLabel(instance), card);
        loader->setObjectName(QStringLiteral("boostInstanceCardBadge"));
        loader->setAlignment(Qt::AlignCenter);
        loader->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
        metadataRow->addWidget(loader);
        metadataRow->addStretch(1);
        details->addLayout(metadataRow);
        auto* status = new QLabel(instance->isRunning() ? tr("● Running")
                                                        : (instance->canLaunch() ? tr("● Ready to play") : tr("● Needs setup")),
                                 card);
        status->setObjectName(instance->isRunning() || instance->canLaunch()
                                  ? QStringLiteral("boostInstanceCardStatus")
                                  : QStringLiteral("boostInstanceCardMeta"));
        status->setToolTip(instance->getStatusbarDescription());
        details->addWidget(status);
        identityRow->addLayout(details, 1);
        cardLayout->addLayout(identityRow, 1);

        auto* actions = new QHBoxLayout();
        auto* manageButton = new QPushButton(tr("Manage"), card);
        manageButton->setObjectName(QStringLiteral("boostSecondaryButton"));
        connect(manageButton, &QPushButton::clicked, this, [this, id] {
            setSelectedInstanceById(id);
            ui->actionEditInstance->trigger();
        });
        actions->addWidget(manageButton);
        actions->addStretch(1);
        auto* launchButton = new QPushButton(instance->isRunning() ? tr("Stop") : tr("Launch"), card);
        launchButton->setObjectName(QStringLiteral("boostLibraryLaunchButton"));
        launchButton->setEnabled(instance->isRunning() || instance->canLaunch());
        connect(launchButton, &QPushButton::clicked, this, [this, id] {
            setSelectedInstanceById(id);
            if (m_selectedInstance && m_selectedInstance->isRunning()) {
                ui->actionKillInstance->trigger();
            } else {
                ui->actionLaunchInstance->trigger();
            }
        });
        actions->addWidget(launchButton);
        cardLayout->addLayout(actions);

        const int cardIndex = row;
        cardsGrid->addWidget(card, cardIndex / columnCount, cardIndex % columnCount, Qt::AlignLeft | Qt::AlignTop);
    }
}

void MainWindow::updateHomeDashboard()
{
    if (!m_homeProfileName || !m_homeVersionSelector || !m_homeLoaderSelector)
        return;

    auto* instances = APPLICATION->instances();
    QStringList versions;
    {
        const QSignalBlocker instanceBlocker(m_homeInstanceSelector);
        m_homeInstanceSelector->clear();
        if (instances->count() > 0) {
            m_homeInstanceSelector->addItem(tr("Select an installation"), QString());
        }
        for (int row = 0; row < instances->count(); ++row) {
            auto* instance = instances->at(row);
            const auto sourceIndex = instances->index(row, 0);
            auto icon = instanceTypeIcon(instance);
            if (icon.isNull()) {
                icon = qvariant_cast<QIcon>(sourceIndex.data(Qt::DecorationRole));
            }
            const auto name = sourceIndex.data(Qt::DisplayRole).toString();
            const auto id = sourceIndex.data(InstanceList::InstanceIDRole).toString();
            m_homeInstanceSelector->addItem(icon, name, id);

            const auto version = installedMinecraftVersion(instance);
            if (!versions.contains(version)) {
                versions.append(version);
            }
        }

        if (instances->count() == 0) {
            m_homeInstanceSelector->addItem(tr("No installations"), QString());
            m_homeInstanceSelector->setEnabled(false);
        } else {
            m_homeInstanceSelector->setEnabled(true);
            const auto selectedId = m_selectedInstance ? m_selectedInstance->id() : QString();
            const int selectedIndex = selectedId.isEmpty() ? 0 : m_homeInstanceSelector->findData(selectedId);
            m_homeInstanceSelector->setCurrentIndex(selectedIndex >= 0 ? selectedIndex : 0);
        }
    }

    QString selectedVersion = m_selectedInstance ? installedMinecraftVersion(m_selectedInstance)
                                                  : (versions.isEmpty() ? QString() : versions.first());
    {
        const QSignalBlocker blocker(m_homeVersionSelector);
        m_homeVersionSelector->clear();
        for (const auto& version : versions) {
            m_homeVersionSelector->addItem(QIcon(QStringLiteral(":/icons/breeze_dark/scalable/minecraft.svg")), version, version);
        }
        m_homeVersionSelector->setEnabled(!versions.isEmpty());
        const int index = m_homeVersionSelector->findData(selectedVersion);
        if (index >= 0) {
            m_homeVersionSelector->setCurrentIndex(index);
        }
    }

    QStringList loaders;
    for (int row = 0; row < instances->count(); ++row) {
        auto* instance = instances->at(row);
        if (installedMinecraftVersion(instance) != selectedVersion) {
            continue;
        }
        const auto loader = installedLoaderLabel(instance);
        if (!loaders.contains(loader)) {
            loaders.append(loader);
        }
    }
    {
        const QSignalBlocker blocker(m_homeLoaderSelector);
        m_homeLoaderSelector->clear();
        for (const auto& loader : loaders) {
            m_homeLoaderSelector->addItem(QIcon(QStringLiteral(":/icons/breeze_dark/scalable/loadermods.svg")), loader, loader);
        }
        m_homeLoaderSelector->setEnabled(!loaders.isEmpty());
        const auto selectedLoader = m_selectedInstance ? installedLoaderLabel(m_selectedInstance) : QString();
        const int index = m_homeLoaderSelector->findData(selectedLoader);
        if (index >= 0) {
            m_homeLoaderSelector->setCurrentIndex(index);
        } else if (!loaders.isEmpty()) {
            m_homeLoaderSelector->setCurrentIndex(0);
        }
    }

    if (auto* cards = findChild<QHBoxLayout*>(QStringLiteral("boostHomeInstanceCards"))) {
        while (auto* item = cards->takeAt(0)) {
            delete item->widget();
            delete item;
        }

        if (instances->count() == 0) {
            auto* empty = new QLabel(tr("No Minecraft installations yet. Create an instance to get started."), this);
            empty->setObjectName(QStringLiteral("boostPanelCaption"));
            cards->addWidget(empty);
            auto* createButton = new QPushButton(tr("Create instance"), this);
            createButton->setObjectName(QStringLiteral("boostSecondaryButton"));
            connect(createButton, &QPushButton::clicked, ui->actionAddInstance, &QAction::trigger);
            cards->addWidget(createButton);
        } else {
            const int previewCount = qMin(instances->count(), 8);
            for (int row = 0; row < previewCount; ++row) {
                auto* instance = instances->at(row);
                const auto id = instance->id();
                const auto sourceIndex = instances->index(row, 0);
                auto icon = instanceTypeIcon(instance);
                if (icon.isNull()) {
                    icon = qvariant_cast<QIcon>(sourceIndex.data(Qt::DecorationRole));
                }
                if (icon.isNull()) {
                    icon = QIcon(QStringLiteral(":/backgrounds/boostclient-grasss"));
                }

                auto* card = new ClickableInstanceFrame(this);
                card->setObjectName(QStringLiteral("boostInstanceCard"));
                card->setMinimumWidth(340);
                card->setMaximumWidth(430);
                card->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
                card->setProperty("selected", instance == m_selectedInstance);
                card->setAccessibleName(tr("%1, Minecraft %2, %3")
                                            .arg(instance->name(), installedMinecraftVersion(instance), installedLoaderLabel(instance)));
                card->setClickHandler([this, id] { setSelectedInstanceById(id); });
                auto* cardLayout = new QHBoxLayout(card);
                cardLayout->setContentsMargins(8, 8, 10, 8);
                cardLayout->setSpacing(10);

                auto* iconLabel = new QLabel(card);
                iconLabel->setObjectName(QStringLiteral("boostInstanceCardIcon"));
                iconLabel->setFixedSize(74, 78);
                iconLabel->setAlignment(Qt::AlignCenter);
                iconLabel->setPixmap(icon.pixmap(64, 64));
                cardLayout->addWidget(iconLabel);
                auto* cardDetails = new QVBoxLayout();
                cardDetails->setContentsMargins(0, 0, 0, 0);
                cardDetails->setSpacing(3);
                auto* cardHeading = new QHBoxLayout();
                auto* name = new QLabel(instance->name(), card);
                name->setObjectName(QStringLiteral("boostInstanceCardName"));
                name->setTextInteractionFlags(Qt::TextSelectableByMouse);
                cardHeading->addWidget(name, 1);

                auto* moreButton = new QToolButton(card);
                moreButton->setText(QStringLiteral("..."));
                moreButton->setAccessibleName(tr("More actions for %1").arg(instance->name()));
                moreButton->setPopupMode(QToolButton::InstantPopup);
                auto* moreMenu = new QMenu(moreButton);
                auto* configureAction = moreMenu->addAction(tr("Configure"));
                auto* duplicateAction = moreMenu->addAction(tr("Duplicate"));
                auto* deleteAction = moreMenu->addAction(tr("Delete"));
                connect(configureAction, &QAction::triggered, this, [this, id] {
                    setSelectedInstanceById(id);
                    ui->actionEditInstance->trigger();
                });
                connect(duplicateAction, &QAction::triggered, this, [this, id] {
                    setSelectedInstanceById(id);
                    ui->actionCopyInstance->trigger();
                });
                connect(deleteAction, &QAction::triggered, this, [this, id] {
                    setSelectedInstanceById(id);
                    ui->actionDeleteInstance->trigger();
                });
                moreButton->setMenu(moreMenu);
                cardHeading->addWidget(moreButton);
                cardDetails->addLayout(cardHeading);

                auto* versionRow = new QHBoxLayout();
                versionRow->setSpacing(7);
                auto* versionLabel = new QLabel(installedMinecraftVersion(instance), card);
                versionLabel->setObjectName(QStringLiteral("boostInstanceCardMeta"));
                versionRow->addWidget(versionLabel);

                const auto modCount = QDir(instance->modsRoot()).entryList({ QStringLiteral("*.jar") }, QDir::Files).size();
                auto* loaderBadge = new QLabel(installedLoaderLabel(instance), card);
                loaderBadge->setObjectName(QStringLiteral("boostInstanceCardBadge"));
                loaderBadge->setAlignment(Qt::AlignCenter);
                loaderBadge->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
                versionRow->addWidget(loaderBadge);
                versionRow->addStretch(1);
                cardDetails->addLayout(versionRow);

                auto* statusLabel = new QLabel(
                    instance->isRunning() ? tr("● Running")
                                          : (instance->canLaunch() ? tr("● Ready to play") : tr("● Needs setup")),
                    card);
                statusLabel->setObjectName(instance->isRunning() || instance->canLaunch()
                                               ? QStringLiteral("boostInstanceCardStatus")
                                               : QStringLiteral("boostInstanceCardMeta"));
                statusLabel->setToolTip(instance->getStatusbarDescription());

                auto* cardActions = new QHBoxLayout();
                auto* launchButton = new QToolButton(card);
                launchButton->setObjectName(QStringLiteral("boostCardLaunchButton"));
                launchButton->setText(instance->isRunning() ? QStringLiteral("■") : QStringLiteral("▶"));
                launchButton->setToolTip(instance->isRunning() ? tr("Stop this instance") : tr("Launch this instance"));
                launchButton->setAccessibleName(
                    instance->isRunning() ? tr("Stop %1").arg(instance->name()) : tr("Launch %1").arg(instance->name()));
                launchButton->setFixedSize(34, 34);
                launchButton->setCursor(Qt::PointingHandCursor);
                launchButton->setEnabled(instance->isRunning() || instance->canLaunch());
                connect(launchButton, &QToolButton::clicked, this, [this, id] {
                    setSelectedInstanceById(id);
                    if (m_selectedInstance && m_selectedInstance->isRunning()) {
                        ui->actionKillInstance->trigger();
                    } else {
                        ui->actionLaunchInstance->trigger();
                    }
                });
                cardActions->addWidget(statusLabel, 1);
                auto* modsLabel = new QLabel(tr("%n mod file(s)", "", modCount), card);
                modsLabel->setObjectName(QStringLiteral("boostInstanceCardMeta"));
                cardActions->addWidget(modsLabel, 0, Qt::AlignRight | Qt::AlignVCenter);
                cardActions->addWidget(launchButton);
                cardDetails->addLayout(cardActions);
                cardLayout->addLayout(cardDetails, 1);
                cards->addWidget(card);
            }
            if (previewCount < instances->count()) {
                auto* more = new QLabel(tr("%n more installation(s) in your library", "", instances->count() - previewCount), this);
                more->setObjectName(QStringLiteral("boostPanelCaption"));
                cards->addWidget(more);
            }
            cards->addStretch(1);
        }
    }

    updateLibraryCards();

    const auto account = APPLICATION->accounts()->defaultAccount();
    if (account) {
        const auto playerName = account->hasProfile() ? account->profileName() : account->displayName();
        m_homeGreeting->setText(tr("Good to see you,"));
        m_homeGreetingSubtitle->setText(playerName);
        const auto face = account->getFace(46, 46);
        m_homeAvatar->setPixmap(face.isNull() ? APPLICATION->logo().pixmap(32, 32) : face);
        m_homeAvatar->setToolTip(account->displayName());
        if (m_homeAccountButton) {
            m_homeAccountButton->setText(account->accountType() == AccountType::MSA ? tr("Microsoft Account") : tr("Offline Account"));
            m_homeAccountButton->setIcon(face.isNull() ? QIcon::fromTheme("accounts") : QIcon(face));
            m_homeAccountButton->setToolTip(account->displayName());
        }
    } else {
        m_homeGreeting->setText(tr("Welcome to %1").arg(BuildConfig.LAUNCHER_DISPLAYNAME));
        m_homeGreetingSubtitle->setText(tr("Ready for your next adventure?"));
        m_homeAvatar->setPixmap(APPLICATION->logo().pixmap(32, 32));
        m_homeAvatar->setToolTip(tr("No Minecraft account selected"));
        if (m_homeAccountButton) {
            m_homeAccountButton->setText(tr("Add Minecraft Account"));
            m_homeAccountButton->setIcon(QIcon::fromTheme("accounts", QApplication::style()->standardIcon(QStyle::SP_DirHomeIcon)));
            m_homeAccountButton->setToolTip(tr("Add or select a Minecraft account"));
        }
    }

    if (m_selectedInstance) {
        m_homeProfileName->setText(tr("Last Played"));
        if (m_selectedInstance->lastLaunch() > 0) {
            const auto lastPlayed = QDateTime::fromMSecsSinceEpoch(m_selectedInstance->lastLaunch());
            const auto playDuration = Time::prettifyDuration(
                m_selectedInstance->lastTimePlayed(),
                APPLICATION->settings()->get("ShowGameTimeWithoutDays").toBool());
            m_homeProfileStatus->setText(tr("%1  ·  %2").arg(QLocale().toString(lastPlayed, "MMM d, yyyy"), playDuration));
        } else {
            m_homeProfileStatus->setText(tr("Not played yet"));
        }
        const bool running = m_selectedInstance->isRunning();
        m_homePlayButton->setText(running ? tr("STOP") : tr("PLAY"));
        m_homePlayButton->setEnabled(running ? ui->actionKillInstance->isEnabled() : ui->actionLaunchInstance->isEnabled());
        m_homeManageButton->setEnabled(m_selectedInstance->canEdit());
    } else {
        m_homeProfileName->setText(tr("NO PLAY HISTORY"));
        m_homeProfileStatus->setText(tr("Create an installation or choose one from your library to get started."));
        m_homePlayButton->setText(tr("PLAY"));
        m_homePlayButton->setEnabled(false);
        m_homeManageButton->setEnabled(false);
    }
}

void MainWindow::updateThemeMenu()
{
    QMenu* themeMenu = ui->actionChangeTheme->menu();

    if (themeMenu) {
        themeMenu->clear();
    } else {
        themeMenu = new QMenu(this);
    }

    auto themes = APPLICATION->themeManager()->getValidApplicationThemes();

    QActionGroup* themesGroup = new QActionGroup(this);

    for (auto* theme : themes) {
        QAction* themeAction = themeMenu->addAction(theme->name());

        themeAction->setCheckable(true);
        if (APPLICATION->settings()->get("ApplicationTheme").toString() == theme->id()) {
            themeAction->setChecked(true);
        }
        themeAction->setActionGroup(themesGroup);

        connect(themeAction, &QAction::triggered, APPLICATION, [id = theme->id()]() {
            APPLICATION->themeManager()->setApplicationTheme(id);
            APPLICATION->settings()->set("ApplicationTheme", id);
        });
    }

    ui->actionChangeTheme->setMenu(themeMenu);
}

void MainWindow::repopulateAccountsMenu()
{
    ui->accountsMenu->clear();

    // NOTE: this is done so the accounts button text is not set to the accounts menu title
    QMenu* accountsButtonMenu = ui->actionAccountsButton->menu();
    if (accountsButtonMenu) {
        accountsButtonMenu->clear();
    } else {
        accountsButtonMenu = new QMenu(this);
        ui->actionAccountsButton->setMenu(accountsButtonMenu);
    }

    auto accounts = APPLICATION->accounts();
    MinecraftAccountPtr defaultAccount = accounts->defaultAccount();

    bool canManageSkins = defaultAccount && !defaultAccount->isActive();
    ui->actionManageSkins->setEnabled(canManageSkins);

    QString active_profileId = "";
    if (defaultAccount) {
        // this can be called before accountMenuButton exists
        if (ui->actionAccountsButton) {
            auto profileLabel = profileInUseFilter(defaultAccount->displayName(), defaultAccount->isInUse());
            ui->actionAccountsButton->setText(profileLabel);
        }
    }

    QActionGroup* accountsGroup = new QActionGroup(this);

    if (accounts->count() <= 0) {
        ui->actionNoAccountsAdded->setEnabled(false);
        ui->accountsMenu->addAction(ui->actionNoAccountsAdded);
    } else {
        // TODO: Nicer way to iterate?
        for (int i = 0; i < accounts->count(); i++) {
            MinecraftAccountPtr account = accounts->at(i);
            auto profileLabel = profileInUseFilter(account->displayName(), account->isInUse());
            QAction* action = new QAction(profileLabel, this);
            action->setData(i);
            action->setCheckable(true);
            action->setActionGroup(accountsGroup);
            if (defaultAccount == account) {
                action->setChecked(true);
            }

            auto face = account->getFace();
            if (!face.isNull()) {
                action->setIcon(face);
            } else {
                action->setIcon(QIcon::fromTheme("noaccount"));
            }

            const int highestNumberKey = 9;
            if (i < highestNumberKey) {
                action->setShortcut(QKeySequence(tr("Ctrl+%1").arg(i + 1)));
            }

            ui->accountsMenu->addAction(action);
            connect(action, &QAction::triggered, this, &MainWindow::changeActiveAccount);
        }
    }

    ui->accountsMenu->addSeparator();

    ui->actionNoDefaultAccount->setData(-1);
    ui->actionNoDefaultAccount->setChecked(!defaultAccount);
    ui->actionNoDefaultAccount->setActionGroup(accountsGroup);

    ui->accountsMenu->addAction(ui->actionNoDefaultAccount);

    connect(ui->actionNoDefaultAccount, &QAction::triggered, this, &MainWindow::changeActiveAccount);

    ui->accountsMenu->addSeparator();
    QAction* addAccountAction = ui->accountsMenu->addAction(tr("Add Account"));
    addAccountAction->setIcon(QIcon::fromTheme("accounts"));
    connect(addAccountAction, &QAction::triggered, this, &MainWindow::on_actionManageAccounts_triggered);

    ui->accountsMenu->addAction(ui->actionManageSkins);
    ui->accountsMenu->addAction(ui->actionManageAccounts);

    accountsButtonMenu->addActions(ui->accountsMenu->actions());
}

void MainWindow::updatesAllowedChanged(bool allowed)
{
    if (!APPLICATION->updaterEnabled()) {
        return;
    }
    ui->actionCheckUpdate->setEnabled(allowed);
}

/*
 * Assumes the sender is a QAction
 */
void MainWindow::changeActiveAccount()
{
    QAction* sAction = (QAction*)sender();

    // Profile's associated Mojang username
    if (sAction->data().typeId() != QMetaType::Int)
        return;

    QVariant action_data = sAction->data();
    bool valid = false;
    int index = action_data.toInt(&valid);
    if (!valid) {
        index = -1;
    }
    auto accounts = APPLICATION->accounts();
    accounts->setDefaultAccount(index == -1 ? nullptr : accounts->at(index));
    defaultAccountChanged();
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
#ifdef Q_OS_WIN
    positionWindowControls();
#endif
    reflowAccountCards();
    if (m_libraryContentStack) {
        updateLibraryCards();
    }
}

#ifdef Q_OS_WIN
void MainWindow::positionWindowControls()
{
    auto* contentColumn = findChild<QWidget*>(QStringLiteral("boostContentColumn"));
    auto* titleBar = findChild<QWidget*>(QStringLiteral("boostTopBar"));
    if (!contentColumn || !titleBar) {
        return;
    }

    titleBar->setGeometry(0, 0, contentColumn->width(), titleBar->height());
    titleBar->raise();
}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
    positionWindowControls();
    const HWND nativeWindow = reinterpret_cast<HWND>(winId());
    SetLastError(0);
    const LONG_PTR windowStyle = GetWindowLongPtr(nativeWindow, GWL_STYLE);
    const DWORD styleError = GetLastError();
    if (windowStyle == 0 && styleError != 0) {
        qWarning() << "Unable to read the native window style:" << styleError;
        return;
    }
    if ((windowStyle & WS_CAPTION) != 0) {
        SetLastError(0);
        const LONG_PTR previousStyle =
            SetWindowLongPtr(nativeWindow, GWL_STYLE, windowStyle & ~static_cast<LONG_PTR>(WS_CAPTION));
        const DWORD setStyleError = GetLastError();
        if (previousStyle == 0 && setStyleError != 0) {
            qWarning() << "Unable to remove the native window caption:" << setStyleError;
            return;
        }
        if (!SetWindowPos(nativeWindow, nullptr, 0, 0, 0, 0,
                          SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED)) {
            qWarning() << "Unable to refresh the borderless window frame:" << GetLastError();
        }
    }
}

bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
    auto* nativeMessage = static_cast<MSG*>(message);
    if (nativeMessage->message == WM_NCHITTEST && !isMaximized() && !isFullScreen()) {
        RECT windowRect;
        const HWND windowHandle = reinterpret_cast<HWND>(winId());
        if (GetWindowRect(windowHandle, &windowRect)) {
            const int x = static_cast<short>(LOWORD(nativeMessage->lParam));
            const int y = static_cast<short>(HIWORD(nativeMessage->lParam));
            const int resizeBorder = qRound(8 * devicePixelRatioF());
            const bool left = x < windowRect.left + resizeBorder;
            const bool right = x >= windowRect.right - resizeBorder;
            const bool top = y < windowRect.top + resizeBorder;
            const bool bottom = y >= windowRect.bottom - resizeBorder;

            if (top && left) {
                *result = HTTOPLEFT;
            } else if (top && right) {
                *result = HTTOPRIGHT;
            } else if (bottom && left) {
                *result = HTBOTTOMLEFT;
            } else if (bottom && right) {
                *result = HTBOTTOMRIGHT;
            } else if (left) {
                *result = HTLEFT;
            } else if (right) {
                *result = HTRIGHT;
            } else if (top) {
                *result = HTTOP;
            } else if (bottom) {
                *result = HTBOTTOM;
            } else {
                return QMainWindow::nativeEvent(eventType, message, result);
            }
            return true;
        }
    }
    return QMainWindow::nativeEvent(eventType, message, result);
}
#endif

void MainWindow::reflowAccountCards()
{
    auto* accountCards = findChild<QWidget*>(QStringLiteral("boostAccountCards"));
    auto* accountGrid = findChild<QGridLayout*>(QStringLiteral("boostAccountRows"));
    if (!accountCards || !accountGrid) {
        return;
    }

    const int availableWidth = accountCards->width();
    constexpr int cardMinimumWidth = 300;
    constexpr int cardSpacing = 16;
    const int columnCount = qBound(1, (availableWidth + cardSpacing) / (cardMinimumWidth + cardSpacing), 3);
    if (accountCards->property("accountGridColumns").toInt() == columnCount && accountGrid->count() > 0) {
        return;
    }

    QList<QWidget*> widgets;
    while (auto* item = accountGrid->takeAt(0)) {
        if (auto* widget = item->widget()) {
            widgets.append(widget);
        }
        delete item;
    }

    for (int column = 0; column < 3; ++column) {
        accountGrid->setColumnStretch(column, column < columnCount ? 1 : 0);
    }

    if (widgets.size() == 1 && widgets.first()->objectName() == QStringLiteral("boostAccountEmptyState")) {
        accountGrid->addWidget(widgets.first(), 0, 0, 1, columnCount);
    } else {
        for (int index = 0; index < widgets.size(); ++index) {
            accountGrid->addWidget(widgets.at(index), index / columnCount, index % columnCount);
        }
    }
    accountCards->setProperty("accountGridColumns", columnCount);
}

void MainWindow::updateAccountCards()
{
    auto* accountGrid = findChild<QGridLayout*>(QStringLiteral("boostAccountRows"));
    auto* accountCards = findChild<QWidget*>(QStringLiteral("boostAccountCards"));
    if (!accountGrid || !accountCards) {
        return;
    }

    while (auto* item = accountGrid->takeAt(0)) {
        if (auto* widget = item->widget()) {
            delete widget;
        }
        delete item;
    }
    accountCards->setProperty("accountGridColumns", -1);

    const auto accounts = APPLICATION->accounts();
    const auto selectedAccount = accounts->defaultAccount();
    if (accounts->count() == 0) {
        auto* empty = new QLabel(tr("No saved accounts yet. Add an account to see your Minecraft profiles here."), accountCards);
        empty->setObjectName(QStringLiteral("boostAccountEmptyState"));
        empty->setWordWrap(true);
        empty->setMinimumHeight(120);
        accountGrid->addWidget(empty, 0, 0);
        reflowAccountCards();
        return;
    }

    for (int index = 0; index < accounts->count(); ++index) {
        const auto savedAccount = accounts->at(index);
        const bool selected = selectedAccount == savedAccount;

        auto* card = new QFrame(accountCards);
        card->setObjectName(QStringLiteral("boostAccountCard"));
        card->setProperty("selected", selected);
        card->setProperty("account", QVariant::fromValue(savedAccount));
        card->setMinimumWidth(300);
        card->setMinimumHeight(208);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        auto* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(14, 12, 14, 12);
        cardLayout->setSpacing(9);

        auto* accountIdentity = new QHBoxLayout();
        accountIdentity->setSpacing(12);
        auto* avatar = new QLabel(card);
        avatar->setObjectName(QStringLiteral("boostAccountAvatar"));
        avatar->setFixedSize(64, 64);
        avatar->setAlignment(Qt::AlignCenter);
        const auto face = savedAccount->getFace(60, 60);
        avatar->setPixmap(face.isNull() ? QIcon::fromTheme("noaccount").pixmap(48, 48) : face);
        accountIdentity->addWidget(avatar, 0, Qt::AlignTop);

        auto* details = new QVBoxLayout();
        details->setSpacing(7);
        auto* name = new QLabel(savedAccount->displayName(), card);
        name->setObjectName(QStringLiteral("boostAccountCardName"));
        name->setWordWrap(true);
        name->setTextInteractionFlags(Qt::TextSelectableByMouse);
        details->addWidget(name);

        auto* status = new QLabel(card);
        status->setObjectName(QStringLiteral("boostAccountStatus"));
        if (savedAccount->isInUse()) {
            status->setText(tr("●  IN GAME"));
        } else if (savedAccount->accountType() == AccountType::Offline) {
            status->setText(tr("●  OFFLINE"));
        } else {
            switch (savedAccount->accountState()) {
                case AccountState::Online:
                    status->setText(tr("●  ONLINE"));
                    break;
                case AccountState::Working:
                    status->setText(tr("●  SIGNING IN"));
                    break;
                case AccountState::Offline:
                    status->setText(tr("●  OFFLINE"));
                    break;
                case AccountState::Unchecked:
                    status->setText(tr("●  MICROSOFT"));
                    break;
                case AccountState::Disabled:
                case AccountState::Errored:
                case AccountState::Expired:
                case AccountState::Gone:
                    status->setText(tr("●  NEEDS ATTENTION"));
                    break;
            }
        }
        details->addWidget(status);
        accountIdentity->addLayout(details, 1);

        auto* selectedBadge = new QLabel(QStringLiteral("✓  %1").arg(tr("Selected")), card);
        selectedBadge->setObjectName(QStringLiteral("boostSelectedAccountBadge"));
        selectedBadge->setVisible(selected);
        selectedBadge->setAlignment(Qt::AlignCenter);
        selectedBadge->setMaximumHeight(24);
        auto* accountPreview = new QVBoxLayout();
        accountPreview->setSpacing(3);
        accountPreview->addWidget(selectedBadge, 0, Qt::AlignRight | Qt::AlignTop);

        auto* skinPreview = new QLabel(card);
        skinPreview->setObjectName(QStringLiteral("boostAccountSkinPreview"));
        skinPreview->setFixedSize(36, 54);
        skinPreview->setAlignment(Qt::AlignCenter);
        const auto& accountSkin = savedAccount->accountData()->minecraftProfile.skin;
        const auto skinModel = accountSkin.variant.compare(QStringLiteral("SLIM"), Qt::CaseInsensitive) == 0
                                   ? SkinModel::SLIM
                                   : SkinModel::CLASSIC;
        const auto skinPreviewImage = SkinModel::previewFromTexture(QImage::fromData(accountSkin.data), skinModel);
        if (!skinPreviewImage.isNull()) {
            const auto frontPreview = skinPreviewImage.copy(0, 0, skinPreviewImage.width() / 2, skinPreviewImage.height());
            skinPreview->setPixmap(QPixmap::fromImage(frontPreview).scaled(skinPreview->size(), Qt::KeepAspectRatio,
                                                                           Qt::FastTransformation));
            skinPreview->setToolTip(tr("Current skin for %1").arg(savedAccount->displayName()));
        } else {
            skinPreview->setText(QStringLiteral("—"));
            skinPreview->setToolTip(tr("No skin preview is available for %1").arg(savedAccount->displayName()));
        }
        accountPreview->addWidget(skinPreview, 0, Qt::AlignHCenter);
        accountIdentity->addLayout(accountPreview, 0);
        cardLayout->addLayout(accountIdentity);

        auto* divider = new QLabel(card);
        divider->setObjectName(QStringLiteral("boostAccountDivider"));
        divider->setFixedHeight(1);
        cardLayout->addWidget(divider);

        auto* actions = new QHBoxLayout();
        actions->setSpacing(7);
        auto* selectButton = new QToolButton(card);
        selectButton->setObjectName(QStringLiteral("boostAccountCardAction"));
        selectButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        selectButton->setIcon(QIcon::fromTheme("media-playback-start", QApplication::style()->standardIcon(QStyle::SP_MediaPlay)));
        selectButton->setIconSize({ 16, 16 });
        selectButton->setText(tr("Select"));
        selectButton->setFixedHeight(38);
        selectButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        selectButton->setProperty("selected", selected);
        selectButton->setEnabled(!selected && !savedAccount->isActive());
        selectButton->setAccessibleName(tr("Select account %1").arg(savedAccount->profileName()));
        connect(selectButton, &QToolButton::clicked, this, [savedAccount] {
            APPLICATION->accounts()->setDefaultAccount(savedAccount);
        });
        actions->addWidget(selectButton, 1);

        auto* editButton = new QToolButton(card);
        editButton->setObjectName(QStringLiteral("boostAccountCardAction"));
        editButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        editButton->setIcon(QIcon::fromTheme("document-edit", QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView)));
        editButton->setIconSize({ 16, 16 });
        editButton->setText(tr("Edit"));
        editButton->setFixedHeight(38);
        editButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        editButton->setToolTip(tr("Open account management to refresh or manage sign-in details."));
        editButton->setAccessibleName(tr("Edit or manage account %1").arg(savedAccount->profileName()));
        connect(editButton, &QToolButton::clicked, this, &MainWindow::on_actionManageAccounts_triggered);
        actions->addWidget(editButton, 1);

        auto* deleteButton = new QToolButton(card);
        deleteButton->setObjectName(QStringLiteral("boostAccountCardAction"));
        deleteButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        deleteButton->setIcon(QIcon::fromTheme("edit-delete", QApplication::style()->standardIcon(QStyle::SP_TrashIcon)));
        deleteButton->setIconSize({ 16, 16 });
        deleteButton->setText(tr("Delete"));
        deleteButton->setFixedHeight(38);
        deleteButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        deleteButton->setEnabled(!savedAccount->isActive());
        deleteButton->setAccessibleName(tr("Delete account %1").arg(savedAccount->profileName()));
        connect(deleteButton, &QToolButton::clicked, this, [this, savedAccount] {
            const auto response = QMessageBox::question(this, tr("Remove account?"),
                                                        tr("Do you really want to delete the account “%1”?")
                                                            .arg(savedAccount->displayName()),
                                                        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (response != QMessageBox::Yes) {
                return;
            }
            const auto accountList = APPLICATION->accounts();
            for (int accountIndex = 0; accountIndex < accountList->count(); ++accountIndex) {
                if (accountList->at(accountIndex) == savedAccount) {
                    accountList->removeAccount(accountList->index(accountIndex, 0));
                    return;
                }
            }
        });
        actions->addWidget(deleteButton, 1);

        auto* skinsButton = new QToolButton(card);
        skinsButton->setObjectName(QStringLiteral("boostAccountCardAction"));
        skinsButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        skinsButton->setIcon(QIcon::fromTheme("preferences-desktop-theme",
                                              QApplication::style()->standardIcon(QStyle::SP_FileDialogInfoView)));
        skinsButton->setIconSize({ 16, 16 });
        skinsButton->setText(tr("Skins"));
        skinsButton->setToolTip(tr("Manage skins for %1").arg(savedAccount->displayName()));
        skinsButton->setFixedHeight(38);
        skinsButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        skinsButton->setEnabled(!savedAccount->isActive());
        skinsButton->setAccessibleName(tr("Manage skins for account %1").arg(savedAccount->profileName()));
        connect(skinsButton, &QToolButton::clicked, this, [this, savedAccount] {
            if (!savedAccount->isActive()) {
                SkinManageDialog dialog(this, savedAccount);
                dialog.exec();
            }
        });
        actions->addWidget(skinsButton, 1);
        cardLayout->addLayout(actions);

        card->installEventFilter(this);
        for (auto* child : card->findChildren<QWidget*>()) {
            if (!qobject_cast<QAbstractButton*>(child)) {
                child->installEventFilter(this);
            }
        }
        accountGrid->addWidget(card, index / 3, index % 3);
    }
    reflowAccountCards();
}

void MainWindow::scheduleAccountCardsUpdate()
{
    if (m_accountCardsUpdatePending) {
        return;
    }
    m_accountCardsUpdatePending = true;
    QTimer::singleShot(0, this, [this] {
        m_accountCardsUpdatePending = false;
        updateAccountCards();
    });
}

void MainWindow::defaultAccountChanged()
{
    repopulateAccountsMenu();

    MinecraftAccountPtr account = APPLICATION->accounts()->defaultAccount();
    auto* profileAvatar = findChild<QLabel*>(QStringLiteral("boostProfileAvatar"));
    auto* profileName = findChild<QLabel*>(QStringLiteral("boostPlayerProfileName"));
    auto* profileDescription = findChild<QLabel*>(QStringLiteral("boostPlayerProfileDescription"));
    auto* accountPageAvatar = findChild<QLabel*>(QStringLiteral("boostAccountProfileAvatar"));
    auto* accountPageProfileName = findChild<QLabel*>(QStringLiteral("boostAccountProfileName"));
    auto* accountPageProfileStatus = findChild<QLabel*>(QStringLiteral("boostAccountProfileStatus"));

    scheduleAccountCardsUpdate();

    if (accountPageAvatar) {
        const auto face = account ? account->getFace(56, 56) : QPixmap();
        accountPageAvatar->setPixmap(face.isNull() ? QIcon::fromTheme("noaccount").pixmap(40, 40) : face);
    }
    if (accountPageProfileName) {
        accountPageProfileName->setText(account ? account->displayName() : tr("No account selected"));
    }
    if (accountPageProfileStatus) {
        if (!account) {
            accountPageProfileStatus->setText(tr("Add a Minecraft account to get started."));
        } else {
            QString status;
            QString statusColor;
            switch (account->accountState()) {
                case AccountState::Online:
                    status = tr("ONLINE");
                    statusColor = QStringLiteral("#39D98A");
                    break;
                case AccountState::Working:
                    status = tr("SIGNING IN");
                    statusColor = QStringLiteral("#F28A1D");
                    break;
                case AccountState::Offline:
                    status = tr("OFFLINE");
                    statusColor = QStringLiteral("#A8A8A8");
                    break;
                case AccountState::Unchecked:
                    status = tr("MICROSOFT");
                    statusColor = QStringLiteral("#F28A1D");
                    break;
                case AccountState::Disabled:
                case AccountState::Errored:
                case AccountState::Expired:
                case AccountState::Gone:
                    status = tr("NEEDS ATTENTION");
                    statusColor = QStringLiteral("#E36B5D");
                    break;
            }
            if (account->isInUse()) {
                status = tr("IN GAME");
                statusColor = QStringLiteral("#39D98A");
            }
            const auto accountType =
                account->accountType() == AccountType::Offline ? tr("Offline account") : tr("Microsoft account");
            const auto profile = account->hasProfile() ? account->profileName() : tr("No Minecraft profile");
            accountPageProfileStatus->setText(
                QStringLiteral("<span style=\"color:%1\">● %2</span> <span style=\"color:#A8A8A8\">· %3 · %4</span>")
                    .arg(statusColor, status.toHtmlEscaped(), accountType.toHtmlEscaped(), profile.toHtmlEscaped()));
        }
    }

    if (profileAvatar) {
        profileAvatar->clear();
        profileAvatar->setPixmap(account ? account->getFace(56, 56) : QPixmap());
        if (!account || account->getFace(56, 56).isNull()) {
            profileAvatar->setPixmap(QIcon::fromTheme("noaccount").pixmap(40, 40));
        }
    }
    if (profileName) {
        profileName->setText(account ? account->displayName() : tr("No account selected"));
    }
    if (profileDescription) {
        if (account) {
            const auto accountType = account->typeString().toUpper();
            const auto statusColor =
                accountType.compare(QStringLiteral("OFFLINE"), Qt::CaseInsensitive) == 0 ? QStringLiteral("#A8A8A8")
                                                                                         : QStringLiteral("#2CE88A");
            profileDescription->setText(
                QStringLiteral("<span style=\"color:%1\">● %2</span> <span style=\"color:#A8A8A8\">%3 · %4</span>")
                    .arg(statusColor, accountType.toHtmlEscaped(), tr("account").toHtmlEscaped(),
                         (account->hasProfile() ? account->profileName() : tr("No Minecraft profile")).toHtmlEscaped()));
        } else {
            profileDescription->setText(tr("Add a Minecraft account to display your player profile.").toHtmlEscaped());
        }
    }
    updateHomeDashboard();

    if (account) {
        auto profileLabel = profileInUseFilter(account->displayName(), account->isInUse());
        ui->actionAccountsButton->setText(profileLabel);
        auto face = account->getFace();
        if (face.isNull()) {
            ui->actionAccountsButton->setIcon(QIcon::fromTheme("noaccount"));
        } else {
            ui->actionAccountsButton->setIcon(face);
        }
        return;
    }

    // Set the icon to the "no account" icon.
    ui->actionAccountsButton->setIcon(QIcon::fromTheme("noaccount"));
    ui->actionAccountsButton->setText(tr("Accounts"));
}

bool MainWindow::eventFilter(QObject* obj, QEvent* ev)
{
#ifdef Q_OS_WIN
    if (obj == findChild<QWidget*>(QStringLiteral("boostContentColumn")) && ev->type() == QEvent::Resize) {
        positionWindowControls();
    }
#endif
    if (obj == findChild<QWidget*>(QStringLiteral("boostAccountCards")) && ev->type() == QEvent::Resize) {
        reflowAccountCards();
    } else if (ev->type() == QEvent::MouseButtonRelease) {
        auto* clickedWidget = qobject_cast<QWidget*>(obj);
        auto* card = clickedWidget;
        while (card && card->objectName() != QStringLiteral("boostAccountCard")) {
            card = card->parentWidget();
        }
        auto* mouseEvent = static_cast<QMouseEvent*>(ev);
        if (card && clickedWidget && !qobject_cast<QAbstractButton*>(clickedWidget) && mouseEvent->button() == Qt::LeftButton) {
            const auto account = card->property("account").value<MinecraftAccountPtr>();
            if (account && !account->isActive()) {
                APPLICATION->accounts()->setDefaultAccount(account);
            }
        }
    } else if (obj == view) {
        if (ev->type() == QEvent::KeyPress) {
            secretEventFilter->input(ev);
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(ev);
            switch (keyEvent->key()) {
                    /*
                case Qt::Key_Enter:
                case Qt::Key_Return:
                    activateInstance(m_selectedInstance);
                    return true;
                    */
                case Qt::Key_Delete:
                    on_actionDeleteInstance_triggered();
                    return true;
                case Qt::Key_F5:
                    refreshInstances();
                    return true;
                case Qt::Key_F2:
                    on_actionRenameInstance_triggered();
                    return true;
                default:
                    break;
            }
        }
    } else if (obj == nameLabel && ev->type() == QEvent::Resize) {
        updateNameLabel();
    }
    return QMainWindow::eventFilter(obj, ev);
}

void MainWindow::updateNewsLabel()
{
    if (m_newsChecker->isLoadingNews()) {
        newsLabel->setText(tr("Loading news..."));
        newsLabel->setEnabled(false);
        ui->actionMoreNews->setVisible(false);
    } else {
        QList<NewsEntryPtr> entries = m_newsChecker->getNewsEntries();
        if (entries.length() > 0) {
            newsLabel->setText(entries[0]->title);
            newsLabel->setEnabled(true);
            ui->actionMoreNews->setVisible(true);
        } else {
            newsLabel->setText(tr("No news available."));
            newsLabel->setEnabled(false);
            ui->actionMoreNews->setVisible(false);
        }
    }
}

QList<int> stringToIntList(const QString& string)
{
    QStringList split = string.split(',', Qt::SkipEmptyParts);
    QList<int> out;
    for (int i = 0; i < split.size(); ++i) {
        out.append(split.at(i).toInt());
    }
    return out;
}
QString intListToString(const QList<int>& list)
{
    QStringList slist;
    for (int i = 0; i < list.size(); ++i) {
        slist.append(QString::number(list.at(i)));
    }
    return slist.join(',');
}

void MainWindow::onCatToggled(bool state)
{
    setCatBackground(state);
    APPLICATION->settings()->set("TheCat", state);
}

void MainWindow::setCatBackground(bool enabled)
{
    view->setPaintCat(enabled);
    view->viewport()->repaint();
}

void MainWindow::updateCatState()
{
    SettingsObject* settings = APPLICATION->settings();
    const bool catEnabled = settings->get("EnableCat").toBool();
    bool catVisible = settings->get("TheCat").toBool();
    if (!catEnabled && catVisible) {
        settings->set("TheCat", false);
        catVisible = false;
    }

    ui->actionCAT->setVisible(catEnabled);
    ui->actionCAT->setChecked(catVisible);
    setCatBackground(catVisible);
}

void MainWindow::runModalTask(Task* task)
{
    connect(task, &Task::failed, this,
            [this](QString reason) { CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show(); });
    connect(task, &Task::succeeded, this, [this, task]() {
        QStringList warnings = task->warnings();
        if (warnings.count()) {
            CustomMessageBox::selectable(this, tr("Warnings"), warnings.join('\n'), QMessageBox::Warning)->show();
        }
    });
    ProgressDialog loadDialog(this);
    loadDialog.showSkipButton();
    loadDialog.execWithTask(task);
}

void MainWindow::instanceFromInstanceTask(InstanceTask* rawTask)
{
    unique_qobject_ptr<Task> task(APPLICATION->instances()->wrapInstanceTask(rawTask));
    runModalTask(task.get());
}

void MainWindow::on_actionCopyInstance_triggered()
{
    if (!m_selectedInstance)
        return;

    CopyInstanceDialog copyInstDlg(m_selectedInstance, this);
    if (!copyInstDlg.exec())
        return;

    auto copyTask = new InstanceCopyTask(m_selectedInstance, copyInstDlg.getChosenOptions());
    copyTask->setName(copyInstDlg.instName());
    copyTask->setGroup(copyInstDlg.instGroup());
    copyTask->setIcon(copyInstDlg.iconKey());
    unique_qobject_ptr<Task> task(APPLICATION->instances()->wrapInstanceTask(copyTask));
    runModalTask(task.get());
}

void MainWindow::addInstance(const QString& url, const QMap<QString, QString>& extra_info)
{
    QString groupName;
    do {
        QObject* obj = sender();
        if (!obj)
            break;
        QAction* action = qobject_cast<QAction*>(obj);
        if (!action)
            break;
        auto map = action->data().toMap();
        if (!map.contains("group"))
            break;
        groupName = map["group"].toString();
    } while (0);

    if (groupName.isEmpty()) {
        groupName = APPLICATION->settings()->get("LastUsedGroupForNewInstance").toString();
    }

    NewInstanceDialog newInstDlg(groupName, url, extra_info, this);
    if (!newInstDlg.exec())
        return;

    APPLICATION->settings()->set("LastUsedGroupForNewInstance", newInstDlg.instGroup());
    APPLICATION->settings()->set("LastUsedInstDirForNewInstance", newInstDlg.instDir());

    InstanceTask* creationTask = newInstDlg.extractTask();
    if (creationTask) {
        instanceFromInstanceTask(creationTask);
    }
}

void MainWindow::on_actionAddInstance_triggered()
{
    addInstance();
}

void MainWindow::processURLs(QList<QUrl> urls)
{
    // NOTE: This loop only processes one dropped file!
    for (auto& url : urls) {
        if (url.isEmpty() || url.toString().trimmed().isEmpty())
            continue;

        qDebug() << "Processing" << url;

        // The isLocalFile() check below doesn't work as intended without an explicit scheme.
        if (url.scheme().isEmpty())
            url.setScheme("file");

        ModPlatform::IndexedVersion version;
        QMap<QString, QString> extra_info;
        QUrl local_url;
        if (!url.isLocalFile()) {  // download the remote resource and identify
            if (url.scheme().compare("modrinth", Qt::CaseInsensitive) == 0) {
                const auto packId = ModrinthAPI::getModpackIdFromUrl(url);
                if (!packId.isEmpty()) {
                    extra_info.insert("pack_id", packId);
                    addInstance(url.toString(), extra_info);
                } else {
                    CustomMessageBox::selectable(
                        this, tr("Error"),
                        tr("Unsupported Modrinth link.\n\nBoostClient currently only supports modpack links such as "
                           "modrinth://modpack/fabulously-optimized."),
                        QMessageBox::Critical)
                        ->show();
                }
                continue;
            }

            const bool isExternalURLImport = (url.host().toLower() == "import") || (url.path().startsWith("/import", Qt::CaseInsensitive));

            QUrl dl_url;
            if (url.scheme() == "curseforge" || (url.scheme() == BuildConfig.LAUNCHER_APP_BINARY_NAME && url.host() == "install")) {
                // need to find the download link for the modpack / resource
                // format of url curseforge://install?addonId=IDHERE&fileId=IDHERE
                // format of url binaryname://install?platform=curseforge&addonId=IDHERE&fileId=IDHERE
                QUrlQuery query(url);

                // check if this is a binaryname:// url
                if (url.scheme() == BuildConfig.LAUNCHER_APP_BINARY_NAME) {
                    // check this is an curseforge platform request
                    if (query.queryItemValue("platform").toLower() != "curseforge") {
                        qDebug() << "Invalid mod distribution platform:" << query.queryItemValue("platform");
                        continue;
                    }
                }

                if (query.allQueryItemValues("addonId").isEmpty() || query.allQueryItemValues("fileId").isEmpty()) {
                    qDebug() << "Invalid curseforge link:" << url;
                    continue;
                }

                auto addonId = query.allQueryItemValues("addonId")[0];
                auto fileId = query.allQueryItemValues("fileId")[0];

                extra_info.insert("pack_id", addonId);
                extra_info.insert("pack_version_id", fileId);

                auto [job, array] = FlameAPI::getFile(addonId, fileId);

                connect(job.get(), &Task::failed, this, [this](const QString& reason) {
                    CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show();
                });
                connect(job.get(), &Task::succeeded, this, [this, array, addonId, fileId, &dl_url, &version] {
                    qDebug() << "Returned CFURL Json:\n" << array->toStdString().c_str();
                    auto doc = Json::requireDocument(*array);
                    if (!doc) {
                        CustomMessageBox::selectable(this, tr("Error"), doc.error(), QMessageBox::Critical)->show();
                        return;
                    }
                    auto data = doc->object()["data"].toObject();
                    // No way to find out if it's a mod or a modpack before here
                    // And also we need to check if it ends with .zip, instead of any better way
                    auto versionRes = FlameMod::loadIndexedPackVersion(data);
                    if (!versionRes) {
                        CustomMessageBox::selectable(this, tr("Error"), versionRes.error(), QMessageBox::Critical)->show();
                        return;
                    }
                    version = versionRes.value();
                    auto fileName = version.fileName;

                    // Have to use ensureString then use QUrl to get proper url encoding
                    dl_url = QUrl(version.downloadUrl);
                    if (!dl_url.isValid()) {
                        CustomMessageBox::selectable(
                            this, tr("Error"),
                            tr("The modpack, mod, or resource %1 is blocked for third-parties! Please download it manually.").arg(fileName),
                            QMessageBox::Critical)
                            ->show();
                        return;
                    }
                });

                {  // drop stack
                    ProgressDialog dlUrlDialod(this);
                    dlUrlDialod.showSkipButton();
                    dlUrlDialod.execWithTask(job.get());
                }

            } else if (url.scheme() == BuildConfig.LAUNCHER_APP_BINARY_NAME && !isExternalURLImport) {
                QVariantMap receivedData;
                const QUrlQuery query(url.query());
                const auto items = query.queryItems();
                for (auto it = items.begin(), end = items.end(); it != end; ++it)
                    receivedData.insert(it->first, it->second);
                emit APPLICATION->oauthReplyRecieved(receivedData);
                continue;
            } else if ((url.scheme() == "prismlauncher" || url.scheme() == BuildConfig.LAUNCHER_APP_BINARY_NAME) && isExternalURLImport) {
                // PrismLauncher URL protocol modpack import
                // works for any prism fork
                // preferred import format: prismlauncher://import?url=ENCODED
                const auto host = url.host().toLower();
                const auto path = url.path();

                QString encodedTarget;

                {
                    QUrlQuery query(url);
                    const auto values = query.allQueryItemValues("url");
                    if (!values.isEmpty()) {
                        encodedTarget = values.first();
                    }
                }

                // alternative import format: prismlauncher://import/ENCODED
                if (encodedTarget.isEmpty()) {
                    QString p = path;

                    if (p.startsWith("/import/", Qt::CaseInsensitive)) {
                        p = p.mid(QString("/import/").size());
                    } else if (host == "import" && p.startsWith("/")) {
                        p = p.mid(1);
                    }

                    if (!p.isEmpty() && p != "/import") {
                        encodedTarget = p;
                    }
                }

                if (encodedTarget.isEmpty()) {
                    CustomMessageBox::selectable(this, tr("Error"), tr("Invalid import link: missing 'url' parameter."),
                                                 QMessageBox::Critical)
                        ->show();
                    continue;
                }

                const QString decodedStr = QUrl::fromPercentEncoding(encodedTarget.toUtf8()).trimmed();

                QUrl target = QUrl::fromUserInput(decodedStr);

                // Validate: only allow http(s)
                if (!target.isValid() || (target.scheme() != "https" && target.scheme() != "http")) {
                    CustomMessageBox::selectable(this, tr("Error"), tr("Invalid import link: URL must be http(s)."), QMessageBox::Critical)
                        ->show();
                    continue;
                }

                const auto res = QMessageBox::question(
                    this, tr("Install modpack"),
                    tr("Do you want to download and import a modpack from:\n%1\n\nURL:\n%2").arg(target.host(), target.toString()),
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
                if (res != QMessageBox::Yes) {
                    continue;
                }

                dl_url = target;
            } else {
                dl_url = url;
            }

            if (!dl_url.isValid()) {
                continue;  // no valid url to download this resource
            }

            const QString path = dl_url.host() + '/' + dl_url.path();
            auto entry = APPLICATION->metacache()->resolveEntry("general", path);
            entry->setStale(true);
            auto dl_job = unique_qobject_ptr<NetJob>(new NetJob(tr("Modpack download"), APPLICATION->network()));
            dl_job->addNetAction(Net::ApiRequest::makeCached(dl_url, entry));
            auto archivePath = entry->getFullPath();

            bool dl_success = false;
            connect(dl_job.get(), &Task::failed, this,
                    [this](QString reason) { CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show(); });
            connect(dl_job.get(), &Task::succeeded, this, [&dl_success] { dl_success = true; });

            {  // drop stack
                ProgressDialog dlUrlDialod(this);
                dlUrlDialod.showSkipButton();
                dlUrlDialod.execWithTask(dl_job.get());
            }

            if (!dl_success) {
                continue;  // no local file to identify
            }
            local_url = QUrl::fromLocalFile(archivePath);

        } else {
            local_url = url;
        }

        auto localFileName = QDir::toNativeSeparators(local_url.toLocalFile());
        QFileInfo localFileInfo(localFileName);

        if (localFileName.isEmpty() || !localFileInfo.exists()) {
            qDebug() << "Ignoring invalid path" << localFileName;
            continue;
        }

        auto type = ResourceUtils::identify(localFileInfo);

        if (ModPlatform::ResourceTypeUtils::g_VALID_RESOURCES.count(type) == 0) {  // probably instance/modpack
            addInstance(localFileName, extra_info);
            continue;
        }

        if (APPLICATION->instances()->count() <= 0) {
            CustomMessageBox::selectable(this, tr("No instance!"),
                                         tr("No instance available to add the resource to.\nPlease create a new instance before "
                                            "attempting to install this resource again."),
                                         QMessageBox::Critical)
                ->show();
            continue;
        }
        ImportResourceDialog dlg(localFileName, type, this);

        if (dlg.exec() != QDialog::Accepted)
            continue;

        qDebug() << "Adding resource" << localFileName << "to" << dlg.selectedInstanceKey;

        auto inst = APPLICATION->instances()->getInstanceById(dlg.selectedInstanceKey);
        auto minecraftInst = inst;

        switch (type) {
            case ModPlatform::ResourceType::ResourcePack:
                minecraftInst->resourcePackList()->installResourceWithFlameMetadata(localFileName, version);
                break;
            case ModPlatform::ResourceType::TexturePack:
                minecraftInst->texturePackList()->installResourceWithFlameMetadata(localFileName, version);
                break;
            case ModPlatform::ResourceType::DataPack:
                qWarning() << "Importing of Data Packs not supported at this time. Ignoring" << localFileName;
                break;
            case ModPlatform::ResourceType::Mod:
                minecraftInst->loaderModList()->installResourceWithFlameMetadata(localFileName, version);
                break;
            case ModPlatform::ResourceType::ShaderPack:
                minecraftInst->shaderPackList()->installResourceWithFlameMetadata(localFileName, version);
                break;
            case ModPlatform::ResourceType::World:
                minecraftInst->worldList()->installWorld(localFileInfo);
                break;
            case ModPlatform::ResourceType::Unknown:
            default:
                qDebug() << "Can't Identify" << localFileName << "Ignoring it.";
                break;
        }
    }
}

void MainWindow::on_actionREDDIT_triggered()
{
    DesktopServices::openUrl(QUrl(BuildConfig.SUBREDDIT_URL));
}

void MainWindow::on_actionDISCORD_triggered()
{
    DesktopServices::openUrl(QUrl(BuildConfig.DISCORD_URL));
}

void MainWindow::on_actionMATRIX_triggered()
{
    DesktopServices::openUrl(QUrl(BuildConfig.MATRIX_URL));
}

void MainWindow::on_actionChangeInstIcon_triggered()
{
    if (!m_selectedInstance)
        return;

    IconPickerDialog dlg(this);
    dlg.execWithSelection(m_selectedInstance->iconKey());
    if (dlg.result() == QDialog::Accepted) {
        m_selectedInstance->setIconKey(dlg.selectedIconKey);
        auto icon = APPLICATION->icons()->getIcon(dlg.selectedIconKey);
        ui->actionChangeInstIcon->setIcon(icon);
        changeIconButton->setIcon(icon);
    }
}

void MainWindow::iconUpdated(QString icon)
{
    if (icon == m_currentInstIcon) {
        updateInstanceToolIcon(icon);
    }
}

void MainWindow::updateInstanceToolIcon(QString new_icon)
{
    m_currentInstIcon = new_icon;
    auto icon = APPLICATION->icons()->getIcon(m_currentInstIcon);
    ui->actionChangeInstIcon->setIcon(icon);
    changeIconButton->setIcon(icon);
}

void MainWindow::setSelectedInstanceById(const QString& id)
{
    if (id.isNull())
        return;
    const QModelIndex index = APPLICATION->instances()->getInstanceIndexById(id);
    if (index.isValid()) {
        QModelIndex selectionIndex = proxymodel->mapFromSource(index);
        view->selectionModel()->setCurrentIndex(selectionIndex, QItemSelectionModel::ClearAndSelect);
        updateStatusCenter();
    }
}

void MainWindow::on_actionChangeInstGroup_triggered()
{
    if (!m_selectedInstance)
        return;

    InstanceId instId = m_selectedInstance->id();
    QString src(APPLICATION->instances()->getInstanceGroup(instId));

    QStringList groups = APPLICATION->instances()->getGroups();
    groups.prepend("");
    int index = groups.indexOf(src);
    bool ok = false;
    QString dst = QInputDialog::getItem(this, tr("Group name"), tr("Enter a new group name."), groups, index, true, &ok);
    dst = dst.simplified();

    if (ok) {
        APPLICATION->instances()->setInstanceGroup(instId, dst);
    }
}

void MainWindow::deleteGroup(QString group)
{
    Q_ASSERT(!group.isEmpty());

    const int reply = QMessageBox::question(this, tr("Delete group"), tr("Are you sure you want to delete the group '%1'?").arg(group),
                                            QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes)
        APPLICATION->instances()->deleteGroup(group);
}

void MainWindow::renameGroup(QString group)
{
    Q_ASSERT(!group.isEmpty());

    QString name = QInputDialog::getText(this, tr("Rename group"), tr("Enter a new group name."), QLineEdit::Normal, group);
    name = name.simplified();
    if (name.isNull() || name == group)
        return;

    const bool empty = name.isEmpty();
    const bool duplicate = APPLICATION->instances()->getGroups().contains(name, Qt::CaseInsensitive) && group.toLower() != name.toLower();

    if (empty || duplicate) {
        QMessageBox::warning(this, tr("Cannot rename group"), empty ? tr("Cannot set empty name.") : tr("Group already exists. :/"));
        return;
    }

    APPLICATION->instances()->renameGroup(group, name);
}

void MainWindow::undoTrashInstance()
{
    if (!APPLICATION->instances()->undoTrashInstance())
        QMessageBox::warning(
            this, tr("Failed to undo trashing instance"),
            tr("Some instances and shortcuts could not be restored.\nPlease check your trashbin to manually restore them."));
    ui->actionUndoTrashInstance->setEnabled(APPLICATION->instances()->trashedSomething());
}

void MainWindow::on_actionViewLauncherRootFolder_triggered()
{
    DesktopServices::openPath(".");
}

void MainWindow::on_actionViewInstanceFolder_triggered()
{
    QString str = APPLICATION->settings()->get("InstanceDir").toString();
    DesktopServices::openPath(str);
}

void MainWindow::on_actionViewCentralModsFolder_triggered()
{
    DesktopServices::openPath(APPLICATION->settings()->get("CentralModsDir").toString(), true);
}

void MainWindow::on_actionViewSkinsFolder_triggered()
{
    DesktopServices::openPath(APPLICATION->settings()->get("SkinsDir").toString(), true);
}

void MainWindow::on_actionViewIconThemeFolder_triggered()
{
    DesktopServices::openPath(APPLICATION->themeManager()->getIconThemesFolder().path(), true);
}

void MainWindow::on_actionViewWidgetThemeFolder_triggered()
{
    DesktopServices::openPath(APPLICATION->themeManager()->getApplicationThemesFolder().path(), true);
}

void MainWindow::on_actionViewCatPackFolder_triggered()
{
    DesktopServices::openPath(APPLICATION->themeManager()->getCatPacksFolder().path(), true);
}

void MainWindow::on_actionViewIconsFolder_triggered()
{
    DesktopServices::openPath(APPLICATION->icons()->getDirectory(), true);
}

void MainWindow::on_actionViewLogsFolder_triggered()
{
    DesktopServices::openPath("logs", true);
}

void MainWindow::on_actionViewJavaFolder_triggered()
{
    DesktopServices::openPath(APPLICATION->javaPath(), true);
}

void MainWindow::refreshInstances()
{
    APPLICATION->instances()->loadList();
}

void MainWindow::checkForUpdates()
{
    if (APPLICATION->updaterEnabled()) {
        APPLICATION->triggerUpdateCheck();
    } else {
        qWarning() << "Updater not set up. Cannot check for updates.";
    }
}

void MainWindow::on_actionSettings_triggered()
{
    APPLICATION->ShowGlobalSettings(this, "global-settings");
}

void MainWindow::globalSettingsClosed()
{
    proxymodel->invalidate();
    proxymodel->sort(0);
    updateMainToolBar();
    updateLaunchButton();
    updateThemeMenu();
    updateStatusCenter();
    updateCatState();
    // This needs to be done to prevent UI elements disappearing in the event the config is changed
    // but Prism Launcher exits abnormally, causing the window state to never be saved:
    APPLICATION->settings()->set("MainWindowState", QString::fromUtf8(saveState().toBase64()));
    update();
}

void MainWindow::on_actionEditInstance_triggered()
{
    if (!m_selectedInstance)
        return;

    if (m_selectedInstance->canEdit()) {
        APPLICATION->showInstanceWindow(m_selectedInstance);
    } else {
        CustomMessageBox::selectable(this, tr("Instance not editable"),
                                     tr("This instance is not editable. It may be broken, invalid, or too old. Check logs for details."),
                                     QMessageBox::Critical)
            ->show();
    }
}

void MainWindow::on_actionManageSkins_triggered()
{
    auto account = APPLICATION->accounts()->defaultAccount();

    if (account && !account->isActive()) {
        SkinManageDialog dialog(this, account);
        dialog.exec();
    }
}

void MainWindow::on_actionManageAccounts_triggered()
{
    APPLICATION->ShowGlobalSettings(this, "accounts");
}

void MainWindow::on_actionReportBug_triggered()
{
    DesktopServices::openUrl(QUrl(BuildConfig.BUG_TRACKER_URL));
}

void MainWindow::on_actionClearMetadata_triggered()
{
    // This if contains side effects!
    if (!APPLICATION->metacache()->evictAll()) {
        CustomMessageBox::selectable(this, tr("Error"),
                                     tr("Metadata cache clear Failed!\nTo clear the metadata cache manually, press Folders -> View "
                                        "Launcher Root Folder, and after closing the launcher delete the folder named \"meta\"\n"),
                                     QMessageBox::Warning)
            ->show();
    }

    APPLICATION->metacache()->SaveNow();
}

#ifdef Q_OS_MAC
void MainWindow::on_actionAddToPATH_triggered()
{
    auto binaryPath = APPLICATION->applicationFilePath();
    auto targetPath = QString("/usr/local/bin/%1").arg(BuildConfig.LAUNCHER_APP_BINARY_NAME);
    qDebug() << "Symlinking" << binaryPath << "to" << targetPath;

    QStringList args;
    args << "-e";
    args << QString("do shell script \"mkdir -p /usr/local/bin && ln -sf '%1' '%2'\" with administrator privileges")
                .arg(binaryPath, targetPath);
    auto outcome = QProcess::execute("/usr/bin/osascript", args);
    if (!outcome) {
        QMessageBox::information(this, tr("Successfully added %1 to PATH").arg(BuildConfig.LAUNCHER_DISPLAYNAME),
                                 tr("%1 was successfully added to your PATH. You can now start it by running `%2`.")
                                     .arg(BuildConfig.LAUNCHER_DISPLAYNAME, BuildConfig.LAUNCHER_APP_BINARY_NAME));
    } else {
        QMessageBox::critical(this, tr("Failed to add %1 to PATH").arg(BuildConfig.LAUNCHER_DISPLAYNAME),
                              tr("An error occurred while trying to add %1 to PATH").arg(BuildConfig.LAUNCHER_DISPLAYNAME));
    }
}
#endif

void MainWindow::on_actionOpenWiki_triggered()
{
    DesktopServices::openUrl(QUrl(BuildConfig.WIKI_URL));
}

void MainWindow::on_actionMoreNews_triggered()
{
    auto entries = m_newsChecker->getNewsEntries();
    NewsDialog news_dialog(entries, this);
    news_dialog.exec();
}

void MainWindow::newsButtonClicked()
{
    auto entries = m_newsChecker->getNewsEntries();
    NewsDialog news_dialog(entries, this);
    news_dialog.toggleArticleList();
    news_dialog.exec();
}

void MainWindow::onCatChanged(int)
{
    setCatBackground(APPLICATION->settings()->get("TheCat").toBool());
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::on_actionDeleteInstance_triggered()
{
    if (!m_selectedInstance) {
        return;
    }

    if (m_selectedInstance->isRunning()) {
        CustomMessageBox::selectable(this, tr("Cannot Delete Running Instance"),
                                     tr("The selected instance is currently running and cannot be deleted. Please stop the instance before "
                                        "attempting to delete it."),
                                     QMessageBox::Warning, QMessageBox::Ok)
            ->exec();
        return;
    }
    auto id = m_selectedInstance->id();

    QString shortcutStr;
    auto shortcuts = m_selectedInstance->shortcuts();
    if (!shortcuts.isEmpty())
        shortcutStr = tr(" and its %n registered shortcut(s)", "", shortcuts.size());
    auto response = CustomMessageBox::selectable(this, tr("Confirm Deletion"),
                                                 tr("You are about to delete \"%1\"%2.\n"
                                                    "This may be permanent and will completely delete the instance.\n\n"
                                                    "Are you sure?")
                                                     .arg(m_selectedInstance->name(), shortcutStr),
                                                 QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                        ->exec();

    if (response != QMessageBox::Yes)
        return;

    if (!checkLinkedInstances(id, this, tr("Deleting")))
        return;

    if (APPLICATION->instances()->trashInstance(id)) {
        ui->actionUndoTrashInstance->setEnabled(APPLICATION->instances()->trashedSomething());
    } else {
        APPLICATION->instances()->deleteInstance(id);
    }
    APPLICATION->settings()->set("SelectedInstance", QString());
    selectionBad();
}

void MainWindow::on_actionExportInstanceZip_triggered()
{
    if (m_selectedInstance) {
        ExportInstanceDialog dlg(m_selectedInstance, this);
        dlg.exec();
    }
}

void MainWindow::on_actionExportInstanceMrPack_triggered()
{
    if (m_selectedInstance) {
        ExportPackDialog dlg(m_selectedInstance, this);
        dlg.exec();
    }
}

void MainWindow::on_actionExportInstanceFlamePack_triggered()
{
    if (m_selectedInstance) {
        if (auto cmp = m_selectedInstance->getPackProfile()->getComponent("net.minecraft");
            cmp && cmp->getVersionFile() && cmp->getVersionFile()->type == "snapshot") {
            QMessageBox msgBox(this);
            msgBox.setText("Snapshots are currently not supported by CurseForge modpacks.");
            msgBox.exec();
            return;
        }
        ExportPackDialog dlg(m_selectedInstance, this, ModPlatform::ResourceProvider::FLAME);
        dlg.exec();
    }
}

void MainWindow::on_actionRenameInstance_triggered()
{
    if (m_selectedInstance) {
        view->edit(view->currentIndex());
    }
}

void MainWindow::on_actionViewSelectedInstFolder_triggered()
{
    if (m_selectedInstance) {
        QString str = m_selectedInstance->instanceRoot();
        DesktopServices::openPath(QFileInfo(str));
    }
}

void MainWindow::trayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        toggleWindowVisibility();
    }
}

void MainWindow::toggleWindowVisibility()
{
    if (isVisible()) {
        hide();
    } else {
        show();
        raise();
        activateWindow();
    }
}

void MainWindow::quitFromTray()
{
    QApplication::quit();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // Save the window state and geometry.
    APPLICATION->settings()->set("MainWindowState", QString::fromUtf8(saveState().toBase64()));
    APPLICATION->settings()->set("MainWindowGeometry", QString::fromUtf8(saveGeometry().toBase64()));
    hide();
    event->ignore();
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
#ifdef Q_OS_WIN
    if (event->type() == QEvent::WindowStateChange && m_windowMaximizeButton) {
        const bool maximized = isMaximized();
        m_windowMaximizeButton->setText(maximized ? QStringLiteral("❐") : QStringLiteral("□"));
        m_windowMaximizeButton->setToolTip(maximized ? tr("Restore") : tr("Maximize"));
        m_windowMaximizeButton->setAccessibleName(maximized ? tr("Restore") : tr("Maximize"));
    }
#endif
    QMainWindow::changeEvent(event);
}

void MainWindow::instanceActivated(QModelIndex index)
{
    if (!index.isValid())
        return;
    QString id = index.data(InstanceList::InstanceIDRole).toString();
    MinecraftInstance* inst = APPLICATION->instances()->getInstanceById(id);
    if (!inst)
        return;

    if (APPLICATION->settings()->get("EditInstanceOnDoubleClick").toBool()) {
        if (inst->canEdit()) {
            APPLICATION->showInstanceWindow(inst);
        } else {
            CustomMessageBox::selectable(
                this, tr("Instance not editable"),
                tr("This instance is not editable. It may be broken, invalid, or too old. Check logs for details."), QMessageBox::Critical)
                ->show();
        }
        return;
    }
    APPLICATION->launch(inst);
}

void MainWindow::on_actionLaunchInstance_triggered()
{
    if (m_selectedInstance && !m_selectedInstance->isRunning()) {
        APPLICATION->launch(m_selectedInstance);
    }
}

void MainWindow::on_actionKillInstance_triggered()
{
    if (m_selectedInstance && m_selectedInstance->isRunning()) {
        APPLICATION->launch(m_selectedInstance);
    }
}

void MainWindow::on_actionCreateInstanceShortcut_triggered()
{
    if (!m_selectedInstance)
        return;

    CreateShortcutDialog shortcutDlg(m_selectedInstance, this);
    if (!shortcutDlg.exec())
        return;
    shortcutDlg.createShortcut();
}

void MainWindow::taskEnd()
{
    QObject* sender = QObject::sender();
    if (sender == m_versionLoadTask)
        m_versionLoadTask = NULL;

    sender->deleteLater();
}

void MainWindow::startTask(Task* task)
{
    connect(task, &Task::succeeded, this, &MainWindow::taskEnd);
    connect(task, &Task::failed, this, &MainWindow::taskEnd);
    task->start();
}

void MainWindow::instanceChanged(const QModelIndex& current, [[maybe_unused]] const QModelIndex& previous)
{
    if (!current.isValid()) {
        APPLICATION->settings()->set("SelectedInstance", QString());
        selectionBad();
        return;
    }
    if (m_selectedInstance) {
        disconnect(m_selectedInstance, &BaseInstance::runningStatusChanged, this, &MainWindow::refreshCurrentInstance);
        disconnect(m_selectedInstance, &BaseInstance::profilerChanged, this, &MainWindow::refreshCurrentInstance);
    }
    QString id = current.data(InstanceList::InstanceIDRole).toString();
    m_selectedInstance = APPLICATION->instances()->getInstanceById(id);
    if (m_selectedInstance) {
        ui->instanceToolBar->setEnabled(true);
        setInstanceActionsEnabled(true);
        ui->actionLaunchInstance->setEnabled(m_selectedInstance->canLaunch());
        ui->actionLaunchOfflineCustom->setEnabled(m_selectedInstance->canLaunch());

        ui->actionKillInstance->setEnabled(m_selectedInstance->isRunning());
        ui->actionExportInstance->setEnabled(m_selectedInstance->canExport());
        updateNameLabel();
        nameLabel->setToolTip(m_selectedInstance->name());
        m_statusLeft->setText(m_selectedInstance->getStatusbarDescription());
        updateStatusCenter();
        updateInstanceToolIcon(m_selectedInstance->iconKey());

        updateLaunchButton();

        APPLICATION->settings()->set("SelectedInstance", m_selectedInstance->id());

        connect(m_selectedInstance, &BaseInstance::runningStatusChanged, this, &MainWindow::refreshCurrentInstance);
        connect(m_selectedInstance, &BaseInstance::profilerChanged, this, &MainWindow::refreshCurrentInstance);
    } else {
        APPLICATION->settings()->set("SelectedInstance", QString());
        selectionBad();
        return;
    }
}

void MainWindow::instanceSelectRequest(QString id)
{
    setSelectedInstanceById(id);
}

void MainWindow::instanceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    auto current = view->selectionModel()->currentIndex();
    QItemSelection test(topLeft, bottomRight);
    if (test.contains(current)) {
        instanceChanged(current, current);
    }
}

void MainWindow::selectionBad()
{
    // start by reseting everything...
    m_selectedInstance = nullptr;
    m_statusLeft->setText(tr("No instance selected"));

    statusBar()->clearMessage();
    ui->instanceToolBar->setEnabled(false);
    setInstanceActionsEnabled(false);
    updateLaunchButton();
    updateNameLabel();
    nameLabel->setToolTip(QString());
    updateInstanceToolIcon("default");

    // ...and then see if we can enable the previously selected instance
    setSelectedInstanceById(APPLICATION->settings()->get("SelectedInstance").toString());
}

void MainWindow::checkInstancePathForProblems()
{
    QString instanceFolder = APPLICATION->settings()->get("InstanceDir").toString();
    if (FS::checkProblemticPathJava(QDir(instanceFolder))) {
        QMessageBox warning(this);
        warning.setText(tr("Your instance folder contains \'!\' and this is known to cause Java problems!"));
        warning.setInformativeText(tr("You have now two options: <br/>"
                                      " - change the instance folder in the settings <br/>"
                                      " - move this installation of %1 to a different folder")
                                       .arg(BuildConfig.LAUNCHER_DISPLAYNAME));
        warning.setDefaultButton(QMessageBox::Ok);
        warning.exec();
    }
    auto tempFolderText =
        tr("This is a problem: <br/>"
           " - The launcher will likely be deleted without warning by the operating system <br/>"
           " - close the launcher now and extract it to a real location, not a temporary folder");
    QString pathfoldername = QDir(instanceFolder).absolutePath();
    if (pathfoldername.contains("Rar$", Qt::CaseInsensitive)) {
        QMessageBox warning(this);
        warning.setText(tr("Your instance folder contains \'Rar$\' - that means you haven't extracted the launcher archive!"));
        warning.setInformativeText(tempFolderText);
        warning.setDefaultButton(QMessageBox::Ok);
        warning.exec();
    } else if (pathfoldername.startsWith(QDir::tempPath()) || pathfoldername.contains("/TempState/")) {
        QMessageBox warning(this);
        warning.setText(tr("Your instance folder is in a temporary folder: \'%1\'!").arg(QDir::tempPath()));
        warning.setInformativeText(tempFolderText);
        warning.setDefaultButton(QMessageBox::Ok);
        warning.exec();
    }
}

void MainWindow::updateStatusCenter()
{
    m_statusCenter->setVisible(APPLICATION->settings()->get("ShowGlobalGameTime").toBool());
    int64_t timePlayed = APPLICATION->playtimeSettings()->get("TotalPlayTime").toLongLong();
    if (timePlayed > 0) {
        m_statusCenter->setText(
            tr("Total playtime: %1")
                .arg(Time::prettifyDuration(timePlayed, APPLICATION->settings()->get("ShowGameTimeWithoutDays").toBool())));
    }
}
// "Instance actions" are actions that require an instance to be selected (i.e. "new instance" is not here)
// Actions that also require other conditions (e.g. a running instance) won't be changed.
void MainWindow::setInstanceActionsEnabled(bool enabled)
{
    ui->actionEditInstance->setEnabled(enabled);
    ui->actionChangeInstGroup->setEnabled(enabled);
    ui->actionViewSelectedInstFolder->setEnabled(enabled);
    ui->actionExportInstance->setEnabled(enabled);
    ui->actionDeleteInstance->setEnabled(enabled);
    ui->actionCopyInstance->setEnabled(enabled);
    ui->actionCreateInstanceShortcut->setEnabled(enabled);
}

void MainWindow::refreshCurrentInstance()
{
    auto current = view->selectionModel()->currentIndex();
    instanceChanged(current, current);
}

void MainWindow::updateNameLabel()
{
    if (m_selectedInstance == nullptr) {
        nameLabel->setText(QObject::tr("No Instance Selected"));
    } else {
        const auto metrics = nameLabel->fontMetrics();
        nameLabel->setText(metrics.elidedText(m_selectedInstance->name(), Qt::ElideRight, nameLabel->width()));
    }
}

void MainWindow::updateInstanceToolBarOrientation(Qt::Orientation orientation)
{
    ui->instanceToolBar->removeAction(ui->actionChangeInstIcon);

    if (orientation == Qt::Vertical) {
        instanceInfoWidget->setVisible(true);
    } else {
        instanceInfoWidget->setVisible(false);
        ui->instanceToolBar->insertActionAfter(ui->actionRenameInstance, ui->actionChangeInstIcon);
    }
    // HACK: sync size (no, I don't know why this works)
    ui->instanceToolBar->resize(0, 0);
}