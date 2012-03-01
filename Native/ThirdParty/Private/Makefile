NODE_VERSION := 0.6.3
LLVM_VERSION := 2.9
PNG_VERSION := 1.4.4
ZLIB_VERSION := 1.2.5
LIBEDIT_VERSION := 3.0
ILMBASE_VERSION := 1.0.2
OPENEXR_VERSION := 1.7.0
FFMPEG_VERSION := 0.6.3
GTKGLEXT_VERSION := 1.2.0
BOOST_VERSION := 1.47.0
HDF5_VERSION := 1.8.7
ALEMBIC_VERSION := 1.0_2011080800
TEEM_VERSION := 1.10.0
TIFF_VERSION := 3.9.5
JPEG_VERSION := 8d
OPENCV_SOURCE_VERSION := 2.3.1a
OPENCV_VERSION := 2.3.1

RSYNC := rsync

ifeq ( $(OS),Windows_NT )
  SHELL := ./bin/Windows/bash
  BUILD_PLATFORM := Windows
  TOOL_PREFIX := ./bin/Windows
else
  BUILD_PLATFORM := $(shell uname -s)
endif

TARGET_BASE=$(PWD)
ifeq ($(BUILD_PLATFORM),Windows)
  ARCHS=i386 x86_64
  FIRST_ARCH=i386
endif
ifeq ($(BUILD_PLATFORM),Linux)
  ARCHS=$(shell uname -m)
  FIRST_ARCH=$(ARCHS)
endif 
ifeq ($(BUILD_PLATFORM),Darwin)
  ARCHS=i386 x86_64
  FIRST_ARCH=i386
  SYS_SDK_ROOT=/Developer/SDKs/MacOSX10.6.sdk
endif

.PHONY: all build install clean check_env_vars \
  unpack_ilmbase patch_ilmbase configure_ilmbase build_ilmbase install_ilmbase clean_ilmbase \
  unpack_openexr patch_openexr configure_openexr build_openexr install_openexr clean_openexr \
  unpack_llvm patch_llvm configure_llvm build_llvm install_llvm clean_llvm \
  unpack_libpng patch_libpng configure_libpng build_libpng install_libpng clean_libpng \
  unpack_ffmpeg patch_ffmpeg configure_ffmpeg build_ffmpeg install_ffmpeg clean_ffmpeg \
  unpack_node patch_node configure_node build_node install_node clean_node \
  unpack_zlib patch_zlib configure_zlib build_zlib install_zlib clean_zlib \
  unpack_libedit patch_libedit configure_libedit build_libedit install_libedit clean_libedit \
  unpack_tiff patch_tiff configure_tiff build_tiff install_tiff clean_tiff \
  unpack_jpeg patch_jpeg configure_jpeg build_jpeg install_jpeg clean_jpeg \
  unpack_opencv patch_opencv configure_opencv build_opencv install_opencv clean_opencv \
  unpack_gtkglext patch_gtkglext configure_gtkglext build_gtkglext install_gtkglext clean_gtkglext \
  unpack_boost patch_boost configure_boost build_boost install_boost clean_boost \
  unpack_hdf5 patch_hdf5 configure_hdf5 build_hdf5 install_hdf5 clean_hdf5 \
  unpack_alembic patch_alembic configure_alembic build_alembic install_alembic clean_alembic \
  unpack_teem patch_teem configure_teem build_teem install_teem clean_teem \
  install_llvm_headers install_llvm_libraries

all: build

build: check_env_vars build_ilmbase build_openexr build_llvm build_libpng build_ffmpeg build_teem build_node build_libedit build_tiff build_jpeg build_opencv

unpack: check_env_vars unpack_ilmbase unpack_openexr unpack_llvm unpack_libpng unpack_ffmpeg unpack_teem unpack_node unpack_libedit unpack_tiff unpack_jpeg unpack_opencv

check_env_vars: 
ifeq ($(BUILD_PLATFORM),Darwin)
	@echo "Compiling thirdparty for Mac."
else ifeq ($(BUILD_PLATFORM),Linux)
	@echo "Compiling thirdparty for Linux."
else ifeq ($(BUILD_PLATFORM),Windows)
	@echo "Compiling thirdparty for Win32."
else
	@echo ERROR: Unsupported BUILD_PLATFORM=$(BUILD_PLATFORM) ARCHS=$(ARCHS)
	@exit 2
endif

build_ilmbase: Build/.ilmbase.build

configure_ilmbase: Build/.ilmbase.configure

patch_ilmbase: Build/.ilmbase.patch

unpack_ilmbase: Build/.ilmbase.unpack

Build/.ilmbase.build: Build/.ilmbase.configure
	for ARCH in $(ARCHS); do \
		( cd Build/ilmbase-$(ILMBASE_VERSION)-$$ARCH && make -j2 ); \
	done
	touch $@

Build/.ilmbase.configure: Build/.ilmbase.patch
	for ARCH in $(ARCHS); do \
                if [ $$ARCH = "x86_64" ]; then \
                  ARCH_FLAGS=-m64; \
                else \
                  ARCH_FLAGS=-m32; \
                fi; \
                ARCH_FLAGS="$$ARCH_FLAGS -fPIC"; \
		( cd Build/ilmbase-$(ILMBASE_VERSION)-$$ARCH \
                  && CXXFLAGS=$$ARCH_FLAGS \
                    LDFLAGS=$$ARCH_FLAGS \
                    ./configure ) \
	done
	touch $@

Build/.ilmbase.patch: Build/.ilmbase.unpack
ifeq ($(BUILD_PLATFORM),Darwin)
	for ARCH in $(ARCHS); do \
		( cd Build/ilmbase-$(ILMBASE_VERSION)-$$ARCH && patch -p1 <../../Patches/universal_build_10.4_ilmbase.diff ); \
	done
endif
	for ARCH in $(ARCHS); do \
		( cd Build/ilmbase-$(ILMBASE_VERSION)-$$ARCH && patch -p1 <../../Patches/ilmbase-linux-memset.diff ); \
	done
	touch $@

Build/.ilmbase.unpack:
	mkdir -p Build
	for ARCH in $(ARCHS); do \
		( cd Build && tar -xjf ../SourcePackages/ilmbase-$(ILMBASE_VERSION).tar.bz2 ); \
		( cd Build && mv ilmbase-$(ILMBASE_VERSION) ilmbase-$(ILMBASE_VERSION)-$$ARCH ); \
	done
	touch $@

build_openexr: Build/.openexr.build

configure_openexr: Build/.openexr.configure

patch_openexr: Build/.openexr.patch

unpack_openexr: Build/.openexr.unpack

Build/.openexr.build: Build/.openexr.configure
	for ARCH in $(ARCHS); do \
		( cd Build/openexr-$(OPENEXR_VERSION)-$$ARCH && make -j2 ); \
	done
	touch $@

