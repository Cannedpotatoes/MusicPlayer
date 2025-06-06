#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <QFileDialog>
#include <QDir>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QRandomGenerator>
#include <QtGui>
#include <QKeyEvent>
#include <QMenu>
#include <QtWidgets>
#include <QDate>
#include <QMediaMetaData>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QHBoxLayout>
#include <QDialog>
#include <QGraphicsDropShadowEffect>

//MADE BY CannedPotatoes

//Widget类的构造函数
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , isMuted(false)
    , savedVolume(50)
{
    ui->setupUi(this);

    //焦点设置
    setFocusPolicy(Qt::StrongFocus);
    ui->listWidget->setFocusPolicy(Qt::NoFocus);
    ui->widget->setFocusPolicy(Qt::NoFocus);
    ui->volumeslider->setFocusPolicy(Qt::NoFocus);
    ui->pushButton->setFocusPolicy(Qt::NoFocus);
    ui->pushButton_2->setFocusPolicy(Qt::NoFocus);
    ui->pushButton_3->setFocusPolicy(Qt::NoFocus);
    ui->pushButton_5->setFocusPolicy(Qt::NoFocus);
    ui->pushButton_6->setFocusPolicy(Qt::NoFocus);
    //UI
    ui->pushButton_2->setIcon(QIcon(":/assists/goon.png"));
    ui->pushButton_6->setIcon(QIcon(":/assists/volume.png"));
    ui->pushButton_2->setIconSize(QSize(32, 32));
    ui->pushButton_6->setIconSize(QSize(24, 24));
    ui->mode->setIcon(QIcon(":/assists/mode-repeat.png"));
    ui->mode->setIconSize(QSize(26, 26));
    //初始化音频输出
    audioOutput = new QAudioOutput(this);
    mediaPlayer = new QMediaPlayer(this);
    mediaPlayer->setAudioOutput(audioOutput);
    //初始化音量
    ui->volumeslider->setRange(0,100);
    ui->volumeslider->setValue(savedVolume);
    audioOutput->setVolume(savedVolume/100.0);
    //同步播放时长和进度条
    connect(mediaPlayer,&QMediaPlayer::durationChanged,this,[=](qint64 duration)
            {
                ui->totlabel->setText(QString("%1:%2").arg(duration/1000/60,2,10,QChar('0')).arg(duration/1000%60,2,10,QChar('0')));
                ui->musiccourselSlider->setRange(0,duration); //滑块移动
            });
    connect(mediaPlayer,&QMediaPlayer::positionChanged,this,[=](qint64 pos)
            {
                ui->curlabel->setText(QString("%1:%2").arg(pos/1000/60,2,10,QChar('0')).arg(pos/1000%60,2,10,QChar('0')));
                ui->musiccourselSlider->setValue(pos);//滑块移动
            });
    // 拖动时实时更新位置
    connect(ui->musiccourselSlider, &QSlider::sliderMoved, [=](int value) {
        mediaPlayer->setPosition(value);
    });
    //Mode切换按钮
    connect(ui->mode,&QPushButton::clicked,this,&Widget::switchPlayMode);
    //音乐结束判定连接槽
    connect(mediaPlayer,&QMediaPlayer::mediaStatusChanged,this,[=](QMediaPlayer::MediaStatus status){
        if(status == QMediaPlayer::EndOfMedia){
            overjudge();
        }
    });
    //初始隐藏listwidget界面
    ui->widget_3->hide();
    //启用自定义右键菜单
    ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listWidget,&QListWidget::customContextMenuRequested,this,&Widget::showListContextMenu);
    //设置窗口图标和默认不透明度
    setWindowIcon(QIcon(":/assists/welcome.png"));
    setWindowOpacity(0.9);
    //音乐信息区初始化
    songLabel=ui->label_2;
    songLabel->setText("Pretty things are about to happen");
    ui->artistLabel->setText("欢迎使用此音乐播放器");
    ui->albumLabel->setText("请播放音乐");
    ui->titleLabel->setText("谢谢");
    //封面标签&设置默认的封面
    ui->coverLabel->setAlignment(Qt::AlignCenter);
    QPixmap defaultCover(":/assists/defaultcover.png");
    defaultCover = defaultCover.scaled(ui->coverLabel->size(),Qt::KeepAspectRatioByExpanding,Qt::SmoothTransformation);
    ui->coverLabel->setPixmap(defaultCover);
    //音乐元数据变化连接槽
    connect(mediaPlayer,&QMediaPlayer::mediaStatusChanged,this,&Widget::updateMetadata);
    //搜索和搜索框
    searchLineEdit = ui->searchLine;
    searchLineEdit->setPlaceholderText("请输入歌曲名搜索");
    ui->trash->show();
    searchLineEdit->hide();
    connect(searchLineEdit,&QLineEdit::returnPressed,this,&Widget::performSearch);
    //初始化当前播放歌曲URL
    currentPlayUrl =QUrl();
    //添加音乐菜单
    addMenu = new QMenu(this);
    QAction* addFileAction = addMenu->addAction("添加音乐文件");
    QAction* addFolderAction = addMenu->addAction("添加文件夹");
    connect(addFileAction,&QAction::triggered,this,&Widget::addMusicFiles);
    connect(addFolderAction,&QAction::triggered,this,&Widget::addMusicFolder);
    //拖放和事件过滤:允许一个对象监控并拦截另一个对象的事件
    setAcceptDrops(true);
    ui->listWidget->setAcceptDrops(true);
    ui->listWidget->installEventFilter(this);
    //一个音效
    musicsource.setSource(QUrl::fromLocalFile(":/assists/sourcemusic.wav"));
    musicsource.setVolume(0.6);
    //音频错误处理连接槽
    connect(mediaPlayer, &QMediaPlayer::errorOccurred,this,&Widget::handleMediaError);
    // 歌词框样式
    ui->lyricsWidget->setFrameShape(QFrame::NoFrame);
    ui->lyricsWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->lyricsWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->lyricsWidget->setSelectionMode(QAbstractItemView::NoSelection);
    ui->lyricsWidget->setFocusPolicy(Qt::NoFocus);
    // 设置歌词样式
    ui->lyricsWidget->setStyleSheet(
        "QListWidget { background: transparent; border: none; }"
        "QListWidget::item { color: #888888; padding: 8px; border: none; font-size: 1em; text-align: center; white-space: pre-wrap }"
        "QListWidget::item:selected { background: transparent; color: #FF5500; font-weight: bold; }"
        );
    ui->lyricsWidget->setWordWrap(true);
    ui->lyricsWidget->setSpacing(2);
    // 连接位置变化信号到歌词更新
    connect(mediaPlayer, &QMediaPlayer::positionChanged, this, &Widget::updateCurrentLyric);
    //设置菜单
    creatSettingsMenu();
    connect(ui->setting, &QPushButton::clicked, this, &Widget::on_setting_clicked);
}

