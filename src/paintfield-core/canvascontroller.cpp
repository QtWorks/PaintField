#include <QtGui>

#include "application.h"
#include "toolmanager.h"
#include "documentio.h"
#include "workspacecontroller.h"
#include "module.h"

#include "dialogs/exportdialog.h"
#include "dialogs/newdocumentdialog.h"
#include "canvasview.h"

#include "canvascontroller.h"

namespace PaintField
{

CanvasController::CanvasController(Document *document, WorkspaceController *parent) :
    QObject(parent),
	_document(document)
{
	document->setParent(this);
	
	_actions << createAction("paintfield.file.save", this, SLOT(saveCanvas()));
	_actions << createAction("paintfield.file.saveAs", this, SLOT(saveAsCanvas()));
	_actions << createAction("paintfield.file.close", this, SLOT(closeCanvas()));
}

CanvasView *CanvasController::createView(QWidget *parent)
{
	_view = new CanvasView(_document, this, parent);
	
	connect(workspace()->toolManager(), SIGNAL(currentToolChanged(QString)), _view, SLOT(setTool(QString)));
	_view->setTool(workspace()->toolManager()->currentTool());
	
	return _view;
}

void CanvasController::addModules(const CanvasModuleList &modules)
{
	for (CanvasModule *module : modules)
		addActions(module->actions());
	_modules += modules;
}

CanvasController *CanvasController::fromNew(WorkspaceController *parent)
{
	NewDocumentDialog dialog;
	if (dialog.exec() != QDialog::Accepted)
		return 0;
	
	RasterLayer *layer = new RasterLayer(tr("Untitled Layer"));
	
	Document *document = new Document(tr("Untitled"), dialog.documentSize(), layer);
	return new CanvasController(document, parent);
}

CanvasController *CanvasController::fromOpen(WorkspaceController *parent)
{
	QString filePath = QFileDialog::getOpenFileName(0,
	                                                tr("Open"),
	                                                QDir::homePath(),
	                                                tr("PaintField Project (*.pfproj)"));
	if (filePath.isEmpty())	// cancelled
		return 0;
	
	DocumentIO documentIO(filePath);
	if (!documentIO.openUnzip())
	{
		QMessageBox::warning(0, tr("Failed to open file."), QString());
	}
	
	Document *document = documentIO.load(0);
	if (document == 0)
	{	// failed to open
		QMessageBox::warning(0, tr("Failed to open file."), QString());
	}
	
	return new CanvasController(document, parent);
}

bool CanvasController::saveAsCanvas()
{
	Document *document = this->document();
	
	QString filePath = QFileDialog::getSaveFileName(0,
	                                                tr("Save As"),
	                                                QDir::homePath(),
	                                                tr("PaintField Project (*.pfproj)"));
	if (filePath.isEmpty())
		return false;
	
	QFileInfo fileInfo(filePath);
	QFileInfo dirInfo(fileInfo.dir().path());
	if (!dirInfo.isWritable())
	{
		QMessageBox::warning(workspace()->view(), tr("The specified folder is not writable."), tr("Save in another folder."));
		return false;
	}
	
	DocumentIO documentIO(filePath);
	
	if (!documentIO.saveAs(document, filePath))
	{
		QMessageBox::warning(workspace()->view(), tr("Failed to save the file."), QString());
		return false;
	}
	return true;
}

bool CanvasController::saveCanvas()
{
	Document *document = this->document();
	
	if (document->filePath().isEmpty())	// first save
		return saveAsCanvas();
	
	if (!document->isModified())
		return true;
	
	DocumentIO documentIO(document->filePath());
	if (!documentIO.save(document))
	{
		QMessageBox::warning(workspace()->view(), tr("Cannot save file."), QString());
		return false;
	}
	return true;
}

bool CanvasController::closeCanvas()
{
	Document *document = this->document();
	
	if (document->isModified())
	{
		auto ret = QMessageBox::question(workspace()->view(),
										 tr("Do you want to save your changes?"),
										 tr("The changes will be lost if you don't save them."),
										 QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
										 QMessageBox::Save);
		
		switch (ret)
		{
			case QMessageBox::Save:
				if (!saveCanvas())
					return false;
				break;
			case QMessageBox::Discard:
				break;
			case QMessageBox::Cancel:
			default:
				return false;
		}
	}
	
	emit shouldBeDeleted();
	return true;
}

}
