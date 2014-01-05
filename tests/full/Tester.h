#pragma once

#include <QTest>

#include "TesterInterface.h"

class QTextStream;

class Executer;

class Tester : public QObject, public TesterInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID TesterInterface_iid)
	Q_INTERFACES(TesterInterface)
public:
	Tester();

	void init();
	void setup(QWindow *window);

private:
	Executer *m_executer;
	QTextStream out;
	QWindow *m_window;

private
slots:
	void failed();

	void outputMessage(const QString &msg);
	void outputError(const QString &msg);
	void outputWarning(const QString &msg);
	void outputCategory(const QString &msg);
	void outputMessageImpl(const QString &msg, const QtMsgType type);

	void keyClick(unsigned char key, Qt::KeyboardModifiers modifiers);
	void keyClicks(const QString &sequence, Qt::KeyboardModifiers modifiers);
	void keyPress(unsigned char key, Qt::KeyboardModifiers modifiers);
	void keyRelease(unsigned char key, Qt::KeyboardModifiers modifiers);

	void mouseClick(Qt::MouseButton button, Qt::KeyboardModifiers stateKey, QPoint pos);
	void mouseDClick(Qt::MouseButton button, Qt::KeyboardModifiers stateKey, QPoint pos);
	void mousePress(Qt::MouseButton button, Qt::KeyboardModifiers stateKey, QPoint pos);
	void mouseMove(Qt::MouseButton button, Qt::KeyboardModifiers stateKey, QPoint pos);
	void mouseRelease(Qt::MouseButton button, Qt::KeyboardModifiers stateKey, QPoint pos);

	void resizeWindow(const QString &name, const QSize &newSize);
	void moveWindow(const QString &name, const QPoint &pos);
};
