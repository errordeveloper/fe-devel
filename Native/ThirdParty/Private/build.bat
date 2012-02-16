@echo off

set TOP=%~dp0
set TOP=%TOP:~0,-1%

@rem Package names
set ZLIB_NAME=zlib-1.2.5
set PNG_NAME=libpng-1.4.4
set ILMBASE_NAME=ilmbase-1.0.2
set OPENEXR_NAME=openexr-1.7.0
set V8_NAME=v8-1.3.18.22
set LLVM_NAME=llvm-2.9
set FFMPEG_NAME=ffmpeg-0.6.3
set BOOST_NAME=boost_1_47_0
set HDF5_NAME=hdf5-1.8.7
set ALEMBIC_NAME=alembic-1.0_2011080800
set TEEM_NAME=teem-1.10.0
set TIFF_NAME=tiff-3.9.5
set JPEG_SOURCE=jpegsrc.v8d
set JPEG_NAME=jpeg-8d
set OPENCV_NAME=OpenCV-2.3.1
set OPENCV_SOURCE=OpenCV-2.3.1a
set OPENCV_VER=231

if "%1" NEQ "" goto %1

set PATH=%PATH%;%TOP%\bin\Windows

cmd /C build.bat x86
rem cmd /C build.bat x64

goto :eof


:x86
set ARCH=x86
set VSARCH=Win32
set PROCESSOR_ARCHITECTURE=x86
@call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" x86
goto start_build

:x64
set ARCH=x64
set VSARCH=x64
echo %PROCESSOR_ARCHITECTURE%
@call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" amd64
goto start_build

:start_build

set BUILD_DIR=%TOP%\Build\%ARCH%
set CHECKPOINT_DIR=%BUILD_DIR%\cp
set LIB_ARCH_DEBUG_DIR=%TOP%\Windows\%ARCH%\Debug\lib
set LIB_ARCH_RELEASE_DIR=%TOP%\Windows\%ARCH%\Release\lib
set INCLUDE_ARCH_DEBUG_DIR=%TOP%\Windows\%ARCH%\Debug\include
set INCLUDE_ARCH_RELEASE_DIR=%TOP%\Windows\%ARCH%\Release\include

if NOT EXIST %BUILD_DIR% mkdir %BUILD_DIR%
if NOT EXIST %CHECKPOINT_DIR% mkdir %CHECKPOINT_DIR%
if NOT EXIST %LIB_ARCH_DEBUG_DIR% mkdir %LIB_ARCH_DEBUG_DIR%
if NOT EXIST %LIB_ARCH_RELEASE_DIR% mkdir %LIB_ARCH_RELEASE_DIR%

rem ============= ZLIB ==============
if EXIST %CHECKPOINT_DIR%\zlib_unpack goto zlib_unpack_done
echo zlib - Unpacking
cd %BUILD_DIR%
type "%TOP%\SourcePackages\%ZLIB_NAME%.tar.bz2" | bzip2 -d -c | tar -x -f- 2> NUL:
type "%TOP%\Patches\Windows\%ZLIB_NAME%-patch.tar.bz2" | bzip2 -d -c | tar -x -f- 2> NUL:
touch %CHECKPOINT_DIR%\zlib_unpack
:zlib_unpack_done

if EXIST %CHECKPOINT_DIR%\zlib_compile_asm goto zlib_compile_asm_done
echo zlib - Compiling assembly code
cd %BUILD_DIR%\%ZLIB_NAME%\contrib\masm%ARCH%
if "%ARCH%" == "x86" (
  call bld_ml32.bat > NUL: 2>&1
)
if "%ARCH%" == "x64" (
  call bld_ml64.bat > NUL: 2>&1
)
touch %CHECKPOINT_DIR%\zlib_compile_asm
:zlib_compile_asm_done


if EXIST %CHECKPOINT_DIR%\zlib_compile_debug goto zlib_compile_debug_done
echo zlib - Compiling debug
cd %BUILD_DIR%\%ZLIB_NAME%\contrib\vstudio\vc10
devenv zlibvc.sln /build "Debug|%VSARCH%" /project "zlibstat"
touch %CHECKPOINT_DIR%\zlib_compile_debug
:zlib_compile_debug_done

if EXIST %CHECKPOINT_DIR%\zlib_compile_release goto zlib_compile_release_done
echo zlib - Compiling release
cd %BUILD_DIR%\%ZLIB_NAME%\contrib\vstudio\vc10
devenv zlibvc.sln /build "Release|%VSARCH%" /project "zlibstat"
touch %CHECKPOINT_DIR%\zlib_compile_release
:zlib_compile_release_done

if EXIST %CHECKPOINT_DIR%\zlib_install goto zlib_install_done
echo zlib - Installing
cd %BUILD_DIR%\%ZLIB_NAME%\contrib\vstudio\vc10
echo F | xcopy %ARCH%\ZlibStatDebug\zlibstat.lib %LIB_ARCH_DEBUG_DIR%\zlib\zlib.lib /Y > NUL:
echo F | xcopy %ARCH%\ZlibStatRelease\zlibstat.lib %LIB_ARCH_RELEASE_DIR%\zlib\zlib.lib /Y > NUL:
cd %BUILD_DIR%\%ZLIB_NAME%
echo D | xcopy zconf.h %TOP%\include\zlib /Y > NUL:
echo D | xcopy zlib.h %TOP%\include\zlib /Y > NUL:
touch %CHECKPOINT_DIR%\zlib_install
:zlib_install_done

