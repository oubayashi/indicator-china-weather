/*
 * Copyright (C) 2013 ~ 2020 National University of Defense Technology(NUDT) & Tianjin Kylin Ltd.
 *
 * Authors:
 *  Kobe Lee    lixiang@kylinos.cn/kobe24_lixiang@126.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mainwindow.h"
#include "titlebar.h"
#include "contentwidget.h"
#include "settingdialog.h"
#include "aboutdialog.h"
#include "promptwidget.h"
#include "weatherworker.h"
#include "maskwidget.h"
#include "weathermanager.h"
#include "preferences.h"
#include "global.h"
using namespace Global;

#include <QApplication>
#include <QDesktopWidget>
#include <QFileInfo>
#include <QMouseEvent>
#include <QMenu>
#include <QScreen>
#include <QShortcut>
#include <QDebug>
#include <QTimer>
#include <math.h>

inline bool testParameterPassingValue(QString &str)
{
    str = QApplication::applicationName();
    return (!str.isEmpty());
}

inline QString convertCodeToBackgroud(int code)
{
    if (code == 100 || code == 900) {
        return ":/res/background/weather-clear.png";
    }
    else if (code <= 103 && code >= 101) {
        return ":/res/background/weather-few-clouds.png";
    }
    else if (code == 104 || code == 901) {
        return ":/res/background/weather-overcast.png";
    }
    else if (code <= 204 && code >= 200) {
        return ":/res/background/weather-clear.png";
    }
    else if (code <= 213 && code >= 205) {
        return ":/res/background/weather-overcast.png";
    }
    else if (code <= 399 && code >= 300) {
        return ":/res/background/weather-rain.png";
    }
    else if (code <= 499 && code >= 400) {
        return ":/res/background/weather-snow.png";
    }
    else if (code <= 502 && code >= 500) {
        return ":/res/background/weather-fog.png";
    }
    else if (code <= 508 && code >= 503) {
        return ":/res/background/weather-sandstorm.png";
    }
    else if (code <= 515 && code >= 509) {
        return ":/res/background/weather-fog.png";
    }
    else {
        return ":/res/background/weather-clear.png";
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(new QWidget(this))
    , m_titleBar(new TitleBar(this))
    , m_contentWidget(new ContentWidget(this))
    , m_tipTimer(new QTimer(this))
    , m_trayTimer(new QTimer(this))
    , m_autoRefreshTimer(new QTimer(this))
    , m_actualizationTime(0)
    , m_maskWidget(new MaskWidget(this))//MaskWidget::Instance();
    , m_weatherManager(new WeatherManager(this))
{
    //将以下类型注册中MetaType系统中
    qRegisterMetaType<ObserveWeather>();
    qRegisterMetaType<QList<ForecastWeather>>();
    qRegisterMetaType<LifeStyle>();

    /*QString appName;
    if (testParameterPassingValue(appName)) {
        qDebug() << "appName:" << appName;
    }
    else {
        qWarning() << "Read appName failed!";
    }*/

    this->setFixedSize(355, 552);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint | Qt::Tool);
    this->setFocusPolicy(Qt::StrongFocus);//this->setFocusPolicy(Qt::NoFocus);
    this->setWindowTitle(tr("Kylin Weather"));
    //const auto ratio = qApp->devicePixelRatio();
    this->setWindowIcon(QIcon::fromTheme("indicator-china-weather", QIcon(":/res/indicator-china-weather.png"))/*.pixmap(QSize(64, 64) * ratio)*/);
    this->setStyleSheet("QMainWindow{color:white;background-image:url(':/res/background/weather-clear.png');background-repeat:no-repeat;}");

    global_init();

    m_updateTimeStr = QString(tr("Refresh time"));

    m_layout = new QVBoxLayout(m_centralWidget);
    m_layout->setContentsMargins(0,0,0,0);
    m_layout->setSpacing(0);

    m_layout->addWidget(m_titleBar);//height:32
    m_layout->addWidget(m_contentWidget);//height:520
    this->setCentralWidget(m_centralWidget);

    m_currentDesktop = qgetenv("XDG_CURRENT_DESKTOP");
    if (m_currentDesktop.isEmpty()) {
        m_currentDesktop = qgetenv("XDG_SESSION_DESKTOP");
    }

    if (m_preferences->m_observerWeather.cond_code.contains(QChar('n'))) {
        this->setStyleSheet("QMainWindow{color:white;background-image:url(':/res/background/weather-clear-night.png');background-repeat:no-repeat;}");
        m_contentWidget->setNightStyleSheets();
        m_titleBar->setNightStyleSheets();
    }
    else {
        QString styleSheetStr = QString("QMainWindow{color:white;background-image:url('%1');background-repeat:no-repeat;}").arg(convertCodeToBackgroud(m_preferences->m_observerWeather.cond_code.toInt()));
        this->setStyleSheet(styleSheetStr);
        m_contentWidget->setDayStyleSheets();
        m_titleBar->setDayStyleSheets();
    }

    connect(m_titleBar, &TitleBar::requestShowSettingDialog, this, [=] () {
        this->showSettingDialog();
    });

    m_hintWidget = new PromptWidget(this);
    m_hintWidget->setIconAndText(":/res/network_warn.png", tr("Network not connected"));
    m_hintWidget->move((this->width() - m_hintWidget->width())/2, (this->height() - m_hintWidget->height())/2);
    m_hintWidget->setVisible(false);

    m_movieWidget = new PromptWidget(tr("Getting data"), this, ":/res/link.gif", true);
    m_movieWidget->move((this->width() - m_hintWidget->width())/2, (this->height() - m_hintWidget->height())/2);
    m_movieWidget->setVisible(false);

    m_titleBar->setCityName(m_preferences->m_currentCity);
    double value = m_preferences->m_opacity*0.01;
    if (value < 0.6) {
        value = 0.60;
        m_preferences->m_opacity = 60;
    }
    this->setOpacity(value);
    this->setVisible(false);

    m_tipTimer->setInterval(60*1000);
    m_tipTimer->setSingleShot(false);
    connect(m_tipTimer, &QTimer::timeout, this, static_cast<void (MainWindow::*)()>(&MainWindow::updateTimeTip));

    m_trayTimer->setInterval(800);
    m_tipTimer->setSingleShot(false);
    connect(m_trayTimer, SIGNAL(timeout()), this, SLOT(changeTrayIcon()));

    connect(m_autoRefreshTimer, &QTimer::timeout, this, [=] () {
        this->startGetWeather();
    });

    this->initMenuAndTray();

    //开始测试网络情况
    m_weatherManager->startTestNetwork();

    //已获取到网络探测的结果
    connect(m_weatherManager, &WeatherManager::nofityNetworkStatus, this, [=] (const QString &status) {
        if (status == "OK") {//互联网可以ping通的情况下，继续开始定位城市
            this->startTrayFlashing(QString(tr("Start to locate the city automatically...")), false);
            m_weatherManager->postSystemInfoToServer();
            m_weatherManager->startAutoLocationTask();//开始自动定位城市
        }
        else {
            this->stopTrayFlashing();

            if (status == "Fail") {//物理网线未连接
                m_contentWidget->setNetworkErrorPages();
                m_hintWidget->setIconAndText(":/res/network_warn.png", status);
                m_autoRefreshTimer->stop();
            }
            else {//互联网无法ping通
                m_contentWidget->setNetworkErrorPages();
                m_hintWidget->setIconAndText(":/res/network_warn.png", status);
                m_autoRefreshTimer->stop();
            }
        }
    });

    //自动定位成功后，更新各个控件的默认城市数据，并开始获取天气数据
    connect(m_weatherManager, &WeatherManager::requestAutoLocationData, this, [=] (const CitySettingData &info, bool success) {
        this->startTrayFlashing(QString(tr("Start getting weather data...")), false);
        if (success) {//自动定位城市成功后，更新各个ui，然后获取天气数据
            if (m_setttingDialog) {
                m_setttingDialog->addCityItem(info);
                m_setttingDialog->refreshCityList(m_preferences->m_currentCity);
            }
            this->refreshCityActions();
            m_cityMenu->menuAction()->setText(info.name);
            this->startGetWeather();
        }
        else {//自动定位城市失败后，获取天气数据
            this->startGetWeather();
        }
    });

    //获取天气数据时发生了异常
    connect(m_weatherManager, &WeatherManager::responseFailure, this, [=] (int code) {
        this->stopTrayFlashing();

        m_maskWidget->hide();
        m_autoRefreshTimer->stop();
        m_movieWidget->setVisible(false);
        m_hintWidget->setVisible(true);

        if (code == 0) {
            m_hintWidget->setIconAndText(":/res/network_warn.png", tr("Incorrect access address"));
        }
        else {
            m_hintWidget->setIconAndText(":/res/network_warn.png", QString(tr("Network error code:%1")).arg(QString::number(code)));
        }
        m_contentWidget->setNetworkErrorPages();
    });

    connect(m_contentWidget, &ContentWidget::requestRetryWeather, this, [=] () {
        this->startTrayFlashing(QString(tr("Start to locate the city automatically...")), true);
        m_weatherManager->postSystemInfoToServer();
        m_weatherManager->startAutoLocationTask();//开始自动定位城市
    });

    connect(m_weatherManager, &WeatherManager::requestDiplayServerNotify, this, [=] (const QString &notifyInfo) {
        if (!notifyInfo.isEmpty() && m_preferences->m_serverNotify)
            m_contentWidget->showServerNotifyInfo(notifyInfo);
    });

    connect(m_weatherManager, &WeatherManager::requesUiRefreshed, this, [=] () {
        this->stopTrayFlashing();
        m_maskWidget->hide();
        m_movieWidget->setVisible(false);

        if (!m_autoRefreshTimer->isActive()) {
            m_autoRefreshTimer->start(m_preferences->m_updateFrequency * 1000 * 60);
        }
        if (m_preferences->m_currentCity.isEmpty()) {
            m_preferences->m_currentCity = m_preferences->m_observerWeather.city;
        }
        m_titleBar->setCityName(m_preferences->m_observerWeather.city);
        m_contentWidget->refreshObserveUI(m_preferences->m_observerWeather);
        this->refreshTrayMenuWeather(m_preferences->m_observerWeather);
        QString condCodeStr = m_preferences->m_observerWeather.cond_code;
        if (!condCodeStr.isEmpty()) {
            if (condCodeStr.contains(QChar('n'))) {
                this->setStyleSheet("QMainWindow{color:white;background-image:url(':/res/background/weather-clear-night.png');background-repeat:no-repeat;}");
                m_contentWidget->setNightStyleSheets();
                m_titleBar->setNightStyleSheets();
            }
            else {
                QString styleSheetStr = QString("QMainWindow{color:white;background-image:url('%1');background-repeat:no-repeat;}").arg(convertCodeToBackgroud(condCodeStr.toInt()));
                this->setStyleSheet(styleSheetStr);
                m_contentWidget->setDayStyleSheets();
                m_titleBar->setDayStyleSheets();
            }
        }

        if (m_setttingDialog) {
            m_setttingDialog->refreshCityList(m_preferences->m_currentCity);
        }

        int len = m_preferences->m_forecastList.size();
        if (len > 3) {
            len = 3;
        }
        for (int i = 0; i < len; ++i) {
            m_contentWidget->refreshForecastUI(m_preferences->m_forecastList[i], i);
        }

        m_contentWidget->refreshLifestyleUI(m_preferences->m_lifestyle);
    });
}

