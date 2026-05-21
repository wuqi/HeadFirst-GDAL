.. highlight:: rst

.. _gdal3-srs-proj:

################################################
GDAL 3.x 空间参考与投影
################################################

GDAL 3.0 对空间参考系统（Spatial Reference System, SRS）进行了重大升级，底层从 PROJ 4/5 迁移到 PROJ 6+，引入了 WKT2:2019 支持、新的坐标轴顺序规则以及更精确的坐标转换管线。本章详细介绍 GDAL 3.x 中 ``OGRSpatialReference`` 和 ``OGRCoordinateTransformation`` 的使用方法。

****************************
OGRSpatialReference 创建方式
****************************

``OGRSpatialReference`` 是 GDAL/OGR 中用于表达坐标系统的核心类。GDAL 3.x 提供了多种方式来创建和初始化 SRS 对象。

importFromEPSG()
===================

通过 EPSG 代码创建 SRS 是最常用的方式。EPSG 数据库包含了全球绝大多数标准坐标系统的定义。

.. code-block:: c++

    #include "ogrsf_frmts.h"
    #include "ogr_spatialref.h"

    // 通过 EPSG 代码创建 WGS 84 地理坐标系
    OGRSpatialReference oSRS;
    OGRErr err = oSRS.importFromEPSG(4326);
    if (err != OGRERR_NONE)
    {
        printf("importFromEPSG 失败，错误码: %d\n", err);
    }

    // 通过 EPSG 代码创建 CGCS 2000 地理坐标系
    OGRSpatialReference oSRS_CGCS;
    oSRS_CGCS.importFromEPSG(4490);

    // 通过 EPSG 代码创建 Web 墨卡托投影
    OGRSpatialReference oSRS_3857;
    oSRS_3857.importFromEPSG(3857);

    // 通过 EPSG 代码创建 UTM Zone 50N 投影（WGS 84）
    OGRSpatialReference oSRS_UTM;
    oSRS_UTM.importFromEPSG(32650);

importFromWkt()
===================

通过 WKT（Well Known Text）字符串创建 SRS，适用于从文件元数据或其他数据源中获取的 WKT 定义。

.. code-block:: c++

    // 从 WKT 1 字符串创建 SRS
    const char* pszWkt =
        "GEOGCS[\"WGS 84\","
        "    DATUM[\"WGS_1984\","
        "        SPHEROID[\"WGS 84\",6378137,298.257223563]],"
        "    PRIMEM[\"Greenwich\",0],"
        "    UNIT[\"degree\",0.0174532925199433]]";

    OGRSpatialReference oSRS;
    OGRErr err = oSRS.importFromWkt(pszWkt);
    if (err != OGRERR_NONE)
    {
        printf("importFromWkt 失败\n");
    }

    // 也可以使用 char** 形式（兼容旧接口）
    char* pszWktCopy = CPLStrdup(pszWkt);
    char* ppszWkt = pszWktCopy;
    OGRSpatialReference oSRS2;
    oSRS2.importFromWkt(&ppszWkt);
    CPLFree(pszWktCopy);

SetFromUserInput()
===================

``SetFromUserInput()`` 是最灵活的 SRS 创建方法，支持多种输入格式，包括 EPSG 代码、WKT 字符串、PROJ 字符串、OGC URN 以及权威机构代码等。

.. code-block:: c++

    OGRSpatialReference oSRS;

    // EPSG 代码格式
    oSRS.SetFromUserInput("EPSG:4326");

    // 常用别名
    oSRS.SetFromUserInput("WGS84");
    oSRS.SetFromUserInput("NAD83");
    oSRS.SetFromUserInput("NAD27");

    // OGC URN 格式
    oSRS.SetFromUserInput("urn:ogc:def:crs:EPSG::4326");

    // PROJ 字符串格式
    oSRS.SetFromUserInput("+proj=longlat +datum=WGS84 +no_defs");

    // 权威机构:代码格式
    oSRS.SetFromUserInput("IAU:2000:39910");  // 火星坐标系示例

    // 也可以直接传入 WKT 字符串
    oSRS.SetFromUserInput(
        "GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\","
        "SPHEROID[\"WGS 84\",6378137,298.257223563]],"
        "PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433]]");

importFromProj4()
===================

通过 PROJ 4 格式的投影参数字符串创建 SRS。虽然 PROJ 4 字符串在 GDAL 3.x 中仍然支持，但建议优先使用 WKT2 或 EPSG 代码。

.. code-block:: c++

    OGRSpatialReference oSRS;

    // 从 PROJ 4 字符串创建 WGS 84 地理坐标系
    oSRS.importFromProj4("+proj=longlat +datum=WGS84 +no_defs");

    // 创建 UTM Zone 50N 投影
    OGRSpatialReference oSRS_UTM;
    oSRS_UTM.importFromProj4(
        "+proj=utm +zone=50 +datum=WGS84 +units=m +no_defs");

    // 创建兰伯特等角圆锥投影
    OGRSpatialReference oSRS_LCC;
    oSRS_LCC.importFromProj4(
        "+proj=lcc +lat_1=25 +lat_0=25 +lon_0=110 "
        "+k_0=1 +x_0=0 +y_0=0 +datum=WGS84 +units=m +no_defs");

    // 将 PROJ 4 字符串转为 WKT 输出
    char* pszWkt = nullptr;
    oSRS.exportToWkt(&pszWkt);
    printf("WKT: %s\n", pszWkt);
    CPLFree(pszWkt);

