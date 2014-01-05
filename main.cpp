#include "MultiMC.h"
#include "gui/MainWindow.h"
#include "config.h"

#ifdef MultiMC_FULL_TEST
#include <QPluginLoader>
#include "tests/full/TesterInterface.h"
#endif

int main_gui(MultiMC &app)
{
#ifdef MultiMC_FULL_TEST
	inject("FullTest");
#endif

	// show main window
	MainWindow mainWin;
#ifdef MultiMC_FULL_TEST
	interface->setup(mainWin.windowHandle());
#endif
	mainWin.restoreState(QByteArray::fromBase64(MMC->settings()->get("MainWindowState").toByteArray()));
	mainWin.restoreGeometry(QByteArray::fromBase64(MMC->settings()->get("MainWindowGeometry").toByteArray()));
	mainWin.show();
	mainWin.checkMigrateLegacyAssets();
	mainWin.checkSetDefaultJava();
	auto exitCode = app.exec();

	// Update if necessary.
	if (!app.getExitUpdatePath().isEmpty())
		app.installUpdates(app.getExitUpdatePath(), false);

	return exitCode;
}

int main(int argc, char *argv[])
{
	// initialize Qt
	MultiMC app(argc, argv);

	Q_INIT_RESOURCE(graphics);
	Q_INIT_RESOURCE(generated);

	switch (app.status())
	{
	case MultiMC::Initialized:
		return main_gui(app);
	case MultiMC::Failed:
		return 1;
	case MultiMC::Succeeded:
		return 0;
	}
}
