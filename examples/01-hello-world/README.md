# 01 - Hello World

第一個 Qt 範例：建立一個最基本的視窗應用程式，認識 Qt Widgets 專案的最小組成。

## 學習目標

完成本範例後，你將了解：

- 一個 Qt Widgets 專案最少需要哪些檔案
- `QApplication` 與 `QMainWindow` 的角色
- `.ui` 檔案如何透過 `uic` 自動轉成 C++ 程式碼
- `Q_OBJECT` 巨集與 MOC（Meta-Object Compiler）的關係
- CMake 中 `AUTOMOC` / `AUTOUIC` / `AUTORCC` 的用途

## 檔案結構

```
01-hello-world/
├── CMakeLists.txt   # 建置設定
├── main.cpp         # 程式進入點
├── helloworld.h     # 主視窗類別宣告
├── helloworld.cpp   # 主視窗類別實作
└── helloworld.ui    # Qt Designer 視覺化介面檔
```

## 逐檔案解析

### 1. `main.cpp` — 程式進入點

```cpp
#include "helloworld.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    HelloWorld w;
    w.show();
    return a.exec();
}
```

- **`QApplication a`**：每個 Qt GUI 程式都必須有且僅有一個 `QApplication` 物件，負責管理事件迴圈、全域設定與資源。
- **`HelloWorld w`**：建立主視窗物件（在 stack 上）。
- **`w.show()`**：Qt 的 widget **預設是隱藏的**，必須呼叫 `show()` 才會顯示。
- **`a.exec()`**：啟動事件迴圈，程式會停在這裡處理事件，直到視窗被關閉才返回。

### 2. `helloworld.h` — 類別宣告

```cpp
class HelloWorld : public QMainWindow
{
    Q_OBJECT

public:
    HelloWorld(QWidget *parent = nullptr);
    ~HelloWorld();

private:
    Ui::HelloWorld *ui;
};
```

重點：

- 繼承自 **`QMainWindow`**，提供選單列、工具列、狀態列等標準視窗結構（若只需簡單視窗可改用 `QWidget`）。
- **`Q_OBJECT` 巨集**：啟用 Qt 的 meta-object 系統（signals & slots、`qobject_cast`、屬性系統等）。只要繼承 `QObject` 並想用這些功能，就一定要加。
- **`Ui::HelloWorld *ui`**：指向由 `helloworld.ui` 自動產生的介面類別，內含 Designer 中拉好的所有元件。

> **🤖 AI 常誤**
>
> 直接問 AI「Qt 怎麼寫一個簡單的視窗類別」，它有時會給你類似這樣的程式碼：
>
> ```cpp
> class HelloWorld : public QMainWindow
> {
> public:
>     HelloWorld(QWidget *parent = nullptr);
> };
> ```
>
> 注意：**沒有 `Q_OBJECT`**。這份程式碼能編譯、能跑、視窗也會出來。但只要你之後想加 signal / slot、用 `qobject_cast`、或用 `tr()` 做多國語言，就會悄悄失敗。
>
> 失敗的方式還很有迷惑性——舊式 connect 寫法會在 console 印 `No such signal`，但你檢查標頭檔的拼字明明對；換成新式寫法又會在連結期噴 `undefined reference`。詳見本文「常見錯誤 / anti-pattern」§A。
>
> **守則**：任何繼承自 `QObject`（或其子類別如 `QWidget`、`QMainWindow`）的類別，**永遠在 class body 第一行加上 `Q_OBJECT`**，不管現在有沒有用到 signal / slot——將來一定會用到。
>

### 3. `helloworld.cpp` — 類別實作

```cpp
HelloWorld::HelloWorld(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::HelloWorld)
{
    ui->setupUi(this);
}

HelloWorld::~HelloWorld()
{
    delete ui;
}
```

- **`new Ui::HelloWorld`**：建立 UI 物件實體。
- **`ui->setupUi(this)`**：把 `.ui` 檔案中設計的元件套用到 `this`（也就是這個主視窗）。
- 解構時 `delete ui` 釋放記憶體（這裡 `ui` 不是 QObject 子物件，因此不會被 Qt 的 parent-child 機制自動回收，需要手動釋放）。

