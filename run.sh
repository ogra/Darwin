#!/bin/bash
# Darwin DAW - 実行用スクリプト

# ビルドディレクトリに実行ファイルがあるか確認
if [ ! -f "build/Darwin.exe" ]; then
    echo "Error: build/Darwin.exe not found."
    echo "Please run ./build.sh first."
    exit 1
fi

echo "Starting Darwin DAW..."
./build/Darwin.exe