//Widget类的析构函数
Widget::~Widget()
{
    delete ui;
}

//打开文件操作
void Widget::on_pushButton_3_clicked()
{
    addMenu->exec(ui->pushButton_3->mapToGlobal(QPoint(0,ui->pushButton_3->height())));
}

//播放与暂停
void Widget::on_pushButton_2_clicked()
{
    if(playList.empty())
    {
        return;
    }
    switch(mediaPlayer->playbackState()){
    case QMediaPlayer::PlaybackState::StoppedState://停止
    {
        resetAudioOutput();
        //如果没有播放，就播放当前选中的音乐
        //1.获取选中的行号
        curPlayIndex = ui->listWidget->currentRow();
        //2.播放对应下标的音乐
        ui->pushButton_2->setIcon(QIcon(":/assists/pause.png"));
        mediaPlayer->setSource(playList[curPlayIndex]);
        mediaPlayer->play();
        break;
    }
    case QMediaPlayer::PlaybackState::PlayingState://正在
        //如果正在播放，暂停
        ui->pushButton_2->setIcon(QIcon(":/assists/goon.png"));
        mediaPlayer->pause();
        break;
    case QMediaPlayer::PlaybackState::PausedState://暂停
        //如果暂停播放，继续
        ui->pushButton_2->setIcon(QIcon(":/assists/pause.png"));
        mediaPlayer->play();
        break;
    }
}

//通用播放函数
void Widget::nowplaySong(int index)
{
    ui->coverLabel->setPixmap(QPixmap(":/assists/loading.png"));
    ui->artistLabel->clear();
    ui->albumLabel->clear();
    resetAudioOutput();
    ui->listWidget->setCurrentRow(index);
    mediaPlayer->setSource(playList[index]);
    mediaPlayer->play();
    ui->pushButton_2->setIcon(QIcon(":/assists/pause.png"));
    currentPlayUrl = playList[index];
    if(songLabel){
        QUrl currentSong = playList[index];
        songLabel->setText(currentSong.fileName().left(currentSong.fileName().lastIndexOf('.')));
    }
    QUrl songUrl = playList[index];
    if (songUrl.isLocalFile()) {
        QString filePath = songUrl.toLocalFile();

        // 构建歌词文件路径
        QFileInfo fileInfo(filePath);
        QString lrcPath = fileInfo.path() + "/" + fileInfo.completeBaseName() + ".lrc";

        // 检查歌词文件是否存在
        if (QFile::exists(lrcPath)) {
            QFile lrcFile(lrcPath);
            if (lrcFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream stream(&lrcFile);
                currentLyrics = stream.readAll();
                lrcFile.close();
            } else {
                currentLyrics = "无法打开歌词文件";
            }
        } else {
            currentLyrics = "暂无歌词";
        }
    } else {
        currentLyrics = "仅支持本地文件";
    }

    parseLyrics();
    updateLyricsDisplay();
}