rem ============= PNG ===============
if EXIST %CHECKPOINT_DIR%\png_unpack goto png_unpack_done
echo PNG - Unpacking
cd %BUILD_DIR%
type "%TOP%\SourcePackages\%PNG_NAME%.tar.bz2" | bzip2 -d -c | tar -x -f- 2> NUL:
type "%TOP%\Patches\Windows\%PNG_NAME%-patch.tar.bz2" | bzip2 -d -c | tar -x -f- 2> NUL:
touch %CHECKPOINT_DIR%\png_unpack
:png_unpack_done

if EXIST %CHECKPOINT_DIR%\png_compile_debug goto png_compile_debug_done
echo PNG - Compiling debug
cd %BUILD_DIR%\%PNG_NAME%\projects\vstudio
if exist "Debug Library" rmdir /s/q "Debug Library"
if exist "libpng\Debug Library" rmdir /s/q "libpng\Debug Library"
devenv vstudio.sln /build "Debug Library|%VSARCH%" /project "zlib"
devenv vstudio.sln /build "Debug Library|%VSARCH%" /project "libpng"
touch %CHECKPOINT_DIR%\png_compile_debug
:png_compile_debug_done

if EXIST %CHECKPOINT_DIR%\png_compile_release goto png_compile_release_done
echo PNG - Compiling release
cd %BUILD_DIR%\%PNG_NAME%\projects\vstudio
if exist "Release Library" rmdir /s/q "Release Library"
if exist "libpng\Release Library" rmdir /s/q "libpng\Release Library"
devenv vstudio.sln /build "Release Library|%VSARCH%" /project "zlib"
devenv vstudio.sln /build "Release Library|%VSARCH%" /project "libpng"
touch %CHECKPOINT_DIR%\png_compile_release
:png_compile_release_done

if EXIST %CHECKPOINT_DIR%\png_install goto png_install_done
echo PNG - Installing
cd %BUILD_DIR%\%PNG_NAME%\projects\vstudio
echo D | xcopy "Debug Library\zlib.lib" %LIB_ARCH_DEBUG_DIR%\libpng\ /Y > NUL:
echo D | xcopy "Release Library\zlib.lib" %LIB_ARCH_RELEASE_DIR%\libpng\ /Y > NUL:
echo D | xcopy "Debug Library\libpng14.lib" %LIB_ARCH_DEBUG_DIR%\libpng\ /Y > NUL:
echo D | xcopy "Release Library\libpng14.lib" %LIB_ARCH_RELEASE_DIR%\libpng\ /Y > NUL:
cd %BUILD_DIR%\%PNG_NAME%
echo D | xcopy png.h %TOP%\include\libpng\libpng /Y > NUL:
echo D | xcopy pngconf.h %TOP%\include\libpng\libpng /Y > NUL:
touch %CHECKPOINT_DIR%\png_install
:png_install_done

rem =========== ILMBASE =============
if EXIST %CHECKPOINT_DIR%\ilmbase_unpack goto ilmbase_unpack_done
echo ILMBase - Unpacking
cd %BUILD_DIR%
type %TOP%\SourcePackages\%ILMBASE_NAME%.tar.bz2 | bzip2 -d -c | tar -x -f- 2> NUL:
type %TOP%\Patches\Windows\%ILMBASE_NAME%-patch.tar.bz2 | bzip2 -d -c | tar -x -f- 2> NUL:
touch %CHECKPOINT_DIR%\ilmbase_unpack
:ilmbase_unpack_done

if EXIST %CHECKPOINT_DIR%\ilmbase_compile_debug goto ilmbase_compile_debug_done
echo ILMBase - Compiling debug
cd %BUILD_DIR%\%ILMBASE_NAME%\vc\vc10\IlmBase
devenv IlmBase.sln /build "Debug|%VSARCH%"
touch %CHECKPOINT_DIR%\ilmbase_compile_debug
:ilmbase_compile_debug_done

if EXIST %CHECKPOINT_DIR%\ilmbase_compile_release goto ilmbase_compile_release_done
echo ILMBase - Compiling release
cd %BUILD_DIR%\%ILMBASE_NAME%\vc\vc10\IlmBase
devenv IlmBase.sln /build "Release|%VSARCH%"
touch %CHECKPOINT_DIR%\ilmbase_compile_release
:ilmbase_compile_release_done

if EXIST %CHECKPOINT_DIR%\ilmbase_install goto ilmbase_install_done
echo ILMBase - Installing
cd %BUILD_DIR%\%ILMBASE_NAME%\vc\vc10\IlmBase
robocopy %VSARCH%\Debug %LIB_ARCH_DEBUG_DIR%\ilmbase *.lib /S /MIR > NUL:
robocopy %VSARCH%\Release %LIB_ARCH_RELEASE_DIR%\ilmbase *.lib /S /MIR > NUL:

cd %BUILD_DIR%\%ILMBASE_NAME%
robocopy config.windows %TOP%\include\Windows\%ARCH%\ilmbase *.h /s > NUL:

robocopy Half %TOP%\include\ilmbase *.h /S > NUL:
robocopy Iex %TOP%\include\ilmbase *.h /S > NUL:
robocopy IlmThread %TOP%\include\ilmbase *.h /S > NUL:
robocopy Imath %TOP%\include\ilmbase *.h /S > NUL:
touch %CHECKPOINT_DIR%\ilmbase_install
:ilmbase_install_done


rem =========== OPENEXR =============
if EXIST %CHECKPOINT_DIR%\openexr_unpack goto openexr_unpack_done
echo OpenEXR - Unpacking
cd %BUILD_DIR%
type %TOP%\SourcePackages\%OPENEXR_NAME%.tar.bz2 | bzip2 -d -c | tar -x -f- 2> NUL:
type %TOP%\Patches\Windows\%OPENEXR_NAME%-patch.tar.bz2 | bzip2 -d -c | tar -x -f- 2> NUL:
touch %CHECKPOINT_DIR%\openexr_unpack
:openexr_unpack_done

