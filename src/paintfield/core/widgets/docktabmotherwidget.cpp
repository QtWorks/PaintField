#include "docktabmotherwidget.h"

#include <QSplitter>
#include <QHBoxLayout>
#include <amulet/range_extension.hh>
#include <amulet/int_range.hh>
#include "docktabwidget.h"

namespace PaintField
{

QPoint mapToAncestor(QWidget *ancestor, QWidget *widget, const QPoint &pos)
{
	QPoint result = pos;
	
	do
	{
		result = widget->mapToParent(result);
		widget = widget->parentWidget();
	}
	while (widget && widget != ancestor);
	
	return result;
}

DockTabMotherWidget::DockTabMotherWidget(QWidget *parent) :
	QWidget(parent)
{
	setStyleSheet("QSplitter::handle { background:darkGray; } QSplitter::handle:horizontal { width:1px; } QSplitter::handle:vertical { height:1px; }");
	
	_mainHorizontalSplitter = createSplitter(Qt::Horizontal);
	_mainVerticalSplitter = createSplitter(Qt::Vertical);
	_centralWidget = new QWidget;
	
	_mainVerticalSplitter->addWidget(_centralWidget);
	_mainVerticalSplitter->setStretchFactor(0, 1);
	
	_mainHorizontalSplitter->addWidget(_mainVerticalSplitter);
	_mainHorizontalSplitter->setStretchFactor(0, 1);
	
	auto layout = new QHBoxLayout;
	layout->addWidget(_mainHorizontalSplitter);
	layout->setContentsMargins(0, 0, 0, 0);
	setLayout(layout);
	
	setAcceptDrops(true);
}

bool DockTabMotherWidget::insertTabWidget(DockTabWidget *tabWidget, const TabWidgetArea &area)
{
	if (!area.isValid())
		goto error;
	
	if (area.tabWidgetIndex == -1)
	{
		Qt::Orientation orientation;
		if (area.dir == Left || area.dir == Right)
			orientation = Qt::Vertical;
		else
			orientation = Qt::Horizontal;
		
		auto splitter = createSplitter(orientation);
		splitter->addWidget(tabWidget);
		
		int mainSplitterIndex;
		QSplitter *mainSplitter;
		switch (area.dir)
		{
			case Left:
				mainSplitterIndex = area.splitterIndex;
				mainSplitter = _mainHorizontalSplitter;
				break;
			case Right:
				mainSplitterIndex = _mainHorizontalSplitter->count() - area.splitterIndex;
				mainSplitter = _mainHorizontalSplitter;
				break;
			case Top:
				mainSplitterIndex = area.splitterIndex;
				mainSplitter = _mainVerticalSplitter;
				break;
			case Bottom:
				mainSplitterIndex = _mainVerticalSplitter->count() - area.splitterIndex;
				mainSplitter = _mainVerticalSplitter;
				break;
			default:
				goto error;
		}
		
		mainSplitter->insertWidget(mainSplitterIndex, splitter);
		mainSplitter->setStretchFactor(mainSplitterIndex, 0);
		
		_splitterLists[area.dir].insert(area.splitterIndex, splitter);
	}
	else
	{
		QSplitter *splitter = _splitterLists[area.dir].at(area.splitterIndex);
		splitter->insertWidget(area.tabWidgetIndex, tabWidget);
	}
	
	connect(tabWidget, SIGNAL(willBeAutomaticallyDeleted(DockTabWidget*)), this, SLOT(onTabWidgetWillBeDeleted(DockTabWidget*)));
	
	return true;
	
error:
	
	PAINTFIELD_WARNING << "failed";
	//tabWidget->deleteLater();
	return false;
}

void DockTabMotherWidget::addTabWidget(DockTabWidget *tabWidget, Direction dir, int splitterIndex)
{
	int tabWidgetIndex;
	
	if (splitterCount(dir) <= splitterIndex)
		tabWidgetIndex = -1;
	else
		tabWidgetIndex = tabWidgetCount(dir, splitterIndex);
	
	insertTabWidget(tabWidget, TabWidgetArea(dir, splitterIndex, tabWidgetIndex));
}

int DockTabMotherWidget::tabWidgetCount(Direction dir, int splitterIndex)
{
	return _splitterLists[dir].at(splitterIndex)->count();
}

int DockTabMotherWidget::splitterCount(Direction dir)
{
	return _splitterLists[dir].size();
}

void DockTabMotherWidget::setCentralWidget(QWidget *widget)
{
	int index = _splitterLists[Top].size();
	_mainVerticalSplitter->widget(index)->deleteLater();
	_mainVerticalSplitter->insertWidget(index, widget);
}

static QList<int> intListFromVariant(const QVariant &x)
{
	return x.toList()++.map([](const QVariant &x){
		return x.toInt();
	}).to<QList>();
}

static QVariant variantFromIntList(const QList<int> list)
{
	return list++.map(&QVariant::fromValue<int>).to<QList>();
}

QString DockTabMotherWidget::stringFromDirection(Direction dir)
{
	switch (dir)
	{
		default:
		case Left:
			return "left";
		case Right:
			return "right";
		case Top:
			return "top";
		case Bottom:
			return "bottom";
	}
}

void DockTabMotherWidget::setSizesState(const QVariantMap &data)
{
	// set vertical / horizontal sizes
	{
		auto verticalSizes = intListFromVariant(data["vertical"]);
		auto horizontalSizes = intListFromVariant(data["horizontal"]);
		
		_mainVerticalSplitter->setSizes(verticalSizes);
		_mainHorizontalSplitter->setSizes(horizontalSizes);
	}
	
	// set sizes of each column
	{
		auto getSizesList = [data]( const QString &str )
		{
			return data[str].toList()++.map(&intListFromVariant).to<QList>();
		};
		
		for (auto dir : {Left, Right, Top, Bottom})
		{
			auto sizesList = getSizesList(stringFromDirection(dir));
			auto splitters = _splitterLists[dir];
			
			if (sizesList.size() != splitters.size())
				return;
			
			auto splittersI = splitters.begin();
			
			for (const auto &sizes : sizesList)
				(*splittersI++)->setSizes(sizes);
		}
	}
}

QVariantMap DockTabMotherWidget::sizesState()
{
	QVariantMap data;
	data["vertical"] = variantFromIntList(_mainVerticalSplitter->sizes());
	data["horizontal"] = variantFromIntList(_mainHorizontalSplitter->sizes());
	
	for (auto dir : {Left, Right, Top, Bottom})
	{
		data[stringFromDirection(dir)] = _splitterLists[dir]++.map([](QSplitter *splitter){
			return variantFromIntList(splitter->sizes());
		}).to<QList>();
	}
	
	return data;
}

void DockTabMotherWidget::setTabIndexState(const QVariantMap &data)
{
	auto setIndexList = [](QSplitter *splitter, const QVariantList &list)
	{
		int count = list.size();
		
		if (splitter->count() != count)
			return;
		
		for (int i = 0; i < count; ++i)
		{
			auto tabWidget = dynamic_cast<DockTabWidget *>(splitter->widget(i));
			if (tabWidget)
				tabWidget->setCurrentIndex(list[i].toInt());
		}
	};
	
	for (auto dir : {Left, Right, Top, Bottom})
	{
		auto splitters = _splitterLists[dir];
		auto lists = data[stringFromDirection(dir)].toList();
		
		if (splitters.size() != lists.size())
			return;
		
		int count = splitters.size();
		for (int i = 0; i < count; ++i)
			setIndexList(splitters[i], lists[i].toList());
	}
}

template <typename TUnaryOperator>
QVariantMap DockTabMotherWidget::packDataForEachTabWidget(TUnaryOperator op)
{
	auto getIndexList = [op](QSplitter *splitter)
	{
		return Amulet::intRange(0, splitter->count()).map([splitter](int x){
			return splitter->widget(x);}
		).map([op](QWidget *w)->QVariant{
			return op(w);
		}).template to<QVariantList>();
	};
	
	QVariantMap data;
	
	for (auto dir : {Left, Right, Top, Bottom})
	{
		data[stringFromDirection(dir)] = _splitterLists[dir]++.map(getIndexList).template to<QVariantList>();
	}
	
	return data;
}

QVariantMap DockTabMotherWidget::tabIndexState()
{
	auto getCurrentIndex = [](QWidget *w)
	{
		auto tabWidget = dynamic_cast<DockTabWidget *>(w);
		return tabWidget ? tabWidget->currentIndex() : 0;
	};
	
	return packDataForEachTabWidget(getCurrentIndex);
}

QVariantMap DockTabMotherWidget::tabObjectNameState()
{
	auto getObjectNames = [](QWidget *w) {

		auto tabWidget = dynamic_cast<DockTabWidget *>(w);
		if (tabWidget) {
			return Amulet::intRange(0, tabWidget->count()).map([tabWidget](int x)->QVariant{
				return tabWidget->widget(x)->objectName();
			}).to<QVariantList>();
		} else {
			return QVariantList();
		}
	};
	
	return packDataForEachTabWidget(getObjectNames);
}

DockTabMotherWidget::TabWidgetArea DockTabMotherWidget::dropArea(const QPoint &pos)
{
	for (int i = Left; i <= Bottom; ++i)
	{
		auto dir = (Direction)i;
		
		if (_splitterLists[dir].size())
		{
			QRect rect = splittersRect(dir).adjusted(-insertDistance(), -insertDistance(), insertDistance(), insertDistance());
			if (rect.contains(pos))
			{
				TabWidgetArea area = dropAreaAt(pos, dir);
				if (area.isValid())
					return area;
			}
		}
	}
	
	for (int i = Left; i <= Bottom; ++i)
	{
		auto dir = (Direction)i;
		
		if (_splitterLists[dir].size() == 0)
		{
			int distFromSide;
			switch (dir)
			{
				default:
				case Left:
					distFromSide = pos.x();
					break;
				case Right:
					distFromSide = width() - pos.x();
					break;
				case Top:
					distFromSide = pos.y();
					break;
				case Bottom:
					distFromSide = height() - pos.y();
					break;
			}
			
			if (distFromSide < insertDistance())
				return TabWidgetArea(dir, 0, -1);
		}
	}
	
	return TabWidgetArea();
}

DockTabMotherWidget::TabWidgetArea DockTabMotherWidget::dropAreaAt(const QPoint &pos, Direction dir)
{
	int countSplitter = _splitterLists[dir].size();
	
	for (int indexSplitter = 0; indexSplitter < countSplitter; ++indexSplitter)
	{
		QSplitter *splitter = _splitterLists[dir].at(indexSplitter);
		
		int countTabWidget = splitter->count();
		
		for (int indexTabWidget = 0; indexTabWidget < countTabWidget; ++indexTabWidget)
		{
			InsertionDirection insertionDir;
			
			if (getInsertionDirection(pos, splitter->widget(indexTabWidget), dir, insertionDir))
			{
				switch (insertionDir)
				{
					case NextSplitter:
						return TabWidgetArea(dir, indexSplitter + 1, -1);
					case PreviousSplitter:
						return TabWidgetArea(dir, indexSplitter, -1);
					case Next:
						return TabWidgetArea(dir, indexSplitter, indexTabWidget + 1);
					case Previous:
						return TabWidgetArea(dir, indexSplitter, indexTabWidget);
					default:
						break;
				}
			}
		}
	}
	return TabWidgetArea();
}

bool DockTabMotherWidget::getInsertionDirection(const QPoint &pos, QWidget *widget, Direction dockDir, InsertionDirection &insertDir)
{
	auto insertionFromAbsoluteDir = [dockDir](Direction absDir)->InsertionDirection
	{
		switch (dockDir)
		{
			default:
			case Left:
				switch (absDir)
				{
					default:
					case Left:
						return PreviousSplitter;
					case Right:
						return NextSplitter;
					case Top:
						return Previous;
					case Bottom:
						return Next;
				}
			case Right:
				switch (absDir)
				{
					default:
					case Left:
						return NextSplitter;
					case Right:
						return PreviousSplitter;
					case Top:
						return Previous;
					case Bottom:
						return Next;
				}
			case Top:
				switch (absDir)
				{
					default:
					case Left:
						return Previous;
					case Right:
						return Next;
					case Top:
						return PreviousSplitter;
					case Bottom:
						return NextSplitter;
				}
			case Bottom:
				switch (absDir)
				{
					default:
					case Left:
						return Previous;
					case Right:
						return Next;
					case Top:
						return NextSplitter;
					case Bottom:
						return PreviousSplitter;
				}
		}
	};
	
	auto isInInsertDist = [this](int x, int border)->bool
	{
		return border - insertDistance() <= x && x < border + insertDistance();
	};
	
	QRect rect = widget->rect();
	rect.moveTopLeft(mapToAncestor(this, widget, rect.topLeft()));
	int left = rect.left();
	int rightEnd = rect.left() + rect.width();
	int top = rect.top();
	int bottomEnd = rect.top() + rect.height();
	
	int x = pos.x();
	int y = pos.y();
	
	if (left <= x && x < rightEnd)
	{
		if (isInInsertDist(top, y))
		{
			insertDir = insertionFromAbsoluteDir(Top);
			return true;
		}
		
		if (isInInsertDist(bottomEnd, y))
		{
			insertDir = insertionFromAbsoluteDir(Bottom);
			return true;
		}
	}
	
	if (top <= y && y < bottomEnd)
	{
		if (isInInsertDist(left, x))
		{
			insertDir = insertionFromAbsoluteDir(Left);
			return true;
		}
		
		if (isInInsertDist(rightEnd, x))
		{
			insertDir = insertionFromAbsoluteDir(Right);
			return true;
		}
	}
	
	return false;
}

QRect DockTabMotherWidget::splittersRect(Direction dir)
{
	QRect rect;
	for (QSplitter *splitter : _splitterLists[dir])
		rect |= splitter->geometry();
	
	return rect;
}

bool DockTabMotherWidget::dropDockTab(DockTabWidget *srcTabWidget, int srcIndex, const QPoint &pos)
{
	TabWidgetArea area = dropArea(pos);
	
	if (!area.isValid())
		return false;
	
	auto dstTabWidget = srcTabWidget->createNewTabWidget();
	
	bool inserted = insertTabWidget(dstTabWidget, area);
	if (inserted)
		srcTabWidget->moveTab(srcIndex, dstTabWidget, 0);
	
	return inserted;
}

void DockTabMotherWidget::onTabWidgetWillBeDeleted(DockTabWidget *widget)
{
	for (QSplitterList &splitters : _splitterLists)
	{
		for (QSplitter *splitter : splitters)
		{
			if (splitter->count() == 1 && splitter->widget(0) == widget)
			{
				splitters.removeAll(splitter);
				splitter->deleteLater();
			}
		}
	}
}

QSplitter *DockTabMotherWidget::createSplitter(Qt::Orientation orientation)
{
	auto splitter = new QSplitter(orientation);
	splitter->setChildrenCollapsible(false);
	return splitter;
}

}

