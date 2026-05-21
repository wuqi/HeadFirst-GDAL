.. highlight:: rst

.. _gdal3-algorithms:

################################
GDAL 3 算法库（alg/）详解
################################

GDAL 的算法库位于源码 ``alg/`` 目录下，提供了栅格化、矢量化、网格插值、等高线生成、距离变换、筛滤、填充值、视域分析、区域统计、全色锐化、颜色量化、校验和、Hilbert编码、坐标变换、DEM分析以及波段代数等一系列空间分析算法。本文逐一介绍这些算法的 C/C++ API 用法。

.. note::

    本文基于 GDAL 3.x 版本，代码示例以 C/C++ 为主。使用前需包含头文件::

        #include "gdal_alg.h"
        #include "gdal.h"
        #include "ogr_api.h"

.. contents:: 目录
    :depth: 2
    :local:

*************************
栅格化（Vector → Raster）
*************************

栅格化是将矢量几何图形"烧录"到栅格数据集上的过程。GDAL 提供了三个核心函数：

- ``GDALRasterizeGeometries()`` — 将一组 OGR 几何对象烧录到已有栅格数据集
- ``GDALRasterizeLayers()`` — 将整个 OGR 图层烧录到已有栅格数据集
- ``GDALRasterizeLayersBuf()`` — 将 OGR 图层栅格化到内存缓冲区

合并模式（Merge Algorithm）
===========================

通过 ``papszOptions`` 中的 ``MERGE_ALG`` 选项控制当多个几何对象重叠时的行为：

- ``REPLACE`` （默认） — 后写入的值覆盖已有值
- ``ADD`` — 将新值累加到已有值上，适用于热力图等场景

烧录值来源（Burn Value Source）
===============================

通过 ``BURN_VALUE_FROM`` 选项控制烧录值的来源：

- ``USER_VALUE``（默认） — 使用 ``padfGeomBurnValues`` 中提供的用户指定值
- ``Z`` — 使用几何对象的 Z 坐标值
- ``M`` — 使用几何对象的 M 值

函数原型
=============

.. code-block:: c++

    // 将一组几何对象烧录到栅格数据集
    CPLErr GDALRasterizeGeometries(
        GDALDatasetH hDS,                // 目标栅格数据集
        int nBandCount,                  // 要烧录的波段数
        const int *panBandList,          // 波段列表
        int nGeomCount,                  // 几何对象数量
        const OGRGeometryH *pahGeometries, // 几何对象数组
        GDALTransformerFunc pfnTransformer, // 坐标变换函数
        void *pTransformArg,             // 变换参数
        const double *padfGeomBurnValues,  // 烧录值数组
        CSLConstList papszOptions,       // 选项列表
        GDALProgressFunc pfnProgress,    // 进度回调
        void *pProgressArg               // 进度回调参数
    );

    // 将整个图层烧录到栅格数据集
    CPLErr GDALRasterizeLayers(
        GDALDatasetH hDS,
        int nBandCount,
        int *panBandList,
        int nLayerCount,
        OGRLayerH *pahLayers,
        GDALTransformerFunc pfnTransformer,
        void *pTransformArg,
        double *padfLayerBurnValues,
        char **papszOptions,
        GDALProgressFunc pfnProgress,
        void *pProgressArg
    );

    // 将图层栅格化到内存缓冲区
    CPLErr GDALRasterizeLayersBuf(
        void *pData,                     // 输出缓冲区
        int nBufXSize,                   // 缓冲区宽度
        int nBufYSize,                   // 缓冲区高度
        GDALDataType eBufType,           // 缓冲区数据类型
        int nPixelSpace,                 // 像素间距（字节）
        int nLineSpace,                  // 行间距（字节）
        int nLayerCount,
        OGRLayerH *pahLayers,
        const char *pszDstProjection,    // 目标投影WKT
        double *padfDstGeoTransform,     // 目标地理变换
        GDALTransformerFunc pfnTransformer,
        void *pTransformArg,
        double dfBurnValue,
        char **papszOptions,
        GDALProgressFunc pfnProgress,
        void *pProgressArg
    );

代码示例：使用 GDALRasterizeGeometries
======================================

.. code-block:: c++

    #include "gdal.h"
    #include "gdal_alg.h"
    #include "ogr_api.h"

    void RasterizeExample()
    {
        GDALAllRegister();

        // 打开目标栅格数据集（已创建好的）
        GDALDatasetH hDS = GDALOpen("output.tif", GA_Update);
        if (hDS == NULL)
        {
            printf("无法打开目标数据集\n");
            return;
        }

        // 准备几何对象和对应的烧录值
        std::vector<OGRGeometryH> ahGeometries;
        std::vector<double> adfBurnValues;

        // 创建一个多边形几何（示例）
        OGRGeometryH hGeom = OGR_G_CreateGeometry(wkbPolygon);
        OGRGeometryH hRing = OGR_G_CreateGeometry(wkbLinearRing);
        OGR_G_AddPoint_2D(hRing, 100.0, 200.0);
        OGR_G_AddPoint_2D(hRing, 200.0, 200.0);
        OGR_G_AddPoint_2D(hRing, 200.0, 300.0);
        OGR_G_AddPoint_2D(hRing, 100.0, 300.0);
        OGR_G_AddPoint_2D(hRing, 100.0, 200.0);
        OGR_G_AddGeometry(hGeom, hRing);
        OGR_G_DestroyGeometry(hRing);

        ahGeometries.push_back(hGeom);
        adfBurnValues.push_back(255.0);  // 烧录值

        // 创建坐标变换（从源坐标系到目标栅格坐标系）
        void *hTransformArg = GDALCreateGenImgProjTransformer(
            NULL, NULL,
            hDS, GDALGetProjectionRef(hDS),
            FALSE, 0.0, 3);
        GDALTransformerFunc pfnTransformer = GDALGenImgProjTransform;

        // 准备波段列表
        int anBandList[1] = {1};

        // 设置选项
        char *papszOptions[] = {
            (char*)"MERGE_ALG=REPLACE",
            NULL
        };

        // 执行栅格化
        CPLErr err = GDALRasterizeGeometries(
            hDS, 1, anBandList,
            (int)ahGeometries.size(), &(ahGeometries[0]),
            pfnTransformer, hTransformArg,
            &(adfBurnValues[0]),
            papszOptions, NULL, NULL);

        // 清理
        GDALDestroyGenImgProjTransformer(hTransformArg);
        OGR_G_DestroyGeometry(hGeom);
        GDALClose(hDS);
    }

.. warning::

    ``padfGeomBurnValues`` 的长度为 ``nGeomCount * nBandCount``，每个波段都需要设置对应的烧录值。如果只有1个波段和3个几何对象，则数组长度为3。

.. _gdal3-vectorize:

*************************
矢量化（Raster → Vector）
*************************

矢量化是将栅格数据转换为矢量多边形的过程。GDAL 提供两个函数：

- ``GDALPolygonize()`` — 将整型栅格转换为多边形
- ``GDALFPolygonize()`` — 将浮点型栅格转换为多边形（GDAL 3.x 新增）

函数原型
=============

.. code-block:: c++

    // 整型栅格矢量化
    CPLErr GDALPolygonize(
        GDALRasterBandH hSrcBand,    // 源波段
        GDALRasterBandH hMaskBand,   // 掩膜波段（NULL表示不使用）
        OGRLayerH hOutLayer,         // 输出矢量图层
        int iPixValField,            // 输出字段索引（-1表示不写像素值）
        char **papszOptions,         // 选项
        GDALProgressFunc pfnProgress,
        void *pProgressArg
    );

    // 浮点型栅格矢量化
    CPLErr GDALFPolygonize(
        GDALRasterBandH hSrcBand,
        GDALRasterBandH hMaskBand,
        OGRLayerH hOutLayer,
        int iPixValField,
        char **papszOptions,
        GDALProgressFunc pfnProgress,
        void *pProgressArg
    );

代码示例
=============

.. code-block:: c++

    #include "gdal.h"
    #include "gdal_alg.h"
    #include "ogrsf_frmts.h"

    void PolygonizeExample()
    {
        GDALAllRegister();

        // 打开源栅格数据集
        GDALDatasetH hSrcDS = GDALOpen("classification.tif", GA_ReadOnly);
        if (hSrcDS == NULL)
        {
            printf("无法打开源数据集\n");
            return;
        }
        GDALRasterBandH hSrcBand = GDALGetRasterBand(hSrcDS, 1);

        // 创建输出矢量数据集
        GDALDriverH hDriver = GDALGetDriverByName("ESRI Shapefile");
        GDALDatasetH hDstDS = GDALCreate(hDriver, "polygons.shp",
                                          0, 0, 0, GDT_Unknown, NULL);
        OGRLayerH hLayer = GDALCreateLayer(hDstDS, "polygons",
                                            NULL, wkbPolygon, NULL);

        // 创建像素值字段
        OGRFieldDefnH hFieldDefn = OGR_Fld_Create("PixelValue", OFTInteger);
        OGR_L_CreateField(hLayer, hFieldDefn, TRUE);
        OGR_Fld_Destroy(hFieldDefn);

        // 执行矢量化，将像素值写入第0个字段
        char **papszOptions = NULL;
        papszOptions = CSLSetNameValue(papszOptions, "8CONNECTED", "8");
        CPLErr err = GDALPolygonize(
            hSrcBand,       // 源波段
            NULL,           // 掩膜波段（不使用）
            hLayer,         // 输出图层
            0,              // 像素值字段索引
            papszOptions,
            NULL, NULL);

        CSLDestroy(papszOptions);
        GDALClose(hDstDS);
        GDALClose(hSrcDS);
    }

.. note::

    - ``GDALPolygonize()`` 要求输入波段为整型数据（Byte, Int16, UInt16, Int32, UInt32 等）
    - ``GDALFPolygonize()`` 支持浮点型数据（Float32, Float64）
    - 掩膜波段中值为0的像素将被忽略，非零像素参与矢量化
    - 选项 ``8CONNECTED=8`` 表示使用8连通分析，默认为4连通

.. _gdal3-gridding:

********************
网格插值（Gridding）
********************

网格插值将离散点数据内插为规则格网。GDAL 提供三种调用方式和11种插值算法。

三种调用方式
=============

1. **一次性调用** ``GDALGridCreate()`` — 简单直接，适合单次插值
2. **可复用上下文** ``GDALGridContextCreate()`` / ``GDALGridContextProcess()`` / ``GDALGridContextFree()`` — 批量插值时效率更高，避免重复构建索引
3. **工具方式** ``GDALGrid()`` — 命令行工具对应的 C API

11种插值算法
=============

通过 ``GDALGridAlgorithm`` 枚举指定：

**插值算法：**

- ``GGA_InverseDistanceToAPower`` (1) — 反距离权重（IDW），使用搜索椭圆
- ``GGA_InverseDistanceToAPowerNearestNeighbor`` (11) — 反距离权重（带最近邻搜索），GDAL 3.x 新增
- ``GGA_MovingAverage`` (2) — 移动平均
- ``GGA_NearestNeighbor`` (3) — 最近邻
- ``GGA_Linear`` (10) — 线性插值（基于 Delaunay 三角网）

**数据指标（Data Metric）算法：**

- ``GGA_MetricMinimum`` (4) — 搜索椭圆内最小值
- ``GGA_MetricMaximum`` (5) — 搜索椭圆内最大值
- ``GGA_MetricRange`` (6) — 搜索椭圆内数据范围（最大值 - 最小值）
- ``GGA_MetricCount`` (7) — 搜索椭圆内点数量
- ``GGA_MetricAverageDistance`` (8) — 到搜索椭圆内各点的平均距离
- ``GGA_MetricAverageDistancePts`` (9) — 搜索椭圆内各点之间的平均距离

