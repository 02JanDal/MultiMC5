#include "SyncGit.h"

#include "logic/tasks/Task.h"

class SyncGitVersionListLoadTask : public Task
{
	Q_OBJECT
public:
	SyncGitVersionListLoadTask(BaseVersionList *list, QObject *parent)
		: Task(parent), m_vlist(list)
	{
	}

protected:
	void executeTask()
	{

	}

private:
	BaseVersionList *m_vlist;
};

class SyncGitVersionList : public BaseVersionList
{
	Q_OBJECT
public:
	SyncGitVersionList(QObject *parent = 0)
		: BaseVersionList(parent)
	{
	}

	Task *getLoadTask()
	{
		return new SyncGitVersionListLoadTask(this, this);
	}
	bool isLoaded()
	{
		return m_loaded;
	}
	const BaseVersionPtr at(int i) const
	{
		return m_versions.at(i);
	}
	int count() const
	{
		return m_versions.count();
	}
	void sort()
	{
		beginResetModel();
		qSort(m_versions.begin(), m_versions.end(), [](BaseVersionPtr v1, BaseVersionPtr v2)
		{
			return std::dynamic_pointer_cast<SyncVersion>(v1)->timestamp < std::dynamic_pointer_cast<SyncVersion>(v2)->timestamp;
		});
	}

protected
slots:
	void updateListData(QList<BaseVersionPtr> versions)
	{
		beginResetModel();
		m_versions.swap(versions);
		endResetModel();
		sort();
	}

private:
	QList<BaseVersionPtr> m_versions;

	bool m_loaded = false;
};

SyncGit::SyncGit(BaseInstance *instance, QObject *parent)
	: SyncInterface(instance, parent)
{

}

QWidget *SyncGit::getConfigWidget()
{
	return 0;
}
void SyncGit::applySettings(const QWidget *widget)
{

}

void SyncGit::addRootEntity(EntityBase *entity)
{
}
void SyncGit::removeRootEntity(EntityBase *entity)
{
}
QList<EntityBase *> SyncGit::getRootEntities()
{
	return QList<EntityBase *>();
}

BaseVersionList *SyncGit::getVersionList(const EntityBase *entity)
{
	return new SyncGitVersionList();
}

Task *SyncGit::push(const EntityBase *entity)
{
	return 0;
}
Task *SyncGit::setVersion(const EntityBase *entity, const SyncVersionPtr version)
{
	return 0;
}
Task *SyncGit::pull(const EntityBase *entity)
{
	return 0;
}

#include "SyncGit.moc"
