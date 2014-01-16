#include "SyncDropboxTasks.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QDesktopServices>
#include <QInputDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDebug>
#include <QFile>

#include "MultiMC.h"
#include "SyncDropbox.h"

namespace SyncDropboxTasks
{

BaseTask::BaseTask(const QUrl &endpoint, bool authenticationNeeded, int operation,
				   const QByteArray &data, SyncDropbox *parent)
	: Task(parent), m_parent(parent), m_endpoint(endpoint), m_auth(authenticationNeeded),
	  m_op(operation)
{
}
BaseTask::BaseTask(const QUrl &endpoint, bool authenticationNeeded, int operation,
				   SyncDropbox *parent)
	: BaseTask(endpoint, authenticationNeeded, operation, QByteArray(), parent)
{
}

void BaseTask::process(const QByteArray &data)
{
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(data, &error);
	if (error.error != QJsonParseError::NoError)
	{
		emitFailed(tr("JSON reply parse error: %1").arg(error.errorString()));
		return;
	}
	process(doc);
}

void BaseTask::downloadFinished()
{
	process(m_reply->readAll());
}
void BaseTask::executeTask()
{
	QNetworkRequest req(m_endpoint);

	if (m_auth)
	{
		req.setRawHeader("Authorization", "Bearer " + m_parent->accessToken().toLocal8Bit());
	}

	switch (m_op)
	{
	case QNetworkAccessManager::HeadOperation:
		m_reply = MMC->qnam()->head(req);
		break;
	case QNetworkAccessManager::GetOperation:
		m_reply = MMC->qnam()->get(req);
		break;
	case QNetworkAccessManager::PutOperation:
		m_reply = MMC->qnam()->put(req, m_data);
		break;
	case QNetworkAccessManager::PostOperation:
		m_reply = MMC->qnam()->post(req, m_data);
		break;
	case QNetworkAccessManager::DeleteOperation:
		m_reply = MMC->qnam()->deleteResource(req);
		break;
	default:
		Q_ASSERT_X(false, Q_FUNC_INFO, "unknown/unsupported network operation");
		return;
	}

	if (m_op == QNetworkAccessManager::PostOperation)
	{
		req.setHeader(QNetworkRequest::ContentLengthHeader, m_data.size());
	}

	connect(m_reply, &QNetworkReply::finished, this, &BaseTask::downloadFinished);
	connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(fail()));
	connect(m_reply, &QNetworkReply::downloadProgress, this, [this](qint64 current, qint64 max)
	{ setProgress(current * 100 / max); });
}

QUrl BaseTask::resolvedWithType(const EntityBase *entity, const QString &instance,
								const QUrl &base)
{
	if (entity->type == EntityBase::Save)
	{
		return resolvedWithType(entity->type, instance, base).resolved(entity->path + "/");
	}
	return resolvedWithType(entity->type, instance, base);
}
QUrl BaseTask::resolvedWithType(const EntityBase::Type type, const QString &instance, const QUrl &base)
{
	if (type == EntityBase::Save)
	{
		return base.resolved(QUrl("saves/" + instance + "/"));
	}
	else if (type == EntityBase::Configs)
	{
		return base.resolved(QUrl("configs/" + instance + "/"));
	}
	else if (type == EntityBase::InstanceFolder)
	{
		return base.resolved(QUrl("instances/" + instance + "/"));
	}
	return base;
}
void BaseTask::fail()
{
	emitFailed(tr("Network error: %1").arg(m_reply->errorString()));
}

DisconnectAccountTask::DisconnectAccountTask(SyncDropbox *parent)
	: BaseTask(QUrl("https://api.dropbox.com/1/disable_access_token"), true,
			   QNetworkAccessManager::PostOperation, parent)
{
}
void DisconnectAccountTask::process(const QJsonDocument &doc)
{
	m_parent->setAccessToken(QString());
	emitSucceeded();
}

ConnectAccountTask::ConnectAccountTask(QWidget *widget_parent, SyncDropbox *parent)
	: Task(parent), m_widgetParent(widget_parent), m_parent(parent)
{
}
void ConnectAccountTask::executeTask()
{
	emit progress(0, 0);

	QUrlQuery query;
	query.addQueryItem("client_id", m_parent->clientKey());
	query.addQueryItem("response_type", "code");
	QUrl url("https://www.dropbox.com/1/oauth2/authorize");
	url.setQuery(query);
	QDesktopServices::openUrl(url);

	bool ok = false;
	const QString code = QInputDialog::getText(
		m_widgetParent, tr("Dropbox"), tr("Enter the code given by the Dropbox website:"),
		QLineEdit::Normal, QString(), &ok);
	if (!ok)
	{
		emitSucceeded();
		return;
	}

	QUrlQuery tokenQuery;
	tokenQuery.addQueryItem("code", code);
	tokenQuery.addQueryItem("grant_type", "authorization_code");
	tokenQuery.addQueryItem("client_id", m_parent->clientKey());
	tokenQuery.addQueryItem("client_secret", m_parent->clientSecret());
	m_reply = MMC->qnam()->post(QNetworkRequest(QUrl("https://api.dropbox.com/1/oauth2/token")),
								tokenQuery.toString(QUrl::FullyEncoded).toLocal8Bit());
	connect(m_reply, &QNetworkReply::finished, this, &ConnectAccountTask::downloadFinished);
	connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(fail()));
	connect(m_reply, &QNetworkReply::downloadProgress, this, [this](qint64 current, qint64 max)
	{ setProgress(current * 100 / max); });
}
void ConnectAccountTask::downloadFinished()
{
	QJsonDocument doc = QJsonDocument::fromJson(m_reply->readAll());
	QJsonObject obj = doc.object();
	if (m_reply->error() != QNetworkReply::NoError)
	{
		emitFailed(obj.value("error_description").toString());
		return;
	}
	m_parent->setAccessToken(obj.value("access_token").toString());
	emitSucceeded();
}
void ConnectAccountTask::fail()
{
}

