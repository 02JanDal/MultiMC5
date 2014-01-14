#pragma once

#include "SyncInterface.h"

class SyncGit : public SyncInterface
{
	Q_OBJECT
public:
	SyncGit(BaseInstance *instance, QObject *parent = 0);

	QWidget *getConfigWidget();
	void applySettings(const QWidget *widget);
	QString key() const { return "Git"; }

	void addRootEntity(EntityBase *entity);
	void removeRootEntity(EntityBase *entity);
	QList<EntityBase *> getRootEntities();

	BaseVersionList *getVersionList(const EntityBase *entity);

	Task *push(const EntityBase *entity);
	Task *setVersion(const EntityBase *entity, const SyncVersion &version);
	Task *pull(const EntityBase *entity);
};
