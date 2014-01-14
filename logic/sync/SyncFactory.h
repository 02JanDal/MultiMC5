#pragma once

#include "SyncInterface.h"

class SyncFactory
{
public:
	static QStringList keys();
	static SyncInterface *create(const QString &key, BaseInstance *instance, QObject *parent = 0);
	static void registerGlobalSettings();
};
