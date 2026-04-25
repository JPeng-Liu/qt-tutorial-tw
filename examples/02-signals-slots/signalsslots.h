#ifndef SIGNALSSLOTS_H
#define SIGNALSSLOTS_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class SignalsSlots;
}
QT_END_NAMESPACE

class SignalsSlots : public QMainWindow
{
    Q_OBJECT

public:
    SignalsSlots(QWidget *parent = nullptr);
    ~SignalsSlots();

signals:
    // 自訂 signal：當使用者按下「重置」時發出，並夾帶重置前的數值
    void valueReset(int previousValue);

private slots:
    // 自訂 slot：更新顯示文字
    void updateValueLabel(int value);

    // 處理「重置」按鈕：清空輸入並 emit valueReset
    void onResetClicked();

private:
    Ui::SignalsSlots *ui;
    int m_resetCount = 0;
};
#endif // SIGNALSSLOTS_H
