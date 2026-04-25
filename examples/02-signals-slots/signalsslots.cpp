#include "signalsslots.h"
#include "./ui_signalsslots.h"

#include <QMessageBox>

SignalsSlots::SignalsSlots(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::SignalsSlots)
{
    ui->setupUi(this);

    // 1. 內建 signal → 內建 slot：spinBox 與 slider 雙向同步
    //    QSpinBox::valueChanged 有兩個多載（int / const QString &），
    //    必須用 qOverload<int> 指定要連接哪一個。
    connect(ui->spinBox, qOverload<int>(&QSpinBox::valueChanged),
            ui->slider,  &QSlider::setValue);
    connect(ui->slider,  &QSlider::valueChanged,
            ui->spinBox, &QSpinBox::setValue);

    // 2. 內建 signal → 自訂成員 slot：每次數值改變就更新 QLabel
    connect(ui->slider, &QSlider::valueChanged,
            this,       &SignalsSlots::updateValueLabel);

    // 3. 內建 signal → 自訂成員 slot：按鈕觸發重置流程
    connect(ui->resetButton, &QPushButton::clicked,
            this,            &SignalsSlots::onResetClicked);

    // 4. 內建 signal → lambda（含 context object 以便自動 disconnect）
    connect(ui->aboutButton, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this, tr("關於"),
            tr("本範例示範 Qt 的 Signals & Slots：\n"
               "• 雙向同步\n• 自訂 signal/slot\n• lambda 連接"));
    });

    // 5. 自訂 signal → lambda：每次重置時於狀態列顯示前一個值
    connect(this, &SignalsSlots::valueReset, this, [this](int previousValue) {
        ui->statusbar->showMessage(
            tr("已重置（前一個值：%1）").arg(previousValue), 2000);
    });
}

SignalsSlots::~SignalsSlots()
{
    delete ui;
}

void SignalsSlots::updateValueLabel(int value)
{
    ui->valueLabel->setText(tr("目前值：%1").arg(value));
}

void SignalsSlots::onResetClicked()
{
    const int previous = ui->spinBox->value();

    // 設定 spinBox → 觸發 valueChanged → slider 與 label 自動跟著更新
    ui->spinBox->setValue(0);

    ++m_resetCount;
    ui->resetCountLabel->setText(tr("重置次數：%1").arg(m_resetCount));

    emit valueReset(previous);
}
