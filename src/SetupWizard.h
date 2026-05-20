#ifndef SETUPWIZARD_H
#define SETUPWIZARD_H

#include <QWizard>
#include <QKeySequence>

class QLineEdit;
class QComboBox;
class QCheckBox;
class QSpinBox;
class QKeySequenceEdit;

class SetupWizard : public QWizard
{
    Q_OBJECT
public:
    explicit SetupWizard(QWidget *parent = nullptr);

    void accept() override;

private:
    void initPages();

    QWizardPage* createHotkeyPage();
    QWizardPage* createDirPage();
    QWizardPage* createQualityPage();
    QWizardPage* createReplayPage();
    QWizardPage* createNotificationPage();
    QWizardPage* createOtherPage();
    QWizardPage* createConfirmPage();

    // 页面控件
    QKeySequenceEdit* m_hotkeyEdit = nullptr;
    QKeySequenceEdit* m_replayHotkeyEdit = nullptr;
    QLineEdit* m_dirEdit = nullptr;
    QComboBox* m_qualityCombo = nullptr;
    QCheckBox* m_replayEnableCheck = nullptr;
    QSpinBox* m_replayDurationSpin = nullptr;
    QSpinBox* m_replayIntervalSpin = nullptr;
    QComboBox* m_replayScaleCombo = nullptr;
    QCheckBox* m_notificationCheck = nullptr;
    QCheckBox* m_soundCheck = nullptr;
    QComboBox* m_durationCombo = nullptr;
    QCheckBox* m_autostartCheck = nullptr;
    QCheckBox* m_desktopCheck = nullptr;

    QHash<QString, QVariant> m_tempConfig;
};

#endif // SETUPWIZARD_H