.. note::

    GDAL 3.x 中 PROJ 字符串的支持依赖于底层 PROJ 库版本。PROJ 6+ 推荐使用权威机构代码或 WKT2 来定义坐标系统，以获得更高的精度和更完整的参数描述。

****************************
AxisMappingStrategy 轴序映射
****************************

GDAL 3.0 最重要的变化之一是坐标轴顺序的改变。在 GDAL 2.x 及更早版本中，EPSG:4326 的坐标顺序始终为（经度, 纬度），即（X, Y）。但从 GDAL 3.0 开始，遵循 OGC/ISO 19111 标准，EPSG:4326 的坐标轴顺序变为（纬度, 经度），即（Y, X）。

OAMS_TRADITIONAL_GIS_ORDER 的必要性
======================================

为了避免兼容性问题，GDAL 3.x 引入了 ``AxisMappingStrategy`` 机制。通过设置 ``OAMS_TRADITIONAL_GIS_ORDER``，可以强制使用传统的 GIS 轴序（经度/纬度），这在大多数 GIS 应用中是必需的。

.. code-block:: c++

    #include "ogr_spatialref.h"

    // GDAL 3.x 中创建 EPSG:4326
    OGRSpatialReference oSRS;
    oSRS.importFromEPSG(4326);

    // 不设置 AxisMappingStrategy 时，轴序为（纬度, 经度）
    // 即 GetAxis(nullptr, 0) 返回 "Lat", GetAxis(nullptr, 1) 返回 "Lon"
    // Transform() 方法期望输入顺序为 (纬度, 经度)

    // 设置为传统 GIS 轴序（经度, 纬度）
    oSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
    // 现在 Transform() 方法期望输入顺序为 (经度, 纬度)

    // 导出 WKT 查看轴序信息
    char* pszWkt = nullptr;
    oSRS.exportToWkt(&pszWkt);
    printf("WKT with axis info:\n%s\n", pszWkt);
    CPLFree(pszWkt);

以下代码展示了不设置轴序映射时可能出现的问题：

.. code-block:: c++

    // 错误示例：不设置 AxisMappingStrategy
    OGRSpatialReference oSrcSRS, oDstSRS;
    oSrcSRS.importFromEPSG(4326);  // GDAL 3.x 默认轴序：纬度, 经度
    oDstSRS.importFromEPSG(3857);

    OGRCoordinateTransformation* poCT =
        OGRCreateCoordinateTransformation(&oSrcSRS, &oDstSRS);

    // 如果按传统习惯传入 (经度=116.4, 纬度=39.9)，实际会被解释为 (纬度=116.4, 经度=39.9)
    // 导致转换结果错误！
    double x = 116.4, y = 39.9;
    poCT->Transform(1, &x, &y);
    printf("错误结果: x=%f, y=%f\n", x, y);
    // 结果将不正确，因为输入被解释为 (纬度=116.4, 经度=39.9)

    delete poCT;

    // 正确示例：设置 OAMS_TRADITIONAL_GIS_ORDER
    oSrcSRS.importFromEPSG(4326);
    oSrcSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
    oDstSRS.importFromEPSG(3857);

    poCT = OGRCreateCoordinateTransformation(&oSrcSRS, &oDstSRS);

    // 现在按传统顺序传入 (经度, 纬度)
    x = 116.4;
    y = 39.9;
    poCT->Transform(1, &x, &y);
    printf("正确结果: x=%f, y=%f\n", x, y);
    // 输出正确的 Web 墨卡托坐标

    delete poCT;

.. warning::

    在 GDAL 3.x 中，使用 EPSG:4326 或其他地理坐标系时，**务必** 调用 ``SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER)``，否则所有坐标操作的输入输出顺序将与预期不同。这是从 GDAL 2.x 迁移到 3.x 时最常见的问题。

********************
坐标转换
********************

OGRCreateCoordinateTransformation()
======================================

``OGRCreateCoordinateTransformation()`` 函数用于创建坐标转换对象，可以在两个不同的 SRS 之间进行坐标转换。该函数接受两个 ``OGRSpatialReference`` 指针作为参数，返回 ``OGRCoordinateTransformation`` 对象指针。

正向转换
----------

以下代码演示了从 WGS 84（EPSG:4326）到 UTM Zone 50N（EPSG:32650）的坐标转换：

.. code-block:: c++

    #include "ogr_spatialref.h"

    // 创建源坐标系和目标坐标系
    OGRSpatialReference oSrcSRS, oDstSRS;
    oSrcSRS.importFromEPSG(4326);
    oSrcSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
    oDstSRS.importFromEPSG(32650);
    oDstSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

    // 创建坐标转换对象
    OGRCoordinateTransformation* poCT =
        OGRCreateCoordinateTransformation(&oSrcSRS, &oDstSRS);
    if (poCT == nullptr)
    {
        printf("创建坐标转换失败！\n");
        return;
    }

    // 转换单个点：北京天安门 (经度 116.397, 纬度 39.908)
    double x = 116.397;
    double y = 39.908;
    if (poCT->Transform(1, &x, &y))
    {
        printf("WGS84 (116.397, 39.908) -> UTM50N (%.3f, %.3f)\n", x, y);
    }
    else
    {
        printf("坐标转换失败！\n");
    }
    delete poCT;