各算法对应的选项结构体
======================

- ``GDALGridInverseDistanceToAPowerOptions`` — IDW 选项
- ``GDALGridInverseDistanceToAPowerNearestNeighborOptions`` — IDW（最近邻）选项
- ``GDALGridMovingAverageOptions`` — 移动平均选项
- ``GDALGridNearestNeighborOptions`` — 最近邻选项
- ``GDALGridDataMetricsOptions`` — 数据指标选项（所有 Metric 算法共用）
- ``GDALGridLinearOptions`` — 线性插值选项

函数原型
=============

.. code-block:: c++

    // 一次性调用
    CPLErr GDALGridCreate(
        GDALGridAlgorithm eAlgorithm,  // 插值算法
        const void *poOptions,         // 算法选项结构体指针
        GUInt32 nPoints,               // 离散点数量
        const double *padfX,           // X坐标数组
        const double *padfY,           // Y坐标数组
        const double *padfZ,           // Z值数组
        double dfXMin, double dfXMax,  // 输出范围
        double dfYMin, double dfYMax,
        GUInt32 nXSize,                // 输出宽度（像素）
        GUInt32 nYSize,                // 输出高度（像素）
        GDALDataType eType,            // 输出数据类型
        void *pData,                   // 输出缓冲区
        GDALProgressFunc pfnProgress,
        void *pProgressArg
    );

    // 可复用上下文
    GDALGridContext* GDALGridContextCreate(
        GDALGridAlgorithm eAlgorithm,
        const void *poOptions,
        GUInt32 nPoints,
        const double *padfX,
        const double *padfY,
        const double *padfZ,
        int bCallerWillKeepPointArraysAlive // 调用者是否保持数组存活
    );

    CPLErr GDALGridContextProcess(
        GDALGridContext *psContext,
        double dfXMin, double dfXMax,
        double dfYMin, double dfYMax,
        GUInt32 nXSize, GUInt32 nYSize,
        GDALDataType eType,
        void *pData,
        GDALProgressFunc pfnProgress,
        void *pProgressArg
    );

    void GDALGridContextFree(GDALGridContext *psContext);

代码示例：IDW 插值
==================

.. code-block:: c++

    #include "gdal.h"
    #include "gdal_alg.h"

    void IDWInterpolationExample()
    {
        // 准备离散点数据（实际应用中从文件或数据库读取）
        std::vector<double> adfX = {100, 200, 300, 150, 250, 350, 100, 400};
        std::vector<double> adfY = {100, 150, 200, 300, 250, 100, 400, 350};
        std::vector<double> adfZ = {10.5, 20.3, 15.7, 25.1, 18.9, 12.4, 30.0, 8.5};

        // 计算输出范围
        double dfXMin = *std::min_element(adfX.begin(), adfX.end());
        double dfXMax = *std::max_element(adfX.begin(), adfX.end());
        double dfYMin = *std::min_element(adfY.begin(), adfY.end());
        double dfYMax = *std::max_element(adfY.begin(), adfY.end());

        int nWidth = 500;   // 输出栅格宽度
        int nHeight = 500;  // 输出栅格高度

        // 配置 IDW 算法选项
        GDALGridInverseDistanceToAPowerOptions sOptions;
        memset(&sOptions, 0, sizeof(sOptions));
        sOptions.nSizeOfStructure = sizeof(sOptions);
        sOptions.dfPower = 2.0;            // 权重幂次（标准IDW为2）
        sOptions.dfSmoothing = 0.0;        // 平滑参数
        sOptions.dfRadius1 = 0.0;          // 搜索椭圆半径1（0表示搜索所有点）
        sOptions.dfRadius2 = 0.0;          // 搜索椭圆半径2
        sOptions.dfAngle = 0.0;            // 椭圆旋转角度
        sOptions.nMaxPoints = 0;           // 最大搜索点数（0表示不限制）
        sOptions.nMinPoints = 0;           // 最小搜索点数
        sOptions.dfNoDataValue = -9999.0;  // 无数据值

        // 分配输出缓冲区
        double *padfGridData = new double[nWidth * nHeight];

        // 执行插值
        CPLErr err = GDALGridCreate(
            GGA_InverseDistanceToAPower,
            &sOptions,
            (GUInt32)adfX.size(),
            &(adfX[0]), &(adfY[0]), &(adfZ[0]),
            dfXMin, dfXMax, dfYMin, dfYMax,
            nWidth, nHeight,
            GDT_Float64,
            padfGridData,
            NULL, NULL);

        if (err == CE_None)
        {
            // 将结果写入 GeoTIFF
            GDALDriverH hDriver = GDALGetDriverByName("GTiff");
            GDALDatasetH hDS = GDALCreate(hDriver, "idw_output.tif",
                                           nWidth, nHeight, 1, GDT_Float64, NULL);

            // 设置地理变换
            double adfGeoTransform[6] = {
                dfXMin, (dfXMax - dfXMin) / nWidth, 0,
                dfYMax, 0, -(dfYMax - dfYMin) / nHeight
            };
            GDALSetGeoTransform(hDS, adfGeoTransform);

            // 写入数据
            GDALRasterBandH hBand = GDALGetRasterBand(hDS, 1);
            GDALSetRasterNoDataValue(hBand, -9999.0);
            GDALRasterIO(hBand, GF_Write, 0, 0, nWidth, nHeight,
                         padfGridData, nWidth, nHeight, GDT_Float64, 0, 0);

            GDALClose(hDS);
        }

        delete[] padfGridData;
    }

代码示例：使用可复用上下文批量插值
==================================

.. code-block:: c++

    void BatchInterpolationExample()
    {
        // 离散点数据
        std::vector<double> adfX, adfY, adfZ;
        // ... 填充数据 ...

        GDALGridLinearOptions sOptions;
        memset(&sOptions, 0, sizeof(sOptions));
        sOptions.nSizeOfStructure = sizeof(sOptions);
        sOptions.dfRadius = 0.0;           // 0表示不使用最近邻搜索
        sOptions.dfNoDataValue = -9999.0;

        // 创建可复用上下文
        GDALGridContext *psContext = GDALGridContextCreate(
            GGA_Linear,
            &sOptions,
            (GUInt32)adfX.size(),
            &(adfX[0]), &(adfY[0]), &(adfZ[0]),
            FALSE  // GDAL 负责复制数据
        );

        if (psContext == NULL)
        {
            printf("创建插值上下文失败\n");
            return;
        }

        // 批量生成不同分辨率的插值结果
        double dfXMin = 0, dfXMax = 1000, dfYMin = 0, dfYMax = 1000;

        // 分辨率1：500x500
        double *pData1 = new double[500 * 500];
        GDALGridContextProcess(psContext, dfXMin, dfXMax, dfYMin, dfYMax,
                               500, 500, GDT_Float64, pData1, NULL, NULL);

        // 分辨率2：1000x1000
        double *pData2 = new double[1000 * 1000];
        GDALGridContextProcess(psContext, dfXMin, dfXMax, dfYMin, dfYMax,
                               1000, 1000, GDT_Float64, pData2, NULL, NULL);

        // 清理
        GDALGridContextFree(psContext);
        delete[] pData1;
        delete[] pData2;
    }

.. warning::

    - GDAL 2.2.3 以下版本中，使用 ``GGA_Linear`` 方法可能会导致断言失败或段错误。建议升级到 2.2.3 或更高版本。
    - 当 ``bCallerWillKeepPointArraysAlive`` 设为 TRUE 时，调用者必须保证 X/Y/Z 数组在上下文生命周期内保持有效。
    - IDW 算法的 ``dfRadius1/dfRadius2`` 设为 0 表示搜索所有点，适用于数据量较小的场景。大数据量建议设置合理的搜索半径。

.. _gdal3-contour:

********************
等高线生成
********************

等高线生成将 DEM 栅格转换为等值线矢量。GDAL 提供三种调用方式：

- ``GDALContourGenerate()`` — 基本等高线生成
- ``GDALContourGenerateEx()`` — 扩展等高线生成（支持更多选项）
- ``GDAL_CG_Create()`` / ``GDAL_CG_FeedLine()`` / ``GDAL_CG_Destroy()`` — 扫描线模式，逐行喂数据

函数原型
=============

.. code-block:: c++

    // 基本等高线生成
    CPLErr GDALContourGenerate(
        GDALRasterBandH hBand,           // 输入DEM波段
        double dfContourInterval,        // 等高距
        double dfContourBase,            // 等高线基准值
        int nFixedLevelCount,            // 固定等高线数量
        double *padfFixedLevels,         // 固定等高线值数组
        int bUseNoData,                  // 是否使用NoData值
        double dfNoDataValue,            // NoData值
        void *hLayer,                    // 输出OGRLayer
        int iIDField,                    // ID字段索引（-1表示不写）
        int iElevField,                  // 高程字段索引（-1表示不写）
        GDALProgressFunc pfnProgress,
        void *pProgressArg
    );

    // 扩展等高线生成
    CPLErr GDALContourGenerateEx(
        GDALRasterBandH hBand,
        void *hLayer,
        CSLConstList options,            // 选项列表
        GDALProgressFunc pfnProgress,
        void *pProgressArg
    );

    // 扫描线模式
    GDALContourGeneratorH GDAL_CG_Create(
        int nWidth, int nHeight,
        int bNoDataSet, double dfNoDataValue,
        double dfContourInterval, double dfContourBase,
        GDALContourWriter pfnWriter,     // 回调函数
        void *pCBData                    // 回调数据
    );

    CPLErr GDAL_CG_FeedLine(GDALContourGeneratorH hCG, double *padfScanline);
    void GDAL_CG_Destroy(GDALContourGeneratorH hCG);

GDALContourGenerateEx 选项
==========================

- ``INTERVAL`` — 等高距
- ``LEVEL_INTERVAL`` — 同 INTERVAL
- ``BASE`` — 等高线基准值
- ``LEVEL_BASE`` — 同 BASE
- ``FIXED_LEVELS`` — 逗号分隔的固定等高线值列表
- ``NODATA`` — NoData 值
- ``ID_FIELD`` — ID 字段名
- ``ELEV_FIELD`` — 高程字段名
- ``ELEV_FIELD_MIN`` — 最小高程字段名（用于多边形等高线）
- ``ELEV_FIELD_MAX`` — 最大高程字段名
- ``POLYGONIZE`` — 设为 YES 生成多边形等高线（区间面）

代码示例
=============

