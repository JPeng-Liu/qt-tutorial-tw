# 02 - Signals & Slots

Qt 最核心的事件溝通機制。本範例做出一個「SpinBox ↔ Slider 雙向同步 + 重置 + 關於」的小視窗，逐一示範五種常見的 `connect()` 用法。

## 學習目標

完成本範例後，你將了解：

- 什麼是 signal、什麼是 slot，為什麼要用它們而不是 callback
- 現代 Qt 5/6 的 **函式指標式** `connect()` 寫法
- 如何宣告 / 發出（`emit`）自己的 signal
- 如何用 **lambda** 當 slot，以及為什麼要傳入 *context object*
- 多載 signal 為什麼需要 `qOverload<...>(&Class::signal)`
- signal 與 slot 的「多對多」特性、`disconnect()`、自動斷線

## 檔案結構

```
02-signals-slots/
├── CMakeLists.txt
├── main.cpp
├── signalsslots.h
├── signalsslots.cpp
└── signalsslots.ui
```

## 介面長相

| 元件 | objectName | 功能 |
|------|-----------|------|
| `QSpinBox`    | `spinBox`         | 數值輸入（0–100） |
| `QSlider`     | `slider`          | 數值拖曳（0–100） |
| `QLabel`      | `valueLabel`      | 顯示「目前值：N」 |
| `QLabel`      | `resetCountLabel` | 顯示「重置次數：N」 |
| `QPushButton` | `resetButton`     | 將數值歸零並 emit 自訂 signal |
| `QPushButton` | `aboutButton`     | 彈出 `QMessageBox` |

## 觀念速覽

### Signals & Slots 是什麼？

Qt 用 signals & slots 取代傳統的 callback：

- **Signal**：「事件發生了」的廣播（例如「按鈕被點擊」、「數值變了」）。發出者**不需要知道誰在聽**。
- **Slot**：能被 signal 呼叫的函式（任何 member function 幾乎都可以當 slot）。
- **`connect()`**：把 signal 接到 slot。一個 signal 可接到多個 slot；一個 slot 也可被多個 signal 觸發。

底層由 **MOC（Meta-Object Compiler）** 在編譯期掃描 `Q_OBJECT` 類別的 `signals:` / `slots:` 區段，產生 meta-object 程式碼，因此所有 signals & slots 都是型別安全且支援跨執行緒。

### 兩種 `connect()` 語法

```cpp
// 舊式（Qt 4 風格）：字串比對，型別錯誤要等到執行期才會發現
connect(spinBox, SIGNAL(valueChanged(int)), slider, SLOT(setValue(int)));

// 新式（Qt 5/6，建議）：函式指標，編譯期就會檢查型別
connect(spinBox, &QSpinBox::valueChanged, slider, &QSlider::setValue);
```

新式寫法的另一個好處是**可以接 lambda**。本範例只用新式語法。

## 逐段解析（`signalsslots.cpp`）

### ① 內建 signal → 內建 slot：雙向同步

```cpp
connect(ui->spinBox, qOverload<int>(&QSpinBox::valueChanged),
        ui->slider,  &QSlider::setValue);
connect(ui->slider,  &QSlider::valueChanged,
        ui->spinBox, &QSpinBox::setValue);
```

兩個元件互相連接，看起來會無窮遞迴？不會 — Qt 內建元件在「值沒變」時不會再次發 signal，所以雙向同步是安全的。

`qOverload<int>(&QSpinBox::valueChanged)` 是因為 `QSpinBox::valueChanged` 有兩個多載：

```cpp
void valueChanged(int);
void valueChanged(const QString &);  // Qt 5；Qt 6 已移除
```

不指定的話編譯器無法判斷你要哪一個。`QSlider::valueChanged` 只有 `int` 一個版本，所以可以直接寫。

### ② 內建 signal → 自訂成員 slot

```cpp
connect(ui->slider, &QSlider::valueChanged,
        this,       &SignalsSlots::updateValueLabel);
```

`updateValueLabel` 宣告在 `signalsslots.h` 的 `private slots:` 區段：

```cpp
private slots:
    void updateValueLabel(int value);
```

> 小提醒：在新式語法中，slot **不一定要寫在 `slots:` 區段**也能被 connect — 任何成員函式都可以。但放在 `slots:` 區段能讓 MOC 把它登記到 meta-object，方便用字串呼叫（例如 `QMetaObject::invokeMethod`）或在 Qt Designer 裡用「Edit Signals/Slots」拉線。

### ③ 按鈕觸發成員 slot

