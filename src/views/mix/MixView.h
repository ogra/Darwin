#pragma once

#include <QWidget>
#include <QList>
#include <QMap>

class Project;
class Track;
class PlaybackController;
class MixerChannelWidget;
class QHBoxLayout;
class QVBoxLayout;
class QLayout;
class VST3PluginInstance;
class PluginEditorWidget;

class MixView : public QWidget
{
    Q_OBJECT
public:
    MixView(QWidget *parent = nullptr);
    void setProject(Project* project);
    void setPlaybackController(PlaybackController* playbackController);

public slots:
    void applyTheme();

private slots:
    void onTrackAdded(Track* track);
    void onTrackRemoved(Track* track);
    void onPluginEditorRequested(VST3PluginInstance* plugin);

private:
    void setupUi();
    void buildMixerChannels();
    void buildFolderGroup(Track* folder, QHBoxLayout* parentLayout, int& trackNumber);
    MixerChannelWidget* createChannelWidget(int trackNumber, Track* track);
    void clearLayout(QLayout* layout);
    void closeCurrentPluginEditor();
    void connectPlaybackMetering(MixerChannelWidget* master);

    Project* m_project;
    PlaybackController* m_playbackController;
    QHBoxLayout* m_mixerLayout;
    QVBoxLayout* m_inspectorLayout;
    PluginEditorWidget* m_currentPluginEditor;
    VST3PluginInstance* m_currentPlugin;
    
    QMap<int, MixerChannelWidget*> m_trackWidgetMap; // trackId → MixerChannelWidget
    QMap<int, MixerChannelWidget*> m_folderWidgets; // folderId → MixerChannelWidget
};
