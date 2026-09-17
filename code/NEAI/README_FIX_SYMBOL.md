# 修复 NanoEdgeAI 库符号冲突问题

## 问题描述

当同时使用多个 NanoEdgeAI 库（如 `libneai_tempad.a`、`libneai_humiad.a`、`libneai_lightad.a`）时，会出现链接错误：

```
Error: L6200E: Symbol _fminf multiply defined
```

这是因为多个库都定义了 `_fminf` 符号，导致链接冲突。

## 解决方案

根据 ST 社区论坛的解决方案（[参考链接](https://community.st.com/t5/forums/forumtopicprintpage/board-id/edge-ai/message-id/4509/print-single-message/true/page/1)），需要重命名其中一个库的 `_fminf` 符号。

## 修复步骤

### 方法1: 使用提供的脚本（推荐）

#### Windows 用户:
```cmd
cd libneai
fix_symbol_conflict.bat
```

#### Linux/Mac 用户:
```bash
cd libneai
chmod +x fix_symbol_conflict.sh
./fix_symbol_conflict.sh
```

### 方法2: 手动修复

1. **提取库文件中的对象文件**
   ```bash
   cd libneai_humi-h_3
   arm-none-eabi-ar x libneai_humiad.a
   ```

2. **检查符号**
   ```bash
   nm *.o | grep fminf
   ```

3. **重命名符号**
   ```bash
   arm-none-eabi-objcopy --redefine-sym _fminf=_fminf_humiad *.o
   ```

4. **重新打包库**
   ```bash
   rm libneai_humiad.a
   arm-none-eabi-ar rcs libneai_humiad_fixed.a *.o
   rm *.o
   ```

## 在 Keil 中使用修复后的库

1. 打开 Keil 项目
2. 在项目树中找到 `NEAI` 组
3. 移除 `libneai_humiad.a`
4. 添加 `libneai_humiad_fixed.a`（位于 `libneai/libneai_humi-h_3/` 目录）
5. 重新编译项目

## 注意事项

- 每次从 NanoEdgeAI Studio 生成新库时，都需要重新执行此修复步骤
- 建议保留一个库（如 tempad）不变，只修复其他库
- 如果使用三个或更多库，需要为每个库使用不同的符号名称

## 工具要求

需要安装以下工具之一：
- ARM GCC 工具链（包含 `ar`、`objcopy`、`nm`）
- MinGW/MSYS2（Windows）
- 系统自带的 binutils（Linux/Mac）

## 参考

- [ST 社区论坛解决方案](https://community.st.com/t5/forums/forumtopicprintpage/board-id/edge-ai/message-id/4509/print-single-message/true/page/1)
