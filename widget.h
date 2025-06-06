#ifndef WIDGET_H
#define WIDGET_H

#include "qmediaplayer.h"
#include <QWidget>
#include <Qurl>
#include <QLabel>
#include <QImage>
#include <QLineEdit>
#include <QSoundEffect>

#include <QListWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE
class QAudioOutput;
class QMediaPlayer;


class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void on_pushButton_3_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_5_clicked();

    void on_pushButton_clicked();

    void on_listWidget_doubleClicked(const QModelIndex &index);

    void on_pushButton_6_clicked();

    void on_volumeslider_valueChanged(int value);

    void showListContextMenu(const QPoint &pos);

    void deleteSelectedSong();

    void deleteAllSongs();

    void updateMetadata();

    void on_search_clicked();

    void addMusicFiles();

    void addMusicFolder();

    void on_setting_clicked();

    void setWindowsOpacity(int opacity);

private:
    Ui::Widget *ui;
    //定义播放列表
    QList<QUrl> playList;
    int curPlayIndex = 0 ;
    QAudioOutput* audioOutput;
    QMediaPlayer* mediaPlayer;
    //音量相关
    bool isMuted;
    int savedVolume;
    //模式相关
    enum PlayMode{
        repeat,//单曲循环(默认)
        random,//随机播放
        list //列表循环
    };
    PlayMode currentMode = repeat;
    void switchPlayMode();
    void overjudge();
    void playRandMusic();
    void nowplaySong(int);
    void keyPressEvent(QKeyEvent *event)override;
    //部分UI相关
    QImage currentCover;
    QLabel *songLabel;
    //搜索相关
    QLineEdit* searchLineEdit;
    QList<QUrl> originPLayList;
    bool isSearchMode;
    void performSearch();
    void updateListWidget();
    //文件相关
    QUrl currentPlayUrl;
    void addSongsToList(const QStringList &filePaths);
    QMenu* addMenu;
    //拖放相关
    void dragEnterEvent(QDragEnterEvent* event)override;
    void dropEvent(QDropEvent* event)override;
    bool eventFilter(QObject* watched,QEvent* event)override;
    void handleDroppedfiles(const QList<QUrl> &urls);
    //小音效
    QSoundEffect musicsource;
    //音频输出问题
    void resetAudioOutput();
    void handleMediaError(QMediaPlayer::Error error, const QString &errorString);
    //歌词相关
    QString currentLyrics;
    QList<QPair<qint64, QString>> lyricsList;
    int currentLyricsIndex = -1;
    void parseLyrics();
    void updateCurrentLyric(qint64 position);
    void updateLyricsDisplay();
    void highlightCurrentLyric();
    void scrollToCurrentLyric();
    int defaultFontSize = 12;  // 默认字体大小
    int highlightedFontSize = 16; // 高亮字体大小
    //设置相关
    void creatSettingsMenu();
    QMenu* settingsMenu;
};
#endif // WIDGET_H
