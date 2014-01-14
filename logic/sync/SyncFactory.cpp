#include "SyncFactory.h"

#include "config.h"

#include "SyncGit.h"
#ifdef MultiMC_ENABLE_DROPBOX
#include "SyncDropbox.h"
#endif

QStringList SyncFactory::keys()
{
	return QStringList() << "Git"
#ifdef MultiMC_ENABLE_DROPBOX
						 << "Dropbox"
#endif
		;
}

SyncInterface *SyncFactory::create(const QString &key, BaseInstance *instance, QObject *parent)
{
	if (key.toLower() == "git")
	{
		return new SyncGit(instance, parent);
	}
#ifdef MultiMC_ENABLE_DROPBOX
	else if (key.toLower() == "dropbox")
	{
		return new SyncDropbox(instance, parent);
	}
#endif
	else
	{
		return 0;
	}
}
