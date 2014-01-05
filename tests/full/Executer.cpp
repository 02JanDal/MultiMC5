#include "Executer.h"

#include <QPoint>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>
#include <QSize>

struct Instruction
{
	enum Type
	{
		Sleep,
		Message,
		Warning,
		Category,
		KeyClick,
		KeyClicks,
		KeyPress,
		KeyRelease,
		MouseClick,
		MouseDClick,
		MousePress,
		MouseMove,
		MouseRelease,
		ResizeWindow,
		MoveWindow
	} type;
	Instruction(Type type) : type(type)
	{
	}
};
struct InstructionSleep : Instruction
{
	InstructionSleep(int msecs) : Instruction(Sleep), msecs(msecs)
	{
	}
	int msecs;
};
struct InstructionMessage : Instruction
{
	InstructionMessage(Type type, const QString &msg) : Instruction(type), msg(msg)
	{
	}
	QString msg;
};
struct InstructionKeyEvent : Instruction
{
	InstructionKeyEvent(Type type, const char key, Qt::KeyboardModifiers modifiers)
		: Instruction(type), key(key), modifiers(modifiers)
	{
	}
	char key;
	Qt::KeyboardModifiers modifiers;
};
struct InstructionKeyClicks : Instruction
{
	InstructionKeyClicks(const QString &sequence, Qt::KeyboardModifiers modifiers)
		: Instruction(KeyClicks), sequence(sequence), modifiers(modifiers)
	{
	}
	QString sequence;
	Qt::KeyboardModifiers modifiers;
};
struct InstructionMouseEvent : Instruction
{
	InstructionMouseEvent(Type type, Qt::MouseButton button, Qt::KeyboardModifiers modifiers,
						  QPoint pos)
		: Instruction(type), button(button), stateKeys(modifiers), pos(pos)
	{
	}
	Qt::MouseButton button;
	Qt::KeyboardModifiers stateKeys;
	QPoint pos;
};
struct InstructionResize : Instruction
{
	InstructionResize(const QString &name, const QSize &size)
		: Instruction(ResizeWindow), name(name), size(size)
	{
	}
	QString name;
	QSize size;
};
struct InstructionMove : Instruction
{
	InstructionMove(const QString &name, const QPoint &to)
		: Instruction(MoveWindow), name(name), to(to)
	{
	}
	QString name;
	QPoint to;
};

struct ScriptContents
{
	QString name;
	QList<Instruction *> instructions;
};

Executer::Executer()
{
}

void Executer::run()
{
	QDir scriptsDir(qApp->applicationDirPath() + "/tests/data/scripts");
	foreach(const QFileInfo & fInfo,
			scriptsDir.entryInfoList(QStringList() << "*.test", QDir::Files))
	{
		QFile file(fInfo.absoluteFilePath());
		if (!file.open(QFile::ReadOnly))
		{
			outputWarning(
				QString("Couldn't open %1: %2").arg(file.fileName(), file.errorString()));
		}
		bool ok = true;
		ScriptContents contents = parse(&file, &ok);
		if (ok)
		{
			if (!qEnvironmentVariableIsSet("MULTIMC_PARSE_ONLY"))
			{
				emit outputCategory("START " + contents.name + " START");
				execute(contents.instructions);
				emit outputCategory("END " + contents.name + " END");
			}
		}
		else
		{
			outputWarning(QString("While parsing %1").arg(file.fileName()));
			failed();
			break;
		}
	}
}

