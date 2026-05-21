#include "SetupWizard.h"
#include "ConfigManager.h"
#include "NotificationManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QKeySequenceEdit>
#include <QFileDialog>
#include <QTextEdit>
#include <QGroupBox>
#include <QPushButton>
#include <QDir>
#include <QStandardPaths>
#include <QGuiApplication>
#include <QScreen>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDebug>

// ================== 支持鼠标按键的热键编辑框（基于 QLineEdit）==================
class SingleKeySequenceEdit : public QLineEdit {
    Q_OBJECT
public:
    explicit SingleKeySequenceEdit(QWidget *parent = nullptr);
    void setParentNotification(NotificationManager* mgr) { m_parentNotification = mgr; }
    void setHotkeyString(const QString& hotkeyStr);
    QString getHotkeyString() const { return m_currentHotkey; }

signals:
    void hotkeyStringChanged(const QString& hotkeyStr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool event(QEvent *event) override; // 用于捕获焦点

private:
    void clearHotkey();
    void setHotkeyFromButton(int buttonCode, Qt::KeyboardModifiers modifiers);
    void setHotkeyFromKey(int key, Qt::KeyboardModifiers modifiers);
    void updateDisplay();
    void showError(const QString& msg);

    QString m_currentHotkey;
    bool m_isCapturing = false;
    NotificationManager* m_parentNotification = nullptr;
};

SingleKeySequenceEdit::SingleKeySequenceEdit(QWidget *parent) : QLineEdit(parent) {
    setReadOnly(true);
    setPlaceholderText(tr("点击后按下键盘或鼠标按键"));
    setFocusPolicy(Qt::StrongFocus);
}

void SingleKeySequenceEdit::setHotkeyString(const QString& hotkeyStr) {
    m_currentHotkey = hotkeyStr;
    updateDisplay();
    emit hotkeyStringChanged(m_currentHotkey);
}

void SingleKeySequenceEdit::mousePressEvent(QMouseEvent *event) {
    if (!hasFocus()) {
        setFocus();
        return;
    }
    Qt::MouseButton button = event->button();
    Qt::KeyboardModifiers mods = event->modifiers();
    int btnCode = -1;
    if (button == Qt::LeftButton) btnCode = 0;
    else if (button == Qt::RightButton) btnCode = 1;
    else if (button == Qt::MiddleButton) btnCode = 2;
    else if (button == Qt::XButton1) btnCode = 3;
    else if (button == Qt::XButton2) btnCode = 4;
    if (btnCode != -1) {
        setHotkeyFromButton(btnCode, mods);
        event->accept();
    } else {
        QLineEdit::mousePressEvent(event);
    }
}

void SingleKeySequenceEdit::keyPressEvent(QKeyEvent *event) {
    int key = event->key();
    if (key == Qt::Key_Backspace || key == Qt::Key_Delete) {
        clearHotkey();
        event->accept();
        return;
    }
    Qt::KeyboardModifiers mods = event->modifiers();
    setHotkeyFromKey(key, mods);
    event->accept();
}

bool SingleKeySequenceEdit::event(QEvent *event) {
    if (event->type() == QEvent::FocusIn) {
        setPlaceholderText(tr("请按下键盘或鼠标按键..."));
        setText(QString());
    } else if (event->type() == QEvent::FocusOut) {
        setPlaceholderText(tr("点击后按下键盘或鼠标按键"));
        if (m_currentHotkey.isEmpty()) {
            setText(QString());
        } else {
            updateDisplay();
        }
    }
    return QLineEdit::event(event);
}

void SingleKeySequenceEdit::clearHotkey() {
    m_currentHotkey.clear();
    updateDisplay();
    emit hotkeyStringChanged(QString());
}

void SingleKeySequenceEdit::setHotkeyFromButton(int buttonCode, Qt::KeyboardModifiers modifiers) {
    QString btnName;
    switch (buttonCode) {
        case 0: btnName = "MouseLeft"; break;
        case 1: btnName = "MouseRight"; break;
        case 2: btnName = "MouseMiddle"; break;
        case 3: btnName = "MouseX1"; break;
        case 4: btnName = "MouseX2"; break;
        default: return;
    }
    QString modStr;
    if (modifiers & Qt::ControlModifier) modStr += "Ctrl+";
    if (modifiers & Qt::AltModifier)     modStr += "Alt+";
    if (modifiers & Qt::ShiftModifier)   modStr += "Shift+";
    if (modifiers & Qt::MetaModifier)    modStr += "Win+";
    m_currentHotkey = modStr + btnName;
    updateDisplay();
    emit hotkeyStringChanged(m_currentHotkey);
}

void SingleKeySequenceEdit::setHotkeyFromKey(int key, Qt::KeyboardModifiers modifiers) {
    // 只允许字母、数字、F1-F24、部分特殊键
    QString keyStr;
    if (key >= Qt::Key_A && key <= Qt::Key_Z) {
        keyStr = QChar(key).toUpper();
    } else if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        keyStr = QChar(key);
    } else if (key >= Qt::Key_F1 && key <= Qt::Key_F24) {
        keyStr = QString("F%1").arg(key - Qt::Key_F1 + 1);
    } else {
        static QHash<int, QString> specialMap = {
            {Qt::Key_Space, "Space"}, {Qt::Key_Return, "Return"}, {Qt::Key_Enter, "Enter"},
            {Qt::Key_Tab, "Tab"}, {Qt::Key_Escape, "Escape"}, {Qt::Key_Backspace, "Backspace"},
            {Qt::Key_Delete, "Delete"}, {Qt::Key_Insert, "Insert"},
            {Qt::Key_Home, "Home"}, {Qt::Key_End, "End"},
            {Qt::Key_PageUp, "PageUp"}, {Qt::Key_PageDown, "PageDown"},
            {Qt::Key_Up, "Up"}, {Qt::Key_Down, "Down"}, {Qt::Key_Left, "Left"}, {Qt::Key_Right, "Right"},
            {Qt::Key_Print, "PrintScreen"}, {Qt::Key_ScrollLock, "ScrollLock"}, {Qt::Key_Pause, "Pause"},
            {Qt::Key_CapsLock, "CapsLock"}, {Qt::Key_NumLock, "NumLock"}
        };
        if (specialMap.contains(key)) {
            keyStr = specialMap[key];
        } else {
            showError(tr("不支持的按键: %1").arg(QKeySequence(key).toString()));
            return;
        }
    }
    QString modStr;
    if (modifiers & Qt::ControlModifier) modStr += "Ctrl+";
    if (modifiers & Qt::AltModifier)     modStr += "Alt+";
    if (modifiers & Qt::ShiftModifier)   modStr += "Shift+";
    if (modifiers & Qt::MetaModifier)    modStr += "Win+";
    m_currentHotkey = modStr + keyStr;
    updateDisplay();
    emit hotkeyStringChanged(m_currentHotkey);
}

