// src/MainWindow.cpp
//----------------------------------
// Pulse Deck TS3 Source Code
// Copyright (c) 2015 Davide (Irkanot)
// All rights reserved
// Contact: support@pulse-deck-ts3.local
//----------------------------------

#include "common.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QResizeEvent>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QColorDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QLineEdit>
#include <QFormLayout>
#include <QHBoxLayout>
#include <algorithm>
#include <random>

#include "MainWindow.h"
#include "ConfigModel.h"
#include "main.h"
#include "SoundSettings.h"
#include "ts3log.h"
#include "SpeechBubble.h"
#include "buildinfo.h"
#include "plugin.h"
#include "ExpandableSection.h"
#include "samples.h"
#include "SoundButton.h"
#include "ButtonBoxWindow.h"
#include "inputfile.h"

#ifdef _WIN32
#include "Windows.h"
#endif

enum button_choices_e
{
	BC_CHOOSE = 0,
	BC_ADVANCED,
	BC_SET_HOTKEY,
	BC_SET_COLOR,
	BC_DELETE,
};

static QString normalizedPlaylistPath(const QString& p)
{
	return QFileInfo(p).absoluteFilePath().toLower();
}

struct PlaylistImportDialogResult
{
	QString category;
	QStringList tags;
	bool accepted = false;
};

static PlaylistImportDialogResult showPlaylistImportDialog(QWidget* parent)
{
	QDialog dialog(parent);
	dialog.setWindowTitle("Playlist metadata");
	QVBoxLayout* layout = new QVBoxLayout(&dialog);
	QFormLayout* form = new QFormLayout();

	QComboBox* categoryPreset = new QComboBox(&dialog);
	categoryPreset->addItems({"", "Music", "Meme", "Voice", "SFX", "Ambient", "Custom"});
	QLineEdit* categoryCustom = new QLineEdit(&dialog);
	categoryCustom->setPlaceholderText("Optional custom category");
	QLineEdit* tagsEdit = new QLineEdit(&dialog);
	tagsEdit->setPlaceholderText("tag1, tag2, ...");

	form->addRow("Category", categoryPreset);
	form->addRow("Custom", categoryCustom);
	form->addRow("Tags", tagsEdit);
	layout->addLayout(form);

	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
	layout->addWidget(buttons);
	QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

	PlaylistImportDialogResult result;
	if (dialog.exec() != QDialog::Accepted)
		return result;

	QString preset = categoryPreset->currentText().trimmed();
	QString custom = categoryCustom->text().trimmed();
	if (!custom.isEmpty())
		result.category = custom;
	else if (!preset.isEmpty() && preset.compare("Custom", Qt::CaseInsensitive) != 0)
		result.category = preset;

	QStringList tags = tagsEdit->text().split(',', Qt::SkipEmptyParts);
	for (QString& tag : tags)
		tag = tag.trimmed();
	tags.erase(std::remove_if(tags.begin(), tags.end(), [](const QString& t) { return t.isEmpty(); }), tags.end());
	result.tags = tags;
	result.accepted = true;
	return result;
}

