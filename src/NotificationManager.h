#ifndef NOTIFICATIONMANAGER_H
#define NOTIFICATIONMANAGER_H

#include <QObject>
#include <QWidget>
#include <QTimer>
#include <QMediaPlayer>

class NotificationManager : public QObject {
    Q_OBJECT
public:
    static NotificationManager& instance();
    void configure(bool enabled, int durationMs, bool soundEnabled);
    void showMessage(const QString& text);

private:
    NotificationManager();
    ~NotificationManager();
    QWidget* m_window = nullptr;
    QTimer m_hideTimer;
    QMediaPlayer* m_player = nullptr;
    bool m_enabled = true;
    int m_duration = 1000;
    bool m_sound = true;
};

#endif // NOTIFICATIONMANAGER_H