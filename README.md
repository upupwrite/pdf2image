# pdf2image

将 PDF 的每一页渲染为位图图像，再重新封装成一个新的 PDF。  
生成的 PDF 中每一页都是一张图片，因此文本不可选中，但能保留原始排版与视觉效果。

- 使用 [Poppler](https://poppler.freedesktop.org/) 进行页面渲染
- 使用 [libHaru](https://github.com/libharu/libharu) 生成新的 PDF

---

## 功能

- 读取输入的 PDF 文件
- 按指定 DPI 将每一页渲染为 RGB 图像
- 将图像写入新的 PDF，页面尺寸与图像尺寸匹配
- 支持压缩，减小输出体积

---

## 依赖

### 系统要求

- CMake ≥ 3.10
- 支持 C++17 的编译器（如 g++）
- pkg-config
- Poppler C++ 开发库
- libHaru 开发库

### 在 Debian / Ubuntu 上安装

```bash
sudo apt update
sudo apt install cmake g++ pkg-config libpoppler-cpp-dev libhpdf-dev
```

如果链接阶段提示缺少 `png` 或 `z`，请额外安装：

```bash
sudo apt install libpng-dev zlib1g-dev
```

---

## 构建

在项目根目录（包含 `CMakeLists.txt` 和 `main.cpp`）执行：

```bash
mkdir build
cd build
cmake ..
make
```

编译完成后，可执行文件位于 `build/pdf2image`。

## 安装  

### 默认安装到 `/usr/local`

```bash
mkdir build && cd build
cmake ..
make
sudo make install
```

可执行文件安装到 `/usr/local/bin/pdf2image`。

### 自定义安装路径

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make
make install
```

### 指定 DESTDIR（打包用）

```bash
make DESTDIR=/tmp/stage install
```

### 卸载

CMake 没有内建卸载目标，可手动删除：

```bash
sudo rm /usr/local/bin/pdf2image
sudo rm -rf /usr/local/share/doc/pdf2image
```

或者构建时记录安装清单：

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make
sudo make install
sudo xargs rm < install_manifest.txt
```

## 要点

- `install(TARGETS ...)` 将可执行文件安装到 `${CMAKE_INSTALL_BINDIR}`（通常为 `bin`）。
- `GNUInstallDirs` 提供标准化的安装目录变量，兼容不同发行版。
- `CMAKE_INSTALL_DOCDIR` 通常为 `share/doc/<project>`，用于放置 README。
- `project(... VERSION 1.0.0 ...)` 声明版本号，便于后续打包与依赖管理。



## 使用

```bash
./pdf2imagepdf input.pdf output.pdf
```

- `input.pdf`：源 PDF 文件路径
- `output.pdf`：生成的基于图像的 PDF 文件路径

程序会依次处理每一页，并输出进度信息。

---

## 配置

默认渲染分辨率为 **200 DPI**。  
如需修改，请编辑 `main.cpp` 中的以下行：

```cpp
const double dpi = 200.0;
```

提高 DPI 会使输出更清晰，但文件体积也会显著增大。

输出 PDF 的页面尺寸按 `像素数 × 72 / DPI` 计算，因此默认保持与原始页面相近的物理尺寸。

---

## 注意事项

- 输出 PDF 完全由位图组成，**文本不可搜索、不可选中**。
- 文件体积通常比原始 PDF 大很多，尤其在高 DPI 下。
- 若原始 PDF 有加密或权限限制，程序可能无法打开。
- 如果遇到链接错误，请确认 Poppler 和 libHaru 的开发包已正确安装，必要时在 `CMakeLists.txt` 中手动追加 `png` 和 `z` 库：

  ```cmake
  target_link_libraries(pdf2imagepdf PRIVATE png z)
  ```

---

## 目录结构

```
.
├── CMakeLists.txt
├── README.md
└── main.cpp
```

