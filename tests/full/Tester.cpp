#include "Tester.h"

#include <QWidget>
#include <QApplication>

#include <qtestkeyboard.h>
#include <qtestmouse.h>

#include <QTextStream>
#include <cstdio>

#include "Executer.h"

Tester::Tester() : out(stdout)
{
}

void Tester::init()
{
	outputCategory("TESTER LOADED");
	outputMessage("Do not attempt to manually control MultiMC");
	m_executer = new Executer;

	connect(m_executer, &Executer::failed, this, &Tester::failed);

	connect(m_executer, &Executer::outputMessage, this, &Tester::outputMessage);
	connect(m_executer, &Executer::outputError, this, &Tester::outputError);
	connect(m_executer, &Executer::outputWarning, this, &Tester::outputWarning);
	connect(m_executer, &Executer::outputCategory, this, &Tester::outputCategory);

	connect(m_executer, &Executer::keyClick, this, &Tester::keyClick);
	connect(m_executer, &Executer::keyClicks, this, &Tester::keyClicks);
	connect(m_executer, &Executer::keyPress, this, &Tester::keyPress);
	connect(m_executer, &Executer::keyRelease, this, &Tester::keyRelease);

	connect(m_executer, &Executer::mouseClick, this, &Tester::mouseClick);
	connect(m_executer, &Executer::mouseDClick, this, &Tester::mouseDClick);
	connect(m_executer, &Executer::mousePress, this, &Tester::mousePress);
	connect(m_executer, &Executer::mouseMove, this, &Tester::mouseMove);
	connect(m_executer, &Executer::mouseRelease, this, &Tester::mouseRelease);

	connect(m_executer, &Executer::resizeWindow, this, &Tester::resizeWindow);
	connect(m_executer, &Executer::moveWindow, this, &Tester::moveWindow);
}

void Tester::setup(QWidget *window)
{
	m_window = window;
	m_executer->start();
}

void Tester::failed()
{
	abort();
}

void Tester::outputMessage(const QString &msg)
{
	outputMessageImpl(msg, QtDebugMsg);
}
void Tester::outputError(const QString &msg)
{
	outputMessageImpl("Error: " + msg, QtCriticalMsg);
}
void Tester::outputWarning(const QString &msg)
{
	outputMessageImpl("Warning: " + msg, QtWarningMsg);
}
void Tester::outputCategory(const QString &msg)
{
	int firstSize = ceil((double)(80 - 2 - msg.size()) / 2.0);
	int lastSize = firstSize;
	while ((firstSize + lastSize + 2 + msg.size()) > 80)
	{
		lastSize -= 1;
	}
	outputMessageImpl(QString().fill('#', firstSize) + " " + msg + " " +
					  QString().fill('#', lastSize), QtDebugMsg);
}
void Tester::outputMessageImpl(const QString &msg, const QtMsgType type)
{
	out << msg << endl << flush;
}

void Tester::keyClick(unsigned char key, Qt::KeyboardModifiers modifiers)
{
	QTest::keyClick(m_window, key, modifiers);
}
void Tester::keyClicks(const QString &sequence, Qt::KeyboardModifiers modifiers)
{
	for (int i = 0; i < sequence.size(); ++i)
	{
		keyClick(sequence.at(i).toLatin1(), modifiers);
	}
}
void Tester::keyPress(unsigned char key, Qt::KeyboardModifiers modifiers)
{
	QTest::keyPress(m_window, key, modifiers);
}
void Tester::keyRelease(unsigned char key, Qt::KeyboardModifiers modifiers)
{
	QTest::keyRelease(m_window, key, modifiers);
}

void Tester::mouseClick(Qt::MouseButton button, Qt::KeyboardModifiers stateKey, QPoint pos)
{
	QWidget *widget = qApp->topLevelAt(pos);
	QTest::mouseClick(widget, button, stateKey, widget->mapFromGlobal(pos));
}
void Tester::mouseDClick(Qt::MouseButton button, Qt::KeyboardModifiers stateKey, QPoint pos)
{
	QWidget *widget = qApp->topLevelAt(pos);
	QTest::mouseDClick(widget, button, stateKey, widget->mapFromGlobal(pos));
}
void Tester::mousePress(Qt::MouseButton button, Qt::KeyboardModifiers stateKey, QPoint pos)
{
	QWidget *widget = qApp->topLevelAt(pos);
	QTest::mousePress(widget, button, stateKey, widget->mapFromGlobal(pos));
}
void Tester::mouseMove(Qt::MouseButton button, Qt::KeyboardModifiers stateKey, QPoint pos)
{
	QWidget *widget = qApp->topLevelAt(pos);
	QTest::mouseMove(widget, widget->mapFromGlobal(pos));
}
void Tester::mouseRelease(Qt::MouseButton button, Qt::KeyboardModifiers stateKey, QPoint pos)
{
	QWidget *widget = qApp->topLevelAt(pos);
	QTest::mouseRelease(widget, button, stateKey, widget->mapFromGlobal(pos));
}

void Tester::resizeWindow(const QString &name, const QSize &newSize)
{
	QWidget *window;
	if (name.isEmpty())
	{
		window = m_window;
	}
	else
	{
		window = m_window->findChild<QWidget *>(name);
	}
	if (window)
	{
		window->resize(newSize);
	}
}
void Tester::moveWindow(const QString &name, const QPoint &pos)
{
	QWidget *window;
	if (name.isEmpty())
	{
		window = m_window;
	}
	else
	{
		window = m_window->findChild<QWidget *>(name);
	}
	if (window)
	{
		window->move(pos);
	}
}
