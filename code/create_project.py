#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
项目创建脚本
从 Template 文件夹创建新项目
"""

import os
import sys
import shutil
import subprocess
import argparse
import glob
import stat
from datetime import datetime
from pathlib import Path


def copy_template_to_project(template_dir, project_name, workspace_path):
    """复制 Template 文件夹到 project 文件夹下并重命名"""
    now = datetime.now()
    monthly_dir = Path(workspace_path) / "project" / f"{now.year}.{now.month}"
    monthly_dir.mkdir(parents=True, exist_ok=True)
    project_dir = monthly_dir / project_name
    
    if not template_dir.exists():
        print(f"错误: Template 文件夹不存在: {template_dir}")
        sys.exit(1)
    
    if project_dir.exists():
        print(f"错误: 项目文件夹已存在: {project_dir}")
        sys.exit(1)
    
    print(f"正在复制 Template 到 {project_dir}...")
    shutil.copytree(template_dir, project_dir)
    print(f"复制完成!")
    
    return project_dir


def remove_readonly(func, path, exc_info):
    """移除只读属性并重试删除"""
    try:
        os.chmod(path, stat.S_IWRITE)
        func(path)
    except Exception as e:
        # 如果还是失败，尝试使用更强制的方式
        try:
            if os.path.isdir(path):
                # 对于文件夹，递归修改所有文件的权限
                for root, dirs, files in os.walk(path):
                    for d in dirs:
                        os.chmod(os.path.join(root, d), stat.S_IWRITE)
                    for f in files:
                        os.chmod(os.path.join(root, f), stat.S_IWRITE)
                func(path)
            else:
                os.chmod(path, stat.S_IWRITE)
                func(path)
        except Exception:
            pass


def force_delete(path):
    """强制删除文件或文件夹，处理权限问题"""
    path = Path(path)
    if not path.exists():
        return False
    
    try:
        if path.is_file():
            # 移除只读属性
            os.chmod(path, stat.S_IWRITE)
            path.unlink()
            return True
        elif path.is_dir():
            # 使用自定义的错误处理函数
            shutil.rmtree(path, onerror=remove_readonly)
            return True
    except PermissionError as e:
        # 如果是权限错误，尝试更强制的方式
        try:
            if path.is_dir():
                # 递归修改所有文件的权限
                for root, dirs, files in os.walk(path):
                    for d in dirs:
                        dir_path = os.path.join(root, d)
                        try:
                            os.chmod(dir_path, stat.S_IWRITE | stat.S_IREAD | stat.S_IEXEC)
                        except:
                            pass
                    for f in files:
                        file_path = os.path.join(root, f)
                        try:
                            os.chmod(file_path, stat.S_IWRITE | stat.S_IREAD)
                        except:
                            pass
                # 再次尝试删除
                shutil.rmtree(path, onerror=remove_readonly)
            else:
                os.chmod(path, stat.S_IWRITE)
                path.unlink()
            return True
        except Exception as e2:
            raise e2
    except Exception as e:
        raise e
    
    return False


def delete_files_and_folders(project_dir, items_to_delete):
    """删除指定的文件或文件夹，支持通配符"""
    if not items_to_delete:
        return
    
    print("\n正在删除指定的文件和文件夹...")
    for item_path in items_to_delete:
        # 检查是否包含通配符
        if '*' in item_path or '?' in item_path:
            # 使用 glob 匹配通配符
            pattern = str(project_dir / item_path)
            matches = glob.glob(pattern, recursive=True)
            
            if not matches:
                print(f"  跳过 (无匹配): {item_path}")
                continue
            
            for match_path in matches:
                full_path = Path(match_path)
                if full_path.exists():
                    try:
                        force_delete(full_path)
                        print(f"  已删除: {full_path.relative_to(project_dir)}")
                    except Exception as e:
                        print(f"  警告: 删除 {full_path.relative_to(project_dir)} 时出错: {e}")
        else:
            # 不使用通配符，直接删除
            full_path = project_dir / item_path
            if full_path.exists():
                try:
                    force_delete(full_path)
                    print(f"  已删除: {item_path}")
                except Exception as e:
                    print(f"  警告: 删除 {item_path} 时出错: {e}")
            else:
                print(f"  跳过 (不存在): {item_path}")
    print("删除完成!\n")


def execute_keil2clangd(project_dir):
    """执行 keil2clangd.exe"""
    exe_path = project_dir / "code" / "USER" / "keil2clangd.exe"
    
    if not exe_path.exists():
        print(f"警告: keil2clangd.exe 不存在: {exe_path}")
        print("跳过执行 keil2clangd.exe")
        return
    
    print(f"正在执行 {exe_path}...")
    try:
        result = subprocess.run(
            [str(exe_path)],
            cwd=str(exe_path.parent),
            capture_output=True,
            text=True
        )
        if result.returncode == 0:
            print("keil2clangd.exe 执行成功!")
        else:
            print(f"keil2clangd.exe 执行完成，返回码: {result.returncode}")
            if result.stderr:
                print(f"错误输出: {result.stderr}")
    except Exception as e:
        print(f"执行 keil2clangd.exe 时出错: {e}")


def open_with_cursor(path):
    """使用 cursor 打开文件或文件夹"""
    abs_path = Path(path).resolve()
    
    try:
        print(f"正在使用 cursor 打开: {abs_path}")
        subprocess.Popen(
            ["cursor", str(abs_path)], 
            shell=True,
            creationflags=subprocess.CREATE_NEW_CONSOLE
        )
        return True  # 成功打开
    except Exception as e:
        print(f"使用 cursor 打开 {path} 时出错: {e}")
        return False  # 打开失败


def open_with_keil(uvprojx_path):
    """使用 Keil 打开 .uvprojx 文件"""
    abs_path = Path(uvprojx_path).resolve()
    
    if not abs_path.exists():
        print(f"错误: 文件不存在: {abs_path}")
        return False
    
    try:
        # 使用 Windows 默认关联程序打开
        print(f"未找到 Keil，尝试使用默认程序打开: {abs_path}")
        if sys.platform == "win32":
            os.startfile(str(abs_path))
        else:
            subprocess.Popen(["xdg-open", str(abs_path)])
        return True
    except Exception as e:
        print(f"使用 Keil 打开 {abs_path} 时出错: {e}")
        return False


def main():
    parser = argparse.ArgumentParser(description="从 Template 创建新项目")
    parser.add_argument("project_name", help="项目名称")
    
    args = parser.parse_args()
    
    # 获取脚本所在目录（Template/code 文件夹）
    script_dir = Path(__file__).parent.resolve()
    # 获取 Template 文件夹（脚本的父目录的父目录）
    template_dir = script_dir.parent.resolve()
    # 获取工作空间路径（Template 的父目录）
    workspace_path = template_dir.parent.resolve()
    
    # 复制 Template 到 project 文件夹
    project_dir = copy_template_to_project(template_dir, args.project_name, workspace_path)
    
    # 定义需要删除的文件和文件夹列表（路径相对于项目根目录）
    items_to_delete = [
        "code/.git",
        "app/*",
        "jx_firm/*",
        "原理图PCB/*",
        "code/*.md"
    ]
    
    # 删除指定的文件和文件夹
    delete_files_and_folders(project_dir, items_to_delete)
    
    # 执行 keil2clangd.exe
    execute_keil2clangd(project_dir)
    
    # 使用 cursor 打开 code 文件夹
    code_dir = project_dir / "code"
    print(f"\n正在使用 cursor 打开文件夹: {code_dir}")
    open_with_cursor(code_dir)
    
    # 等待一下，避免同时打开太多窗口
    import time
    time.sleep(1)
    
    # 使用 Keil 打开 LCD.uvprojx 文件
    uvprojx_file = project_dir / "code" / "USER" / "LCD.uvprojx"
    print(f"正在使用 Keil 打开文件: {uvprojx_file}")
    open_with_keil(uvprojx_file)
    
    print(f"\n项目创建完成! 项目路径: {project_dir}")


if __name__ == "__main__":
    main()