MainWindow::MainWindow(ConfigModel* model, QWidget* parent /*= 0*/) :
	QWidget(parent),
	ui(new Ui::MainWindow),
	m_model(model),
	m_modelObserver(*this),
	m_buttonBubble(nullptr),
	m_playlistStartButton(nullptr),
	m_playlistPrevButton(nullptr),
	m_playlistNextButton(nullptr),
	m_playlistRandomButton(nullptr),
	m_playlistAddButton(nullptr),
	m_playlistRemoveButton(nullptr),
	m_openButtonBoxButton(nullptr),
	m_themeToggleButton(nullptr),
	m_playlistView(nullptr),
	m_buttonBoxWindow(nullptr),
	m_playlistNowLabel(nullptr),
	m_playlistNextLabel(nullptr),
	m_playlistElapsedLabel(nullptr),
	m_playlistRemainingLabel(nullptr),
	m_playlistProgressBar(nullptr),
	m_playlistCurrentPos(-1),
	m_playlistRunning(false),
	m_ignoreNextStopEvent(false),
	m_playlistRandomMode(false),
	m_playlistProgressTimer(nullptr),
	m_playlistPausedMs(0),
	m_playlistPauseStartedMs(-1),
	m_playlistCurrentDurationMs(0)
{
	/* Ensure resources are loaded */
	Q_INIT_RESOURCE(qtres);

	m_pauseIcon = QIcon(":/icon/img/pausebutton_32.png");
	m_playIcon = QIcon(":/icon/img/playarrow_32.png");
	m_rng.seed(std::random_device{}());

	ui->setupUi(this);
	// setAttribute(Qt::WA_DeleteOnClose);

	createConfigButtons();

	settingsSection = new ExpandableSection("Settings", 200, this);
	settingsSection->setContentLayout(*ui->settingsWidget->layout());
	layout()->addWidget(settingsSection);

	configsSection = new ExpandableSection("Configurations", 200, this);
	configsSection->setContentLayout(*ui->configsWidget->layout());
	layout()->addWidget(configsSection);

	QAction* actChooseFile = new QAction("Choose File", this);
	actChooseFile->setData((int)BC_CHOOSE);
	m_buttonContextMenu.addAction(actChooseFile);

	QAction* actAdvancedOpts = new QAction("Advanced Options", this);
	actAdvancedOpts->setData((int)BC_ADVANCED);
	m_buttonContextMenu.addAction(actAdvancedOpts);

	actSetHotkey = new QAction("Set hotkey", this);
	actSetHotkey->setData((int)BC_SET_HOTKEY);
	m_buttonContextMenu.addAction(actSetHotkey);

	QAction* actSetColor = new QAction("Set color", this);
	actSetColor->setData((int)BC_SET_COLOR);
	m_buttonContextMenu.addAction(actSetColor);

	QAction* actDeleteButton = new QAction("Make button great again (delete)", this);
	actDeleteButton->setData((int)BC_DELETE);
	m_buttonContextMenu.addAction(actDeleteButton);

	createButtons();

	m_playlistPrevButton = new QPushButton("â®", this);
	m_playlistPrevButton->setToolTip("Previous track");
	m_playlistNextButton = new QPushButton("â­", this);
	m_playlistNextButton->setToolTip("Next track");
	m_playlistStartButton = new QPushButton("â–¶ Playlist", this);
	m_playlistStartButton->setToolTip("Start playlist");
	m_playlistRandomButton = new QPushButton("Random OFF", this);
	m_playlistRandomButton->setToolTip("Toggle random mode (no repeat until all tracks played)");
	m_playlistAddButton = new QPushButton("ï¼‹ Files", this);
	m_playlistAddButton->setToolTip("Add one or more songs to playlist");
	m_playlistRemoveButton = new QPushButton("ï¼ Remove", this);
	m_playlistRemoveButton->setToolTip("Remove selected songs from playlist");
	m_openButtonBoxButton = new QPushButton("ButtonBox", this);
	m_openButtonBoxButton->setToolTip("Open detached ButtonBox window");
	m_themeToggleButton = new QPushButton(this);
	m_themeToggleButton->setToolTip("Switch between original light theme and dark theme");
	ui->horizontalLayout_4->insertWidget(0, m_playlistPrevButton);
	ui->horizontalLayout_4->insertWidget(1, m_playlistStartButton);
	ui->horizontalLayout_4->insertWidget(2, m_playlistNextButton);
	ui->horizontalLayout_4->insertWidget(3, m_playlistRandomButton);
	ui->horizontalLayout_4->insertWidget(4, m_playlistAddButton);
	ui->horizontalLayout_4->insertWidget(5, m_playlistRemoveButton);
	ui->horizontalLayout_4->insertWidget(6, m_openButtonBoxButton);
	ui->horizontalLayout_4->insertWidget(7, m_themeToggleButton);

	QWidget* playlistWidget = new QWidget(this);
	QVBoxLayout* playlistLayout = new QVBoxLayout(playlistWidget);
	playlistLayout->setContentsMargins(9, 3, 9, 3);
	playlistLayout->setSpacing(3);
	m_playlistNowLabel = new QLabel("Now: -", playlistWidget);
	m_playlistNextLabel = new QLabel("Next: -", playlistWidget);
	m_playlistProgressBar = new QProgressBar(playlistWidget);
	m_playlistProgressBar->setRange(0, 1000);
	m_playlistProgressBar->setValue(0);
	m_playlistProgressBar->setTextVisible(false);
	m_playlistElapsedLabel = new QLabel("0:00", playlistWidget);
	m_playlistRemainingLabel = new QLabel("-0:00", playlistWidget);
	QHBoxLayout* playlistTimeLayout = new QHBoxLayout();
	playlistTimeLayout->setContentsMargins(0, 0, 0, 0);
	playlistTimeLayout->addWidget(m_playlistElapsedLabel);
	playlistTimeLayout->addStretch();
	playlistTimeLayout->addWidget(m_playlistRemainingLabel);
	m_playlistView = new QListWidget(playlistWidget);
	m_playlistView->setMinimumHeight(110);
	m_playlistView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_playlistView->setAcceptDrops(true);
	m_playlistView->viewport()->setAcceptDrops(true);
	m_playlistView->setDragDropMode(QAbstractItemView::DropOnly);
	m_playlistView->viewport()->installEventFilter(this);
	playlistLayout->addWidget(m_playlistNowLabel);
	playlistLayout->addWidget(m_playlistNextLabel);
	playlistLayout->addWidget(m_playlistProgressBar);
	playlistLayout->addLayout(playlistTimeLayout);
	playlistLayout->addWidget(m_playlistView);
	layout()->addWidget(playlistWidget);

	ui->b_stop->setContextMenuPolicy(Qt::CustomContextMenu);
	ui->b_pause->setContextMenuPolicy(Qt::CustomContextMenu);
	ui->cb_mute_locally->setContextMenuPolicy(Qt::CustomContextMenu);
	ui->cb_mute_myself->setContextMenuPolicy(Qt::CustomContextMenu);
	ui->sl_volumeLocal->setContextMenuPolicy(Qt::CustomContextMenu);
	ui->sl_volumeRemote->setContextMenuPolicy(Qt::CustomContextMenu);
	m_playlistStartButton->setContextMenuPolicy(Qt::CustomContextMenu);
	m_playlistPrevButton->setContextMenuPolicy(Qt::CustomContextMenu);
	m_playlistNextButton->setContextMenuPolicy(Qt::CustomContextMenu);
	m_playlistRandomButton->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(ui->b_stop, SIGNAL(clicked()), this, SLOT(onClickedStop()));
	connect(
		ui->b_stop, SIGNAL(customContextMenuRequested(const QPoint&)), this,
		SLOT(showStopButtonContextMenu(const QPoint&))
	);
	connect(ui->b_pause, SIGNAL(clicked()), this, SLOT(onButtonPausePressed()));
	connect(m_playlistStartButton, SIGNAL(clicked()), this, SLOT(onPlaylistStartPressed()));
	connect(m_playlistPrevButton, SIGNAL(clicked()), this, SLOT(onPlaylistPrevPressed()));
	connect(m_playlistNextButton, SIGNAL(clicked()), this, SLOT(onPlaylistNextPressed()));
	connect(m_playlistRandomButton, SIGNAL(clicked()), this, SLOT(onPlaylistRandomPressed()));
	connect(m_playlistAddButton, SIGNAL(clicked()), this, SLOT(onPlaylistAddFilesPressed()));
	connect(m_playlistRemoveButton, SIGNAL(clicked()), this, SLOT(onPlaylistRemoveFilesPressed()));
	connect(m_openButtonBoxButton, SIGNAL(clicked()), this, SLOT(onOpenButtonBoxPressed()));
	connect(m_themeToggleButton, SIGNAL(clicked()), this, SLOT(onThemeTogglePressed()));
	connect(m_playlistView, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(onPlaylistItemDoubleClicked(QListWidgetItem*)));
	connect(
		ui->b_pause, SIGNAL(customContextMenuRequested(const QPoint&)), this,
		SLOT(showPauseButtonContextMenu(const QPoint&))
	);
	connect(ui->sl_volumeLocal, SIGNAL(valueChanged(int)), this, SLOT(onUpdateVolumeLocal(int)));
	connect(ui->sl_volumeRemote, SIGNAL(valueChanged(int)), this, SLOT(onUpdateVolumeRemote(int)));
	connect(ui->cb_mute_locally, SIGNAL(clicked(bool)), this, SLOT(onUpdateMuteLocally(bool)));
	connect(ui->sb_rows, SIGNAL(valueChanged(int)), this, SLOT(onUpdateRows(int)));
	connect(ui->sb_cols, SIGNAL(valueChanged(int)), this, SLOT(onUpdateCols(int)));
	connect(ui->cb_mute_myself, SIGNAL(clicked(bool)), this, SLOT(onUpdateMuteMyself(bool)));
	connect(ui->cb_show_hotkeys_on_buttons, SIGNAL(clicked(bool)), this, SLOT(onUpdateShowHotkeysOnButtons(bool)));
	connect(ui->cb_disable_hotkeys, SIGNAL(clicked(bool)), this, SLOT(onUpdateHotkeysDisabled(bool)));
	connect(ui->filterEdit, SIGNAL(textChanged(const QString&)), this, SLOT(onFilterEditTextChanged(const QString&)));
	connect(
		ui->cb_mute_locally, &QCheckBox::customContextMenuRequested, [this](const QPoint& point)
		{ this->showSetHotkeyMenu(HOTKEY_MUTE_ON_MY_CLIENT, ui->cb_mute_locally->mapToGlobal(point)); }
	);
	connect(
		ui->cb_mute_myself, &QCheckBox::customContextMenuRequested, [this](const QPoint& point)
		{ this->showSetHotkeyMenu(HOTKEY_MUTE_MYSELF, ui->cb_mute_myself->mapToGlobal(point)); }
	);
	connect(
		ui->sl_volumeLocal, &QSlider::customContextMenuRequested, this, &MainWindow::onVolumeSliderContextMenuLocal
	);
	connect(
		ui->sl_volumeRemote, &QSlider::customContextMenuRequested, this, &MainWindow::onVolumeSliderContextMenuRemote
	);
	connect(
		m_playlistStartButton, &QPushButton::customContextMenuRequested, [this](const QPoint& point)
		{ this->showSetHotkeyMenu(HOTKEY_PLAYLIST_START, m_playlistStartButton->mapToGlobal(point)); }
	);
	connect(
		m_playlistNextButton, &QPushButton::customContextMenuRequested, [this](const QPoint& point)
		{ this->showSetHotkeyMenu(HOTKEY_PLAYLIST_NEXT, m_playlistNextButton->mapToGlobal(point)); }
	);
	connect(
		m_playlistPrevButton, &QPushButton::customContextMenuRequested, [this](const QPoint& point)
		{ this->showSetHotkeyMenu(HOTKEY_PLAYLIST_PREV, m_playlistPrevButton->mapToGlobal(point)); }
	);
	connect(
		m_playlistRandomButton, &QPushButton::customContextMenuRequested, [this](const QPoint& point)
		{ this->showSetHotkeyMenu(HOTKEY_PLAYLIST_RANDOM, m_playlistRandomButton->mapToGlobal(point)); }
	);

	/* Load/Save Model */
	connect(ui->pushLoad, SIGNAL(released()), this, SLOT(onLoadModel()));
	connect(ui->pushSave, SIGNAL(released()), this, SLOT(onSaveModel()));

	ui->playingIconLabel->hide();
	ui->playingLabel->setText("");
	playingIconTimer = new QTimer(this);
	playingIconTimer->setInterval(150);
	connect(playingIconTimer, SIGNAL(timeout()), this, SLOT(onPlayingIconTimer()));
	m_playlistProgressTimer = new QTimer(this);
	m_playlistProgressTimer->setInterval(250);
	connect(m_playlistProgressTimer, SIGNAL(timeout()), this, SLOT(onPlaylistProgressTimer()));

	Sampler* sampler = sb_getSampler();
	connect(
		sampler, SIGNAL(onStartPlaying(bool, QString)), this, SLOT(onStartPlayingSound(bool, QString)),
		Qt::QueuedConnection
	);
	connect(sampler, SIGNAL(onStopPlaying()), this, SLOT(onStopPlayingSound()), Qt::QueuedConnection);
	connect(
		sampler, SIGNAL(onPausePlaying()), this, SLOT(onPausePlayingSound())
	); // No queued connection since signal is emitted from GUI Thread
	connect(
		sampler, SIGNAL(onUnpausePlaying()), this, SLOT(onUnpausePlayingSound())
	); // No queued connection since signal is emitted from GUI Thread

	ui->gridWidget->hide(); // Button grid is now intended for detached ButtonBox window

	createBubbles();

	m_model->addObserver(&m_modelObserver);

	/* Force configuration 0 */
	setConfiguration(0);
	loadPersistentPlaylist();
	rebuildPlaylist();

	applyTheme(m_model->getDarkThemeEnabled());
}

