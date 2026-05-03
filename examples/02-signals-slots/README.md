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

> **🤖 AI 常誤**
>
> 直接問 AI「Qt connect lambda 範例」，或翻 Stack Overflow 歷年答案，**精簡版**幾乎處處都是：
>
> ```cpp
> // 精簡版（常見但不安全）
> connect(saveButton, &QPushButton::clicked, [this]() {
>     // ...
> });
> ```
>
> 注意：**沒有第三個參數**。這個版本能編譯、能跑、開發階段也測得過——所以它在範例與教學裡氾濫。但它在 production 不安全：lambda 不具備 QObject 屬性，Qt 無法追蹤其生命週期，`this` 一旦提早 destroy 就 crash。詳見「常見錯誤 / anti-pattern §A」。
>
> **守則**：每個 `connect(..., lambda)` 都養成傳第三個參數作為 context object 的肌肉記憶——通常傳 `this`：
>
> ```cpp
> // 安全版（推薦）
> connect(saveButton, &QPushButton::clicked, this, [this]() {
>     // ...
> });
> ```
>
> 多寫一個逗號 + 一個物件指標，換來的是 production 環境記憶體安全。
>

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

## 常見錯誤 / anti-pattern

### A. Lambda 不傳 context object：物件 destroy 後 crash

**症狀**：你寫了一個典型的跨視窗回呼：

```cpp
// 在 detail 視窗的 constructor
connect(saveButton, &QPushButton::clicked, [this, mainWindow]() {
    mainWindow->refresh();
});
```

自己測試**完全正常**：開 detail、按儲存、list 更新、關閉、再開、再儲存——一切都順。

但只要使用者「狂點」——detail 開了之後馬上 ESC 關掉、再雙擊 row 開新的、連續做幾十次——就會收到：

```
EXC_BAD_ACCESS at MyDetailDialog::on_saveButton_clicked()::operator()()
```

**為什麼會發生「懸空指標」？呼叫源在哪？**

關鍵點是：「按鈕都已經跟著視窗 destroy 了，誰來叫 lambda？」

在 Qt 中，`QObject` 的銷毀分為兩個層面：**C++ 物件析構** 與 **Qt Event Loop 中的 Deferred Delete**。

當你按下儲存鈕時，`QPushButton::clicked` 被觸發，這時 Qt 會把這個 signal 放進 **Event Queue** 中。如果在極短時間內，detail 視窗因為某些邏輯（或程式碼後面接了 `deleteLater()`）被銷毀，但 Event Queue 中**仍有尚未處理完的 signal 請求**——

當 lambda 執行時，它會嘗試讀取 `this`（`MyDetailDialog`）的成員或進入該物件的作用域。如果 `MyDetailDialog` 物件已經被 `delete`，但 lambda 的執行緒（通常是 Main UI Thread）剛好執行到那一段，`this` 就成了**懸空指標 (Dangling Pointer)**。

呼叫源通常**不是因為 worker thread**，而是因為 **Deferred Deletion**：當你呼叫 `deleteLater()` 或關閉視窗時，物件不會立刻從記憶體消失，而是等待 Event Loop 處理。若此時有一個 signal 已經發出並排在佇列中，且該 lambda 沒有指定 context object，Qt 就**無法在 `this` 銷毀時自動切斷 (disconnect) 該連線**。

**核心原因**：lambda 本身**不具備 QObject 屬性**，Qt 無法追蹤其生命週期。

**正反論點分析**

針對這個寫法，比較「不帶 context」與「帶 context」的差異：

| 方案 | 寫法 | 優點 | 缺點 |
|---|---|---|---|
| **A. 不帶 context** | `connect(btn, &QPushButton::clicked, [this, mainWindow](){...});` | 語法最精簡 | **不安全**。如果 `btn` 的生命週期長於 `this`（例如 `btn` 是全域控制項），或像「狂點」那樣極速操作導致 `this` 領先於連線處理被釋放，程式就會 crash |
| **B. 帶 context（推薦）** | `connect(btn, &QPushButton::clicked, this, [this, mainWindow](){...});` | **記憶體安全**。`this` 消失，連線自動切斷 | 多寫一個參數 |

**補充**：如果連 `mainWindow` 都有可能比 detail 先死（雖然機率極低），你甚至應該考慮**把 `mainWindow` 作為 context**。

**實戰補述（作者經驗）**：本系列作者親自處理過這類 issue。**crash 是因為沒有 context 導致，lambda 內 capture 的變數被釋放，導致非法存取**——不限於 `this`，任何被 capture 的物件指標 / reference 都可能踩中。