Build/.openexr.configure: Build/.openexr.patch
	for ARCH in $(ARCHS); do \
                if [ $$ARCH = "x86_64" ]; then \
                  ARCH_FLAGS=-m64; \
                else \
                  ARCH_FLAGS=-m32; \
                fi; \
                ARCH_FLAGS="$$ARCH_FLAGS -fPIC"; \
		( cd Build/openexr-$(OPENEXR_VERSION)-$$ARCH \
                  && \
                    CXXFLAGS="$$ARCH_FLAGS -I$(TARGET_BASE)/include/ilmbase -I$(TARGET_BASE)/include/$(BUILD_PLATFORM)/$$ARCH/ilmbase" \
                    LDFLAGS="$$ARCH_FLAGS -L$(TARGET_BASE)/lib/$(BUILD_PLATFORM)/$$ARCH/Release/ilmbase" \
                    ./configure \
                    --disable-ilmbasetest ); \
	done
	touch $@

Build/.openexr.patch: Build/.openexr.unpack
ifeq ($(BUILD_PLATFORM),Darwin)
	for ARCH in $(ARCHS); do \
		( cd Build/openexr-$(OPENEXR_VERSION)-$$ARCH && patch -p1 <../../Patches/universal_build_10.4_openexr.diff ); \
	done
endif
	for ARCH in $(ARCHS); do \
		( cd Build/openexr-$(OPENEXR_VERSION)-$$ARCH && patch -p1 <../../Patches/openexr-linux-mem.diff ); \
		( cd Build/openexr-$(OPENEXR_VERSION)-$$ARCH && patch -p1 <../../Patches/openexr-universal-memset.diff ); \
	done
	touch $@

Build/.openexr.unpack:
	mkdir -p Build
	for ARCH in $(ARCHS); do \
		( cd Build && tar -xjf ../SourcePackages/openexr-$(OPENEXR_VERSION).tar.bz2 ); \
		( cd Build && mv openexr-$(OPENEXR_VERSION) openexr-$(OPENEXR_VERSION)-$$ARCH ); \
	done
	touch $@

build_llvm: Build/.llvm.build

configure_llvm: Build/.llvm.configure

patch_llvm: Build/.llvm.patch

unpack_llvm: Build/.llvm.unpack

Build/.llvm.build: Build/.llvm.configure
	for ARCH in $(ARCHS); do \
		( cd Build/llvm-$(LLVM_VERSION)-$$ARCH && make -j2 ); \
	done
	touch $@

Build/.llvm.configure: Build/.llvm.patch
ifeq ($(BUILD_PLATFORM),Darwin)
	for ARCH in $(ARCHS); do \
                if [ "$$ARCH" = "x86_64" ]; then LLVM_ARCH=x86_64; else LLVM_ARCH=x86; fi; \
		( cd Build/llvm-$(LLVM_VERSION)-$$ARCH && CFLAGS="-fvisibility=hidden" ./configure --host=$$ARCH-apple-darwin10.4.0 --enable-optimized --enable-targets=$$LLVM_ARCH ); \
	done
else
	for ARCH in $(ARCHS); do \
                if [ "$$ARCH" = "x86_64" ]; then LLVM_ARCH=x86_64; else LLVM_ARCH=x86; fi; \
		( cd Build/llvm-$(LLVM_VERSION)-$$ARCH && ./configure --enable-optimized --enable-targets=$$LLVM_ARCH ); \
	done
endif
	touch $@

Build/.llvm.patch: Build/.llvm.unpack
ifeq ($(BUILD_PLATFORM),Windows)
	for ARCH in $(ARCHS); do \
		( cd Build && tar -xzf ../SourcePackages/llvm-$(LLVM_VERSION)-vc10-patch.tar.gz ); \
	done
endif
	touch $@

Build/.llvm.unpack:
	mkdir -p Build
	for ARCH in $(ARCHS); do \
		( cd Build && tar -xjf ../SourcePackages/llvm-$(LLVM_VERSION).tar.bz2 ); \
		pwd; \
		( cd Build && mv llvm-$(LLVM_VERSION) llvm-$(LLVM_VERSION)-$$ARCH ); \
	done
	touch $@

build_node: Build/.node.build

configure_node: Build/.node.configure

patch_node: Build/.node.patch

unpack_node: Build/.node.unpack

Build/.node.build: Build/.node.configure
	for ARCH in $(ARCHS); do \
          if [ "$$ARCH" = "x86_64" ]; then \
            export CFLAGS=-m64; \
            export CXXFLAGS=-m64; \
            export LINKFLAGS=-m64; \
          else \
            export CFLAGS=-m32; \
            export CXXFLAGS=-m32; \
            export LINKFLAGS=-m32; \
          fi; \
		( cd Build/node-$(NODE_VERSION)-$$ARCH && make ); \
	done

Build/.node.configure: Build/.node.patch
	for ARCH in $(ARCHS); do \
          if [ "$$ARCH" = "x86_64" ]; then \
            NODE_ARCH=x64; \
            export CFLAGS=-m64; \
            export CXXFLAGS=-m64; \
            export LINKFLAGS=-m64; \
          else \
            NODE_ARCH=ia32; \
            export CFLAGS=-m32; \
            export CXXFLAGS=-m32; \
            export LINKFLAGS=-m32; \
          fi; \
          ( cd Build/node-$(NODE_VERSION)-$$ARCH && ./configure --dest-cpu=$$NODE_ARCH --prefix=/usr/local ); \
	done
	touch $@

Build/.node.patch: Build/.node.unpack
	touch $@

Build/.node.unpack:
	mkdir -p Build
	for ARCH in $(ARCHS); do \
		( cd Build && tar jxf ../SourcePackages/node-$(NODE_VERSION).tar.bz2 && mv node-$(NODE_VERSION) node-$(NODE_VERSION)-$$ARCH ); \
	done
	touch $@

build_libedit: build_libedit Build/.libedit.build

configure_libedit: Build/.libedit.configure

patch_libedit: Build/.libedit.patch

unpack_libedit: Build/.libedit.unpack

Build/.libedit.build: Build/.libedit.configure
	for ARCH in $(ARCHS); do \
		( cd Build/libedit-$(LIBEDIT_VERSION)-$$ARCH && make -j2 ); \
	done
	touch $@

Build/.libedit.configure: Build/.libedit.patch
ifeq ($(BUILD_PLATFORM),Linux)
	for ARCH in $(ARCHS); do \
		( cd Build/libedit-$(LIBEDIT_VERSION)-$$ARCH && CFLAGS="-fPIC" ./configure ); \
	done
else 
	for ARCH in $(ARCHS); do \
		( cd Build/libedit-$(LIBEDIT_VERSION)-$$ARCH && CFLAGS="-arch $$ARCH" ./configure ); \
	done
endif
	touch $@

Build/.libedit.patch: Build/.libedit.unpack
	touch $@

Build/.libedit.unpack:
	mkdir -p Build
	for ARCH in $(ARCHS); do \
		( cd Build && tar jxf ../SourcePackages/libedit-$(LIBEDIT_VERSION).tar.bz2 && mv libedit-$(LIBEDIT_VERSION) libedit-$(LIBEDIT_VERSION)-$$ARCH ); \
	done
	touch $@

#####

build_jpeg: build_jpeg Build/.jpeg.build

configure_jpeg: Build/.jpeg.configure

patch_jpeg: Build/.jpeg.patch

unpack_jpeg: Build/.jpeg.unpack

Build/.jpeg.build: Build/.jpeg.configure
	for ARCH in $(ARCHS); do \
		( cd Build/jpeg-$(JPEG_VERSION)-$$ARCH && make -j2 ); \
	done
	touch $@

