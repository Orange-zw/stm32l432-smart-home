#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
将当前目录下所有文本文件转换为UTF-8编码格式

功能：
- 递归遍历当前目录及所有子目录
- 识别文本文件（.c, .h, .ignore, .md, .txt等）
- 检测并跳过二进制文件
- 将文件内容转换为UTF-8编码并保存
"""

import os
import sys
from pathlib import Path

# 延迟导入chardet，在main函数中检查
try:
    import chardet
except ImportError:
    chardet = None


# 常见的文本文件扩展名列表
TEXT_FILE_EXTENSIONS = {
    # 源代码文件
    '.c', '.h', '.cpp', '.cc', '.cxx', '.hpp', '.hxx',
    # 脚本文件
    '.py', '.js', '.ts', '.sh', '.bat', '.cmd', '.ps1',
    # 配置文件
    '.json', '.xml', '.yaml', '.yml', '.toml', '.ini', '.cfg', '.conf',
    # 文档文件
    '.md', '.txt', '.rst', '.adoc',
    # 其他文本文件
    '.ignore', '.gitignore', '.gitattributes', '.editorconfig',
    '.cmake', '.make', '.mk', '.s', '.asm', '.sx', '.src',
    '.css', '.html', '.htm', '.scss', '.less',
    '.sql', '.lua', '.rb', '.go', '.rs', '.java', '.kt',
    '.properties', '.log', '.csv', '.tsv',
    '.uvprojx', '.uvoptx', '.ewp', '.eww', '.uvguix',
    '.clang-format', '.clang-tidy', '.clangd'
}

# 需要跳过的目录
SKIP_DIRS = {
    '.git', '.svn', '.hg', '__pycache__', '.vs', '.vscode',
    'node_modules', 'build', 'dist', 'bin', 'obj', 'OBJ',
    '.idea', '.settings', 'DebugConfig', 'Listings'
}

# 需要跳过的文件模式
SKIP_FILES = {
    '.exe', '.dll', '.so', '.dylib', '.a', '.lib', '.o', '.obj',
    '.bin', '.hex', '.elf', '.map', '.lst', '.bak'
}


def is_binary_file(file_path):
    """
    检测文件是否为二进制文件
    通过检查文件是否包含null字节来判断
    """
    try:
        with open(file_path, 'rb') as f:
            # 读取前1KB内容检查
            chunk = f.read(1024)
            if b'\x00' in chunk:
                return True
        return False
    except Exception:
        return True  # 读取失败，假设是二进制文件


def detect_encoding(file_path):
    """
    检测文件编码
    返回编码名称和置信度
    """
    if chardet is None:
        return 'utf-8', 0
    
    try:
        with open(file_path, 'rb') as f:
            raw_data = f.read()
            result = chardet.detect(raw_data)
            encoding = result.get('encoding', 'utf-8')
            confidence = result.get('confidence', 0)
            return encoding, confidence
    except Exception:
        return 'utf-8', 0


def convert_file_to_utf8(file_path):
    """
    将单个文件转换为UTF-8编码
    
    返回：(success, message)
    """
    try:
        # 检测当前编码
        detected_encoding, confidence = detect_encoding(file_path)
        
        # 尝试读取文件
        encodings_to_try = []
        if detected_encoding and confidence > 0.7:
            encodings_to_try.append(detected_encoding)
        # 添加常见编码
        encodings_to_try.extend(['utf-8', 'gbk', 'gb2312', 'gb18030', 
                                 'latin1', 'cp1252', 'iso-8859-1'])
        
        content = None
        used_encoding = None
        
        for enc in encodings_to_try:
            try:
                with open(file_path, 'r', encoding=enc, errors='replace') as f:
                    content = f.read()
                    used_encoding = enc
                    break
            except (UnicodeDecodeError, LookupError):
                continue
        
        if content is None:
            return False, f"无法确定文件编码"
        
        # 检查内容是否已经是UTF-8（简单检查）
        try:
            content.encode('utf-8')
        except UnicodeEncodeError:
            return False, f"内容包含无法编码为UTF-8的字符"
        
        # 如果编码已经是UTF-8，检查是否需要更新BOM
        # 读取原始字节检查BOM
        with open(file_path, 'rb') as f:
            first_bytes = f.read(3)
            has_utf8_bom = first_bytes == b'\xef\xbb\xbf'
        
        # 如果已经是UTF-8且没有BOM问题，可以跳过（可选）
        # 这里为了确保一致性，总是保存
        
        # 保存为UTF-8（无BOM）
        with open(file_path, 'w', encoding='utf-8', newline='', errors='replace') as f:
            f.write(content)
        
        if used_encoding and used_encoding.lower() not in ('utf-8', 'utf8'):
            return True, f"已转换: {used_encoding} -> UTF-8"
        else:
            return True, f"已确认: UTF-8"
            
    except Exception as e:
        return False, f"错误: {str(e)}"


def should_process_file(file_path):
    """
    判断是否应该处理该文件
    """
    path_obj = Path(file_path)
    file_name = path_obj.name.lower()
    ext = path_obj.suffix.lower()
    
    # 检查文件扩展名（跳过二进制文件扩展名）
    if ext in SKIP_FILES:
        return False
    
    # 检查是否为以点开头的特殊文件（如.gitignore, .ignore等）
    if file_name.startswith('.'):
        # 检查是否在文本文件列表中（去除开头的点）
        if file_name in ['.gitignore', '.gitattributes', '.editorconfig', '.ignore', '.clang-format', '.clang-tidy', '.clangd']:
            # 这些文件需要进一步检查是否为二进制
            if is_binary_file(file_path):
                return False
            return True
    
    # 如果有扩展名，检查是否在文本文件列表中
    if ext and ext in TEXT_FILE_EXTENSIONS:
        # 检查是否为二进制文件
        if is_binary_file(file_path):
            return False
        return True
    
    # 对于没有扩展名或未知扩展名的文件，跳过
    return False


def process_directory(root_dir='.'):
    """
    递归处理目录中的所有文件
    """
    root_path = Path(root_dir).resolve()
    processed_count = 0
    skipped_count = 0
    error_count = 0
    
    print(f"开始处理目录: {root_path}")
    print(f"文本文件扩展名: {', '.join(sorted(TEXT_FILE_EXTENSIONS))}")
    print("-" * 80)
    
    # 遍历所有文件
    for file_path in root_path.rglob('*'):
        # 跳过目录
        if not file_path.is_file():
            continue
        
        # 跳过指定目录中的文件
        if any(skip_dir in file_path.parts for skip_dir in SKIP_DIRS):
            continue
        
        # 检查是否应该处理
        if not should_process_file(file_path):
            skipped_count += 1
            continue
        
        # 获取相对路径用于显示
        try:
            rel_path = file_path.relative_to(root_path)
        except ValueError:
            rel_path = file_path
        
        # 转换文件
        success, message = convert_file_to_utf8(file_path)
        
        if success:
            processed_count += 1
            print(f"[✓] {rel_path}: {message}")
        else:
            error_count += 1
            print(f"[✗] {rel_path}: {message}")
    
    print("-" * 80)
    print(f"处理完成!")
    print(f"  成功处理: {processed_count} 个文件")
    print(f"  跳过文件: {skipped_count} 个文件")
    print(f"  错误文件: {error_count} 个文件")
    print(f"  总计文件: {processed_count + skipped_count + error_count} 个文件")


def main():
    """
    主函数
    """
    # 检查chardet库
    if chardet is None:
        print("错误: 需要安装 chardet 库")
        print("请运行: pip install chardet")
        sys.exit(1)
    
    # 获取工作目录
    work_dir = os.getcwd()
    
    # 确认操作
    print("=" * 80)
    print("文件编码转换工具 - 转换为UTF-8")
    print("=" * 80)
    print(f"工作目录: {work_dir}")
    print()
    print("警告: 此操作将直接修改文件，请确保已备份重要文件!")
    print()
    
    response = input("是否继续? (y/n): ").strip().lower()
    if response not in ('y', 'yes', '是'):
        print("操作已取消")
        return
    
    print()
    process_directory(work_dir)


if __name__ == '__main__':
    main()