> ⚠️ 這個坑在開發階段幾乎抓不到——測試環境沒人會「狂點」。它通常在 production 被真實使用者撞出來。**寧可在每個 connect+lambda 都養成傳 context 的肌肉記憶**，也不要靠 code review 抓。

### B. 跨執行緒 connect 的 ConnectionType 誤解

**症狀**：你寫了一個典型的背景工作 worker：

```cpp
class Worker : public QObject {
    Q_OBJECT
public slots:
    void doHeavyWork() { /* 跑幾秒 */ emit progress(50); }
signals:
    void progress(int percent);
};

// 在 MainWindow constructor:
auto *worker = new Worker;
auto *thread = new QThread;
worker->moveToThread(thread);
connect(worker, &Worker::progress, this, &MainWindow::updateProgressBar);
thread->start();
```

跑起來基本對了，但有幾個怪事：

1. emit 後立刻讀某個狀態，讀到的是**舊值（stale）**。
2. 偶爾 console 噴一行：`QObject::connect: Cannot queue arguments of type 'MyCustomStruct'`。
3. 聽朋友說「跨 thread connect 改 `Qt::DirectConnection` 比較快」，照做後 race 或 crash。

#### ConnectionType 三選一

| 類型 | 行為 | 風險 |
|---|---|---|
| **`Qt::QueuedConnection`** | 異步。Signal 丟進隊伍後立刻返回。Slot 在 receiver 執行緒執行。 | **延遲**。檢查狀態時可能尚未更新。 |
| **`Qt::DirectConnection`** | 同步。像直接呼叫 function 一樣。在 sender 執行緒執行。 | **地獄入口**。如果 slot 會存取物件成員，而該物件屬於另一個 thread，則**必噴 race condition 或 crash**。 |
| **`Qt::BlockingQueuedConnection`** | 同步。Sender 會**卡住**直到 receiver 執行完畢。 | **死鎖 (Deadlock)**。如果 sender / receiver 在**同一個 thread**，會直接把自己卡死。 |

#### 建議與總結

- **不要隨意改 ConnectionType**：讓 Qt 決定。
- **狀態回傳**：如果你需要知道 `doHeavyWork` 的結果，請**定義一個 `finished(ResultStruct)` 訊號**，而不是 emit 後立刻讀取。「透過下一個 signal 通知」是 Qt 的正確姿勢。
- **註冊型別**：任何自定義結構體要過 signal / slot，請務必 `qRegisterMetaType`。

#### 為什麼會 stale？

因為跨 thread 的 connect 預設是 `Qt::QueuedConnection`——當你執行 `invokeMethod` 或 emit 後，slot 只是**「排進隊伍」**，並未立即執行。程式碼會立刻往下走，這時去讀取狀態，slot 可能還在排隊，所以你讀到的是**舊值**。

#### `Cannot queue arguments of type 'MyCustomStruct'` 是怎麼回事？

當訊號跨執行緒傳輸時（`QueuedConnection`），Qt 必須把你的參數**「複製一份」**存進事件佇列中。但 Qt 的元物件系統 (MOC) **並非預設認識所有的 C++ 型別**。如果你的 `MyCustomStruct` 沒有在元物件系統中註冊，Qt 就不知道如何 copy 這個結構體。

**修復方法**：在程式初始化時呼叫：

```cpp
qRegisterMetaType<MyCustomStruct>("MyCustomStruct");
```

**MOC 與 `qRegisterMetaType` 的角色分工**：

- **MOC 的角色**：在**編譯期**生成代碼，記錄物件的 slot / signal 資訊。
- **`qRegisterMetaType` 的角色**：這是**執行期**的動作。它告訴 Qt 的型別資料庫：「以後看到這個名字，請用這個型別的 copy constructor 來處理它」。

> ⚠️ 如果你的參數要在 signal / slot 中傳遞，該結構體必須具備 **public default constructor** 和 **public copy constructor**。

### C. connect 兩次：Slot 重複觸發

**症狀**：你有一個 `init()` 函式被多種路徑呼叫——constructor 裡呼叫一次（建立連線），「重新整理」按鈕按下時也呼叫（讓使用者更換資料源後重連）：

```cpp
void MyWindow::init() {
    connect(submitButton, &QPushButton::clicked,
            this, &MyWindow::onSubmit);
    // ... load data
}
```

第一次按 submit：`onSubmit` 跑 **1** 次。
按 refresh、再按 submit：跑 **2** 次。
再 refresh、再 submit：**3** 次。

**沒 crash、沒警告**——同一個 slot 在每次 refresh 後被多叫一次。發現的時候通常是「某個對話框被開了 N 次」這類使用者反應。

#### Qt 為什麼不阻止重複連接？

