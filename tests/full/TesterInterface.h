#pragma once

#define TesterInterface_iid "org.multimc.Test.FullTest.TesterInterface"

#include <QtPlugin>

class Tester;
class QWindow;

class TesterInterface
{
public:
	virtual ~TesterInterface()
	{
	}

	virtual void init() = 0;
	virtual void setup(QWidget *window) = 0;
};

Q_DECLARE_INTERFACE(TesterInterface, TesterInterface_iid)

#define inject(NAME) \
QPluginLoader loader; \
loader.setFileName(  NAME  ); \
if (!loader.load()) \
{ \
	qFatal("Couldn't load " #NAME " plugin %s: %s", qPrintable(loader.fileName()), \
		   qPrintable(loader.errorString())); \
} \
TesterInterface *interface = qobject_cast<TesterInterface *>(loader.instance()); \
if (!interface) \
{ \
	qFatal("Error fetching the TesterInterface"); \
} \
interface->init();
