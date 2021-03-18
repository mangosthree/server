@echo off
:main
if not exist ..\src\modules\Eluna\.git goto eluna:
if not exist ..\src\tools\Extractor_projects\.git goto extractors:
goto endpoint:

:eluna
copy Patch_Easybuild_Mangos3.cmd ..\src\modules\Eluna\.git
goto main:

:extractors
mkdir ..\src\tools\Extractor_projects
copy Patch_Easybuild_Mangos3.cmd ..\src\tools\Extractor_projects\.git
goto main:

:endpoint