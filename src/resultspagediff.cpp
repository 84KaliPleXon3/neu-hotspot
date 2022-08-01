/*
    SPDX-FileCopyrightText: Lieven Hey <lieven.hey@kdab.com>
    SPDX-FileCopyrightText: 2022 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "resultspagediff.h"
#include "ui_resultspagediff.h"

#include "parsers/perf/perfparser.h"

#include "costcontextmenu.h"
#include "dockwidgetsetup.h"
#include "resultsbottomuppage.h"
#include "resultscallercalleepage.h"
#include "resultsflamegraphpage.h"
#include "resultstopdownpage.h"
#include "resultsutil.h"

#include "timelinewidget.h"

#include "models/filterandzoomstack.h"

#include <KLocalizedString>

#include <kddockwidgets/DockWidget.h>
#include <kddockwidgets/MainWindow.h>

#include <QDebug>
#include <QLabel>
#include <QMenu>
#include <QProgressBar>
#include <QTimer>

#include <QDebug>

ResultsPageDiff::ResultsPageDiff(QWidget* parent)
    : QWidget(parent)
    , m_fileA(new PerfParser(this))
    , m_fileB(new PerfParser(this))
    , ui(new Ui::ResultsPageDiff)
    , m_contents(createDockingArea(QStringLiteral("resultsDiff"), this))
    , m_filterAndZoomStack(new FilterAndZoomStack(this))
    , m_costContextMenu(new CostContextMenu(this))
    , m_filterMenu(new QMenu(this))
    , m_exportMenu(new QMenu(tr("Export"), this))
    , m_resultsBottomUpPage(
          new ResultsBottomUpPage(m_filterAndZoomStack, nullptr, m_costContextMenu, m_exportMenu, this))
    , m_resultsTopDownPage(new ResultsTopDownPage(m_filterAndZoomStack, m_fileA, m_costContextMenu, this))
    , m_resultsFlameGraphPage(new ResultsFlameGraphPage(m_filterAndZoomStack, m_fileA, m_exportMenu, this))
    , m_resultsCallerCalleePage(new ResultsCallerCalleePage(m_filterAndZoomStack, m_fileA, m_costContextMenu, this))
    , m_timeLineWidget(new TimeLineWidget(m_fileA, m_filterMenu, m_filterAndZoomStack, this))
    , m_timelineVisible(true)
{
    m_exportMenu->setIcon(QIcon::fromTheme(QStringLiteral("document-export")));
    {
        const auto actions = m_filterAndZoomStack->actions();
        m_filterMenu->addAction(actions.filterOut);
        m_filterMenu->addAction(actions.resetFilter);
        m_filterMenu->addSeparator();
        m_filterMenu->addAction(actions.zoomOut);
        m_filterMenu->addAction(actions.resetZoom);
        m_filterMenu->addSeparator();
        m_filterMenu->addAction(actions.resetFilterAndZoom);
    }

    ui->setupUi(this);
    ui->verticalLayout->addWidget(m_contents);

    ui->errorWidget->hide();
    ui->lostMessage->hide();

    auto dockify = [](QWidget* widget, const QString& id, const QString& title, const QString& shortcut) {
        auto* dock = new KDDockWidgets::DockWidget(id);
        dock->setWidget(widget);
        dock->setTitle(title);
        dock->toggleAction()->setShortcut(shortcut);
        return dock;
    };

    m_bottomUpDock = dockify(m_resultsBottomUpPage, QStringLiteral("dbottomUp"), tr("Bottom &Up"), tr("Ctrl+U"));
    m_contents->addDockWidget(m_bottomUpDock, KDDockWidgets::Location_OnTop);
    m_topDownDock = dockify(m_resultsTopDownPage, QStringLiteral("dtopDown"), tr("Top &Down"), tr("Ctrl+D"));
    m_bottomUpDock->addDockWidgetAsTab(m_topDownDock);
    m_flameGraphDock =
        dockify(m_resultsFlameGraphPage, QStringLiteral("dflameGraph"), tr("Flame &Graph"), tr("Ctrl+G"));
    m_bottomUpDock->addDockWidgetAsTab(m_flameGraphDock);
    m_callerCalleeDock =
        dockify(m_resultsCallerCalleePage, QStringLiteral("dcallerCallee"), tr("Ca&ller / Callee"), tr("Ctrl+L"));
    m_bottomUpDock->addDockWidgetAsTab(m_callerCalleeDock);
    m_bottomUpDock->setAsCurrentTab();

    m_timeLineDock = dockify(m_timeLineWidget, QStringLiteral("dtimeLine"), tr("&Time Line"), tr("Ctrl+T"));
    m_contents->addDockWidget(m_timeLineDock, KDDockWidgets::Location_OnBottom);

    connect(m_filterAndZoomStack, &FilterAndZoomStack::filterChanged, m_fileA, &PerfParser::filterResults);

    connect(m_fileA, &PerfParser::parserWarning, this, &ResultsPageDiff::showError);

    connect(m_resultsCallerCalleePage, &ResultsCallerCalleePage::navigateToCode, this,
            &ResultsPageDiff::navigateToCode);
    connect(m_resultsCallerCalleePage, &ResultsCallerCalleePage::navigateToCodeFailed, this,
            &ResultsPageDiff::showError);
    connect(m_resultsCallerCalleePage, &ResultsCallerCalleePage::selectSymbol, m_timeLineWidget,
            &TimeLineWidget::selectSymbol);

    connect(m_resultsBottomUpPage, &ResultsBottomUpPage::jumpToCallerCallee, this,
            &ResultsPageDiff::onJumpToCallerCallee);
    connect(m_resultsBottomUpPage, &ResultsBottomUpPage::openEditor, this, &ResultsPageDiff::onOpenEditor);
    connect(m_resultsBottomUpPage, &ResultsBottomUpPage::selectSymbol, m_timeLineWidget, &TimeLineWidget::selectSymbol);
    connect(m_resultsTopDownPage, &ResultsTopDownPage::jumpToCallerCallee, this,
            &ResultsPageDiff::onJumpToCallerCallee);
    connect(m_resultsTopDownPage, &ResultsTopDownPage::openEditor, this, &ResultsPageDiff::onOpenEditor);
    connect(m_resultsTopDownPage, &ResultsTopDownPage::selectSymbol, m_timeLineWidget, &TimeLineWidget::selectSymbol);
    connect(m_resultsFlameGraphPage, &ResultsFlameGraphPage::jumpToCallerCallee, this,
            &ResultsPageDiff::onJumpToCallerCallee);
    connect(m_resultsFlameGraphPage, &ResultsFlameGraphPage::openEditor, this, &ResultsPageDiff::onOpenEditor);
    connect(m_resultsFlameGraphPage, &ResultsFlameGraphPage::selectStack, m_timeLineWidget,
            &TimeLineWidget::selectStack);
    connect(m_timeLineWidget, &TimeLineWidget::stacksHovered, m_resultsFlameGraphPage,
            &ResultsFlameGraphPage::setHoveredStacks);

    connect(m_fileA, &PerfParser::parsingStarted, this, [this]() {
        // disable when we apply a filter
        m_contents->setEnabled(false);
        repositionFilterBusyIndicator();
        m_filterBusyIndicator->setVisible(true);
    });
    connect(m_fileA, &PerfParser::parsingFinished, this, [this]() {
        // re-enable when we finished filtering
        m_contents->setEnabled(true);
        m_filterBusyIndicator->setVisible(false);
    });

    for (const auto& parser : {m_fileA, m_fileB}) {
        connect(parser, &PerfParser::parsingStarted, this, [this] { m_runningParsersCounter++; });
        // TODO: error handling
        connect(parser, &PerfParser::parsingFailed, this, [this] { m_runningParsersCounter--; });
        connect(parser, &PerfParser::parsingFinished, this, [this] {
            m_runningParsersCounter--;
            if (m_runningParsersCounter == 0) {
                emit parsingFinished();
            }
        });
    }

    connect(this, &ResultsPageDiff::parsingFinished, this, [this] {
        const auto bottomUpData =
            Data::BottomUpResults::diffBottomUpResults(m_fileA->bottomUpResults(), m_fileB->bottomUpResults());
        m_resultsBottomUpPage->setBottomUpResults(bottomUpData);
    });

    {
        // create a busy indicator
        m_filterBusyIndicator = new QWidget(this);
        m_filterBusyIndicator->setMinimumHeight(100);
        m_filterBusyIndicator->setVisible(false);
        m_filterBusyIndicator->setToolTip(i18n("Filtering in progress, please wait..."));
        auto layout = new QVBoxLayout(m_filterBusyIndicator);
        layout->setAlignment(Qt::AlignCenter);
        auto progressBar = new QProgressBar(m_filterBusyIndicator);
        layout->addWidget(progressBar);
        progressBar->setMaximum(0);
        auto label = new QLabel(m_filterBusyIndicator->toolTip(), m_filterBusyIndicator);
        label->setAlignment(Qt::AlignCenter);
        layout->addWidget(label);
    }
}

ResultsPageDiff::~ResultsPageDiff() = default;

void ResultsPageDiff::setSysroot(const QString& path)
{
    m_resultsCallerCalleePage->setSysroot(path);
}

void ResultsPageDiff::setAppPath(const QString& path)
{
    m_resultsCallerCalleePage->setAppPath(path);
}

static void showDock(KDDockWidgets::DockWidget* dock)
{
    dock->show();
    dock->setFocus();
    dock->setAsCurrentTab();
}

void ResultsPageDiff::onJumpToCallerCallee(const Data::Symbol& symbol)
{
    m_resultsCallerCalleePage->jumpToCallerCallee(symbol);
    showDock(m_callerCalleeDock);
}

void ResultsPageDiff::onOpenEditor(const Data::Symbol& symbol)
{
    m_resultsCallerCalleePage->openEditor(symbol);
}

void ResultsPageDiff::clear()
{
    m_resultsBottomUpPage->clear();
    m_resultsTopDownPage->clear();
    m_resultsCallerCalleePage->clear();
    m_resultsFlameGraphPage->clear();
    m_exportMenu->clear();

    m_filterAndZoomStack->clear();
}

QMenu* ResultsPageDiff::filterMenu() const
{
    return m_filterMenu;
}

QMenu* ResultsPageDiff::exportMenu() const
{
    return m_exportMenu;
}

QList<QAction*> ResultsPageDiff::windowActions() const
{
    auto ret = QList<QAction*> {m_bottomUpDock->toggleAction(), m_topDownDock->toggleAction(),
                                m_flameGraphDock->toggleAction(), m_callerCalleeDock->toggleAction(),
                                m_timeLineDock->toggleAction()};
    return ret;
}

void ResultsPageDiff::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    repositionFilterBusyIndicator();
}

void ResultsPageDiff::repositionFilterBusyIndicator()
{
    auto geometry = m_filterBusyIndicator->geometry();
    geometry.setWidth(width() / 2);
    geometry.moveCenter(rect().center());
    m_filterBusyIndicator->setGeometry(geometry);
}

void ResultsPageDiff::showError(const QString& message)
{
    ui->errorWidget->setText(message);
    ui->errorWidget->animatedShow();
    QTimer::singleShot(5000, ui->errorWidget, &KMessageWidget::animatedHide);
}

void ResultsPageDiff::initDockWidgets(const QVector<KDDockWidgets::DockWidgetBase*>& restored)
{
    Q_ASSERT(restored.contains(m_bottomUpDock));

    const auto docks = {m_bottomUpDock, m_topDownDock, m_flameGraphDock, m_callerCalleeDock, m_timeLineDock};
    for (auto dock : docks) {
        if (!dock || restored.contains(dock))
            continue;

        auto initialOption = KDDockWidgets::InitialOption {};
        m_bottomUpDock->addDockWidgetAsTab(dock, initialOption);
    }
}

void ResultsPageDiff::createDiffReport(const QString& fileA, const QString& fileB)
{
    m_fileA->startParseFile(fileA);
    m_fileB->startParseFile(fileB);
}
