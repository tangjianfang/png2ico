# Project Description / 项目说明

English:
A lightweight C++14 Windows utility that batch-converts PNG images into multi-resolution ICO files using GDI+. Each PNG in the executable directory (or a specified single PNG file) is cropped to a centered square, rescaled to common icon sizes (16–256 px), PNG-compressed, and packed into a valid ICO container. This simplifies generating high-quality application or shortcut icons without external design tools.

Chinese / 中文:
这是一个基于 C++14 与 GDI+ 的轻量级 Windows 工具，用于批量将 PNG 图片转换为多尺寸 ICO 图标。程序会遍历可执行文件所在目录中的所有 PNG 文件（或处理指定的单一文件），将图片居中裁剪为正方形，生成常用图标尺寸（16–256 像素），采用 PNG 压缩并写入标准 ICO 文件。适合快速为应用或快捷方式生成高质量图标。

## Features / 特性
English:
- Auto batch conversion (no arguments) or single-file mode (one PNG path).
- Center square crop to preserve key visual focus.
- Multiple embedded sizes: 16, 24, 32, 48, 64, 128, 256.
- Uses PNG compression for reduced file size.
- Unicode (wide char) file handling.
- Distinct exit codes: 0 success, 2 partial success, 1 failure.

中文:
- 无参数批量模式或指定单文件模式。
- 居中裁剪正方形保留主体。
- 内嵌多尺寸：16、24、32、48、64、128、256。
- 使用 PNG 压缩减小文件体积。
- 支持 Unicode 文件路径。
- 退出码区分：0 成功，2 部分成功，1 失败。

## Usage / 使用
English:
- Place PNG files beside the executable.
- Run without arguments: converts all *.png → *.ico (same base name).
- Run with one argument: png2ico.exe input.png
- Explorer may cache old icons; clear icon cache if size preview seems incorrect.

中文:
- 将 PNG 文件放在程序同目录。
- 无参数运行：批量转换当前目录所有 *.png → 同名 *.ico。
- 单文件模式：png2ico.exe input.png
- 若预览尺寸异常，可清理系统图标缓存后再查看。

## Build / 构建
English:
- Toolchain: Visual Studio 2022, C++14.
- Link against Gdiplus (pragma already included).
- Requires Windows SDK (for headers like windows.h, gdiplus.h).

中文:
- 使用 Visual Studio 2022，C++14。
- 已通过 pragma 链接 Gdiplus。
- 需安装 Windows SDK（提供 windows.h、gdiplus.h 等头文件）。

## Implementation Notes / 实现说明
English:
- GDI+ startup/shutdown encapsulated in ImageConverter.
- PNG encoding discovered via enumerating ImageCodecInfo.
- ICO directory entries written with correct offsets and PNG blobs.
- Center crop before scaling ensures consistent aspect ratio.

中文:
- GDI+ 启动与关闭逻辑封装在 ImageConverter 中。
- 通过枚举 ImageCodecInfo 获取 PNG 编码器。
- ICO 目录项正确写入偏移及 PNG 数据块。
- 先居中裁剪再缩放，保证比例一致。

## Limitations / 限制
English:
- Only square output; non-square source edges are discarded.
- No alpha premultiplication adjustments (relies on source PNG).
- Assumes modern Windows (supports PNG-compressed ICO entries).

中文:
- 仅生成正方形图标，非方形原图边缘会被裁剪。
- 未额外处理 Alpha 预乘，依赖原始 PNG。
- 假设运行于现代 Windows，支持 PNG 压缩 ICO。

## Potential Extensions / 可扩展方向
English:
- Option to reorder sizes (largest-first) for better preview behavior.
- Switch for BMP encoding of small sizes for legacy viewers.
- CLI flags for custom size sets or output directory.

中文:
- 可添加最大尺寸优先的写入顺序改善预览。
- 支持对小尺寸使用 BMP 编码提升旧环境兼容性。
- 增加命令行参数自定义尺寸与输出目录。

## License / 许可
English:
Provide your preferred open-source license (e.g., MIT) if distributing publicly.

中文:
若公开发布，请自行添加适用的开源许可（如 MIT）。