批量转换
----------

``Transform()`` 方法支持批量转换多个点，提高转换效率：

.. code-block:: c++

    // 批量转换多个点
    double adfX[] = {116.397, 121.474, 113.264, 104.066};
    double adfY[] = { 39.908,  31.230,  23.130,  30.671};
    int nPointCount = 4;

    OGRCoordinateTransformation* poCT =
        OGRCreateCoordinateTransformation(&oSrcSRS, &oDstSRS);

    if (poCT->Transform(nPointCount, adfX, adfY))
    {
        for (int i = 0; i < nPointCount; i++)
        {
            printf("Point %d: UTM50N (%.3f, %.3f)\n", i, adfX[i], adfY[i]);
        }
    }
    delete poCT;

三维坐标转换
--------------

``Transform()`` 也支持三维坐标（含高程）的转换：

.. code-block:: c++

    // 三维坐标转换
    double x = 116.397;
    double y = 39.908;
    double z = 50.0;  // 高程 50 米

    OGRCoordinateTransformation* poCT =
        OGRCreateCoordinateTransformation(&oSrcSRS, &oDstSRS);

    if (poCT->Transform(1, &x, &y, &z))
    {
        printf("WGS84 (%.3f, %.3f, %.1f) -> UTM50N (%.3f, %.3f, %.1f)\n",
               116.397, 39.908, 50.0, x, y, z);
    }
    delete poCT;

逆向转换
----------

创建转换对象时交换源和目标 SRS 即可实现逆向转换：

.. code-block:: c++

    // 从 UTM 转回 WGS84
    OGRSpatialReference oUTM_SRS, oWGS84_SRS;
    oUTM_SRS.importFromEPSG(32650);
    oWGS84_SRS.importFromEPSG(4326);
    oWGS84_SRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

    OGRCoordinateTransformation* poReverseCT =
        OGRCreateCoordinateTransformation(&oUTM_SRS, &oWGS84_SRS);
    if (poReverseCT == nullptr)
    {
        printf("创建逆向坐标转换失败！\n");
        return;
    }

    // UTM 50N 坐标转回 WGS84
    double x = 448280.0;
    double y = 4418320.0;
    if (poReverseCT->Transform(1, &x, &y))
    {
        printf("UTM50N (448280, 4418320) -> WGS84 (%.6f, %.6f)\n", x, y);
    }
    delete poReverseCT;

.. note::

    ``OGRCreateCoordinateTransformation()`` 失败时返回 ``nullptr``，常见原因包括：
    - PROJ 库不支持所需的转换方法
    - 缺少必要的基准面转换网格文件（如 NTv2 格式的 .gsb 文件）
    - 源和目标 SRS 之间无法建立转换管线

**************************
TransformBounds 边界框变换
**************************

GDAL 3.4 引入了 ``OGRCoordinateTransformation::TransformBounds()`` 方法，用于对地理边界框进行坐标转换。该方法使用 21 个采样点来计算变换后的边界框，比简单转换四个角点更加准确。

边界框变换的难点在于：投影变换是非线性的，简单地转换四个角点可能导致结果边界框不包含原始区域的所有点。``TransformBounds()`` 通过在边界框边缘上采样 21 个点（四条边上各 5 个点加 4 个角点），确保变换后的边界框足够覆盖原始区域。

.. code-block:: c++

    #include "ogr_spatialref.h"

    OGRSpatialReference oSrcSRS, oDstSRS;
    oSrcSRS.importFromEPSG(4326);
    oSrcSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
    oDstSRS.importFromEPSG(3857);
    oDstSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

    OGRCoordinateTransformation* poCT =
        OGRCreateCoordinateTransformation(&oSrcSRS, &oDstSRS);
    if (poCT == nullptr)
    {
        printf("创建坐标转换失败！\n");
        return;
    }

    // 定义 WGS84 下的边界框（经度最小, 纬度最小, 经度最大, 纬度最大）
    double dfXMin = 116.0;  // 经度最小
    double dfYMin =  39.0;  // 纬度最小
    double dfXMax = 117.0;  // 经度最大
    double dfYMax =  40.0;  // 纬度最大

    // 变换后的边界框
    double dfOutXMin, dfOutYMin, dfOutXMax, dfOutYMax;

    // padfXMargin 和 padfYMargin 为边界框扩展比例（通常为 0）
    int bSuccess = poCT->TransformBounds(
        dfXMin, dfYMin, dfXMax, dfYMax,
        &dfOutXMin, &dfOutYMin, &dfOutXMax, &dfOutYMax,
        0.0,   // X 方向扩展比例
        0.0    // Y 方向扩展比例
    );

    if (bSuccess)
    {
        printf("原始边界框 (WGS84):  (%.4f, %.4f) - (%.4f, %.4f)\n",
               dfXMin, dfYMin, dfXMax, dfYMax);
        printf("变换后边界框 (3857): (%.2f, %.2f) - (%.2f, %.2f)\n",
               dfOutXMin, dfOutYMin, dfOutXMax, dfOutYMax);
    }

    delete poCT;