.. code-block:: c++

    #include "gdal.h"
    #include "gdal_alg.h"
    #include "ogrsf_frmts.h"

    void ContourGenerateExample()
    {
        GDALAllRegister();

        // 打开 DEM 数据集
        GDALDatasetH hDS = GDALOpen("dem.tif", GA_ReadOnly);
        if (hDS == NULL)
        {
            printf("无法打开 DEM 数据集\n");
            return;
        }
        GDALRasterBandH hBand = GDALGetRasterBand(hDS, 1);

        // 创建输出矢量数据集
        GDALDriverH hDriver = GDALGetDriverByName("ESRI Shapefile");
        GDALDatasetH hDstDS = GDALCreate(hDriver, "contours.shp",
                                          0, 0, 0, GDT_Unknown, NULL);
        OGRLayerH hLayer = GDALCreateLayer(hDstDS, "contours",
                                            NULL, wkbLineString, NULL);

        // 创建 ID 字段和高程字段
        OGRFieldDefnH hIDField = OGR_Fld_Create("ID", OFTInteger);
        OGR_L_CreateField(hLayer, hIDField, TRUE);
        OGR_Fld_Destroy(hIDField);

        OGRFieldDefnH hElevField = OGR_Fld_Create("Elevation", OFTReal);
        OGR_L_CreateField(hLayer, hElevField, TRUE);
        OGR_Fld_Destroy(hElevField);

        // 方式一：使用 GDALContourGenerate（基本方式）
        double dfNoData = -9999.0;
        CPLErr err = GDALContourGenerate(
            hBand,
            10.0,           // 等高距：10米
            0.0,            // 基准值
            0,              // 固定等高线数量（0表示不使用）
            NULL,           // 固定等高线值
            TRUE,           // 使用NoData
            dfNoData,
            hLayer,
            0,              // ID 字段索引
            1,              // 高程字段索引
            NULL, NULL);

        GDALClose(hDstDS);
        GDALClose(hDS);
    }

    void ContourGenerateExExample()
    {
        GDALAllRegister();

        GDALDatasetH hDS = GDALOpen("dem.tif", GA_ReadOnly);
        GDALRasterBandH hBand = GDALGetRasterBand(hDS, 1);

        GDALDriverH hDriver = GDALGetDriverByName("ESRI Shapefile");
        GDALDatasetH hDstDS = GDALCreate(hDriver, "contours_ex.shp",
                                          0, 0, 0, GDT_Unknown, NULL);
        OGRLayerH hLayer = GDALCreateLayer(hDstDS, "contours",
                                            NULL, wkbLineString, NULL);

        // 方式二：使用 GDALContourGenerateEx（扩展方式，支持更多选项）
        char **papszOptions = NULL;
        papszOptions = CSLSetNameValue(papszOptions, "INTERVAL", "10");
        papszOptions = CSLSetNameValue(papszOptions, "BASE", "0");
        papszOptions = CSLSetNameValue(papszOptions, "NODATA", "-9999");
        papszOptions = CSLSetNameValue(papszOptions, "ID_FIELD", "ID");
        papszOptions = CSLSetNameValue(papszOptions, "ELEV_FIELD", "Elevation");

        CPLErr err = GDALContourGenerateEx(hBand, hLayer, papszOptions, NULL, NULL);

        CSLDestroy(papszOptions);
        GDALClose(hDstDS);
        GDALClose(hDS);
    }

代码示例：扫描线模式
====================

.. code-block:: c++

    // 自定义回调函数，用于处理生成的等高线
    CPLErr MyContourWriter(double dfLevel, int nPoints,
                           double *padfX, double *padfY, void *pCBData)
    {
        // 在此处处理等高线数据
        // dfLevel: 等高线高程值
        // nPoints: 等高线上的点数
        // padfX/padfY: 等高线坐标数组
        printf("等高线: 高程=%.1f, 点数=%d\n", dfLevel, nPoints);
        return CE_None;
    }

    void ScanlineContourExample()
    {
        // 打开 DEM
        GDALDatasetH hDS = GDALOpen("dem.tif", GA_ReadOnly);
        GDALRasterBandH hBand = GDALGetRasterBand(hDS, 1);
        int nWidth = GDALGetRasterXSize(hDS);
        int nHeight = GDALGetRasterYSize(hDS);

        // 创建扫描线等高线生成器
        GDALContourGeneratorH hCG = GDAL_CG_Create(
            nWidth, nHeight,
            TRUE, -9999.0,      // NoData
            10.0, 0.0,          // 等高距和基准值
            MyContourWriter,    // 回调函数
            NULL                // 回调数据
        );

        // 逐行喂数据
        double *padfScanline = new double[nWidth];
        for (int iLine = 0; iLine < nHeight; iLine++)
        {
            GDALRasterIO(hBand, GF_Read, 0, iLine, nWidth, 1,
                         padfScanline, nWidth, 1, GDT_Float64, 0, 0);
            GDAL_CG_FeedLine(hCG, padfScanline);
        }

        // 清理
        GDAL_CG_Destroy(hCG);
        delete[] padfScanline;
        GDALClose(hDS);
    }

.. _gdal3-proximity:

*********************
距离变换（Proximity）
*********************

距离变换计算每个像素到最近的指定源像素的距离。``GDALComputeProximity()`` 函数实现此功能。

函数原型
=============

.. code-block:: c++

    CPLErr GDALComputeProximity(
        GDALRasterBandH hSrcBand,        // 源波段
        GDALRasterBandH hProximityBand,  // 输出距离波段
        char **papszOptions,             // 选项
        GDALProgressFunc pfnProgress,
        void *pProgressArg
    );

选项说明
=============

- ``VALUES`` — 逗号分隔的源像素值列表，只有这些像素作为距离计算的起始点
- ``MAXDIST`` — 最大搜索距离，超过此距离的像素赋值为 NoData
- ``NODATA`` — 输出的 NoData 值
- ``USE_INPUT_NODATA`` — 设为 YES 时，输入的 NoData 像素在输出中也保持 NoData
- ``DISTUNITS`` — 距离单位：``PIXEL`` （像素，默认）或 ``GEO`` （地理单位）
- ``OPTIONS`` — 传递给底层算法的额外选项

代码示例
=============

.. code-block:: c++

    #include "gdal.h"
    #include "gdal_alg.h"

    void ProximityExample()
    {
        GDALAllRegister();

        // 打开源栅格数据集
        GDALDatasetH hSrcDS = GDALOpen("source_features.tif", GA_ReadOnly);
        if (hSrcDS == NULL)
        {
            printf("无法打开源数据集\n");
            return;
        }
        GDALRasterBandH hSrcBand = GDALGetRasterBand(hSrcDS, 1);

        // 创建输出距离栅格
        int nWidth = GDALGetRasterXSize(hSrcDS);
        int nHeight = GDALGetRasterYSize(hSrcDS);
        GDALDriverH hDriver = GDALGetDriverByName("GTiff");
        GDALDatasetH hDstDS = GDALCreate(hDriver, "proximity.tif",
                                          nWidth, nHeight, 1, GDT_Float32, NULL);

        // 复制地理参考信息
        double adfGeoTransform[6];
        GDALGetGeoTransform(hSrcDS, adfGeoTransform);
        GDALSetGeoTransform(hDstDS, adfGeoTransform);
        GDALSetProjection(hDstDS, GDALGetProjectionRef(hSrcDS));

        GDALRasterBandH hProximityBand = GDALGetRasterBand(hDstDS, 1);
        GDALSetRasterNoDataValue(hProximityBand, -1.0);

        // 设置选项
        char **papszOptions = NULL;
        papszOptions = CSLSetNameValue(papszOptions, "VALUES", "1");
        papszOptions = CSLSetNameValue(papszOptions, "MAXDIST", "100");
        papszOptions = CSLSetNameValue(papszOptions, "NODATA", "-1");
        papszOptions = CSLSetNameValue(papszOptions, "DISTUNITS", "PIXEL");

        // 计算距离
        CPLErr err = GDALComputeProximity(
            hSrcBand, hProximityBand, papszOptions, NULL, NULL);

        CSLDestroy(papszOptions);
        GDALClose(hDstDS);
        GDALClose(hSrcDS);
    }

.. _gdal3-sieve:

********************
筛滤（Sieve）
********************

``GDALSieveFilter()`` 用于移除栅格中面积小于指定阈值的多边形区域，常用于分类结果的后处理，去除小的椒盐噪声。

函数原型
=============

.. code-block:: c++

    CPLErr GDALSieveFilter(
        GDALRasterBandH hSrcBand,        // 源波段
        GDALRasterBandH hMaskBand,       // 掩膜波段（NULL表示不使用）
        GDALRasterBandH hDstBand,        // 输出波段（可以与源相同）
        int nSizeThreshold,              // 面积阈值（像素数）
        int nConnectedness,              // 连通性：4或8
        char **papszOptions,             // 选项
        GDALProgressFunc pfnProgress,
        void *pProgressArg
    );

代码示例
=============

.. code-block:: c++

    #include "gdal.h"
    #include "gdal_alg.h"

    void SieveFilterExample()
    {
        GDALAllRegister();

        // 打开分类结果栅格
        GDALDatasetH hSrcDS = GDALOpen("classification.tif", GA_Update);
        if (hSrcDS == NULL)
        {
            printf("无法打开数据集\n");
            return;
        }
        GDALRasterBandH hSrcBand = GDALGetRasterBand(hSrcDS, 1);

        // 创建输出数据集
        int nWidth = GDALGetRasterXSize(hSrcDS);
        int nHeight = GDALGetRasterYSize(hSrcDS);
        GDALDriverH hDriver = GDALGetDriverByName("GTiff");
        GDALDatasetH hDstDS = GDALCreate(hDriver, "sieved.tif",
                                          nWidth, nHeight, 1,
                                          GDALGetRasterDataType(hSrcBand), NULL);

        // 复制地理参考
        double adfGeoTransform[6];
        GDALGetGeoTransform(hSrcDS, adfGeoTransform);
        GDALSetGeoTransform(hDstDS, adfGeoTransform);
        GDALSetProjection(hDstDS, GDALGetProjectionRef(hSrcDS));

        GDALRasterBandH hDstBand = GDALGetRasterBand(hDstDS, 1);

        // 执行筛滤：移除面积小于100像素的区域，使用8连通
        CPLErr err = GDALSieveFilter(
            hSrcBand,       // 源波段
            NULL,           // 掩膜波段
            hDstBand,       // 输出波段
            100,            // 面积阈值：100像素
            8,              // 8连通
            NULL,           // 选项
            NULL, NULL      // 进度回调
        );

        GDALClose(hDstDS);
        GDALClose(hSrcDS);
    }

.. note::

    - ``nSizeThreshold`` 的单位是像素个数，不是地理面积
    - ``nConnectedness`` 为4表示上下左右四方向连通，8表示加上对角线八方向连通
    - 源波段和输出波段可以是同一个（原地修改），也可以不同

.. _gdal3-fillnodata:

*********************
填充值（Fill Nodata）
*********************

``GDALFillNodata()`` 使用插值方法填充栅格中的 NoData 区域，通常用于修复 DEM 中的空洞。算法从 NoData 区域的边缘向内部插值。

函数原型
=============

.. code-block:: c++

    CPLErr GDALFillNodata(
        GDALRasterBandH hTargetBand,       // 目标波段（输入输出）
        GDALRasterBandH hMaskBand,         // 掩膜波段（NULL表示自动检测NoData）
        double dfMaxSearchDist,            // 最大搜索距离（像素为单位）
        int bDeprecatedOption,             // 已废弃，设为0
        int nSmoothingIterations,          // 平滑迭代次数
        char **papszOptions,               // 选项
        GDALProgressFunc pfnProgress,
        void *pProgressArg
    );

代码示例
=============

.. code-block:: c++

    #include "gdal.h"
    #include "gdal_alg.h"

    void FillNodataExample()
    {
        GDALAllRegister();

        // 打开含有空洞的 DEM
        GDALDatasetH hDS = GDALOpen("dem_with_holes.tif", GA_Update);
        if (hDS == NULL)
        {
            printf("无法打开数据集\n");
            return;
        }
        GDALRasterBandH hBand = GDALGetRasterBand(hDS, 1);

        // 设置 NoData 值（如果尚未设置）
        GDALSetRasterNoDataValue(hBand, -9999.0);

        // 填充 NoData 区域
        // dfMaxSearchDist: 最大搜索距离（像素），设为0表示不限制
        // nSmoothingIterations: 平滑迭代次数，通常3-10次
        CPLErr err = GDALFillNodata(
            hBand,          // 目标波段
            NULL,           // 掩膜波段（自动使用NoData）
            100,            // 最大搜索距离：100像素
            0,              // 废弃参数
            5,              // 平滑迭代5次
            NULL,           // 选项
            NULL, NULL      // 进度回调
        );

        GDALClose(hDS);
    }

