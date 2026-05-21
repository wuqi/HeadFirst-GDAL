.. highlight:: rst
.. _gdal3overview:

####################
GDAL 3.x 概述
####################

GDAL 3.0 是一次重大版本升级，于 2019 年发布，带来了多项底层架构变化。本章将介绍 GDAL 3.x 系列的核心变化、各版本亮点、新头文件体系、统一命令行工具以及编译系统变化，帮助读者从整体上把握 GDAL 3.x 的演进方向。

.. _gdal3-core-changes:

******************
GDAL 3.0 核心变化
******************

GDAL 3.0 相比 2.x 系列是一次跨越式的升级，主要体现在坐标参考系统处理方式的根本性改变。以下介绍三个最重要的变化。

PROJ 6+ 集成
================

GDAL 3.0 最重要的变化之一是将底层坐标转换库从 PROJ 4 升级到 PROJ 6+。PROJ 6 引入了全新的坐标转换管道机制，基于 `PROJ pipeline <https://proj.org/operations/index.html>`_ 的坐标转换方式，取代了旧版本中基于简单参数的转换方法。

这一变化带来的主要影响包括：

- 坐标转换精度显著提高，特别是涉及到基准面转换（datum transformation）的场景
- 支持更多坐标参考系统定义方式
- 需要 ``proj.db`` 数据库文件，运行时必须能访问到该文件
- 不再支持 PROJ 4 中的部分过时接口

如果运行时找不到 ``proj.db``，程序会报错。可以通过以下方式设置搜索路径：

.. code-block:: c++

    #include "ogr_spatialref.h"

    // 方式一：在代码中设置 PROJ 数据库搜索路径
    const char *const apszSearchPaths[] = { "/path/to/proj" };
    OSRSetPROJSearchPaths(apszSearchPaths);

    // 方式二：设置环境变量 PROJ_LIB 指向 proj.db 所在目录

也可以通过设置环境变量 ``PROJ_LIB`` 来指定路径：

.. code-block:: bash

    # Linux / macOS
    export PROJ_LIB=/usr/local/share/proj

    # Windows (PowerShell)
    $env:PROJ_LIB = "C:\OSGeo4W64\share\proj"

WKT2 支持
================

GDAL 3.0 开始全面支持 WKT2（Well Known Text 2）格式，即 ISO 19162 标准。相比传统的 WKT1（基于 OGC 01-009），WKT2 提供了更丰富的表达能力。

WKT2 与 WKT1 的主要区别：

- WKT2 使用 ``BOUNDCRS`` 来表达坐标参考系统之间的绑定关系，而非 WKT1 中隐含的 TOWGS84 节点
- WKT2 支持 ``ENSEMBLE`` 概念来描述大地基准面
- WKT2 中的坐标系轴顺序严格按照定义，不再像 WKT1 那样默认将经纬度翻转为经度-纬度顺序

.. code-block:: c++

    #include "ogr_spatialref.h"

    OGRSpatialReference oSRS;
    oSRS.importFromEPSG(4326);

    // 导出为 WKT2_2019 格式
    char *pszWKT2 = nullptr;
    oSRS.exportToWkt(&pszWKT2, OGRSpatialReference::WKT2_2019);
    // 输出类似：
    // GEOGCRS["WGS 84",
    //   DATUM["World Geodetic System 1984",
    //     ELLIPSOID["WGS 84",6378137,298.257223563,
    //       LENGTHUNIT["metre",1]]],
    //   PRIMEM["Greenwich",0,
    //     ANGLEUNIT["degree",0.0174532925199433]],
    //   CS[ellipsoidal,2],
    //     AXIS["geodetic latitude (Lat)",north,
    //       ORDER[1],
    //       ANGLEUNIT["degree",0.0174532925199433]],
    //     AXIS["geodetic longitude (Lon)",east,
    //       ORDER[2],
    //       ANGLEUNIT["degree",0.0174532925199433]],
    //   ID["EPSG",4326]]
    CPLFree(pszWKT2);

    // 也可以导出为传统 WKT1 格式
    char *pszWKT1 = nullptr;
    oSRS.exportToWkt(&pszWKT1, OGRSpatialReference::WKT1_GDAL);
    CPLFree(pszWKT1);

AxisMappingStrategy 引入
=========================

由于 WKT2 严格遵循轴顺序定义，EPSG:4326 在 WKT2 下纬度在前、经度在后，这与 GDAL 2.x 中默认的经度在前、纬度在后不同。为了处理这种差异，GDAL 3.0 引入了 ``AxisMappingStrategy`` 机制。