if EXIST %CHECKPOINT_DIR%\openexr_compile_debug goto openexr_compile_debug_done
echo OpenEXR - Compiling debug
cd %BUILD_DIR%\%OPENEXR_NAME%\vc\vc10\OpenEXR
devenv OpenEXR.sln /build "Debug|%VSARCH%"
touch %CHECKPOINT_DIR%\openexr_compile_debug
:openexr_compile_debug_done

if EXIST %CHECKPOINT_DIR%\openexr_compile_release goto openexr_compile_release_done
echo OpenEXR - Compiling release
cd %BUILD_DIR%\%OPENEXR_NAME%\vc\vc10\OpenEXR
devenv OpenEXR.sln /build "Release|%VSARCH%"
touch %CHECKPOINT_DIR%\openexr_compile_release
:openexr_compile_release_done

if EXIST %CHECKPOINT_DIR%\openexr_install goto openexr_install_done
echo OpenEXR - Installing
cd %BUILD_DIR%\%OPENEXR_NAME%\vc\vc10\OpenEXR
robocopy %VSARCH%\Debug %LIB_ARCH_DEBUG_DIR%\openexr *.lib /S /MIR > NUL:
robocopy %VSARCH%\Release %LIB_ARCH_RELEASE_DIR%\openexr *.lib /S /MIR > NUL:

cd %BUILD_DIR%\%OPENEXR_NAME%
robocopy config.windows %TOP%\include\Windows\%ARCH%\openexr *.h /s > NUL:

robocopy IlmImf %TOP%\include\openexr *.h /S > NUL:
touch %CHECKPOINT_DIR%\openexr_install
:openexr_install_done


rem ============== V8 ===============
if "%ARCH%" == "x86" (
  set V8ARCH=ia32
)
if "%ARCH%" == "x64" (
  set V8ARCH=x64
)

if EXIST %CHECKPOINT_DIR%\v8_unpack goto v8_unpack_done
echo V8 - Unpacking
cd %BUILD_DIR%
type %TOP%\SourcePackages\%V8_NAME%.tar.bz2 | bzip2 -d -c | tar -x -f- 2> NUL:
type %TOP%\Patches\Windows\%V8_NAME%-patch.tar.bz2 | bzip2 -d -c | tar -x -f- 2> NUL:
touch %CHECKPOINT_DIR%\v8_unpack
:v8_unpack_done

if EXIST %CHECKPOINT_DIR%\v8_compile_debug goto v8_compile_debug_done
echo V8 - Compiling debug
cd %BUILD_DIR%\%V8_NAME%
call scons mode=debug msvcrt=static arch=%V8ARCH%
touch %CHECKPOINT_DIR%\v8_compile_debug
:v8_compile_debug_done

if EXIST %CHECKPOINT_DIR%\v8_compile_release goto v8_compile_release_done
echo V8 - Compiling release
cd %BUILD_DIR%\%V8_NAME%
call scons mode=release msvcrt=static arch=%V8ARCH%
touch %CHECKPOINT_DIR%\v8_compile_release
:v8_compile_release_done

if EXIST %CHECKPOINT_DIR%\v8_install goto v8_install_done
echo V8 - Installing
cd %BUILD_DIR%\%V8_NAME%
echo F | xcopy v8_g.lib %LIB_ARCH_DEBUG_DIR%\v8\v8.lib /Y > NUL:
echo F | xcopy vc100.pdb %LIB_ARCH_DEBUG_DIR%\v8\vc100.pdb /Y > NUL:
echo F | xcopy v8.lib %LIB_ARCH_RELEASE_DIR%\v8\v8.lib /Y > NUL:

robocopy include %TOP%\include\v8\v8 *.h /S /MIR > NUL:

touch %CHECKPOINT_DIR%\v8_install
:v8_install_done


rem ============= LLVM ==============

if EXIST %CHECKPOINT_DIR%\llvm_unpack goto llvm_unpack_done
echo LLVM - Unpacking
cd %BUILD_DIR%
type "%TOP%\SourcePackages\%LLVM_NAME%.tar.bz2" | bzip2 -d -c | tar -x -f-
type %TOP%\Patches\Windows\%LLVM_NAME%-patch.tar.bz2 | bzip2 -d -c | tar -x -f- 2> NUL:
touch %CHECKPOINT_DIR%\llvm_unpack
:llvm_unpack_done

if EXIST %CHECKPOINT_DIR%\llvm_cmake goto llvm_cmake_done
echo LLVM - Creating build files
if NOT EXIST %BUILD_DIR%\llvm-build mkdir %BUILD_DIR%\llvm-build
cd %BUILD_DIR%\llvm-build
cmake -G "Visual Studio 10" ..\%LLVM_NAME%
touch %CHECKPOINT_DIR%\llvm_cmake
:llvm_cmake_done

if EXIST %CHECKPOINT_DIR%\llvm_build_debug goto llvm_build_debug_done
echo LLVM - Compiling debug
cd %BUILD_DIR%\llvm-build
devenv LLVM.sln /build "Debug|%VSARCH%"
touch %CHECKPOINT_DIR%\llvm_build_debug
:llvm_build_debug_done

if EXIST %CHECKPOINT_DIR%\llvm_build_release goto llvm_build_release_done
echo LLVM - Compiling release
cd %BUILD_DIR%\llvm-build
devenv LLVM.sln /build "Release|%VSARCH%"
touch %CHECKPOINT_DIR%\llvm_build_release
:llvm_build_release_done

