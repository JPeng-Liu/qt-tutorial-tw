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

## 常見問題

| 問題 | 解法 |
|------|------|
| `Could not find Qt` | 加上 `-DCMAKE_PREFIX_PATH=/path/to/Qt/<ver>/<platform>` |
| 修改了 `.ui` 卻沒生效 | 重新 build；確認 `CMAKE_AUTOUIC` 為 `ON` |
| 加了 signal/slot 卻連不上 | 確認類別內有 `Q_OBJECT`，並重新 build（觸發 MOC） |
| 視窗沒出現 | 忘了呼叫 `w.show()` 或忘了 `a.exec()` |

## 延伸練習

1. 把 `QMainWindow` 換成 `QWidget`，觀察少了什麼。
2. 在 Designer 中拖一個 `QPushButton`，按下時用 `QMessageBox` 顯示「Hello, Qt!」。
3. 改用 `QLabel` 顯示文字，並嘗試以程式碼（而非 `.ui`）動態建立元件。

## 下一步

→ 02 - Signals & Slots：認識 Qt 最核心的事件溝通機制。