``AxisMappingStrategy`` 的常见设置：

.. code-block:: c++

    #include "ogr_spatialref.h"

    OGRSpatialReference oSRS;
    oSRS.importFromEPSG(4326);

    // OAMS_TRADITIONAL_GIS_ORDER：强制使用传统 GIS 顺序（经度, 纬度）
    // 这是 GDAL 2.x 的默认行为
    oSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

    // OAMS_AUTHORITY_COMPLIENT：严格按照坐标参考系统定义的轴顺序
    // 对于 EPSG:4326，即为（纬度, 经度）
    oSRS.SetAxisMappingStrategy(OAMS_AUTHORITY_COMPLIENT);

在使用坐标转换时，必须正确设置 ``AxisMappingStrategy``，否则坐标转换结果可能出错：

.. code-block:: c++

    #include "ogr_spatialref.h"

    OGRSpatialReference oSrcSRS, oDstSRS;
    oSrcSRS.importFromEPSG(4326);
    oSrcSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

    oDstSRS.importFromEPSG(32652);
    oDstSRS.SetAxisMappingStrategy(OAMS_TRADORITY_COMPLIENT);

    OGRCoordinateTransformation *poCT =
        OGRCreateCoordinateTransformation(&oSrcSRS, &oDstSRS);

    double x = 129.0;  // 经度
    double y = 36.0;   // 纬度
    poCT->Transform(1, &x, &y);

    // 结果为 UTM 52N 坐标
    CPLDebug("Transform", "Result: x=%.2f, y=%.2f", x, y);

    OGRCoordinateTransformation::DestroyCT(poCT);

.. note::

    在 GDAL 3.0 中，如果通过 Python 绑定使用 ``OGRSpatialReference``，默认情况下 ``AxisMappingStrategy`` 为 ``OAMS_AUTHORITY_COMPLIENT``。而在 C/C++ API 中，需要显式设置。建议始终显式设置该策略，以避免跨版本兼容问题。

.. _gdal3-version-highlights:

**********************
GDAL 3.1~3.13 版本亮点
**********************

GDAL 3.0 发布后，后续的 3.x 版本持续引入了大量改进和新功能。以下按版本简要列举各版本的重要变化。

3.1（2020年1月）
=================

- 新增 GDALDataset 栅格数据集的多线程读取支持
- 新增 ``--if`` 选项用于 ``gdal_translate`` 等工具，显式指定输入驱动
- COG（Cloud Optimized GeoTIFF）驱动改进

3.2（2020年11月）
=================

- 新增多维（Multidimensional）数据 API 的 Python 绑定
- STACIT 虚拟驱动：基于 STAC 目录访问影像
- FlatGeoBuf 驱动性能优化

3.3（2021年5月）
================

- 新增 RFC 84：Raster dataset thread safety improvements
- 改进 GeoTIFF 读写性能
- 新增 ESRIC 缓存切片驱动

3.4（2021年11月）
=================

- 新增 Zarr 格式驱动（多维数组数据）
- 改进 HDF5 驱动对多维数据的支持
- 新增 ``gdal raster pipeline`` 概念原型

3.5（2022年5月）
================

- 新增 Rasterio 数组协议支持
- 改进 STAC 集成
- 新增 OGR SQLite WAL 模式支持

3.6（2022年11月）
=================

- 改进多线程瓦片读写
- 新增 RFC 100: Float16 支持（IEEE 754 half-precision）
- QOI 图像格式支持

3.7（2023年5月）
================

- 新增 ``gdal vector pipeline`` 和 ``gdal raster pipeline`` 管道语法
- 改进 CMake 编译系统模块化
- 新增 GRIB2 产品的更多模板支持

3.8（2023年11月）
=================

- 新增 RFC 104: Unified CLI tool (``gdal`` 命令)
- OpenFileGDB 驱动完全取代 FileGDB SDK 驱动
- 改进 Parquet 驱动性能

3.9（2024年5月）
================

- ``gdal`` 统一 CLI 进一步完善
- 新增 RFC 109: C/C++ Header File Reorganization
- 改进 Zarr 和 netCDF 多维数据驱动

3.10（2024年11月）
==================

- 新头文件体系初步可用（``gdal_raster_cpp.h`` 等）
- 改进 MVT（Mapbox Vector Tile）驱动
- 性能优化：大规模栅格数据读取

3.11（2025年2月）
=================

- 新头文件体系成为默认推荐方式
- 改进 COG 驱动的 HTTP 范围请求效率
- SQLite 3.47+ 集成优化