if EXIST %CHECKPOINT_DIR%\llvm_install goto llvm_install_done
echo LLVM - Installing

rem General headers
cd %BUILD_DIR%\%LLVM_NAME%
robocopy include\llvm %TOP%\include\llvm\llvm *.h /S > NUL:

cd %BUILD_DIR%\llvm-build
rem Arch-specific headers
robocopy include\llvm %TOP%\include\Windows\%ARCH%\llvm\llvm *.gen *.def *.h /S > NUL:

rem Arch-specific libraries
robocopy lib\Debug %LIB_ARCH_DEBUG_DIR%\llvm llvm*.lib llvm*.pdb /S /MIR /XF llvm_headers_do_not_build.* llvm-*.lib > NUL:
robocopy lib\Release %LIB_ARCH_RELEASE_DIR%\llvm llvm*.lib /S /MIR /XF llvm_headers_do_not_build.* llvm-*.lib > NUL:

touch %CHECKPOINT_DIR%\llvm_install
:llvm_install_done

rem =========== BOOST =============
if EXIST %CHECKPOINT_DIR%\boost_unpack goto boost_unpack_done
echo boost - Unpacking
cd %BUILD_DIR%
type %TOP%\SourcePackages\%BOOST_NAME%.tar.bz2 | bzip2 -d -c | tar -x -f- 2> NUL:
touch %CHECKPOINT_DIR%\boost_unpack
:boost_unpack_done

if EXIST %CHECKPOINT_DIR%\boost_buildjam goto boost_buildjam_done
echo boost - Build Jam
cd %BUILD_DIR%\%BOOST_NAME%\tools\build\v2\engine
call .\build.bat
touch %CHECKPOINT_DIR%\boost_buildjam
:boost_buildjam_done

if EXIST %CHECKPOINT_DIR%\boost_compile goto boost_compile_done
echo boost - Compile debug + release
cd %BUILD_DIR%\%BOOST_NAME%
.\tools\build\v2\engine\bin.ntx86\bjam program_options serialization date_time iostreams system filesystem thread variant=debug,release toolset=msvc link=static runtime-link=static threading=multi define=_SCL_SECURE_NO_WARNINGS=1 define=_ITERATOR_DEBUG_LEVEL=0 define=_SECURE_SCL=0

touch %CHECKPOINT_DIR%\boost_compile
:boost_compile_done

if EXIST %CHECKPOINT_DIR%\boost_install goto boost_install_done
echo boost - Installing
cd %BUILD_DIR%\%BOOST_NAME%

if NOT EXIST %LIB_ARCH_DEBUG_DIR%\boost mkdir %LIB_ARCH_DEBUG_DIR%\boost
copy bin.v2\libs\program_options\build\msvc-10.0\debug\link-static\runtime-link-static\threading-multi\libboost_program_options-vc100-mt-sgd-1_47.lib %LIB_ARCH_DEBUG_DIR%\boost
copy bin.v2\libs\date_time\build\msvc-10.0\debug\link-static\runtime-link-static\threading-multi\libboost_date_time-vc100-mt-sgd-1_47.lib %LIB_ARCH_DEBUG_DIR%\boost
copy bin.v2\libs\iostreams\build\msvc-10.0\debug\link-static\runtime-link-static\threading-multi\libboost_iostreams-vc100-mt-sgd-1_47.lib %LIB_ARCH_DEBUG_DIR%\boost
copy bin.v2\libs\thread\build\msvc-10.0\debug\link-static\runtime-link-static\threading-multi\libboost_thread-vc100-mt-sgd-1_47.lib %LIB_ARCH_DEBUG_DIR%\boost
copy bin.v2\libs\serialization\build\msvc-10.0\debug\link-static\runtime-link-static\threading-multi\libboost_serialization-vc100-mt-sgd-1_47.lib %LIB_ARCH_DEBUG_DIR%\boost
copy bin.v2\libs\system\build\msvc-10.0\debug\link-static\runtime-link-static\threading-multi\libboost_system-vc100-mt-sgd-1_47.lib %LIB_ARCH_DEBUG_DIR%\boost
copy bin.v2\libs\filesystem\build\msvc-10.0\debug\link-static\runtime-link-static\threading-multi\libboost_filesystem-vc100-mt-sgd-1_47.lib %LIB_ARCH_DEBUG_DIR%\boost

if NOT EXIST %LIB_ARCH_RELEASE_DIR%\boost mkdir %LIB_ARCH_RELEASE_DIR%\boost
copy bin.v2\libs\program_options\build\msvc-10.0\release\link-static\runtime-link-static\threading-multi\libboost_program_options-vc100-mt-s-1_47.lib %LIB_ARCH_RELEASE_DIR%\boost
copy bin.v2\libs\date_time\build\msvc-10.0\release\link-static\runtime-link-static\threading-multi\libboost_date_time-vc100-mt-s-1_47.lib %LIB_ARCH_RELEASE_DIR%\boost
copy bin.v2\libs\iostreams\build\msvc-10.0\release\link-static\runtime-link-static\threading-multi\libboost_iostreams-vc100-mt-s-1_47.lib %LIB_ARCH_RELEASE_DIR%\boost
copy bin.v2\libs\thread\build\msvc-10.0\release\link-static\runtime-link-static\threading-multi\libboost_thread-vc100-mt-s-1_47.lib %LIB_ARCH_RELEASE_DIR%\boost
copy bin.v2\libs\serialization\build\msvc-10.0\release\link-static\runtime-link-static\threading-multi\libboost_serialization-vc100-mt-s-1_47.lib %LIB_ARCH_RELEASE_DIR%\boost
copy bin.v2\libs\system\build\msvc-10.0\release\link-static\runtime-link-static\threading-multi\libboost_system-vc100-mt-s-1_47.lib %LIB_ARCH_RELEASE_DIR%\boost
copy bin.v2\libs\filesystem\build\msvc-10.0\release\link-static\runtime-link-static\threading-multi\libboost_filesystem-vc100-mt-s-1_47.lib %LIB_ARCH_RELEASE_DIR%\boost