QStringList split(const QString &in)
{
	QStringList out;
	int index = 0;
	while (true)
	{
		int start = index == 0 ? 0 : in.indexOf(' ', index);
		if (start == -1)
		{
			break;
		}
		int end = in.indexOf(' ', start + 1);
		QString string = in.mid(start, end - start);
		if (string.startsWith('"'))
		{
			end = in.indexOf('"', start + 1);
			string = in.mid(start, end - start);
		}
		out.append(string.remove('"').trimmed());
		index = end;
	}
	return out;
}
Qt::KeyboardModifiers Executer::parseModifiers(const QString &in)
{
	QStringList modifiers = in.split('|');
	Qt::KeyboardModifiers out;
	foreach(const QString & modifier, modifiers)
	{
		if (modifier == "SHIFT")
		{
			out |= Qt::ShiftModifier;
		}
		else if (modifier == "CONTROL" || modifier == "CNTRL" || modifier == "CTRL")
		{
			out |= Qt::ControlModifier;
		}
		else if (modifier == "ALT")
		{
			out |= Qt::AltModifier;
		}
		else if (modifier == "META")
		{
			out |= Qt::MetaModifier;
		}
		else
		{
			outputWarning(tr("Unknown modifier: %1").arg(modifier));
		}
	}
	return out;
}
Qt::MouseButton Executer::parseButton(const QString &in)
{
	if (in == "LEFT" || in == "L")
	{
		return Qt::LeftButton;
	}
	else if (in == "MID" || in == "MIDDLE" || in == "M")
	{
		return Qt::MidButton;
	}
	else if (in == "RIGHT" || in == "R")
	{
		return Qt::RightButton;
	}
	else
	{
		return Qt::NoButton;
	}
}