.. warning::

    - 此函数直接修改输入数据集，建议先备份原始数据
    - ``dfMaxSearchDist`` 设为 0 时会搜索整个图像，对于大图像可能非常慢
    - ``nSmoothingIterations`` 越大结果越平滑，但计算时间也越长

.. _gdal3-viewshed:

********************
视域分析（Viewshed）
********************

视域分析计算从观察点可以看到的区域。GDAL 提供两个函数：

- ``GDALViewshedGenerate()`` — 生成视域栅格
- ``GDALIsLineOfSightVisible()`` — 判断两点间是否通视

视域输出模式
=============

通过 ``GDALViewshedOutputType`` 枚举指定输出模式：

- ``GVOT_NORMAL`` (1) — 标准模式：可见为 ``dfVisibleVal``，不可见为 ``dfInvisibleVal``
- ``GVOT_MIN_TARGET_HEIGHT_FROM_DEM`` (2) — 输出使目标可见所需的最小目标高度（基于 DEM）
- ``GVOT_MIN_TARGET_HEIGHT_FROM_GROUND`` (3) — 输出使目标可见所需的最小目标高度（基于地面）

观测模式
=============

通过 ``GDALViewshedMode`` 枚举指定：

- ``GVM_Diagonal`` (1) — 对角线方向
- ``GVM_Edge`` (2) — 边缘方向
- ``GVM_Max`` (3) — 最大值模式
- ``GVM_Min`` (4) — 最小值模式

函数原型
=============

.. code-block:: c++

    GDALDatasetH GDALViewshedGenerate(
        GDALRasterBandH hBand,           // 输入DEM波段
        const char *pszDriverName,       // 输出驱动名
        const char *pszTargetRasterName, // 输出文件名
        CSLConstList papszCreationOptions, // 创建选项
        double dfObserverX,              // 观察点X坐标（地理坐标）
        double dfObserverY,              // 观察点Y坐标
        double dfObserverHeight,         // 观察点高度（米）
        double dfTargetHeight,           // 目标高度（米）
        double dfVisibleVal,             // 可见区域填充值
        double dfInvisibleVal,           // 不可见区域填充值
        double dfOutOfRangeVal,          // 超出范围填充值
        double dfNoDataVal,              // NoData值
        double dfCurvCoeff,              // 地球曲率改正系数
        GDALViewshedMode eMode,          // 观测模式
        double dfMaxDistance,            // 最大观测距离（地理单位）
        GDALProgressFunc pfnProgress,
        void *pProgressArg,
        GDALViewshedOutputType heightMode, // 输出模式
        CSLConstList papszExtraOptions   // 额外选项
    );

    // 两点通视判断
    bool GDALIsLineOfSightVisible(
        const GDALRasterBandH hBand,     // DEM波段
        const int xA, const int yA,      // 点A的像素坐标
        const double zA,                 // 点A的高度偏移
        const int xB, const int yB,      // 点B的像素坐标
        const double zB,                 // 点B的高度偏移
        int *pnxTerrainIntersection,     // 输出：地形交点X
        int *pnyTerrainIntersection,     // 输出：地形交点Y
        CSLConstList papszOptions        // 选项
    );

代码示例
=============

.. code-block:: c++

    #include "gdal.h"
    #include "gdal_alg.h"

    void ViewshedExample()
    {
        GDALAllRegister();

        // 打开 DEM 数据集
        GDALDatasetH hDS = GDALOpen("dem.tif", GA_ReadOnly);
        if (hDS == NULL)
        {
            printf("无法打开 DEM\n");
            return;
        }
        GDALRasterBandH hBand = GDALGetRasterBand(hDS, 1);

        // 观察点地理坐标（经度, 纬度）
        double dfObserverX = 116.4074;
        double dfObserverY = 39.9042;
        double dfObserverHeight = 50.0;   // 观察点离地高度（米）
        double dfTargetHeight = 0.0;      // 目标离地高度（米）
        double dfMaxDistance = 10000.0;    // 最大观测距离（地理单位）

        // 地球曲率改正系数
        // 0.0 = 不改正, 0.857 = 标准改正（考虑大气折射）
        double dfCurvCoeff = 0.857;

        // 生成标准视域栅格
        char **papszCreationOptions = NULL;
        papszCreationOptions = CSLSetNameValue(papszCreationOptions,
                                                "COMPRESS", "LZW");

        GDALDatasetH hViewshedDS = GDALViewshedGenerate(
            hBand,
            "GTiff",                    // 输出驱动
            "viewshed.tif",             // 输出文件名
            papszCreationOptions,
            dfObserverX, dfObserverY,
            dfObserverHeight,
            dfTargetHeight,
            255.0,                      // 可见区域值
            0.0,                        // 不可见区域值
            -1.0,                       // 超出范围值
            -1.0,                       // NoData值
            dfCurvCoeff,
            GVM_Edge,                   // 边缘模式
            dfMaxDistance,
            NULL, NULL,
            GVOT_NORMAL,                // 标准输出模式
            NULL                        // 额外选项
        );

        if (hViewshedDS != NULL)
        {
            GDALClose(hViewshedDS);
        }

        // 两点通视判断示例
        int nTerrainX, nTerrainY;
        bool bVisible = GDALIsLineOfSightVisible(
            hBand,
            100, 200, 5.0,    // 点A: 像素(100,200), 高度偏移5米
            500, 600, 0.0,    // 点B: 像素(500,600), 高度偏移0米
            &nTerrainX, &nTerrainY,
            NULL
        );

        if (bVisible)
            printf("两点之间通视\n");
        else
            printf("两点之间不通视，地形遮挡点: (%d, %d)\n",
                   nTerrainX, nTerrainY);

        CSLDestroy(papszCreationOptions);
        GDALClose(hDS);
    }

.. _gdal3-zonalstats:

***********************
区域统计（Zonal Stats）
***********************

``GDALZonalStats()`` 计算给定区域内的栅格统计量。区域可以是栅格或矢量定义的分区。

函数原型
=============

.. code-block:: c++

    CPLErr GDALZonalStats(
        GDALDatasetH hSrcDS,     // 源栅格数据集（要统计的值）
        GDALDatasetH hWeightsDS, // 权重栅格数据集（可选，NULL表示不使用权重）
        GDALDatasetH hZonesDS,   // 区域数据集（栅格或矢量）
        GDALDatasetH hOutDS,     // 输出数据集（矢量图层将写入此处）
        CSLConstList papszOptions, // 选项
        GDALProgressFunc pfnProgress,
        void *pProgressArg
    );

支持的统计量
=============

通过 ``STATS`` 选项指定要计算的统计量（逗号分隔），支持以下18种：

- ``count`` — 像素数量
- ``min`` — 最小值
- ``max`` — 最大值
- ``mean`` — 平均值
- ``sum`` — 总和
- ``stdev`` — 标准差
- ``variance`` — 方差
- ``mode`` — 众数（出现频率最高的值）
- ``minority`` — 少数派（出现频率最低的值）
- ``unique`` — 唯一值数量
- ``values`` — 所有唯一值列表
- ``frac`` — 各值的比例
- ``coverage`` — 覆盖面积
- ``center_x`` — 区域中心X坐标
- ``center_y`` — 区域中心Y坐标
- ``min_center_x`` / ``min_center_y`` — 最小值所在位置
- ``max_center_x`` / ``max_center_y`` — 最大值所在位置
- ``weighted_mean`` — 加权平均值
- ``weighted_sum`` — 加权总和
- ``weighted_stdev`` — 加权标准差
- ``weighted_variance`` — 加权方差
- ``weighted_frac`` — 加权比例
- ``weights`` — 权重总和

选项说明
=============

- ``STATS`` — 逗号分隔的统计量列表，或 ``ALL``（全部）/ ``NONE``
- ``STRATEGY`` — 处理策略：``FEATURE_SEQUENTIAL`` （遍历区域）或 ``RASTER_SEQUENTIAL`` （遍历栅格）
- ``PIXEL_INTERSECTION`` — 像素包含方式：``DEFAULT``、``ALL_TOUCHED`` 或 ``FRACTIONAL``
- ``ZONES_BAND`` — 区域栅格的波段号
- ``ZONES_LAYER`` — 区域矢量的图层名
- ``WEIGHTS_BAND`` — 权重栅格的波段号
- ``INCLUDE_GEOM`` — 是否在输出中包含区域几何（YES/NO）
- ``OUTPUT_LAYER`` — 输出图层名

代码示例
=============

.. code-block:: c++

    #include "gdal.h"
    #include "gdal_alg.h"

    void ZonalStatsExample()
    {
        GDALAllRegister();

        // 打开源栅格（待统计的值）
        GDALDatasetH hSrcDS = GDALOpen("temperature.tif", GA_ReadOnly);
        if (hSrcDS == NULL)
        {
            printf("无法打开源栅格\n");
            return;
        }

        // 打开区域栅格（定义分区）
        GDALDatasetH hZonesDS = GDALOpen("landuse_zones.tif", GA_ReadOnly);
        if (hZonesDS == NULL)
        {
            printf("无法打开区域栅格\n");
            GDALClose(hSrcDS);
            return;
        }

        // 创建输出矢量数据集
        GDALDriverH hDriver = GDALGetDriverByName("ESRI Shapefile");
        GDALDatasetH hOutDS = GDALCreate(hDriver, "zonal_stats.shp",
                                          0, 0, 0, GDT_Unknown, NULL);

        // 设置选项
        char **papszOptions = NULL;
        papszOptions = CSLSetNameValue(papszOptions,
            "STATS", "count,min,max,mean,stdev,sum");
        papszOptions = CSLSetNameValue(papszOptions,
            "INCLUDE_GEOM", "YES");

        // 执行区域统计
        CPLErr err = GDALZonalStats(
            hSrcDS,
            NULL,       // 不使用权重
            hZonesDS,
            hOutDS,
            papszOptions,
            NULL, NULL
        );

        CSLDestroy(papszOptions);
        GDALClose(hOutDS);
        GDALClose(hZonesDS);
        GDALClose(hSrcDS);
    }

.. note::

    - GDAL 3.13 引入了此 API，早期版本需要使用 Python 或命令行工具
    - ``FRACTIONAL`` 像素包含模式需要 GEOS 3.14 或更高版本
    - 当区域数据集为矢量时，支持 ``STRATEGY`` 选项选择处理策略

.. _gdal3-pansharpen:

**********************
Pansharpen（全色锐化）
**********************

全色锐化将高分辨率全色波段与低分辨率多波段影像融合，生成高分辨率的多波段影像。GDAL 使用加权 Brovey 变换（Weighted Brovey Transform）。

函数原型
=============

.. code-block:: c++

    #include "gdalpansharpen.h"

    // 创建全色锐化选项
    GDALPansharpenOptions* GDALCreatePansharpenOptions(void);
    void GDALDestroyPansharpenOptions(GDALPansharpenOptions *);
    GDALPansharpenOptions* GDALClonePansharpenOptions(
        const GDALPansharpenOptions *psOptions);

    // 创建全色锐化操作
    GDALPansharpenOperationH GDALCreatePansharpenOperation(
        const GDALPansharpenOptions *);
    void GDALDestroyPansharpenOperation(GDALPansharpenOperationH);

    // 执行区域处理
    CPLErr GDALPansharpenProcessRegion(
        GDALPansharpenOperationH hOperation,
        int nXOff, int nYOff,       // 区域偏移
        int nXSize, int nYSize,     // 区域大小
        void *pDataBuf,             // 输出缓冲区
        GDALDataType eBufDataType   // 输出数据类型
    );

