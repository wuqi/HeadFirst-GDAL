.. highlight:: rst

################
GDAL3编译
################

本篇适用于GDAL3.3的静态编译,环境请参照静态编译,此处就不重复了,此处只编了一些自用的库,如果有啥其他的,可以在issue里提,libpq之类的可以直接用编好的,更方便

******************
依赖库:
******************

-  PROJ > 6

   -  SQLite3
   -  libtiff

      -  libzip(option)

   -  curl

-  LIBICONV
-  libtiff >= 4.0(option)
-  Expat
-  HDF4
-  HDF5
-  PostgreSQL
-  SQLite Libraries
-  libspatialite

   -  GEOS
   -  freexl
   -  proj
   -  sqlite3
   -  zlib
   -  iconv
   -  libxml2
   -  librttopo
   -  libcurl
   -  libtiff

-  PCRE
-  NETCDF4
-  curl
-  geos
-  Podofo

   -  freetype
   -  tiff
   -  jpeg

-  LZMA_CFLAGS
-  ZSTD_LIBS
-  TILEDB_LIBS
-  RDB_LIB
-  WEBP
-  libxml2

   -  libiconv

-  FREEXL

   -  libiconv

-  heif

   -  libde265

******************
编译顺序:
******************

`libde265 <https://github.com/strukturag/libheif>`__
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

解压后,按照\ `Fix CMake install issue
(#278) <https://hub.fastgit.org/strukturag/libde265/pull/280>`__
修改cmake文件,然后编译

如果需要静态库,修改CMakeLists.txt里的_BUILD_SHARED_LIBS_参数即可

.. code:: bash

       cd libde265-1.0.8
       mkdir build
       cd build
       cmake -G "Visual Studio 15 2017 Win64" .. -DCMAKE_INSTALL_PREFIX=e:/GDAL_BUILD/libde265_1.0.8_x64
       cmake --build . --config Release --target install

`heif <https://github.com/strukturag/libde265>`__
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

依赖\ **libde265** ,解压后

如果需要静态库,修改CMakeLists.txt里的_BUILD_SHARED_LIBS_参数即可

.. code:: bash

       cd libheif-1.12.0
       mkdir build
       cd build
       cmake -G "Visual Studio 15 2017 Win64" ..  -DCMAKE_INSTALL_PREFIX=e:/GDAL_BUILD/libheif_1_12_0_x64 -DLIBDE265_FOUND=ON -DLIBDE265_INCLUDE_DIR=e:/GDAL_BUILD/libde265_1.0.8_x64/include -DLIBDE265_LIBRARIES=e:/GDAL_BUILD/libde265_1.0.8_x64/lib/libde265.lib
       cmake --build . --config Release --target install

`curl <https://github.com/curl/curl/releases>`__
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

libcurl更新比较快,1-2月更新一个版本,请选择最新的版本,可选 ``openssl`` 和
``zlib`` 等库,自己参考winbuild下readme文档,最简单的编译方式如下:

.. code:: bash

       cd winbuild
       nmake /f Makefile.vc mode=static

生成文件将在 ``builds`` 文件夹下.需要自己拷贝出来,

在curl.h第一行添加

``define CURL_STATICLIB``

`libTiff <http://download.osgeo.org/libtiff/>`__
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

cmake,依然是修改CmakeList编译静态库

.. code:: bash

   cd build
   cmake -G "Visual Studio 15 2017 Win64" .. -DCMAKE_INSTALL_PREFIX=e:/GDAL_BUILD/libtiff_4_3_0_x64
   cmake --build . --config Release --target install

`sqlite <https://www.sqlite.org/index.html>`__
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

`sqlite.cmake.build <https://github.com/snikulov/sqlite.cmake.build/tree/master/src>`__\ 中的cmake文件可用

.. code:: bash

   cd build
   cmake -G "Visual Studio 15 2017 Win64" .. -DCMAKE_INSTALL_PREFIX=e:/GDAL_BUILD/libsqlite3_36_x64
   cmake --build . --config Release --target install

`PROJ <https://proj.org/install.html#compilation-and-installation-from-source-code>`__
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

cmake编译

.. code:: bash

       cd build
       cmake -G "Visual Studio 15 2017 Win64" .. -DCMAKE_INSTALL_PREFIX=e:/GDAL_BUILD/proj8_1_0_x64 -DBUILD_SHARED_LIBS=OFF -DEXE_SQLITE3=E:\GDAL_BUILD\libsqlite3_36_x64\bin\shell.exe -DSQLITE3_INCLUDE_DIR=E:\GDAL_BUILD\libsqlite3_36_x64\include -DSQLITE3_LIBRARY=E:\GDAL_BUILD\libsqlite3_36_x64\lib\sqlite3-static.lib -DCURL_INCLUDE_DIR=E:\GDAL_BUILD\libcurl-7.77.0-x64\include -DCURL_LIBRARY="E:\GDAL_BUILD\libcurl-7.77.0-x64\lib\libcurl_a.lib;wsock32.lib;wldap32.lib;winmm.lib" -DTIFF_INCLUDE_DIR=E:\GDAL_BUILD\libtiff_4_3_0_x64\include -DTIFF_LIBRARY_RELEASE=E:\GDAL_BUILD\libtiff_4_3_0_x64\lib\tiff.lib -DBUILD_TESTING=OFF

       cmake --build . --config Release --target install

