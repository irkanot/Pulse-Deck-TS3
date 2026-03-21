// src/SoundInfo.h
//----------------------------------
// Pulse Deck TS3 Source Code
// Copyright (c) 2015 Davide (Irkanot)
// All rights reserved
// Contact: support@pulse-deck-ts3.local
//----------------------------------

#pragma once

#include <QSettings>
#include <QColor>
#include <stdexcept>

class SoundInfo
{
  public:
	SoundInfo();
	void readFromConfig(const QSettings& settings);
	void saveToConfig(QSettings& settings) const;
	double getStartTime() const;
	double getPlayTime() const;

	static double getTimeUnitFactor(int unit);
	bool customColorEnabled() const
	{
		return customColor.alpha() != 0;
	}
	void setCustomColorEnabled(bool enabled)
	{
		customColor.setAlpha(enabled ? 255 : 0);
	}

  public:
	QString filename;
	QString customText;
	QColor customColor;
	int volume;
	bool cropEnabled;
	int cropStartValue;
	int cropStartUnit;
	int cropStopAfterAt;
	int cropStopValue;
	int cropStopUnit;
};