MainWindow::~MainWindow()
{
    if (m_autoRefreshTimer->isActive()) {
        m_autoRefreshTimer->stop();
        delete m_autoRefreshTimer;
    }
    if (m_tipTimer->isActive()) {
        m_tipTimer->stop();
        delete m_tipTimer;
    }
    if (m_trayTimer->isActive()) {
        m_trayTimer->stop();
        delete m_trayTimer;
    }

    global_end();
}

void MainWindow::startTrayFlashing(const QString &msg, bool restart)
{
    if (restart) {
        if (m_trayTimer->isActive()) {
            m_trayTimer->stop();
        }
        m_trayTimer->start();
    }
    m_systemTray->setToolTip(msg);
}

void MainWindow::stopTrayFlashing()
{
    m_trayTimer->stop();
    m_systemTray->setToolTip(QString(tr("Kylin Weather")));
    m_systemTray->setIcon(QIcon::fromTheme("indicator-china-weather", QIcon(":/res/indicator-china-weather.png")));
}

void MainWindow::updateTimeTip()
{
    QDateTime time = QDateTime::currentDateTime();
    int timeIntValue = time.toTime_t();
    int ut = int((round(timeIntValue - m_actualizationTime)/60));
    if (ut == 0 || m_actualizationTime == 0) {
        m_updateTimeStr = QString(tr("Refresh time:Just updated"));
    }
    else {
        if (ut < 2) {
            m_updateTimeStr = QString(tr("Refresh time:%1 minute ago")).arg(QString::number(ut));
        }
        else {
            m_updateTimeStr = QString(tr("Refresh time:%1 minutes ago")).arg(QString::number(ut));
        }
    }

    if (QDateTime::currentDateTime().toTime_t()-m_actualizationTime > m_preferences->m_updateFrequency*1000*60) {
        m_actualizationTime = QDateTime::currentDateTime().toTime_t();
    }

    m_updateTimeAction->setText(m_updateTimeStr);

    //TODO
    //m_weatherWorker->requestPingBackWeatherServer();
}