选项结构体
=============

``GDALPansharpenOptions`` 结构体关键字段：

- ``ePansharpenAlg`` — 算法类型（目前仅 ``GDAL_PSH_WEIGHTED_BROVEY``）
- ``eResampleAlg`` — 上采样重采样算法
- ``nBitDepth`` — 波段位深（0表示默认）
- ``nWeightCount`` / ``padfWeights`` — 权重系数
- ``hPanchroBand`` — 全色波段
- ``nInputSpectralBands`` / ``pahInputSpectralBands`` — 输入多光谱波段
- ``nOutPansharpenedBands`` / ``panOutPansharpenedBands`` — 输出波段映射
- ``bHasNoData`` / ``dfNoData`` — NoData 设置
- ``nThreads`` — 并行线程数（-1表示使用所有CPU）

代码示例
=============

.. code-block:: c++

    #include "gdal.h"
    #include "gdalpansharpen.h"

    void PansharpenExample()
    {
        GDALAllRegister();

        // 打开全色波段（高分辨率）
        GDALDatasetH hPanDS = GDALOpen("panchromatic.tif", GA_ReadOnly);
        GDALRasterBandH hPanBand = GDALGetRasterBand(hPanDS, 1);

        // 打开多光谱影像（低分辨率）
        GDALDatasetH hSpectralDS = GDALOpen("multispectral.tif", GA_ReadOnly);
        int nSpectralBands = GDALGetRasterCount(hSpectralDS);

        // 配置全色锐化选项
        GDALPansharpenOptions *psOptions = GDALCreatePansharpenOptions();
        psOptions->ePansharpenAlg = GDAL_PSH_WEIGHTED_BROVEY;
        psOptions->eResampleAlg = GRIORA_Bilinear;
        psOptions->hPanchroBand = hPanBand;
        psOptions->nInputSpectralBands = nSpectralBands;
        psOptions->pahInputSpectralBands = (GDALRasterBandH *)
            CPLMalloc(sizeof(GDALRasterBandH) * nSpectralBands);
        for (int i = 0; i < nSpectralBands; i++)
        {
            psOptions->pahInputSpectralBands[i] =
                GDALGetRasterBand(hSpectralDS, i + 1);
        }
        psOptions->nOutPansharpenedBands = nSpectralBands;
        psOptions->panOutPansharpenedBands = (int *)
            CPLMalloc(sizeof(int) * nSpectralBands);
        for (int i = 0; i < nSpectralBands; i++)
        {
            psOptions->panOutPansharpenedBands[i] = i;
        }

        // 设置权重（可选）
        psOptions->nWeightCount = nSpectralBands;
        psOptions->padfWeights = (double *)
            CPLMalloc(sizeof(double) * nSpectralBands);
        for (int i = 0; i < nSpectralBands; i++)
        {
            psOptions->padfWeights[i] = 1.0 / nSpectralBands;  // 等权重
        }

        psOptions->nThreads = -1;  // 使用所有CPU核心

        // 创建操作
        GDALPansharpenOperationH hOp =
            GDALCreatePansharpenOperation(psOptions);

        // 获取全色波段尺寸
        int nPanWidth = GDALGetRasterXSize(hPanDS);
        int nPanHeight = GDALGetRasterYSize(hPanDS);

        // 分配输出缓冲区（多波段交错存储）
        GByte *pabyBuffer = (GByte *)CPLMalloc(
            nPanWidth * nPanHeight * nSpectralBands * sizeof(GByte));

        // 执行全色锐化
        CPLErr err = GDALPansharpenProcessRegion(
            hOp, 0, 0, nPanWidth, nPanHeight,
            pabyBuffer, GDT_Byte);

        // 清理
        CPLFree(pabyBuffer);
        GDALDestroyPansharpenOperation(hOp);
        CPLFree(psOptions->pahInputSpectralBands);
        CPLFree(psOptions->panOutPansharpenedBands);
        CPLFree(psOptions->padfWeights);
        GDALDestroyPansharpenOptions(psOptions);
        GDALClose(hSpectralDS);
        GDALClose(hPanDS);
    }

.. _gdal3-colorquant:

********************
颜色量化
********************

颜色量化将 RGB 影像转换为调色板影像。GDAL 提供两个函数：

- ``GDALComputeMedianCutPCT()`` — 使用中位切分法生成调色板
- ``GDALDitherRGB2PCT()`` — 使用 Floyd-Steinberg 误差扩散抖动

函数原型
=============

.. code-block:: c++

    // 生成调色板
    int GDALComputeMedianCutPCT(
        GDALRasterBandH hRed,            // 红色波段
        GDALRasterBandH hGreen,          // 绿色波段
        GDALRasterBandH hBlue,           // 蓝色波段
        int (*pfnIncludePixel)(int, int, void *), // 像素过滤回调
        int nColors,                     // 目标颜色数（最大256）
        GDALColorTableH hColorTable,     // 输出调色板
        GDALProgressFunc pfnProgress,
        void *pProgressArg
    );

    // Floyd-Steinberg 抖动
    int GDALDitherRGB2PCT(
        GDALRasterBandH hRed,
        GDALRasterBandH hGreen,
        GDALRasterBandH hBlue,
        GDALRasterBandH hTarget,         // 输出索引波段
        GDALColorTableH hColorTable,     // 调色板
        GDALProgressFunc pfnProgress,
        void *pProgressArg
    );

代码示例
=============

.. code-block:: c++

    #include "gdal.h"
    #include "gdal_alg.h"

    void ColorQuantizationExample()
    {
        GDALAllRegister();

        // 打开 RGB 影像
        GDALDatasetH hSrcDS = GDALOpen("rgb_image.tif", GA_ReadOnly);
        if (hSrcDS == NULL)
        {
            printf("无法打开影像\n");
            return;
        }

        GDALRasterBandH hRed   = GDALGetRasterBand(hSrcDS, 1);
        GDALRasterBandH hGreen = GDALGetRasterBand(hSrcDS, 2);
        GDALRasterBandH hBlue  = GDALGetRasterBand(hSrcDS, 3);

        int nWidth = GDALGetRasterXSize(hSrcDS);
        int nHeight = GDALGetRasterYSize(hSrcDS);

        // 创建调色板
        GDALColorTableH hColorTable = GDALCreateColorTable(GPI_RGB);

        // 第一步：使用中位切分法生成调色板（256色）
        GDALComputeMedianCutPCT(
            hRed, hGreen, hBlue,
            NULL,           // 不过滤像素
            256,            // 256色
            hColorTable,
            NULL, NULL
        );

        // 第二步：使用 Floyd-Steinberg 抖动生成索引影像
        GDALDriverH hDriver = GDALGetDriverByName("GTiff");
        GDALDatasetH hDstDS = GDALCreate(hDriver, "quantized.tif",
                                          nWidth, nHeight, 1, GDT_Byte, NULL);

        // 设置调色板
        GDALRasterBandH hDstBand = GDALGetRasterBand(hDstDS, 1);
        GDALSetRasterColorTable(hDstBand, hColorTable);
        GDALSetRasterColorInterpretation(hDstBand, GCI_PaletteIndex);

        // 执行抖动
        GDALDitherRGB2PCT(
            hRed, hGreen, hBlue,
            hDstBand,
            hColorTable,
            NULL, NULL
        );

        GDALDestroyColorTable(hColorTable);
        GDALClose(hDstDS);
        GDALClose(hSrcDS);
    }

.. _gdal3-checksum:

********************
校验和（Checksum）
********************

``GDALChecksumImage()`` 计算栅格波段指定区域的校验和，常用于数据完整性验证和自动化测试。

函数原型
=============

.. code-block:: c++

    int GDALChecksumImage(
        GDALRasterBandH hBand,   // 输入波段
        int nXOff,               // 起始X偏移
        int nYOff,               // 起始Y偏移
        int nXSize,              // 宽度
        int nYSize               // 高度
    );

    // 返回值：校验和（整数），失败返回-1

代码示例
=============

.. code-block:: c++

    #include "gdal.h"
    #include "gdal_alg.h"

    void ChecksumExample()
    {
        GDALAllRegister();

        GDALDatasetH hDS = GDALOpen("test.tif", GA_ReadOnly);
        if (hDS == NULL) return;

        GDALRasterBandH hBand = GDALGetRasterBand(hDS, 1);

        // 计算整个波段的校验和
        int nWidth = GDALGetRasterXSize(hDS);
        int nHeight = GDALGetRasterYSize(hDS);
        int nChecksum = GDALChecksumImage(hBand, 0, 0, nWidth, nHeight);
        printf("全图校验和: %d\n", nChecksum);

        // 计算子区域的校验和
        int nSubChecksum = GDALChecksumImage(hBand, 100, 100, 256, 256);
        printf("子区域校验和: %d\n", nSubChecksum);

        GDALClose(hDS);
    }

.. _gdal3-hilbert:

********************
Hilbert 编码
********************

``GDALHilbertCode()`` 计算指定点在给定空间范围内的 Hilbert 空间填充曲线编码。Hilbert 编码可以将二维空间映射为一维索引，保持空间局部性，适用于空间索引和数据组织。

函数原型
=============

.. code-block:: c++

    #include "ogr_core.h"  // OGREnvelope

    uint32_t GDALHilbertCode(
        const OGREnvelope *poDomain,  // 空间范围（域）
        double dfX,                   // X坐标
        double dfY                    // Y坐标
    );

代码示例
=============

.. code-block:: c++

    #include "gdal.h"
    #include "gdal_alg.h"
    #include "ogr_core.h"

    void HilbertCodeExample()
    {
        // 定义空间范围
        OGREnvelope sDomain;
        sDomain.MinX = 0.0;
        sDomain.MaxX = 1000.0;
        sDomain.MinY = 0.0;
        sDomain.MaxY = 1000.0;

        // 计算几个点的 Hilbert 编码
        uint32_t code1 = GDALHilbertCode(&sDomain, 100.0, 200.0);
        uint32_t code2 = GDALHilbertCode(&sDomain, 500.0, 500.0);
        uint32_t code3 = GDALHilbertCode(&sDomain, 800.0, 300.0);

        printf("点(100,200)的Hilbert编码: %u\n", code1);
        printf("点(500,500)的Hilbert编码: %u\n", code2);
        printf("点(800,300)的Hilbert编码: %u\n", code3);

        // 可以按 Hilbert 编码排序，实现空间有序访问
    }

.. _gdal3-transformers:

****************************
坐标变换框架（Transformers）
****************************

GDAL 的坐标变换框架是 ``alg/`` 模块的核心基础设施，为 Warp（重投影/几何校正）等操作提供坐标转换能力。所有变换器共享统一的函数签名：

.. code-block:: c++

    typedef int (*GDALTransformerFunc)(
        void *pTransformerArg,   // 变换器参数
        int bDstToSrc,           // 方向：TRUE=目标→源，FALSE=源→目标
        int nPointCount,         // 点数量
        double *x, double *y, double *z,  // 坐标数组
        int *panSuccess          // 输出：每个点是否成功
    );

可用变换器类型
==============

**GenImgProj（通用影像投影变换器）**
    最常用的变换器，综合了 GeoTransform、GCP、RPC 等多种变换能力。

