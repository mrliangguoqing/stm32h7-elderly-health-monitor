@echo off
:: 删除 MDK-ARM(µVision5) 项目中的中间文件、日志文件、调试文件等

echo Cleaning project files...

:: 删除编译产生的文件
del /q /s *.o
del /q /s *.axf
del /q /s *.lst
del /q /s *.map
del /q /s *.dep
del /q /s *.d
del /q /s *.obj
del /q /s *.omf
del /q /s *.tra
del /q /s *.plg
del /q /s *.rpt
del /q /s *.tmp
del /q /s *.bak
del /q /s *.__i
del /q /s *.htm
del /q /s *.sct

:: 删除编译日志和编译结果文件
del /q /s *.lnp
del /q /s *.crf

:: 删除 JLink 相关文件
del /q /s JLink*

:: 删除其他调试相关文件
del /q /s *.ddk
del /q /s *.edk
del /q /s *.iex
del /q /s *.mpf
del /q /s *.scvd

:: 删除用户界面状态（如项目树展开/折叠状态、最近打开的文件）
del /q /s *.uvguix.*

:: 排除 JLink 设置文件，避免误删除
:: del /q /s *.opt

echo All project-related intermediate files have been deleted.

:: 暂停程序并显示“请按任意键继续...”
:: pause

exit