Build/.jpeg.configure: Build/.jpeg.patch
ifeq ($(BUILD_PLATFORM),Linux)
	for ARCH in $(ARCHS); do \
		( cd Build/jpeg-$(JPEG_VERSION)-$$ARCH && CFLAGS="-fPIC" ./configure --enable-static --disable-shared ); \
	done
else 
	for ARCH in $(ARCHS); do \
		( cd Build/jpeg-$(JPEG_VERSION)-$$ARCH && CFLAGS="-arch $$ARCH" ./configure --enable-static --disable-shared ); \
	done
endif
	touch $@

Build/.jpeg.patch: Build/.jpeg.unpack
	touch $@

Build/.jpeg.unpack:
	mkdir -p Build
	for ARCH in $(ARCHS); do \
		( cd Build && tar jxf ../SourcePackages/jpegsrc.v$(JPEG_VERSION).tar.bz2 && mv jpeg-$(JPEG_VERSION) jpeg-$(JPEG_VERSION)-$$ARCH ); \
	done
	touch $@

#####

build_tiff: build_tiff Build/.tiff.build

configure_tiff: Build/.tiff.configure

patch_tiff: Build/.tiff.patch

unpack_tiff: Build/.tiff.unpack

Build/.tiff.build: Build/.tiff.configure
	for ARCH in $(ARCHS); do \
		( cd Build/tiff-$(TIFF_VERSION)-$$ARCH && make -j2 ); \
	done
	touch $@

Build/.tiff.configure: Build/.tiff.patch
ifeq ($(BUILD_PLATFORM),Linux)
	for ARCH in $(ARCHS); do \
		( cd Build/tiff-$(TIFF_VERSION)-$$ARCH && CFLAGS="-fPIC" ./configure --enable-static --disable-shared ); \
	done
else 
	for ARCH in $(ARCHS); do \
		( cd Build/tiff-$(TIFF_VERSION)-$$ARCH && CFLAGS="-arch $$ARCH" ./configure --enable-static --disable-shared ); \
	done
endif
	touch $@

Build/.tiff.patch: Build/.tiff.unpack
	touch $@

Build/.tiff.unpack:
	mkdir -p Build
	for ARCH in $(ARCHS); do \
		( cd Build && tar jxf ../SourcePackages/tiff-$(TIFF_VERSION).tar.bz2 && mv tiff-$(TIFF_VERSION) tiff-$(TIFF_VERSION)-$$ARCH ); \
	done
	touch $@

#####

build_opencv: Build/.opencv.build

configure_opencv: Build/.opencv.configure

patch_opencv: Build/.opencv.patch

unpack_opencv: Build/.opencv.unpack

Build/.opencv.build: Build/.opencv.configure
	for ARCH in $(ARCHS); do \
		( cd Build/OpenCV-$(OPENCV_VERSION)-$$ARCH && make -j2 ); \
	done
	touch $@

Build/.opencv.configure: Build/.opencv.patch
ifeq ($(BUILD_PLATFORM),Darwin)
	for ARCH in $(ARCHS); do \
                if [ $$ARCH = "x86_64" ]; then \
                  CMAKE_OSX_ARCHITECTURES="x86_64"; \
                else \
                  CMAKE_OSX_ARCHITECTURES="i386"; \
                fi; \
		( cd Build/OpenCV-$(OPENCV_VERSION)-$$ARCH && cmake . -DCMAKE_OSX_ARCHITECTURES="$$CMAKE_OSX_ARCHITECTURES" -DBUILD_TESTS=OFF -DBUILD_SHARED_LIBS=OFF -DWITH_JPEG=OFF -DWITH_FFMPEG=OFF -DWITH_PNG=OFF -DWITH_TIFF=OFF -DWITH_JASPER=OFF -DWITH_OPENEXR=OFF -DWITH_GTK=OFF -DWITH_GSTREAMER=OFF -DWITH_V4L=OFF -DBUILD_NEW_PYTHON_SUPPORT=OFF ); \
	done
else
	for ARCH in $(ARCHS); do \
		( cd Build/OpenCV-$(OPENCV_VERSION)-$$ARCH && cmake . -DCMAKE_C_FLAGS="-fPIC" -DBUILD_TESTS=OFF -DBUILD_SHARED_LIBS=OFF -DWITH_JPEG=OFF -DWITH_FFMPEG=OFF -DWITH_PNG=OFF -DWITH_TIFF=OFF -DWITH_JASPER=OFF -DWITH_OPENEXR=OFF -DWITH_GTK=OFF -DWITH_GSTREAMER=OFF -DWITH_V4L=OFF -DBUILD_NEW_PYTHON_SUPPORT=OFF ); \
	done
endif
	touch $@

Build/.opencv.patch: Build/.opencv.unpack
	touch $@

Build/.opencv.unpack:
	mkdir -p Build
	for ARCH in $(ARCHS); do \
		( cd Build && tar jxf ../SourcePackages/OpenCV-$(OPENCV_SOURCE_VERSION).tar.bz2 && mv OpenCV-$(OPENCV_VERSION) OpenCV-$(OPENCV_VERSION)-$$ARCH ); \
	done
	touch $@

#####

build_gtkglext: build_gtkglext Build/.gtkglext.build

configure_gtkglext: Build/.gtkglext.configure

patch_gtkglext: Build/.gtkglext.patch

unpack_gtkglext: Build/.gtkglext.unpack

Build/.gtkglext.build: Build/.gtkglext.configure
	for ARCH in $(ARCHS); do \
		( cd Build/gtkglext-$(GTKGLEXT_VERSION)-$$ARCH && make -j2 ); \
	done
	touch $@

Build/.gtkglext.configure: Build/.gtkglext.patch
ifeq ($(BUILD_PLATFORM),Linux)
	for ARCH in $(ARCHS); do \
		( cd Build/gtkglext-$(GTKGLEXT_VERSION)-$$ARCH && ./configure --with-pic --enable-static --disable-shared ); \
	done
else 
	for ARCH in $(ARCHS); do \
		( cd Build/gtkglext-$(GTKGLEXT_VERSION)-$$ARCH && CFLAGS="-arch $$ARCH" ./configure ); \
	done
endif
	touch $@

Build/.gtkglext.patch: Build/.gtkglext.unpack
	for ARCH in $(ARCHS); do \
		( cd Build/gtkglext-$(GTKGLEXT_VERSION)-$$ARCH && patch -p1 <../../Patches/gtkglext.diff ); \
	done
	touch $@

Build/.gtkglext.unpack:
	mkdir -p Build
	for ARCH in $(ARCHS); do \
		( cd Build && tar jxf ../SourcePackages/gtkglext-$(GTKGLEXT_VERSION).tar.bz2 && mv gtkglext-$(GTKGLEXT_VERSION) gtkglext-$(GTKGLEXT_VERSION)-$$ARCH ); \
	done
	touch $@



build_boost: Build/.boost.build

configure_boost: Build/.boost.configure

patch_boost: Build/.boost.patch

unpack_boost: Build/.boost.unpack

