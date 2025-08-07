@echo off
echo 编译Java程序...
javac HousePriceAnalysis.java

if %errorlevel% equ 0 (
    echo 编译成功！运行程序...
    java HousePriceAnalysis
) else (
    echo 编译失败！
    pause
) 