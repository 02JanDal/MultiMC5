#pragma once

#include <QThread>
#include <QPoint>

class QIODevice;

struct Instruction;
struct ScriptContents;

class Executer : public QThread
{
	Q_OBJECT
public:
	Executer();

	void run();

private:
	ScriptContents parse(QIODevice *device, bool *ok);
	void execute(const QList<Instruction *> &instructions);
	void execute(const Instruction *instruction);
	Qt::KeyboardModifiers parseModifiers(const QString &in);
	Qt::MouseButton parseButton(const QString &in);

signals:
	void failed();

	void outputMessage(const QString &msg);
	void outputError(const QString &msg);
	void outputWarning(const QString &msg);
	void outputCategory(const QString &msg);

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
