/* Copyright 2013 MultiMC Contributors
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

#pragma once

#include <QString>
#include <QVariant>
#include <QIODevice>

#include "libsettings_config.h"

// Sectionless INI parser (for instance config files)
class LIBSETTINGS_EXPORT INIFile : public QMap<QString, QVariant>
{
public:
	explicit INIFile();

	bool loadFile(QByteArray file);
	bool loadFile(QString fileName);
	bool saveFile(QString fileName);

	QVariant get(QString key, QVariant def) const;
	void set(QString key, QVariant val);
	QString unescape(QString orig);
	QString escape(QString orig);

private:
	QString variantToString(const QVariant &variant);
	QVariant stringToVariant(const QString &string);
};