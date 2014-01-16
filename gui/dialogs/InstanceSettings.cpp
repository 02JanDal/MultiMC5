/* Copyright 2013 MultiMC Contributors
 *
 * Authors: Andrew Okin
 *          Peterix
 *          Orochimarufan <orochimarufan.x3@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "MultiMC.h"
#include "InstanceSettings.h"
#include "ui_InstanceSettings.h"
#include "gui/Platform.h"
#include "gui/dialogs/VersionSelectDialog.h"
#include "gui/dialogs/ProgressDialog.h"

#include "logic/JavaUtils.h"
#include "logic/NagUtils.h"
#include "logic/lists/JavaVersionList.h"
#include "logic/JavaChecker.h"

#include "logic/sync/SyncFactory.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>

InstanceSettings::InstanceSettings(SettingsObject *obj, BaseInstance *instance, QWidget *parent)
	: QDialog(parent), ui(new Ui::InstanceSettings), m_obj(obj), m_instance(instance), m_syncSettingsWidget(0)
{
	MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);

	restoreGeometry(QByteArray::fromBase64(MMC->settings()->get("SettingsGeometry").toByteArray()));

	if (instance->sync())
	{
		connect(instance->sync(), &SyncInterface::rootEntitiesChanged, this, &InstanceSettings::syncEntriesChanged);
		syncEntriesChanged();
	}

	loadSettings();
}

InstanceSettings::~InstanceSettings()
{
	delete ui;
}

void InstanceSettings::showEvent(QShowEvent *ev)
{
	QDialog::showEvent(ev);
}

void InstanceSettings::closeEvent(QCloseEvent *ev)
{
	MMC->settings()->set("SettingsGeometry", saveGeometry().toBase64());

	QDialog::closeEvent(ev);
}

void InstanceSettings::on_customCommandsGroupBox_toggled(bool state)
{
	ui->labelCustomCmdsDescription->setEnabled(state);
}

void InstanceSettings::on_buttonBox_accepted()
{
	MMC->settings()->set("SettingsGeometry", saveGeometry().toBase64());

	applySettings();
	accept();
}

void InstanceSettings::on_buttonBox_rejected()
{
	MMC->settings()->set("SettingsGeometry", saveGeometry().toBase64());

	reject();
}

void InstanceSettings::on_syncBox_currentTextChanged(const QString &text)
{
	bool isEnabled = m_obj->get("Sync").toString() == text;
	ui->syncToggleBtn->setText(isEnabled ? tr("Disable") : tr("Enable"));
	if (m_syncSettingsWidget)
	{
		ui->syncSettingsBox->layout()->removeWidget(m_syncSettingsWidget);
		m_syncSettingsWidget->deleteLater();
		m_syncSettingsWidget = 0;
	}
	if (isEnabled && m_instance->sync())
	{
		m_syncSettingsWidget = m_instance->sync()->getConfigWidget();
		if (m_syncSettingsWidget)
		{
			ui->syncSettingsBox->layout()->addWidget(m_syncSettingsWidget);
		}
	}
}
void InstanceSettings::on_syncToggleBtn_clicked()
{
	m_obj->set("Sync", ui->syncBox->currentText());
	m_instance->setSync(ui->syncBox->currentText());
	on_syncBox_currentTextChanged(ui->syncBox->currentText());
}

void InstanceSettings::syncEntriesChanged()
{
	if (!m_instance->sync())
	{
		return;
	}
	QList<EntityBase *> entities = m_instance->sync()->getRootEntities();
	ui->syncEntriesWidget->clear();
	ui->syncEntriesWidget->setRowCount(entities.size());
	int row = 0;
	foreach (EntityBase *entity, entities)
	{
		QTableWidgetItem *type = new QTableWidgetItem(EntityBase::typeToString(entity->type));
		type->setData(Qt::UserRole, QVariant::fromValue(entity));
		QTableWidgetItem *path = new QTableWidgetItem(entity->path);
		QPushButton *btn = new QPushButton(tr("Set version"), this);
		connect(btn, &QPushButton::clicked, [this, entity]()
		{
			entity->changeVersion(this);
		});
		ui->syncEntriesWidget->setItem(row, 0, type);
		ui->syncEntriesWidget->setItem(row, 1, path);
		ui->syncEntriesWidget->setCellWidget(row, 2, btn);
		++row;
	}
}
void InstanceSettings::on_addSyncEntryBtn_clicked()
{
	if (!m_instance->sync() || !m_instance->sync())
	{
		return;
	}
	QStringList items;
	items << tr("Save") << tr("Configs") << tr("Instance folder");
	bool ok = false;
	const QString item = QInputDialog::getItem(this, m_instance->sync()->key(), tr("What type of sync to you want to setup?"),
											   items, 0, false, &ok);
	if (!ok)
	{
		return;
	}
	if (items.indexOf(item) == 0)
	{
		QStringList saves;
		foreach (const QFileInfo &info, QDir(m_instance->minecraftRoot() + "/saves").entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot))
		{
			if (!info.isDir())
			{
				continue;
			}
			saves.append(info.fileName());
		}
		if (saves.isEmpty())
		{
			return;
		}
		const QString save = QInputDialog::getItem(this, m_instance->sync()->key(), tr("Please select which save to backup"), saves, 0, false, &ok);
		if (!ok)
		{
			return;
		}
		EntityBase *entity = new EntitySave(save, m_instance->sync());
		m_instance->sync()->addRootEntity(entity);
		ProgressDialog dialog(this);
		dialog.exec(m_instance->sync()->push(entity));
	}
	else if (items.indexOf(item) == 1)
	{
		EntityBase *entity = new EntityConfigs(m_instance->sync());
		m_instance->sync()->addRootEntity(entity);
		ProgressDialog dialog(this);
		dialog.exec(m_instance->sync()->push(entity));
	}
	else if (items.indexOf(item) == 2)
	{
		EntityBase *entity = new EntityInstanceFolder(m_instance->sync());
		m_instance->sync()->addRootEntity(entity);
		ProgressDialog dialog(this);
		dialog.exec(m_instance->sync()->push(entity));
	}
}

void InstanceSettings::on_pullSyncEntryBtn_clicked()
{
	if (!m_instance->sync() || ui->syncEntriesWidget->currentRow() == -1 || !m_instance->sync())
	{
		return;
	}
	EntityBase *entity = ui->syncEntriesWidget->item(ui->syncEntriesWidget->currentRow(), 0)->data(Qt::UserRole).value<EntityBase *>();
	m_instance->sync()->pull(entity);
}
void InstanceSettings::on_removeSyncEntryBtn_clicked()
{
	if (!m_instance->sync() || ui->syncEntriesWidget->currentRow() == -1 || !m_instance->sync())
	{
		return;
	}
	EntityBase *entity = ui->syncEntriesWidget->item(ui->syncEntriesWidget->currentRow(), 0)->data(Qt::UserRole).value<EntityBase *>();
	if (QMessageBox::question(this, m_instance->sync()->key(), tr("Really remove the %1 (%2)? This will not delete them, but it will not be possible to get back old versions from inside MultiMC")
							  .arg(EntityBase::typeToString(entity->type), entity->path)) == QMessageBox::Yes)
	{
		m_instance->sync()->removeRootEntity(entity);
	}
}

void InstanceSettings::applySettings()
{
	// Console
	bool console = ui->consoleSettingsBox->isChecked();
	m_obj->set("OverrideConsole", console);
	if (console)
	{
		m_obj->set("ShowConsole", ui->showConsoleCheck->isChecked());
		m_obj->set("AutoCloseConsole", ui->autoCloseConsoleCheck->isChecked());
	}
	else
	{
		m_obj->reset("ShowConsole");
		m_obj->reset("AutoCloseConsole");
	}

	// Window Size
	bool window = ui->windowSizeGroupBox->isChecked();
	m_obj->set("OverrideWindow", window);
	if (window)
	{
		m_obj->set("LaunchMaximized", ui->maximizedCheckBox->isChecked());
		m_obj->set("MinecraftWinWidth", ui->windowWidthSpinBox->value());
		m_obj->set("MinecraftWinHeight", ui->windowHeightSpinBox->value());
	}
	else
	{
		m_obj->reset("LaunchMaximized");
		m_obj->reset("MinecraftWinWidth");
		m_obj->reset("MinecraftWinHeight");
	}

	// Memory
	bool memory = ui->memoryGroupBox->isChecked();
	m_obj->set("OverrideMemory", memory);
	if (memory)
	{
		m_obj->set("MinMemAlloc", ui->minMemSpinBox->value());
		m_obj->set("MaxMemAlloc", ui->maxMemSpinBox->value());
		m_obj->set("PermGen", ui->permGenSpinBox->value());
	}
	else
	{
		m_obj->reset("MinMemAlloc");
		m_obj->reset("MaxMemAlloc");
		m_obj->reset("PermGen");
	}

	// Java Settings
	bool java = ui->javaSettingsGroupBox->isChecked();
	m_obj->set("OverrideJava", java);
	if (java)
	{
		m_obj->set("JavaPath", ui->javaPathTextBox->text());
		m_obj->set("JvmArgs", ui->jvmArgsTextBox->text());

		NagUtils::checkJVMArgs(m_obj->get("JvmArgs").toString(), this->parentWidget());
	}
	else
	{
		m_obj->reset("JavaPath");
		m_obj->reset("JvmArgs");
	}

	// Custom Commands
	bool custcmd = ui->customCommandsGroupBox->isChecked();
	m_obj->set("OverrideCommands", custcmd);
	if (custcmd)
	{
		m_obj->set("PreLaunchCommand", ui->preLaunchCmdTextBox->text());
		m_obj->set("PostExitCommand", ui->postExitCmdTextBox->text());
	}
	else
	{
		m_obj->reset("PreLaunchCommand");
		m_obj->reset("PostExitCommand");
	}

	// Sync
	{
		if (m_instance->sync())
		{
			m_obj->set("Sync", m_instance->sync()->key());
			if (m_syncSettingsWidget)
			{
				m_instance->sync()->applySettings(m_syncSettingsWidget);
			}
		}
		else
		{
			m_obj->set("Sync", "");
		}
	}
}

void InstanceSettings::loadSettings()
{
	// Console
	ui->consoleSettingsBox->setChecked(m_obj->get("OverrideConsole").toBool());
	ui->showConsoleCheck->setChecked(m_obj->get("ShowConsole").toBool());
	ui->autoCloseConsoleCheck->setChecked(m_obj->get("AutoCloseConsole").toBool());

	// Window Size
	ui->windowSizeGroupBox->setChecked(m_obj->get("OverrideWindow").toBool());
	ui->maximizedCheckBox->setChecked(m_obj->get("LaunchMaximized").toBool());
	ui->windowWidthSpinBox->setValue(m_obj->get("MinecraftWinWidth").toInt());
	ui->windowHeightSpinBox->setValue(m_obj->get("MinecraftWinHeight").toInt());

	// Memory
	ui->memoryGroupBox->setChecked(m_obj->get("OverrideMemory").toBool());
	ui->minMemSpinBox->setValue(m_obj->get("MinMemAlloc").toInt());
	ui->maxMemSpinBox->setValue(m_obj->get("MaxMemAlloc").toInt());
	ui->permGenSpinBox->setValue(m_obj->get("PermGen").toInt());

	// Java Settings
	ui->javaSettingsGroupBox->setChecked(m_obj->get("OverrideJava").toBool());
	ui->javaPathTextBox->setText(m_obj->get("JavaPath").toString());
	ui->jvmArgsTextBox->setText(m_obj->get("JvmArgs").toString());

	// Custom Commands
	ui->customCommandsGroupBox->setChecked(m_obj->get("OverrideCommands").toBool());
	ui->preLaunchCmdTextBox->setText(m_obj->get("PreLaunchCommand").toString());
	ui->postExitCmdTextBox->setText(m_obj->get("PostExitCommand").toString());

	// Sync
	{
		ui->syncBox->clear();
		ui->syncBox->addItems(SyncFactory::keys());
		on_syncBox_currentTextChanged(ui->syncBox->currentText());
	}
}

void InstanceSettings::on_javaDetectBtn_clicked()
{
	JavaVersionPtr java;

	VersionSelectDialog vselect(MMC->javalist().get(), tr("Select a Java version"), this, true);
	vselect.setResizeOn(2);
	vselect.exec();

	if (vselect.result() == QDialog::Accepted && vselect.selectedVersion())
	{
		java = std::dynamic_pointer_cast<JavaVersion>(vselect.selectedVersion());
		ui->javaPathTextBox->setText(java->path);
	}
}

void InstanceSettings::on_javaBrowseBtn_clicked()
{
	QString dir = QFileDialog::getOpenFileName(this, tr("Find Java executable"));
	if (!dir.isNull())
	{
		ui->javaPathTextBox->setText(dir);
	}
}

void InstanceSettings::on_javaTestBtn_clicked()
{
	checker.reset(new JavaChecker());
	connect(checker.get(), SIGNAL(checkFinished(JavaCheckResult)), this,
			SLOT(checkFinished(JavaCheckResult)));
	checker->path = ui->javaPathTextBox->text();
	checker->performCheck();
}

void InstanceSettings::checkFinished(JavaCheckResult result)
{
	if (result.valid)
	{
		QString text;
		text += "Java test succeeded!\n";
		if (result.is_64bit)
			text += "Using 64bit java.\n";
		text += "\n";
		text += "Platform reported: " + result.realPlatform;
		QMessageBox::information(this, tr("Java test success"), text);
	}
	else
	{
		QMessageBox::warning(
			this, tr("Java test failure"),
			tr("The specified java binary didn't work. You should use the auto-detect feature, "
			   "or set the path to the java executable."));
	}
}
