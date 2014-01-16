#pragma once

#include "logic/tasks/Task.h"

#include <QUrl>
#include <memory>

#include "SyncInterface.h"

class SyncDropbox;
class QNetworkReply;

namespace SyncDropboxTasks
{
class BaseTask : public Task
{
	Q_OBJECT
public:
	BaseTask(const QUrl &endpoint, bool authenticationNeeded, int operation, const QByteArray &data, SyncDropbox *parent = 0);
	BaseTask(const QUrl &endpoint, bool authenticationNeeded, int operation, SyncDropbox *parent = 0);

protected:
	virtual void process(const QByteArray &data);
	virtual void process(const QJsonDocument &doc)
	{
		Q_UNUSED(doc);
		emitSucceeded();
	}
	virtual void executeTask();

	QUrl resolvedWithType(const EntityBase *entity, const QString &instance, const QUrl &base);
	QUrl resolvedWithType(const EntityBase::Type type, const QString &instance, const QUrl &base);

private
slots:
	void downloadFinished();
	void fail();

protected:
	SyncDropbox *m_parent;
	QUrl m_endpoint;
	bool m_auth;
	int m_op;
	QByteArray m_data;
	QNetworkReply *m_reply;
};
class ConnectAccountTask : public Task
{
	Q_OBJECT
public:
	ConnectAccountTask(QWidget *widget_parent, SyncDropbox *parent = 0);

protected:
	void executeTask();

private
slots:
	void downloadFinished();
	void fail();

private:
	QWidget *m_widgetParent;
	SyncDropbox *m_parent;
	QNetworkReply *m_reply;
};
class UserInfoTask : public BaseTask
{
	Q_OBJECT
public:
	UserInfoTask(SyncDropbox *parent = 0);

protected:
	void process(const QJsonDocument &doc);
};
class DisconnectAccountTask : public BaseTask
{
	Q_OBJECT
public:
	DisconnectAccountTask(SyncDropbox *parent = 0);

protected:
	void process(const QJsonDocument &doc);
};
class RestoreTask : public BaseTask
{
	Q_OBJECT
public:
	RestoreTask(const EntityBase *entity, const SyncVersionPtr version, SyncDropbox *parent = 0);
};
class PutFileTask : public BaseTask
{
	Q_OBJECT
public:
	PutFileTask(const QString &in, const QString &remote, const EntityBase *entity, SyncDropbox *parent);

protected:
	void executeTask();

private:
	QString m_in;
};
class GetFileTask : public BaseTask
{
	Q_OBJECT
public:
	GetFileTask(const QString &out, const QString &remote, const EntityBase *entity, SyncDropbox *parent);

protected:
	void process(const QByteArray &data);

private:
	QString m_out;
	int m_triesLeft;
};
class GetRootEntitiesTask : public BaseTask
{
	Q_OBJECT
public:
	GetRootEntitiesTask(const EntityBase::Type type, SyncDropbox *parent);

protected:
	void process(const QJsonDocument &doc);

private:
	EntityBase::Type m_type;
};
}