### 4. `helloworld.ui` — 介面定義

由 **Qt Designer** 編輯的 XML 檔，描述視窗的元件、佈局與屬性。
建置時，CMake 的 `AUTOUIC` 會呼叫 `uic` 將它轉成 `ui_helloworld.h`，提供 `Ui::HelloWorld` 類別。

> 想修改視窗外觀？用 Qt Creator 或 Qt Designer 直接開啟 `.ui` 檔即可，無需改 C++ 程式碼。

### 5. `CMakeLists.txt` — 建置設定

關鍵段落：

```cmake
set(CMAKE_AUTOUIC ON)   # 自動處理 .ui  → ui_*.h
set(CMAKE_AUTOMOC ON)   # 自動處理 Q_OBJECT → moc_*.cpp
set(CMAKE_AUTORCC ON)   # 自動處理 .qrc → qrc_*.cpp

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

target_link_libraries(01-hello-world PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
```

- **AUTOMOC / AUTOUIC / AUTORCC**：開啟後不必手動執行 `moc`、`uic`、`rcc`，CMake 會自動處理。
- **`find_package` 兩段式寫法**：先找 Qt6 或 Qt5（取得 `QT_VERSION_MAJOR`），再針對該版本找需要的元件，達到 Qt5/Qt6 雙相容。

## 常見錯誤 / anti-pattern

### A. 漏寫 `Q_OBJECT`：connect 安靜失敗

**症狀**：你跟著 01 跑通 hello world 後，自己拉了一個 `QPushButton`，加了一段 connect，結果：

- **Build 階段完全沒錯誤**
- Run 起來，視窗也正常
- 點按鈕，沒反應
- 沒有 crash、沒有警告

唯一可疑的線索是 console 印出這一行：

```
QObject::connect: No such signal HelloWorld::clicked() in helloworld.cpp:23
```

**這行警告是執行期 (Runtime) 才印出來的，在 Build（編譯）階段完全不會有任何錯誤。**

**為什麼 Build 會通過？**

如果你用 Qt 4 的舊式巨集寫法：

```cpp
connect(btn, SIGNAL(clicked()), this, SLOT(onClicked()));
```

在 C++ 編譯器的眼裡，`SIGNAL` 和 `SLOT` 只是將裡面的內容轉成字串。編譯器只檢查 `connect` 這個函式的參數型別是否正確（兩個指標、兩個字串），它根本不知道、也不在乎 `clicked()` 這個函式存不存在。因此，編譯完美通過。

**這行警告的「詐胡」程度有多高？**

當程式執行到第 23 行時，Qt 的底層邏輯是這樣運作的：

1. `connect` 拿到一個字串 `"clicked()"`。
2. 它去問 `HelloWorld` 這個物件：「把你的元物件 (Meta-Object) 的字串表交出來，我要找 `clicked()`。」
3. 如果漏了 `Q_OBJECT`：`HelloWorld` 沒有自己的元物件，它只好往上找，交出了父類別（例如 `QWidget` 或 `QObject`）的字串表。
4. Qt 在父類別的表裡找不到 `clicked()`，於是 `connect` 宣告失敗，回傳 `false`，並在 console 印出 `No such signal`。

新手視角的困境：看到 `No such signal`，第一反應絕對是：「怎麼可能！我標頭檔明明寫了 `signals: void clicked();`！」於是反覆檢查拼字、參數，浪費大量時間。

**訊號在 C++ 層面是存在的，只是在 Qt 的元物件字串表中不存在。**

**如何確認是漏了 `Q_OBJECT`？**

唯一的線索是：**你確認拼寫 100% 正確，參數也對應，但就是連不上**。這時請立刻檢查 header 有沒有 `Q_OBJECT`。

> ⚠️ 有時候加了 `Q_OBJECT` 還是沒用，那是因為**建構系統沒察覺檔案改變**。需要手動執行 **Clean → Run qmake / CMake → Rebuild** 讓 MOC 重新掃描。這個動作對 IDE 內建的快速 Build 與 CMake 都成立。

**反方對照：用 Qt 5 / Qt 6 的新式寫法呢？**

