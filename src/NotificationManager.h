#ifndef NOTIFICATIONMANAGER_H
#define NOTIFICATIONMANAGER_H

#include <QObject>
#include <QWidget>
#include <QTimer>
#include <QMediaPlayer>
#include <QPointer>
#include <QLabel>

class NotificationManager : public QObject {
    Q_OBJECT
public:
    static NotificationManager& instance();
    void configure(bool enabled, int durationMs, bool soundEnabled);
    void showMessage(const QString& text);
    void playSound();

private:
    NotificationManager();
    ~NotificationManager();
    QPointer<QWidget> m_window;
    QTimer m_hideTimer;
    QMediaPlayer* m_player = nullptr;
    QLabel* m_contentLabel = nullptr; 
    bool m_enabled = true;
    int m_duration = 1000;
    bool m_sound = true;
    bool m_soundAvailable = false;
};

#endif // NOTIFICATIONMANAGER_H