void SingleKeySequenceEdit::updateDisplay() {
    setText(m_currentHotkey);
}

void SingleKeySequenceEdit::showError(const QString &msg) {
    if (m_parentNotification)
        m_parentNotification->showMessage(msg);
    else
        qWarning() << msg;
}

// ================== 确认页面类 ==================
class ConfirmPage : public QWizardPage {
public:
    ConfirmPage(SetupWizard* wizard, QTextEdit* textEdit)
        : m_wizard(wizard), m_textEdit(textEdit)
    {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->addWidget(m_textEdit);
        layout->addStretch();
        setTitle(tr("确认设置"));
    }

    void initializePage() override {
        auto& temp = m_wizard->m_tempConfig;
        temp["hotkey"] = m_wizard->m_hotkeyEdit->getHotkeyString();
        temp["replay_hotkey"] = m_wizard->m_replayHotkeyEdit->getHotkeyString();
        temp["save_dir"] = m_wizard->m_dirEdit->text();
        QString quality = m_wizard->m_qualityCombo->currentText();
        int qualityVal = (quality == tr("高") ? 2 : (quality == tr("中") ? 1 : 0));
        temp["quality"] = qualityVal;
        temp["replay_enabled"] = m_wizard->m_replayEnableCheck->isChecked();
        temp["replay_duration"] = m_wizard->m_replayDurationSpin->value();
        temp["replay_interval"] = m_wizard->m_replayIntervalSpin->value();
        int scaleIdx = m_wizard->m_replayScaleCombo->currentIndex();
        int scaleVal[] = {100, 75, 50, 25};
        temp["replay_scale"] = scaleVal[scaleIdx];
        temp["autostart"] = m_wizard->m_autostartCheck->isChecked();
        temp["desktop_shortcut"] = m_wizard->m_desktopCheck->isChecked();
        temp["notifications_enabled"] = m_wizard->m_notificationCheck->isChecked();
        temp["sound_enabled"] = m_wizard->m_soundCheck->isChecked();
        int durIdx = m_wizard->m_durationCombo->currentIndex();
        int durMs[] = {500, 1000, 1500, 2000};
        temp["notification_duration"] = durMs[durIdx];

        QString html = QString(
            "<b>普通截图快捷键:</b> %1<br>"
            "<b>回放截屏快捷键:</b> %2<br>"
            "<b>保存目录:</b> %3<br>"
            "<b>图片质量:</b> %4<br>"
            "<b>回放功能:</b> %5<br>"
            "<b>回放时长/间隔/采样:</b> %6秒 / %7毫秒 / %8%%<br>"
            "<b>开机自启动:</b> %9<br>"
            "<b>桌面快捷方式:</b> %10<br>"
            "<b>通知:</b> %11<br>"
            "<b>通知时长:</b> %12秒<br>"
            "<b>提示音:</b> %13"
        ).arg(temp["hotkey"].toString(),
              temp["replay_hotkey"].toString(),
              temp["save_dir"].toString(),
              m_wizard->m_qualityCombo->currentText(),
              temp["replay_enabled"].toBool() ? tr("启用") : tr("禁用"),
              QString::number(temp["replay_duration"].toInt()),
              QString::number(temp["replay_interval"].toInt()),
              QString::number(temp["replay_scale"].toInt()),
              temp["autostart"].toBool() ? tr("是") : tr("否"),
              temp["desktop_shortcut"].toBool() ? tr("是") : tr("否"),
              temp["notifications_enabled"].toBool() ? tr("启用") : tr("禁用"),
              QString::number(durMs[durIdx] / 1000.0, 'f', 1),
              temp["sound_enabled"].toBool() ? tr("启用") : tr("禁用"));
        m_textEdit->setHtml(html);
    }

private:
    SetupWizard* m_wizard;
    QTextEdit* m_textEdit;
};