//上一曲
void Widget::on_pushButton_5_clicked()
{
    if(playList.empty()) return;
    if(currentMode==random){
        playRandMusic();
    }else{
        curPlayIndex=(curPlayIndex-1+playList.size())%playList.size();
        nowplaySong(curPlayIndex);
    }
}

//下一曲
void Widget::on_pushButton_clicked()
{
    if(playList.empty()) return;
    if(currentMode==random){
        playRandMusic();
    }else{
        curPlayIndex=(curPlayIndex+1+playList.size())%playList.size();
        nowplaySong(curPlayIndex);
    }
}

//双击切换音乐
void Widget::on_listWidget_doubleClicked(const QModelIndex &index)
{
    curPlayIndex = index.row();
    ui->pushButton_2->setIcon(QIcon(":/assists/pause.png"));
    nowplaySong(curPlayIndex);
}

//设置静音
void Widget::on_pushButton_6_clicked()
{
    if(isMuted){ //取消静音，恢复音量
        ui->pushButton_6->setIcon(QIcon(":/assists/volume1.png"));
        isMuted = false;
        ui->volumeslider->setValue(savedVolume);
        audioOutput->setVolume(savedVolume/100.0);
    }else{//静音
        ui->pushButton_6->setIcon(QIcon(":/assists/isMuted.png"));
        isMuted = true;
        savedVolume = ui->volumeslider->value();
        ui->volumeslider->setValue(0);
        audioOutput->setVolume(0);
    }
}

//滑块设置音量
void Widget::on_volumeslider_valueChanged(int value)
{

    audioOutput->setVolume(value/100.0);

    if(value ==0){
        ui->pushButton_6->setIcon(QIcon(":/assists/isMuted.png"));
    }else{
        if(value <= 33){
            ui->pushButton_6->setIcon(QIcon(":/assists/volume1.png"));
        }if(value >33 && value <= 66){
            ui->pushButton_6->setIcon(QIcon(":/assists/volume2.png"));
        }if(value >66 && value <= 100){
            ui->pushButton_6->setIcon(QIcon(":/assists/volume3.png"));
        }
    }

    if(isMuted == true && value>0){
        isMuted = false;
    }
}

//切换音乐模式
void Widget::switchPlayMode()
{
    currentMode = static_cast<PlayMode>((currentMode+1)%3);
    switch(currentMode){
    case repeat:
        ui->mode->setIcon(QIcon(":/assists/mode-repeat.png"));
        break;
    case random:
        ui->mode->setIcon(QIcon(":/assists/mode-rand.png"));
        break;
    case list:
        ui->mode->setIcon(QIcon(":/assists/mode-list.png"));
        break;
    }
}

//音乐结束后的判定处理
void Widget::overjudge()
{
    switch(currentMode){
    case repeat:
        mediaPlayer->setPosition(0);
        resetAudioOutput();
        mediaPlayer->play();
        ui->pushButton_2->setIcon(QIcon(":/assists/pause.png"));
        break;
    case random:
        resetAudioOutput();
        playRandMusic();
        break;
    case list:
        curPlayIndex = (curPlayIndex+1)%playList.size();
        resetAudioOutput();
        nowplaySong(curPlayIndex);
        break;
    }
}

//随机播放
void Widget::playRandMusic()
{
    if(playList.empty()) return;
    int newIndex;
    do{
        newIndex= QRandomGenerator::global()->bounded(playList.size());
    }while(newIndex == curPlayIndex && playList.size()>1);
    curPlayIndex = newIndex;
    nowplaySong(curPlayIndex);
}

