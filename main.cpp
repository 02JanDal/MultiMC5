#include "MultiMC.h"
#include "gui/MainWindow.h"

int main_gui(MultiMC &app)
{
	// show main window
	MainWindow mainWin;
	mainWin.restoreState(QByteArray::fromBase64(MMC->settings()->get("MainWindowState").toByteArray()));
	mainWin.restoreGeometry(QByteArray::fromBase64(MMC->settings()->get("MainWindowGeometry").toByteArray()));
	mainWin.show();
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

	Q_INIT_RESOURCE(generated);
	Q_INIT_RESOURCE(graphics);

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