這看似是 bug，但實際上是 Qt 核心團隊的**效能考量與架構設計選擇**。

**效能考量 (O(1) vs O(N))**：

- 預設情況下（`Qt::AutoConnection` 或 `Qt::DirectConnection`），`QObject::connect` 的實作是將一個 connection 物件附加到 sender 的內部連接串列（linked list 或 vector）中。**這是一個 O(1) 的操作**。
- 如果 Qt 預設要阻止重複連接，每次呼叫 `connect` 時，都必須**遍歷該 sender 現有的所有連接**，核對 `(signal, receiver, slot)` 是否已經存在。這會讓時間複雜度變成 **O(N)**。
- 在擁有數千個物件與連接的大型 GUI 應用程式啟動時，這會造成**顯著的效能瓶頸**。

**合理使用場景**——同一個訊號觸發同一個槽函數多次是合理且必要的：

- **綁定不同參數 (Bound arguments)**：使用 `std::bind` 或 lambda 時，同一個槽函數可能被連接多次，但每次攜帶不同的上下文資料。如果 Qt 預設阻擋，這種設計就無法實現。
- **多重狀態機路由**：狀態機中，同一個按鈕點擊可能因為處於不同平行狀態，需要將事件派發給同一個接收者的不同處理邏輯（或同一個處理邏輯處理多份實體）。
- **Log / 計數器系統**：某些情況下，你可能真的需要每觸發一次特定條件，就為同一個目標增加一次權重或寫入多筆紀錄。

**結論**：Qt **將「是否檢查重複」的權力下放給開發者**，以換取預設情況下的最高效能與最大靈活性。

#### `Qt::UniqueConnection` 是「銀彈」嗎？

**它不是銀彈**——尤其在現代 C++（C++11 之後引入 lambda）的開發環境中，它甚至會是一個**隱患**。

**判定範圍**：

`Qt::UniqueConnection` 的判定標準非常嚴格——必須是**完全相同的 `(sender, signal, receiver, slot)` 4-tuple** 才算重複並拒絕連接。

| Slot 寫法 | UniqueConnection 是否能擋 |
|---|---|
| 字串巨集 `SIGNAL() / SLOT()` | ✅ 完美擋住 |
| 函式指標 `&MyWindow::onSubmit` | ✅ 完美擋住 |
| **Lambda 表達式** | ❌ **形同虛設** |

**Lambda 的致命傷**：

- 在 C++ 中，每一個 lambda 表達式在編譯期都會被編譯器轉換成一個**獨一無二的匿名類別 (functor)**。
- 即使兩個 lambda 的程式碼**完全一模一樣**（甚至寫在同一個迴圈裡），Qt 底層在比較物件型別或記憶體位址時，依然會（或極有可能）判定它們是「**不同的 slot**」。
- 這導致 `Qt::UniqueConnection` 形同虛設——同樣的動作依然會綁定多個不同的匿名物件，造成多次觸發。

#### 正確的解決方案：架構解耦

與其依賴 `Qt::UniqueConnection` 來「擦屁股」，不如從**生命週期管理**或**連線控制**下手。

最根本的解法是認知到：**「建立 UI 事件連線」和「載入資料」是不同的生命週期**。

```cpp
MyWindow::MyWindow() {
    setupConnections();  // 整個物件生命週期只跑一次
    init();              // 初始化資料
}

void MyWindow::setupConnections() {
    // 這裡保證整個物件生命週期只跑一次
    connect(submitButton, &QPushButton::clicked,
            this, &MyWindow::onSubmit);
}

void MyWindow::init() {
    // 按下 refresh 時只呼叫這裡，不碰 connect
    loadData();
    updateUI();
}
```

這個 pattern 把「**只該做一次的事**」（建立 signal / slot 連線）與「**可能被觸發多次的事**」（載入資料、更新顯示）分離成不同函式，從根本上避免了 `connect` 在 refresh 路徑被重複呼叫的可能。

## 設計脈絡 / 為什麼這樣設計

### 1. Context object 是什麼？跟 receiver 是同一個東西嗎？

從 `QObject::connect` 的演進歷史來看，第三個參數的角色變化很有故事：

| 形式 | 語法 | 第三個參數的作用 |
|---|---|---|
| **舊版（string-based）** | `connect(s, SIGNAL(...), r, SLOT(...))` | **Receiver**：明確指定由誰來執行這個 slot。因為 `SLOT(...)` 是字串，必須有 receiver 物件來查表 |
| **新版（functor / lambda）** | `connect(s, &S::sig, [=](){...})` | **無**。Lambda 本身不具備 QObject 屬性，Qt 無法追蹤其生命週期 |
| **安全版（lambda + context）** | `connect(s, &S::sig, ctx, [=](){...})` | **Context object**：充當「守護者」與「執行環境」 |

