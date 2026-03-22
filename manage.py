#!/usr/bin/env python3
import argparse
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.abspath(__file__))
CLIENT_DIR = os.path.join(ROOT, 'client', 'USV_GS_Test')

def run(cmd, cwd=None):
    print('>',' '.join(cmd))
    result = subprocess.run(cmd, cwd=cwd)
    sys.exit(result.returncode)

def run_client():
    if not os.path.isdir(CLIENT_DIR):
        print('client/USV_GS_Test 不存在，先检查路径')
        sys.exit(1)
    # 简单启动提示；具体构建命令按项目需要调整
    print('请使用 Qt Creator 打开: {}'.format(CLIENT_DIR))

def run_backend():
    print('后端占位，尚未实现')

def run_simulator():
    print('模拟器占位，尚未实现')

def run_visualization():
    print('可视化占位，尚未实现')

def main():
    parser = argparse.ArgumentParser(description='USV 项目管理脚本')
    sub = parser.add_subparsers(dest='cmd')
    sub.add_parser('run-client')
    sub.add_parser('run-backend')
    sub.add_parser('run-simulator')
    sub.add_parser('run-visualization')
    args = parser.parse_args()
    if args.cmd == 'run-client':
        run_client()
    elif args.cmd == 'run-backend':
        run_backend()
    elif args.cmd == 'run-simulator':
        run_simulator()
    elif args.cmd == 'run-visualization':
        run_visualization()
    else:
        parser.print_help()

if __name__ == '__main__':
    main()