//键盘监控
void Widget::keyPressEvent(QKeyEvent *event)
{
    if (playList.isEmpty()) return;
    //上下左右改变进度和音量大小，ESC退出搜索
    if (event->key() == Qt::Key_Up) {
        //音频系统（0.0-1.0）和滑块（0-100）需要同步：
        int slidervalue = ui->volumeslider->value();
        int newvalue= qMin(100,slidervalue + 10 );
        audioOutput->setVolume(newvalue/100);
        ui->volumeslider->setValue(newvalue);
    } else if (event->key() == Qt::Key_Down) {
        //音频系统（0.0-1.0）和滑块（0-100）需要同步：
        int slidervalue = ui->volumeslider->value();
        int newvalue= qMin(100,slidervalue - 10 );
        audioOutput->setVolume(newvalue/100);
        ui->volumeslider->setValue(newvalue);
    }else if (event->key() == Qt::Key_Left) {
        mediaPlayer->setPosition(qMax(0, mediaPlayer->position() - 10000)); // -10秒
    } else if (event->key() == Qt::Key_Right) {
        mediaPlayer->setPosition(qMin(mediaPlayer->duration(), mediaPlayer->position() + 10000)); // +10秒
    }else {
        QWidget::keyPressEvent(event);
    }
    if(event->key() == Qt::Key_Escape && searchLineEdit->isVisible()){
        ui->trash->show();
        searchLineEdit->hide();
        if(!originPLayList.isEmpty()){
            playList=originPLayList;
            updateListWidget();
            isSearchMode=false;
        }
        event->accept();
        return;
    }
    //F5上一曲 F6暂停 F7下一曲 F8静音 F10模式1 F11模式2 F12模式3 ESC返回默认模式
    if (event->key() == Qt::Key_F5) {
        on_pushButton_5_clicked();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_F6) {
        on_pushButton_2_clicked();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_F7) {
        on_pushButton_clicked();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_F8) {
        on_pushButton_6_clicked();
        event->accept();
        return;
    }
    if(event->key() == Qt::Key_F10){
        defaultFontSize = 16;  // 全屏默认字体大小
        highlightedFontSize = 20; // 全屏高亮字体大小
        highlightCurrentLyric();
        ui->search->hide();
        ui->pushButton_3->hide();
        ui->trash->hide();
        ui->searchLine->hide();
        ui->titleLabel->hide();
        ui->listWidget->hide();
        ui->albumLabel->hide();
        ui->artistLabel->hide();
        highlightCurrentLyric();
    }else if(event->key() == Qt::Key_Escape){
        defaultFontSize = 12;  // 默认字体大小
        highlightedFontSize = 16; // 高亮字体大小
        highlightCurrentLyric();
        ui->search->show();
        ui->pushButton_3->show();
        ui->trash->show();
        ui->titleLabel->show();
        ui->listWidget->show();
        ui->albumLabel->show();
        ui->artistLabel->show();
        highlightCurrentLyric();
    }
    if(event->key() == Qt::Key_F11){
        defaultFontSize = 16;  // 全屏默认字体大小
        highlightedFontSize = 20; // 全屏高亮字体大小
        highlightCurrentLyric();
        ui->search->hide();
        ui->pushButton_3->hide();
        ui->widget_2->hide();
        ui->widget->hide();
        ui->lyric->hide();
        ui->titleLabel->hide();
        ui->listWidget->hide();
        ui->albumLabel->hide();
        ui->artistLabel->hide();
        highlightCurrentLyric();
    }else if(event->key() == Qt::Key_Escape){
        defaultFontSize = 12;  // 默认字体大小
        highlightedFontSize = 16; // 高亮字体大小
        highlightCurrentLyric();
        ui->search->show();
        ui->pushButton_3->show();
        ui->widget_2->hide();
        ui->widget->hide();
        ui->lyric->hide();
        ui->titleLabel->show();
        ui->listWidget->show();
        ui->albumLabel->show();
        ui->artistLabel->show();
        highlightCurrentLyric();
    }
    if(event->key() == Qt::Key_F12){
        setWindowState(Qt::WindowFullScreen);
        defaultFontSize = 16;  // 全屏默认字体大小
        highlightedFontSize = 20; // 全屏高亮字体大小
        highlightCurrentLyric();
        ui->search->hide();
        ui->pushButton_3->hide();
        ui->widget_2->hide();
        ui->widget->hide();
        ui->lyric->hide();
        ui->listWidget->hide();
        ui->titleLabel->hide();
        ui->albumLabel->hide();
        ui->artistLabel->hide();
        highlightCurrentLyric();
    }else if(event->key() == Qt::Key_Escape){
        setWindowState(Qt::WindowNoState);
        defaultFontSize = 12;  // 默认字体大小
        highlightedFontSize = 16; // 高亮字体大小
        highlightCurrentLyric();
        ui->search->show();
        ui->pushButton_3->show();
        ui->widget_2->show();
        ui->widget->show();
        ui->lyric->show();
        ui->listWidget->show();
        ui->titleLabel->show();
        ui->albumLabel->show();
        ui->artistLabel->show();
        highlightCurrentLyric();
    }
}

//右键菜单
void Widget::showListContextMenu(const QPoint &pos)
{
    QMenu contextMenu(this);
    //删除选中选项
    QAction* deleAction = contextMenu.addAction("删除选中歌曲",this,&Widget::deleteSelectedSong);
    deleAction->setEnabled(ui->listWidget->currentRow()>=0);
    //删除所有选项
    contextMenu.addAction("删除所有歌曲",this,&Widget::deleteAllSongs);
    //坐标映射
    contextMenu.exec(ui->listWidget->mapToGlobal(pos));
}