robocopy boost %TOP%\include\boost\boost *.* /S > NUL:

touch %CHECKPOINT_DIR%\boost_install
:boost_install_done

rem ============= HDF5 ==============

if EXIST %CHECKPOINT_DIR%\hdf5_unpack goto hdf5_unpack_done
echo HDF5 - Unpacking
cd %BUILD_DIR%
type "%TOP%\SourcePackages\%HDF5_NAME%.tar.bz2" | bzip2 -d -c | tar -x -f-
call %HDF5_NAME%\Windows\copy_hdf.bat
type %TOP%\Patches\Windows\%HDF5_NAME%-patch.tar.bz2 | bzip2 -d -c | tar -x -f- 2> NUL:
touch %CHECKPOINT_DIR%\hdf5_unpack
:hdf5_unpack_done

if EXIST %CHECKPOINT_DIR%\hdf5_compile_debug goto hdf5_compile_debug_done
echo HDF5 - Compiling debug
cd %BUILD_DIR%\%HDF5_NAME%\windows\proj\all
devenv all.sln /build "Debug|%VSARCH%"
touch %CHECKPOINT_DIR%\hdf5_compile_debug
:hdf5_compile_debug_done

if EXIST %CHECKPOINT_DIR%\hdf5_compile_release goto hdf5_compile_release_done
echo HDF5 - Compiling release
cd %BUILD_DIR%\%HDF5_NAME%\windows\proj\all
devenv all.sln /build "Release|%VSARCH%"
touch %CHECKPOINT_DIR%\hdf5_compile_release
:hdf5_compile_release_done

if EXIST %CHECKPOINT_DIR%\hdf5_install goto hdf5_install_done
echo hdf5 - Installing

cd %BUILD_DIR%\%HDF5_NAME%\proj

if NOT EXIST %LIB_ARCH_DEBUG_DIR%\hdf5 mkdir %LIB_ARCH_DEBUG_DIR%\hdf5
copy hdf5\Debug\hdf5d.lib %LIB_ARCH_DEBUG_DIR%\hdf5\hdf5.lib
copy hdf5_hl\Debug\hdf5_hld.lib %LIB_ARCH_DEBUG_DIR%\hdf5\hdf5_hl.lib

if NOT EXIST %LIB_ARCH_RELEASE_DIR%\hdf5 mkdir %LIB_ARCH_RELEASE_DIR%\hdf5
copy hdf5\Release\*.lib %LIB_ARCH_RELEASE_DIR%\hdf5
copy hdf5_hl\Release\*.lib %LIB_ARCH_RELEASE_DIR%\hdf5

cd %BUILD_DIR%\%HDF5_NAME%
robocopy src %TOP%\include\hdf5 *.h > NUL:
robocopy hl\src %TOP%\include\hdf5 *.h > NUL:

touch %CHECKPOINT_DIR%\hdf5_install
:hdf5_install_done

rem ============= ALEMBIC ==============

if EXIST %CHECKPOINT_DIR%\alembic_unpack goto alembic_unpack_done
echo ALEMBIC - Unpacking
cd %BUILD_DIR%
type %TOP%\SourcePackages\%ALEMBIC_NAME%.tar.bz2 | bzip2 -d -c | tar -x -f-
type %TOP%\Patches\Windows\%ALEMBIC_NAME%-patch.tar.bz2 | bzip2 -d -c | tar -x -f- 2> NUL:
if NOT EXIST %BUILD_DIR%\%ALEMBIC_NAME%\Output mkdir %BUILD_DIR%\%ALEMBIC_NAME%\Output
if NOT EXIST %BUILD_DIR%\%ALEMBIC_NAME%\thirdparty mklink /J %BUILD_DIR%\%ALEMBIC_NAME%\thirdparty %BUILD_DIR%
if NOT EXIST %BUILD_DIR%\%ALEMBIC_NAME%\thirdpartyinc mklink /J %BUILD_DIR%\%ALEMBIC_NAME%\thirdpartyinc %TOP%\include
if NOT EXIST %BUILD_DIR%\%ALEMBIC_NAME%\thirdpartylib mklink /J %BUILD_DIR%\%ALEMBIC_NAME%\thirdpartylib %TOP%\lib\Windows
touch %CHECKPOINT_DIR%\alembic_unpack
:alembic_unpack_done

SET ALEMBIC_ROOT=%BUILD_DIR%\%ALEMBIC_NAME%
SET ALEMBIC_OUT=%BUILD_DIR%\%ALEMBIC_NAME%\Output

if EXIST %CHECKPOINT_DIR%\alembic_createproj goto alembic_createproj_done
echo ALEMBIC - Creating projects
cd %BUILD_DIR%
rem call %BUILD_DIR%\%ALEMBIC_NAME%\init_Alembic.cmd db:
call %BUILD_DIR%\%ALEMBIC_NAME%\init_Alembic.cmd
touch %CHECKPOINT_DIR%\alembic_createproj
:alembic_createproj_done

if EXIST %CHECKPOINT_DIR%\alembic_build goto alembic_build_done
echo ALEMBIC - Buiding
cd %BUILD_DIR%
call %BUILD_DIR%\%ALEMBIC_NAME%\build_Alembic.cmd
touch %CHECKPOINT_DIR%\alembic_build
:alembic_build_done

