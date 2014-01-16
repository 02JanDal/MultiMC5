#include "SyncInterface.h"

#include "gui/dialogs/VersionSelectDialog.h"
#include "gui/dialogs/ProgressDialog.h"
#include "logic/tasks/Task.h"

QString SyncVersion::descriptor()
{
	return identifier;
}
QString SyncVersion::name()
{
	return timestamp.toString("ddd MMM d yyyy HH:mm:ss");
}
QString SyncVersion::typeString() const
{
	return QString();
}

EntityBase::EntityBase(const EntityBase::Type type, const QString &path, SyncInterface *interface)
	: type(type), path(path), interface(interface)
{

}
EntityBase::~EntityBase()
{

}

void EntityBase::changeVersion(QWidget *widgetParent)
{
	VersionSelectDialog dialog(interface->getVersionList(this), QObject::tr("Select sync version"), widgetParent, true);
	if (dialog.exec() == QDialog::Rejected)
	{
		return;
	}
	ProgressDialog progDialog(widgetParent);
	progDialog.exec(interface->setVersion(this, std::dynamic_pointer_cast<SyncVersion>(dialog.selectedVersion())));
}

QString EntityBase::typeToString(const EntityBase::Type t)
{
	switch (t)
	{
	case InstanceFolder: return QObject::tr("InstanceFolder");
	case Save: return QObject::tr("Save");
	case Configs: return QObject::tr("Configs");
	default: return QString();
	}
}

SyncInterface::SyncInterface(BaseInstance *instance, QObject *parent)
	: QObject(parent), m_instance(instance)
{
}
SyncInterface::~SyncInterface()
{

}


EntitySave::EntitySave(const QString &name, SyncInterface *interface)
	: EntityBase(EntityBase::Save, name, interface)
{
}
EntityConfigs::EntityConfigs(SyncInterface *interface)
	: EntityBase(EntityBase::Configs, QString(), interface)
{
}
EntityInstanceFolder::EntityInstanceFolder(SyncInterface *interface)
	: EntityBase(EntityBase::InstanceFolder, QString(), interface)
{
}
