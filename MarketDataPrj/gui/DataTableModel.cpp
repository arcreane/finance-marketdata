#include "DataTableModel.h"
#include <QString>

DataTableModel::DataTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int DataTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return ticks_.size();
}

int DataTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return 7; // symbol, last, open, high, low, volume, timestamp
}

QVariant DataTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return {};

    const MarketDataTick& t = ticks_.at(index.row());
    switch (index.column()) {
    case 0: return QString::fromStdString(t.symbol);
    case 1: return t.last_price;
    case 2: return t.open;
    case 3: return t.high;
    case 4: return t.low;
    case 5: return static_cast<qlonglong>(t.volume);
    case 6: return static_cast<qlonglong>(t.timestamp);
    default: return {};
    }
}

QVariant DataTableModel::headerData(int section, Qt::Orientation orientation,
    int role) const
{
    if (role != Qt::DisplayRole)
        return {};

    if (orientation == Qt::Horizontal) {
        switch (section) {
        case 0: return "Symbol";
        case 1: return "Last";
        case 2: return "Open";
        case 3: return "High";
        case 4: return "Low";
        case 5: return "Volume";
        case 6: return "Timestamp";
        default: return {};
        }
    }
    return section + 1;
}

void DataTableModel::addTick(const MarketDataTick& tick)
{
    beginInsertRows(QModelIndex(), ticks_.size(), ticks_.size());
    ticks_.push_back(tick);
    endInsertRows();
}

