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
	QString typeString() const;
};
typedef std::shared_ptr<SyncVersion> SyncVersionPtr;

class EntityBase
{
public:
	enum Type
	{
		InstanceFolder,
		Save,
		Configs
	};
	EntityBase(const Type type, const QString &path, SyncInterface *interface);
	virtual ~EntityBase();

	template<typename T> T getAs() const
	{
		return static_cast<T>(this);
	}
	template<typename T> bool is() const
	{
		return (bool) getAs<T>();
	}

	virtual void changeVersion(QWidget *widgetParent);

	Type type;
	static QString typeToString(const Type t);
	/// What's in here depends on the type. If the type is Save it's the name of a save for
	/// example.
	QString path;
	SyncInterface *interface;
	template<typename T> T getInterface() const
	{
		return qobject_cast<T>(interface);
	}
	virtual EntityBase *getChild(const QString &id) { return 0; }
	virtual QStringList keys() { return QStringList(); }
};
Q_DECLARE_METATYPE(EntityBase*)

class EntitySave : public EntityBase
{
public:
	EntitySave(const QString &name, SyncInterface *interface);
};
class EntityConfigs : public EntityBase
{
public:
	EntityConfigs(SyncInterface *interface);
};
class EntityInstanceFolder : public EntityBase
{
public:
	EntityInstanceFolder(SyncInterface *interface);
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
	virtual Task *setVersion(const EntityBase *entity, const SyncVersionPtr version) = 0;
	/// git fetch (+ pull if we are on the latest version)
	virtual Task *pull(const EntityBase *entity) = 0;

	BaseInstance *instance() const
	{
		return m_instance;
	}

protected:
	BaseInstance *m_instance;

signals:
	void rootEntitiesChanged();
};
