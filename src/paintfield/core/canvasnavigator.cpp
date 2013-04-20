#include <QKeySequence>
#include <QMouseEvent>
#include <QTabletEvent>
#include "tabletevent.h"
#include "canvas.h"
#include "keytracker.h"
#include "appcontroller.h"
#include "settingsmanager.h"
#include "cursorstack.h"
#include "canvasview.h"

#include "canvasnavigator.h"

namespace PaintField {

struct CanvasNavigator::Data
{
	Canvas *canvas = 0;
	CanvasViewController *controller = 0;
	KeyTracker *keyTracker = 0;
	
	QKeySequence scaleKeys, rotationKeys, translationKeys;
	
	DragMode navigationMode = NoNavigation;
	QPoint navigationOrigin;
	
	double backupScale = 1, backupRotation = 0;
	QPoint backupTranslation;
	
	void backupTransforms()
	{
		backupScale = canvas->scale();
		backupRotation = canvas->rotation();
		backupTranslation = canvas->translation();
	}
};

CanvasNavigator::CanvasNavigator(KeyTracker *keyTracker, CanvasViewController *controller) :
	QObject(controller),
	d(new Data)
{
	d->canvas = controller->canvas();
	d->controller = controller;
	d->keyTracker = keyTracker;
	
	connect(d->keyTracker, SIGNAL(pressedKeysChanged(QSet<int>)), this, SLOT(onPressedKeysChanged()));
	
	// setup key bindings
	{
		auto keyBindingHash = appController()->settingsManager()->settings()[".key-bindings"].toMap();
		
		d->translationKeys = keyBindingHash["paintfield.canvas.dragTranslation"].toString();
		d->scaleKeys = keyBindingHash["paintfield.canvas.dragScale"].toString();
		d->rotationKeys = keyBindingHash["paintfield.canvas.dragRotation"].toString();
	}
}

CanvasNavigator::~CanvasNavigator()
{
	delete d;
}

CanvasNavigator::DragMode CanvasNavigator::dragMode() const
{
	return d->navigationMode;
}

static const QString navigatingCursorId = "paintfield.canvas.navigate";

static const QString readyToTranslateCursorId = "paintfield.canvas.readyToTranslate";
static const QString readyToScaleCursorId = "paintfield.canvas.readyToScale";
static const QString readyToRotateCursorId = "paintfield.canvas.readyToRotate";

void CanvasNavigator::onPressedKeysChanged()
{
	PAINTFIELD_DEBUG;
	
	auto cs = appController()->cursorStack();
	auto kt = d->keyTracker;
	
	auto addOrRemove = [cs, kt](const QKeySequence &seq, const QString &id, const QCursor &cursor)
	{
		if (kt->match(seq))
			cs->add(id, cursor);
		else
			cs->remove(id);
	};
	
	addOrRemove(d->translationKeys, readyToTranslateCursorId, Qt::OpenHandCursor);
	addOrRemove(d->scaleKeys, readyToScaleCursorId, Qt::SizeVerCursor);
	addOrRemove(d->rotationKeys, readyToRotateCursorId, Qt::OpenHandCursor);
}

void CanvasNavigator::mouseEvent(QMouseEvent *event)
{
	event->ignore();
	
	switch (event->type())
	{
		case QEvent::MouseButtonPress:
			
			emit clicked();
			
			if (event->button() == Qt::LeftButton)
			{
				if (tryBeginDragNavigation(event->pos()))
					event->accept();
			}
			break;
			
		case QEvent::MouseMove:
			
			if (continueDragNavigation(event->pos()))
				event->accept();
			break;
			
		case QEvent::MouseButtonRelease:
			
			endDragNavigation();
			break;
			
		default:
			break;
	}
}

void CanvasNavigator::tabletEvent(QTabletEvent *event)
{
	event->ignore();
	
	switch (event->type())
	{
		case QEvent::TabletPress:
			
			emit clicked();
			if (tryBeginDragNavigation(event->pos()))
				event->accept();
			break;
			
		case QEvent::TabletMove:
			
			if (continueDragNavigation(event->pos()))
				event->accept();
			break;
			
		case QEvent::TabletRelease:
			
			endDragNavigation();
			break;
			
		default:
			break;
	}
}

void CanvasNavigator::customTabletEvent(WidgetTabletEvent *event)
{
	event->ignore();
	
	switch (int(event->type()))
	{
		case EventWidgetTabletPress:
			
			emit clicked();
			if (tryBeginDragNavigation(event->posInt))
				event->accept();
			break;
			
		case EventWidgetTabletMove:
			
			if (continueDragNavigation(event->posInt))
				event->accept();
			break;
			
		case EventWidgetTabletRelease:
			
			endDragNavigation();
			break;
			
		default:
			break;
	}
}

void CanvasNavigator::wheelEvent(QWheelEvent *event)
{
	QPoint translation = d->canvas->translation();
	
	if (event->orientation() == Qt::Horizontal)
		translation += QPoint(event->delta(), 0);
	else
		translation += QPoint(0, event->delta());
	
	d->canvas->setTranslation(translation);
}

bool CanvasNavigator::tryBeginDragNavigation(const QPoint &pos)
{
	if (d->keyTracker->match(d->scaleKeys))
	{
		beginDragScaling(pos);
		return true;
	}
	if (d->keyTracker->match(d->rotationKeys))
	{
		beginDragRotation(pos);
		return true;
	}
	 if (d->keyTracker->match(d->translationKeys))
	{
		beginDragTranslation(pos);
		return true;
	}
	
	return false;
}

bool CanvasNavigator::continueDragNavigation(const QPoint &pos)
{
	switch (d->navigationMode)
	{
		default:
		case NoNavigation:
			return false;
		case Translating:
			continueDragTranslation(pos);
			return true;
		case Scaling:
			continueDragScaling(pos);
			return true;
		case Rotating:
			continueDragRotation(pos);
			return true;
	}
}

void CanvasNavigator::endDragNavigation()
{
	endDragTranslation();
	endDragScaling();
	endDragRotation();
}

void CanvasNavigator::beginDragTranslation(const QPoint &pos)
{
	appController()->cursorStack()->add(navigatingCursorId, Qt::ClosedHandCursor);
	
	d->navigationMode = Translating;
	d->navigationOrigin = pos;
	
	d->backupTransforms();
}

void CanvasNavigator::continueDragTranslation(const QPoint &pos)
{
	d->canvas->setTranslation(d->backupTranslation + (pos - d->navigationOrigin));
}

void CanvasNavigator::endDragTranslation()
{
	appController()->cursorStack()->remove(navigatingCursorId);
	d->navigationMode = NoNavigation;
}

void CanvasNavigator::beginDragScaling(const QPoint &pos)
{
	appController()->cursorStack()->add(navigatingCursorId, Qt::SizeVerCursor);
	
	d->navigationMode = Scaling;
	d->navigationOrigin = pos;
	d->backupTransforms();
}

void CanvasNavigator::continueDragScaling(const QPoint &pos)
{
	auto delta = pos - d->navigationOrigin;
	
	constexpr double divisor = 100;
	
	
	double scaleRatio = exp2(-delta.y() / divisor);
	double scale = d->backupScale * scaleRatio;
	
	auto navigationOffset = d->navigationOrigin - d->controller->viewCenter();
	
	auto translation = (d->backupTranslation - navigationOffset) * scaleRatio + navigationOffset;
	
	d->canvas->setScale(scale);
	d->canvas->setTranslation(translation);
}

void CanvasNavigator::endDragScaling()
{
	appController()->cursorStack()->remove(navigatingCursorId);
	d->navigationMode = NoNavigation;
}

void CanvasNavigator::beginDragRotation(const QPoint &pos)
{
	appController()->cursorStack()->add(navigatingCursorId, Qt::ClosedHandCursor);
	
	d->navigationMode = Rotating;
	d->navigationOrigin = pos;
	d->backupTransforms();
}

void CanvasNavigator::continueDragRotation(const QPoint &pos)
{
	auto originalDelta = d->navigationOrigin - d->controller->viewCenter();
	auto delta = pos - d->controller->viewCenter();
	if (originalDelta != QPoint() && delta != QPoint())
	{
		auto originalRotation = atan2(originalDelta.y(), originalDelta.x()) / M_PI * 180.0;
		auto deltaRotation = atan2(delta.y(), delta.x()) / M_PI * 180.0;
		
		QTransform transform;
		transform.rotate(deltaRotation - originalRotation);
		
		auto translation = d->backupTranslation * transform;
		d->canvas->setRotation(d->backupRotation + deltaRotation - originalRotation);
		d->canvas->setTranslation(translation);
	}
}

void CanvasNavigator::endDragRotation()
{
	appController()->cursorStack()->remove(navigatingCursorId);
	d->navigationMode = NoNavigation;
}

} // namespace PaintField