void MainWindow::setConfiguration(int cfg)
{
	if (cfg < 0 || cfg >= NUM_CONFIGS)
	{
		logError("Invalid config id: %i", cfg);
		return;
	}

	m_configRadioButtons[cfg]->setChecked(true);
	m_model->setConfiguration(cfg);
	ui->labelStatus->setText(QString("Configuration %1").arg(cfg + 1));
	rebuildPlaylist();
}

void MainWindow::onSetConfig()
{
	QRadioButton* button = qobject_cast<QRadioButton*>(sender());
	int configId = button->property("configId").toInt();
	setConfiguration(configId);
}

void MainWindow::onConfigHotkey()
{
	QPushButton* button = qobject_cast<QPushButton*>(sender());
	int configId = button->property("configId").toInt();

	char buf[16];
	sb_getInternalConfigHotkeyName(configId, buf);
	ts3Functions.requestHotkeyInputDialog(getPluginID(), buf, configId, this);
}


MainWindow::~MainWindow()
{
	m_model->remObserver(&m_modelObserver);
	delete ui;
}

void MainWindow::onSaveModel()
{
	QString fn = QFileDialog::getSaveFileName(this, tr("Choose File to Save"), QString(), tr("Ini Files (*.ini)"));
	if (fn.isNull())
		return;
	m_model->writeConfig(fn);
}

void MainWindow::onLoadModel()
{
	QString fn = QFileDialog::getOpenFileName(this, tr("Choose File to Load"), QString(), tr("Ini Files (*.ini)"));
	if (fn.isNull())
		return;
	m_model->readConfig(fn);
}

void MainWindow::closeEvent(QCloseEvent*)
{
	m_model->setWindowSize(size().width(), size().height());
}

void MainWindow::onClickedPlay()
{
	QPushButton* button = dynamic_cast<QPushButton*>(sender());
	size_t buttonId =
		std::find_if(m_buttons.begin(), m_buttons.end(), [button](SoundButton* b) { return b == button; }) -
		m_buttons.begin();

	playSound(buttonId);
}


void MainWindow::onClickedStop()
{
	m_playlistRunning = false;
	m_playlistCurrentDurationMs = 0;
	m_playlistPausedMs = 0;
	m_playlistPauseStartedMs = -1;
	if (m_playlistProgressTimer)
		m_playlistProgressTimer->stop();
	sb_stopPlayback();
	refreshPlaylistUi();
}


void MainWindow::onUpdateVolumeLocal(int val)
{
	m_model->setVolumeLocal(val);
}


void MainWindow::onUpdateVolumeRemote(int val)
{
	m_model->setVolumeRemote(val);
}


void MainWindow::onUpdateMuteLocally(bool val)
{
	m_model->setPlaybackLocal(!val);
}


void MainWindow::onUpdateCols(int val)
{
	m_model->setCols(val);
}


void MainWindow::onUpdateRows(int val)
{
	m_model->setRows(val);
}


void MainWindow::onUpdateMuteMyself(bool val)
{
	m_model->setMuteMyselfDuringPb(val);
}


void MainWindow::createButtons()
{
	for (SoundButton* button : m_buttons)
		delete button;
	m_buttons.clear();

	int numRows = m_model->getRows();
	int numCols = m_model->getCols();

	for (int i = 0; i < numRows; i++)
	{
		for (int j = 0; j < numCols; j++)
		{
			SoundButton* elem = new SoundButton(this);
			elem->setProperty("buttonId", (int)m_buttons.size());
			elem->setText("(no file)");
			elem->setEnabled(true);
			QSizePolicy policy(QSizePolicy::Ignored, QSizePolicy::Expanding);
			policy.setRetainSizeWhenHidden(true);
			elem->setSizePolicy(policy);
			ui->gridLayout->addWidget(elem, i, j);
			connect(elem, SIGNAL(clicked()), this, SLOT(onClickedPlay()));
			elem->setContextMenuPolicy(Qt::CustomContextMenu);
			connect(
				elem, SIGNAL(customContextMenuRequested(const QPoint&)), this,
				SLOT(showButtonContextMenu(const QPoint&))
			);
			connect(elem, SIGNAL(fileDropped(QList<QUrl>)), this, SLOT(onButtonFileDropped(QList<QUrl>)));
			connect(elem, SIGNAL(buttonDropped(SoundButton*)), this, SLOT(onButtonDroppedOnButton(SoundButton*)));

			elem->updateGeometry();
			m_buttons.push_back(elem);
		}
	}

	for (int i = 0; i < (int)m_buttons.size(); i++)
		updateButtonText(i);

	if (m_buttonBubble && !m_buttons.empty())
		m_buttonBubble->attachTo(m_buttons[0]);

	if (m_buttonBoxWindow)
	{
		m_buttonBoxWindow->setDimensions(numRows, numCols);
		for (int i = 0; i < (int)m_buttons.size(); ++i)
		{
			m_buttonBoxWindow->setButtonText(i, m_buttons[i]->text());
			const SoundInfo* info = m_model->getSoundInfo(i);
			m_buttonBoxWindow->setButtonColor(i, info ? info->customColor : QColor(0, 0, 0, 0));
		}
	}
}


void MainWindow::createConfigButtons()
{
	for (int i = 0; i < NUM_CONFIGS; i++)
	{
		m_configRadioButtons[i] = new QRadioButton(this);
		m_configRadioButtons[i]->setText(QString("Config %1").arg(i + 1));
		m_configRadioButtons[i]->setProperty("configId", i);
		m_configRadioButtons[i]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		ui->configsGrid->addWidget(m_configRadioButtons[i], 0, i, Qt::AlignCenter);
		connect(m_configRadioButtons[i], SIGNAL(clicked()), this, SLOT(onSetConfig()));

		m_configHotkeyButtons[i] = new QPushButton(this);
		m_configHotkeyButtons[i]->setText("Set hotkey");
		m_configHotkeyButtons[i]->setProperty("configId", i);
		m_configHotkeyButtons[i]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		m_configHotkeyButtons[i]->setToolTipDuration(0);
		m_configHotkeyButtons[i]->setToolTip(getConfigShortcutString(i));
		ui->configsGrid->addWidget(m_configHotkeyButtons[i], 1, i, Qt::AlignCenter);
		connect(m_configHotkeyButtons[i], SIGNAL(clicked()), this, SLOT(onConfigHotkey()));

		ui->configsWidget->updateGeometry();
	}
}


bool MainWindow::hotkeysEnabled()
{
	return m_model->getHotkeysEnabled();
}

void MainWindow::updateButtonText(int i)
{
	if (i >= (int)m_buttons.size())
		return;

	QString text;
	const SoundInfo* info = m_model->getSoundInfo(i);
	if (info && !info->filename.isEmpty())
	{
		if (!info->customText.isEmpty())
			text = unescapeCustomText(info->customText);
		else
			text = QFileInfo(info->filename).baseName();
	}
	else
		text = "(no file)";

	if (m_model->getShowHotkeysOnButtons())
	{
		QString shortcut = getShortcutString(i);
		if (shortcut.length() > 0)
			text = text + "\n" + shortcut;
	}
	m_buttons[i]->setText(text);
	m_buttons[i]->setBackgroundColor(info ? info->customColor : QColor(0, 0, 0, 0));
	if (m_buttonBoxWindow)
	{
		m_buttonBoxWindow->setButtonText(i, text);
		m_buttonBoxWindow->setButtonColor(i, info ? info->customColor : QColor(0, 0, 0, 0));
	}
}


void MainWindow::showButtonContextMenu(const QPoint& point)
{
	QPushButton* button = dynamic_cast<QPushButton*>(sender());
	size_t buttonId =
		std::find_if(m_buttons.begin(), m_buttons.end(), [button](SoundButton* b) { return b == button; }) -
		m_buttons.begin();
	if (buttonId >= m_buttons.size())
		return;
	QPoint globalPos = m_buttons[buttonId]->mapToGlobal(point);
	showButtonContextMenuForButton(buttonId, globalPos);
}

