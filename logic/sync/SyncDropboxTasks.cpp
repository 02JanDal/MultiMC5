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

#include "MultiMC.h"
#include "SyncDropbox.h"

namespace SyncDropboxTasks
{

BaseTask::BaseTask(const QUrl &endpoint, bool authenticationNeeded, bool post,
				   const QByteArray &data, SyncDropbox *parent)
	: Task(parent), m_parent(parent), m_endpoint(endpoint), m_auth(authenticationNeeded), m_post(post)
{
}
void BaseTask::downloadFinished()
{
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(m_reply->readAll(), &error);
	if (error.error != QJsonParseError::NoError)
	{
		emitFailed(tr("JSON reply parse error: %1").arg(error.errorString()));
		return;
	}
	process(doc);
}
void BaseTask::executeTask()
{
	QNetworkRequest req(m_endpoint);

	if (m_auth)
	{
		req.setRawHeader("Authorization", "Bearer " + m_parent->accessToken().toLocal8Bit());
	}

	m_reply = m_post ? MMC->qnam()->post(req, m_data) : MMC->qnam()->get(req);
	connect(m_reply, &QNetworkReply::finished, this, &BaseTask::downloadFinished);
	connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(fail()));
	connect(m_reply, &QNetworkReply::downloadProgress, this, [this](qint64 current, qint64 max)
	{
		setProgress(current * 100 / max);
	});
}
void BaseTask::fail()
{
	emitFailed(tr("Network error: %1").arg(m_reply->errorString()));
}

DisconnectAccountTask::DisconnectAccountTask(SyncDropbox *parent)
	: BaseTask(QUrl("https://api.dropbox.com/1/disable_access_token"), true, true, QByteArray(),
			   parent)
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
	m_reply = MMC->qnam()->post(QNetworkRequest(QUrl("https://api.dropbox.com/1/oauth2/token")), tokenQuery.toString(QUrl::FullyEncoded).toLocal8Bit());
	connect(m_reply, &QNetworkReply::finished, this, &ConnectAccountTask::downloadFinished);
	connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(fail()));
	connect(m_reply, &QNetworkReply::downloadProgress, this, [this](qint64 current, qint64 max)
	{
		setProgress(current * 100 / max);
	});
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
	: BaseTask(QUrl("https://api.dropbox.com/1/account/info"), true, false, QByteArray(), parent)
{
}
void UserInfoTask::process(const QJsonDocument &doc)
{
	QJsonObject obj = doc.object();
	m_parent->setUsername(obj.value("display_name").toString());
	emitSucceeded();
}

}