```cpp
connect(btn, &HelloWorld::clicked, this, &HelloWorld::onClicked);
```

這時如果漏了 `Q_OBJECT`，你**根本無法成功 Build**。

推導：在 Qt 中，`signals:` 區段宣告的函式（如 `clicked()`），開發者是不需要寫實作碼的——是由 MOC 自動生成實作。既然你漏了 `Q_OBJECT`，MOC 就不會幫你產生 `clicked()` 的實作。

結果：編譯器能看懂指標，但到了**連結期 (Linking stage)**，連結器會找不到函式主體，噴出：

```
error: undefined reference to 'HelloWorld::clicked()'
```

這個錯誤訊息就比執行期的字串警告明確多了——**這也是現代 Qt 強烈建議用新式 connect 寫法的隱藏好處之一**：把 runtime 詐胡訊息變成 link-time 明確錯誤。

**順帶一提：`qobject_cast` 也會默默失敗**

如果你的程式碼還有：

```cpp
MyWidget *w = qobject_cast<MyWidget*>(sender());
w->doSomething();  // w 是 nullptr，這裡引發 Segmentation Fault
```

`qobject_cast` 失敗時**回傳 `nullptr`，本身不會 crash**；但下一行的方法呼叫會。表面看起來是程式崩潰，根源是漏 `Q_OBJECT` 導致 cast 失敗——詳見「設計脈絡」§2 解釋為什麼 `qobject_cast` 必須仰賴 MOC。

### B. 建構系統的幽靈：UIC 與影子建構 (Shadow Build)

在 Qt 的世界裡，除了處理 C++ 巨集的 `moc`，另一個關鍵代碼生成器就是 `uic`。當你在 Qt Designer 拖拉 UI 時，存檔的 `.ui` 檔案本質上只是一堆 XML 標籤。C++ 編譯器是看不懂 XML 的，因此需要 `uic` 將 `.ui` 轉換成名為 `ui_helloworld.h` 的 C++ 標頭檔。

這個自動轉換的過程有兩個常見的失靈場景。

#### 場景一：找不到 `ui_helloworld.h`

**症狀**：剛接手專案，或是剛用 IDE 建立好專案，程式碼編輯器立刻在 `#include "ui_helloworld.h"` 底下畫上刺眼的紅線，甚至在 Build 的第一步就噴出：

```
fatal error: ui_helloworld.h: No such file or directory
```

**思考與推導過程**：

**檔案去哪了？** 這個 `ui_*.h` 檔案絕對不在你的原始碼資料夾裡。為了保持原始碼目錄乾淨，Qt 預設採用「**影子建構 (Shadow Build)**」，所有生成的檔案都會放在一個獨立的 build 資料夾中（例如 `build-HelloWorld-Desktop-Debug`）。

**為何 IDE 會報錯？** 兩種可能：

- **情況 A（真報錯）**：你可能還沒有執行過 `qmake` 或 `cmake` 產生建構規則，或者你的 `CMakeLists.txt` 漏寫了 `set(CMAKE_AUTOUIC ON)`。因此 `uic` 根本還沒被呼叫，檔案當然不存在。
- **情況 B（IDE 詐胡）**：程式其實**可以成功編譯並執行**，但 IDE（如 VSCode 的 clangd 或 Visual Studio 的 IntelliSense）依然報錯。這是因為 IDE 的靜態分析工具「不知道」那個隱藏的 build 資料夾路徑，所以它在原始碼目錄找不到檔案就亮紅燈。

#### 場景二：`.ui` 改了卻沒生效

**症狀**：程式可以跑。你在 Qt Designer 裡明明加了一個按鈕，存檔後按 Build & Run，彈出來的視窗卻還是舊的，沒有那個按鈕。**沒有報錯，沒有警告。**

**思考與推導過程**：

**`uic` 有執行嗎？** 當你修改了 `.ui` 檔，建構系統（如 Make 或 Ninja）會發現 `.ui` 的修改時間比 `ui_helloworld.h` 新，於是觸發 `uic` 重新生成了標頭檔。所以新代碼已經產生了。

