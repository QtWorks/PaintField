#include "paintfield-core/util.h"

#include "brushlibrarymodel.h"

namespace PaintField {

BrushLibraryModel::BrushLibraryModel(QObject *parent) :
    LibraryModel(parent)
{
}

QVariant  BrushLibraryModel::data(const QModelIndex &index, int role) const
{
	auto value = LibraryModel::data(index, role);
	
	switch (role)
	{
		case Qt::DisplayRole:
		case Qt::EditRole:
		{
			if (itemFromIndex(index)->type() == LibraryItemType::File)
				return value.toString().section('.', 0, -2);
			else
				break;
		}
		default:
			break;
	}
	return value;
}

QVariantMap BrushLibraryModel::loadPreset(const QModelIndex &index)
{
	return loadJsonFromFile(pathFromIndex(index)).toMap();
}

bool BrushLibraryModel::savePreset(const QModelIndex &parent, const QString &name, const QVariantMap &data)
{
	auto parentItem = itemFromIndex(parent);
	if (parentItem->type() != LibraryItemType::Dir)
		return false;
	
	return saveJsonToFile(QDir(pathFromItem(parentItem)).filePath(name), data);
}

}