// ================== SetupWizard 实现 ==================
SetupWizard::SetupWizard(QWidget *parent) : QWizard(parent) {
    setWindowTitle(tr("Flashshot 设置向导"));
    setWizardStyle(ClassicStyle);
    setOption(HaveHelpButton, false);
    setMinimumSize(600, 500);
    resize(640, 520);

    initPages();

    auto& cfg = ConfigManager::instance();
    m_hotkeyEdit->setHotkeyString(cfg.hotkey());
    m_replayHotkeyEdit->setHotkeyString(cfg.replayHotkey());
    m_dirEdit->setText(cfg.saveDir());
    int quality = cfg.quality();
    m_qualityCombo->setCurrentIndex(quality == 2 ? 0 : (quality == 1 ? 1 : 2));
    m_replayEnableCheck->setChecked(cfg.replayEnabled());
    m_replayDurationSpin->setValue(cfg.replayDuration());
    m_replayIntervalSpin->setValue(cfg.replayInterval());
    int scale = cfg.replayScale();
    int scaleIdx = (scale == 100 ? 0 : (scale == 75 ? 1 : (scale == 50 ? 2 : 3)));
    m_replayScaleCombo->setCurrentIndex(scaleIdx);
    m_autostartCheck->setChecked(cfg.autostart());
    m_desktopCheck->setChecked(cfg.desktopShortcut());
    m_notificationCheck->setChecked(cfg.notificationsEnabled());
    m_soundCheck->setChecked(cfg.soundEnabled());
    int durMs = cfg.notificationDuration();
    int durIdx = (durMs == 500 ? 0 : (durMs == 1000 ? 1 : (durMs == 1500 ? 2 : 3)));
    m_durationCombo->setCurrentIndex(durIdx);

    QRect screenGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    move(screenGeometry.center() - rect().center());
}