**編譯器吃到哪一份代碼？** 這是最可怕的坑。當你發生過切換編譯器套件 (Kit)、或是手動搬移過專案資料夾，你的電腦裡可能存在**兩個或多個不同的 build 資料夾**。

**路徑污染（Include Path 順序）**：當 C++ 編譯器執行到 `#include "ui_helloworld.h"` 時，它會按照 `-I` 參數提供的路徑順序去尋找。如果建構環境混亂，編譯器可能會**先撞見「舊 build 資料夾」裡的那份沒更新的 `ui_helloworld.h`**，然後就心滿意足地編譯下去了。

**解法**：最暴力的有效解法是**關閉 IDE，把原始碼目錄以外的 `build-*` 資料夾全部刪除**（徹底清空快取），重新 configure 再編譯。

## 設計脈絡 / 為什麼這樣設計

### 1. 為什麼 Q_OBJECT / MOC 是必要的？

**C++ 繼承的限制**：標準繼承只能傳遞「函式」與「變數」，無法自動生成類別專屬的「字串名稱」或「訊號 / 槽的映射表」。若只依賴父類別 `QObject`，取得的永遠是父類別的資訊。

**`Q_OBJECT` 的作用**：強制在每一個衍生類別內部，注入**專屬的靜態元資料與覆寫函式**（由 MOC 程式碼生成器在編譯前自動完成）。它彌補了標準 C++ 缺乏的「執行期反射 (Reflection)」能力，讓程式碼在執行時能知道物件的類別名稱、擁有哪些函式與屬性。

漏寫此巨集會喪失：

- **訊號與槽 (Signals & Slots)**：無法定義與連結新的 signal / slot。
- **動態轉型**：高效安全的 `qobject_cast` 會失效。
- **動態屬性系統 (`Q_PROPERTY`)**：無法運作。
- **多國語言翻譯 (`tr()`)**：無法運作。

### 2. 為什麼用 `qobject_cast` 而不是 `dynamic_cast`？

如果你用 `dynamic_cast<MyWidget*>(sender)`：

- 這是**純 C++ 標準**。C++ 編譯器在編譯 `MyWidget` 時，因為它繼承自 `QObject`（帶有虛擬函式），編譯器已經偷偷為它建了一張**虛擬函式表 (vtable)** 與 **RTTI (執行期型別資訊)**。
- 執行時，`dynamic_cast` 去查 C++ 自己的 RTTI，發現記憶體裡的真實物件確實是 `MyWidget`，於是**成功轉型**。
- **這件事完全不需要 `Q_OBJECT` 或 MOC 的介入**——這也是為什麼漏 `Q_OBJECT` 時 `dynamic_cast` 仍能動，但 `qobject_cast` 會回 `nullptr`。

如果你用 `qobject_cast<MyWidget*>(sender)`：

- 這是 **Qt 自己寫的機制**。它不看 C++ 的 RTTI，它只看 MOC 生成的元物件樹。
- 你漏了 `Q_OBJECT`，`MyWidget` 就沒有註冊進這棵樹。當 `qobject_cast` 順著樹狀圖比對字串名字時，找不到 `MyWidget` 這個節點，於是判定轉型失敗，給出 `nullptr`。

**那為什麼 Qt 還要推 `qobject_cast`？**

回到架構設計的取捨：

- **效能**：`dynamic_cast` 在某些編譯器下的字串比對極度耗時；`qobject_cast` 只比較指標與查表，速度快很多（有些測試顯示快上 5 到 10 倍）。
- **跨 DLL 穩定性**：在 Windows 上，當你的物件是透過動態連結庫 (DLL) 傳遞時，由於不同 DLL 可能有各自的 C++ 執行時期環境，`dynamic_cast` 經常會詭異地失敗。而 `qobject_cast` 依賴純虛擬函式回傳字串比對，跨 DLL 也能穩定運作。

換句話說：MOC 不只是補上 C++ 的 reflection，也順便解掉了 `dynamic_cast` 在跨 DLL 場景下的長年痛點。

### 3. 為什麼 `Ui::HelloWorld` 是指標 `*ui` 而非 stack 變數？

當你打開 Qt 預設生成的類別，會看到這樣的寫法：