Build/.boost.build: Build/.boost.configure
ifeq ($(BUILD_PLATFORM),Linux)
	for ARCH in $(ARCHS); do \
		if [ "$$ARCH" = "x86_64" ]; then \
			BJAM_FLAGS="toolset=gcc architecture=x86 instruction-set=opteron address-model=64"; \
		else \
			BJAM_FLAGS="toolset=gcc architecture=x86 instruction-set=i686 address-model=32"; \
		fi; \
		( cd Build/boost-$(BOOST_VERSION)-$$ARCH && ./bjam -d+2 $$BJAM_FLAGS link=static threading=multi variant=release cflags=-fPIC ); \
	done
else
	for ARCH in $(ARCHS); do \
		if [ "$$ARCH" = "x86_64" ]; then \
			BJAM_FLAGS="toolset=darwin architecture=x86 address-model=64"; \
		else \
			BJAM_FLAGS="toolset=darwin architecture=x86 address-model=32"; \
		fi; \
		( cd Build/boost-$(BOOST_VERSION)-$$ARCH && ./bjam -d+2 $$BJAM_FLAGS link=static threading=multi variant=release ); \
	done
endif
	touch $@

Build/.boost.configure: Build/.boost.patch
	for ARCH in $(ARCHS); do \
		( cd Build/boost-$(BOOST_VERSION)-$$ARCH && ./bootstrap.sh --with-libraries=date_time,program_options,iostreams,thread,serialization,system,filesystem ); \
	done
	touch $@

Build/.boost.patch: Build/.boost.unpack
	touch $@

Build/.boost.unpack:
	mkdir -p Build
	for ARCH in $(ARCHS); do \
		( cd Build && tar jxf ../SourcePackages/boost-$(BOOST_VERSION).tar.bz2 && mv boost-$(BOOST_VERSION) boost-$(BOOST_VERSION)-$$ARCH ); \
	done
	touch $@



build_hdf5: Build/.hdf5.build

configure_hdf5: Build/.hdf5.configure

patch_hdf5: Build/.hdf5.patch

unpack_hdf5: Build/.hdf5.unpack

Build/.hdf5.build: Build/.hdf5.configure
	for ARCH in $(ARCHS); do \
		( cd Build/hdf5-$(HDF5_VERSION)-$$ARCH && make -j4 ); \
	done
	touch $@

Build/.hdf5.configure: Build/.hdf5.patch
	for ARCH in $(ARCHS); do \
		if [ "$$ARCH" = "x86_64" ]; then \
			HDF5_CFLAGS=-m64; \
		else \
			HDF5_CFLAGS=-m32; \
		fi; \
		( cd Build/hdf5-$(HDF5_VERSION)-$$ARCH && CFLAGS=$$HDF5_CFLAGS ./configure --enable-threadsafe --enable-static --disable-shared --with-pic --with-pthread ); \
	done
	touch $@

Build/.hdf5.patch: Build/.hdf5.unpack
	touch $@

Build/.hdf5.unpack:
	mkdir -p Build
	for ARCH in $(ARCHS); do \
		( cd Build && tar jxf ../SourcePackages/hdf5-$(HDF5_VERSION).tar.bz2 && mv hdf5-$(HDF5_VERSION) hdf5-$(HDF5_VERSION)-$$ARCH ); \
	done
	touch $@



build_alembic: Build/.alembic.build

configure_alembic: Build/.alembic.configure

patch_alembic: Build/.alembic.patch

unpack_alembic: Build/.alembic.unpack

Build/.alembic.build: Build/.alembic.configure
	for ARCH in $(ARCHS); do \
		( cd Build/alembic-$(ALEMBIC_VERSION)-$$ARCH/alembic_build && make -j4 ); \
	done
	touch $@

Build/.alembic.configure: Build/.alembic.patch
ifeq ($(BUILD_PLATFORM),Linux)
	for ARCH in $(ARCHS); do \
		if [ "$$ARCH" = "x86_64" ]; then \
			ALEMBIC_CFLAGS="-fPIC"; \
			ALEMBIC_LDFLAGS=""; \
		else \
			ALEMBIC_CFLAGS="-fPIC -march=i686"; \
			ALEMBIC_LDFLAGS="-march=i686"; \
		fi; \
		( cd Build/alembic-$(ALEMBIC_VERSION)-$$ARCH && \
			CFLAGS="$$ALEMBIC_CFLAGS" CXXFLAGS="$$ALEMBIC_CFLAGS" LDFLAGS="$$ALEMBIC_LDFLAGS" python build/bootstrap/alembic_bootstrap.py \
				--disable-prman \
				--disable-arnold \
				--disable-maya \
				--hdf5_include_dir=$(TARGET_BASE)/include/hdf5/hdf5 \
				--hdf5_hdf5_library=$(TARGET_BASE)/Linux/$$ARCH/Release/lib/hdf5/libhdf5.a \
				--ilmbase_include_dir=$(TARGET_BASE)/include/ilmbase \
				--ilmbase_imath_library=$(TARGET_BASE)/Linux/$$ARCH/Release/lib/ilmbase/libImath.a \
				--boost_include_dir=$(TARGET_BASE)/include/boost \
				--boost_thread_library=$(TARGET_BASE)/Linux/$$ARCH/Release/lib/boost/libboost_thread.a \
				--zlib_include_dir=/usr/include \
				--zlib_library=/usr/lib/libz.a \
				alembic_build \
		); \
	done
else
	for ARCH in $(ARCHS); do \
		if [ "$$ARCH" = "x86_64" ]; then \
			ALEMBIC_CFLAGS="-fPIC -m64"; \
			ALEMBIC_LDFLAGS="-m64"; \
		else \
			ALEMBIC_CFLAGS="-fPIC -m32"; \
			ALEMBIC_LDFLAGS="-m32"; \
		fi; \
		( cd Build/alembic-$(ALEMBIC_VERSION)-$$ARCH && \
			CFLAGS="$$ALEMBIC_CFLAGS" CXXFLAGS="$$ALEMBIC_CFLAGS" LDFLAGS="$$ALEMBIC_LDFLAGS" python build/bootstrap/alembic_bootstrap.py \
				--disable-prman \
				--disable-arnold \
				--disable-maya \
				--hdf5_include_dir=$(TARGET_BASE)/include/hdf5/hdf5 \
				--hdf5_hdf5_library=$(TARGET_BASE)/Darwin/$$ARCH/Release/lib/hdf5/libhdf5.a \
				--ilmbase_include_dir=$(TARGET_BASE)/include/ilmbase \
				--ilmbase_imath_library=$(TARGET_BASE)/Darwin/$$ARCH/Release/lib/ilmbase/libImath.a \
				--boost_include_dir=$(TARGET_BASE)/include/boost \
				--boost_thread_library=$(TARGET_BASE)/Darwin/$$ARCH/Release/lib/boost/libboost_thread.a \
				--zlib_include_dir=$(SYS_SDK_ROOT)/usr/include \
				--zlib_library=$(SYS_SDK_ROOT)/usr/lib/libz.dylib \
				alembic_build \
		); \
	done
endif
	touch $@

Build/.alembic.patch: Build/.alembic.unpack
	touch $@