3.12（2025年5月）
=================

- 废弃旧头文件 ``gdal_priv.h`` 的使用警告
- 改进 FlatGeoBuf 和 PMTiles 驱动
- 增强 ``gdal`` CLI 的子命令功能

3.13（2025年11月）
==================

- RFC 109 头文件重组基本完成
- 改进 Parquet 和 GeoParquet 驱动
- 增强多维数据 API 性能

.. _gdal3-new-headers:

***********************
新头文件体系（RFC 109）
***********************

GDAL 3.9 引入了 RFC 109（C/C++ Header File Reorganization），对头文件进行了重新组织，将原先庞大而集中的头文件拆分为多个细粒度的头文件。这一变化的目的是：

- 减少编译时不必要的依赖引入
- 缩小编译时间
- 使 API 结构更加清晰

``gdal_priv.h`` 的变化
=======================

在 GDAL 2.x 和早期 3.x 中，几乎所有 C++ 开发都使用 ``gdal_priv.h`` 作为主要头文件：

.. code-block:: c++

    // 旧方式：引入整个 gdal_priv.h
    #include "gdal_priv.h"

    // gdal_priv.h 包含了 GDALDataset、GDALRasterBand、GDALDriver 等所有核心类
    // 也包含了 cpl_conv.h、cpl_error.h 等基础设施头文件

在新体系中，``gdal_priv.h`` 被拆分为多个功能明确的头文件。``gdal_priv.h`` 仍然存在以保持向后兼容，但推荐使用新的细粒度头文件。

``gdal_raster_cpp.h``
----------------------

``gdal_raster_cpp.h`` 是新的栅格操作主头文件，替代 ``gdal_priv.h`` 用于栅格数据读写场景。它聚合了栅格操作所需的核心声明：

.. code-block:: c++

    // 新方式：使用 gdal_raster_cpp.h
    #include "gdal_raster_cpp.h"

    // 等效于引入了以下内容：
    // - GDALDataset（数据集）
    // - GDALRasterBand（波段）
    // - GDALDriver（驱动）
    // - GDALMajorObject（基类）
    // 以及其他栅格操作相关的类型和函数

``gdal_vector_cpp.h``
----------------------

``gdal_vector_cpp.h`` 是矢量操作的主头文件，用于 OGR 矢量数据的读写：

.. code-block:: c++

    // 矢量操作专用头文件
    #include "gdal_vector_cpp.h"

    // 包含了 OGRLayer、OGRFeature、OGRGeometry 等核心矢量类
    // 以及 OGRDataSource 等相关定义

``gdal_multidim_cpp.h``
-------------------------

``gdal_multidim_cpp.h`` 用于多维数组（Multidimensional Array）操作，这是 GDAL 3.x 中引入的全新数据模型：

.. code-block:: c++

    // 多维数据操作头文件
    #include "gdal_multidim_cpp.h"

    // 包含了 GDALMDArray（多维数组）、GDALDimension（维度）
    // GDALAttribute（属性）等多维数据相关类

    // 示例：打开多维数据集
    GDALAllRegister();
    auto poDataset = std::unique_ptr<GDALDataset>(
        GDALDataset::Open("data.nc", GDAL_OF_MULTIDIM_RASTER));
    if (!poDataset) {
        CPLError(CE_Failure, CPLE_OpenFailed, "Cannot open dataset");
        return;
    }

    auto poRootGroup = poDataset->GetRootGroup();
    auto poVar = poRootGroup->OpenMDArray("temperature");
    if (poVar) {
        auto poDim = poVar->GetDimensions();
        for (const auto &poD : poDim) {
            CPLDebug("Dim", "Name: %s, Size: %d",
                     poD->GetName().c_str(),
                     (int)poD->GetSize());
        }
    }

细粒度头文件
================

除了上述三个聚合头文件，RFC 109 还提供了更细粒度的头文件，适用于只需要引入特定类的场景：

.. code-block:: c++

    // 只需要数据集相关定义时
    #include "gdal_dataset.h"

    // 只需要驱动相关定义时
    #include "gdal_driver.h"

    // 只需要波段相关定义时
    #include "gdal_rasterband.h"

    // 只需要主对象基类时
    #include "gdal_majorobject.h"

    // 只需要颜色表相关定义时
    #include "gdal_colortable.h"

    // 只需要栅格属性表时
    #include "gdal_rat.h"  // Raster Attribute Table

以下是一个完整的细粒度头文件使用示例：