void MainWindow::showButtonContextMenuForButton(size_t buttonId, const QPoint& globalPos)
{
	if (buttonId >= m_buttons.size())
		return;

	QString shortcutName = getShortcutString(buttonId);
	QString hotkeyText = "Set hotkey (Current: " + (shortcutName.isEmpty() ? QString("None") : shortcutName) + ")";
	actSetHotkey->setText(hotkeyText);

	QAction* action = m_buttonContextMenu.exec(globalPos);
	if (!action)
		return;

	bool ok = false;
	int choice = action->data().toInt(&ok);
	if (!ok)
	{
		logError("Invalid user data in context menu");
		return;
	}

	switch (choice)
	{
	case BC_CHOOSE:
		chooseFile(buttonId);
		break;
	case BC_ADVANCED:
		openAdvanced(buttonId);
		break;
	case BC_SET_HOTKEY:
		openHotkeySetDialog(buttonId);
		break;
	case BC_SET_COLOR:
		openButtonColorDialog(buttonId);
		break;
	case BC_DELETE:
		deleteButton(buttonId);
		break;
	default:
		break;
	}
}


void MainWindow::setPlayingLabelIcon(int index)
{
	ui->playingIconLabel->setPixmap(QPixmap(QString(":/icon/img/speaker_icon_%1_64.png").arg(index)));
}


void MainWindow::playSound(size_t buttonId)
{
	const SoundInfo* info = m_model->getSoundInfo(buttonId);
	if (!info)
		return;

	// Manual button playback must not drive playlist progression.
	m_playlistRunning = false;
	m_ignoreNextStopEvent = true;
	sb_playFile(*info);
	refreshPlaylistUi();
}

void MainWindow::rebuildPlaylist()
{
	m_randomRemaining.clear();
	if (m_playlistView)
		m_playlistView->clear();

	QStringList sorted = m_playlistFiles;
	sorted.sort(Qt::CaseInsensitive);
	m_playlistFiles = sorted;

	for (int i = 0; i < m_playlistFiles.size(); ++i)
	{
		const QString filePath = m_playlistFiles[i];
		const QString key = normalizedPlaylistPath(filePath);
		const PlaylistFileMeta meta = m_playlistMetaByPath.value(key);
		QString label = QString("%1. %2").arg(i + 1).arg(QFileInfo(filePath).baseName());
		if (!meta.category.isEmpty())
			label += QString(" [%1]").arg(meta.category);
		if (!meta.tags.isEmpty())
			label += QString(" {%1}").arg(meta.tags.join(", "));
		if (m_playlistView)
		{
			QListWidgetItem* item = new QListWidgetItem(label, m_playlistView);
			item->setToolTip(filePath);
		}
	}

	if (m_playlistFiles.isEmpty())
	{
		m_playlistCurrentPos = -1;
		m_playlistRunning = false;
	}
	else if (m_playlistCurrentPos < 0 || m_playlistCurrentPos >= m_playlistFiles.size())
	{
		m_playlistCurrentPos = 0;
	}

	refreshPlaylistUi();
}

void MainWindow::refreshPlaylistUi()
{
	if (!m_playlistNowLabel || !m_playlistNextLabel)
		return;

	if (m_playlistFiles.isEmpty() || m_playlistCurrentPos < 0)
	{
		m_playlistNowLabel->setText("Now: -");
		m_playlistNextLabel->setText("Next: -");
		if (m_playlistStartButton)
			m_playlistStartButton->setText("â–¶ Playlist");
		updatePlaylistProgressUi();
		return;
	}

	QString nowTitle = QFileInfo(m_playlistFiles[m_playlistCurrentPos]).baseName();
	m_playlistNowLabel->setText(QString("Now: %1").arg(nowTitle));

	int nextPos = (m_playlistCurrentPos + 1) % m_playlistFiles.size();
	QString nextTitle = QFileInfo(m_playlistFiles[nextPos]).baseName();
	m_playlistNextLabel->setText(QString("Next: %1").arg(nextTitle));

	if (m_playlistView)
		m_playlistView->setCurrentRow(m_playlistCurrentPos);
	if (m_playlistStartButton)
		m_playlistStartButton->setText(m_playlistRunning ? "â–  Playlist" : "â–¶ Playlist");
	if (m_playlistRandomButton)
		m_playlistRandomButton->setText(m_playlistRandomMode ? "Random ON" : "Random OFF");
	updatePlaylistProgressUi();
}

QString MainWindow::formatSeconds(qint64 totalSeconds) const
{
	if (totalSeconds < 0)
		totalSeconds = 0;
	qint64 minutes = totalSeconds / 60;
	qint64 seconds = totalSeconds % 60;
	return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}

qint64 MainWindow::estimateTrackDurationMs(const QString& filePath) const
{
	InputFile* input = CreateInputFileFFmpeg();
	if (!input)
		return 0;
	qint64 result = 0;
	if (input->open(filePath.toUtf8().constData()) == 0)
	{
		const int64_t samples = input->outputSamplesEstimation();
		if (samples > 0)
			result = (samples * 1000) / 48000;
		input->close();
	}
	delete input;
	return result;
}

void MainWindow::updatePlaylistProgressUi()
{
	if (!m_playlistProgressBar || !m_playlistElapsedLabel || !m_playlistRemainingLabel)
		return;

	if (m_playlistCurrentDurationMs <= 0 || (!m_playlistRunning && m_playlistPauseStartedMs < 0))
	{
		m_playlistProgressBar->setValue(0);
		m_playlistElapsedLabel->setText("0:00");
		m_playlistRemainingLabel->setText("-0:00");
		return;
	}

	qint64 elapsedMs = m_playlistPausedMs;
	if (m_playlistElapsedTimer.isValid())
		elapsedMs += m_playlistElapsedTimer.elapsed();
	if (m_playlistPauseStartedMs >= 0)
		elapsedMs = m_playlistPauseStartedMs;

	elapsedMs = (std::max<qint64>)(0, (std::min<qint64>)(elapsedMs, m_playlistCurrentDurationMs));
	qint64 remainingMs = (std::max<qint64>)(0, m_playlistCurrentDurationMs - elapsedMs);
	int barValue = (m_playlistCurrentDurationMs > 0) ? (int)((elapsedMs * 1000) / m_playlistCurrentDurationMs) : 0;
	m_playlistProgressBar->setValue((std::max<int>)(0, (std::min<int>)(1000, barValue)));
	m_playlistElapsedLabel->setText(formatSeconds(elapsedMs / 1000));
	m_playlistRemainingLabel->setText(QString("-%1").arg(formatSeconds(remainingMs / 1000)));
}

void MainWindow::onPlaylistProgressTimer()
{
	updatePlaylistProgressUi();
}

void MainWindow::playPlaylistPosition(int pos)
{
	if (m_playlistFiles.isEmpty())
		return;
	if (pos < 0 || pos >= m_playlistFiles.size())
		return;
	m_playlistCurrentPos = pos;
	SoundInfo info;
	info.filename = m_playlistFiles[m_playlistCurrentPos];
	m_ignoreNextStopEvent = true;
	sb_playFile(info);
	m_playlistRunning = true;
	m_playlistCurrentDurationMs = estimateTrackDurationMs(info.filename);
	m_playlistPausedMs = 0;
	m_playlistPauseStartedMs = -1;
	m_playlistElapsedTimer.restart();
	if (m_playlistProgressTimer)
		m_playlistProgressTimer->start();
	refreshPlaylistUi();
}

void MainWindow::playlistStart()
{
	if (m_playlistFiles.isEmpty())
		rebuildPlaylist();
	if (m_playlistFiles.isEmpty())
		return;

	if (m_playlistRunning)
	{
		m_playlistRunning = false;
		m_playlistCurrentDurationMs = 0;
		m_playlistPausedMs = 0;
		m_playlistPauseStartedMs = -1;
		if (m_playlistProgressTimer)
			m_playlistProgressTimer->stop();
		sb_stopPlayback();
		refreshPlaylistUi();
		return;
	}

	if (m_playlistRandomMode)
	{
		playlistRandom();
		return;
	}

	if (m_playlistCurrentPos < 0)
		m_playlistCurrentPos = 0;
	playPlaylistPosition(m_playlistCurrentPos);
}

void MainWindow::playlistNext()
{
	if (m_playlistFiles.isEmpty())
		return;
	if (m_playlistRandomMode)
	{
		playlistRandom();
		return;
	}
	int nextPos = (m_playlistCurrentPos < 0) ? 0 : (m_playlistCurrentPos + 1) % m_playlistFiles.size();
	playPlaylistPosition(nextPos);
}

void MainWindow::playlistPrev()
{
	if (m_playlistFiles.isEmpty())
		return;
	int prevPos = (m_playlistCurrentPos <= 0) ? (int)m_playlistFiles.size() - 1 : (m_playlistCurrentPos - 1);
	playPlaylistPosition(prevPos);
}