Build/.alembic.unpack:
	mkdir -p Build
	for ARCH in $(ARCHS); do \
		( cd Build && tar jxf ../SourcePackages/alembic-$(ALEMBIC_VERSION).tar.bz2 && mv alembic-$(ALEMBIC_VERSION) alembic-$(ALEMBIC_VERSION)-$$ARCH ); \
	done
	touch $@


build_libpng: build_zlib Build/.libpng.build

configure_libpng: Build/.libpng.configure

patch_libpng: Build/.libpng.patch

unpack_libpng: Build/.libpng.unpack

Build/.libpng.build: Build/.libpng.configure
	for ARCH in $(ARCHS); do \
		( cd Build/libpng-$(PNG_VERSION)-$$ARCH && make -j2 ); \
	done
	touch $@

Build/.libpng.configure: Build/.libpng.patch
ifeq ($(BUILD_PLATFORM),Linux)
	for ARCH in $(ARCHS); do \
		( cd Build/libpng-$(PNG_VERSION)-$$ARCH && CFLAGS="-fPIC" ./configure ); \
	done
else 
	for ARCH in $(ARCHS); do \
		( cd Build/libpng-$(PNG_VERSION)-$$ARCH && CFLAGS="-arch $$ARCH" ./configure ); \
	done
endif
	touch $@

Build/.libpng.patch: Build/.libpng.unpack
	touch $@

Build/.libpng.unpack:
	mkdir -p Build
	for ARCH in $(ARCHS); do \
		( cd Build && tar jxf ../SourcePackages/libpng-$(PNG_VERSION).tar.bz2 && mv libpng-$(PNG_VERSION) libpng-$(PNG_VERSION)-$$ARCH ) ; \
	done
	touch $@

build_ffmpeg: build_zlib Build/.ffmpeg.build

configure_ffmpeg: Build/.ffmpeg.configure

patch_ffmpeg: Build/.ffmpeg.patch

unpack_ffmpeg: Build/.ffmpeg.unpack

Build/.ffmpeg.build: Build/.ffmpeg.configure
	for ARCH in $(ARCHS); do \
		( cd Build/ffmpeg-$(FFMPEG_VERSION)-$$ARCH && make -j4 ); \
	done
	touch $@

Build/.ffmpeg.configure: Build/.ffmpeg.patch
ifeq ($(BUILD_PLATFORM),Linux)
	for ARCH in $(ARCHS); do \
		( cd Build/ffmpeg-$(FFMPEG_VERSION)-$$ARCH && ./configure --enable-static --disable-shared --enable-pthreads --enable-pic --disable-debug --disable-asm); \
	done
else 
	for ARCH in $(ARCHS); do \
    if [ $$ARCH = "x86_64" ]; then \
      ARCH_FLAGS="-m64"; \
    else \
      ARCH_FLAGS="-m32"; \
    fi; \
		( cd Build/ffmpeg-$(FFMPEG_VERSION)-$$ARCH && CFLAGS="$$ARCH_FLAGS" LDFLAGS="$$ARCH_FLAGS" ./configure --enable-pic --disable-asm ); \
	done
endif
	touch $@

Build/.ffmpeg.patch: Build/.ffmpeg.unpack
	touch $@

Build/.ffmpeg.unpack:
	mkdir -p Build
	for ARCH in $(ARCHS); do \
		( cd Build && tar jxf ../SourcePackages/ffmpeg-$(FFMPEG_VERSION).tar.bz2 && mv ffmpeg-$(FFMPEG_VERSION) ffmpeg-$(FFMPEG_VERSION)-$$ARCH ) ; \
	done
	touch $@

build_teem: build_zlib Build/.teem.build

configure_teem: Build/.teem.configure

patch_teem: Build/.teem.patch

unpack_teem: Build/.teem.unpack

Build/.teem.build: Build/.teem.configure
ifeq ($(BUILD_PLATFORM),Darwin)
	for ARCH in $(ARCHS); do \
		if [ $$ARCH = "x86_64" ]; then \
                        CFLAGS="-m64 -DTEEM_32BIT=0 -UTEEM_BZIP2"; \
		else \
                        CFLAGS="-m32 -DTEEM_32BIT=1 -UTEEM_BZIP2"; \
		fi; \
		( mkdir -p Build/teem-$(TEEM_VERSION)-$$ARCH/arch/$$TEEM_ARCH/obj ); \
		( mkdir -p Build/teem-$(TEEM_VERSION)-$$ARCH/arch/$$TEEM_ARCH/lib ); \
		( mkdir -p Build/teem-$(TEEM_VERSION)-$$ARCH/arch/$$TEEM_ARCH/bin ); \
		( cd Build/teem-$(TEEM_VERSION)-$$ARCH && cmake . -DCMAKE_C_FLAGS="-fPIC $$CFLAGS -I$$(pwd)/../../include/zlib -I$$(pwd)/../../include/libpng/libpng" && make VERBOSE=1 ); \
	done
else ifeq ($(BUILD_PLATFORM),Linux)
	for ARCH in $(ARCHS); do \
		if [ $$ARCH = "x86_64" ]; then \
			TEEM_ARCH="linux.amd64"; \
		else \
			TEEM_ARCH="linux.32"; \
		fi; \
		( mkdir -p Build/teem-$(TEEM_VERSION)-$$ARCH/arch/$$TEEM_ARCH/obj ); \
		( mkdir -p Build/teem-$(TEEM_VERSION)-$$ARCH/arch/$$TEEM_ARCH/lib ); \
		( mkdir -p Build/teem-$(TEEM_VERSION)-$$ARCH/arch/$$TEEM_ARCH/bin ); \
		( cd Build/teem-$(TEEM_VERSION)-$$ARCH && cmake . -DTEEM_ARCH=$$TEEM_ARCH -DCMAKE_C_FLAGS="-fPIC" && make -e TEEM_ARCH=$$TEEM_ARCH ); \
	done
endif
	touch $@

Build/.teem.configure: Build/.teem.patch
	for ARCH in $(ARCHS); do \
		( cd Build/teem-$(TEEM_VERSION)-$$ARCH && patch -p1 <../../Patches/teem-libpng-set-enable-gray.diff ); \
	done
	touch $@

Build/.teem.patch: Build/.teem.unpack
	for ARCH in $(ARCHS); do \
		( cd Build/teem-$(TEEM_VERSION)-$$ARCH && patch -p1 <../../Patches/teem_cmakelists.diff ); \
	done
	touch $@

Build/.teem.unpack:
	mkdir -p Build
	for ARCH in $(ARCHS); do \
		( cd Build && tar jxf ../SourcePackages/teem-$(TEEM_VERSION).tar.bz2 && mv teem-$(TEEM_VERSION) teem-$(TEEM_VERSION)-$$ARCH ) ; \
		( cd Build && chmod -R 777 teem-$(TEEM_VERSION)-$$ARCH ) ; \
	done
	touch $@

build_zlib: Build/.zlib.build

configure_zlib: Build/.zlib.configure

patch_zlib: Build/.zlib.patch

unpack_zlib: Build/.zlib.unpack

Build/.zlib.build: Build/.zlib.configure
	mkdir -p Build
	touch $@

Build/.zlib.configure: Build/.zlib.patch
	mkdir -p Build
	touch $@

Build/.zlib.patch: Build/.zlib.unpack
	mkdir -p Build
	touch $@

