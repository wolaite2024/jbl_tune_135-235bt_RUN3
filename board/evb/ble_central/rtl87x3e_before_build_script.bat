@echo off
set flash_bank_path=%1
set feature=%2

if %feature% == ANC (
     copy ..\..\..\bin\rtl87x3e\flash_map_config\%flash_bank_path%\flash_%flash_bank_path%_ANC\flash_map.h ..\..\..\bin\rtl87x3e\flash_map_config\flash_map.h
) else (
     copy ..\..\..\bin\rtl87x3e\flash_map_config\%flash_bank_path%\flash_%flash_bank_path%\flash_map.h ..\..\..\bin\rtl87x3e\flash_map_config\flash_map.h
)

 copy ..\..\..\bin\rtl87x3e\upperstack_stamp\%flash_bank_path%\upperstack_compile_stamp.h ..\..\..\bin\rtl87x3e\upperstack_stamp\upperstack_compile_stamp.h


