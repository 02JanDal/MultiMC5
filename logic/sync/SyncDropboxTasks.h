#pragma once

#include "logic/tasks/Task.h"

#include <QUrl>
#include <memory>

class SyncDropbox;
class QNetworkReply;

namespace SyncDropboxTasks
{
class BaseTask : public Task
{
	Q_OBJECT
public:
	BaseTask(const QUrl &endpoint, bool authenticationNeeded, bool post, const QByteArray &data, SyncDropbox *parent = 0);

protected:
	virtual void process(const QJsonDocument &doc)
	{
		Q_UNUSED(doc);
		emitSucceeded();
	}
	virtual void executeTask();

private
slots:
	void downloadFinished();
	void fail();

protected:
	SyncDropbox *m_parent;
	QUrl m_endpoint;
	bool m_auth;
	bool m_post;
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
}