``TransformBounds()`` 的方法签名为：

.. code-block:: c++

    int TransformBounds(
        double dfXMin,     // 源 X 最小值
        double dfYMin,     // 源 Y 最小值
        double dfXMax,     // 源 X 最大值
        double dfYMax,     // 源 Y 最大值
        double* pdfXMin,   // 输出 X 最小值
        double* pdfYMin,   // 输出 Y 最小值
        double* pdfXMax,   // 输出 X 最大值
        double* pdfYMax,   // 输出 Y 最大值
        double dfXMargin,  // X 方向边界扩展比例
        double dfYMargin   // Y 方向边界扩展比例
    );

.. note::

    ``TransformBounds()`` 使用 21 采样点算法（而非简单的四角点变换），确保在跨投影带、高纬度地区等非线性变换场景下，变换后的边界框能够完整覆盖原始区域。这在数据裁切、空间索引构建等场景中非常重要。

********************
SRS 比较与克隆
********************

IsSame() 比较两个 SRS
=========================

``IsSame()`` 方法用于比较两个 ``OGRSpatialReference`` 对象是否表示相同的坐标系统。比较会考虑所有参数，包括基准面、椭球体、投影参数和权威机构代码等。

.. code-block:: c++

    OGRSpatialReference oSRS1, oSRS2, oSRS3;

    // 两种方式创建 WGS84
    oSRS1.importFromEPSG(4326);
    oSRS2.importFromEPSG(4326);
    oSRS3.importFromEPSG(3857);

    // 比较两个相同的 SRS
    if (oSRS1.IsSame(&oSRS2))
    {
        printf("oSRS1 和 oSRS2 是相同的坐标系统\n");
    }

    // 比较两个不同的 SRS
    if (!oSRS1.IsSame(&oSRS3))
    {
        printf("oSRS1 和 oSRS3 是不同的坐标系统\n");
    }

    // 使用 IsSameGeogCS() 仅比较地理坐标系部分
    // 即使一个是投影坐标系，一个是地理坐标系，如果基准面相同则返回 TRUE
    OGRSpatialReference oProjSRS;
    oProjSRS.importFromEPSG(32650);  // UTM 50N, 基于 WGS84

    if (oSRS1.IsSameGeogCS(&oProjSRS))
    {
        printf("两者使用相同的地理坐标系（基准面）\n");
    }

    // IsSameGeogCS() 可以传入 nullptr，表示与 WGS84 比较
    if (oSRS1.IsSameGeogCS(nullptr))
    {
        printf("oSRS1 的地理坐标系与默认的 WGS84 相同\n");
    }

Clone() 克隆 SRS
===================

``Clone()`` 方法用于深拷贝一个 ``OGRSpatialReference`` 对象，返回一个独立的新对象。

.. code-block:: c++

    OGRSpatialReference oOriginal;
    oOriginal.importFromEPSG(4326);
    oOriginal.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

    // 克隆 SRS
    OGRSpatialReference* poClone = oOriginal.Clone();
    if (poClone != nullptr)
    {
        printf("克隆成功\n");

        // 验证克隆对象与原始对象相同
        if (oOriginal.IsSame(poClone))
        {
            printf("克隆对象与原始对象相同\n");
        }

        // 修改克隆对象不影响原始对象
        poClone->importFromEPSG(3857);
        // 此时 oOriginal 仍然是 EPSG:4326，poClone 变为 EPSG:3857

        // 使用完毕后释放克隆对象
        delete poClone;
    }

    // CloneGeogCS() 克隆地理坐标系部分
    OGRSpatialReference oProjSRS;
    oProjSRS.importFromEPSG(32650);

    OGRSpatialReference* poGeogCS = oProjSRS.CloneGeogCS();
    if (poGeogCS != nullptr)
    {
        // poGeogCS 包含了 oProjSRS 的地理坐标系部分（WGS84）
        printf("克隆的地理坐标系:\n");
        char* pszWkt = nullptr;
        poGeogCS->exportToWkt(&pszWkt);
        printf("%s\n", pszWkt);
        CPLFree(pszWkt);

        delete poGeogCS;
    }

********************
WKT2:2019 支持
********************

GDAL 3.x 完整支持 OGC WKT2:2019 标准（ISO 19162:2019），相比 WKT 1（传统 WKT），WKT2 提供了更精确的坐标系统描述，包括更严格的语法、更丰富的元数据以及对现代坐标操作的支持。

importFromWkt() 导入 WKT2
============================

``importFromWkt()`` 方法可以同时接受 WKT 1 和 WKT 2 格式的字符串。GDAL 会自动识别格式版本。