if EXIST %CHECKPOINT_DIR%\alembic_install goto alembic_install_done
echo alembic - Installing
cd %BUILD_DIR%\%ALEMBIC_NAME%
robocopy Output\x86\alembic\RelWithDebInfo %LIB_ARCH_DEBUG_DIR%\alembic *.lib > NUL:
robocopy Output\x86\alembic\RelWithDebInfo %LIB_ARCH_RELEASE_DIR%\alembic *.lib > NUL:
robocopy lib %TOP%\include\alembic *.h /S > NUL:
touch %CHECKPOINT_DIR%\alembic_install
:alembic_install_done

rem ============= TEEM ==============
if EXIST %CHECKPOINT_DIR%\teem_unpack goto teem_unpack_done
echo teem - Unpacking
cd %BUILD_DIR%
type "%TOP%\SourcePackages\%TEEM_NAME%.tar.bz2" | bzip2 -d -c | tar -x -f- 2> NUL:
type "%TOP%\Patches\Windows\%TEEM_NAME%-patch.tar.bz2" | bzip2 -d -c | tar -x -f- 2> NUL:
touch %CHECKPOINT_DIR%\teem_unpack
:teem_unpack_done

if EXIST %CHECKPOINT_DIR%\teem_cmake goto teem_cmake_done
echo teem - CMake - generating projects
cd %BUILD_DIR%\%TEEM_NAME%
cmake -G "Visual Studio 10" -DTeem_BZIP2=OFF -DTeem_PTHREAD=OFF -DTeem_PNG=OFF -DZLIB_LIBRARY=%LIB_ARCH_RELEASE_DIR%\zlib\zlib.lib -DZLIB_INCLUDE_DIR=%TOP%\include\zlib .
rem Do it twice; first time it gives errors about no bz or png but we don't care
rem cmake -G "Visual Studio 10" -DZLIB_LIBRARY=%LIB_ARCH_RELEASE_DIR%\zlib\zlib.lib -DZLIB_INCLUDE_DIR=%TOP%\include\zlib .
touch %CHECKPOINT_DIR%\teem_cmake
:teem_cmake_done

if EXIST %CHECKPOINT_DIR%\teem_compile_debug goto teem_compile_debug_done
echo teem - Compiling debug
cd %BUILD_DIR%\%TEEM_NAME%
devenv teem.vcxproj /build "Debug|%VSARCH%"
touch %CHECKPOINT_DIR%\teem_compile_debug
:teem_compile_debug_done

if EXIST %CHECKPOINT_DIR%\teem_compile_release goto teem_compile_release_done
echo teem - Compiling release
cd %BUILD_DIR%\%TEEM_NAME%
devenv teem.vcxproj /build "Release|%VSARCH%"
touch %CHECKPOINT_DIR%\teem_compile_release
:teem_compile_release_done

if EXIST %CHECKPOINT_DIR%\teem_install goto teem_install_done
echo teem - Installing
cd %BUILD_DIR%\%TEEM_NAME%\bin
echo F | xcopy Debug\teem.lib %LIB_ARCH_DEBUG_DIR%\teem\teem.lib /Y > NUL:
echo F | xcopy Debug\teem.pdb %LIB_ARCH_DEBUG_DIR%\teem\teem.pdb /Y > NUL:
echo F | xcopy Release\teem.lib %LIB_ARCH_RELEASE_DIR%\teem\teem.lib /Y > NUL:
cd %BUILD_DIR%\%TEEM_NAME%\include\teem
echo D | xcopy *.h %TOP%\include\teem\teem /Y > NUL:
touch %CHECKPOINT_DIR%\teem_install
:teem_install_done

rem ============= TIFF ===============
if EXIST %CHECKPOINT_DIR%\tiff_unpack goto tiff_unpack_done
echo TIFF - Unpacking
cd %BUILD_DIR%
type "%TOP%\SourcePackages\%TIFF_NAME%.tar.bz2" | bzip2 -d -c | tar -x -f- 2> NUL:
type "%TOP%\Patches\Windows\%TIFF_NAME%-patch.tar.bz2" | bzip2 -d -c | tar -x -f- 2> NUL:
touch %CHECKPOINT_DIR%\tiff_unpack
:tiff_unpack_done

if EXIST %CHECKPOINT_DIR%\tiff_config goto tiff_config_debug_done
echo TIFF - Configuring. Note: this builds too but we just want to generate config files.
cd %BUILD_DIR%\%TIFF_NAME%\libtiff
nmake /f makefile.vc
cd %BUILD_DIR%\%TIFF_NAME%\port
nmake /f makefile.vc
devenv vstudio.sln /build "Debug|%VSARCH%"
touch %CHECKPOINT_DIR%\tiff_config
:tiff_config_debug_done

if EXIST %CHECKPOINT_DIR%\tiff_compile_debug goto tiff_compile_debug_done
echo TIFF - Compiling debug
cd %BUILD_DIR%\%TIFF_NAME%\vstudio
devenv vstudio.sln /build "Debug|%VSARCH%"
touch %CHECKPOINT_DIR%\tiff_compile_debug
:tiff_compile_debug_done

if EXIST %CHECKPOINT_DIR%\tiff_compile_release goto tiff_compile_release_done
echo TIFF - Compiling release
cd %BUILD_DIR%\%TIFF_NAME%\vstudio
devenv vstudio.sln /build "Release|%VSARCH%"
touch %CHECKPOINT_DIR%\tiff_compile_release
:tiff_compile_release_done