//删除单个歌曲实现
void Widget::deleteSelectedSong()
{
    int currentRow=ui->listWidget->currentRow();
    if(currentRow<0) return;

    if(!originPLayList.isEmpty()){
        QUrl removedUrl = playList[currentRow];
        for(int i=0;i<originPLayList.size();++i){
            if(originPLayList[i] == removedUrl){
                originPLayList.removeAt(i);
                break;
            }
        }
    }
    if(currentRow == curPlayIndex){
        mediaPlayer->stop();
        currentPlayUrl = QUrl(0);
        songLabel->setText("Pretty things are about to happen");
        ui->artistLabel->setText("欢迎使用此音乐播放器");
        ui->albumLabel->setText("请播放音乐");
        ui->titleLabel->setText("谢谢");
        QPixmap defaultCover(":/assists/defaultcover.png");
        defaultCover = defaultCover.scaled(ui->coverLabel->size(),Qt::KeepAspectRatioByExpanding,Qt::SmoothTransformation);
        ui->coverLabel->setPixmap(defaultCover);
        ui->pushButton_2->setIcon(QIcon(":/assists/goon.png"));
        curPlayIndex = -1;
        lyricsList.clear();
        ui->lyricsWidget->clear();

        if (currentLyrics.isEmpty() || currentLyrics == "暂无歌词") {
            lyricsList.append(qMakePair(0, "暂无歌词"));
            ui->lyricsWidget->addItem("暂无歌词");
            return;
        }
    }

    playList.removeAt(currentRow);
    delete ui->listWidget->takeItem(currentRow);

    //更新一下
    if(curPlayIndex>currentRow){
        curPlayIndex--;
    }
    if(playList.empty()){
        ui->widget_3->hide();
        ui->welcome->show();
    }
}

//删除所有歌曲实现
void Widget::deleteAllSongs()
{
    if(playList.isEmpty())return;

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this,"确认删除","确定要删除所有歌曲吗?",QMessageBox::Yes|QMessageBox::No);

    if(reply == QMessageBox::No) return;
    mediaPlayer->stop();
    musicsource.play();
    ui->artistLabel->setText("欢迎使用此音乐播放器");
    ui->albumLabel->setText("请播放音乐");
    ui->titleLabel->setText("谢谢");
    currentCover = QImage(":/assists/defaultcover.png");
    QPixmap coverPix = QPixmap::fromImage(currentCover).scaled(ui->coverLabel->size(),Qt::KeepAspectRatioByExpanding,Qt::SmoothTransformation);
    ui->coverLabel->setPixmap(coverPix);
    ui->pushButton_2->setIcon(QIcon(":/assists/goon.png"));
    playList.clear();
    ui->listWidget->clear();
    curPlayIndex = -1;
    ui->widget_3->hide();
    ui->welcome->show();
    if(songLabel){
        songLabel->setText("Pretty things are about to happen");
    }
    originPLayList.clear();
    isSearchMode=false;
    searchLineEdit->hide();
    currentPlayUrl=QUrl(0);
    curPlayIndex=-1;

    lyricsList.clear();
    ui->lyricsWidget->clear();
}

//更新音乐元数据
void Widget::updateMetadata()
{
    if(!mediaPlayer->source().isValid()) return;
    //基本的元数据
    QString title = mediaPlayer->metaData().value(QMediaMetaData::Title).toString();
    QString artist = mediaPlayer->metaData().value(QMediaMetaData::ContributingArtist).toString();
    QString album = mediaPlayer->metaData().value(QMediaMetaData::AlbumTitle).toString();
    //获取封面
    QVariant coverVar = mediaPlayer->metaData().value(QMediaMetaData::ThumbnailImage);

    if(coverVar.isValid()){
        currentCover = coverVar.value<QImage>();
    }else{
        currentCover = QImage(":/assists/defaultcover.png");
    }
    //其他的UI
    if(title.isEmpty()){
        QUrl currentUrl = mediaPlayer->source();
        title = currentUrl.fileName();
        title = title.left(title.lastIndexOf('.'));
    }
    ui->titleLabel->setText(title.isEmpty()?"未知音乐":title);
    ui->artistLabel->setText(artist.isEmpty()?"未知艺术家": artist);
    QString albumInfo = album;
    ui->albumLabel->setText(albumInfo.isEmpty()?"未知专辑": "专辑:"+albumInfo);
    QPixmap coverPix = QPixmap::fromImage(currentCover).scaled(ui->coverLabel->size(),Qt::KeepAspectRatioByExpanding,Qt::SmoothTransformation);
    ui->coverLabel->setPixmap(coverPix);
}

//点击搜索
void Widget::on_search_clicked()
{
    if(!searchLineEdit->isVisible()){
        ui->trash->hide();
        searchLineEdit->show();
        searchLineEdit->setFocus();
        if(!isSearchMode && !playList.isEmpty()){
            originPLayList = playList;
        }
    }else{
        performSearch();
    }
}

