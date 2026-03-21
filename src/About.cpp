// src/About.cpp
//----------------------------------
// Pulse Deck TS3 Source Code
// Copyright (c) 2015 Davide (Irkanot)
// All rights reserved
// Contact: support@pulse-deck-ts3.local
//----------------------------------


#include "About.h"
#include "buildinfo.h"

// Import from ffmpeg
extern "C" const char* av_version_info();


AboutQt::AboutQt(QWidget* parent) :
	QWidget(parent, Qt::Window | Qt::WindowTitleHint /*| Qt::CustomizeWindowHint*/ | Qt::WindowCloseButtonHint),
	ui(new Ui::AboutQt)
{
	const char* ffmpeg_version = av_version_info();
	ui->setupUi(this);
	ui->l_version->setText(
		QString(buildinfo_getPluginVersion()) + "\nBuild on " + buildinfo_getBuildDate() + " " +
		buildinfo_getBuildTime() + "\nFFmpeg Version: " + (ffmpeg_version ? ffmpeg_version : "unknown") +
		"\nLinked against Qt " QT_VERSION_STR
	);
	setFixedSize(size());
}


AboutQt::~AboutQt()
{
	delete ui;
}

