#include "SyncDropbox.h"

#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QMessageBox>
#include <QUrlQuery>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDir>
#include <QDebug>

#include "logic/tasks/Task.h"
#include "logic/tasks/SequentialTask.h"
#include "gui/dialogs/ProgressDialog.h"
#include "config.h"
#include "SyncDropboxTasks.h"

class SyncDropboxVersionList;

template<typename T>
void addTasksRecursive(SequentialTask *root, const QDir &dir, const QDir &rootDir, const EntityBase *entity, SyncDropbox *parent)
{
	foreach (const QFileInfo &info, QDir(dir).entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot))
	{
		if (info.isDir())
		{
			addTasksRecursive<T>(root, QDir(info.absoluteFilePath()), rootDir, entity, parent);
		}
		else
		{
			root->addTask(std::shared_ptr<Task>(
							  new T(info.absoluteFilePath(), rootDir.relativeFilePath(info.absoluteFilePath()), entity, parent)));
		}
	}
}

class SyncDropboxVersionListLoadTask : public SyncDropboxTasks::BaseTask
{
	Q_OBJECT
public:
	SyncDropboxVersionListLoadTask(const EntityBase *entity, SyncDropboxVersionList *list, SyncDropbox *parent)
		: SyncDropboxTasks::BaseTask(QUrl(), true, false, QByteArray(), parent), m_vlist(list), m_entity(entity)
	{
		QUrl url("https://api.dropbox.com/1/revisions/sandbox/");
		url = resolvedWithType(entity, entity->interface->instance()->id(), url);
		m_endpoint.swap(url);
	}

protected:
	void process(const QJsonDocument &doc);

private:
	SyncDropboxVersionList *m_vlist;
	const EntityBase *m_entity;
};

class SyncDropboxVersionList : public BaseVersionList
{
	Q_OBJECT
public:
	SyncDropboxVersionList(const EntityBase *entity, QObject *parent = 0) : BaseVersionList(parent), m_entity(entity)
	{
	}

	Task *getLoadTask()
	{
		return new SyncDropboxVersionListLoadTask(m_entity, this, m_entity->getInterface<SyncDropbox*>());
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
			return std::dynamic_pointer_cast<SyncVersion>(v1)->timestamp <
				   std::dynamic_pointer_cast<SyncVersion>(v2)->timestamp;
		});
	}

public
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
	const EntityBase *m_entity;

	bool m_loaded = false;
};

void SyncDropboxVersionListLoadTask::process(const QJsonDocument &doc)
{
	QJsonArray list = doc.array();
	QList<BaseVersionPtr> out;
	foreach (const QJsonValue &val, list)
	{
		QJsonObject obj = val.toObject();
		if (obj.value("is_deleted").toBool())
		{
			continue;
		}
		SyncVersion *version = new SyncVersion;
		version->timestamp = QDateTime::fromString(obj.value("modified").toString(), Qt::RFC2822Date);
		version->identifier = obj.value("rev").toString();
		out.append(SyncVersionPtr(version));
	}
	m_vlist->updateListData(out);
	emitSucceeded();
}

class SyncConfigWidget : public QWidget
{
	Q_OBJECT
public:
	SyncConfigWidget(SettingsObject *settings, SyncDropbox *sync, QWidget *parent = 0)
		: QWidget(parent)
	{
		m_toggleButton =
			new QPushButton(sync->isConnected() ? tr("Disconnect") : tr("Connect"), this);
		connect(sync, &SyncDropbox::connectedChanged, [this](const bool connected)
		{ m_toggleButton->setText(connected ? tr("Disconnect") : tr("Connect")); });
		connect(m_toggleButton, &QPushButton::clicked, [this,sync]()
		{
			if (sync->isConnected())
			{
				if (QMessageBox::question(
						this, tr("Dropbox"),
						tr("Do you really want to disable Dropbox access?")) ==
					QMessageBox::Yes)
				{
					ProgressDialog dialog(this);
					dialog.exec(sync->disconnectFromDropbox());
				}
			}
			else
			{
				ProgressDialog dialog(this);
				dialog.exec(sync->connectToDropbox(this));
			}
		});

		m_usernameLabel = new QLabel(sync->username(), this);
		connect(sync, &SyncDropbox::usernameChanged, m_usernameLabel, &QLabel::setText);
		connect(sync, &SyncDropbox::connectedChanged, m_usernameLabel, &QLabel::setVisible);

		QHBoxLayout *userinfoLayout = new QHBoxLayout;
		userinfoLayout->addWidget(new QLabel(tr("Name:"), this));
		userinfoLayout->addWidget(m_usernameLabel, 1);

		m_customClientBox = new QGroupBox(tr("Custom client key/secret"), this);
		m_customClientBox->setCheckable(true);
		m_customClientBox->setChecked(settings->get("DropboxCustomClient").toBool());

		m_customClientKey =
			new QLineEdit(settings->get("DropboxCustomClientKey").toString(), this);
		m_customClientSecret =
			new QLineEdit(settings->get("DropboxCustomClientSecret").toString(), this);
		QFormLayout *customBoxLayout = new QFormLayout;
		customBoxLayout->addRow(tr("Key"), m_customClientKey);
		customBoxLayout->addRow(tr("Secret"), m_customClientSecret);
		m_customClientBox->setLayout(customBoxLayout);

		QVBoxLayout *layout = new QVBoxLayout;
		layout->addWidget(m_toggleButton);
		layout->addLayout(userinfoLayout);
		layout->addWidget(m_customClientBox);
		layout->addStretch(1);
		setLayout(layout);
	}