.. code-block:: c++

    // WKT2:2019 格式的 EPSG:4326
    const char* pszWkt2 =
        "GEOGCRS[\"WGS 84\","
        "    ENSEMBLE[\"World Geodetic System 1984 ensemble\","
        "        MEMBER[\"World Geodetic System 1984 (Transit)\"],"
        "        MEMBER[\"World Geodetic System 1984 (G730)\"],"
        "        MEMBER[\"World Geodetic System 1984 (G873)\"],"
        "        MEMBER[\"World Geodetic System 1984 (G1150)\"],"
        "        MEMBER[\"World Geodetic System 1984 (G1674)\"],"
        "        MEMBER[\"World Geodetic System 1984 (G1762)\"],"
        "        MEMBER[\"World Geodetic System 1984 (G2139)\"],"
        "        ELLIPSOID[\"WGS 84\",6378137,298.257223563,"
        "            LENGTHUNIT[\"metre\",1]],"
        "        ENSEMBLEACCURACY[2.0]],"
        "    PRIMEM[\"Greenwich\",0,"
        "        ANGLEUNIT[\"degree\",0.0174532925199433]],"
        "    CS[ellipsoidal,2],"
        "        AXIS[\"geodetic latitude (Lat)\",north,"
        "            ORDER[1],"
        "            ANGLEUNIT[\"degree\",0.0174532925199433]],"
        "        AXIS[\"geodetic longitude (Lon)\",east,"
        "            ORDER[2],"
        "            ANGLEUNIT[\"degree\",0.0174532925199433]],"
        "    USAGE["
        "        SCOPE[\"Horizontal component of 3D system.\"],"
        "        AREA[\"World.\"],"
        "        BBOX[-90,-180,90,180]],"
        "    ID[\"EPSG\",4326]]";

    OGRSpatialReference oSRS;
    OGRErr err = oSRS.importFromWkt(pszWkt2);
    if (err == OGRERR_NONE)
    {
        printf("WKT2 导入成功\n");
    }

exportToWkt() 导出 WKT2
==========================

使用 ``exportToWkt()`` 并指定格式选项来导出 WKT2 格式：

.. code-block:: c++

    OGRSpatialReference oSRS;
    oSRS.importFromEPSG(4326);
    oSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

    // 导出为 WKT 1（传统格式）
    char* pszWkt1 = nullptr;
    oSRS.exportToWkt(&pszWkt1);
    printf("WKT1:\n%s\n\n", pszWkt1);
    CPLFree(pszWkt1);

    // 导出为 WKT2:2019
    const char* const apszOptions[] = { "FORMAT=WKT2_2019", nullptr };
    char* pszWkt2 = nullptr;
    oSRS.exportToWkt(&pszWkt2, apszOptions);
    printf("WKT2:2019:\n%s\n\n", pszWkt2);
    CPLFree(pszWkt2);

    // 导出为 WKT2:2019 简洁模式（单行输出）
    const char* const apszOptionsFlat[] = {
        "FORMAT=WKT2_2019", "MULTILINE=NO", nullptr
    };
    char* pszWkt2Flat = nullptr;
    oSRS.exportToWkt(&pszWkt2Flat, apszOptionsFlat);
    printf("WKT2:2019 (single line): %s\n", pszWkt2Flat);
    CPLFree(pszWkt2Flat);

    // 导出为 WKT2:2015（旧版 WKT2）
    const char* const apszOptions2015[] = { "FORMAT=WKT2_2015", nullptr };
    char* pszWkt2_2015 = nullptr;
    oSRS.exportToWkt(&pszWkt2_2015, apszOptions2015);
    printf("WKT2:2015:\n%s\n", pszWkt2_2015);
    CPLFree(pszWkt2_2015);

可选的 FORMAT 参数值：

- ``WKT1`` - 传统 WKT 格式（默认）
- ``WKT1_GDAL`` - GDAL 扩展的 WKT 1 格式
- ``WKT1_ESRI`` - ESRI 风格的 WKT 1 格式
- ``WKT2_2015`` - WKT2:2015（ISO 19162:2015）
- ``WKT2_2019`` - WKT2:2019（ISO 19162:2019，推荐）
- ``WKT2`` - 当前最新的 WKT2 版本（等同于 WKT2_2019）

.. note::

    WKT2:2019 相比 WKT1 的主要改进包括：
    - 更严格的语法规则，消除了 WKT1 中的歧义
    - 使用 ``GEOGCRS`` / ``PROJCRS`` 替代 ``GEOGCS`` / ``PROJCS``
    - 使用 ``ENSEMBLE`` 描述基准面，取代 ``DATUM``
    - 使用 ``CS`` 和 ``AXIS`` 更精确地描述坐标系类型和轴信息
    - 支持 ``USAGE`` 元素，描述坐标系统的适用范围和区域

**********************
坐标精度控制（RFC 99）
**********************

GDAL 3.9 引入了 RFC 99（Coordinate Precision），提供了在数据读写过程中控制坐标精度的能力。这对于数据压缩、减少存储空间以及在数据交换时控制精度非常有用。

.. code-block:: c++

    #include "ogr_spatialref.h"
    #include "ogr_geometry.h"

    // 通过 OGRGeomCoordinatePrecision 设置坐标精度
    // 以下示例将坐标精度设置到小数点后 2 位（约 1 米精度）
    OGRGeomCoordinatePrecision oPrecision;
    oPrecision.dfXYResolution = 0.01;  // XY 方向分辨率为 0.01 度
    oPrecision.dfZResolution = 0.01;   // Z 方向分辨率为 0.01 米

    // 将精度设置到 SRS 上
    OGRSpatialReference oSRS;
    oSRS.importFromEPSG(4326);
    oSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

    // 设置坐标精度
    oSRS.SetCoordinatePrecision(oPrecision);

    // 创建几何体并应用精度控制
    OGRPoint oPoint(116.397123456, 39.908654321, 50.123);
    printf("原始坐标: %.9f, %.9f, %.3f\n",
           oPoint.getX(), oPoint.getY(), oPoint.getZ());

    // 使用指定精度舍入坐标
    oPoint.setPrecision(&oPrecision, true);
    printf("精度控制后: %.2f, %.2f, %.2f\n",
           oPoint.getX(), oPoint.getY(), oPoint.getZ());