Build/.zlib.unpack:
	mkdir -p Build
	touch $@
  
install: check_env_vars install_ilmbase install_openexr install_llvm install_libpng install_ffmpeg install_teem install_node install_libedit install_gtkglext install_jpeg install_tiff

###

install_jpeg: install_jpeg_headers install_jpeg_libraries

install_jpeg_headers:
	for ARCH in $(ARCHS); do \
          mkdir -p $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/include/jpeg/jpeg; \
          cp Build/jpeg-$(JPEG_VERSION)-$(FIRST_ARCH)/*.h $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/include/jpeg/jpeg/; \
        done

install_jpeg_libraries:
	for ARCH in $(ARCHS); do \
          mkdir -p $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/lib/jpeg/; \
          cp Build/jpeg-$(JPEG_VERSION)-$$ARCH/.libs/*.a $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/lib/jpeg/; \
        done

###

install_tiff: install_tiff_headers install_tiff_libraries

install_tiff_headers:
	for ARCH in $(ARCHS); do \
          mkdir -p $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/include/tiff/libtiff; \
          cp Build/tiff-$(TIFF_VERSION)-$(FIRST_ARCH)/libtiff/*.h $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/include/tiff/libtiff/; \
        done

install_tiff_libraries:
	for ARCH in $(ARCHS); do \
          mkdir -p $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/lib/tiff/; \
          cp Build/tiff-$(TIFF_VERSION)-$$ARCH/libtiff/.libs/*.a $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/lib/tiff/; \
        done

install_ilmbase: install_ilmbase_headers install_ilmbase_libraries

install_ilmbase_headers:
	mkdir -p $(TARGET_BASE)/include/ilmbase
	cp Build/ilmbase-$(ILMBASE_VERSION)-$(FIRST_ARCH)/Iex/*.h $(TARGET_BASE)/include/ilmbase/
	cp Build/ilmbase-$(ILMBASE_VERSION)-$(FIRST_ARCH)/Imath/*.h $(TARGET_BASE)/include/ilmbase/
	cp Build/ilmbase-$(ILMBASE_VERSION)-$(FIRST_ARCH)/IlmThread/*.h $(TARGET_BASE)/include/ilmbase/
	cp Build/ilmbase-$(ILMBASE_VERSION)-$(FIRST_ARCH)/Half/*.h $(TARGET_BASE)/include/ilmbase/
	for ARCH in $(ARCHS); do \
          mkdir -p $(TARGET_BASE)/include/$(BUILD_PLATFORM)/$$ARCH/ilmbase; \
          cp Build/ilmbase-$(ILMBASE_VERSION)-$(FIRST_ARCH)/config/*.h $(TARGET_BASE)/include/$(BUILD_PLATFORM)/$$ARCH/ilmbase/; \
        done

install_ilmbase_libraries:
	for ARCH in $(ARCHS); do \
          mkdir -p $(TARGET_BASE)/lib/$(BUILD_PLATFORM)/$$ARCH/Release/ilmbase; \
          cp Build/ilmbase-$(ILMBASE_VERSION)-$$ARCH/Iex/.libs/*.a $(TARGET_BASE)/lib/$(BUILD_PLATFORM)/$$ARCH/Release/ilmbase/; \
          cp Build/ilmbase-$(ILMBASE_VERSION)-$$ARCH/Imath/.libs/*.a $(TARGET_BASE)/lib/$(BUILD_PLATFORM)/$$ARCH/Release/ilmbase/; \
          cp Build/ilmbase-$(ILMBASE_VERSION)-$$ARCH/IlmThread/.libs/*.a $(TARGET_BASE)/lib/$(BUILD_PLATFORM)/$$ARCH/Release/ilmbase/; \
          cp Build/ilmbase-$(ILMBASE_VERSION)-$$ARCH/Half/.libs/*.a $(TARGET_BASE)/lib/$(BUILD_PLATFORM)/$$ARCH/Release/ilmbase/; \
        done

install_openexr: install_openexr_headers install_openexr_libraries

install_openexr_headers:
	mkdir -p $(TARGET_BASE)/include/openexr
	cp Build/openexr-$(OPENEXR_VERSION)-$(FIRST_ARCH)/IlmImf/*.h $(TARGET_BASE)/include/openexr/
	#for ARCH in $(ARCHS); do \
        #  mkdir -p $(TARGET_BASE)/include/$(BUILD_PLATFORM)/$$ARCH/openexr; \
        #  cp Build/openexr-$(OPENEXR_VERSION)-$(FIRST_ARCH)/config/*.h $(TARGET_BASE)/include/$(BUILD_PLATFORM)/$$ARCH/openexr/; \
        #done

install_openexr_libraries:
	for ARCH in $(ARCHS); do \
          mkdir -p $(TARGET_BASE)/lib/$(BUILD_PLATFORM)/$$ARCH/Release/openexr; \
          cp Build/openexr-$(OPENEXR_VERSION)-$$ARCH/IlmImf/.libs/*.a $(TARGET_BASE)/lib/$(BUILD_PLATFORM)/$$ARCH/Release/openexr/; \
        done

install_llvm: install_llvm_headers install_llvm_libraries

install_llvm_headers:
	mkdir -p $(TARGET_BASE)/include/llvm/llvm; \
        $(RSYNC) --progress --recursive --delete --prune-empty-dirs --exclude='llvm/Config/' --exclude='llvm/Support/DataTypes.h' --include='*/' --include='*.h' --include='*.def' --include='*.inc' --exclude='*' Build/llvm-$(LLVM_VERSION)-$(FIRST_ARCH)/include/llvm $(TARGET_BASE)/include/llvm/; \
	for ARCH in $(ARCHS); do \
          mkdir -p $(TARGET_BASE)/include/$(BUILD_PLATFORM)/$$ARCH/llvm/llvm; \
          $(RSYNC) --progress --recursive --delete --prune-empty-dirs --include='llvm/Config/*.h' --include='llvm/Config/*.def' --include='llvm/Support/DataTypes.h' --include='*/' --include='*.gen' --exclude='*' Build/llvm-$(LLVM_VERSION)-$$ARCH/include/llvm $(TARGET_BASE)/include/$(BUILD_PLATFORM)/$$ARCH/llvm; \
	done

install_llvm_libraries:
	for ARCH in $(ARCHS); do \
          mkdir -p $(TARGET_BASE)/lib/$(BUILD_PLATFORM)/$$ARCH/Release/llvm; \
          $(RSYNC) --progress --recursive --delete --include='*/' --include='libLLVM*.a' --exclude='*' Build/llvm-$(LLVM_VERSION)-$$ARCH/Release/lib/ $(TARGET_BASE)/lib/$(BUILD_PLATFORM)/$$ARCH/Release/llvm/; \
        done

# [pzion 20111007] Note that the "install_node_lib" target is intentionally left
# out here; we don't V8 for the Node.js add-on
install_node: install_node_inc install_node_bin

