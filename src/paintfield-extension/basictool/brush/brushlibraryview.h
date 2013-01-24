#ifndef PAINTFIELD_BRUSH_BRUSHLIBRARYVIEW_H
#define PAINTFIELD_BRUSH_BRUSHLIBRARYVIEW_H

#include <QWidget>

class QItemSelectionModel;
class QModelIndex;

namespace PaintField {

class BrushLibraryModel;

class BrushLibraryView : public QWidget
{
	Q_OBJECT
public:
	BrushLibraryView(BrushLibraryModel *model, QItemSelectionModel *selectionModel, QWidget *parent = 0);
	
signals:
	
	void saveRequested();
	void reloadRequested();
	
public slots:
	
private:
	
	
};

} // namespace PaintField

#endif // PAINTFIELD_BRUSH_BRUSHLIBRARYVIEW_H