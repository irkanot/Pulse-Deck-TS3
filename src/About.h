// src/About.h
//----------------------------------
// Pulse Deck TS3 Source Code
// Copyright (c) 2015 Davide (Irkanot)
// All rights reserved
// Contact: support@pulse-deck-ts3.local
//----------------------------------

#pragma once

#include <QWidget>

#include "ui_About.h"

namespace Ui
{
class AboutQt;
}


class AboutQt : public QWidget
{
	Q_OBJECT

  public:
	explicit AboutQt(QWidget* parent = 0);
	~AboutQt();

  private:
	Ui::AboutQt* ui;
};

