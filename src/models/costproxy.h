/*
    SPDX-FileCopyrightText: Lieven Hey <lieven.hey@kdab.com>
    SPDX-FileCopyrightText: Milian Wolff <milian.wolff@kdab.com>
    SPDX-FileCopyrightText: 2016-2022 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QSortFilterProxyModel>

#include "../util.h"
#include "callercalleeproxy.h"

template<typename Model>
class CostProxy : public QSortFilterProxyModel
{
public:
    explicit CostProxy(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
        setRecursiveFilteringEnabled(true);
        setDynamicSortFilter(true);
    }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& parent) const override
    {
        Q_UNUSED(source_row);

        const auto* model = qobject_cast<Model*>(sourceModel());
        Q_ASSERT(model);

        const auto* item = model->itemFromIndex(parent);
        if (!item) {
            return false;
        }

        return CallerCalleeProxyDetail::match(this, item->symbol);
    }
};

template<typename Model>
class CostProxyDiff : public CostProxy<Model>
{
public:
    explicit CostProxyDiff(QObject* parent = nullptr)
        : CostProxy<Model>(parent)
    {
    }

    QVariant data(const QModelIndex& index, int role) const override
    {
        if (index.column() >= Model::NUM_BASE_COLUMNS) {
            const auto model = qobject_cast<Model*>(CostProxy<Model>::sourceModel());
            Q_ASSERT(model);

            const auto node = model->itemFromIndex(CostProxy<Model>::mapToSource(index));

            const auto baseColumn = (index.column() - Model::NUM_BASE_COLUMNS) / 2;
            const auto column = baseColumn + (index.column() - Model::NUM_BASE_COLUMNS) % 2;

            auto cost = [model, node](int column) -> float { return model->results().costs.cost(column, node->id); };

            auto totalCost = [model](int column) -> float { return model->results().costs.totalCost(column); };

            if (column == baseColumn) {
                if (role == Model::TotalCostRole) {
                    return totalCost(column);
                } else if (role == Model::SortRole) {
                    return cost(column) / totalCost(column);
                } else if (role == Qt::DisplayRole) {
                    return Util::formatCostRelative(cost(column), totalCost(column), true);
                }
            } else {
                if (role == Model::TotalCostRole) {
                    return cost(baseColumn);
                } else if (role == Model::SortRole) {
                    return cost(column) / cost(baseColumn);
                } else if (role == Qt::DisplayRole) {
                    return Util::formatCostRelative(cost(column), cost(baseColumn), true);
                }
            }
        }

        return CostProxy<Model>::data(index, role);
    }
};