void MainWindow::playlistRandom()
{
	if (m_playlistFiles.isEmpty())
		return;

	if (m_randomRemaining.empty())
	{
		m_randomRemaining.resize((int)m_playlistFiles.size());
		for (int i = 0; i < (int)m_playlistFiles.size(); ++i)
			m_randomRemaining[i] = i;
		if (m_randomRemaining.size() > 1 && m_playlistCurrentPos >= 0)
		{
			auto it = std::find(m_randomRemaining.begin(), m_randomRemaining.end(), m_playlistCurrentPos);
			if (it != m_randomRemaining.end())
				m_randomRemaining.erase(it);
		}
	}

	if (m_randomRemaining.empty())
		return;

	std::uniform_int_distribution<int> dist(0, (int)m_randomRemaining.size() - 1);
	int pick = dist(m_rng);
	int nextPos = m_randomRemaining[pick];
	m_randomRemaining.erase(m_randomRemaining.begin() + pick);
	playPlaylistPosition(nextPos);
}

void MainWindow::addFilesToPlaylist(const QStringList& files)
{
	if (files.isEmpty())
		return;

	PlaylistImportDialogResult metaResult = showPlaylistImportDialog(this);
	if (!metaResult.accepted)
		return;

	int added = 0;
	for (const QString& f : files)
	{
		QFileInfo fi(f);
		if (!fi.exists() || !fi.isFile())
			continue;
		QString abs = fi.absoluteFilePath();
		if (!m_playlistFiles.contains(abs, Qt::CaseInsensitive))
		{
			m_playlistFiles << abs;
			added++;
		}

		PlaylistFileMeta& meta = m_playlistMetaByPath[normalizedPlaylistPath(abs)];
		meta.category = metaResult.category;
		meta.tags = metaResult.tags;
	}

	if (added > 0)
	{
		savePersistentPlaylist();
		rebuildPlaylist();
	}
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
	if (m_playlistView && watched == m_playlistView->viewport())
	{
		if (event->type() == QEvent::DragEnter)
		{
			auto* ev = static_cast<QDragEnterEvent*>(event);
			if (ev->mimeData()->hasUrls())
				ev->acceptProposedAction();
			return true;
		}
		if (event->type() == QEvent::Drop)
		{
			auto* ev = static_cast<QDropEvent*>(event);
			QStringList files;
			for (const QUrl& url : ev->mimeData()->urls())
			{
				if (url.isLocalFile())
				{
					QFileInfo fi(url.toLocalFile());
					if (fi.isFile())
						files << fi.absoluteFilePath();
				}
			}
			if (!files.isEmpty())
			{
				addFilesToPlaylist(files);
				ev->acceptProposedAction();
			}
			return true;
		}
	}
	return QWidget::eventFilter(watched, event);
}

QString MainWindow::playlistStorePath() const
{
	QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	if (base.isEmpty())
		base = QDir::tempPath();
	QDir dir(base);
	dir.mkpath(".");
	return dir.filePath("playlist.json");
}

void MainWindow::loadPersistentPlaylist()
{
	m_playlistFiles.clear();
	m_playlistMetaByPath.clear();
	QFile f(playlistStorePath());
	if (!f.exists())
		return;
	if (!f.open(QIODevice::ReadOnly))
		return;
	QJsonParseError e;
	QJsonDocument d = QJsonDocument::fromJson(f.readAll(), &e);
	if (e.error != QJsonParseError::NoError)
		return;

	if (d.isArray())
	{
		for (const auto& v : d.array())
		{
			QString p = v.toString();
			if (!p.isEmpty())
				m_playlistFiles << p;
		}
		return;
	}

	if (!d.isObject())
		return;
	QJsonObject root = d.object();
	QJsonArray files = root.value("files").toArray();
	for (const auto& v : files)
	{
		if (!v.isObject())
			continue;
		QJsonObject obj = v.toObject();
		QString path = obj.value("path").toString();
		if (path.isEmpty())
			continue;
		m_playlistFiles << path;

		PlaylistFileMeta meta;
		meta.category = obj.value("category").toString().trimmed();
		for (const QJsonValue& tag : obj.value("tags").toArray())
		{
			QString t = tag.toString().trimmed();
			if (!t.isEmpty())
				meta.tags << t;
		}
		if (!meta.category.isEmpty() || !meta.tags.isEmpty())
			m_playlistMetaByPath.insert(normalizedPlaylistPath(path), meta);
	}
}

