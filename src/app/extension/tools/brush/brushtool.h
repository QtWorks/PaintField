#ifndef FSBRUSHTOOL_H
#define FSBRUSHTOOL_H

#include <QObject>
#include "core/tool.h"
#include "brushsetting.h"

namespace PaintField
{

class Stroker;
class TabletInputData;

class BrushTool : public Tool
{
	Q_OBJECT
public:
	
	explicit BrushTool(CanvasView *parent = 0);
	~BrushTool();
	
	void drawLayer(Malachite::SurfacePainter *painter, const Layer *layer);
	
	void setBrushSetting(const BrushSetting *setting) { _brushSetting = setting; }
	const BrushSetting *brushSetting() const { return _brushSetting; }
	
signals:
	
public slots:
	
protected:
	
	void cursorPressEvent(TabletEvent *event);
	void cursorMoveEvent(TabletEvent *event);
	void cursorReleaseEvent(TabletEvent *event);
	
	bool canvasEventFilter(QEvent *event);
	
	void beginStroke(const TabletInputData &data);
	void drawStroke(const TabletInputData &data);
	void endStroke(const TabletInputData &data);
	
	void updateTiles();
	
private:
	
	void setPrevData(const TabletInputData &data);
	
	QScopedPointer<Stroker> _stroker;
	TabletInputData _dataPrev, _dataBeforePrev;
	bool _dataPrevSet = false, _trailing = false;
	bool _trailingEnabled = false;
	const BrushSetting *_brushSetting = 0;
	const Layer *_layer = 0;
	Malachite::Surface _surface;
};

class BrushToolFactory : public ToolFactory
{
	Q_OBJECT
public:
	explicit BrushToolFactory(QObject *parent = 0);
	
	Tool *createTool(CanvasView *parent);
	
	bool isTypeSupported(Layer::Type type) const;
	
	BrushSetting *setting() { return &_setting; }
	const BrushSetting *setting() const { return &_setting; }
	
signals:
	
public slots:
	
private:
	BrushSetting _setting;
};

}

#endif // FSBRUSHTOOL_H