.. code-block:: c++

    // 方式1：基于数据集创建
    void* GDALCreateGenImgProjTransformer(
        GDALDatasetH hSrcDS, const char *pszSrcWKT,
        GDALDatasetH hDstDS, const char *pszDstWKT,
        int bGCPUseOK, double dfGCPErrorThreshold, int nOrder);

    // 方式2：使用选项创建
    void* GDALCreateGenImgProjTransformer2(
        GDALDatasetH hSrcDS, GDALDatasetH hDstDS, CSLConstList papszOptions);

    // 方式3：基于WKT和GeoTransform创建
    void* GDALCreateGenImgProjTransformer3(
        const char *pszSrcWKT, const double *padfSrcGeoTransform,
        const char *pszDstWKT, const double *padfDstGeoTransform);

    // 方式4：基于OGRSpatialReference创建（推荐）
    void* GDALCreateGenImgProjTransformer4(
        OGRSpatialReferenceH hSrcSRS, const double *padfSrcGeoTransform,
        OGRSpatialReferenceH hDstSRS, const double *padfDstGeoTransform,
        const char *const *papszOptions);

    void GDALDestroyGenImgProjTransformer(void *);
    int GDALGenImgProjTransform(void *pTransformArg, int bDstToSrc,
                                 int nPointCount, double *x, double *y,
                                 double *z, int *panSuccess);

**Reprojection（重投影变换器）**
    纯地理坐标重投影，不涉及像素坐标。

.. code-block:: c++

    void* GDALCreateReprojectionTransformer(
        const char *pszSrcWKT, const char *pszDstWKT);
    void* GDALCreateReprojectionTransformerEx(
        OGRSpatialReferenceH hSrcSRS, OGRSpatialReferenceH hDstSRS,
        const char *const *papszOptions);
    void GDALDestroyReprojectionTransformer(void *);
    int GDALReprojectionTransform(void *pTransformArg, int bDstToSrc,
                                   int nPointCount, double *x, double *y,
                                   double *z, int *panSuccess);

**GCP（地面控制点变换器）**
    基于多项式的 GCP 变换。

.. code-block:: c++

    void* GDALCreateGCPTransformer(
        int nGCPCount, const GDAL_GCP *pasGCPList,
        int nReqOrder, int bReversed);
    // nReqOrder: 1=仿射, 2=二次, 3=三次

    void* GDALCreateGCPRefineTransformer(
        int nGCPCount, const GDAL_GCP *pasGCPList,
        int nReqOrder, int bReversed,
        double tolerance, int minimumGcps);
    // 带 GCP 精炼的变换器

    void GDALDestroyGCPTransformer(void *);
    int GDALGCPTransform(void *pTransformArg, int bDstToSrc,
                          int nPointCount, double *x, double *y,
                          double *z, int *panSuccess);

**TPS（薄板样条变换器）**
    基于薄板样条的精确变换，GCP 点精确通过。

.. code-block:: c++

    void* GDALCreateTPSTransformer(
        int nGCPCount, const GDAL_GCP *pasGCPList, int bReversed);
    void GDALDestroyTPSTransformer(void *);
    int GDALTPSTransform(void *pTransformArg, int bDstToSrc,
                          int nPointCount, double *x, double *y,
                          double *z, int *panSuccess);

**RPC（有理多项式系数变换器）**
    基于 RPC 模型的变换，常用于卫星影像。

.. code-block:: c++

    void* GDALCreateRPCTransformerV2(
        const GDALRPCInfoV2 *psRPC, int bReversed,
        double dfPixErrThreshold, CSLConstList papszOptions);
    void GDALDestroyRPCTransformer(void *);
    int GDALRPCTransform(void *pTransformArg, int bDstToSrc,
                          int nPointCount, double *x, double *y,
                          double *z, int *panSuccess);

**GeoLoc（地理定位变换器）**
    基于地理定位数组（如 MODIS 数据）的变换。

.. code-block:: c++

    void* GDALCreateGeoLocTransformer(
        GDALDatasetH hBaseDS, char **papszGeolocationInfo, int bReversed);
    void GDALDestroyGeoLocTransformer(void *);
    int GDALGeoLocTransform(void *pTransformArg, int bDstToSrc,
                             int nPointCount, double *x, double *y,
                             double *z, int *panSuccess);

**Homography（单应变换器）**
    基于 3x3 单应矩阵的变换。

.. code-block:: c++

    void* GDALCreateHomographyTransformer(double adfHomography[9]);
    void* GDALCreateHomographyTransformerFromGCPs(
        int nGCPCount, const GDAL_GCP *pasGCPList);
    void GDALDestroyHomographyTransformer(void *);
    int GDALHomographyTransform(void *pTransformArg, int bDstToSrc,
                                 int nPointCount, double *x, double *y,
                                 double *z, int *panSuccess);

**Approx（近似变换器）**
    包装其他变换器，通过线性近似减少调用次数，提高性能。

.. code-block:: c++

    void* GDALCreateApproxTransformer(
        GDALTransformerFunc pfnRawTransformer,  // 原始变换器函数
        void *pRawTransformerArg,               // 原始变换器参数
        double dfMaxError);                     // 最大允许误差（像素）
    void GDALDestroyApproxTransformer(void *);
    int GDALApproxTransform(void *pTransformArg, int bDstToSrc,
                             int nPointCount, double *x, double *y,
                             double *z, int *panSuccess);

**GDALGenericInverse2D — 通用二维逆变换**
    使用 Newton-Raphson 迭代法求解正向变换的逆变换。内部函数，位于 ``gdalgenericinverse.h``。

.. code-block:: c++

    // 正向变换函数类型
    typedef bool (*GDALForwardCoordTransformer)(
        double xIn, double yIn,
        double &xOut, double &yOut,
        void *pUserData);

    // Newton-Raphson 逆变换
    bool GDALGenericInverse2D(
        double xIn, double yIn,
        double guessedXOut, double guessedYOut,
        GDALForwardCoordTransformer pfnForwardTransformer,
        void *pForwardTransformerUserData,
        double &xOut, double &yOut);

代码示例：使用 GenImgProj 变换器进行 Warp
=========================================

.. code-block:: c++

    #include "gdal.h"
    #include "gdal_alg.h"
    #include "gdalwarper.h"

    void WarpWithGenImgProjExample()
    {
        GDALAllRegister();

        // 打开源数据集
        GDALDatasetH hSrcDS = GDALOpen("source.tif", GA_ReadOnly);
        if (hSrcDS == NULL)
        {
            printf("无法打开源数据集\n");
            return;
        }

        // 创建 GenImgProj 变换器（源→目标的坐标变换）
        void *hTransformArg = GDALCreateGenImgProjTransformer(
            hSrcDS,                     // 源数据集
            GDALGetProjectionRef(hSrcDS), // 源投影WKT
            NULL,                       // 目标数据集（NULL表示自动计算输出范围）
            "+proj=merc +datum=WGS84",  // 目标投影（Mercator）
            FALSE,                      // 不使用GCP
            0.0,                        // GCP误差阈值
            0                           // 多项式阶数
        );

        if (hTransformArg == NULL)
        {
            printf("创建变换器失败\n");
            GDALClose(hSrcDS);
            return;
        }

        // 使用变换器计算建议的输出范围和大小
        double adfGeoTransform[6];
        int nPixels, nLines;
        CPLErr err = GDALSuggestedWarpOutput(
            hSrcDS,
            GDALGenImgProjTransform,
            hTransformArg,
            adfGeoTransform,
            &nPixels,
            &nLines
        );

        if (err == CE_None)
        {
            printf("建议输出大小: %d x %d\n", nPixels, nLines);
            printf("输出GeoTransform: %.6f, %.6f, %.6f, %.6f, %.6f, %.6f\n",
                   adfGeoTransform[0], adfGeoTransform[1], adfGeoTransform[2],
                   adfGeoTransform[3], adfGeoTransform[4], adfGeoTransform[5]);
        }

        // 清理
        GDALDestroyGenImgProjTransformer(hTransformArg);
        GDALClose(hSrcDS);
    }

代码示例：使用 Approx 变换器加速
================================

.. code-block:: c++

    void ApproxTransformerExample()
    {
        GDALAllRegister();

        GDALDatasetH hSrcDS = GDALOpen("source.tif", GA_ReadOnly);

        // 创建原始 GenImgProj 变换器
        void *hGenTransformArg = GDALCreateGenImgProjTransformer(
            hSrcDS, NULL, NULL, "+proj=longlat +datum=WGS84",
            FALSE, 0.0, 0);

        // 包装为近似变换器，最大误差0.1像素
        void *hApproxArg = GDALCreateApproxTransformer(
            GDALGenImgProjTransform,  // 原始变换函数
            hGenTransformArg,         // 原始变换参数
            0.1                       // 最大误差：0.1像素
        );

        // 近似变换器接管了原始变换器的生命周期
        GDALApproxTransformerOwnsSubtransformer(hApproxArg, TRUE);

        // 使用近似变换器进行坐标变换（速度更快）
        double x = 100.0, y = 200.0, z = 0.0;
        int bSuccess = 0;
        GDALApproxTransform(hApproxArg, FALSE, 1, &x, &y, &z, &bSuccess);

        if (bSuccess)
            printf("变换后坐标: (%.6f, %.6f)\n", x, y);

        // 清理（近似变换器会自动清理原始变换器）
        GDALDestroyApproxTransformer(hApproxArg);
        GDALClose(hSrcDS);
    }

.. _gdal3-dem:

********************
DEM 分析
********************

DEM 分析函数通过 ``GDALDEMProcessing()`` 统一接口提供，支持以下分析类型：

- **Hillshade** — 山体阴影（模拟光照效果）
- **Slope** — 坡度
- **Aspect** — 坡向
- **TPI** — 地形位置指数（Topographic Position Index）
- **TRI** — 地形粗糙度指数（Terrain Ruggedness Index）
- **Roughness** — 粗糙度

算法原理
=============

DEM 分析基于 3x3 滑动窗口，使用 Horn 公式计算中心像素的梯度。对于窗口中的 9 个像素：

::

    a  b  c
    d  e  f    (e 为中心像素)
    g  h  i

X 方向梯度: dz/dx = ((c + 2f + i) - (a + 2d + g)) / (8 * cellsize)
Y 方向梯度: dz/dy = ((g + 2h + i) - (a + 2b + c)) / (8 * cellsize)

函数原型
=============

.. code-block:: c++

    #include "gdal_utils.h"

    typedef struct GDALDEMProcessingOptions GDALDEMProcessingOptions;

    GDALDEMProcessingOptions* GDALDEMProcessingOptionsNew(
        char **papszArgv,
        GDALDEMProcessingOptionsForBinary *psOptionsForBinary);
    void GDALDEMProcessingOptionsFree(GDALDEMProcessingOptions *psOptions);

    GDALDatasetH GDALDEMProcessing(
        const char *pszDestFilename,     // 输出文件名
        GDALDatasetH hSrcDataset,        // 输入DEM数据集
        const char *pszProcessing,       // 分析类型
        const char *pszColorFilename,    // 颜色文件（hillshade可为NULL）
        const GDALDEMProcessingOptions *psOptions,
        int *pbUsageError
    );

    // pszProcessing 取值:
    //   "hillshade" — 山体阴影
    //   "slope"     — 坡度
    //   "aspect"    — 坡向
    //   "color-relief" — 颜色渲染
    //   "TRI"       — 地形粗糙度指数
    //   "TPI"       — 地形位置指数
    //   "roughness" — 粗糙度

代码示例
=============