```cpp
namespace Ui { class HelloWorld; }   // 前置宣告 (Forward Declaration)

class HelloWorld : public QMainWindow {
    Q_OBJECT
public:
    HelloWorld(QWidget *parent = nullptr);
    ~HelloWorld();
private:
    Ui::HelloWorld *ui;   // 為什麼這裡一定要是指標？
};
```

這不是 Qt 的怪癖——這是 C++ 中極為經典的 **Pimpl 慣用手法 (Pointer to Implementation)**。

**反方推導：如果改成 stack？**

如果我們把指標拿掉，改成實體變數 `Ui::HelloWorld ui;`（分配在 stack 上），那麼在編譯 `helloworld.h` 時，C++ 編譯器必須知道 `Ui::HelloWorld` 這個類別的**確切記憶體大小**。要讓編譯器知道大小，我們就被迫必須在 `helloworld.h` 頂部寫上 `#include "ui_helloworld.h"`。

這會引發災難性的連鎖反應：

- `ui_helloworld.h` 通常非常巨大，裡面包含了所有按鈕、標籤的 `#include <QPushButton>` 等。
- 任何其他使用到你的 `HelloWorld` 視窗的 C++ 檔案，只要 `#include "helloworld.h"`，就會被迫一起展開那份巨大的 UI 標頭檔。
- **最慘的是**：你只要在 UI 拖拉改動一個按鈕位置，`ui_helloworld.h` 就會改變，這會導致整個專案裡所有 `#include "helloworld.h"` 的檔案**全部都要重新編譯**（編譯時間大爆炸）。

**正方解法：用指標**

使用指標 `*ui`，編譯器只需要知道「這是一個指標（通常佔 8 bytes）」，不需要知道詳細內容。因此可以使用 `namespace Ui { class HelloWorld; }` 這種**前置宣告**混過去。真正的 `#include "ui_helloworld.h"` 被藏在 `helloworld.cpp` 裡面。

這不僅加快了編譯速度，也完美隱藏了 `uic` 生成的機器碼。順帶解釋了一個讀者讀逐檔解析時可能會困惑的點：**為什麼 `helloworld.cpp` 解構子裡需要 `delete ui;`？** 因為它是 raw pointer，不在 Qt 的 parent-child 樹內，必須手動釋放。

## 編譯與執行

```bash
# 從專案根目錄
cd examples/01-hello-world

cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.9.1/macos
cmake --build build

# macOS
open build/01-hello-world.app

# Linux / Windows
./build/01-hello-world
```

或直接用 **Qt Creator** 開啟 `CMakeLists.txt`，選擇 Kit 後按 ▶ 執行。

### 環境設定常見問題

| 問題 | 解法 |
|------|------|
| `Could not find Qt` | 加上 `-DCMAKE_PREFIX_PATH=/path/to/Qt/<ver>/<platform>` |
| 修改了 `.ui` 卻沒生效 | 重新 build；確認 `CMAKE_AUTOUIC` 為 `ON` |

> **跨平台註腳**（建議由 Hugo 取捨）：
> - **macOS**：Qt 通常裝在 `~/Qt/6.x.y/macos`，PREFIX_PATH 指這層即可。Homebrew 安裝的 Qt 路徑為 `/opt/homebrew/opt/qt`，但要注意 Homebrew 的 Qt 預設不含 Qt Creator。
> - **Windows**：MSVC 與 MinGW 各自要對齊 Kit；MSVC 路徑常見於 `C:\Qt\6.x.y\msvc2019_64`。
> - **Linux**：發行版套件（如 `qt6-base-dev`）路徑由系統管理，通常 `find_package` 直接找到，無需 PREFIX_PATH。

## 延伸練習

1. 把 `QMainWindow` 換成 `QWidget`，觀察少了什麼。
2. 在 Designer 中拖一個 `QPushButton`，按下時用 `QMessageBox` 顯示「Hello, Qt!」。
3. 改用 `QLabel` 顯示文字，並嘗試以程式碼（而非 `.ui`）動態建立元件。

## 下一步

→ 02 - Signals & Slots：認識 Qt 最核心的事件溝通機制。