ScriptContents Executer::parse(QIODevice *device, bool *ok)
{
	ScriptContents contents;
	int line = 0;
	QStringList lines = QString::fromUtf8(device->readAll()).split('\n');
	while (!lines.isEmpty())
	{
		line++;
		QString line = lines.takeFirst().remove(QRegExp("#.*")).trimmed();
		if (line.isEmpty())
		{
			continue;
		}
		QStringList parts = split(line);
		if (parts.size() < 2)
		{
			outputError(QString("To few arguments on line %1").arg(line));
			*ok = false;
			return contents;
		}
		const QString cmd = parts.takeFirst();
		if (cmd == "NAME")
		{
			contents.name = parts.join(' ');
		}
		else if (cmd == "MESSAGE")
		{
			contents.instructions.append(
				new InstructionMessage(Instruction::Message, parts.join(' ')));
		}
		else if (cmd == "WARNING")
		{
			contents.instructions.append(
				new InstructionMessage(Instruction::Warning, parts.join(' ')));
		}
		else if (cmd == "CATEGORY")
		{
			contents.instructions.append(
				new InstructionMessage(Instruction::Category, parts.join(' ')));
		}
		else if (cmd == "SLEEP")
		{
			int msec;
			if (parts.first().endsWith("ms"))
			{
				msec = parts.first().remove("ms").toInt(ok);
			}
			else if (parts.first().endsWith("s"))
			{
				msec = parts.first().remove("s").toInt(ok) * 1000;
			}
			else
			{
				msec = parts.first().toInt(ok);
			}
			if (!(*ok))
			{
				emit outputError(QString("Error parsing line %1: Cannot parse time").arg(line));
				return contents;
			}
			contents.instructions.append(new InstructionSleep(msec));
		}
		else if (cmd == "KEY")
		{
			const QString subcmd = parts.takeFirst();
			if (parts.isEmpty())
			{
				outputError(QString("To few arguments on line %1").arg(line));
				*ok = false;
				return contents;
			}
			const QString argument = parts.takeFirst();
			Qt::KeyboardModifiers modifiers =
				parseModifiers(parts.isEmpty() ? "" : parts.takeFirst());
			if (subcmd == "SEQUENCE")
			{
				contents.instructions.append(new InstructionKeyClicks(argument, modifiers));
			}
			else
			{
				const char key = argument.at(0).toLatin1();
				if (subcmd == "CLICK")
				{
					contents.instructions.append(
						new InstructionKeyEvent(Instruction::KeyClick, key, modifiers));
				}
				else if (subcmd == "PRESS")
				{
					contents.instructions.append(
						new InstructionKeyEvent(Instruction::KeyPress, key, modifiers));
				}
				else if (subcmd == "RELEASE")
				{
					contents.instructions.append(
						new InstructionKeyEvent(Instruction::KeyRelease, key, modifiers));
				}
				else
				{
					outputError(QString("Unknown subcommand for KEY on line %1: %2")
									.arg(line)
									.arg(subcmd));
					(*ok) = false;
					return contents;
				}
			}
		}
		else if (cmd == "MOUSE")
		{
			const QString subcmd = parts.takeFirst();
			if (parts.size() < 3)
			{
				outputError(QString("To few arguments on line %1").arg(line));
				*ok = false;
				return contents;
			}
			Qt::MouseButton button = parseButton(parts.takeFirst());
			int x = parts.takeFirst().toInt(ok);
			if (!(*ok))
			{
				outputError(QString("Couldn't parse X value on line %1").arg(line));
				return contents;
			}
			int y = parts.takeFirst().toInt(ok);
			if (!(*ok))
			{
				outputError(QString("Couldn't parse Y value on line %1").arg(line));
				return contents;
			}
			QPoint pos(x, y);
			Qt::KeyboardModifiers modifiers =
				parseModifiers(parts.isEmpty() ? "" : parts.takeFirst());
			if (subcmd == "CLICK")
			{
				contents.instructions.append(
					new InstructionMouseEvent(Instruction::MouseClick, button, modifiers, pos));
			}
			else if (subcmd == "DCLICK" || subcmd == "DOUBLECLICK")
			{
				contents.instructions.append(new InstructionMouseEvent(Instruction::MouseDClick,
																	   button, modifiers, pos));
			}
			else if (subcmd == "PRESS")
			{
				contents.instructions.append(
					new InstructionMouseEvent(Instruction::MousePress, button, modifiers, pos));
			}
			else if (subcmd == "MOVE")
			{
				contents.instructions.append(
					new InstructionMouseEvent(Instruction::MouseMove, button, modifiers, pos));
			}
			else if (subcmd == "Release")
			{
				contents.instructions.append(new InstructionMouseEvent(
					Instruction::MouseRelease, button, modifiers, pos));
			}
		}
		else if (cmd == "RESIZE")
		{
			QString name;
			int width;
			int height;
			if (parts.first().at(0).isNumber())
			{
				if (parts.size() < 2)
				{
					outputError(QString("To few arguments on line %1").arg(line));
					*ok = false;
					return contents;
				}
				width = parts.takeFirst().toInt(ok);
				if (!(*ok))
				{
					outputError(QString("Couldn't parse width on line %1").arg(line));
					return contents;
				}
				height = parts.takeFirst().toInt(ok);
				if (!(*ok))
				{
					outputError(QString("Couldn't parse height on line %1").arg(line));
					return contents;
				}
			}
			else
			{
				if (parts.size() < 3)
				{
					outputError(QString("To few arguments on line %1").arg(line));
					*ok = false;
					return contents;
				}
				name = parts.takeFirst();
				width = parts.takeFirst().toInt(ok);
				if (!(*ok))
				{
					outputError(QString("Couldn't parse width on line %1").arg(line));
					return contents;
				}
				height = parts.takeFirst().toInt(ok);
				if (!(*ok))
				{
					outputError(QString("Couldn't parse height on line %1").arg(line));
					return contents;
				}
			}
			contents.instructions.append(new InstructionResize(name, QSize(width, height)));
		}
		else if (cmd == "MOVE")
		{
			QString name;
			int x;
			int y;
			if (parts.first().at(0).isNumber())
			{
				if (parts.size() < 2)
				{
					outputError(QString("To few arguments on line %1").arg(line));
					*ok = false;
					return contents;
				}
				x = parts.takeFirst().toInt(ok);
				if (!(*ok))
				{
					outputError(QString("Couldn't parse X value on line %1").arg(line));
					return contents;
				}
				y = parts.takeFirst().toInt(ok);
				if (!(*ok))
				{
					outputError(QString("Couldn't parse Y value on line %1").arg(line));
					return contents;
				}
			}
			else
			{
				if (parts.size() < 3)
				{
					outputError(QString("To few arguments on line %1").arg(line));
					*ok = false;
					return contents;
				}
				name = parts.takeFirst();
				x = parts.takeFirst().toInt(ok);
				if (!(*ok))
				{
					outputError(QString("Couldn't parse X value on line %1").arg(line));
					return contents;
				}
				y = parts.takeFirst().toInt(ok);
				if (!(*ok))
				{
					outputError(QString("Couldn't parse Y value on line %1").arg(line));
					return contents;
				}
			}
			contents.instructions.append(new InstructionMove(name, QPoint(x, y)));
		}
		else
		{
			*ok = false;
			emit outputError(QString("Unknown command at line %1: %2").arg(line).arg(cmd));
			return contents;
		}
	}
	if (contents.name.isEmpty())
	{
		*ok = false;
		emit outputError(QString("No name specified"));
	}
	return contents;
}