void SetupWizard::initPages() {
    setPage(0, createHotkeyPage());
    setPage(1, createDirPage());
    setPage(2, createQualityPage());
    setPage(3, createReplayPage());
    setPage(4, createNotificationPage());
    setPage(5, createOtherPage());
    setPage(6, createConfirmPage());
    setStartId(0);
}

QWizardPage* SetupWizard::createHotkeyPage() {
    QWizardPage* page = new QWizardPage;
    page->setTitle(tr("快捷键设置"));
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->addWidget(new QLabel(tr("普通截图快捷键:")));
    m_hotkeyEdit = new SingleKeySequenceEdit();
    layout->addWidget(m_hotkeyEdit);

    QLabel* tip = new QLabel(tr("提示：支持字母、数字、F1-F12、鼠标左中右键、侧键等，可配合Ctrl/Alt/Shift/Win组合"));
    tip->setStyleSheet("color: gray; font-size: 11px;");
    layout->addWidget(tip);
    layout->addStretch();
    return page;
}

QWizardPage* SetupWizard::createDirPage() {
    QWizardPage* page = new QWizardPage;
    page->setTitle(tr("保存目录"));
    QVBoxLayout* layout = new QVBoxLayout(page);

    layout->addWidget(new QLabel(tr("截图保存目录:")));
    m_dirEdit = new QLineEdit;
    m_dirEdit->setReadOnly(true);
    QString defaultDir = QDir::homePath() + "/Pictures/Flashshot_x64";
    m_dirEdit->setText(defaultDir);
    layout->addWidget(m_dirEdit);

    QPushButton* browseBtn = new QPushButton(tr("浏览..."));
    connect(browseBtn, &QPushButton::clicked, [this](){
        QString dir = QFileDialog::getExistingDirectory(this, tr("选择目录"), m_dirEdit->text());
        if (!dir.isEmpty())
            m_dirEdit->setText(dir);
    });
    layout->addWidget(browseBtn);
    layout->addStretch();
    return page;
}

QWizardPage* SetupWizard::createQualityPage() {
    QWizardPage* page = new QWizardPage;
    page->setTitle(tr("图片质量"));
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->addWidget(new QLabel(tr("截图质量:")));
    m_qualityCombo = new QComboBox;
    m_qualityCombo->addItems({tr("高"), tr("中"), tr("低")});
    layout->addWidget(m_qualityCombo);
    layout->addStretch();
    return page;
}

QWizardPage* SetupWizard::createReplayPage() {
    QWizardPage* page = new QWizardPage;
    page->setTitle(tr("回放截屏设置"));
    QVBoxLayout* layout = new QVBoxLayout(page);

    m_replayEnableCheck = new QCheckBox(tr("启用回放截屏功能"));
    layout->addWidget(m_replayEnableCheck);

    QGroupBox* gb = new QGroupBox(tr("回放参数"));
    QVBoxLayout* gbLayout = new QVBoxLayout(gb);

    QHBoxLayout* h1 = new QHBoxLayout;
    h1->addWidget(new QLabel(tr("回溯时长(秒):")));
    m_replayDurationSpin = new QSpinBox;
    m_replayDurationSpin->setRange(1, 60);
    m_replayDurationSpin->setValue(10);
    h1->addWidget(m_replayDurationSpin);
    h1->addStretch();
    gbLayout->addLayout(h1);

    QHBoxLayout* h2 = new QHBoxLayout;
    h2->addWidget(new QLabel(tr("抓帧间隔(毫秒):")));
    m_replayIntervalSpin = new QSpinBox;
    m_replayIntervalSpin->setRange(100, 2000);
    m_replayIntervalSpin->setSingleStep(100);
    m_replayIntervalSpin->setValue(500);
    h2->addWidget(m_replayIntervalSpin);
    h2->addStretch();
    gbLayout->addLayout(h2);

    QHBoxLayout* h3 = new QHBoxLayout;
    h3->addWidget(new QLabel(tr("采样比例:")));
    m_replayScaleCombo = new QComboBox;
    m_replayScaleCombo->addItems({"100%", "75%", "50%", "25%"});
    m_replayScaleCombo->setCurrentIndex(2);
    h3->addWidget(m_replayScaleCombo);
    h3->addStretch();
    gbLayout->addLayout(h3);

    gb->setLayout(gbLayout);
    layout->addWidget(gb);

    layout->addWidget(new QLabel(tr("回放截屏快捷键:")));
    m_replayHotkeyEdit = new SingleKeySequenceEdit();
    layout->addWidget(m_replayHotkeyEdit);

    QLabel* tip = new QLabel(tr("提示：支持字母、数字、F1-F12、鼠标左中右键、侧键等，可配合Ctrl/Alt/Shift/Win组合"));
    tip->setStyleSheet("color: gray; font-size: 11px;");
    layout->addWidget(tip);

    connect(m_replayEnableCheck, &QCheckBox::toggled, m_replayDurationSpin, &QSpinBox::setEnabled);
    connect(m_replayEnableCheck, &QCheckBox::toggled, m_replayIntervalSpin, &QSpinBox::setEnabled);
    connect(m_replayEnableCheck, &QCheckBox::toggled, m_replayScaleCombo, &QComboBox::setEnabled);
    connect(m_replayEnableCheck, &QCheckBox::toggled, m_replayHotkeyEdit, &SingleKeySequenceEdit::setEnabled);

    layout->addStretch();
    return page;
}

