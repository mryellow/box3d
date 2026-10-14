@echo off
set "PATH=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%PATH%"
for %%S in (main master) do (
  cd /d E:\src\box3d-bench\%%S
  cmake -S . -B build -DBOX3D_BENCHMARKS=ON -DBOX3D_SAMPLES=OFF -DBOX3D_UNIT_TESTS=OFF -DBOX3D_DOCS=OFF > E:\src\box3d-bench\configure-%%S.log 2>&1 || (echo CONFIGURE FAILED %%S > E:\src\box3d-bench\build-done.txt & exit /b 1)
  cmake --build build --config Release --target benchmark > E:\src\box3d-bench\build-%%S.log 2>&1 || (echo BUILD FAILED %%S > E:\src\box3d-bench\build-done.txt & exit /b 1)
)
echo OK > E:\src\box3d-bench\build-done.txt