`libiconv-win-build: <https://github.com/kiyolee/libiconv-win-build>`__
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

win下编译1.16版本的iconv,直接打开2017工程编译就行

`libExpat <https://github.com/libexpat/libexpat/releases>`__
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

下载源码后，修改 ``/lib/expat.h`` 文件,在文件首加上：

``define XML_STATIC``

打开命令行,使用cmake

.. code:: bash

       cd build

       cmake -G "Visual Studio 15 2017 Win64" .. -DCMAKE_INSTALL_PREFIX=e:/GDAL_BUILD/liexpat_2_4_1_x64 -DBUILD_SHARED_LIBS=OFF

       cmake --build . --config Release --target install

`HDF4 <https://support.hdfgroup.org/release4/cmakebuild.html>`__
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

尽量使用4.2.12 之后版本,方便编译

直接下载cmake版本,注意是win还是其他,win下载zip压缩,其他下载tar.gz

最新是\ `4.2.15 <https://portal.hdfgroup.org/display/support/HDF+4.2.15#files>`__\ 版本,,下载\ `CMake-hdf-4.2.15.zip <https://support.hdfgroup.org/ftp/HDF/releases/HDF4.2.15/src/CMake-hdf-4.2.15.zip>`__

修改其中的 HDF4options.cmake 文件,文件末尾添加