.. code-block:: c++

    #include "gdal.h"
    #include "gdal_utils.h"

    void DEMAnalysisExample()
    {
        GDALAllRegister();

        GDALDatasetH hSrcDS = GDALOpen("dem.tif", GA_ReadOnly);
        if (hSrcDS == NULL)
        {
            printf("无法打开 DEM\n");
            return;
        }

        int bUsageError = FALSE;

        // 1. 生成山体阴影
        {
            char *papszArgv[] = {
                (char*)"-z", (char*)"1.0",     // 垂直夸张系数
                (char*)"-s", (char*)"111120",  // 像素间距（度→米）
                (char*)"-az", (char*)"315",    // 太阳方位角
                (char*)"-alt", (char*)"45",    // 太阳高度角
                NULL
            };
            GDALDEMProcessingOptions *psOpts =
                GDALDEMProcessingOptionsNew(papszArgv, NULL);

            GDALDatasetH hDstDS = GDALDEMProcessing(
                "hillshade.tif", hSrcDS, "hillshade", NULL, psOpts, &bUsageError);

            if (hDstDS != NULL)
                GDALClose(hDstDS);
            GDALDEMProcessingOptionsFree(psOpts);
        }

        // 2. 计算坡度
        {
            char *papszArgv[] = {
                (char*)"-s", (char*)"111120",  // 像素间距
                NULL
            };
            GDALDEMProcessingOptions *psOpts =
                GDALDEMProcessingOptionsNew(papszArgv, NULL);

            GDALDatasetH hDstDS = GDALDEMProcessing(
                "slope.tif", hSrcDS, "slope", NULL, psOpts, &bUsageError);

            if (hDstDS != NULL)
                GDALClose(hDstDS);
            GDALDEMProcessingOptionsFree(psOpts);
        }

        // 3. 计算坡向
        {
            char *papszArgv[] = { NULL };
            GDALDEMProcessingOptions *psOpts =
                GDALDEMProcessingOptionsNew(papszArgv, NULL);

            GDALDatasetH hDstDS = GDALDEMProcessing(
                "aspect.tif", hSrcDS, "aspect", NULL, psOpts, &bUsageError);

            if (hDstDS != NULL)
                GDALClose(hDstDS);
            GDALDEMProcessingOptionsFree(psOpts);
        }

        // 4. 计算 TPI（地形位置指数）
        {
            char *papszArgv[] = { NULL };
            GDALDEMProcessingOptions *psOpts =
                GDALDEMProcessingOptionsNew(papszArgv, NULL);

            GDALDatasetH hDstDS = GDALDEMProcessing(
                "tpi.tif", hSrcDS, "TPI", NULL, psOpts, &bUsageError);

            if (hDstDS != NULL)
                GDALClose(hDstDS);
            GDALDEMProcessingOptionsFree(psOpts);
        }

        // 5. 计算 TRI（地形粗糙度指数）
        {
            char *papszArgv[] = { NULL };
            GDALDEMProcessingOptions *psOpts =
                GDALDEMProcessingOptionsNew(papszArgv, NULL);

            GDALDatasetH hDstDS = GDALDEMProcessing(
                "tri.tif", hSrcDS, "TRI", NULL, psOpts, &bUsageError);

            if (hDstDS != NULL)
                GDALClose(hDstDS);
            GDALDEMProcessingOptionsFree(psOpts);
        }

        // 6. 计算粗糙度
        {
            char *papszArgv[] = { NULL };
            GDALDEMProcessingOptions *psOpts =
                GDALDEMProcessingOptionsNew(papszArgv, NULL);

            GDALDatasetH hDstDS = GDALDEMProcessing(
                "roughness.tif", hSrcDS, "roughness", NULL, psOpts, &bUsageError);

            if (hDstDS != NULL)
                GDALClose(hDstDS);
            GDALDEMProcessingOptionsFree(psOpts);
        }

        GDALClose(hSrcDS);
    }

.. note::

    - ``-s`` 参数在地理坐标系（度）下通常设为 111120（赤道上1度约111120米），在投影坐标系下设为1
    - ``-z`` 垂直夸张系数设为大于1的值可以增强地形效果
    - TPI 计算中心像素与周围像素平均值的差
    - TRI 计算中心像素与周围像素差值的均方根
    - Roughness 计算窗口内最大值与最小值之差

.. _gdal3-bandarithmetic:

****************************
波段代数（Band Arithmetic）
****************************

GDAL 3.12 引入了 C API 波段代数接口，支持对栅格波段进行算术运算、数学函数、比较运算和逻辑运算，生成延迟计算的 ``GDALComputedRasterBandH`` 对象。

.. note::

    此功能为 GDAL 3.12 新增，需要 ``#include "gdal.h"`` 且链接 GDAL 3.12+ 版本。

一元运算
=============

.. code-block:: c++

    typedef enum
    {
        GRAUO_LOGICAL_NOT,  // 逻辑非
        GRAUO_ABS,          // 绝对值（复数类型为模）
        GRAUO_SQRT,         // 平方根
        GRAUO_LOG,          // 自然对数（ln）
        GRAUO_LOG10         // 以10为底的对数
    } GDALRasterAlgebraUnaryOperation;

    GDALComputedRasterBandH GDALRasterBandUnaryOp(
        GDALRasterBandH hBand,
        GDALRasterAlgebraUnaryOperation eOp);

二元运算
=============

.. code-block:: c++

    typedef enum
    {
        GRABO_ADD,           // 加法
        GRABO_SUB,           // 减法
        GRABO_MUL,           // 乘法
        GRABO_DIV,           // 除法
        GRABO_POW,           // 幂运算
        GRABO_GT,            // 大于
        GRABO_GE,            // 大于等于
        GRABO_LT,            // 小于
        GRABO_LE,            // 小于等于
        GRABO_EQ,            // 等于
        GRABO_NE,            // 不等于
        GRABO_LOGICAL_AND,   // 逻辑与
        GRABO_LOGICAL_OR     // 逻辑或
    } GDALRasterAlgebraBinaryOperation;

    // 波段与波段运算
    GDALComputedRasterBandH GDALRasterBandBinaryOpBand(
        GDALRasterBandH hBand,
        GDALRasterAlgebraBinaryOperation eOp,
        GDALRasterBandH hOtherBand);

    // 波段与常数运算
    GDALComputedRasterBandH GDALRasterBandBinaryOpDouble(
        GDALRasterBandH hBand,
        GDALRasterAlgebraBinaryOperation eOp,
        double constant);

    // 常数与波段运算（常数在左侧）
    GDALComputedRasterBandH GDALRasterBandBinaryOpDoubleToBand(
        double constant,
        GDALRasterAlgebraBinaryOperation eOp,
        GDALRasterBandH hBand);

条件运算与类型转换
==================

.. code-block:: c++

    // 条件运算：If(condition, then, else)
    GDALComputedRasterBandH GDALRasterBandIfThenElse(
        GDALRasterBandH hCondBand,   // 条件波段（非零为真）
        GDALRasterBandH hThenBand,   // 条件为真时的值
        GDALRasterBandH hElseBand);  // 条件为假时的值

    // 数据类型转换
    GDALComputedRasterBandH GDALRasterBandAsDataType(
        GDALRasterBandH hBand,
        GDALDataType eDT);

多波段归约运算
==============

.. code-block:: c++

    // 多波段最大值
    GDALComputedRasterBandH GDALMaximumOfNBands(
        size_t nBandCount, GDALRasterBandH *pahBands);

    // 波段与常数取最大值
    GDALComputedRasterBandH GDALRasterBandMaxConstant(
        GDALRasterBandH hBand, double dfConstant);

    // 多波段最小值
    GDALComputedRasterBandH GDALMinimumOfNBands(
        size_t nBandCount, GDALRasterBandH *pahBands);

    // 波段与常数取最小值
    GDALComputedRasterBandH GDALRasterBandMinConstant(
        GDALRasterBandH hBand, double dfConstant);

    // 多波段平均值
    GDALComputedRasterBandH GDALMeanOfNBands(
        size_t nBandCount, GDALRasterBandH *pahBands);

释放计算波段
=============

.. code-block:: c++

    // 使用完毕后必须释放
    void GDALComputedRasterBandRelease(GDALComputedRasterBandH hBand);

代码示例
=============

.. code-block:: c++

    #include "gdal.h"

    void BandArithmeticExample()
    {
        GDALAllRegister();

        GDALDatasetH hDS = GDALOpen("multiband.tif", GA_ReadOnly);
        if (hDS == NULL) return;

        GDALRasterBandH hRed   = GDALGetRasterBand(hDS, 1);
        GDALRasterBandH hNIR   = GDALGetRasterBand(hDS, 2);

        // 计算 NDVI = (NIR - Red) / (NIR + Red)
        // 步骤1: NIR - Red
        GDALComputedRasterBandH hDiff =
            GDALRasterBandBinaryOpBand(hNIR, GRABO_SUB, hRed);

        // 步骤2: NIR + Red
        GDALComputedRasterBandH hSum =
            GDALRasterBandBinaryOpBand(hNIR, GRABO_ADD, hRed);

        // 步骤3: (NIR - Red) / (NIR + Red)
        GDALComputedRasterBandH hNDVI =
            GDALRasterBandBinaryOpBand(hDiff, GRABO_DIV, hSum);

        // 限制 NDVI 范围到 [-1, 1]
        GDALComputedRasterBandH hNDVIClampMin =
            GDALRasterBandMaxConstant(hNDVI, -1.0);
        GDALComputedRasterBandH hNDVIClampMax =
            GDALRasterBandMinConstant(hNDVIClampMin, 1.0);

        // 条件运算：NDVI > 0.3 为植被，否则为非植被
        GDALComputedRasterBandH hThreshold =
            GDALRasterBandBinaryOpDouble(hNDVIClampMax, GRABO_GT, 0.3);
        GDALComputedRasterBandH hConst1 =
            GDALRasterBandBinaryOpDouble(hNDVIClampMax, GRABO_ADD, 0.0);
        GDALRasterBandRelease(hConst1);
        hConst1 = GDALRasterBandBinaryOpDouble(hNDVIClampMax, GRABO_MUL, 0.0);
        GDALComputedRasterBandH hConst2 =
            GDALRasterBandBinaryOpDouble(hConst1, GRABO_ADD, 1.0);

        // 多波段归约：取三个波段的最大值
        GDALRasterBandH ahBands[3] = {
            GDALGetRasterBand(hDS, 1),
            GDALGetRasterBand(hDS, 2),
            GDALGetRasterBand(hDS, 3)
        };
        GDALComputedRasterBandH hMaxBand = GDALMaximumOfNBands(3, ahBands);

        // 释放所有计算波段
        GDALComputedRasterBandRelease(hDiff);
        GDALComputedRasterBandRelease(hSum);
        GDALComputedRasterBandRelease(hNDVI);
        GDALComputedRasterBandRelease(hNDVIClampMin);
        GDALComputedRasterBandRelease(hNDVIClampMax);
        GDALComputedRasterBandRelease(hThreshold);
        GDALComputedRasterBandRelease(hConst1);
        GDALComputedRasterBandRelease(hConst2);
        GDALComputedRasterBandRelease(hMaxBand);

        GDALClose(hDS);
    }

.. warning::

    - ``GDALComputedRasterBandH`` 是延迟计算的，只有在读取数据时才真正执行运算
    - 使用完毕后必须调用 ``GDALComputedRasterBandRelease()`` 释放资源
    - 链式运算会创建多个中间对象，注意及时释放避免内存泄漏
    - 比较运算和逻辑运算的结果为 0（假）或 1（真）