```cpp
connect(ui->resetButton, &QPushButton::clicked,
        this,            &SignalsSlots::onResetClicked);
```

`QPushButton::clicked` 預設多載是 `clicked()`（無參數），所以不需要 `qOverload`。

### ④ 內建 signal → lambda

```cpp
connect(ui->aboutButton, &QPushButton::clicked, this, [this]() {
    QMessageBox::information(this, tr("關於"), tr("..."));
});
```

第三個參數 `this` 是 **context object**，強烈建議帶上：

- 當 `this` 被 destroy 時，Qt 會**自動斷開**這個連線，避免 lambda 在懸空指標上被呼叫。
- 在多執行緒情境下，context 也決定 lambda 在**哪個 thread** 執行（同 context 物件所在 thread）。

不傳 context（即 `connect(sender, signal, lambda)`）也合法，但 lambda 永遠不會自動斷開，容易出 bug。

### ⑤ 自訂 signal → lambda

```cpp
// 宣告（.h）
signals:
    void valueReset(int previousValue);

// 連接（建構子內）
connect(this, &SignalsSlots::valueReset, this, [this](int previousValue) {
    ui->statusbar->showMessage(
        tr("已重置（前一個值：%1）").arg(previousValue), 2000);
});

// 發出（onResetClicked()）
emit valueReset(previous);
```

重點：

- **`signals:`** 區段的函式**只宣告、不實作** — MOC 會幫你產生實作。
- **`emit`** 只是個空白巨集，純粹給人看的標記，寫不寫都可以編，但寫了能讓讀者一眼看出「這裡發出 signal」。
- 自訂 signal 可以帶任意參數，但型別必須能被 Qt 的 meta system 認得（基本型別、`QString`、`QVariant` 與所有已 `Q_DECLARE_METATYPE` 的型別都可以）。

## 觸發鏈：按下「重置」之後

```
resetButton.clicked
  └─ onResetClicked()
       ├─ spinBox.setValue(0)
       │    └─ spinBox.valueChanged(0)
       │         ├─ slider.setValue(0)
       │         │    └─ slider.valueChanged(0)
       │         │         └─ updateValueLabel(0)  → valueLabel 更新
       │         └─ (slider 已在 0，不會再 emit)
       ├─ resetCountLabel 更新
       └─ emit valueReset(previous)
            └─ statusbar.showMessage("已重置...")
```

一個動作 → 多個 slot 連動，這就是 signals & slots 的價值：**呼叫者不必知道有誰在聽**，新增聽眾也不需要改動發出端。

## 編譯與執行

```bash
cd examples/02-signals-slots

cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.9.1/macos
cmake --build build

# macOS
open build/02-signals-slots.app

# Linux / Windows
./build/02-signals-slots
```

或用 **Qt Creator** 開啟 `CMakeLists.txt` 直接 ▶ 執行。

## 常見問題

| 問題 | 解法 |
|------|------|
| `No matching member function for call to 'connect'` | 多載 signal 沒指定型別，請用 `qOverload<...>(&Class::signal)` |
| 連了 signal 卻沒反應 | 1) 類別有 `Q_OBJECT` 嗎？2) 改完有重新 build（觸發 MOC）嗎？3) `connect()` 的回傳值（`QMetaObject::Connection`）是否為 false |
| Lambda 在物件死掉後 crash | `connect()` 第三個參數帶 context object（通常傳 `this`），讓 Qt 自動斷線 |
| 雙向同步無窮遞迴 | Qt 內建 setter 在值沒變時不會再 emit，安全；如果是自寫 setter，記得加上 `if (m_value == v) return;` |
| 想暫時關掉某條連線 | 用 `disconnect(...)`，或在 `connect()` 時保留回傳的 `QMetaObject::Connection` 之後 `QObject::disconnect(conn)` |

## 延伸練習

1. 把 lambda 改寫成成員 slot，比較兩種寫法的可讀性。
2. 在 `valueLabel` 顯示的同時，也讓 `windowTitle()` 顯示目前值。**不需要改 `updateValueLabel`**，只要再 `connect` 一條 signal 即可。
3. 加一個 `QCheckBox`，勾選時暫停同步：用 `disconnect()` 解開 spinBox ↔ slider 的連線，取消勾選時再 `connect()` 回去。
4. 把 `valueReset(int)` 改成帶兩個參數 `valueReset(int previous, int current)`，觀察 MOC 自動更新。

## 下一步

→ 03 - Layouts（規劃中）：認識 `QHBoxLayout` / `QVBoxLayout` / `QGridLayout`，讓視窗能正確隨大小縮放。
