// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "DarkTheme.h"

class BoostTheme : public DarkTheme {
   public:
    explicit BoostTheme(QString themeId = "boostclient");

    QString id() override;
    QString name() override;
    QString tooltip() override;
    QString appStyleSheet() override;
    QPalette colorScheme() override;
    double fadeAmount() override;
    QColor fadeColor() override;

   private:
    QString m_themeId;
};