``OGRGeomCoordinatePrecision`` 的主要成员：

.. code-block:: c++

    // 精度控制结构体
    struct OGRGeomCoordinatePrecision
    {
        double dfXYResolution;   // XY 坐标分辨率
        double dfZResolution;    // Z 坐标分辨率
        double dfMResolution;    // M 坐标分辨率
    };

    // 获取 SRS 关联的坐标精度
    const OGRGeomCoordinatePrecision& oPrecision =
        oSRS.GetCoordinatePrecision();

    // 在写入数据时，某些驱动（如 GeoPackage, FlatGeobuf）会自动应用坐标精度
    // 以实现更好的数据压缩

.. note::

    RFC 99 的坐标精度控制主要用于：
    - 在写入数据时减少不必要的小数位数，提高压缩率
    - 在数据交换时统一坐标精度
    - 在 FlatGeobuf 等格式中实现更高效的 Hilbert 排序索引

********************
SRS 生命周期管理
********************

引用计数机制
================

``OGRSpatialReference`` 采用引用计数（Reference Counting）机制来管理生命周期。当引用计数降为零时，对象自动销毁。

.. code-block:: c++

    // C++ 中的生命周期管理

    // 栈上创建的对象，作用域结束时自动销毁
    {
        OGRSpatialReference oSRS;
        oSRS.importFromEPSG(4326);
        // oSRS 在此作用域结束时自动销毁
    }

    // 堆上创建的对象，需要手动 delete
    {
        OGRSpatialReference* poSRS = new OGRSpatialReference();
        poSRS->importFromEPSG(4326);

        // 使用完毕后释放
        delete poSRS;
    }

    // 引用计数操作（通常不需要手动调用）
    OGRSpatialReference* poSRS = new OGRSpatialReference();
    poSRS->importFromEPSG(4326);

    poSRS->Reference();    // 引用计数 +1
    poSRS->Reference();    // 引用计数 +1
    poSRS->Dereference();  // 引用计数 -1
    poSRS->Dereference();  // 引用计数 -1
    // 当引用计数为 0 时对象被销毁（此处由 delete 处理）
    delete poSRS;

C API 中的生命周期管理
=========================

在 C API 中，使用 ``OSRDestroySpatialReference()`` 来销毁 SRS 对象：

.. code-block:: c

    #include "ogr_srs_api.h"

    // C API 创建 SRS
    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(NULL);
    OSRImportFromEPSG(hSRS, 4326);

    // 使用 SRS...
    int bIsGeog = OSRIsGeographic(hSRS);
    printf("Is Geographic: %d\n", bIsGeog);

    // 导出 WKT
    char* pszWkt = NULL;
    OSRExportToWkt(hSRS, &pszWkt);
    printf("WKT: %s\n", pszWkt);
    CPLFree(pszWkt);

    // 引用计数管理
    OSRReference(hSRS);     // 引用计数 +1
    OSRDereference(hSRS);   // 引用计数 -1

    // 销毁 SRS（释放所有引用）
    OSRDestroySpatialReference(hSRS);

C++ 自动管理
================

在 C++ 中推荐使用智能指针或栈对象来自动管理 ``OGRSpatialReference`` 的生命周期：

.. code-block:: c++

    #include "ogr_spatialref.h"
    #include <memory>

    // 方式一：栈上对象（推荐用于局部变量）
    void processSpatialReference()
    {
        OGRSpatialReference oSRS;
        oSRS.importFromEPSG(4326);
        oSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

        // 使用 oSRS...
        char* pszWkt = nullptr;
        oSRS.exportToWkt(&pszWkt);
        printf("WKT: %s\n", pszWkt);
        CPLFree(pszWkt);

        // 离开作用域时自动销毁，无需手动 delete
    }

    // 方式二：使用 std::unique_ptr 管理堆上对象
    std::unique_ptr<OGRSpatialReference> createSRS(int nEPSG)
    {
        auto poSRS = std::make_unique<OGRSpatialReference>();
        poSRS->importFromEPSG(nEPSG);
        return poSRS;
    }

    // 方式三：使用 GDAL 自带的智能指针
    void smartPointerExample()
    {
        // GDAL 3.x 提供的智能指针
        auto poSRS = std::unique_ptr<OGRSpatialReference>(
            new OGRSpatialReference());
        poSRS->importFromEPSG(4326);

        // 自动释放
    }

.. warning::

    ``OGRCoordinateTransformation`` 对象也需要手动管理生命周期。使用 ``delete`` 释放，不要使用 ``OGRDestroyCoordinateTransformation()`` （那是 C API 的函数）。

********************
常用 SRS 操作
********************

IsGeographic() / IsProjected()
================================

用于判断 SRS 的类型：地理坐标系（经纬度）或投影坐标系（平面坐标）。

