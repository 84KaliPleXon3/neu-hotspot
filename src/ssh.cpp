/*
    SPDX-FileCopyrightText: Lieven Hey <lieven.hey@kdab.com>
    SPDX-FileCopyrightText: 2022 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ssh.h"

#include <memory>

#include <QProcess>

#include <KConfigGroup>
#include <KSharedConfig>
#include <KShell>

#include "settings.h"
#include "util.h"

namespace {
QStringList sshConnectArguments(const QString& username, const QString& hostname, const QString& sshOptions)
{
    const auto sshConnectArg = username.isEmpty() ? hostname : QLatin1String("%1@%2").arg(username, hostname);
    QStringList arguments = {sshConnectArg};
    if (!sshOptions.isEmpty()) {
        arguments.append(KShell::splitArgs(sshOptions));
    }
    return arguments;
};

QStringList assembleSSHArguments(const QString& deviceName, const QStringList& command)
{
    KConfigGroup device = KSharedConfig::openConfig()->group("devices").group(deviceName);

    const auto username = device.readEntry("username", QString());
    const auto hostname = device.readEntry("hostname", QString());
    const auto sshOptions = device.readEntry("sshoptions", QString());

    auto arguments = sshConnectArguments(username, hostname, sshOptions);
    arguments.append(command);

    return arguments;
}

QProcessEnvironment sshEnvironment()
{
    auto env = Util::appImageEnvironment();
    Settings* settings = Settings::instance();
    const auto path = settings->sshaskPassPath();
    if (!path.isEmpty() && env.value(QStringLiteral("SSH_ASKPASS")).isEmpty()) {
        env.insert(QStringLiteral("SSH_ASKPASS"), path);
    }
    return env;
}
}

std::unique_ptr<QProcess> createSshProcess(const QString& username, const QString& hostname, const QString& options,
                                           const QStringList& command, const QString& executable)
{
    std::unique_ptr<QProcess> ssh(new QProcess);
    ssh->setProgram(QStandardPaths::findExecutable(executable));
    const auto arguments = sshConnectArguments(username, hostname, options) + command;
    ssh->setArguments(arguments);
    ssh->setProcessEnvironment(sshEnvironment());
    ssh->start();
    ssh->waitForFinished();
    return ssh;
}

std::unique_ptr<QProcess> createSshProcess(const QString& deviceName, const QStringList& command)
{
    std::unique_ptr<QProcess> ssh(new QProcess);
    ssh->setProgram(QStandardPaths::findExecutable(QStringLiteral("ssh")));
    const auto arguments = assembleSSHArguments(deviceName, command);
    ssh->setArguments(arguments);
    ssh->setProcessEnvironment(sshEnvironment());
    ssh->start();
    ssh->waitForFinished();
    return ssh;
}

QString sshOutput(const QString& deviceName, const QStringList& command)
{
    auto ssh = createSshProcess(deviceName, command);
    return QString::fromUtf8(ssh->readAll());
}

int sshExitCode(const QString& deviceName, const QStringList& command)
{
    auto ssh = createSshProcess(deviceName, command);
    return ssh->exitCode();
}
