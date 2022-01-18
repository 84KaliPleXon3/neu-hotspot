/*
    SPDX-FileCopyrightText: Lieven Hey <lieven.hey@kdab.com>
    SPDX-FileCopyrightText: 2022 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <memory>

#include <QString>

class QStringList;
class QProcess;

// used in the dialog to test connection and copy ssh keys
std::unique_ptr<QProcess> createSshProcess(const QString& username, const QString& hostname, const QString& options,
                                           const QStringList& command,
                                           const QString& executable = QStringLiteral("ssh"));

// used when recording
std::unique_ptr<QProcess> createSshProcess(const QString& deviceName, const QStringList& command);
QString sshOutput(const QString& deviceName, const QStringList& command);
int sshExitCode(const QString& deviceName, const QStringList& command);
