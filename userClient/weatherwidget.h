#ifndef WEATHERWIDGET_H
#define WEATHERWIDGET_H
#pragma once

#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Ui {
class WeatherWidget;
}

class WeatherWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WeatherWidget(QWidget *parent = nullptr);
    ~WeatherWidget();

private slots:
    void onWeatherReply(QNetworkReply *reply);

private:
    Ui::WeatherWidget *ui;
    QNetworkAccessManager *manager;
    void fetchWeather();
};

#endif // WEATHERWIDGET_H
