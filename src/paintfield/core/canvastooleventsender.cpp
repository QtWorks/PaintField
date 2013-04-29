#include <QMouseEvent>
#include <QTabletEvent>
#include <QPointer>
#include <Malachite/Vec2D>
#include "tabletevent.h"
#include "tool.h"
#include "canvas.h"

#include "canvastooleventsender.h"

using namespace Malachite;

namespace PaintField {

struct CanvasToolEventSender::Data
{
	QPointer<Tool> tool;
	Canvas *canvas;
	CanvasViewController *controller;
	
	double mousePressure = 0;
};

CanvasToolEventSender::CanvasToolEventSender(CanvasViewController *controller) :
	QObject(controller),
	d(new Data)
{
	d->canvas = controller->canvas();
	d->controller = controller;
}

CanvasToolEventSender::~CanvasToolEventSender()
{
	delete d;
}

void CanvasToolEventSender::setTool(Tool *tool)
{
	d->tool = tool;
}

void CanvasToolEventSender::keyEvent(QKeyEvent *event)
{
	if (d->tool)
		d->tool->toolEvent(event);
}

void CanvasToolEventSender::mouseEvent(QMouseEvent *event)
{
	switch (event->type())
	{
		case QEvent::MouseButtonDblClick:
			event->setAccepted(sendCanvasMouseEvent(event));
			break;
		case QEvent::MouseButtonPress:
		case QEvent::MouseButtonRelease:
		case QEvent::MouseMove:
			event->setAccepted(sendCanvasTabletEvent(event) || sendCanvasMouseEvent(event));
			break;
		default:
			event->ignore();
			break;
	}
}

void CanvasToolEventSender::tabletEvent(QTabletEvent *event)
{
	auto toNewEventType = [](QEvent::Type type)
	{
		switch (type)
		{
			default:
			case QEvent::TabletMove:
				return PaintField::EventWidgetTabletMove;
			case QEvent::TabletPress:
				return PaintField::EventWidgetTabletPress;
			case QEvent::TabletRelease:
				return PaintField::EventWidgetTabletRelease;
		}
	};
	
	TabletInputData data(event->hiResGlobalPos(), event->pressure(), event->rotation(), event->tangentialPressure(), Vec2D(event->xTilt(), event->yTilt()));
	WidgetTabletEvent widgetTabletEvent(toNewEventType(event->type()), event->globalPos(), event->pos(), data, event->modifiers());
	
	customTabletEvent(&widgetTabletEvent);
	event->setAccepted(widgetTabletEvent.isAccepted());
}

void CanvasToolEventSender::customTabletEvent(WidgetTabletEvent *event)
{
	event->setAccepted(sendCanvasTabletEvent(event));
}

bool CanvasToolEventSender::sendCanvasMouseEvent(QMouseEvent *event)
{
	if (!d->tool)
		return false;
	
	auto toCanvasEventType = [](QEvent::Type type)
	{
		switch (type)
		{
			default:
			case QEvent::MouseMove:
				return EventCanvasMouseMove;
			case QEvent::MouseButtonPress:
				return EventCanvasMousePress;
			case QEvent::MouseButtonRelease:
				return EventCanvasMouseRelease;
			case QEvent::MouseButtonDblClick:
				return EventCanvasMouseDoubleClick;
		}
	};
	
	CanvasMouseEvent canvasEvent(toCanvasEventType(event->type()), event->globalPos(), event->pos(), d->canvas->transformToScene() * event->posF(), event->modifiers());
	d->tool->toolEvent(&canvasEvent);
	
	return canvasEvent.isAccepted();
}

bool CanvasToolEventSender::sendCanvasTabletEvent(WidgetTabletEvent *event)
{
	if (!d->tool)
		return false;
	
	TabletInputData data = event->globalData;
	Vec2D globalPos = data.pos;
	Vec2D viewPos = globalPos + Vec2D(event->posInt - event->globalPosInt);
	data.pos = d->canvas->transformToScene() * viewPos;
	
	auto toCanvasEventType = [](int type)
	{
		switch (type)
		{
			default:
			case EventWidgetTabletMove:
				return EventCanvasTabletMove;
			case EventWidgetTabletPress:
				return EventCanvasTabletPress;
			case EventWidgetTabletRelease:
				return EventCanvasTabletRelease;
		}
	};
	
	CanvasTabletEvent canvasEvent(toCanvasEventType(event->type()), globalPos, event->globalPosInt, viewPos, event->posInt, data, event->modifiers());
	d->tool->toolEvent(&canvasEvent);
	
	return canvasEvent.isAccepted();
}

bool CanvasToolEventSender::sendCanvasTabletEvent(QMouseEvent *mouseEvent)
{
	if (!d->tool)
		return false;
	
	auto toCanvasEventType = [](QEvent::Type type)
	{
		switch (type)
		{
			default:
			case QEvent::MouseMove:
				return EventCanvasTabletMove;
			case QEvent::MouseButtonPress:
				return EventCanvasTabletPress;
			case QEvent::MouseButtonRelease:
				return EventCanvasTabletRelease;
		}
	};
	
	int type = toCanvasEventType(mouseEvent->type());
	
	if (type == EventCanvasTabletPress)
		d->mousePressure = 1.0;
	if (type == EventCanvasTabletRelease)
		d->mousePressure = 0.0;
	
	TabletInputData data(d->canvas->transformToScene() * mouseEvent->posF(), d->mousePressure, 0, 0, Vec2D(0));
	CanvasTabletEvent tabletEvent(type, mouseEvent->globalPos(), mouseEvent->globalPos(), mouseEvent->pos(), mouseEvent->pos(), data, mouseEvent->modifiers());
	d->tool->toolEvent(&tabletEvent);
	return tabletEvent.isAccepted();
}

} // namespace PaintField