install_node_inc:
	mkdir -p include/node/node
	cp Build/node-$(NODE_VERSION)-$(FIRST_ARCH)/src/*.h $(TARGET_BASE)/include/node/node
	mkdir -p include/node/uv
	cp Build/node-$(NODE_VERSION)-$(FIRST_ARCH)/deps/uv/include/*.h $(TARGET_BASE)/include/node/uv
	mkdir -p include/node/uv/uv-private
	cp Build/node-$(NODE_VERSION)-$(FIRST_ARCH)/deps/uv/include/uv-private/*.h $(TARGET_BASE)/include/node/uv/uv-private
	mkdir -p include/node/v8
	$(RSYNC) --progress --recursive --delete --include='*/' --include='*.h' --exclude='*' Build/node-$(NODE_VERSION)-$(FIRST_ARCH)/deps/v8/include/ $(TARGET_BASE)/include/node/v8/

install_node_bin:
	for ARCH in $(ARCHS); do \
          mkdir -p $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/bin; \
          cp Build/node-$(NODE_VERSION)-$$ARCH/out/Release/node $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/bin/; \
        done
	
install_node_lib:
	for ARCH in $(ARCHS); do \
          mkdir -p $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/lib/v8; \
          cp Build/node-$(NODE_VERSION)-$$ARCH/out/Release/libv8.a $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/lib/v8/; \
        done
	
install_boost: install_boost_headers install_boost_libraries

install_boost_headers:
	mkdir -p include/boost/boost
	$(RSYNC) --progress --recursive --delete --exclude='._*' Build/boost-$(BOOST_VERSION)-$(FIRST_ARCH)/boost/ $(TARGET_BASE)/include/boost/boost/

install_boost_libraries:
	for ARCH in $(ARCHS); do \
		mkdir -p $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/lib/boost; \
		cp Build/boost-$(BOOST_VERSION)-$$ARCH/stage/lib/*.a $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/lib/boost/; \
	done


install_hdf5: install_hdf5_headers install_hdf5_libraries

install_hdf5_headers:
	mkdir -p include/hdf5/hdf5
	$(RSYNC) --progress --recursive --include='*/' --exclude '*private.h' --exclude 'H5config.h' --exclude 'H5pubconf.h' --include='*.h' --exclude='*' Build/hdf5-$(HDF5_VERSION)-$(FIRST_ARCH)/src/ $(TARGET_BASE)/include/hdf5/hdf5/
	$(RSYNC) --progress --recursive --include='*/' --exclude '*private.h' --include='*.h' --exclude='*' Build/hdf5-$(HDF5_VERSION)-$(FIRST_ARCH)/hl/src/ $(TARGET_BASE)/include/hdf5/hdf5/
	for ARCH in $(ARCHS); do \
          mkdir -p $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/include/hdf5/hdf5/; \
          $(RSYNC) --progress --recursive --include='*/' --include='H5config.h' --include 'H5pubconf.h' --exclude='*' Build/hdf5-$(HDF5_VERSION)-$(FIRST_ARCH)/src/ $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/include/hdf5/hdf5/; \
        done

install_hdf5_libraries:
	for ARCH in $(ARCHS); do \
		mkdir -p $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/lib/hdf5; \
		cp Build/hdf5-$(HDF5_VERSION)-$$ARCH/src/.libs/lib*.a $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/lib/hdf5/; \
		cp Build/hdf5-$(HDF5_VERSION)-$$ARCH/hl/src/.libs/lib*.a $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/lib/hdf5/; \
	done
	


install_alembic: install_alembic_headers install_alembic_libraries

install_alembic_headers:
	mkdir -p include/alembic
	$(RSYNC) --progress --recursive --include='*/' --exclude '*private.h' --include='*.h' --exclude='*' Build/alembic-$(ALEMBIC_VERSION)-$(FIRST_ARCH)/lib/Alembic $(TARGET_BASE)/include/alembic/

install_alembic_libraries:
	for ARCH in $(ARCHS); do \
		mkdir -p $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/lib/alembic; \
		cp Build/alembic-$(ALEMBIC_VERSION)-$$ARCH/alembic_build/lib/Alembic/Abc/lib*.a $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/lib/alembic/; \
		cp Build/alembic-$(ALEMBIC_VERSION)-$$ARCH/alembic_build/lib/Alembic/AbcCoreAbstract/lib*.a $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/lib/alembic/; \
		cp Build/alembic-$(ALEMBIC_VERSION)-$$ARCH/alembic_build/lib/Alembic/AbcCoreHDF5/lib*.a $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/lib/alembic/; \
		cp Build/alembic-$(ALEMBIC_VERSION)-$$ARCH/alembic_build/lib/Alembic/AbcGeom/lib*.a $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/lib/alembic/; \
		cp Build/alembic-$(ALEMBIC_VERSION)-$$ARCH/alembic_build/lib/Alembic/Util/lib*.a $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/lib/alembic/; \
	done
	

install_libpng: install_zlib
	for ARCH in $(ARCHS); do \
		mkdir -p $(TARGET_BASE)/lib/$(BUILD_PLATFORM)/$$ARCH/Release/libpng && cp Build/libpng-$(PNG_VERSION)-$$ARCH/.libs/libpng14.a $(TARGET_BASE)/lib/$(BUILD_PLATFORM)/$$ARCH/Release/libpng/; \
	done
	mkdir -p $(TARGET_BASE)/include/libpng/libpng
	cp Build/libpng-$(PNG_VERSION)-$(FIRST_ARCH)/pngconf.h $(TARGET_BASE)/include/libpng/libpng/
	cp Build/libpng-$(PNG_VERSION)-$(FIRST_ARCH)/png.h $(TARGET_BASE)/include/libpng/libpng/
	
install_ffmpeg: install_ffmpeg_headers install_ffmpeg_libs
	
install_ffmpeg_libs:
	for ARCH in $(ARCHS); do \
		mkdir -p $(TARGET_BASE)/lib/$(BUILD_PLATFORM)/$$ARCH/Release/ffmpeg; \
		for l in libavcodec libavdevice libavformat libavutil libswscale; do \
		 	cp Build/ffmpeg-$(FFMPEG_VERSION)-$$ARCH/$$l/$$l.a $(TARGET_BASE)/lib/$(BUILD_PLATFORM)/$$ARCH/Release/ffmpeg/; \
		done; \
	done