	QPushButton *m_toggleButton;
	QLabel *m_usernameLabel;
	QGroupBox *m_customClientBox;
	QLineEdit *m_customClientKey;
	QLineEdit *m_customClientSecret;
};

SyncDropbox::SyncDropbox(BaseInstance *instance, QObject *parent)
	: SyncInterface(instance, parent)
{
	instance->settings().registerSetting("DropboxCustomClient", false);
	instance->settings().registerSetting("DropboxCustomClientKey", "");
	instance->settings().registerSetting("DropboxCustomClientSecret", "");
	instance->settings().registerSetting("DropboxAccessToken", "");

	setAccessToken(instance->settings().get("DropboxAccessToken").toString());

	connect(this, &SyncDropbox::connectedChanged, [this]()
	{
		SequentialTask *task = new SequentialTask(this);
		task->addTask(std::shared_ptr<Task>(new SyncDropboxTasks::GetRootEntitiesTask(EntityBase::Save, this)));
		task->addTask(std::shared_ptr<Task>(new SyncDropboxTasks::GetRootEntitiesTask(EntityBase::Configs, this)));
		task->addTask(std::shared_ptr<Task>(new SyncDropboxTasks::GetRootEntitiesTask(EntityBase::InstanceFolder, this)));
		task->start();
	});
}

QWidget *SyncDropbox::getConfigWidget()
{
	SyncConfigWidget *widget = new SyncConfigWidget(&m_instance->settings(), this);
	return widget;
}
void SyncDropbox::applySettings(const QWidget *widget)
{
}

void SyncDropbox::setRootEntities(const QList<EntityBase *> &entities)
{
	m_rootEntities = entities;
	emit rootEntitiesChanged();
}
void SyncDropbox::addRootEntity(EntityBase *entity)
{
	m_rootEntities.append(entity);
	emit rootEntitiesChanged();
}
void SyncDropbox::removeRootEntity(EntityBase *entity)
{
	m_rootEntities.removeAll(entity);
	emit rootEntitiesChanged();
}
QList<EntityBase *> SyncDropbox::getRootEntities()
{
	return QList<EntityBase *>();
}

BaseVersionList *SyncDropbox::getVersionList(const EntityBase *entity)
{
	return new SyncDropboxVersionList(entity);
}

Task *SyncDropbox::push(const EntityBase *entity)
{
	SequentialTask *task = new SequentialTask(this);
	QDir root;
	if (entity->type == EntityBase::Save)
	{
		root = QDir(m_instance->minecraftRoot() + "/saves/" + entity->path);
	}
	else if (entity->type == EntityBase::Configs)
	{
		root = QDir(m_instance->minecraftRoot() + "/config");
	}
	else if (entity->type == EntityBase::InstanceFolder)
	{
		root = QDir(m_instance->instanceRoot());
	}
	addTasksRecursive<SyncDropboxTasks::PutFileTask>(task, root, root, entity, this);
	return task;
}
Task *SyncDropbox::setVersion(const EntityBase *entity, const SyncVersionPtr version)
{
	return new SyncDropboxTasks::RestoreTask(entity, version, this);
}
Task *SyncDropbox::pull(const EntityBase *entity)
{
	SequentialTask *task = new SequentialTask(this);
	QDir root;
	if (entity->type == EntityBase::Save)
	{
		root = QDir(m_instance->minecraftRoot() + "/saves/" + entity->path);
	}
	else if (entity->type == EntityBase::Configs)
	{
		root = QDir(m_instance->minecraftRoot() + "/config");
	}
	else if (entity->type == EntityBase::InstanceFolder)
	{
		root = QDir(m_instance->instanceRoot());
	}
	addTasksRecursive<SyncDropboxTasks::GetFileTask>(task, root, root, entity, this);
	return task;
}

void SyncDropbox::setUsername(const QString &name)
{
	m_username = name;
	emit usernameChanged(m_username);
}

QString SyncDropbox::clientKey() const
{
	return m_instance->settings().get("DropboxCustomClientKey").toString().isEmpty()
			   ? QString(DROPBOX_CLIENT_KEY)
			   : m_instance->settings().get("DropboxCustomClientKey").toString();
}
QString SyncDropbox::clientSecret() const
{
	return m_instance->settings().get("DropboxCustomClientSecret").toString().isEmpty()
			   ? QString(DROPBOX_CLIENT_SECRET)
			   : m_instance->settings().get("DropboxCustomClientSecret").toString();
}

void SyncDropbox::setAccessToken(const QString &token)
{
	m_accessToken = token;
	m_instance->settings().set("DropboxAccessToken", m_accessToken);
	m_connected = !accessToken().isEmpty();
	emit connectedChanged(m_connected);
	if (m_connected)
	{
		auto userInfoTask = new SyncDropboxTasks::UserInfoTask(this);
		userInfoTask->start();
	}
}

Task *SyncDropbox::disconnectFromDropbox()
{
	return new SyncDropboxTasks::DisconnectAccountTask(this);
}
Task *SyncDropbox::connectToDropbox(QWidget *widget_parent)
{
	return new SyncDropboxTasks::ConnectAccountTask(widget_parent, this);
}

#include "SyncDropbox.moc"
