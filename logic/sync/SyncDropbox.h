#pragma once

#include "SyncInterface.h"

class SyncDropbox : public SyncInterface
{
	Q_OBJECT
	Q_PROPERTY(QString username READ username NOTIFY usernameChanged)
	Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
public:
	SyncDropbox(BaseInstance *instance, QObject *parent = 0);

	QWidget *getConfigWidget();
	void applySettings(const QWidget *widget);
	QString key() const { return "Dropbox"; }

	void addRootEntity(EntityBase *entity);
	void removeRootEntity(EntityBase *entity);
	QList<EntityBase *> getRootEntities();

	BaseVersionList *getVersionList(const EntityBase *entity);

	Task *push(const EntityBase *entity);
	Task *setVersion(const EntityBase *entity, const SyncVersion &version);
	Task *pull(const EntityBase *entity);

	void setUsername(const QString &name);
	QString username() const
	{
		return m_username;
	}
	bool isConnected() const
	{
		return m_connected;
	}

	QString clientKey() const;
	QString clientSecret() const;

	QString accessToken() const
	{
		return m_accessToken;
	}
	void setAccessToken(const QString &token);

public
slots:
	Task *disconnectFromDropbox();
	Task *connectToDropbox(QWidget *widget_parent = 0);

signals:
	void usernameChanged(const QString &username);
	void connectedChanged(bool isConnected);

private:
	QString m_username;
	bool m_connected;
	QString m_accessToken;
};