``set(ADD_BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -DHDF4_ENABLE_NETCDF:BOOL=OFF")``

防止与 netcdf 库冲突

同时取消注释,编译静态库

``set (ADD_BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -DBUILD_SHARED_LIBS:BOOL=OFF")``

下载完成后,根据Visual Studio版本,运行相应的 ``build-VS20xx-32.bat`` 或者
``build-VS20xx-64.bat`` 文件,会自动新建build文件夹,最终在 build
文件夹下生成zip文件.

老版本或者新版本的Visual Studio可直接编辑 HDF4config.cmake
文件,仿照其他bat文件写脚本.

`HDF5 <https://www.hdfgroup.org/downloads/hdf5/source-code/>`__
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

基本与hdf4一样,下载\ `cmake <https://www.hdfgroup.org/package/cmake-hdf5-1-12-0-zip/?wpdmdl=14583&refresh=60e79b3625edd1625791286>`__\ 版本,修改HDF5options.cmake文件,取消注释,最后加上一行

.. code:: cmake

   set (ADD_BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -DBUILD_SHARED_LIBS:BOOL=OFF")
   #add# at last
   set (ADD_BUILD_OPTIONS ${ADD_BUILD_OPTIONS} -DDEFAULT_API_VERSION:STRING=v18")

即静态编译后,双击bat即可

`NetCDF <https://hub.fastgit.org/Unidata/netcdf-c>`__
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

注意:

1. hdf5需要加DEAFAULT_API_VESION=v18

2. 改cmakelist.txt文件 : `Use of unset variable HDF5_VERSION · Issue
   #1962 · Unidata/netcdf-c · GitHub
   (fastgit.org) <https://hub.fastgit.org/Unidata/netcdf-c/issues/1962>`__

3. 在utf8环境下编译

4. 屏蔽掉\ ``netcdf-c-4.8.0\nc_test4\tst_udf.c``\ 中的19-26行

   .. code:: cpp

      /*
      #if# defined(_WIN32) || defined(_WIN64)
      int
      NC4_show_metadata(int ncid)
      {
      return 0;
      }
      #endif#
      */

编译

.. code:: bash

       chcp 65001
       mkdir build
       cd build
       cmake  -G "Visual Studio 15 2017 Win64" .. -DCMAKE_INSTALL_PREFIX=e:/GDAL_BUILD/netcdf_4_8_0_x64 -DBUILD_SHARED_LIBS=OFF  -DUSE_SZIP=ON -DUSE_HDF5=ON -DENABLE_DAP=ON -D"SZIP=E:/GDAL_BUILD/HDF5-1.12.0-win64/lib/libszip.lib” -D"ZLIB_INCLUDE_DIR=E:/GDAL_BUILD/SOURCE/CMake-hdf5-1.12.0/build/ZLIB-prefix/src/ZLIB" -D"ZLIB_LIBRARY=E:/GDAL_BUILD/HDF5-1.12.0-win64/lib/libzlib.lib"  -DENABLE_NETCDF_4=ON -D"CURL_LIBRARY=E:/GDAL_BUILD/libcurl-7.77.0-x64/lib/libcurl_a.lib;wsock32.lib;wldap32.lib;winmm.lib;crypt32.lib;Ws2_32.lib;Normaliz.lib"  -D"CURL_INCLUDE_DIR=E:/GDAL_BUILD/libcurl-7.77.0-x64/include"  -D"HAVE_HDF5_H=E:/GDAL_BUILD/HDF5-1.12.0-win64/include"  -D"HDF5_INCLUDE_DIR=E:/GDAL_BUILD/HDF5-1.12.0-win64/include"  -D"HDF5_C_LIBRARY=E:/GDAL_BUILD/HDF5-1.12.0-win64/lib/libhdf5.lib"  -D"HDF5_HL_LIBRARY=E:/GDAL_BUILD/HDF5-1.12.0-win64/lib/libhdf5_hl.lib" -DBUILD_TESTING=OFF

       cmake --build . --config Release --target install

`GEOS <https://git.osgeo.org/gitea/geos/geos>`__
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

新版本的 ``geos`` 直接采用 ``cmake`` 可以生成静态库和动态库。

.. code:: bash

       cd build

       cmake -G "Visual Studio 15 2017 Win64" .. -DCMAKE_INSTALL_PREFIX=e:/GDAL_BUILD/GEOS_3_9_1_x64 -DBUILD_SHARED_LIBS=OFF

       cmake --build . --config Release --target install

`podofo <http://podofo.sourceforge.net/>`__
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code:: bash

       del cmakecache.txt
       set FTDIR=E:/GDAL_BUILD/freetype_2.10.4_x64
       set FTLIBDIR=E:/GDAL_BUILD/freetype_2.10.4_x64/release static/win64
       set ZLIBDIR=E:/GDAL_BUILD/HDF5-1.12.0-win64
       set TIFDIR=E:/GDAL_BUILD/libtiff_4_3_0_x64

       cmake -G "Visual Studio 15 2017 Win64" .. -DCMAKE_INCLUDE_PATH="%FTDIR%/include;%JPEGDIR%/include;%ZLIBDIR%/include;%TIFDIR%/include" -DCMAKE_LIBRARY_PATH="%FTLIBDIR%;%FTDIR%/lib;%ZLIBDIR%/lib;%TIFDIR%/lib" -DPODOFO_BUILD_SHARED:BOOL=FALSE -DFREETYPE_LIBRARY_NAMES=freetype -DZLIB_LIBRARY_NAMES=libzlib -DTIFF_LIBRARY_NAMES=libTiff -DZLIB_INCLUDE_DIR=%ZLIBDIR%/include -DCMAKE_INSTALL_PREFIX=e:/GDAL_BUILD/podofo_0_9_7_x64

       cmake --build . --config Release --target install

`FreeType <https://sourceforge.net/projects/freetype/>`__
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

可以直接下载编译好的\ `2.10.4 <https://github.com/ubawurinna/freetype-windows-binaries>`__\ 版本,其中有动态版本和静态版本,没有尝试编译

`FreeXL <https://www.gaia-gis.it/fossil/freexl/index>`__
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. 修改nmake.opt中的输出路径
2. 修改makefile.vc中iconv库路径和头文件路径
   ``nmake /f makefile.vc install``

`LibXML2 <http://xmlsoft.org/>`__
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  进入 ``win32`` 文件夹下,运行,注意编译静态库需要 static=yes 和
   cruntime=/MT 同时设置

   .. code:: bash

      cscript configure.js  static=yes compiler=msvc prefix=E:\GDAL_BUILD\libxml2-2.9.12_x64 include=E:\GDAL_BUILD\iconv1.16\include lib=E:\GDAL_BUILD\iconv1.16\lib cruntime=/MT

      nmake /f Makefile.msvc
      nmake /f Makefile.msvc install

编译完成后,在头文件中定义 LIBXML_STATIC 宏

`Librttopo <https://git.osgeo.org/gitea/rttopo/librttopo>`__
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  修改 ``makefile.vc`` 中的geos头文件和lib文件路径

-  修改 ``nmake.opt`` 中的安装路径 INSTDIR

-  下载 `#25 - Handle missing 2 header files when compiling on
   Windows  <https://git.osgeo.org/gitea/rttopo/librttopo/issues/25>`__
   中的两个文件

-  在makefile.vc中第20行添加 ``src\rtt_tpsnap.obj``
    下面编译spatialite需要

-  在vs命令行中编译

   .. code:: bash

      nmake /f Makefile.vc
      nmake /f Makefile.vc install

zlib
^^^^

cmake-hdf5里有压缩文件夹,可以直接解压编译,不需要重复下载

.. code:: bash

   mkdir build
       cd build
       cmake -G "Visual Studio 15 2017 Win64" .. -DCMAKE_INSTALL_PREFIX=e:/GDAL_BUILD/zlib_x64
       cmake --build . --config Release --target install

`libspatialite <https://www.gaia-gis.it/fossil/libspatialite/index>`__
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

修改 ``gg_utf8.c`` 文件中72行,注释掉

``extern const char *locale_charset (void);``

替换为

``include <localcharset.h>``

修改nmake.opt 中的输出路径

修改makefile64.vc中头文件
-I和lib的路径,所有路径都得改,主要是头文件和lib这两段,请按实际情况修改

.. code:: make

       CFLAGS = /nologo -I.\src\headers -I.\src\topology   -I.  -IE:\GDAL_BUILD\proj8_1_0_x64\include  -IE:\GDAL_BUILD\GEOS_3_9_1_x64\include  -IE:\GDAL_BUILD\iconv1.16\include -IE:\GDAL_BUILD\freeXL1_0_6_X64\include  -IE:\GDAL_BUILD\libcurl-7.77.0-x64\include  -IE:\GDAL_BUILD\libtiff_4_3_0_x64\include -IE:\GDAL_BUILD\libsqlite3_36_x64\include -IE:\GDAL_BUILD\zlib_x64\include -IE:\GDAL_BUILD\libxml2-2.9.12_x64\include\libxml2 -IE:\GDAL_BUILD\librttopo1_1_0_x64\include $(OPTFLAGS)

       ...

       spatialite_i.lib:     $(LIBOBJ)
           link /dll /out:$(SPATIALITE_DLL) \
           /implib:spatialite_i.lib $(LIBOBJ) \
           E:\GDAL_BUILD\proj8_1_0_x64\lib\proj.lib  E:\GDAL_BUILD\GEOS_3_9_1_x64\lib\geos_c.lib E:\GDAL_BUILD\GEOS_3_9_1_x64\lib\geos.lib\
           E:\GDAL_BUILD\freeXL1_0_6_X64\lib\freexl.lib E:\GDAL_BUILD\iconv1.16\lib\libiconv-static.lib \
           E:\GDAL_BUILD\libsqlite3_36_x64\lib\sqlite3-static.lib E:\GDAL_BUILD\zlib_x64\lib\libzlib.lib \
               E:\GDAL_BUILD\libxml2-2.9.12_x64\lib\libxml2_a.lib E:\GDAL_BUILD\librttopo1_1_0_x64\lib\librttopo.lib \
           E:\GDAL_BUILD\libtiff_4_3_0_x64\lib\tiff.lib E:\GDAL_BUILD\libtiff_4_3_0_x64\lib\tiffxx.lib \           E:\GDAL_BUILD\libcurl-7.77.0-x64\lib\libcurl_a.lib wsock32.lib shell32.lib wldap32.lib winmm.lib crypt32.lib Ws2_32.lib Normaliz.lib  Ole32.lib Advapi32.lib

然后

.. code:: bash

   nmake /f Makefile.vc
   nmake /f Makefile.vc install

拷贝sqlite3.h的头文件到include和include/spatialite 里备用

`PCRE <http://www.pcre.org/>`__
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

       cd build
       cmake -G "Visual Studio 15 2017 Win64" .. -DCMAKE_INSTALL_PREFIX=e:/GDAL_BUILD/PCRE_x64
       cmake --build . --config Release --target install

pcre2在gdal中是无效的,注意,需要pcre1,但是已经不怎么维护了

`GDAL <https://gdal.org/>`__
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

修改nmake.opt,添加依赖和修改编译环境,然后运行

注意:

1. 如果需要编HDF NETCDF等库的plugin模式,请编译动态库,不要静态库
2. 注意依赖关系,libspatalite库的其他依赖,hdf和netcdf的其他依赖需要带上
3. 注意头文件中静态宏的声明
4. 编译gdal静态库的话,修改nmake.opt 设置 ``DLLBUILD=0``
   ,静态库编译比较占空间,其实不太推荐,其他库都是静态,编出来gdal303.dll不到30M,lib
   2M,总体30M,静态编出来的话lib接近300M,编出来的工具大概就是30M,总体接近1G,不过好处是干净,看着没啥依赖,请按实际需求进行编译
5. 编译后,建议先设置PROJ_LIB环境变量,或者在代码中调用
   ``OSRSetPROJSearchPaths``
   设置proj.db的路径,gdal3中主要变化一个是编译都升级到c++11,另一个就是proj库升级到6以上,支持WKT2了,不加上的话会出现找不到proj.db的错误