void Executer::execute(const QList<Instruction *> &instructions)
{
	for (auto it = instructions.cbegin(); it != instructions.cend(); ++it)
	{
		execute(*it);
	}
}
void Executer::execute(const Instruction *instruction)
{
	switch (instruction->type)
	{
	case Instruction::Sleep:
	{
		const InstructionSleep *sleep = static_cast<const InstructionSleep *>(instruction);
		QThread::msleep(sleep->msecs);
		break;
	}
	case Instruction::Message:
	case Instruction::Warning:
	case Instruction::Category:
	{
		const InstructionMessage *message =
			static_cast<const InstructionMessage *>(instruction);
		switch (message->type)
		{
		case InstructionMessage::Message:
			outputMessage(message->msg);
			break;
		case InstructionMessage::Warning:
			outputWarning(message->msg);
			break;
		case InstructionMessage::Category:
			outputCategory(message->msg);
			break;
		}
	}
	case Instruction::KeyClicks:
	{
		const InstructionKeyClicks *clicks =
			static_cast<const InstructionKeyClicks *>(instruction);
		keyClicks(clicks->sequence, clicks->modifiers);
		break;
	}
	case Instruction::KeyClick:
	case Instruction::KeyPress:
	case Instruction::KeyRelease:
	{
		const InstructionKeyEvent *keyEvent =
			static_cast<const InstructionKeyEvent *>(instruction);
		switch (keyEvent->type)
		{
		case InstructionKeyEvent::KeyClick:
			keyClick(keyEvent->key, keyEvent->modifiers);
			break;
		case InstructionKeyEvent::KeyPress:
			keyPress(keyEvent->key, keyEvent->modifiers);
			break;
		case InstructionKeyEvent::KeyRelease:
			keyRelease(keyEvent->key, keyEvent->modifiers);
		}
		break;
	}
	case Instruction::MouseClick:
	case Instruction::MouseDClick:
	case Instruction::MousePress:
	case Instruction::MouseMove:
	case Instruction::MouseRelease:
	{
		const InstructionMouseEvent *mouseEvent =
			static_cast<const InstructionMouseEvent *>(instruction);
		switch (mouseEvent->type)
		{
		case InstructionMouseEvent::MouseClick:
			mouseClick(mouseEvent->button, mouseEvent->stateKeys, mouseEvent->pos);
			break;
		case InstructionMouseEvent::MouseDClick:
			mouseDClick(mouseEvent->button, mouseEvent->stateKeys, mouseEvent->pos);
			break;
		case InstructionMouseEvent::MousePress:
			mousePress(mouseEvent->button, mouseEvent->stateKeys, mouseEvent->pos);
			break;
		case InstructionMouseEvent::MouseMove:
			mouseMove(mouseEvent->button, mouseEvent->stateKeys, mouseEvent->pos);
			break;
		case InstructionMouseEvent::MouseRelease:
			mouseRelease(mouseEvent->button, mouseEvent->stateKeys, mouseEvent->pos);
			break;
		}
		break;
	}
	case Instruction::ResizeWindow:
	{
		const InstructionResize *resizeEvent =
			static_cast<const InstructionResize *>(instruction);
		resizeWindow(resizeEvent->name, resizeEvent->size);
		break;
	}
	case Instruction::MoveWindow:
	{
		const InstructionMove *moveEvent = static_cast<const InstructionMove *>(instruction);
		moveWindow(moveEvent->name, moveEvent->to);
		break;
	}
	}
}