if EXIST %CHECKPOINT_DIR%\tiff_install goto tiff_install_done
echo TIFF - Installing
cd %BUILD_DIR%\%TIFF_NAME%\vstudio
echo D | xcopy "Debug\libtiff.lib" %LIB_ARCH_DEBUG_DIR%\tiff\ /Y > NUL:
echo D | xcopy "Release\libtiff.lib" %LIB_ARCH_RELEASE_DIR%\tiff\ /Y > NUL:
cd %BUILD_DIR%\%TIFF_NAME%
robocopy libtiff %INCLUDE_ARCH_DEBUG_DIR%\tiff\libtiff *.h /s > NUL:
robocopy libtiff %INCLUDE_ARCH_RELEASE_DIR%\tiff\libtiff *.h /s > NUL:
touch %CHECKPOINT_DIR%\tiff_install
:tiff_install_done

rem ============= JPEG ===============
if EXIST %CHECKPOINT_DIR%\jpeg_unpack goto jpeg_unpack_done
echo JPEG - Unpacking
cd %BUILD_DIR%
type "%TOP%\SourcePackages\%JPEG_SOURCE%.tar.bz2" | bzip2 -d -c | tar -x -f- 2> NUL:
type "%TOP%\Patches\Windows\%JPEG_SOURCE%-patch.tar.bz2" | bzip2 -d -c | tar -x -f- 2> NUL:
touch %CHECKPOINT_DIR%\jpeg_unpack
:jpeg_unpack_done

if EXIST %CHECKPOINT_DIR%\jpeg_build goto jpeg_build_done
echo JPEG - Building (only Release). Note: this builds too but we just want to generate config files.
cd %BUILD_DIR%\%JPEG_NAME%\
copy /Y jconfig.vc jconfig.h
nmake /f makefile.vc nodebug=1
touch %CHECKPOINT_DIR%\jpeg_build_done
:jpeg_build_done

if EXIST %CHECKPOINT_DIR%\jpeg_install goto jpeg_install_done
echo JPEG - Installing
cd %BUILD_DIR%\%JPEG_NAME%
echo D | xcopy "libjpeg.lib" %LIB_ARCH_DEBUG_DIR%\jpeg\ /Y > NUL:
echo D | xcopy "libjpeg.lib" %LIB_ARCH_RELEASE_DIR%\jpeg\ /Y > NUL:
robocopy . %INCLUDE_ARCH_DEBUG_DIR%\jpeg\jpeg *.h /s > NUL:
robocopy . %INCLUDE_ARCH_RELEASE_DIR%\jpeg\jpeg *.h /s > NUL:
touch %CHECKPOINT_DIR%\jpeg_install
:jpeg_install_done

rem ============= OPENCV ===============
if EXIST %CHECKPOINT_DIR%\opencv_unpack goto opencv_unpack_done
echo OPENCV - Unpacking
cd %BUILD_DIR%
type "%TOP%\SourcePackages\%OPENCV_SOURCE%.tar.bz2" | bzip2 -d -c | tar -x -f- 2> NUL:
type "%TOP%\Patches\Windows\%OPENCV_SOURCE%-patch.tar.bz2" | bzip2 -d -c | tar -x -f- 2> NUL:
touch %CHECKPOINT_DIR%\opencv_unpack
:opencv_unpack_done

if EXIST %CHECKPOINT_DIR%\opencv_cmake goto opencv_cmake_done
echo OPENCV - CMake - generating projects
cd %BUILD_DIR%\%OPENCV_NAME%
echo %BUILD_DIR%\%OPENCV_NAME%
cmake -G "Visual Studio 10" -DWITH_CUDA=OFF -DBUILD_SHARED_LIBS=OFF -DBUILD_WITH_STATIC_CRT=ON -DCMAKE_CXX_FLAGS="/D_SCL_SECURE_NO_WARNINGS=1 /D_ITERATOR_DEBUG_LEVEL=0 /D_SECURE_SCL=0" -DCMAKE_C_FLAGS="/D_SCL_SECURE_NO_WARNINGS=1 /D_ITERATOR_DEBUG_LEVEL=0 /D_SECURE_SCL=0"
touch %CHECKPOINT_DIR%\opencv_cmake
:opencv_cmake_done

if EXIST %CHECKPOINT_DIR%\opencv_compile_debug goto opencv_compile_debug_done
echo OPENCV - Compiling debug
cd %BUILD_DIR%\%OPENCV_NAME%
devenv OpenCV.sln /build "Debug|%VSARCH%"
touch %CHECKPOINT_DIR%\opencv_compile_debug
:opencv_compile_debug_done

if EXIST %CHECKPOINT_DIR%\opencv_compile_release goto opencv_compile_release_done
echo OPENCV - Compiling release
cd %BUILD_DIR%\%OPENCV_NAME%
devenv OpenCV.sln /build "Release|%VSARCH%"
touch %CHECKPOINT_DIR%\opencv_compile_release
:opencv_compile_release_done