//搜索实现
void Widget::performSearch()
{
    QString searchSong = searchLineEdit->text().trimmed();
    if(searchSong.isEmpty()){
        if(!originPLayList.isEmpty()){
            playList = originPLayList;
            updateListWidget();
            isSearchMode =false;
        }
        ui->trash->show();
        searchLineEdit->hide();
        return;
    }
    //保存URL
    if(mediaPlayer->playbackState() == QMediaPlayer::PlayingState && mediaPlayer->source().isValid()){
        currentPlayUrl=mediaPlayer->source();
    }
    isSearchMode = true;
    QList<QUrl> fileList;
    QStringList fileNames;
    for(int i=0; i < originPLayList.size();++i){
        QUrl url = originPLayList[i];
        QString fileName = QFileInfo(url.fileName()).baseName();
        if(fileName.contains(searchSong,Qt::CaseInsensitive)){
            fileList.append(url);
            fileNames.append(fileName);
        }
    }
    //更新列表
    playList = fileList;
    ui->listWidget->clear();
    ui->listWidget->addItems(fileNames);
    if(playList.isEmpty()){
        ui->listWidget->addItem("未找到匹配歌曲");
        ui->listWidget->item(0)->setForeground(Qt::black);
        ui->listWidget->item(0)->setFlags(Qt::NoItemFlags);
    }
    if(ui->widget_3->isHidden() && !playList.isEmpty()){
        ui->widget_3->show();
        ui->welcome->hide();
    }
}

//更新列表
void Widget::updateListWidget()
{
    ui->listWidget->clear();
    QStringList showNames;
    for(const QUrl &url : playList){
        showNames.append(QFileInfo(url.fileName()).baseName());
    }
    ui->listWidget->addItems(showNames);
    //恢复选中
    if(currentPlayUrl.isValid()){
        int index=playList.indexOf(currentPlayUrl);
        if(index != -1){
            ui->listWidget->setCurrentRow(index);
            curPlayIndex=index;
        }
    }
}

//添加音乐文件
void Widget::addMusicFiles()
{
    QStringList filePath=QFileDialog::getOpenFileNames(this,"请选择音乐文件","C:\\","音乐文件(*.mp3 *.wav *.flac)");
    if(!filePath.isEmpty()){
        addSongsToList(filePath);
    }
}

//添加音乐文件夹
void Widget::addMusicFolder()
{
    auto path = QFileDialog::getExistingDirectory(this,"请选择音乐所在文件夹","C:\\");
    if(!path.isEmpty()){
        QDir dir(path);
        QStringList musicfiles = dir.entryList(QStringList()<<"*.mp3"<<"*.wav"<<"*.flac",QDir::Files);
        QStringList fullPath;
        for(const QString &file : musicfiles){
            fullPath.append(dir.filePath(file));
        }
        addSongsToList(fullPath);
    }
}

//通用添加函数
void Widget::addSongsToList(const QStringList &filePath)
{
    ui->welcome->hide();
    ui->widget_3->show();
    int addednum =0;

    for(const QString &filePath : filePath){
        QUrl fileUrl = QUrl::fromLocalFile(filePath);
        if(!playList.contains(fileUrl)){
            playList.append(fileUrl);
            QString baseName = QFileInfo(filePath).baseName();
            ui->listWidget->addItem(baseName);
            addednum++;
        }
    }
    if(addednum>0){
        ui->listWidget->setCurrentRow(ui->listWidget->count()-1);
        QString message;
        if(addednum==filePath.size()){
            message=QString("成功添加%1首歌曲").arg(addednum);
        }else{
            message=QString("成功添加%1首歌曲(跳过%2首重复歌曲)").arg(addednum).arg(filePath.size()-addednum);
        }
        QMessageBox::information(this,"添加完成",message);
    }
}

//拖拽进入判断
void Widget::dragEnterEvent(QDragEnterEvent* event)
{
    if(event->mimeData()->hasUrls()){
        event->acceptProposedAction();
    }else{
        event->ignore();
    }
}

//拖拽处理
void Widget::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();
    if(mimeData->hasUrls()){
        QList<QUrl> urlList = mimeData->urls();
        handleDroppedfiles(urlList);
        event->acceptProposedAction();
    }else{
        event->ignore();
    }
}

//拖拽文件处理
void Widget::handleDroppedfiles(const QList<QUrl> &urls)
{
    QStringList filePaths;
    for(const QUrl &url : urls){
        if(url.isLocalFile()){
            QString filePath = url.toLocalFile();
            QFileInfo fileInfo(filePath);
            if(fileInfo.isDir()){
                QDir dir(filePath);
                QStringList musicFiles =dir.entryList(QStringList()<<"*.mp3"<<"*.wav"<<"*.flac",QDir::Files);
                for(const QString &file : musicFiles){
                    filePaths.append(dir.filePath(file));
                }
            }else{
                //检查是否为支持文件
                QString suffix = fileInfo.suffix().toLower();
                if(suffix == "mp3"||suffix == "wav"||suffix == "flac"){
                    filePaths.append(filePath);
                }
            }
        }
    }
    if(!filePaths.isEmpty()){
        addSongsToList(filePaths);
    }
}

