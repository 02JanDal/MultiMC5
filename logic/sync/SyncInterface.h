#pragma once

#include <QObject>

#include "logic/BaseVersion.h"
#include "logic/BaseInstance.h"

class SyncVersion : public BaseVersion
{
public:
	QString identifier;
	QDateTime timestamp;

	QString descriptor();
	QString name();
	QString typeString();
};
typedef std::shared_ptr<SyncVersion> SyncVersinPtr;

class EntityBase
{
public:
	enum Type
	{
		InstanceFolder,
		Save,
		Configs
	};
	EntityBase(const Type type, const QString &path);
	virtual ~EntityBase();

	Type type;
	/// What's in here depends on the type. If the type is Save it's the name of a save for
	/// example.
	QString path;
	virtual EntityBase *getChild(const QString &id) = 0;
};

class SyncInterface : public QObject
{
	Q_OBJECT
public:
	SyncInterface(BaseInstance *instance, QObject *parent = 0);
	virtual ~SyncInterface();

	virtual QWidget *getConfigWidget() = 0;
	virtual void applySettings(const QWidget *widget) = 0;
	virtual QString key() const = 0;

	virtual void addRootEntity(EntityBase *entity) = 0;
	virtual void removeRootEntity(EntityBase *entity) = 0;
	virtual QList<EntityBase *> getRootEntities() = 0;

	/// Used when rolling back files
	virtual BaseVersionList *getVersionList(const EntityBase *entity) = 0;

	/// git add/commit/push
	virtual Task *push(const EntityBase *entity) = 0;
	/// git checkout <version>
	virtual Task *setVersion(const EntityBase *entity, const SyncVersion &version) = 0;
	/// git fetch (+ pull if we are on the latest version)
	virtual Task *pull(const EntityBase *entity) = 0;

protected:
	BaseInstance *m_instance;
};