.. code-block:: c++

    OGRSpatialReference oSRS;

    // 地理坐标系
    oSRS.importFromEPSG(4326);
    if (oSRS.IsGeographic())
    {
        printf("EPSG:4326 是地理坐标系\n");
    }
    if (!oSRS.IsProjected())
    {
        printf("EPSG:4326 不是投影坐标系\n");
    }

    // 投影坐标系
    OGRSpatialReference oProjSRS;
    oProjSRS.importFromEPSG(32650);  // UTM 50N
    if (oProjSRS.IsProjected())
    {
        printf("EPSG:32650 是投影坐标系\n");
    }
    if (!oProjSRS.IsGeographic())
    {
        printf("EPSG:32650 不是地理坐标系\n");
    }

    // 也可以使用 IsLocal() 判断是否为局部坐标系
    // 和 IsGeocentric() 判断是否为地心坐标系

GetAttrValue() 获取属性值
============================

``GetAttrValue()`` 方法用于从 WKT 树中获取指定节点的字符串值。

.. code-block:: c++

    OGRSpatialReference oSRS;
    oSRS.importFromEPSG(4326);

    // 获取 GEOGCS 名称
    const char* pszGeogCS = oSRS.GetAttrValue("GEOGCS");
    printf("GEOGCS: %s\n", pszGeogCS);  // 输出: "WGS 84"

    // 获取 DATUM 名称
    const char* pszDatum = oSRS.GetAttrValue("DATUM");
    printf("DATUM: %s\n", pszDatum);  // 输出: "WGS_1984"

    // 获取椭球体名称
    const char* pszSpheroid = oSRS.GetAttrValue("SPHEROID");
    printf("SPHEROID: %s\n", pszSpheroid);  // 输出: "WGS 84"

    // 获取本初子午线名称
    const char* pszPM = oSRS.GetAttrValue("PRIMEM");
    printf("PRIMEM: %s\n", pszPM);  // 输出: "Greenwich"

    // 对于投影坐标系，获取 PROJCS 和 PROJECTION 名称
    OGRSpatialReference oProjSRS;
    oProjSRS.importFromEPSG(32650);

    const char* pszProjCS = oProjSRS.GetAttrValue("PROJCS");
    printf("PROJCS: %s\n", pszProjCS);

    const char* pszProjection = oProjSRS.GetAttrValue("PROJECTION");
    printf("PROJECTION: %s\n", pszProjection);

    // 使用子节点索引获取嵌套属性
    // GetAttrValue("DATUM", 0) 获取第一个 DATUM 节点
    // GetAttrValue("SPHEROID", 0) 获取第一个 SPHEROID 节点
    const char* pszSpheroidName = oSRS.GetAttrValue("SPHEROID", 0);
    printf("SPHEROID Name: %s\n", pszSpheroidName);

    // 获取子节点（如 AUTHORITY）
    const char* pszAuthority = oSRS.GetAuthorityCode(nullptr);
    const char* pszAuthName = oSRS.GetAuthorityName(nullptr);
    printf("Authority: %s:%s\n", pszAuthName, pszAuthority);
    // 输出: "EPSG:4326"

SetWellKnownGeogCS() 设置常用地理坐标系
============================================

``SetWellKnownGeogCS()`` 用于快速设置常用的地理坐标系统，支持短名称和 EPSG 代码。

.. code-block:: c++

    OGRSpatialReference oSRS;

    // 设置 WGS 84
    oSRS.SetWellKnownGeogCS("WGS84");
    // 等同于 oSRS.importFromEPSG(4326);

    // 设置 NAD83
    oSRS.SetWellKnownGeogCS("NAD83");

    // 设置 NAD27
    oSRS.SetWellKnownGeogCS("NAD27");

    // 设置 WGS72
    oSRS.SetWellKnownGeogCS("WGS72");

    // 也可以使用 EPSG:XXXX 格式
    oSRS.SetWellKnownGeogCS("EPSG:4326");   // WGS84
    oSRS.SetWellKnownGeogCS("EPSG:4490");   // CGCS2000
    oSRS.SetWellKnownGeogCS("EPSG:4214");   // Beijing 1954

    // 设置克拉克1866椭球体（常用于北美坐标系）
    oSRS.SetWellKnownGeogCS("clrk66");

    // 导出查看
    char* pszWkt = nullptr;
    oSRS.exportToWkt(&pszWkt);
    printf("%s\n", pszWkt);
    CPLFree(pszWkt);

其他常用操作
================