void MainWindow::setOpacity(double opacity)
{
    this->setWindowOpacity(opacity);
}

void MainWindow::startGetWeather()
{
    m_maskWidget->showMask();
    m_tipTimer->stop();
    m_movieWidget->setVisible(true);
    m_titleBar->setCityName(m_preferences->m_currentCity);
    m_weatherManager->startRefresheWeatherData(m_preferences->m_currentCityId);
}

void MainWindow::initMenuAndTray()
{
    m_mainMenu = new QMenu(this);
    m_cityMenu = new QMenu(this);
    m_cityMenu->menuAction()->setText(tr("City"));
    m_cityActionGroup = new MenuActionGroup(this);
    connect(m_cityActionGroup, &MenuActionGroup::activated, this, [=] (int index) {
        QString cur_cityName = m_cityActionGroup->setCurrentCheckedIndex(index);
        if (m_preferences->citiesCount() > 0 && m_preferences->citiesCount() >= index) {
            m_cityMenu->menuAction()->setText(cur_cityName/*m_preferences->cityName(index)*/);

            m_preferences->setCurrentCityIdAndName(cur_cityName/*index*/);

            if (m_setttingDialog) {
                m_setttingDialog->refreshCityList(m_preferences->m_currentCity);
            }
            this->startGetWeather();
        }
    });
    this->refreshCityActions();

    m_mainMenu->addMenu(m_cityMenu);
    //QAction *m_switchAciton = m_mainMenu->addAction(tr("Switch"));
    m_mainMenu->addSeparator();

    m_weatherAction = new QAction("N/A",this);
    m_temperatureAction = new QAction("N/A",this);
    m_sdAction = new QAction("N/A",this);
    m_aqiAction = new QAction("N/A",this);
    m_releaseTimeAction = new QAction(tr("Release time"),this);
    m_updateTimeAction = new QAction(m_updateTimeStr, this);
    m_mainMenu->addAction(m_weatherAction);
    m_mainMenu->addAction(m_temperatureAction);
    m_mainMenu->addAction(m_sdAction);
    m_mainMenu->addAction(m_aqiAction);
    m_mainMenu->addAction(m_releaseTimeAction);
    m_mainMenu->addAction(m_updateTimeAction);

    connect(m_updateTimeAction, &QAction::triggered, this, [=] () {
        this->startGetWeather();
    });


    QAction *m_forecastAction = m_mainMenu->addAction(tr("Weather Forecast"));
    connect(m_forecastAction, &QAction::triggered, this, [=] () {
        if (!this->isVisible()) {
            this->movePosition();
        }
        else {
            this->setVisible(false);
        }
    });

    m_mainMenu->addSeparator();
    QAction *m_settingAction = m_mainMenu->addAction(tr("Settings"));
    m_settingAction->setIcon(QIcon(":/res/prefs.png"));
    QAction *m_aboutAction = m_mainMenu->addAction(tr("Kylin Weather - About"));
    m_aboutAction->setIcon(QIcon(":/res/about_normal.png"));
    QAction *m_quitAction = m_mainMenu->addAction(tr("Exit"));
    m_quitAction->setIcon(QIcon(":/res/quit_normal.png"));

    connect(m_settingAction, &QAction::triggered, this, [=] (){
        this->showSettingDialog();
    });
    connect(m_aboutAction, &QAction::triggered, this, [=] () {
        AboutDialog dlg;
        dlg.exec();
    });
    connect(m_quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_systemTray = new QSystemTrayIcon(this);
    m_systemTray->setToolTip(QString(tr("Kylin Weather")));
    m_systemTray->setIcon(QIcon::fromTheme("indicator-china-weather", QIcon(":/res/indicator-china-weather.png")));
    connect(m_systemTray,SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    m_systemTray->show();
    m_systemTray->setContextMenu(m_mainMenu);

    QShortcut *m_quitShortCut = new QShortcut(QKeySequence("Ctrl+Q"), this);
    connect(m_quitShortCut, SIGNAL(activated()), qApp, SLOT(quit()));

    this->startTrayFlashing(QString(tr("Detecting network...")), true);
}

void MainWindow::showSettingDialog()
{
    if (!m_setttingDialog) {
        createSettingDialog();
    }
    m_setttingDialog->moveToCenter();
}

void MainWindow::createSettingDialog()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_setttingDialog = new SettingDialog;
    m_setttingDialog->setModal(false);
    connect(m_setttingDialog, SIGNAL(applied()), this, SLOT(applySettings()));
    connect(m_setttingDialog, &SettingDialog::requestRefreshCityMenu, this, [this] (bool removedDefault) {
        this->refreshCityActions();

        if (removedDefault) {//刪除了默认城市后，重新设置了列表中第一个城市为默认城市后，从服务端获取该城市的天气
            this->startGetWeather();
        }
    });
    connect(m_setttingDialog, &SettingDialog::requestRefreshWeatherById, this, [this] (const QString &id) {
        m_preferences->resetCurrentCityNameById(id);
        this->refreshCityActions();
        this->startGetWeather();
    });
    connect(m_setttingDialog, &SettingDialog::requestChangeOpacity, this, [this] (int opcatity) {
        double value = opcatity*0.01;
        if (value < 0.6) {
            value = 0.60;
            m_preferences->m_opacity = 60;
        }
        this->setOpacity(value);
    });

    QApplication::restoreOverrideCursor();
}

void MainWindow::refreshCityActions()
{
    // clear orig action list
    m_cityActionGroup->clearAllActions();

    // add new action list
    int i = 0;
    int currentIndex = 0;
    foreach (QString city, m_preferences->getCitiesList()) {
        if (city == m_preferences->m_currentCity) {
            currentIndex = i;
        }

        MenuActionGroupItem *cityAction = new MenuActionGroupItem(this, m_cityActionGroup, i);
        cityAction->setActionText(city);
        i++;
    }
    m_cityMenu->addActions(m_cityActionGroup->actions());
    m_cityMenu->menuAction()->setText(m_preferences->m_currentCity);
    m_cityActionGroup->setCurrentCheckedIndex(currentIndex);
}

void MainWindow::refreshTrayMenuWeather(const ObserveWeather &data)
{
    QPixmap iconPix = QPixmap(QString(":/res/weather_icons/white/%1.png").arg(data.cond_code));
    if (iconPix.isNull()) {
        iconPix = QPixmap(":/res/weather_icons/white/999.png");//TODO:或者使用软件logo?
    }
    //Q_ASSERT(!iconPix.isNull());
    m_systemTray->setIcon(iconPix);
    m_weatherAction->setText(data.cond_txt);
    m_temperatureAction->setText(QString(tr("Temperature:%1")).arg(data.tmp) + "˚C");
    m_sdAction->setText(QString(tr("Relative humidity:%1")).arg(data.hum));
    if (data.air.isEmpty() || data.air == "-") {
        m_aqiAction->setText(QString(tr("Air quality:%1")).arg(QString(tr("Unknown"))));
    }
    else {
        m_aqiAction->setText(QString(tr("Air quality:%1")).arg(data.air));
    }
    m_releaseTimeAction->setText(QString(tr("Release time:%1")).arg(data.updatetime));

    m_actualizationTime = 0;
    this->updateTimeTip();
    m_tipTimer->start();
}

void MainWindow::applySettings()
{

}

void MainWindow::resetWeatherBackgroud(const QString &imgPath)
{
    QString weatherBg = imgPath;
    if(!QFileInfo(weatherBg).exists())
        weatherBg = ":/res/background/weather-clear.png";
    QString currentBg = QString("QMainWindow{color:white;background-image:url('%1');background-repeat:no-repeat;}").arg(weatherBg);
    this->setStyleSheet(currentBg);
}

void MainWindow::changeTrayIcon()
{
    static bool changed = false;
    if (changed) {
        m_systemTray->setIcon(QIcon(":/res/statusBusy1.png"));
    }
    else {
        m_systemTray->setIcon(QIcon(":/res/statusBusy2.png"));
    }
    changed = !changed;
}

void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch(reason) {
    /*case QSystemTrayIcon::DoubleClick:
    {
        QRect rect = m_systemTray->geometry();
        QMenu *currentMenu = m_systemTray->contextMenu();
        if (currentMenu->isHidden()) {
            currentMenu->popup(QPoint(rect.x()+8, rect.y()));
        }
    }
        break;*/
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::MiddleClick:
        if (this->isVisible()) {
            this->setVisible(false);
        }
        else {
            this->movePosition();
        }
        break;
    default:
        break;
    }
}

