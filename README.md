# qt-tutorial-tw

繁體中文 Qt 教學範例集，使用 **CMake** 與 **Qt 6 / Qt 5 (Widgets)** 建構，逐步示範從 Hello World 開始的桌面應用程式開發。

## 環境需求

- Qt 6.x（建議 6.9.1）或 Qt 5.x，需安裝 `Widgets` 模組
- CMake 3.16 以上
- 支援 C++17 的編譯器（Clang / GCC / MSVC）
- macOS / Windows / Linux 皆可

## 專案結構

```
qt-tutorial-tw/
└── examples/
    └── 01-hello-world/
        ├── CMakeLists.txt
        ├── main.cpp
        ├── helloworld.h
        ├── helloworld.cpp
        └── helloworld.ui
```

## 編譯與執行

以 `01-hello-world` 為例：

```bash
cd examples/01-hello-world

# 設定 build 目錄（請依實際 Qt 安裝路徑調整 CMAKE_PREFIX_PATH）
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.9.1/macos

# 編譯
cmake --build build

# 執行（macOS）
open build/01-hello-world.app

# 執行（Linux / Windows）
./build/01-hello-world
```

也可直接以 **Qt Creator** 開啟 `CMakeLists.txt` 進行編譯與除錯。

## 範例列表

| 編號 | 範例名稱 | 說明 |
|------|---------|------|
| 01 | [hello-world](examples/01-hello-world) | 建立第一個 Qt 視窗應用程式，認識 `QApplication`、`QMainWindow` 與 `.ui` 檔案 |

> 後續範例會陸續新增，例如 Signals & Slots、Layouts、Model/View、QML 等主題。

## 授權

本專案採用 MIT License（如需調整請自行修改）。