//拖进事件过滤
bool Widget::eventFilter(QObject* watched,QEvent* event)
{
    if(watched == ui->listWidget){
        if(event->type() == QEvent::DragEnter){
            QDragEnterEvent* dragEvent = static_cast<QDragEnterEvent*>(event);
            if(dragEvent->mimeData()->hasUrls()){
                dragEvent->acceptProposedAction();
                return true;
            }
        }
        else if(event->type() == QEvent::Drop){
            QDropEvent* dropEvent = static_cast<QDropEvent*>(event);
            const QMimeData* mimeData=dropEvent->mimeData();
            if(mimeData->hasUrls()){
                QList<QUrl> urlList=mimeData->urls();
                handleDroppedfiles(urlList);
                dropEvent->acceptProposedAction();
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched,event);
}

//音频输出设置
void Widget::resetAudioOutput()
{
    if(audioOutput){
        mediaPlayer->setAudioOutput(nullptr);
        delete audioOutput;
        audioOutput = nullptr;
    }
    audioOutput = new QAudioOutput(this);
    audioOutput->setVolume(savedVolume/100.0);
    mediaPlayer->setAudioOutput(audioOutput);
}

//音频输出错误处理
void Widget::handleMediaError(QMediaPlayer::Error error, const QString &errorString)
{
    qDebug()<<"音频输出错误:"<<errorString;
    if(error == QMediaPlayer::ResourceError){
        resetAudioOutput();
        mediaPlayer->play();
    }
}

//歌词解析
void Widget::parseLyrics()
{
    lyricsList.clear();
    ui->lyricsWidget->clear();

    if (currentLyrics.isEmpty() || currentLyrics == "暂无歌词") {
        lyricsList.append(qMakePair(0, "暂无歌词"));
        ui->lyricsWidget->addItem("暂无歌词");
        return;
    }
    // 歌词解析
    QStringList lines = currentLyrics.split('\n');
    QRegularExpression timeRegex(R"(\[(\d+):(\d+)(?:\.(\d+))?\](.*))");

    for (const QString &line : lines) {
        if (line.trimmed().isEmpty()) continue;

        QRegularExpressionMatchIterator it = timeRegex.globalMatch(line);
        bool hasTimeTag = false;

        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            if (match.hasMatch()) {
                qint64 min = match.captured(1).toLongLong();
                qint64 sec = match.captured(2).toLongLong();
                qint64 ms = match.captured(3).leftJustified(3, '0').toLongLong();
                QString text = match.captured(4).trimmed();

                qint64 time = (min * 60 + sec) * 1000 + ms;
                lyricsList.append(qMakePair(time, text));
                hasTimeTag = true;
            }
        }

        // 没有时间标签的行 静态歌词
        if (!hasTimeTag && !line.trimmed().isEmpty()) {
            lyricsList.append(qMakePair(-1, line.trimmed()));
        }
    }

    // 按时间排序
    std::sort(lyricsList.begin(), lyricsList.end(),
              [](const QPair<qint64, QString> &a, const QPair<qint64, QString> &b) {
                  return a.first < b.first;
              });

    // 添加到列表
    for (const auto &lyric : lyricsList) {
        ui->lyricsWidget->addItem(lyric.second);
    }
}

//歌词更新
void Widget::updateCurrentLyric(qint64 position)
{
    if (lyricsList.isEmpty()) return;

    int newIndex = -1;
    int lastValidIndex = -1;  // 记录最后一个有效歌词行

    for (int i = 0; i < lyricsList.size(); i++) {
        const auto& lyric = lyricsList[i];

        // 静态歌词（无时间标签）
        if (lyric.first < 0) {
            // 记录位置
            lastValidIndex = i;
            continue;
        }

        // 找到当前时间点对应的歌词
        if (position >= lyric.first) {
            newIndex = i;
            lastValidIndex = i;  // 更新最后一个有效位置
        }
    }

    // 如果没有找到动态歌词，使用最后的静态歌词
    if (newIndex == -1 && lastValidIndex != -1) {
        newIndex = lastValidIndex;
    }

    if (newIndex == currentLyricsIndex) return;

    currentLyricsIndex = newIndex;
    highlightCurrentLyric();
    scrollToCurrentLyric();
}

//歌词显示
void Widget::updateLyricsDisplay()
{
    ui->lyricsWidget->clear();

    if (lyricsList.isEmpty()) {
        ui->lyricsWidget->addItem("暂无歌词");
        return;
    }

    // 显示所有歌词（包括静态歌词）
    for (const auto &lyric : lyricsList) {
        ui->lyricsWidget->addItem(lyric.second);
    }

    currentLyricsIndex = -1;
}

//歌词高亮
void Widget::highlightCurrentLyric()
{

    // 清除所有高亮
    for (int i = 0; i < ui->lyricsWidget->count(); i++) {
        QListWidgetItem* item = ui->lyricsWidget->item(i);
        QFont font = item->font();
        font.setPointSize(defaultFontSize);
        font.setBold(false);
        item->setFont(font);
        item->setForeground(QColor("#888888"));
    }

    // 高亮行
    if (currentLyricsIndex >= 0 && currentLyricsIndex < ui->lyricsWidget->count()) {
        QListWidgetItem* currentItem = ui->lyricsWidget->item(currentLyricsIndex);
        QFont font = currentItem->font();
        font.setPointSize(highlightedFontSize);
        font.setBold(true);
        currentItem->setFont(font);
        currentItem->setForeground(QColor("#FF5500"));

        // 确保当前行居中显示
        ui->lyricsWidget->scrollToItem(currentItem, QAbstractItemView::PositionAtCenter);
    }
}

//歌词视图
void Widget::scrollToCurrentLyric()
{
    if (currentLyricsIndex >= 0 && currentLyricsIndex < ui->lyricsWidget->count()) {
        // 确保当前歌词行在视图中央
        ui->lyricsWidget->scrollToItem(ui->lyricsWidget->item(currentLyricsIndex),QAbstractItemView::PositionAtCenter);
    }
}

//设置点击
void Widget::on_setting_clicked()
{
    // 在按钮下方显示菜单
    settingsMenu->exec(ui->setting->mapToGlobal(
        QPoint(0, ui->setting->height())));
}

//设置菜单
void Widget::creatSettingsMenu()
{
    settingsMenu = new QMenu(this);

    settingsMenu->setStyleSheet(
        "QMenu {"
        "   background-color: #333333;"
        "   color: white;"
        "   border: 1px solid #555;"
        "}"
        "QMenu::item {"
        "   background-color: transparent;"
        "   padding: 5px 15px;"
        "}"
        "QMenu::item:selected {"
        "   background-color: #555;"
        "}"
        "QLabel { color: white; }"
        "}"
        );

    // 创建不透明度滑块区域
    QWidget *opacityWidget = new QWidget(settingsMenu);
    QHBoxLayout *opacityLayout = new QHBoxLayout(opacityWidget);

    QLabel *opacityLabel = new QLabel("不透明度:", opacityWidget);
    QSlider *opacitySlider = new QSlider(Qt::Horizontal, opacityWidget);
    opacitySlider->setRange(30, 100); // 30%-100%
    opacitySlider->setValue(static_cast<int>(windowOpacity() * 100));

    connect(opacitySlider, &QSlider::valueChanged, this, &Widget::setWindowsOpacity);

    opacityLayout->addWidget(opacityLabel);
    opacityLayout->addWidget(opacitySlider);
    opacityWidget->setLayout(opacityLayout);

    // 将滑块区域添加到菜单
    QWidgetAction *opacityAction = new QWidgetAction(settingsMenu);
    opacityAction->setDefaultWidget(opacityWidget);
    settingsMenu->addAction(opacityAction);

    // 添加分隔线
    settingsMenu->addSeparator();

    // 添加快捷键提示区域
    QWidget *shortcutWidget = new QWidget(settingsMenu);
    QVBoxLayout *shortcutLayout = new QVBoxLayout(shortcutWidget);

    // 创建快捷键列表
    QLabel *shortcutInfo = new QLabel(
        "F5: 上一曲\n"
        "F6: 暂停/播放\n"
        "F7: 下一曲\n"
        "F8: 静音\n"
        "F10: 模式1\n"
        "F11: 模式2\n"
        "F12: 模式3\n"
        "ESC: 恢复默认模式"
        );

    // 设置文本格式
    shortcutInfo->setAlignment(Qt::AlignLeft);
    shortcutInfo->setTextInteractionFlags(Qt::TextSelectableByMouse);
    shortcutInfo->setMargin(3);

    // 添加到布局
    shortcutLayout->addWidget(shortcutInfo);
    shortcutLayout->setSpacing(3);
    shortcutWidget->setLayout(shortcutLayout);

    // 将快捷键提示添加到菜单
    QWidgetAction *shortcutAction = new QWidgetAction(settingsMenu);
    shortcutAction->setDefaultWidget(shortcutWidget);
    settingsMenu->addAction(shortcutAction);

}

//设置窗口不透明度
void Widget::setWindowsOpacity(int opacity)
{
    // 转换为0.0-1.0范围
    qreal opacityValue = qBound(0.3, opacity / 100.0, 1.0);
    QWidget::setWindowOpacity(opacityValue);

    // 更新滑块位置
    QSlider *opacitySlider = qobject_cast<QSlider*>(sender());
    if (opacitySlider && opacitySlider->value() != opacity) {
        opacitySlider->setValue(opacity);
    }
}