.. code-block:: c++

    // 使用细粒度头文件的完整读取示例
    #include "gdal_dataset.h"   // GDALDataset
    #include "gdal_rasterband.h" // GDALRasterBand
    #include "gdal_driver.h"    // GDALDriver
    #include "cpl_conv.h"       // CPLMalloc 等内存工具

    void ReadRasterData(const char *pszFilename)
    {
        GDALAllRegister();

        // 打开数据集
        GDALDataset *poDS = static_cast<GDALDataset *>(
            GDALOpen(pszFilename, GA_ReadOnly));
        if (poDS == nullptr) {
            CPLError(CE_Failure, CPLE_OpenFailed,
                     "Cannot open %s", pszFilename);
            return;
        }

        // 获取第一个波段
        GDALRasterBand *poBand = poDS->GetRasterBand(1);
        int nXSize = poBand->GetXSize();
        int nYSize = poBand->GetYSize();

        // 读取整行数据
        float *pafScanline = static_cast<float *>(
            CPLMalloc(sizeof(float) * nXSize));

        for (int row = 0; row < nYSize; ++row) {
            CPLErr err = poBand->RasterIO(
                GF_Read, 0, row, nXSize, 1,
                pafScanline, nXSize, 1, GDT_Float32,
                0, 0);
            if (err != CE_None) {
                CPLError(CE_Failure, CPLE_FileIO,
                         "Read error at row %d", row);
                break;
            }
            // 处理 pafScanline 数据...
        }

        CPLFree(pafScanline);
        GDALClose(poDS);
    }

头文件选择建议
================

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - 场景
     - 推荐头文件
   * - 通用栅格读写
     - ``gdal_raster_cpp.h``
   * - 矢量数据操作
     - ``gdal_vector_cpp.h``
   * - 多维数组操作
     - ``gdal_multidim_cpp.h``
   * - 只需打开数据集读取元数据
     - ``gdal_dataset.h``
   * - 编写新的 GDAL 驱动
     - ``gdal_raster_cpp.h`` + ``gdal_driver.h``
   * - 向后兼容旧代码
     - ``gdal_priv.h`` （不推荐新项目使用）

.. note::

    新头文件体系在 GDAL 3.9 中引入，3.10 开始可选使用，3.11 起成为推荐方式。旧的 ``gdal_priv.h`` 在当前版本中仍然可用，但未来版本可能会发出废弃警告。建议新项目直接使用新头文件体系。

.. _gdal3-unified-cli:

***************************
新 gdal 统一 CLI（RFC 104）
***************************

GDAL 3.8 引入了 RFC 104，即 ``gdal`` 统一命令行工具。在此之前，GDAL 提供了多个独立的命令行工具（如 ``gdal_translate``、``gdalwarp``、``ogr2ogr`` 等），新版本中将它们统一到一个 ``gdal`` 命令下，通过子命令的方式调用。

旧方式与新方式对比
====================

.. code-block:: bash

    # ===== 旧方式（仍然可用） =====

    # 栅格格式转换
    gdal_translate -of GTiff input.tif output.tif

    # 栅格重投影和裁剪
    gdalwarp -t_srs EPSG:4326 -te 126 35 130 38 input.tif output.tif

    # 矢量格式转换
    ogr2ogr -f GeoJSON output.json input.shp

    # 查看数据信息
    gdalinfo input.tif

    # ===== 新方式 =====

    # 栅格格式转换
    gdal raster convert -of GTiff input.tif output.tif

    # 栅格重投影和裁剪
    gdal raster pipeline read input.tif ! reproject -t_srs EPSG:4326 ! write output.tif

    # 矢量格式转换
    gdal vector convert -f GeoJSON output.json input.shp

    # 查看数据信息
    gdal info input.tif

管道语法
=========

``gdal`` 统一 CLI 的一大特色是支持管道（pipeline）语法，可以将多个操作串联起来：

.. code-block:: bash

    # 栅格处理管道：读取 -> 裁剪 -> 重投影 -> 重采样 -> 写入
    gdal raster pipeline \
        read input.tif \
        ! crop -bbox 126 35 130 38 \
        ! reproject -t_srs EPSG:3857 \
        ! write -of COG output.tif

    # 矢量处理管道：读取 -> 过滤 -> 投影转换 -> 写入
    gdal vector pipeline \
        read input.shp \
        ! filter -where "population > 100000" \
        ! reproject -t_srs EPSG:4326 \
        ! write -f GeoJSON output.json

    # 管道也支持从标准输入读取
    cat input.tif | gdal raster pipeline read /vsistdin/ ! info

子命令列表
===========

``gdal`` 统一 CLI 提供了以下主要子命令：