void MainWindow::savePersistentPlaylist()
{
	QJsonArray files;
	for (const QString& p : m_playlistFiles)
	{
		QJsonObject item;
		item.insert("path", p);
		const PlaylistFileMeta meta = m_playlistMetaByPath.value(normalizedPlaylistPath(p));
		if (!meta.category.isEmpty())
			item.insert("category", meta.category);
		if (!meta.tags.isEmpty())
		{
			QJsonArray tags;
			for (const QString& t : meta.tags)
				tags.append(t);
			item.insert("tags", tags);
		}
		files.append(item);
	}
	QJsonObject root;
	root.insert("version", 2);
	root.insert("files", files);
	QFile f(playlistStorePath());
	if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
		f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

void MainWindow::chooseFile(size_t buttonId)
{
	QString filePath = m_model->getFileName(buttonId);
	QString fn = QFileDialog::getOpenFileName(this, tr("Choose File"), filePath, tr("Files (*.*)"));
	if (fn.isNull())
		return;
	setButtonFile(buttonId, fn);
}


void MainWindow::openAdvanced(size_t buttonId)
{
	const SoundInfo* buttonInfo = m_model->getSoundInfo(buttonId);
	SoundInfo defaultInfo;
	const SoundInfo& info = buttonInfo ? *buttonInfo : defaultInfo;
	SoundSettingsQt dlg(info, buttonId, this);
	dlg.setWindowTitle(QString("Sound %1 Settings").arg(QString::number(buttonId + 1)));
	if (dlg.exec() == QDialog::Accepted)
		m_model->setSoundInfo(buttonId, dlg.getSoundInfo());
}


void MainWindow::deleteButton(size_t buttonId)
{
	const SoundInfo* info = m_model->getSoundInfo(buttonId);
	if (info)
		m_model->setSoundInfo(buttonId, SoundInfo());
}


void MainWindow::createBubbles()
{
	if (m_model->getBubbleButtonsBuild() == 0)
	{
		m_buttonBubble = new SpeechBubble(this);
		m_buttonBubble->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
		m_buttonBubble->setFixedSize(180, 80);
		m_buttonBubble->setText("Right click to choose sound file\nor open advanced options.");
		m_buttonBubble->attachTo(m_buttons[0]);
		connect(m_buttonBubble, SIGNAL(closePressed()), this, SLOT(onButtonBubbleFinished()));
	}

	if (m_model->getBubbleStopBuild() == 0)
	{
		SpeechBubble* stopBubble = new SpeechBubble(this);
		stopBubble->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
		stopBubble->setFixedSize(180, 60);
		stopBubble->setText("Stop the currently playing sound.");
		stopBubble->attachTo(ui->b_stop);
		connect(stopBubble, SIGNAL(closePressed()), this, SLOT(onStopBubbleFinished()));
	}

	if (m_model->getBubbleColsBuild() == 0)
	{
		settingsSection->setExpanded(true);
		SpeechBubble* colsBubble = new SpeechBubble(this);
		colsBubble->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
		colsBubble->setFixedSize(180, 80);
		colsBubble->setText("Change the number of buttons\non the soundboard.");
		colsBubble->attachTo(ui->sb_cols);
		connect(colsBubble, SIGNAL(closePressed()), this, SLOT(onColsBubbleFinished()));
	}
}


void MainWindow::onStopBubbleFinished()
{
	m_model->setBubbleStopBuild(buildinfo_getBuildNumber());
}


void MainWindow::onButtonBubbleFinished()
{
	m_model->setBubbleButtonsBuild(buildinfo_getBuildNumber());
}


void MainWindow::onColsBubbleFinished()
{
	m_model->setBubbleColsBuild(buildinfo_getBuildNumber());
}


void MainWindow::showStopButtonContextMenu(const QPoint& point)
{
	showSetHotkeyMenu(HOTKEY_STOP_ALL, ui->b_stop->mapToGlobal(point));
}


void MainWindow::showPauseButtonContextMenu(const QPoint& point)
{
	showSetHotkeyMenu(HOTKEY_PAUSE_ALL, ui->b_pause->mapToGlobal(point));
}


void MainWindow::showSetHotkeyMenu(const char* hotkeyName, const QPoint& point)
{
	QString hotkeyString = getShortcutString(hotkeyName);
	QString hotkeyText = "Set hotkey (Current: " + (hotkeyString.isEmpty() ? QString("None") : hotkeyString) + ")";

	QMenu menu;
	menu.addAction(hotkeyText);
	QAction* action = menu.exec(point);
	if (action)
		ts3Functions.requestHotkeyInputDialog(getPluginID(), hotkeyName, 0, this);
}


void MainWindow::onStartPlayingSound(bool preview, QString filename)
{
	QFileInfo info(filename);
	ui->playingLabel->setText(info.fileName());
	setPlayingLabelIcon(0);
	ui->playingIconLabel->show();
	playingIconIndex = 1;
	playingIconTimer->start();
	ui->b_stop->setEnabled(true);
	ui->b_pause->setEnabled(true);
	ui->b_pause->setIcon(m_pauseIcon);
	if (!m_playlistRunning)
	{
		m_playlistCurrentDurationMs = 0;
		m_playlistPausedMs = 0;
		m_playlistPauseStartedMs = -1;
		if (m_playlistProgressTimer)
			m_playlistProgressTimer->stop();
		updatePlaylistProgressUi();
	}
}


void MainWindow::onStopPlayingSound()
{
	playingIconTimer->stop();
	ui->playingLabel->setText("");
	ui->playingIconLabel->hide();
	ui->b_pause->setIcon(m_pauseIcon);

	if (m_ignoreNextStopEvent)
	{
		m_ignoreNextStopEvent = false;
		return;
	}

	if (m_playlistRunning && !m_playlistFiles.isEmpty())
	{
		if (m_playlistRandomMode)
			playlistRandom();
		else
		{
			int nextPos = (m_playlistCurrentPos < 0) ? 0 : (m_playlistCurrentPos + 1) % m_playlistFiles.size();
			playPlaylistPosition(nextPos);
		}
	}
	else
	{
		m_playlistCurrentDurationMs = 0;
		m_playlistPausedMs = 0;
		m_playlistPauseStartedMs = -1;
		if (m_playlistProgressTimer)
			m_playlistProgressTimer->stop();
		refreshPlaylistUi();
	}
}


void MainWindow::onPausePlayingSound()
{
	playingIconTimer->stop();
	ui->b_pause->setIcon(m_playIcon);
	if (m_playlistElapsedTimer.isValid() && m_playlistPauseStartedMs < 0)
		m_playlistPauseStartedMs = m_playlistPausedMs + m_playlistElapsedTimer.elapsed();
	updatePlaylistProgressUi();
}


void MainWindow::onUnpausePlayingSound()
{
	playingIconTimer->start();
	ui->b_pause->setIcon(m_pauseIcon);
	if (m_playlistPauseStartedMs >= 0)
	{
		m_playlistPausedMs = m_playlistPauseStartedMs;
		m_playlistPauseStartedMs = -1;
		m_playlistElapsedTimer.restart();
	}
	updatePlaylistProgressUi();
}


void MainWindow::onPlayingIconTimer()
{
	setPlayingLabelIcon(playingIconIndex);
	++playingIconIndex %= 4;
}


void MainWindow::openHotkeySetDialog(size_t buttonId)
{
	openHotkeySetDialog(buttonId, this);
}


void MainWindow::openButtonColorDialog(size_t buttonId)
{
	QColorDialog dialog;
	const SoundInfo* pinfo = m_model->getSoundInfo(buttonId);
	SoundInfo info = pinfo ? *pinfo : SoundInfo();
	dialog.setCurrentColor(info.customColor);
	dialog.setWindowTitle("Choose button color");
	if (dialog.exec())
	{
		info.customColor = dialog.currentColor();
		m_model->setSoundInfo(buttonId, info);
	}
}


QString MainWindow::unescapeCustomText(const QString& text)
{
	QString cpy = text;
	return cpy.replace("\\n", "\n");
}


void MainWindow::openHotkeySetDialog(size_t buttonId, QWidget* parent)
{
	char intName[16];
	sb_getInternalHotkeyName((int)buttonId, intName);
	ts3Functions.requestHotkeyInputDialog(getPluginID(), intName, 0, parent);
}


QString MainWindow::getShortcutString(const char* internalName)
{
	std::vector<char> name(128, 0);
	char* namePtr = name.data();
	unsigned int res = ts3Functions.getHotkeyFromKeyword(getPluginID(), &internalName, &namePtr, 1, 128);
	QString str = res == 0 ? QString(name.data()) : QString();
	return str;
}


QString MainWindow::getShortcutString(size_t buttonId)
{
	char intName[16];
	sb_getInternalHotkeyName((int)buttonId, intName);
	return getShortcutString(intName);
}

QString MainWindow::getConfigShortcutString(int cfg)
{
	char buf[16];
	sb_getInternalConfigHotkeyName(cfg, buf);
	QString shortcut = getShortcutString(buf);
	if (!shortcut.isEmpty())
		return shortcut;

	return QString("no hotkey");
}


void MainWindow::onHotkeyRecordedEvent(const char* keyword, const char* key)
{
	QString sKey = key;

	int configId = -1;
	if (sscanf(keyword, "config_%i", &configId) == 1)
	{
		if (configId >= 0 && configId < NUM_CONFIGS)
			m_configHotkeyButtons[configId]->setToolTip(sKey);
		else
			logError("Invalid hotkey keyword: %s", keyword);
	}
	else
	{
		QString sKeyword = keyword;

		emit hotkeyRecordedEvent(sKey, sKeyword);

		if (m_model->getShowHotkeysOnButtons())
			for (size_t i = 0; i < m_buttons.size(); i++)
				updateButtonText(i);
	}
}


void MainWindow::onUpdateShowHotkeysOnButtons(bool val)
{
	m_model->setShowHotkeysOnButtons(val);
}


void MainWindow::onUpdateHotkeysDisabled(bool val)
{
	m_model->setHotkeysEnabled(!val);
}


void MainWindow::showEvent(QShowEvent* evt)
{
	QWidget::showEvent(evt);

	for (size_t i = 0; i < m_buttons.size(); i++)
		updateButtonText(i);
}


void MainWindow::onButtonFileDropped(const QList<QUrl>& urls)
{
	int buttonId = sender()->property("buttonId").toInt();
	int buttonNr = 0;

	for (int i = 0; i < urls.size(); i++)
	{
		if (buttonNr == 1)
		{
			QMessageBox msgbox(
				QMessageBox::Icon::Question, "Fill buttons?",
				"You dropped multiple files. Consecutively apply them to the buttons following the one you dropped "
				"your files on?",
				QMessageBox::Yes | QMessageBox::No, this
			);
			if (msgbox.exec() == QMessageBox::No)
				break;
		}

		if (urls[i].isLocalFile())
		{
			QFileInfo info(urls[i].toLocalFile());
			if (info.isFile())
			{
				setButtonFile(buttonId + buttonNr, urls[i].toLocalFile(), buttonNr == 0);
				++buttonNr;
			}
			else if (buttonNr == 0)
			{
				QMessageBox msgBox(
					QMessageBox::Icon::Critical, "Unsupported drop type", "Some things could not be dropped here :(",
					QMessageBox::Ok, this
				);
				msgBox.exec();
			}
		}
	}
}


void MainWindow::setButtonFile(size_t buttonId, const QString& fn, bool askForDisablingCrop)
{
	const SoundInfo* info = m_model->getSoundInfo(buttonId);
	if (askForDisablingCrop && info && info->cropEnabled && info->filename != fn)
	{
		QMessageBox mb(
			QMessageBox::Question, "Keep crop settings?",
			"You selected a new file for a button that has 'crop sound' enabled.", QMessageBox::NoButton, this
		);
		QPushButton* btnDisable = mb.addButton("Disable cropping (recommended)", QMessageBox::YesRole);
		QPushButton* btnKeep = mb.addButton("Keep old crop settings", QMessageBox::NoRole);
		mb.setDefaultButton(btnDisable);
		mb.exec();
		if (mb.clickedButton() != btnKeep)
		{
			SoundInfo newInfo(*info);
			newInfo.cropEnabled = false;
			m_model->setSoundInfo(buttonId, newInfo);
		}
	}
	m_model->setFileName(buttonId, fn);
}


void MainWindow::onButtonPausePressed()
{
	sb_pauseButtonPressed();
}

void MainWindow::onPlaylistStartPressed()
{
	playlistStart();
}

void MainWindow::onPlaylistNextPressed()
{
	playlistNext();
}

void MainWindow::onPlaylistPrevPressed()
{
	playlistPrev();
}

void MainWindow::onPlaylistRandomPressed()
{
	m_playlistRandomMode = !m_playlistRandomMode;
	m_randomRemaining.clear();
	refreshPlaylistUi();
}

void MainWindow::onPlaylistAddFilesPressed()
{
	QStringList files = QFileDialog::getOpenFileNames(
		this,
		tr("Add songs to playlist"),
		QString(),
		tr("Audio files (*.mp3 *.wav *.ogg *.flac *.m4a *.aac *.wma *.opus *.mp4);;All files (*.*)")
	);
	if (files.isEmpty())
		return;
	addFilesToPlaylist(files);
}

void MainWindow::onPlaylistRemoveFilesPressed()
{
	if (!m_playlistView)
		return;

	QList<QListWidgetItem*> selected = m_playlistView->selectedItems();
	if (selected.isEmpty())
	{
		QMessageBox::information(this, tr("Playlist"), tr("Select one or more songs to remove."));
		return;
	}

	QList<int> rows;
	rows.reserve(selected.size());
	for (QListWidgetItem* it : selected)
		rows << m_playlistView->row(it);

	std::sort(rows.begin(), rows.end(), std::greater<int>());
	rows.erase(std::unique(rows.begin(), rows.end()), rows.end());

	bool removedCurrent = false;
	for (int row : rows)
	{
		if (row < 0 || row >= m_playlistFiles.size())
			continue;
		if (row == m_playlistCurrentPos)
			removedCurrent = true;
		else if (row < m_playlistCurrentPos)
			m_playlistCurrentPos--;
		const QString removedPath = m_playlistFiles[row];
		m_playlistFiles.removeAt(row);
		m_playlistMetaByPath.remove(normalizedPlaylistPath(removedPath));
	}

	if (m_playlistFiles.isEmpty())
	{
		m_playlistCurrentPos = -1;
		m_playlistRunning = false;
	}
	else
	{
		if (m_playlistCurrentPos < 0 || m_playlistCurrentPos >= m_playlistFiles.size())
			m_playlistCurrentPos = 0;
		if (removedCurrent && m_playlistRunning)
		{
			m_playlistRunning = false;
			sb_stopPlayback();
		}
	}

	m_randomRemaining.clear();
	savePersistentPlaylist();
	rebuildPlaylist();
}

void MainWindow::onPlaylistItemDoubleClicked(QListWidgetItem* item)
{
	if (!item)
		return;
	int row = m_playlistView->row(item);
	playPlaylistPosition(row);
}

void MainWindow::onOpenButtonBoxPressed()
{
	if (!m_buttonBoxWindow)
	{
		m_buttonBoxWindow = new ButtonBoxWindow();
		m_buttonBoxWindow->setDimensions(m_model->getRows(), m_model->getCols());
		connect(m_buttonBoxWindow, SIGNAL(rowsChanged(int)), this, SLOT(onUpdateRows(int)));
		connect(m_buttonBoxWindow, SIGNAL(colsChanged(int)), this, SLOT(onUpdateCols(int)));
		connect(m_buttonBoxWindow, SIGNAL(buttonPlayRequested(int)), this, SLOT(onButtonBoxPlayRequested(int)));
		connect(
			m_buttonBoxWindow, SIGNAL(buttonContextRequested(int, QPoint)), this,
			SLOT(onButtonBoxContextRequested(int, QPoint))
		);
		connect(
			m_buttonBoxWindow, SIGNAL(buttonFileDropped(int, QList<QUrl>)), this,
			SLOT(onButtonBoxFileDropped(int, QList<QUrl>))
		);
		connect(
			m_buttonBoxWindow, SIGNAL(buttonSwapRequested(int, int)), this,
			SLOT(onButtonBoxSwapRequested(int, int))
		);
	}

	for (int i = 0; i < (int)m_buttons.size(); ++i)
	{
		m_buttonBoxWindow->setButtonText(i, m_buttons[i]->text());
		const SoundInfo* info = m_model->getSoundInfo(i);
		m_buttonBoxWindow->setButtonColor(i, info ? info->customColor : QColor(0, 0, 0, 0));
	}

	applyTheme(m_model->getDarkThemeEnabled());
	m_buttonBoxWindow->show();
	m_buttonBoxWindow->raise();
	m_buttonBoxWindow->activateWindow();
}

void MainWindow::applyTheme(bool dark)
{
	if (dark)
	{
		setStyleSheet(
			"QMainWindow, QWidget { background-color: #151923; color: #e7ebf2; }"
			"QGroupBox { border: 1px solid #30384a; border-radius: 4px; margin-top: 6px; padding-top: 6px; }"
			"QGroupBox::title { color: #bfc8d8; subcontrol-origin: margin; left: 8px; padding: 0 2px; }"
			"QPushButton { background-color: #273043; color: #f2f5fa; border: 1px solid #39455d; border-radius: 0px; padding: 4px; min-height: 0px; }"
			"QPushButton:hover { background-color: #31415c; border-color: #4d83ff; }"
			"QPushButton:pressed { background-color: #25324a; }"
			"QPushButton:disabled { background-color: #202635; color: #7f8aa0; border-color: #2f3749; }"
			"QLineEdit, QListWidget, QComboBox, QSpinBox { background-color: #1f2533; color: #e7ebf2; border: 1px solid #344058; border-radius: 0px; padding: 0px; }"
			"QCheckBox, QRadioButton { color: #d7deea; }"
			"QTabWidget::pane { border: 1px solid #30384a; }"
			"QTabBar::tab { background: #222a3a; color: #bfc8d8; padding: 4px 8px; border: 1px solid #30384a; }"
			"QTabBar::tab:selected { background: #2f3d56; color: #f4f7fc; }"
		);
		ui->gridWidget->setStyleSheet(
			"QPushButton { background-color: #2a3142; color: #f2f5fa; border: 1px solid #3a4358; border-radius: 6px; padding: 4px; }"
			"QPushButton:hover { background-color: #33405a; border-color: #4d83ff; }"
			"QPushButton:pressed { background-color: #26324a; }"
			"QPushButton:disabled { background-color: #242938; color: #7f8aa0; border-color: #32384b; }"
		);
		if (m_themeToggleButton)
			m_themeToggleButton->setText("Tema: Scuro");
		if (m_buttonBoxWindow)
			m_buttonBoxWindow->setStyleSheet(
				"QWidget { background-color: #1b1f2a; color: #e8ecf3; }"
				"QLabel { color: #cdd6e3; font-weight: 600; }"
				"QPushButton { background-color: #2a3142; color: #f2f5fa; border: 1px solid #3a4358; border-radius: 0px; padding: 4px; min-height: 0px; }"
				"QPushButton:hover { background-color: #33405a; border-color: #4d83ff; }"
				"QPushButton:pressed { background-color: #26324a; }"
				"QPushButton:disabled { background-color: #242938; color: #7f8aa0; border-color: #32384b; }"
			);
	}
	else
	{
		setStyleSheet("");
		ui->gridWidget->setStyleSheet(
			"QPushButton {"
			"  padding: 4px;"
			"  border: 1px solid rgb(173, 173, 173);"
			"  color: black;"
			"  background-color: rgb(225, 225, 225);"
			"}"
			"QPushButton:disabled {"
			"  background-color: rgb(204, 204, 204);"
			"  border-color: rgb(191, 191, 191);"
			"  color: rgb(120, 120, 120);"
			"}"
			"QPushButton:hover {"
			"  background-color: rgb(228, 239, 249);"
			"  border-color: rgb(11, 123, 212);"
			"}"
			"QPushButton:pressed {"
			"  background-color: rgb(204, 228, 247);"
			"  border-color: rgb(0, 85, 155);"
			"}"
		);
		if (m_themeToggleButton)
			m_themeToggleButton->setText("Tema: Chiaro");
		if (m_buttonBoxWindow)
			m_buttonBoxWindow->setStyleSheet("");
	}
}

void MainWindow::onThemeTogglePressed()
{
	const bool nextDark = !m_model->getDarkThemeEnabled();
	m_model->setDarkThemeEnabled(nextDark);
	applyTheme(nextDark);
}

void MainWindow::onButtonBoxPlayRequested(int buttonId)
{
	if (buttonId < 0)
		return;
	playSound((size_t)buttonId);
}

void MainWindow::onButtonBoxContextRequested(int buttonId, QPoint globalPos)
{
	if (buttonId < 0)
		return;
	showButtonContextMenuForButton((size_t)buttonId, globalPos);
}

void MainWindow::onButtonBoxFileDropped(int buttonId, QList<QUrl> urls)
{
	if (buttonId < 0)
		return;

	for (QUrl url : urls)
	{
		QString fn = url.toLocalFile();
		if (!fn.isNull())
		{
			setButtonFile((size_t)buttonId, fn);
			break;
		}
	}
}

void MainWindow::onButtonBoxSwapRequested(int sourceButtonId, int targetButtonId)
{
	if (sourceButtonId < 0 || targetButtonId < 0)
		return;
	if (sourceButtonId >= (int)m_buttons.size() || targetButtonId >= (int)m_buttons.size())
		return;

	const SoundInfo* info0 = m_model->getSoundInfo(sourceButtonId);
	const SoundInfo* info1 = m_model->getSoundInfo(targetButtonId);
	SoundInfo copy0;
	SoundInfo copy1;
	if (info0)
		copy0 = *info0;
	if (info1)
		copy1 = *info1;
	m_model->setSoundInfo(sourceButtonId, copy1);
	m_model->setSoundInfo(targetButtonId, copy0);
}


void MainWindow::onButtonDroppedOnButton(SoundButton* button)
{
	SoundButton* btn0 = button;
	SoundButton* btn1 = qobject_cast<SoundButton*>(sender());
	int bid0 = btn0->property("buttonId").toInt();
	int bid1 = btn1->property("buttonId").toInt();
	const SoundInfo* info0 = m_model->getSoundInfo(bid0);
	const SoundInfo* info1 = m_model->getSoundInfo(bid1);

	// Copy sound info
	SoundInfo infoCopy0;
	SoundInfo infoCopy1;
	if (info0)
		infoCopy0 = *info0;
	if (info1)
		infoCopy1 = *info1;

	// And switch em
	m_model->setSoundInfo(bid0, infoCopy1);
	m_model->setSoundInfo(bid1, infoCopy0);

	// Switch button position and then animate the buttons to slide into place
	const int animDuration = 300;
	QPropertyAnimation* anim0 = new QPropertyAnimation(btn0, "pos");
	anim0->setStartValue(btn1->pos());
	anim0->setEndValue(btn0->pos());
	anim0->setDuration(animDuration);

	QPropertyAnimation* anim1 = new QPropertyAnimation(btn1, "pos");
	anim1->setStartValue(btn0->pos());
	anim1->setEndValue(btn1->pos());
	anim1->setDuration(animDuration);

	anim0->start();
	anim1->start();
	btn0->raise();
	btn1->raise();
}


void MainWindow::onFilterEditTextChanged(const QString& text)
{
	QString filter = text.trimmed();
	for (auto& button : m_buttons)
	{
		int buttonId = button->property("buttonId").toInt();
		const SoundInfo* info = m_model->getSoundInfo(buttonId);
		bool hasFile = info && !info->filename.isEmpty();
		QString buttonText = button->text();
		bool pass = filter.length() == 0 || (hasFile && buttonText.contains(filter, Qt::CaseInsensitive));
		button->setVisible(pass);
	}
}


void MainWindow::onVolumeSliderContextMenuLocal(const QPoint& point)
{
	QString hotkeyStringIncr = getShortcutString(HOTKEY_VOLUME_INCREASE);
	QString hotkeyTextIncr =
		"Set 'increase 20%' hotkey (Current: " + (hotkeyStringIncr.isEmpty() ? QString("None") : hotkeyStringIncr) +
		")";

	QString hotkeyStringDecr = getShortcutString(HOTKEY_VOLUME_DECREASE);
	QString hotkeyTextDecr =
		"Set 'decrease 20%' hotkey (Current: " + (hotkeyStringDecr.isEmpty() ? QString("None") : hotkeyStringDecr) +
		")";

	QMenu menu;
	QAction* actIncr = menu.addAction(hotkeyTextIncr);
	QAction* actDecr = menu.addAction(hotkeyTextDecr);
	QAction* action = menu.exec(ui->sl_volumeLocal->mapToGlobal(point));
	if (action == actIncr)
		ts3Functions.requestHotkeyInputDialog(getPluginID(), HOTKEY_VOLUME_INCREASE, 0, this);
	else if (action == actDecr)
		ts3Functions.requestHotkeyInputDialog(getPluginID(), HOTKEY_VOLUME_DECREASE, 0, this);
}


void MainWindow::onVolumeSliderContextMenuRemote(const QPoint& point)
{
	QString hotkeyStringIncr = getShortcutString(HOTKEY_VOLUME_INCREASE);
	QString hotkeyTextIncr =
		"Set 'increase 20%' hotkey (Current: " + (hotkeyStringIncr.isEmpty() ? QString("None") : hotkeyStringIncr) +
		")";

	QString hotkeyStringDecr = getShortcutString(HOTKEY_VOLUME_DECREASE);
	QString hotkeyTextDecr =
		"Set 'decrease 20%' hotkey (Current: " + (hotkeyStringDecr.isEmpty() ? QString("None") : hotkeyStringDecr) +
		")";

	QMenu menu;
	QAction* actIncr = menu.addAction(hotkeyTextIncr);
	QAction* actDecr = menu.addAction(hotkeyTextDecr);
	QAction* action = menu.exec(ui->sl_volumeRemote->mapToGlobal(point));
	if (action == actIncr)
		ts3Functions.requestHotkeyInputDialog(getPluginID(), HOTKEY_VOLUME_INCREASE, 0, this);
	else if (action == actDecr)
		ts3Functions.requestHotkeyInputDialog(getPluginID(), HOTKEY_VOLUME_DECREASE, 0, this);
}

void MainWindow::ModelObserver::notify(ConfigModel& model, ConfigModel::notifications_e what, int data)
{
	switch (what)
	{
	case ConfigModel::NOTIFY_SET_ROWS:
		p.ui->sb_rows->setValue(model.getRows());
		p.createButtons();
		p.rebuildPlaylist();
		break;
	case ConfigModel::NOTIFY_SET_COLS:
		p.ui->sb_cols->setValue(model.getCols());
		p.createButtons();
		p.rebuildPlaylist();
		break;
	case ConfigModel::NOTIFY_SET_VOLUME_LOCAL:
		if (p.ui->sl_volumeLocal->value() != model.getVolumeLocal())
			p.ui->sl_volumeLocal->setValue(model.getVolumeLocal());
		break;
	case ConfigModel::NOTIFY_SET_VOLUME_REMOTE:
		if (p.ui->sl_volumeRemote->value() != model.getVolumeRemote())
			p.ui->sl_volumeRemote->setValue(model.getVolumeRemote());
		break;
	case ConfigModel::NOTIFY_SET_PLAYBACK_LOCAL:
		if (p.ui->cb_mute_locally->isChecked() != !model.getPlaybackLocal())
			p.ui->cb_mute_locally->setChecked(!model.getPlaybackLocal());
		break;
	case ConfigModel::NOTIFY_SET_SOUND:
		p.updateButtonText(data);
		p.rebuildPlaylist();
		break;
	case ConfigModel::NOTIFY_SET_MUTE_MYSELF_DURING_PB:
		if (p.ui->cb_mute_myself->isChecked() != model.getMuteMyselfDuringPb())
			p.ui->cb_mute_myself->setChecked(model.getMuteMyselfDuringPb());
		break;
	case ConfigModel::NOTIFY_SET_WINDOW_SIZE:
	{
		QSize s = p.size();
		int w = 0, h = 0;
		model.getWindowSize(&w, &h);
		if (s.width() != w || s.height() != h)
			p.resize(w, h);
	}
	break;
	case ConfigModel::NOTIFY_SET_SHOW_HOTKEYS_ON_BUTTONS:
		if (p.ui->cb_show_hotkeys_on_buttons->isChecked() != model.getShowHotkeysOnButtons())
			p.ui->cb_show_hotkeys_on_buttons->setChecked(model.getShowHotkeysOnButtons());
		for (size_t i = 0; i < p.m_buttons.size(); i++)
			p.updateButtonText(i);
		break;
	case ConfigModel::NOTIFY_SET_HOTKEYS_ENABLED:
		if (p.ui->cb_disable_hotkeys->isChecked() == model.getHotkeysEnabled())
			p.ui->cb_disable_hotkeys->setChecked(!model.getHotkeysEnabled());
		break;
	case ConfigModel::NOTIFY_SET_DARK_THEME_ENABLED:
		p.applyTheme(model.getDarkThemeEnabled());
		break;
	default:
		break;
	}
}

