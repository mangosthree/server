@echo off
:main
if not exist ..\src\modules\Eluna\.git goto eluna:
if not exist ..\src\tools\Extractor_projects\.git goto extractors:
if not exist ..\dep\.git goto dep:
if not exist ..\src\modules\sd3\.git goto sd3:
if not exist ..\src\realmd\.git goto realm:
goto endpoint:

:eluna
echo Patching Eluna
copy Patch_Easybuild_Mangos3.cmd ..\src\modules\Eluna\.git
goto main:

:extractors
echo Patching Extractors
copy Patch_Easybuild_Mangos3.cmd ..\src\tools\Extractor_projects\.git
goto main:

:dep
echo Patching Dep
copy Patch_Easybuild_Mangos3.cmd ..\dep\.git
goto main:

:sd3
echo Patching SD3
copy Patch_Easybuild_Mangos3.cmd ..\src\modules\sd3\.git
goto main:

:realm
echo Patching Realm
copy Patch_Easybuild_Mangos3.cmd ..\src\realmd\.git
goto main:

:endpoint