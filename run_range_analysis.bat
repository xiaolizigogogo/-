@echo off
echo 编译Java价格范围分析程序...
javac SimpleHousePriceRangeAnalysis.java

if %errorlevel% equ 0 (
    echo 编译成功！运行程序...
    java SimpleHousePriceRangeAnalysis
) else (
    echo 编译失败！
    pause
) 