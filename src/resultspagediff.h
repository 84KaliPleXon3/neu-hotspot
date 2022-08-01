/*
    SPDX-FileCopyrightText: Lieven Hey <lieven.hey@kdab.com>
    SPDX-FileCopyrightText: 2022 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QWidget>

class QMenu;
class QAction;

namespace Ui {
class ResultsPageDiff;
}

namespace Data {
struct Symbol;
}

namespace KDDockWidgets {
class MainWindow;
class DockWidget;
class DockWidgetBase;
}

class PerfParser;
class ResultsBottomUpPage;
class ResultsTopDownPage;
class ResultsFlameGraphPage;
class ResultsCallerCalleePage;
class FilterAndZoomStack;
class TimeLineWidget;
class CostContextMenu;

class ResultsPageDiff : public QWidget
{
    Q_OBJECT
public:
    explicit ResultsPageDiff(QWidget* parent = nullptr);
    ~ResultsPageDiff();

    void clear();
    QMenu* filterMenu() const;
    QMenu* exportMenu() const;
    QList<QAction*> windowActions() const;

    void initDockWidgets(const QVector<KDDockWidgets::DockWidgetBase*>& restored);

public slots:
    void setSysroot(const QString& path);
    void setAppPath(const QString& path);

    void onJumpToCallerCallee(const Data::Symbol& symbol);
    void onOpenEditor(const Data::Symbol& symbol);
    void showError(const QString& message);
    void createDiffReport(const QString& fileA, const QString& fileB);

signals:
    void navigateToCode(const QString& url, int lineNumber, int columnNumber);
    void parsingFinished();

private:
    void resizeEvent(QResizeEvent* event) override;
    void repositionFilterBusyIndicator();

    PerfParser* m_fileA;
    PerfParser* m_fileB;

    QScopedPointer<Ui::ResultsPageDiff> ui;
    KDDockWidgets::MainWindow* m_contents;
    FilterAndZoomStack* m_filterAndZoomStack;
    CostContextMenu* m_costContextMenu;
    QMenu* m_filterMenu;
    QMenu* m_exportMenu;
    KDDockWidgets::DockWidget* m_bottomUpDock;
    ResultsBottomUpPage* m_resultsBottomUpPage;
    KDDockWidgets::DockWidget* m_topDownDock;
    ResultsTopDownPage* m_resultsTopDownPage;
    KDDockWidgets::DockWidget* m_flameGraphDock;
    ResultsFlameGraphPage* m_resultsFlameGraphPage;
    KDDockWidgets::DockWidget* m_callerCalleeDock;
    ResultsCallerCalleePage* m_resultsCallerCalleePage;
    KDDockWidgets::DockWidget* m_timeLineDock;
    TimeLineWidget* m_timeLineWidget;
    QWidget* m_filterBusyIndicator = nullptr;

    int m_runningParsersCounter = 0;

    bool m_timelineVisible;
};