**本質上，Context object 就是 Receiver。** 它的兩個核心作用：

1. **生命週期守護**：當你傳入 `this` 作為第三個參數，Qt 會把此連線**註冊在 `this` 的內部清單中**。一旦 `this` 被銷毀，Qt 會立刻偵測到並執行 disconnect。Lambda 就絕對不會在 `this` 死後被呼叫。

2. **執行緒親和性 (Thread Affinity)**：在多執行緒環境下，傳入 context 可以確保 lambda 在 **context 所屬的執行緒**中執行。如果沒傳，lambda 會在信號發出的那個執行緒直接執行（類似 `Qt::DirectConnection`）。

這也回答了一個容易困惑的點：**為什麼不能讓 lambda 自己當 receiver？** 因為 lambda 不是 `QObject`，Qt 沒辦法在 lambda 上掛 disconnect 觸發器，也沒辦法問 lambda「你住在哪個 thread？」。所以必須有一個外部的 QObject 作為「代理身份證」——這就是 context 的本質。

### 2. 為什麼會有 ConnectionType 與 AutoConnection？

預設的 `Qt::AutoConnection` 不是「Qt 偷懶」——它在每次 emit 時做一個關鍵判斷。

**判斷邏輯**：當 emit 訊號時，Qt 會檢查兩件事：

- **Sender 所在的 thread**（signal 發出的當下執行緒）。
- **Receiver 所在的 thread**（slot 被宣告時歸屬的執行緒，即 `receiver->thread()`）。

如果兩者**相同**，自動轉為 `DirectConnection`；如果**不同**，則轉為 `QueuedConnection`。

**為什麼要這個機制？** 核心目的是 **執行緒安全 (Thread Safety)**。

- **避免 race condition**：如果不同執行緒可以直接呼叫另一個執行緒的物件函式，會導致多個執行緒同時讀寫同一個物件的成員變數。
- **串流化處理**：`QueuedConnection` 本質上是將 slot 的呼叫**封裝成一個 `QEvent`**，丟進 receiver 所在執行緒的 Event Loop 佇列。這保證了該物件的所有 slot 都會在它「自己的執行緒」中**依序執行**，就像**排隊買票**一樣，不會發生插隊或同時搶票的情況。

換句話說：`AutoConnection` 不是「Qt 偷懶不讓你選」，而是「Qt 替你做了正確的執行緒安全選擇」——只有在你**真的知道自己在做什麼**的場合，才該手動覆蓋。這也是為什麼 anti-pattern §B 把「不要隨意改 ConnectionType」列為第一條建議。

<!-- TODO(Hugo): §3 待對談（候選：為什麼不直接 callback / std::function？為什麼要 MOC 而不是純 C++ 模板？emit 為什麼是空巨集？signal 為什麼宣告卻不實作？）。-->

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

> **跨平台註腳**（建議由 Hugo 取捨）：
> - **macOS**：`open build/02-signals-slots.app` 開啟 bundle；用 lldb 除錯 signal/slot 時，需指定 `bundle/Contents/MacOS/02-signals-slots` 為執行檔。
> - **Windows**：MSVC 與 MinGW build 出的 connect 錯誤訊息格式略有不同；MinGW 對 lambda capture 的型別推導歷史上比 MSVC 嚴格。
> - **Linux**：若使用發行版內建的 Qt，注意 Qt5 / Qt6 共存時 `find_package(QT NAMES Qt6 Qt5)` 找到的版本可能與你預期不符；可加 `-DQT_VERSION=6` 或設環境變數 `QT_DEFAULT_MAJOR_VERSION` 強制。

## 延伸練習

1. 把 lambda 改寫成成員 slot，比較兩種寫法的可讀性。
2. 在 `valueLabel` 顯示的同時，也讓 `windowTitle()` 顯示目前值。**不需要改 `updateValueLabel`**，只要再 `connect` 一條 signal 即可。
3. 加一個 `QCheckBox`，勾選時暫停同步：用 `disconnect()` 解開 spinBox ↔ slider 的連線，取消勾選時再 `connect()` 回去。
4. 把 `valueReset(int)` 改成帶兩個參數 `valueReset(int previous, int current)`，觀察 MOC 自動更新。

## 下一步

→ [03 - Layouts](../03-layouts)：認識 `QHBoxLayout` / `QVBoxLayout` / `QGridLayout` / `QFormLayout`，學會用 Qt Designer 與程式碼兩種方式建立 layout，讓視窗能跟著大小自動排版。
