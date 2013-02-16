#include <QToolBar>
#include <QtPlugin>
#include "paintfield/core/workspace.h"
#include "paintfield/core/appcontroller.h"
#include "paintfield/core/toolmanager.h"
#include "tooluiextension.h"

namespace PaintField
{

static const QString _toolToolbarName = "paintfield.toolbar.tool";

ToolUIExtension::ToolUIExtension(Workspace *workspace, QObject *parent) :
	WorkspaceExtension(workspace, parent)
{
}

void ToolUIExtension::updateToolBar(QToolBar *toolBar, const QString &name)
{
	if (name == _toolToolbarName)
	{
		toolBar->clear();
		toolBar->addActions(workspace()->toolManager()->actions());
	}
}

void ToolUIExtensionFactory::initialize(AppController *app)
{
	app->settingsManager()->declareToolbar(_toolToolbarName, ToolBarInfo(QObject::tr("Tools")));
	//app->declareSidebar(_toolSettingSidebarName, SidebarInfo(tr("Tool Settings")));
}

}

Q_EXPORT_PLUGIN2(paintfield-toolui, PaintField::ToolUIExtensionFactory)