if EXIST %CHECKPOINT_DIR%\opencv_install goto opencv_install_done
echo OPENCV - Installing
cd %BUILD_DIR%\%OPENCV_NAME%\lib\Debug
if NOT EXIST %LIB_ARCH_DEBUG_DIR%\opencv mkdir %LIB_ARCH_DEBUG_DIR%\opencv
echo DOS batch files are just SOOO bad. Can't even remove suffix from a filename in batch!
copy opencv_calib3d%OPENCV_VER%d.lib %LIB_ARCH_DEBUG_DIR%\opencv\opencv_calib3d.lib
copy opencv_contrib%OPENCV_VER%d.lib %LIB_ARCH_DEBUG_DIR%\opencv\opencv_contrib.lib
copy opencv_flann%OPENCV_VER%d.lib %LIB_ARCH_DEBUG_DIR%\opencv\opencv_flann.lib
copy opencv_legacy%OPENCV_VER%d.lib %LIB_ARCH_DEBUG_DIR%\opencv\opencv_legacy.lib
copy opencv_gpu%OPENCV_VER%d.lib %LIB_ARCH_DEBUG_DIR%\opencv\opencv_gpu.lib
copy opencv_ml%OPENCV_VER%d.lib %LIB_ARCH_DEBUG_DIR%\opencv\opencv_ml.lib
copy opencv_objdetect%OPENCV_VER%d.lib %LIB_ARCH_DEBUG_DIR%\opencv\opencv_objdetect.lib
copy opencv_features2d%OPENCV_VER%d.lib %LIB_ARCH_DEBUG_DIR%\opencv\opencv_features2d.lib
copy opencv_imgproc%OPENCV_VER%d.lib %LIB_ARCH_DEBUG_DIR%\opencv\opencv_imgproc.lib
copy opencv_highgui%OPENCV_VER%d.lib %LIB_ARCH_DEBUG_DIR%\opencv\opencv_highgui.lib
copy opencv_video%OPENCV_VER%d.lib %LIB_ARCH_DEBUG_DIR%\opencv\opencv_video.lib
copy opencv_core%OPENCV_VER%d.lib %LIB_ARCH_DEBUG_DIR%\opencv\opencv_core.lib
cd %BUILD_DIR%\%OPENCV_NAME%\lib\Release
if NOT EXIST %LIB_ARCH_RELEASE_DIR%\opencv mkdir %LIB_ARCH_RELEASE_DIR%\opencv
copy opencv_calib3d%OPENCV_VER%.lib %LIB_ARCH_RELEASE_DIR%\opencv\opencv_calib3d.lib
copy opencv_contrib%OPENCV_VER%.lib %LIB_ARCH_RELEASE_DIR%\opencv\opencv_contrib.lib
copy opencv_flann%OPENCV_VER%.lib %LIB_ARCH_RELEASE_DIR%\opencv\opencv_flann.lib
copy opencv_legacy%OPENCV_VER%.lib %LIB_ARCH_RELEASE_DIR%\opencv\opencv_legacy.lib
copy opencv_gpu%OPENCV_VER%.lib %LIB_ARCH_RELEASE_DIR%\opencv\opencv_gpu.lib
copy opencv_ml%OPENCV_VER%.lib %LIB_ARCH_RELEASE_DIR%\opencv\opencv_ml.lib
copy opencv_objdetect%OPENCV_VER%.lib %LIB_ARCH_RELEASE_DIR%\opencv\opencv_objdetect.lib
copy opencv_features2d%OPENCV_VER%.lib %LIB_ARCH_RELEASE_DIR%\opencv\opencv_features2d.lib
copy opencv_imgproc%OPENCV_VER%.lib %LIB_ARCH_RELEASE_DIR%\opencv\opencv_imgproc.lib
copy opencv_highgui%OPENCV_VER%.lib %LIB_ARCH_RELEASE_DIR%\opencv\opencv_highgui.lib
copy opencv_video%OPENCV_VER%.lib %LIB_ARCH_RELEASE_DIR%\opencv\opencv_video.lib
copy opencv_core%OPENCV_VER%.lib %LIB_ARCH_RELEASE_DIR%\opencv\opencv_core.lib
cd %BUILD_DIR%\%OPENCV_NAME%
robocopy include %INCLUDE_ARCH_DEBUG_DIR%\OpenCV\ *.h *.hpp /s > NUL:
robocopy modules\calib3d\include %INCLUDE_ARCH_DEBUG_DIR%\OpenCV\ *.h *.hpp /s > NUL:
robocopy modules\contrib\include %INCLUDE_ARCH_DEBUG_DIR%\OpenCV\ *.h *.hpp /s > NUL:
robocopy modules\flann\include %INCLUDE_ARCH_DEBUG_DIR%\OpenCV\ *.h *.hpp /s > NUL:
robocopy modules\legacy\include %INCLUDE_ARCH_DEBUG_DIR%\OpenCV\ *.h *.hpp /s > NUL:
robocopy modules\gpu\include %INCLUDE_ARCH_DEBUG_DIR%\OpenCV\ *.h *.hpp /s > NUL:
robocopy modules\ml\include %INCLUDE_ARCH_DEBUG_DIR%\OpenCV\ *.h *.hpp /s > NUL:
robocopy modules\objdetect\include %INCLUDE_ARCH_DEBUG_DIR%\OpenCV\ *.h *.hpp /s > NUL:
robocopy modules\features2d\include %INCLUDE_ARCH_DEBUG_DIR%\OpenCV\ *.h *.hpp /s > NUL:
robocopy modules\imgproc\include %INCLUDE_ARCH_DEBUG_DIR%\OpenCV\ *.h *.hpp /s > NUL:
robocopy modules\highgui\include %INCLUDE_ARCH_DEBUG_DIR%\OpenCV\ *.h *.hpp /s > NUL:
robocopy modules\video\include %INCLUDE_ARCH_DEBUG_DIR%\OpenCV\ *.h *.hpp /s > NUL:
robocopy modules\core\include %INCLUDE_ARCH_DEBUG_DIR%\OpenCV\ *.h *.hpp /s > NUL:

robocopy %INCLUDE_ARCH_DEBUG_DIR%\OpenCV\ %INCLUDE_ARCH_RELEASE_DIR%\OpenCV\ *.* /s > NUL:
touch %CHECKPOINT_DIR%\opencv_install
:opencv_install_done

rem exit