.. code-block:: c++

    // 获取椭球体参数
    OGRSpatialReference oSRS;
    oSRS.importFromEPSG(4326);

    double dfSemiMajor = oSRS.GetSemiMajor();      // 半长轴
    double dfSemiMinor = oSRS.GetSemiMinor();       // 半短轴
    double dfInvFlattening = oSRS.GetInvFlattening(); // 反扁率

    printf("Semi-major axis: %.3f m\n", dfSemiMajor);
    printf("Semi-minor axis: %.3f m\n", dfSemiMinor);
    printf("Inverse flattening: %.10f\n", dfInvFlattening);

    // 获取线性单位
    OGRSpatialReference oProjSRS;
    oProjSRS.importFromEPSG(32650);

    char* pszUnitsName = nullptr;
    double dfToMeters = oProjSRS.GetLinearUnits(&pszUnitsName);
    printf("Linear units: %s (%.6f to meters)\n", pszUnitsName, dfToMeters);
    CPLFree(pszUnitsName);

    // 获取角度单位
    char* pszAngularUnits = nullptr;
    double dfToRadians = oSRS.GetAngularUnits(&pszAngularUnits);
    printf("Angular units: %s (%.10f to radians)\n",
           pszAngularUnits, dfToRadians);
    CPLFree(pszAngularUnits);

    // 获取 UTM 带号
    int bNorth = FALSE;
    int nZone = oProjSRS.GetUTMZone(&bNorth);
    printf("UTM Zone: %d %s\n", nZone, bNorth ? "North" : "South");

    // 设置 UTM 带号
    OGRSpatialReference oUTM;
    oUTM.SetProjCS("UTM 50 / WGS84");
    oUTM.SetWellKnownGeogCS("WGS84");
    oUTM.SetUTM(50, TRUE);  // 50 号带，北半球

    // 设置自定义投影参数
    oUTM.SetProjParm(SRS_PP_CENTRAL_MERIDIAN, 117.0);
    oUTM.SetProjParm(SRS_PP_LATITUDE_OF_ORIGIN, 0.0);
    oUTM.SetProjParm(SRS_PP_SCALE_FACTOR, 0.9996);
    oUTM.SetProjParm(SRS_PP_FALSE_EASTING, 500000.0);
    oUTM.SetProjParm(SRS_PP_FALSE_NORTHING, 0.0);

********************
C 语言接口参考
********************

以下是 C 语言接口中与空间参考相关的核心函数声明：

.. code-block:: c

    /* 创建与销毁 */
    OGRSpatialReferenceH OSRNewSpatialReference(const char* pszWKT);
    void OSRDestroySpatialReference(OGRSpatialReferenceH hSRS);

    /* 引用计数 */
    int OSRReference(OGRSpatialReferenceH hSRS);
    int OSRDereference(OGRSpatialReferenceH hSRS);

    /* 导入 */
    OGRErr OSRImportFromEPSG(OGRSpatialReferenceH hSRS, int nCode);
    OGRErr OSRImportFromWkt(OGRSpatialReferenceH hSRS, char** ppszWkt);
    OGRErr OSRSetFromUserInput(OGRSpatialReferenceH hSRS, const char* pszDef);
    OGRErr OSRImportFromProj4(OGRSpatialReferenceH hSRS, const char* pszProj4);

    /* 导出 */
    OGRErr OSRExportToWkt(OGRSpatialReferenceH hSRS, char** ppszWkt);
    OGRErr OSRExportToProj4(OGRSpatialReferenceH hSRS, char** ppszProj4);
    OGRErr OSRExportToWktEx(OGRSpatialReferenceH hSRS, char** ppszWkt,
                            const char* const* papszOptions);

    /* 查询 */
    int OSRIsGeographic(OGRSpatialReferenceH hSRS);
    int OSRIsProjected(OGRSpatialReferenceH hSRS);
    int OSRIsSame(OGRSpatialReferenceH hSRS1, OGRSpatialReferenceH hSRS2);
    int OSRIsSameGeogCS(OGRSpatialReferenceH hSRS1, OGRSpatialReferenceH hSRS2);

    /* 属性查询 */
    const char* OSRGetAttrValue(OGRSpatialReferenceH hSRS,
                                const char* pszName, int iChild);
    OGRErr OSRSetAttrValue(OGRSpatialReferenceH hSRS,
                           const char* pszNodePath, const char* pszNewNodeValue);

    /* 常用设置 */
    OGRErr OSRSetWellKnownGeogCS(OGRSpatialReferenceH hSRS, const char* pszName);
    OGRErr OSRSetProjCS(OGRSpatialReferenceH hSRS, const char* pszName);
    OGRErr OSRSetUTM(OGRSpatialReferenceH hSRS, int nZone, int bNorth);

    /* 椭球体参数 */
    double OSRGetSemiMajor(OGRSpatialReferenceH hSRS, OGRErr* pnErr);
    double OSRGetSemiMinor(OGRSpatialReferenceH hSRS, OGRErr* pnErr);
    double OSRGetInvFlattening(OGRSpatialReferenceH hSRS, OGRErr* pnErr);

    /* 单位 */
    double OSRGetLinearUnits(OGRSpatialReferenceH hSRS, char** ppszName);
    double OSRGetAngularUnits(OGRSpatialReferenceH hSRS, char** ppszName);

    /* 权威机构 */
    OGRErr OSRSetAuthority(OGRSpatialReferenceH hSRS,
                           const char* pszTargetKey,
                           const char* pszAuthority, int nCode);
    const char* OSRGetAuthorityCode(OGRSpatialReferenceH hSRS,
                                    const char* pszTargetKey);
    const char* OSRGetAuthorityName(OGRSpatialReferenceH hSRS,
                                    const char* pszTargetKey);

    /* 坐标转换 */
    OGRCoordinateTransformationH
    OCTNewCoordinateTransformation(OGRSpatialReferenceH hSourceSRS,
                                   OGRSpatialReferenceH hTargetSRS);
    void OCTDestroyCoordinateTransformation(OGRCoordinateTransformationH hCT);
    int OCTTransform(OGRCoordinateTransformationH hCT,
                     int nCount, double* x, double* y, double* z);