UserInfoTask::UserInfoTask(SyncDropbox *parent)
	: BaseTask(QUrl("https://api.dropbox.com/1/account/info"), true,
			   QNetworkAccessManager::GetOperation, parent)
{
}
void UserInfoTask::process(const QJsonDocument &doc)
{
	QJsonObject obj = doc.object();
	m_parent->setUsername(obj.value("display_name").toString());
	emitSucceeded();
}

RestoreTask::RestoreTask(const EntityBase *entity, const SyncVersionPtr version,
						 SyncDropbox *parent)
	: BaseTask(resolvedWithType(entity, parent->instance()->id(),
								QUrl("https://api.dropbox.com/1/restore/sandbox/")),
			   true, QNetworkAccessManager::PostOperation, parent)
{
	m_data = "rev=" + version->identifier.toLocal8Bit();
}

PutFileTask::PutFileTask(const QString &in, const QString &remote, const EntityBase *entity,
						 SyncDropbox *parent)
	: BaseTask(resolvedWithType(entity, parent->instance()->id(),
								QUrl("https://api-content.dropbox.com/1/files_put/sandbox/"))
			   .resolved(QUrl(remote)),
			   true, QNetworkAccessManager::PutOperation, parent),
	  m_in(in)
{
}

void PutFileTask::executeTask()
{
	QFile file(m_in);
	if (!file.open(QFile::ReadOnly))
	{
		emitFailed(tr("Error accessing %1: %2").arg(file.fileName(), file.errorString()));
		return;
	}
	m_data = file.readAll();
	BaseTask::executeTask();
}

GetFileTask::GetFileTask(const QString &out, const QString &remote, const EntityBase *entity, SyncDropbox *parent)
	: BaseTask(resolvedWithType(entity, parent->instance()->id(),
								QUrl("https://api-content.dropbox.com/1/files/sandbox/"))
			   .resolved(QUrl(remote)),
			   true, QNetworkAccessManager::GetOperation, parent),
	  m_out(out), m_triesLeft(3)
{
}
void GetFileTask::process(const QByteArray &data)
{
	if (!m_reply->header(QNetworkRequest::ContentLengthHeader).isNull())
	{
		if (m_reply->header(QNetworkRequest::ContentLengthHeader).toInt() != data.size())
		{
			if (m_triesLeft == 0)
			{
				emitFailed(tr("Failed to get %1 3 times. Aborting").arg(m_out));
				return;
			}
			else
			{
				QLOG_INFO() << "Failed to get" << m_out << "." << m_triesLeft << " tries left. Retrying...";
				m_triesLeft--;
				executeTask();
				return;
			}
		}
	}
	QFile file(m_out);
	if (!file.open(QFile::WriteOnly))
	{
		emitFailed(tr("Error accessing %1: %2").arg(file.fileName(), file.errorString()));
		return;
	}
	file.write(data);
	file.close();
	emitSucceeded();
}

GetRootEntitiesTask::GetRootEntitiesTask(const EntityBase::Type type, SyncDropbox *parent)
	: BaseTask(resolvedWithType(type, parent->instance()->id(),
								QUrl("https://api.dropbox.com/1/metadata/sandbox/")),
			   true, QNetworkAccessManager::GetOperation, parent),
	  m_type(type)
{
}

void GetRootEntitiesTask::process(const QJsonDocument &doc)
{
	QJsonArray array = doc.object().value("contents").toArray();
	QList<EntityBase *> entities;
	foreach (const QJsonValue &val, array)
	{
		QJsonObject obj = val.toObject();
		if (!obj.value("is_dir").toBool())
		{
			continue;
		}
		switch (m_type)
		{
		case EntityBase::Save:
		{
			QString name = obj.value("path").toString();
			name = name.mid(name.lastIndexOf('/') + 1);
			m_parent->addRootEntity(new EntitySave(name, m_parent));
			break;
		}
		case EntityBase::Configs:
			m_parent->addRootEntity(new EntityConfigs(m_parent));
			break;
		case EntityBase::InstanceFolder:
			m_parent->addRootEntity(new EntityInstanceFolder(m_parent));
			break;
		default:
			continue;
		}
	}
	emitSucceeded();
}

}