QWizardPage* SetupWizard::createNotificationPage() {
    QWizardPage* page = new QWizardPage;
    page->setTitle(tr("通知设置"));
    QVBoxLayout* layout = new QVBoxLayout(page);

    m_notificationCheck = new QCheckBox(tr("启用通知（截图时右下角提示）"));
    m_notificationCheck->setChecked(true);
    layout->addWidget(m_notificationCheck);

    QHBoxLayout* h = new QHBoxLayout;
    h->addWidget(new QLabel(tr("通知显示时长:")));
    m_durationCombo = new QComboBox;
    m_durationCombo->addItems({"0.5 秒", "1.0 秒", "1.5 秒", "2.0 秒"});
    m_durationCombo->setCurrentIndex(1);
    h->addWidget(m_durationCombo);
    h->addStretch();
    layout->addLayout(h);

    m_soundCheck = new QCheckBox(tr("截图时播放提示音"));
    m_soundCheck->setChecked(true);
    layout->addWidget(m_soundCheck);

    layout->addStretch();
    return page;
}

QWizardPage* SetupWizard::createOtherPage() {
    QWizardPage* page = new QWizardPage;
    page->setTitle(tr("其他选项"));
    QVBoxLayout* layout = new QVBoxLayout(page);
    m_autostartCheck = new QCheckBox(tr("开机自启动"));
    m_desktopCheck = new QCheckBox(tr("创建桌面快捷方式"));
    layout->addWidget(m_autostartCheck);
    layout->addWidget(m_desktopCheck);
    layout->addStretch();
    return page;
}

QWizardPage* SetupWizard::createConfirmPage() {
    QTextEdit* textEdit = new QTextEdit;
    textEdit->setReadOnly(true);
    return new ConfirmPage(this, textEdit);
}

void SetupWizard::accept() {
    auto& cfg = ConfigManager::instance();

    cfg.setHotkey(m_tempConfig["hotkey"].toString());
    cfg.setReplayHotkey(m_tempConfig["replay_hotkey"].toString());
    cfg.setSaveDir(m_tempConfig["save_dir"].toString());
    qDebug() << "Saving directory to config:" << m_tempConfig["save_dir"].toString();
    cfg.setQuality(m_tempConfig["quality"].toInt());
    cfg.setReplayEnabled(m_tempConfig["replay_enabled"].toBool());
    cfg.setReplayDuration(m_tempConfig["replay_duration"].toInt());
    cfg.setReplayInterval(m_tempConfig["replay_interval"].toInt());
    cfg.setReplayScale(m_tempConfig["replay_scale"].toInt());
    cfg.setAutostart(m_tempConfig["autostart"].toBool());
    cfg.setDesktopShortcut(m_tempConfig["desktop_shortcut"].toBool());
    cfg.setNotificationsEnabled(m_tempConfig["notifications_enabled"].toBool());
    cfg.setSoundEnabled(m_tempConfig["sound_enabled"].toBool());
    cfg.setNotificationDuration(m_tempConfig["notification_duration"].toInt());

    QWizard::accept();
}

#include "SetupWizard.moc"