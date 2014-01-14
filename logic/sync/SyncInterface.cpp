#include "SyncInterface.h"

QString SyncVersion::descriptor()
{
	return identifier;
}
QString SyncVersion::name()
{
	return timestamp.toString("ddd MMM d yyyy HH:mm:ss");
}
QString SyncVersion::typeString()
{
	return QString();
}

EntityBase::EntityBase(const EntityBase::Type type, const QString &path)
	: type(type), path(path)
{

}
EntityBase::~EntityBase()
{

}

SyncInterface::SyncInterface(BaseInstance *instance, QObject *parent)
	: QObject(parent), m_instance(instance)
{
}
SyncInterface::~SyncInterface()
{

}