void MainWindow::movePosition()
{
    QRect availableGeometry = qApp->primaryScreen()->availableGeometry();
    QRect screenGeometry = qApp->primaryScreen()->geometry();

    /*QRect primaryGeometry;
    for (QScreen *screen : qApp->screens()) {
        if (screen->geometry().contains(QCursor::pos())) {
            primaryGeometry = screen->geometry();
        }
    }
    if (primaryGeometry.isEmpty()) {
        primaryGeometry = qApp->primaryScreen()->geometry();
    }
    this->move(primaryGeometry.x() + primaryGeometry.width() - this->width(), primaryGeometry.y());
    */

    //panel in bottom or right, then show on topRight
    if (availableGeometry.x() == screenGeometry.x() && availableGeometry.y() == screenGeometry.y()) {
        this->move(availableGeometry.x() + availableGeometry.width() - this->width(), availableGeometry.height() - this->height());
    }
    else {//panel in top or left, then show on bottomRight
        this->move(availableGeometry.x() + availableGeometry.width() - this->width(), availableGeometry.y());
    }
    this->showNormal();
    this->raise();
    this->activateWindow();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        if (this->isVisible()) {
            this->setVisible(false);
        }
        event->accept();
    }

    QMainWindow::keyPressEvent(event);
}

void MainWindow::focusOutEvent(QFocusEvent *event)
{
    if (event->reason() == Qt::ActiveWindowFocusReason) {
        this->setVisible(false);
    }

    return QMainWindow::focusOutEvent(event);
}
