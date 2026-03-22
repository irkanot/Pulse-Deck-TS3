// src/MainWindow.h
//----------------------------------
// Pulse Deck TS3 Source Code
// Copyright (c) 2015 Davide (Irkanot)
// All rights reserved
// Contact: support@pulse-deck-ts3.local
//----------------------------------

#pragma once

#include <QDialog>
#include <QWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QBoxLayout>
#include <QMenu>
#include <QPointer>
#include <QTimer>
#include <QList>
#include <QUrl>
#include <QRadioButton>
#include <QListWidget>
#include <vector>
#include <random>
#include <QEvent>
#include <QStringList>
#include <QMap>
#include <QProgressBar>
#include <QElapsedTimer>

#include "ui_MainWindow.h"
#include "ConfigModel.h"

class SpeechBubble;
class ExpandableSection;
class SoundButton;
class ButtonBoxWindow;

namespace Ui
{
class MainWindow;
}

class MainWindow : public QWidget
{
	Q_OBJECT

  public:
	explicit MainWindow(ConfigModel* model, QWidget* parent = 0);

	void createBubbles();

	virtual ~MainWindow();

	static QString getShortcutString(const char* internalName);
	static QString getShortcutString(size_t buttonId);
	static QString getConfigShortcutString(int cfg);
	static void openHotkeySetDialog(size_t buttonId, QWidget* parent);
	void onHotkeyRecordedEvent(const char* keyword, const char* key);

	void setConfiguration(int cfg);
	bool hotkeysEnabled();
	void playlistStart();
	void playlistNext();
	void playlistPrev();
	void playlistRandom();

  protected:
	virtual void closeEvent(QCloseEvent* evt) override;
	virtual void showEvent(QShowEvent* evt) override;
	virtual bool eventFilter(QObject* watched, QEvent* event) override;

  private slots:
	void onClickedPlay();
	void onClickedStop();
	void onUpdateVolumeLocal(int val);
	void onUpdateVolumeRemote(int val);
	void onUpdateMuteLocally(bool val);
	void onUpdateCols(int val);
	void onUpdateRows(int val);
	void onUpdateMuteMyself(bool val);
	void showButtonContextMenu(const QPoint& point);
	void onStopBubbleFinished();
	void onButtonBubbleFinished();
	void onColsBubbleFinished();
	void showStopButtonContextMenu(const QPoint& point);
	void showPauseButtonContextMenu(const QPoint& point);
	void onStartPlayingSound(bool preview, QString filename);
	void onStopPlayingSound();
	void onPausePlayingSound();
	void onUnpausePlayingSound();
	void onPlayingIconTimer();
	void onPlaylistProgressTimer();
	void onUpdateShowHotkeysOnButtons(bool val);
	void onUpdateHotkeysDisabled(bool val);
	void onButtonFileDropped(const QList<QUrl>& urls);
	void onButtonPausePressed();
	void onButtonDroppedOnButton(SoundButton* button);
	void onFilterEditTextChanged(const QString& filter);
	void onVolumeSliderContextMenuLocal(const QPoint& point);
	void onVolumeSliderContextMenuRemote(const QPoint& point);

	void onSetConfig();
	void onConfigHotkey();


	void onSaveModel();
	void onLoadModel();
	void onPlaylistStartPressed();
	void onPlaylistNextPressed();
	void onPlaylistPrevPressed();
	void onPlaylistRandomPressed();
	void onPlaylistAddFilesPressed();
	void onPlaylistRemoveFilesPressed();
	void onPlaylistItemDoubleClicked(QListWidgetItem* item);
	void onOpenButtonBoxPressed();
	void onThemeTogglePressed();
	void onButtonBoxPlayRequested(int buttonId);
	void onButtonBoxContextRequested(int buttonId, QPoint globalPos);
	void onButtonBoxFileDropped(int buttonId, QList<QUrl> urls);
	void onButtonBoxSwapRequested(int sourceButtonId, int targetButtonId);

  signals:
	void hotkeyRecordedEvent(QString keyword, QString key);

  private:
	void showSetHotkeyMenu(const char* hotkeyName, const QPoint& point);
	void showButtonContextMenuForButton(size_t buttonId, const QPoint& globalPos);
	void setPlayingLabelIcon(int index);
	void playSound(size_t buttonId);
	void chooseFile(size_t buttonId);

	void setButtonFile(size_t buttonId, const QString& fn, bool askForDisablingCrop = true);

	void openAdvanced(size_t buttonId);
	void deleteButton(size_t buttonId);
	void createButtons();
	void createConfigButtons();
	void updateButtonText(int i);
	void openHotkeySetDialog(size_t buttonId);
	void openButtonColorDialog(size_t buttonId);
	QString unescapeCustomText(const QString& text);
	void rebuildPlaylist();
	void refreshPlaylistUi();
	void playPlaylistPosition(int pos);
	void addFilesToPlaylist(const QStringList& files);
	void loadPersistentPlaylist();
	void savePersistentPlaylist();
	QString playlistStorePath() const;
	void applyTheme(bool dark);
	void updatePlaylistProgressUi();
	QString formatSeconds(qint64 totalSeconds) const;
	qint64 estimateTrackDurationMs(const QString& filePath) const;

	class ModelObserver : public ConfigModel::Observer
	{
	  public:
		ModelObserver(MainWindow& parent) :
			p(parent)
		{
		}
		void notify(ConfigModel& model, ConfigModel::notifications_e what, int data) override;

	  private:
		MainWindow& p;
	};

	Ui::MainWindow* ui;
	std::vector<SoundButton*> m_buttons;
	ConfigModel* m_model;
	QBoxLayout* m_configArea;
	ModelObserver m_modelObserver;
	QMenu m_buttonContextMenu;
	QPointer<SpeechBubble> m_buttonBubble;
	QAction* actSetHotkey;
	ExpandableSection* settingsSection;
	ExpandableSection* configsSection;
	QTimer* playingIconTimer;
	int playingIconIndex;
	QIcon m_pauseIcon;
	QIcon m_playIcon;
	QPushButton* m_playlistStartButton;
	QPushButton* m_playlistPrevButton;
	QPushButton* m_playlistNextButton;
	QPushButton* m_playlistRandomButton;
	QPushButton* m_playlistAddButton;
	QPushButton* m_playlistRemoveButton;
	QPushButton* m_openButtonBoxButton;
	QPushButton* m_themeToggleButton;
	QListWidget* m_playlistView;
	ButtonBoxWindow* m_buttonBoxWindow;
	struct PlaylistFileMeta
	{
		QString category;
		QStringList tags;
	};

	QLabel* m_playlistNowLabel;
	QLabel* m_playlistNextLabel;
	QLabel* m_playlistElapsedLabel;
	QLabel* m_playlistRemainingLabel;
	QProgressBar* m_playlistProgressBar;
	QStringList m_playlistFiles;
	QMap<QString, PlaylistFileMeta> m_playlistMetaByPath;
	std::vector<int> m_randomRemaining;
	int m_playlistCurrentPos;
	bool m_playlistRunning;
	bool m_ignoreNextStopEvent;
	bool m_playlistRandomMode;
	QTimer* m_playlistProgressTimer;
	QElapsedTimer m_playlistElapsedTimer;
	qint64 m_playlistPausedMs;
	qint64 m_playlistPauseStartedMs;
	qint64 m_playlistCurrentDurationMs;
	std::mt19937 m_rng;
	std::array<QRadioButton*, NUM_CONFIGS> m_configRadioButtons;
	std::array<QPushButton*, NUM_CONFIGS> m_configHotkeyButtons;
};

