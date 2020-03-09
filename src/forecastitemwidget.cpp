#include "forecastitemwidget.h"
#include "tooltip.h"

#include <QDebug>
#include <QEvent>
#include <QCursor>
#include <QDate>
#include <QDateTime>
#include <QHelpEvent>

ForecastItemWidget::ForecastItemWidget(QWidget *parent) :
    QWidget(parent)
    , m_isPressed(false)
{
    this->setFixedSize(100, 140);//140:initForecastWidget's height - 2*space = 160- 10*2
    this->setStyleSheet("QWidget{border-radius: 0px;color:rgb(250,250,250);background-color:rgba(0,0,0,0.2)}");
//    setAttribute(Qt::WA_TransparentForMouseEvents);

    m_weekLabel = new QLabel(this);
    m_dateLabel = new QLabel(this);
    m_weatherLabel = new QLabel(this);
    m_iconLabel = new QLabel(this);
    m_tempLabel = new QLabel(this);
    m_iconLabel->setFixedSize(48, 48);
    m_iconLabel->setStyleSheet("QLabel{border:none;background-color:transparent;}");

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(5);
    m_layout->addWidget(m_weekLabel, 0, Qt::AlignTop | Qt::AlignHCenter);
    m_layout->addWidget(m_dateLabel, 0, Qt::AlignHCenter);
    m_layout->addWidget(m_weatherLabel, 0, Qt::AlignHCenter);
    m_layout->addWidget(m_iconLabel, 0, Qt::AlignHCenter);
    m_layout->addWidget(m_tempLabel, 0, Qt::AlignBottom | Qt::AlignHCenter);

    m_toolTip = new ToolTip();

    this->setDefaultData();
}

void ForecastItemWidget::resetForecastData(const ForecastWeather &data, int index)
{
    if (index == 0) {
        m_weekLabel->setText(tr("Today"));
    }
    else {
        if (data.forcast_date.isEmpty()) {
            m_weekLabel->setText("--");
        }
        else {
            QLocale locale;
            locale = locale.language();
    //        if (locale.language() == QLocale::Chinese) {
    //            qDebug() << "CH";
    //        }
    //        else if (locale.language() == QLocale::English) {
    //            qDebug() << "EN";
    //            locale = QLocale::English;
    //        }
            QDateTime dt = QDateTime::fromString(data.forcast_date,"yyyy-MM-dd");
            QDate m_date = dt.date();
            m_weekLabel->setText(locale.toString(m_date, "ddd"));

            /*
            QDateTime dt = QDateTime::fromString(data.forcast_date,"yyyy-MM-dd");
    //        QDateTime dt;
    //        dt.setTime_t(str.toInt());
            QDate m_date = dt.date();//qDebug() << QDate::currentDate().toString("ddd");
            m_weekLabel->setText(m_date.toString("ddd"));
            */
        }
    }

    m_dateLabel->setText(data.forcast_date);
    m_weatherLabel->setText(data.cond_txt_d);

    //darkgrey or lightgrey
    QPixmap iconPix;
    if (m_isDayOrNight) {
        iconPix = QPixmap(QString(":/res/weather_icons/darkgrey/%1.png").arg(data.cond_code_d));
        if (iconPix.isNull()) {
            iconPix = QPixmap(":/res/weather_icons/darkgrey/999.png");
        }
    }
    else {
        iconPix = QPixmap(QString(":/res/weather_icons/lightgrey/%1.png").arg(data.cond_code_d));
        if (iconPix.isNull()) {
            iconPix = QPixmap(":/res/weather_icons/lightgrey/999.png");
        }
    }
    //Q_ASSERT(!iconPix.isNull());
    iconPix = iconPix.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_iconLabel->setPixmap(iconPix);
    m_tempLabel->setText(QString("%1°C~%2°C").arg(data.tmp_min).arg(data.tmp_max));

    m_toolTip->resetData(data, m_weekLabel->text());
}

void ForecastItemWidget::setDayStyleSheets()
{
    m_isDayOrNight = true;

    m_weekLabel->setStyleSheet("QLabel{border:none;background-color:transparent;color:#808080;font-size:12px;}");
    m_dateLabel->setStyleSheet("QLabel{border:none;background-color:transparent;color:#808080;font-size:12px;}");
    m_weatherLabel->setStyleSheet("QLabel{border:none;background-color:transparent;color:#808080;font-size:12px;}");
    m_tempLabel->setStyleSheet("QLabel{border:none;background-color:transparent;color:#808080;font-size:12px;}");
}

void ForecastItemWidget::setNightStyleSheets()
{
    m_isDayOrNight = false;

    m_weekLabel->setStyleSheet("QLabel{border:none;background-color:transparent;color:#d9d9d9;font-size:12px;}");
    m_dateLabel->setStyleSheet("QLabel{border:none;background-color:transparent;color:#d9d9d9;font-size:12px;}");
    m_weatherLabel->setStyleSheet("QLabel{border:none;background-color:transparent;color:#d9d9d9;font-size:12px;}");
    m_tempLabel->setStyleSheet("QLabel{border:none;background-color:transparent;color:#d9d9d9;font-size:12px;}");
}

void ForecastItemWidget::setDefaultData()
{
    m_weekLabel->setText("-");
    m_dateLabel->setText("-");
    m_weatherLabel->setText("-");
    QPixmap iconPix = QPixmap(":/res/weather_icons/darkgrey/999.png");
    //Q_ASSERT(!iconPix.isNull());
    iconPix = iconPix.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_iconLabel->setPixmap(iconPix);
    m_tempLabel->setText("-°C");
}

bool ForecastItemWidget::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        //m_toolTip->popupTip(QCursor::pos());
        if (QHelpEvent *e = static_cast<QHelpEvent *>(event)) {
            m_toolTip->popupTip(e->globalPos());
            return false;
        }
    }
    /*else if (event->type() == QEvent::Leave)  {
        m_toolTip->hide();
//        QWidget::leaveEvent(event);
    }
    else if (event->type() == QEvent::MouseButtonPress) {
        m_toolTip->hide();
    }*/

    return QWidget::event(event);
}

void ForecastItemWidget::enterEvent(QEvent *event)
{
//    m_toolTip->popupTip(QCursor::pos());
    QWidget::enterEvent(event);
}

void ForecastItemWidget::leaveEvent(QEvent *event)
{
    m_toolTip->hide();
    QWidget::leaveEvent(event);
}

void ForecastItemWidget::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    m_isPressed = true;
}

void ForecastItemWidget::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    if (m_isPressed) {
        m_isPressed = false;
        m_toolTip->hide();
    }
}
