@echo off
set flash_bank_path=%1
set feature=%2
set ic_type=%3
set target=%4

if %feature% == ANC (
	 copy ..\..\..\bin\%ic_type%\flash_map_config\%flash_bank_path%\flash_%flash_bank_path%_ANC\flash_map.h ..\..\..\bin\%ic_type%\flash_map_config\flash_map.h
	 copy ..\..\..\bin\%ic_type%\hal_lib\hal_utils_anc.lib ..\..\..\bin\%ic_type%\hal_utils.lib
) else (
	 copy ..\..\..\bin\%ic_type%\flash_map_config\%flash_bank_path%\flash_%flash_bank_path%\flash_map.h ..\..\..\bin\%ic_type%\flash_map_config\flash_map.h
	 copy ..\..\..\bin\%ic_type%\hal_lib\hal_utils.lib ..\..\..\bin\%ic_type%\hal_utils.lib
)

copy ..\..\..\bin\%ic_type%\upperstack_stamp\%flash_bank_path%\upperstack_compile_stamp.h ..\..\..\bin\%ic_type%\upperstack_stamp\upperstack_compile_stamp.h

if "%target%" == "" (
	echo %target%
) else (
	copy ..\..\..\bin\%ic_type%\framework_lib\framework_%target%.lib ..\..\..\bin\%ic_type%\framework.lib
)