- ``gdal info`` — 查看数据集信息（替代 ``gdalinfo`` 和 ``ogrinfo``）
- ``gdal raster`` — 栅格操作子命令组
- ``gdal vector`` — 矢量操作子命令组
- ``gdal mdim`` — 多维数据操作子命令组

其中 ``gdal raster`` 和 ``gdal vector`` 各自包含以下常用子命令：

.. code-block:: bash

    # 栅格子命令
    gdal raster convert   # 格式转换（替代 gdal_translate）
    gdal raster pipeline  # 管道处理
    gdal raster overview  # 金字塔管理
    gdal raster info      # 栅格信息

    # 矢量子命令
    gdal vector convert   # 格式转换（替代 ogr2ogr）
    gdal vector pipeline  # 管道处理
    gdal vector info      # 矢量信息

    # 查看所有可用子命令
    gdal --help
    gdal raster --help
    gdal vector --help

.. note::

    ``gdal`` 统一 CLI 在 GDAL 3.8 中引入，目前仍处于积极开发中。旧的独立命令行工具在可预见的未来仍然可用，但新功能可能优先在 ``gdal`` 统一 CLI 中实现。建议新项目和脚本使用 ``gdal`` 统一 CLI。

.. _gdal3-cmake:

******************
CMake 编译系统变化
******************

GDAL 3.x 全面转向 CMake 编译系统，取代了之前基于 GNU Autotools（configure/make）和 Windows nmake 的编译方式。

基本编译流程
=============

.. code-block:: bash

    # 克隆源码
    git clone https://github.com/OSGeo/gdal.git
    cd gdal
    git checkout v3.13.0  # 切换到指定版本

    # 创建构建目录（推荐 out-of-source 构建）
    mkdir build
    cd build

    # 配置（默认 Release 模式）
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local

    # 编译
    cmake --build . -j$(nproc)

    # 安装
    cmake --build . --target install

常用 CMake 配置选项
====================

.. code-block:: bash

    # 基础配置示例
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DGDAL_BUILD_OPTIONAL_DRIVERS=ON \
        -DGDAL_USE_GEOS=ON \
        -DGDAL_USE_PROJ=ON \
        -DGDAL_USE_TIFF=ON \
        -DGDAL_USE_GEOTIFF=ON \
        -DGDAL_USE_JPEG=ON \
        -DGDAL_USE_PNG=ON \
        -DBUILD_TESTING=OFF

以下是一些常用的 CMake 选项：

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - 选项
     - 说明
   * - ``CMAKE_BUILD_TYPE``
     - 构建类型：Release、Debug、RelWithDebInfo 等
   * - ``CMAKE_INSTALL_PREFIX``
     - 安装路径前缀
   * - ``BUILD_SHARED_LIBS``
     - 是否构建动态库（默认 ON）
   * - ``BUILD_TESTING``
     - 是否构建测试（默认 ON）
   * - ``GDAL_BUILD_OPTIONAL_DRIVERS``
     - 是否构建可选驱动（默认 ON）
   * - ``GDAL_USE_<LIB>``
     - 是否启用特定依赖库（如 GDAL_USE_GEOS、GDAL_USE_PROJ）
   * - ``GDAL_BUILD_APPS``
     - 是否构建命令行工具（默认 ON）
   * - ``GDAL_BUILD_PYTHON_BINDINGS``
     - 是否构建 Python 绑定（默认 ON）

Windows 下使用 vcpkg 编译
===========================

推荐使用 vcpkg 管理依赖库，在 Windows 下的编译流程：

.. code-block:: bash

    # 假设 vcpkg 已安装在 C:/vcpkg
    cd gdal
    mkdir build
    cd build

    cmake .. \
        -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake \
        -DCMAKE_INSTALL_PREFIX=C:/GDAL_INSTALL \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=OFF

    cmake --build . --config Release
    cmake --build . --config Release --target install

CMake 与旧编译系统的对比
==========================

.. list-table::
   :header-rows: 1
   :widths: 20 40 40

   * - 特性
     - 旧系统（nmake/configure）
     - CMake
   * - 跨平台
     - 需要分别维护 nmake.opt 和 configure
     - 统一的 CMakeLists.txt
   * - 依赖管理
     - 手动配置路径
     - 自动检测 + find_package
   * - IDE 集成
     - 需手动生成工程文件
     - 直接生成 VS / Xcode 工程
   * - 构建速度
     - 单线程编译
     - 支持并行编译（-j 参数）
   * - 模块化
     - 较差
     - 较好，可按需启用/禁用驱动