install_ffmpeg_headers:
	mkdir -p $(TARGET_BASE)/include/ffmpeg/
	for l in libavcodec libavdevice libavformat libavutil libswscale; do \
		mkdir -p $(TARGET_BASE)/include/ffmpeg/$$l; \
		cp Build/ffmpeg-$(FFMPEG_VERSION)-$(FIRST_ARCH)/$$l/*.h $(TARGET_BASE)/include/ffmpeg/$$l/; \
	done
	
install_teem: install_teem_headers install_teem_libs
	
install_teem_libs:
ifeq ($(BUILD_PLATFORM),Darwin)
	for ARCH in $(ARCHS); do \
		mkdir -p $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/lib/teem; \
		cp Build/teem-$(TEEM_VERSION)-$$ARCH/bin/libteem.a $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/lib/teem/; \
	done
else ifeq ($(BUILD_PLATFORM),Linux)
	for ARCH in $(ARCHS); do \
		mkdir -p $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/lib/teem; \
		if [ $$ARCH = "x86_64" ]; then \
			TEEM_ARCH="linux.amd64"; \
		else \
			TEEM_ARCH="linux.32"; \
		fi; \
		cp Build/teem-$(TEEM_VERSION)-$$ARCH/bin/libteem.a $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/lib/teem/; \
	done
endif

install_teem_headers:
	mkdir -p $(TARGET_BASE)/include/teem
	mkdir -p $(TARGET_BASE)/include/teem/teem
	cp Build/teem-$(TEEM_VERSION)-$(FIRST_ARCH)/include/teem/*.h $(TARGET_BASE)/include/teem/teem/;

install_libedit:
	for ARCH in $(ARCHS); do \
		mkdir -p $(TARGET_BASE)/lib/$(BUILD_PLATFORM)/$$ARCH/libedit && cp Build/libedit-$(LIBEDIT_VERSION)-$$ARCH/src/.libs/libedit.a $(TARGET_BASE)/lib/$(BUILD_PLATFORM)/$$ARCH/libedit/; \
	done
	mkdir -p $(TARGET_BASE)/include/libedit/libedit
	cp Build/libedit-$(LIBEDIT_VERSION)-$(FIRST_ARCH)/src/histedit.h $(TARGET_BASE)/include/libedit/libedit/

install_gtkglext:
	for ARCH in $(ARCHS); do \
		mkdir -p $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/lib/gtkglext \
                  && cp Build/gtkglext-$(GTKGLEXT_VERSION)-$$ARCH/gdk/.libs/libgdkglext-x11-1.0.a $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/lib/gtkglext/ \
                  && cp Build/gtkglext-$(GTKGLEXT_VERSION)-$$ARCH/gtk/.libs/libgtkglext-x11-1.0.a $(TARGET_BASE)/$(BUILD_PLATFORM)/$$ARCH/Release/lib/gtkglext/; \
	done
	mkdir -p $(TARGET_BASE)/$(BUILD_PLATFORM)/$(FIRST_ARCH)/Release/include/gtkglext
	cp Build/gtkglext-$(GTKGLEXT_VERSION)-$(FIRST_ARCH)/gdk/gdkglext-config.h $(TARGET_BASE)/$(BUILD_PLATFORM)/$(FIRST_ARCH)/Release/include/gtkglext/
	mkdir -p $(TARGET_BASE)/$(BUILD_PLATFORM)/$(FIRST_ARCH)/Release/include/gtkglext/gdk
	cp Build/gtkglext-$(GTKGLEXT_VERSION)-$(FIRST_ARCH)/gdk/*.h $(TARGET_BASE)/$(BUILD_PLATFORM)/$(FIRST_ARCH)/Release/include/gtkglext/gdk/
	mkdir -p $(TARGET_BASE)/$(BUILD_PLATFORM)/$(FIRST_ARCH)/Release/include/gtkglext/gtk
	cp Build/gtkglext-$(GTKGLEXT_VERSION)-$(FIRST_ARCH)/gtk/*.h $(TARGET_BASE)/$(BUILD_PLATFORM)/$(FIRST_ARCH)/Release/include/gtkglext/gtk/

install_zlib:
ifeq ($(BUILD_PLATFORM),"mingw")
	for ARCH in $(ARCHS); do \
		mkdir -p $(TARGET_BASE)/lib/$(BUILD_PLATFORM)/$$ARCH/zlib && cp Build/zlib-$(ZLIB_VERSION)-$$ARCH/libz.a $(TARGET_BASE)/lib/$(BUILD_PLATFORM)/$$ARCH/zlib/; \
	done
	mkdir -p $(TARGET_BASE)/include/zlib/zlib
	cp Build/zlib-$(ZLIB_VERSION)-$(FIRST_ARCH)/zconf.h $(TARGET_BASE)/include/zlib/zlib/
	cp Build/zlib-$(ZLIB_VERSION)-$(FIRST_ARCH)/zlib.h $(TARGET_BASE)/include/zlib/zlib/
endif

clean: clean_ilmbase clean_openexr clean_llvm clean_libpng clean_ffmpeg clean_teem clean_node clean_libedit clean_gtkglext clean_boost clean_hdf5 clean_alembic clean_jpeg clean_tiff clean_opencv
	rm -rf Build

clean_ilmbase:
	for ARCH in $(ARCHS); do \
		rm -rf Build/ilmbase-$(ILMBASE_VERSION)-$$ARCH; \
	done
	rm -rf Build/.ilmbase.*

clean_openexr:
	for ARCH in $(ARCHS); do \
		rm -rf Build/openexr-$(OPENEXR_VERSION)-$$ARCH; \
	done
	rm -rf Build/.openexr.*

clean_llvm:
	for ARCH in $(ARCHS); do \
		rm -rf Build/llvm-$(LLVM_VERSION)-$$ARCH; \
	done
	rm -rf Build/.llvm.*

clean_node:
	for ARCH in $(ARCHS); do \
		rm -rf Build/node-$(NODE_VERSION)-$$ARCH; \
	done
	rm -rf Build/.node.*

clean_ffmpeg:
	for ARCH in $(ARCHS); do \
		rm -rf Build/ffmpeg-$(FFMPEG_VERSION)-$$ARCH; \
	done
	rm -rf Build/.ffmpeg.*

clean_libpng:
	for ARCH in $(ARCHS); do \
		rm -rf Build/libpng-$(PNG_VERSION)-$$ARCH; \
	done
	rm -rf Build/.libpng.*

clean_teem:
	for ARCH in $(ARCHS); do \
		rm -rf Build/teem-$(TEEM_VERSION)-$$ARCH; \
	done
	rm -rf Build/.teem.*

clean_jpeg:
	for ARCH in $(ARCHS); do \
		rm -rf Build/jpeg-$(TIFF_VERSION)-$$ARCH; \
	done
	rm -rf Build/.jpeg.*

clean_tiff:
	for ARCH in $(ARCHS); do \
		rm -rf Build/tiff-$(TIFF_VERSION)-$$ARCH; \
	done
	rm -rf Build/.tiff.*

clean_libedit:
	for ARCH in $(ARCHS); do \
		rm -rf Build/libedit-$(LIBEDIT_VERSION)-$$ARCH; \
	done
	rm -rf Build/.libedit.*

clean_gtkglext:
	for ARCH in $(ARCHS); do \
		rm -rf Build/gtkglext-$(GTKGLEXT_VERSION)-$$ARCH; \
	done
	rm -rf Build/.gtkglext.*

clean_boost:
	for ARCH in $(ARCHS); do \
		rm -rf Build/boost-$(BOOST_VERSION)-$$ARCH; \
	done
	rm -rf Build/.boost.*

clean_hdf5:
	for ARCH in $(ARCHS); do \
		rm -rf Build/hdf5-$(HDF5_VERSION)-$$ARCH; \
	done
	rm -rf Build/.hdf5.*

clean_alembic:
	for ARCH in $(ARCHS); do \
		rm -rf Build/alembic-$(ALEMBIC_VERSION)-$$ARCH; \
	done
	rm -rf Build/.alembic.*

clean_zlib:
ifeq ($(PLATFORM),"mingw")
	for ARCH in $(ARCHS); do \
		rm -rf Build/zlib-$(ZLIB_VERSION)-$$ARCH; \
	done
endif
	rm -rf Build/.zlib.*