.. _gdal3-warp-api:

***********************
Warp（重投影/几何校正）
***********************

Warp 是 GDAL 最核心的算法之一，用于影像的重投影、几何校正、裁剪、拼接等操作。``alg/`` 目录下的 ``gdalwarper.cpp``、``gdalwarpkernel.cpp``、``gdalwarpoperation.cpp`` 实现了完整的 Warp 管线。

高层 API
=============

**GDALReprojectImage()** — 一步完成重投影：

.. code-block:: c++

    CPLErr GDALReprojectImage(
        GDALDatasetH hSrcDS, const char *pszSrcWKT,
        GDALDatasetH hDstDS, const char *pszDstWKT,
        GDALResampleAlg eResampleAlg,   // 重采样算法
        double dfWarpMemoryLimit,        // 内存限制（字节，0=自动）
        double dfMaxError,               // 近似变换器最大误差
        GDALProgressFunc pfnProgress, void *pProgressArg,
        GDALWarpOptions *psOptions       // 可选的额外选项
    );

    // 示例：将 WGS84 影像重投影到 UTM Zone 50N
    GDALReprojectImage(
        hSrcDS, NULL,                   // 源数据集，自动获取投影
        hDstDS, "+proj=utm +zone=50 +datum=WGS84",  // 目标投影
        GRA_Bilinear,                   // 双线性重采样
        0, 0, NULL, NULL, NULL          // 默认参数
    );

**GDALAutoCreateWarpedVRT()** — 创建虚拟 Warp 数据集（惰性求值）：

.. code-block:: c++

    // 自动计算输出范围和分辨率
    GDALDatasetH hWarpedDS = GDALAutoCreateWarpedVRT(
        hSrcDS,
        NULL,                           // 源投影（NULL=从数据集获取）
        "+proj=longlat +datum=WGS84",   // 目标投影
        GRA_NearestNeighbour,           // 重采样算法
        0,                              // 最大误差（0=精确）
        NULL                            // 额外选项
    );
    // hWarpedDS 可以像普通数据集一样读取，数据在读取时才真正变换

**GDALCreateWarpedVRT()** — 使用自定义几何创建虚拟 Warp：

.. code-block:: c++

    GDALWarpOptions *psWarpOptions = GDALCreateWarpOptions();
    psWarpOptions->hSrcDS = hSrcDS;
    psWarpOptions->nBandCount = 1;
    psWarpOptions->panSrcBands = (int *)CPLMalloc(sizeof(int));
    psWarpOptions->panSrcBands[0] = 1;
    psWarpOptions->panDstBands = (int *)CPLMalloc(sizeof(int));
    psWarpOptions->panDstBands[0] = 1;
    psWarpOptions->pfnTransformer = GDALGenImgProjTransform;
    psWarpOptions->pTransformerArg = GDALCreateGenImgProjTransformer(
        hSrcDS, NULL, NULL, NULL, FALSE, 0, 0);
    psWarpOptions->eResampleAlg = GRA_Cubic;

    GDALDatasetH hVRT = GDALCreateWarpedVRT(
        hSrcDS, nPixels, nLines, adfGeoTransform, psWarpOptions);

Warp 操作管线
=============

对于需要精细控制的场景，可以使用底层 Warp 操作管线：

.. code-block:: c++

    // 1. 创建 Warp 选项
    GDALWarpOptions *psOptions = GDALCreateWarpOptions();
    psOptions->eResampleAlg = GRA_Lanczos;
    psOptions->dfWarpMemoryLimit = 64 * 1024 * 1024;  // 64MB

    // 2. 设置源和目标波段
    psOptions->hSrcDS = hSrcDS;
    psOptions->hDstDS = hDstDS;
    psOptions->nBandCount = 3;
    psOptions->panSrcBands = (int *)CPLMalloc(3 * sizeof(int));
    psOptions->panDstBands = (int *)CPLMalloc(3 * sizeof(int));
    for (int i = 0; i < 3; i++) {
        psOptions->panSrcBands[i] = i + 1;
        psOptions->panDstBands[i] = i + 1;
    }

    // 3. 设置变换器
    psOptions->pfnTransformer = GDALGenImgProjTransform;
    psOptions->pTransformerArg = GDALCreateGenImgProjTransformer(
        hSrcDS, NULL, hDstDS, NULL, FALSE, 0, 0);

    // 4. 创建 Warp 操作并执行
    GDALWarpOperationH hOperation = GDALCreateWarpOperation(psOptions);
    GDALChunkAndWarpImage(hOperation, 0, 0, nPixels, nLines);
    GDALDestroyWarpOperation(hOperation);

    // 5. 清理
    GDALDestroyWarpOptions(psOptions);

重采样算法
=============

``GDALResampleAlg`` 枚举定义了所有可用的重采样算法：

- ``GRA_NearestNeighbour`` (0) — 最近邻，速度最快，适用于分类数据
- ``GRA_Bilinear`` (1) — 双线性插值，2x2 窗口
- ``GRA_Cubic`` (2) — 三次卷积，4x4 窗口
- ``GRA_CubicSpline`` (3) — 三次 B 样条，4x4 窗口
- ``GRA_Lanczos`` (4) — Lanczos 窗口化 sinc 函数，6x6 窗口
- ``GRA_Average`` (5) — 所有贡献像素的加权平均
- ``GRA_Mode`` (6) — 最频繁出现的值
- ``GRA_Max`` (8) — 最大值
- ``GRA_Min`` (9) — 最小值
- ``GRA_Med`` (10) — 中位数
- ``GRA_Q1`` (11) — 第一四分位数
- ``GRA_Q3`` (12) — 第三四分位数
- ``GRA_Sum`` (13) — 加权和
- ``GRA_RMS`` (14) — 均方根

Cutline 裁剪线
==============

Warp 支持使用多边形裁剪线（Cutline）进行不规则裁剪。``gdalcutline.cpp`` 使用 GEOS 进行多边形运算，生成沿裁剪边界的混合掩膜。

.. code-block:: c++

    // 在 Warp 选项中设置裁剪线
    psWarpOptions->hCutline = hCutlinePolygon;  // OGRGeometryH
    psWarpOptions->dfCutlineBlendDist = 10.0;   // 混合距离（像素）

.. _gdal3-delaunay:

********************
Delaunay 三角网
********************

GDAL 内置了基于 QHull 库的 Delaunay 三角网实现，用于散点插值和几何处理。API 位于 ``gdal_alg.h``。

.. code-block:: c++

    #include "gdal_alg.h"

    // 创建 Delaunay 三角网
    // padfX, padfY: 点坐标数组
    // nPoints: 点数量
    // pahOutTriangles: 输出三角形数组（每3个索引为一个三角形）
    // bIncludeEdges: 是否包含边界边
    GDALTriangulation *psTri = GDALTriangulationCreateDelaunay(
        nPoints, padfX, padfY);

    if (psTri != NULL)
    {
        // 计算重心坐标系数（用于快速插值）
        GDALTriangulationComputeBarycentricCoefficients(psTri, padfX, padfY);

        // 查询某点所在的三角形及其重心坐标
        double dfL1, dfL2, dfL3;
        int iFacet = GDALTriangulationFindFacetDirected(
            psTri, x, y, -1, &dfL1, dfL2, &dfL3);

        if (iFacet >= 0)
        {
            // 使用重心坐标进行插值
            // value = L1 * z[tri[3*iFacet]] + L2 * z[tri[3*iFacet+1]] + L3 * z[tri[3*iFacet+2]]
        }

        GDALTriangulationFree(psTri);
    }

三角网的典型应用：

- 散点数据的线性插值（``GDALGridLinear`` 算法内部使用）
- 不规则三角网 (TIN) 地形建模
- 几何约束剖分

.. _gdal3-matching:

********************
影像匹配（SURF）
********************

GDAL 提供了基于简化 SURF（Speeded Up Robust Features）算法的自动影像匹配功能，用于在两幅影像之间寻找同名点。API 位于 ``gdalmatching.cpp``。

.. code-block:: c++

    #include "gdal_alg.h"

    // 在两幅影像之间计算匹配点
    // 返回值为 GDAL_GCP 数组，包含匹配的控制点对
    int nGCPCount = 0;
    GDAL_GCP *pasGCPs = NULL;

    CPLErr err = GDALComputeMatchingPoints(
        hFirstDS,       // 第一幅影像
        hSecondDS,      // 第二幅影像
        &nGCPCount,     // 输出：匹配点数量
        &pasGCPs,       // 输出：匹配的 GCP 数组
        NULL            // 额外选项（暂未使用）
    );

    if (err == CE_None && nGCPCount > 0)
    {
        printf("找到 %d 个匹配点\n", nGCPCount);

        // 使用匹配点创建 TPS 变换器进行配准
        void *hTPSArg = GDALCreateTPSTransformer(nGCPCount, pasGCPs, FALSE);

        // ... 使用 TPS 变换器进行 Warp ...

        GDALDestroyTPSTransformer(hTPSArg);
        GDALDeinitGCPs(nGCPCount, pasGCPs);
        CPLFree(pasGCPs);
    }

.. note::

    SURF 匹配算法的特点：

    - 尺度不变性：能处理不同分辨率的影像
    - 旋转敏感：最大容忍约 10-15 度旋转
    - 适用于同源影像的自动配准
    - 对纹理丰富的区域效果较好，对水域等均匀区域效果差

.. _gdal3-geoloc-transform:

********************
地理定位数组变换
********************

``gdaltransformgeolocs.cpp`` 提供了对地理定位数组（Geolocation Arrays）进行变换的功能。地理定位数组是存储在栅格波段中的 X/Y/Z 坐标网格，常用于 MODIS、AVHRR 等卫星数据。

**GDALTransformGeolocations()** — 变换地理定位数组的坐标值：

.. code-block:: c++

    #include "gdal_alg.h"

    // 将地理定位数组从一个 CRS 变换到另一个 CRS
    CPLErr GDALTransformGeolocations(
        GDALRasterBandH hXBand,     // X 坐标波段（经度）
        GDALRasterBandH hYBand,     // Y 坐标波段（纬度）
        GDALRasterBandH hZBand,     // Z 坐标波段（高程，可选）
        GDALTransformerFunc pfnTransformer,
        void *pTransformerArg,
        GDALProgressFunc pfnProgress,
        void *pProgressArg
    );

与 ``GDALCreateGeoLocTransformer()``（用于 Warp 中的坐标映射）不同，``GDALTransformGeolocations()`` 直接修改地理定位数组的值，将其从源 CRS 转换到目标 CRS。

.. _gdal3-vertical-shift:

**********************
垂直偏移网格（已弃用）
**********************

``gdalapplyverticalshiftgrid.cpp`` 提供了基于大地水准面网格的高程校正功能，用于椭球高和正高之间的转换。

.. code-block:: c++

    // 打开大地水准面网格
    GDALDatasetH hGridDS = GDALOpenVerticalShiftGrid("egm96_15.gtx");

    // 应用垂直偏移（椭球高 → 正高）
    GDALDatasetH hResultDS = GDALApplyVerticalShiftGrid(
        hSrcDS, hGridDS,
        TRUE,           // bInverse: TRUE=正高→椭球高, FALSE=椭球高→正高
        0.0,            // dfSrcUnitToMeter
        0.0,            // dfDstUnitToMeter
        NULL            // papszOptions
    );

.. warning::

    此功能已在 GDAL 3.x 中标记为弃用，计划在 GDAL 4.0 中移除。
    推荐使用 PROJ 的 ``+geoidgrids`` 参数在坐标转换管线中直接处理垂直偏移。
