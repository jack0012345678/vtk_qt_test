# DICOM Viewer (VTK + Qt5)

這是一個使用 **VTK 8.2** 搭配 **Qt5** 開發的簡易 DICOM viewer，可顯示 Axial、Coronal、Sagittal、Volume 及 PolyData view

## 開發環境

- Windows 11
- Qt 5.12.8
- VTK 8.2
- CMake 3.22
- Visual Studio 2022

---

## 編譯步驟（Debug）

```bash
# 1. 在專案目錄下建立 build_debug 資料夾
cmake -B build_debug -DCMAKE_BUILD_TYPE=Debug

# 2. 編譯專案
cmake --build build_debug --config Debug

# 3. 將Qt相關 DLL複製到執行檔目錄
windeployqt --debug build_debug/Debug/vtk_qt_test.exe

# 4. 將編譯好的VTK相關DLL相關DLL複製到執行檔目錄