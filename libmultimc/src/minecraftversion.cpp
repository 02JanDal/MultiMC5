/* Copyright 2013 Andrew Okin
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

#include "minecraftversion.h"

MinecraftVersion::MinecraftVersion(QString descriptor, 
								   QString name, 
								   qint64 timestamp, 
								   QString dlUrl, 
								   QString etag, 
								   InstVersionList *parent) :
		InstVersion(descriptor, name, timestamp, parent), m_dlUrl(dlUrl), m_etag(etag)
{
	m_isNewLauncherVersion = false;
}

QString MinecraftVersion::descriptor() const
{
	return m_descriptor;
}

QString MinecraftVersion::name() const
{
	return m_name;
}

QString MinecraftVersion::typeName() const
{
	switch (versionType())
	{
	case OldSnapshot:
		return "Snapshot";
		
	case Stable:
		return "Stable";
		
	case CurrentStable:
		return "Current Stable";
		
	case Snapshot:
		return "Snapshot";
		
	case MCNostalgia:
		return "MCNostalgia";
		
	default:
		return QString("Unknown Type %1").arg(versionType());
	}
}

qint64 MinecraftVersion::timestamp() const
{
	return m_timestamp;
}

MinecraftVersion::VersionType MinecraftVersion::versionType() const
{
	return m_type;
}

void MinecraftVersion::setVersionType(MinecraftVersion::VersionType typeName)
{
	m_type = typeName;
}

QString MinecraftVersion::downloadURL() const
{
	return m_dlUrl;
}

QString MinecraftVersion::etag() const
{
	return m_etag;
}

MinecraftVersion::LauncherVersion MinecraftVersion::launcherVersion() const
{
	return m_launcherVersion;
};

void MinecraftVersion::setLauncherVersion(LauncherVersion launcherVersion)
{
	m_launcherVersion = launcherVersion;
}

InstVersion *MinecraftVersion::copyVersion(InstVersionList *newParent) const
{
	MinecraftVersion *version = new MinecraftVersion(
				descriptor(), name(), timestamp(), downloadURL(), etag(), newParent);
	version->setVersionType(versionType());
	version->setLauncherVersion(launcherVersion());
	return version;
}