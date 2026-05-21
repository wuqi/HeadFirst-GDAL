.. highlight:: rst

.. _gdal3-vector-api:

##################################
GDAL 3.x 矢量/OGR API 变化
##################################

GDAL 3.x 版本对矢量 API 做了大量改进，包括统一数据集打开接口、新增 ``OGRFieldDefn`` 密封机制（RFC 97）、Arrow 列式读取（RFC 86）等。本章系统梳理 GDAL 3.x 中矢量相关 API 的使用方法，并给出完整的 C++ 代码示例。


.. _gdal3-open:

************************************
GDALOpenEx() 打开矢量数据
************************************

GDAL 2.x 起统一使用 ``GDALOpenEx()`` 替代旧的 ``OGROpen()`` / ``OGRSFDriverRegistrar::Open()`` 接口。通过 ``GDAL_OF_VECTOR`` 标志指定以矢量模式打开数据集。在 GDAL 3.x 中该接口保持不变，但内部驱动和坐标参考系统（CRS）处理有显著改进。

``GDALOpenEx()`` 函数签名如下：

.. code-block:: c++

    /**
     * @param pszFilename    文件路径
     * @param nOpenFlags     打开标志，可组合使用：
     *                       GDAL_OF_VECTOR    — 以矢量模式打开
     *                       GDAL_OF_READONLY  — 只读模式
     *                       GDAL_OF_UPDATE    — 读写模式
     *                       GDAL_OF_SHARED    — 共享模式（同一文件复用句柄）
     *                       GDAL_OF_VERBOSE_ERROR — 输出详细错误信息
     * @param papszAllowedDrivers  NULL 或以 NULL 结尾的字符串数组，指定允许的驱动名
     * @param papszOpenOptions     NULL 或以 NULL 结尾的字符串数组，驱动特定的打开选项
     * @param papszSiblingFiles    NULL 或以 NULL 结尾的兄弟文件列表
     * @return GDALDatasetH 句柄，失败返回 NULL
     */
    GDALDatasetH GDALOpenEx(const char *pszFilename,
                            unsigned int nOpenFlags,
                            const char *const *papszAllowedDrivers,
                            const char *const *papszOpenOptions,
                            const char *const *papszSiblingFiles);

使用示例——打开一个 Shapefile 并只读访问矢量图层：

.. code-block:: c++

    #include "ogrsf_frmts.h"
    #include "cpl_conv.h"

    int main()
    {
        // Windows 下确保中文路径和属性编码正确
        CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
        CPLSetConfigOption("SHAPE_ENCODING", "");

        // GDAL 3.x 无需手动注册驱动，GDALAllRegister() 内部完成
        GDALAllRegister();

        // 以矢量只读模式打开
        GDALDataset *poDS = (GDALDataset *)GDALOpenEx(
            "data/cities.shp",
            GDAL_OF_VECTOR | GDAL_OF_READONLY,
            NULL,   // 不限定驱动
            NULL,   // 无额外打开选项
            NULL    // 无兄弟文件
        );

        if (poDS == NULL)
        {
            printf("打开失败: %s\n", CPLGetLastErrorMsg());
            return 1;
        }

        // 获取图层数量
        int nLayers = poDS->GetLayerCount();
        printf("图层数量: %d\n", nLayers);

        // 使用完毕后关闭
        GDALClose(poDS);
        return 0;
    }

.. attention::

    * ``GDAL_OF_VECTOR`` 只打开矢量驱动，避免栅格驱动干扰，提升打开速度。
    * 若同时需要矢量和栅格，可组合 ``GDAL_OF_VECTOR | GDAL_OF_RASTER``。
    * GDAL 3.x 中 ``GDALAllRegister()`` 已包含所有内置驱动注册，无需再调用 ``OGRRegisterAll()`` （该函数已被废弃）。


.. _gdal3-layer:

************************************
Layer 操作
************************************

数据集打开后，需要通过 ``OGRLayer`` 对象访问具体的矢量图层。GDAL 提供多种获取图层的方式，以及空间过滤功能。

获取图层
======================

``GDALDataset`` 提供以下方法获取图层：

* ``GetLayer(int iLayer)`` — 按索引获取图层（从 0 开始）
* ``GetLayerByName(const char *pszName)`` — 按名称获取图层
* ``CreateLayer(const char *pszName, ...)``
* ``GetLayerCount()`` — 获取图层总数

创建图层
======================

``CreateLayer()`` 的完整签名：

.. code-block:: c++

    /**
     * @param pszName       图层名称
     * @param poSpatialRef  空间参考（OGRSpatialReference*），NULL 表示无空间参考
     * @param eGeomType     几何类型，如 wkbPoint、wkbLineString、wkbPolygon 等
     * @param papszOptions  驱动特定的创建选项
     * @return OGRLayer*，失败返回 NULL
     */
    OGRLayer *CreateLayer(const char *pszName,
                          OGRSpatialReference *poSpatialRef = NULL,
                          OGRwkbGeometryType eGeomType = wkbUnknown,
                          char **papszOptions = NULL);

空间过滤
======================

``OGRLayer`` 提供空间过滤功能，只返回与给定几何形状相交的要素：

* ``SetSpatialFilter(OGRGeometry *)`` — 设置几何形状作为空间过滤器
* ``SetSpatialFilterRect(double dfMinX, double dfMinY, double dfMaxX, double dfMaxY)`` — 以矩形范围作为空间过滤器
* ``SetSpatialFilter(int iGeomField, OGRGeometry *)`` — 对指定几何字段设置过滤器（GDAL 3.x 多几何字段支持）

完整代码示例：

.. code-block:: c++

    #include "ogrsf_frmts.h"

    void layerOperations(GDALDataset *poDS)
    {
        // ========== 按索引获取图层 ==========
        OGRLayer *poLayer0 = poDS->GetLayer(0);
        if (poLayer0 == NULL)
        {
            printf("无法获取第 0 个图层\n");
            return;
        }
        printf("图层名称: %s\n", poLayer0->GetName());
        printf("要素数量: %d\n", (int)poLayer0->GetFeatureCount());

        // ========== 按名称获取图层 ==========
        OGRLayer *poLayer = poDS->GetLayerByName("cities");
        if (poLayer == NULL)
        {
            printf("未找到名为 'cities' 的图层\n");
            return;
        }

        // ========== 矩形空间过滤 ==========
        // 只返回落在指定矩形范围内的要素
        poLayer->SetSpatialFilterRect(116.0, 39.0, 117.0, 40.0);
        printf("过滤后要素数量: %d\n", (int)poLayer->GetFeatureCount());

        // 遍历过滤后的要素
        OGRFeature *poFeature;
        poLayer->ResetReading();
        while ((poFeature = poLayer->GetNextFeature()) != NULL)
        {
            OGRGeometry *poGeom = poFeature->GetGeometryRef();
            if (poGeom != NULL)
            {
                printf("要素 FID=%lld: %s\n",
                       (long long)poFeature->GetFID(),
                       poGeom->exportToWkt());
            }
            OGRFeature::DestroyFeature(poFeature);
        }

        // ========== 几何形状空间过滤 ==========
        // 构造一个多边形作为过滤器
        OGRPolygon filterPoly;
        OGRLinearRing ring;
        ring.addPoint(116.0, 39.0);
        ring.addPoint(117.0, 39.0);
        ring.addPoint(117.0, 40.0);
        ring.addPoint(116.0, 40.0);
        ring.addPoint(116.0, 39.0);
        filterPoly.addRing(&ring);

        poLayer->SetSpatialFilter(&filterPoly);
        printf("多边形过滤后要素数量: %d\n", (int)poLayer->GetFeatureCount());

        // 清除空间过滤
        poLayer->SetSpatialFilter(NULL);
    }


.. _gdal3-feature-iterate:

************************************
Feature 遍历模式
************************************

OGRLayer 采用迭代器模式遍历要素。标准流程为：先调用 ``ResetReading()`` 将游标重置到图层起始位置，然后循环调用 ``GetNextFeature()`` 逐个获取要素，直到返回 ``NULL`` 表示遍历结束。

每次 ``GetNextFeature()`` 返回的要素由调用者负责释放，必须调用 ``OGRFeature::DestroyFeature()``。

完整的遍历示例——读取所有要素的属性和几何信息：

.. code-block:: c++

    #include "ogrsf_frmts.h"
    #include <cstdio>

    void iterateFeatures(OGRLayer *poLayer)
    {
        // 获取图层的要素类定义（字段元数据）
        OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();

        // 打印所有字段名
        printf("字段列表:\n");
        for (int i = 0; i < poFDefn->GetFieldCount(); i++)
        {
            OGRFieldDefn *poFieldDefn = poFDefn->GetFieldDefn(i);
            printf("  [%d] %s (类型: %s, 宽度: %d)\n",
                   i,
                   poFieldDefn->GetNameRef(),
                   OGRFieldDefn::GetFieldTypeName(poFieldDefn->GetType()),
                   poFieldDefn->GetWidth());
        }
        printf("\n");

        // ========== 标准遍历循环 ==========
        OGRFeature *poFeature;
        poLayer->ResetReading();

        while ((poFeature = poLayer->GetNextFeature()) != NULL)
        {
            // 打印要素 FID
            printf("FID: %lld\n", (long long)poFeature->GetFID());

            // 遍历所有属性字段
            for (int iField = 0; iField < poFDefn->GetFieldCount(); iField++)
            {
                OGRFieldDefn *poFieldDefn = poFDefn->GetFieldDefn(iField);
                OGRFieldType fieldType = poFieldDefn->GetType();

                if (!poFeature->IsFieldSet(iField))
                {
                    printf("  %s: (null)\n", poFieldDefn->GetNameRef());
                    continue;
                }

                switch (fieldType)
                {
                case OFTInteger:
                    printf("  %s: %d\n",
                           poFieldDefn->GetNameRef(),
                           poFeature->GetFieldAsInteger(iField));
                    break;
                case OFTInteger64:
                    printf("  %s: " CPL_FRMT_GIB "\n",
                           poFieldDefn->GetNameRef(),
                           poFeature->GetFieldAsInteger64(iField));
                    break;
                case OFTReal:
                    printf("  %s: %.6f\n",
                           poFieldDefn->GetNameRef(),
                           poFeature->GetFieldAsDouble(iField));
                    break;
                case OFTString:
                    printf("  %s: %s\n",
                           poFieldDefn->GetNameRef(),
                           poFeature->GetFieldAsString(iField));
                    break;
                case OFTDate:
                case OFTTime:
                case OFTDateTime:
                {
                    int nYear, nMonth, nDay, nHour, nMin, nSec, nTZ;
                    poFeature->GetFieldAsDateTime(iField,
                        &nYear, &nMonth, &nDay, &nHour, &nMin, &nSec, &nTZ);
                    printf("  %s: %04d-%02d-%02d %02d:%02d:%02d\n",
                           poFieldDefn->GetNameRef(),
                           nYear, nMonth, nDay, nHour, nMin, nSec);
                    break;
                }
                default:
                    printf("  %s: %s\n",
                           poFieldDefn->GetNameRef(),
                           poFeature->GetFieldAsString(iField));
                    break;
                }
            }

            // 获取并打印几何信息
            OGRGeometry *poGeometry = poFeature->GetGeometryRef();
            if (poGeometry != NULL)
            {
                OGRwkbGeometryType eType = wkbFlatten(poGeometry->getGeometryType());

                switch (eType)
                {
                case wkbPoint:
                {
                    OGRPoint *poPoint = (OGRPoint *)poGeometry;
                    printf("  几何: Point(%.6f, %.6f)\n",
                           poPoint->getX(), poPoint->getY());
                    break;
                }
                case wkbLineString:
                {
                    OGRLineString *poLine = (OGRLineString *)poGeometry;
                    printf("  几何: LineString(点数=%d)\n", poLine->getNumPoints());
                    break;
                }
                case wkbPolygon:
                {
                    OGRPolygon *poPoly = (OGRPolygon *)poGeometry;
                    printf("  几何: Polygon(面积=%.6f)\n", poPoly->get_Area());
                    break;
                }
                default:
                    printf("  几何: %s\n", poGeometry->getGeometryName());
                    break;
                }
            }
            else
            {
                printf("  几何: (空)\n");
            }

            // 必须释放要素
            OGRFeature::DestroyFeature(poFeature);
        }
    }


.. _gdal3-feature-create:

************************************
Feature 创建与写入
************************************

创建要素并写入图层是矢量数据输出的核心流程。基本步骤为：创建 ``OGRFeature`` 对象、设置属性字段值、设置几何形状，最后通过 ``OGRLayer::CreateFeature()`` 写入图层。

完整的创建示例——构建一个包含属性和几何的要素并写入图层：

.. code-block:: c++

    #include "ogrsf_frmts.h"
    #include "ogr_spatialref.h"

    int createAndWriteFeature()
    {
        // 创建 Shapefile 数据集
        GDALDriver *poDriver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
        if (poDriver == NULL)
        {
            printf("ESRI Shapefile 驱动不可用\n");
            return 1;
        }

        GDALDataset *poDS = poDriver->Create(
            "output/poi.shp", 0, 0, 0, GDT_Unknown, NULL);
        if (poDS == NULL)
        {
            printf("创建数据集失败: %s\n", CPLGetLastErrorMsg());
            return 1;
        }

        // 创建空间参考（WGS 84）
        OGRSpatialReference oSRS;
        oSRS.SetWellKnownGeogCS("WGS84");

        // 创建图层
        OGRLayer *poLayer = poDS->CreateLayer("poi", &oSRS, wkbPoint, NULL);
        if (poLayer == NULL)
        {
            printf("创建图层失败\n");
            GDALClose(poDS);
            return 1;
        }

        // ========== 定义字段 ==========
        // 名称字段
        OGRFieldDefn oNameField("name", OFTString);
        oNameField.SetWidth(64);
        if (poLayer->CreateField(&oNameField) != OGRERR_NONE)
        {
            printf("创建 name 字段失败\n");
        }

        // 人口字段
        OGRFieldDefn oPopField("population", OFTInteger);
        if (poLayer->CreateField(&oPopField) != OGRERR_NONE)
        {
            printf("创建 population 字段失败\n");
        }

        // 面积字段（浮点）
        OGRFieldDefn oAreaField("area_km2", OFTReal);
        if (poLayer->CreateField(&oAreaField) != OGRERR_NONE)
        {
            printf("创建 area_km2 字段失败\n");
        }

        // 面积字段（64位整数）
        OGRFieldDefn oBigIdField("global_id", OFTInteger64);
        if (poLayer->CreateField(&oBigIdField) != OGRERR_NONE)
        {
            printf("创建 global_id 字段失败\n");
        }

        // ========== 创建并写入要素 ==========
        // 要素 1
        OGRFeature *poFeature1 = OGRFeature::CreateFeature(poLayer->GetLayerDefn());
        poFeature1->SetField("name", "北京");
        poFeature1->SetField("population", 21540000);
        poFeature1->SetField("area_km2", 16410.54);
        poFeature1->SetField("global_id", (GIntBig)10000000001LL);

        OGRPoint pt1;
        pt1.setX(116.4074);
        pt1.setY(39.9042);
        poFeature1->SetGeometry(&pt1);

        if (poLayer->CreateFeature(poFeature1) != OGRERR_NONE)
        {
            printf("写入要素 1 失败\n");
        }
        OGRFeature::DestroyFeature(poFeature1);

        // 要素 2
        OGRFeature *poFeature2 = OGRFeature::CreateFeature(poLayer->GetLayerDefn());
        poFeature2->SetField("name", "上海");
        poFeature2->SetField("population", 24870000);
        poFeature2->SetField("area_km2", 6340.50);
        poFeature2->SetField("global_id", (GIntBig)10000000002LL);

        OGRPoint pt2;
        pt2.setX(121.4737);
        pt2.setY(31.2304);
        poFeature2->SetGeometry(&pt2);

        if (poLayer->CreateFeature(poFeature2) != OGRERR_NONE)
        {
            printf("写入要素 2 失败\n");
        }
        OGRFeature::DestroyFeature(poFeature2);

        // 刷新缓存并关闭
        poDS->FlushCache();
        GDALClose(poDS);

        printf("写入完成\n");
        return 0;
    }


.. _gdal3-field-types:

************************************
字段类型
************************************

``OGRFieldType`` 枚举定义了 OGR 支持的所有字段类型。GDAL 3.x 与 2.x 的字段类型基本一致，新增了 ``OFTInteger64`` 类型（GDAL 2.0 起支持）。

常用字段类型如下：

.. list-table::
   :header-rows: 1
   :widths: 25 15 60

   * - 类型常量
     - C++ 类型
     - 说明
   * - ``OFTInteger``
     - int
     - 32 位有符号整数
   * - ``OFTInteger64``
     - GIntBig (int64_t)
     - 64 位有符号整数（GDAL 2.0+）
   * - ``OFTReal``
     - double
     - 双精度浮点数
   * - ``OFTString``
     - const char*
     - UTF-8 字符串
   * - ``OFTDate``
     - int[3]
     - 日期（年、月、日）
   * - ``OFTTime``
     - int[4]
     - 时间（时、分、秒、时区偏移）
   * - ``OFTDateTime``
     - int[7]
     - 日期时间
   * - ``OFTIntegerList``
     - int[]
     - 整数列表
   * - ``OFTRealList``
     - double[]
     - 浮点数列表
   * - ``OFTStringList``
     - const char*[]
     - 字符串列表
   * - ``OFTBinary``
     - unsigned char[]
     - 二进制数据

``OGRFieldDefn`` 构造示例——演示各种字段类型的定义：

.. code-block:: c++

    #include "ogrsf_frmts.h"

    void defineFields(OGRLayer *poLayer)
    {
        // OFTInteger — 32 位整数
        OGRFieldDefn oIntField("count", OFTInteger);
        oIntField.SetWidth(10);
        poLayer->CreateField(&oIntField);

        // OFTInteger64 — 64 位整数，适用于大 ID 或计数
        OGRFieldDefn oInt64Field("global_id", OFTInteger64);
        poLayer->CreateField(&oInt64Field);

        // OFTReal — 浮点数，可设置精度
        OGRFieldDefn oRealField("temperature", OFTReal);
        oRealField.SetWidth(10);
        oRealField.SetPrecision(2);  // 保留 2 位小数
        poLayer->CreateField(&oRealField);

        // OFTString — 字符串，设置最大宽度
        OGRFieldDefn oStrField("name", OFTString);
        oStrField.SetWidth(128);
        poLayer->CreateField(&oStrField);

        // OFTDate — 日期
        OGRFieldDefn oDateField("create_date", OFTDate);
        poLayer->CreateField(&oDateField);

        // OFTDateTime — 完整日期时间
        OGRFieldDefn oDTField("update_time", OFTDateTime);
        poLayer->CreateField(&oDTField);

        // OFTBinary — 二进制数据
        OGRFieldDefn oBinField("thumbnail", OFTBinary);
        poLayer->CreateField(&oBinField);

        // ========== 设置字段值的不同方式 ==========
        OGRFeature *poFeature = OGRFeature::CreateFeature(poLayer->GetLayerDefn());

        // 按索引设置（索引从 0 开始，对应创建顺序）
        poFeature->SetField(0, 42);                    // OFTInteger
        poFeature->SetField(1, (GIntBig)9876543210LL); // OFTInteger64
        poFeature->SetField(2, 36.5);                  // OFTReal
        poFeature->SetField(3, "测试城市");              // OFTString

        // 按名称设置
        poFeature->SetField("count", 100);
        poFeature->SetField("name", "北京市");

        // 设置日期
        poFeature->SetField("create_date", 2024, 1, 15);

        // 设置日期时间
        poFeature->SetField("update_time", 2024, 1, 15, 14, 30, 0, 0);

        OGRFeature::DestroyFeature(poFeature);
    }


.. _gdal3-sealed:

************************************
Feature/FieldDefn 密封机制（RFC 97）
************************************

GDAL 3.9 引入了 RFC 97（Feature and FieldDefn Sealing），对 ``OGRFeature`` 和 ``OGRFieldDefn`` 实施密封（Sealed）机制，以提高性能和安全性。

密封机制的背景
======================

在 GDAL 3.9 之前，``OGRFeature::Clone()`` 和 ``OGRFieldDefn`` 的拷贝操作存在性能问题——每次拷贝都会深拷贝所有字段定义。更重要的是，从图层获取的要素共享 ``OGRFeatureDefn``，用户不应该修改这些共享的定义对象。

密封机制的规则
======================

* 从 ``OGRLayer::GetNextFeature()`` 或 ``OGRLayer::GetFeature()`` 获取的要素，其 ``OGRFeatureDefn`` 是**密封的**（sealed），不能被修改。
* 通过 ``OGRFeature::CreateFeature()`` 创建的独立要素，其定义**未密封**，可以修改。
* 调用 ``OGRLayer::CreateFeature()`` 写入后，要素的定义会被自动密封。
* ``OGRFieldDefn`` 被密封后，``SetName()``、``SetType()``、``SetWidth()`` 等修改方法会抛出 ``CPLInvalidStateException``。

代码示例——密封机制的行为：

.. code-block:: c++

    #include "ogrsf_frmts.h"

    void sealedDemo(OGRLayer *poLayer)
    {
        // 从图层获取的要素，其定义是密封的
        poLayer->ResetReading();
        OGRFeature *poFeature = poLayer->GetNextFeature();
        if (poFeature != NULL)
        {
            // 获取要素的定义
            OGRFeatureDefn *poFDefn = poFeature->GetDefnRef();

            // 尝试修改密封的定义——会抛出异常
            // 以下代码不应在生产中执行，仅作演示
            /*
            OGRFieldDefn *poFieldDefn = poFDefn->GetFieldDefn(0);
            poFieldDefn->SetName("new_name");  // CPLInvalidStateException!
            poFieldDefn->SetType(OFTReal);     // CPLInvalidStateException!
            */

            // 读取操作是安全的
            printf("字段数: %d\n", poFDefn->GetFieldCount());
            printf("第一个字段名: %s\n", poFDefn->GetFieldDefn(0)->GetNameRef());

            OGRFeature::DestroyFeature(poFeature);
        }

        // ========== 创建独立要素（未密封） ==========
        OGRFeature *poNewFeature = OGRFeature::CreateFeature(poLayer->GetLayerDefn());
        // 新要素的定义是从图层定义拷贝的，未密封，可以修改
        // 但通常不建议修改，应使用图层定义的结构
        poNewFeature->SetField("name", "新要素");
        OGRFeature::DestroyFeature(poNewFeature);

        // ========== 直接构造 FieldDefn（未密封） ==========
        OGRFieldDefn oField("temp_field", OFTString);
        oField.SetWidth(64);
        // 这是独立对象，未密封，可以自由修改
        oField.SetType(OFTReal);
        oField.SetWidth(20);
        oField.SetPrecision(6);
    }

.. attention::

    * GDAL 3.9+ 的密封机制是向后兼容的——现有代码通常无需修改。
    * 如果需要修改从图层获取的字段定义，应先调用 ``Clone()`` 创建副本。
    * 该机制主要用于防止意外修改共享定义，提升多线程安全性。


.. _gdal3-geometry:

************************************
几何操作全集
************************************

GDAL 3.x 中 ``OGRGeometry`` 类提供了丰富的几何操作方法。以下按功能分类详细说明。

几何创建
======================

``OGRGeometryFactory`` 是几何对象的工厂类，支持从 WKT、WKB 等格式创建几何对象。

.. code-block:: c++

    #include "ogr_geometry.h"

    void geometryCreation()
    {
        OGRGeometry *poGeom = NULL;

        // ========== 从 WKT 创建 ==========
        const char *pszWkt = "POINT (116.4074 39.9042)";
        OGRGeometryFactory::createFromWkt(pszWkt, NULL, &poGeom);
        if (poGeom != NULL)
        {
            printf("从 WKT 创建: %s\n", poGeom->exportToWkt());
            OGRGeometryFactory::destroyGeometry(poGeom);
        }

        // ========== 从 WKB 创建 ==========
        // WKB 是二进制格式，常用于数据库和网络传输
        unsigned char wkbData[] = {
            0x01,                         // 字节序：小端
            0x01, 0x00, 0x00, 0x00,       // 几何类型：Point (1)
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5E, 0x40,  // X: 120.0
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x40   // Y: 38.0
        };
        poGeom = NULL;
        OGRGeometryFactory::createFromWkb(wkbData, NULL, &poGeom, sizeof(wkbData));
        if (poGeom != NULL)
        {
            printf("从 WKB 创建: %s\n", poGeom->exportToWkt());
            OGRGeometryFactory::destroyGeometry(poGeom);
        }

        // ========== 直接构造几何对象 ==========
        // 点
        OGRPoint pt(116.4074, 39.9042);
        pt.setZ(50.0);  // 设置高程
        printf("点: %s\n", pt.exportToWkt());

        // 线
        OGRLineString line;
        line.addPoint(116.0, 39.0);
        line.addPoint(117.0, 40.0);
        line.addPoint(118.0, 41.0);
        printf("线: %s (点数=%d)\n", line.exportToWkt(), line.getNumPoints());

        // 多边形
        OGRPolygon poly;
        OGRLinearRing ring;
        ring.addPoint(116.0, 39.0);
        ring.addPoint(117.0, 39.0);
        ring.addPoint(117.0, 40.0);
        ring.addPoint(116.0, 40.0);
        ring.addPoint(116.0, 39.0);  // 闭合
        poly.addRing(&ring);
        printf("多边形: %s (面积=%.6f)\n", poly.exportToWkt(), poly.get_Area());

        // 多点
        OGRMultiPoint mpt;
        mpt.addGeometry(&pt);
        OGRPoint pt2(121.0, 31.0);
        mpt.addGeometry(&pt2);
        printf("多点: %s\n", mpt.exportToWkt());

        // 多线
        OGRMultiLineString mls;
        mls.addGeometry(&line);
        printf("多线: %s\n", mls.exportToWkt());

        // 多多边形
        OGRMultiPolygon mpoly;
        mpoly.addGeometry(&poly);
        printf("多多边形: %s\n", mpoly.exportToWkt());

        // 几何集合
        OGRGeometryCollection gc;
        gc.addGeometry(&pt);
        gc.addGeometry(&line);
        gc.addGeometry(&poly);
        printf("几何集合: %s\n", gc.exportToWkt());
    }


几何变换
======================

``OGRGeometry`` 支持坐标变换（通过 ``OGRCoordinateTransformation``）和克隆操作。

.. code-block:: c++

    #include "ogr_geometry.h"
    #include "ogr_spatialref.h"

    void geometryTransform()
    {
        // ========== 坐标变换 ==========
        // 创建源坐标系（WGS 84 经纬度）
        OGRSpatialReference oSrcSRS;
        oSrcSRS.SetWellKnownGeogCS("WGS84");

        // 创建目标坐标系（UTM Zone 50N）
        OGRSpatialReference oDstSRS;
        oDstSRS.SetUTM(50, TRUE);  // 50 带，北半球
        oDstSRS.SetWellKnownGeogCS("WGS84");

        // 创建坐标转换对象
        OGRCoordinateTransformation *poCT =
            OGRCreateCoordinateTransformation(&oSrcSRS, &oDstSRS);
        if (poCT == NULL)
        {
            printf("创建坐标转换失败\n");
            return;
        }

        // 变换点坐标
        OGRPoint pt(116.4074, 39.9042);
        printf("原始坐标: %s\n", pt.exportToWkt());

        // transform() 会就地修改几何坐标
        pt.transform(poCT);
        printf("变换后坐标: %s\n", pt.exportToWkt());

        // ========== 克隆几何 ==========
        // clone() 创建几何的深拷贝
        OGRPoint *ptClone = (OGRPoint *)pt.clone();
        printf("克隆: %s\n", ptClone->exportToWkt());

        // 克隆与原始独立，修改互不影响
        ptClone->setX(0.0);
        printf("原始: %s\n", pt.exportToWkt());
        printf("克隆后修改: %s\n", ptClone->exportToWkt());

        OGRGeometryFactory::destroyGeometry(ptClone);
        OCTDestroyCT(poCT);
    }


空间关系判断
======================

``OGRGeometry`` 提供多种空间关系判断方法，返回布尔值。

.. code-block:: c++

    #include "ogr_geometry.h"

    void spatialRelations()
    {
        // 构造两个多边形
        OGRPolygon polyA, polyB;

        // polyA: 116-117, 39-40
        OGRLinearRing ringA;
        ringA.addPoint(116.0, 39.0);
        ringA.addPoint(117.0, 39.0);
        ringA.addPoint(117.0, 40.0);
        ringA.addPoint(116.0, 40.0);
        ringA.addPoint(116.0, 39.0);
        polyA.addRing(&ringA);

        // polyB: 116.5-118.5, 39.5-40.5（与 polyA 相交）
        OGRLinearRing ringB;
        ringB.addPoint(116.5, 39.5);
        ringB.addPoint(118.5, 39.5);
        ringB.addPoint(118.5, 40.5);
        ringB.addPoint(116.5, 40.5);
        ringB.addPoint(116.5, 39.5);
        polyB.addRing(&ringB);

        // Intersect() — 是否相交
        printf("A 与 B 相交: %s\n", polyA.Intersect(&polyB) ? "是" : "否");

        // Contains() — A 是否包含 B
        printf("A 包含 B: %s\n", polyA.Contains(&polyB) ? "是" : "否");

        // Within() — A 是否在 B 内部
        printf("A 在 B 内: %s\n", polyA.Within(&polyB) ? "是" : "否");

        // Disjoint() — A 与 B 是否不相交
        printf("A 与 B 不相交: %s\n", polyA.Disjoint(&polyB) ? "是" : "否");

        // Equals() — A 与 B 是否相等
        printf("A 等于 B: %s\n", polyA.Equals(&polyB) ? "是" : "否");

        // Touches() — A 与 B 是否仅边界接触
        printf("A 接触 B: %s\n", polyA.Touches(&polyB) ? "是" : "否");

        // Crosses() — A 与 B 是否交叉
        printf("A 交叉 B: %s\n", polyA.Crosses(&polyB) ? "是" : "否");

        // Overlaps() — A 与 B 是否重叠
        printf("A 重叠 B: %s\n", polyA.Overlaps(&polyB) ? "是" : "否");

        // Intersection() — 获取相交部分的几何
        OGRGeometry *poIntersection = polyA.Intersection(&polyB);
        if (poIntersection != NULL)
        {
            printf("A 与 B 的交集: %s\n", poIntersection->exportToWkt());
            printf("交集面积: %.6f\n",
                   ((OGRPolygon *)poIntersection)->get_Area());
            OGRGeometryFactory::destroyGeometry(poIntersection);
        }
    }


几何处理
======================

``OGRGeometry`` 提供多种几何处理方法，包括简化、有效性修复、缓冲区、合并、差集等。

.. code-block:: c++

    #include "ogr_geometry.h"

    void geometryProcessing()
    {
        // 构造一个复杂线
        OGRLineString line;
        line.addPoint(0.0, 0.0);
        line.addPoint(1.0, 0.1);
        line.addPoint(2.0, -0.1);
        line.addPoint(3.0, 0.05);
        line.addPoint(4.0, 0.0);
        line.addPoint(5.0, -0.05);
        printf("原始线: %s\n", line.exportToWkt());
        printf("原始点数: %d\n", line.getNumPoints());

        // ========== Simplify() — 线简化 ==========
        // 使用 Douglas-Peucker 算法，tolerance 为简化容差
        OGRGeometry *poSimple = line.Simplify(0.2);
        if (poSimple != NULL)
        {
            printf("简化后: %s\n", poSimple->exportToWkt());
            printf("简化后点数: %d\n",
                   ((OGRLineString *)poSimple)->getNumPoints());
            OGRGeometryFactory::destroyGeometry(poSimple);
        }

        // ========== Buffer() — 缓冲区 ==========
        // 围绕点创建缓冲区
        OGRPoint pt(116.4, 39.9);
        OGRGeometry *poBuffer = pt.Buffer(0.5);  // 0.5 度缓冲区
        if (poBuffer != NULL)
        {
            printf("缓冲区类型: %s\n", poBuffer->getGeometryName());
            printf("缓冲区面积: %.6f\n",
                   ((OGRPolygon *)poBuffer)->get_Area());
            OGRGeometryFactory::destroyGeometry(poBuffer);
        }

        // ========== MakeValid() — 修复无效几何 ==========
        // GDAL 3.x 内置 MakeValid，不再依赖 GEOS
        OGRPolygon invalidPoly;
        OGRLinearRing badRing;
        // 构造一个自相交的多边形（8 字形）
        badRing.addPoint(0.0, 0.0);
        badRing.addPoint(2.0, 2.0);
        badRing.addPoint(2.0, 0.0);
        badRing.addPoint(0.0, 2.0);
        badRing.addPoint(0.0, 0.0);
        invalidPoly.addRing(&badRing);

        printf("原始几何有效: %s\n", invalidPoly.IsValid() ? "是" : "否");
        OGRGeometry *poValid = invalidPoly.MakeValid();
        if (poValid != NULL)
        {
            printf("修复后: %s\n", poValid->exportToWkt());
            printf("修复后有效: %s\n", poValid->IsValid() ? "是" : "否");
            OGRGeometryFactory::destroyGeometry(poValid);
        }

        // ========== Union() — 合并 ==========
        OGRPolygon poly1, poly2;
        OGRLinearRing r1, r2;
        r1.addPoint(0.0, 0.0); r1.addPoint(2.0, 0.0);
        r1.addPoint(2.0, 2.0); r1.addPoint(0.0, 2.0);
        r1.addPoint(0.0, 0.0);
        poly1.addRing(&r1);

        r2.addPoint(1.0, 1.0); r2.addPoint(3.0, 1.0);
        r2.addPoint(3.0, 3.0); r2.addPoint(1.0, 3.0);
        r2.addPoint(1.0, 1.0);
        poly2.addRing(&r2);

        OGRGeometry *poUnion = poly1.Union(&poly2);
        if (poUnion != NULL)
        {
            printf("合并结果: %s\n", poUnion->exportToWkt());
            OGRGeometryFactory::destroyGeometry(poUnion);
        }

        // ========== Difference() — 差集 ==========
        OGRGeometry *poDiff = poly1.Difference(&poly2);
        if (poDiff != NULL)
        {
            printf("差集结果: %s\n", poDiff->exportToWkt());
            OGRGeometryFactory::destroyGeometry(poDiff);
        }

        // ========== SymDifference() — 对称差集 ==========
        OGRGeometry *poSymDiff = poly1.SymDifference(&poly2);
        if (poSymDiff != NULL)
        {
            printf("对称差集: %s\n", poSymDiff->exportToWkt());
            OGRGeometryFactory::destroyGeometry(poSymDiff);
        }
    }


几何导出
======================

``OGRGeometry`` 支持导出为多种格式。

.. code-block:: c++

    #include "ogr_geometry.h"

    void geometryExport()
    {
        OGRPolygon poly;
        OGRLinearRing ring;
        ring.addPoint(116.0, 39.0);
        ring.addPoint(117.0, 39.0);
        ring.addPoint(117.0, 40.0);
        ring.addPoint(116.0, 40.0);
        ring.addPoint(116.0, 39.0);
        poly.addRing(&ring);

        // ========== exportToWkt() — 导出为 WKT ==========
        char *pszWkt = NULL;
        poly.exportToWkt(&pszWkt);
        printf("WKT: %s\n", pszWkt);
        CPLFree(pszWkt);

        // 带精度控制的 WKT 导出
        char *pszWktPrecision = NULL;
        char **papszOptions = CSLSetNameValue(NULL, "FORMAT", "SFSQL");
        papszOptions = CSLSetNameValue(papszOptions, "PRECISION", "8");
        poly.exportToWkt(&pszWktPrecision, papszOptions);
        printf("WKT (精度=8): %s\n", pszWktPrecision);
        CPLFree(pszWktPrecision);
        CSLDestroy(papszOptions);

        // ========== exportToWkb() — 导出为 WKB ==========
        int nWkbSize = poly.WkbSize();
        unsigned char *pabyWkb = (unsigned char *)CPLMalloc(nWkbSize);
        poly.exportToWkb(wkbNDR, pabyWkb);  // wkbNDR = 小端序
        printf("WKB 大小: %d 字节\n", nWkbSize);
        CPLFree(pabyWkb);

        // ========== exportToGML() — 导出为 GML ==========
        char *pszGML = poly.exportToGML();
        printf("GML: %s\n", pszGML);
        CPLFree(pszGML);

        // ========== exportToJSON() — 导出为 GeoJSON ==========
        char *pszJSON = poly.exportToJson();
        printf("GeoJSON: %s\n", pszJSON);
        CPLFree(pszJSON);
    }


几何生命周期管理
======================

OGR 几何对象的内存管理遵循明确的规则。``OGRGeometryFactory::destroyGeometry()`` 是释放几何对象的标准方法。

.. code-block:: c++

    #include "ogr_geometry.h"

    void geometryLifecycle()
    {
        // ========== 规则 1: 工厂创建的几何，用 destroyGeometry() 释放 ==========
        OGRGeometry *poGeom = NULL;
        OGRGeometryFactory::createFromWkt(
            "POINT (116.4 39.9)", NULL, &poGeom);
        // ... 使用 poGeom ...
        OGRGeometryFactory::destroyGeometry(poGeom);

        // ========== 规则 2: clone() 创建的几何，用 destroyGeometry() 释放 ==========
        OGRPoint pt(116.4, 39.9);
        OGRGeometry *poClone = pt.clone();
        // ... 使用 poClone ...
        OGRGeometryFactory::destroyGeometry(poClone);

        // ========== 规则 3: 栈上几何自动释放 ==========
        {
            OGRPoint stackPt(116.4, 39.9);  // 栈上对象
            printf("栈上点: %s\n", stackPt.exportToWkt());
        } // stackPt 在此自动析构

        // ========== 规则 4: GetGeometryRef() 返回的指针不要释放 ==========
        // Feature 的 GetGeometryRef() 返回内部引用
        // 不要调用 destroyGeometry() 释放它！
        OGRFeature *poFeature = OGRFeature::CreateFeature(NULL);
        OGRPoint ptSet(116.4, 39.9);
        poFeature->SetGeometry(&ptSet);
        OGRGeometry *poRef = poFeature->GetGeometryRef();
        // poRef 是内部引用，不要释放！
        // OGRGeometryFactory::destroyGeometry(poRef);  // 错误！
        OGRFeature::DestroyFeature(poFeature);  // 通过 DestroyFeature 释放

        // ========== 规则 5: 几何运算结果用 destroyGeometry() 释放 ==========
        OGRPolygon poly1, poly2;
        OGRLinearRing r1, r2;
        r1.addPoint(0, 0); r1.addPoint(2, 0);
        r1.addPoint(2, 2); r1.addPoint(0, 2); r1.addPoint(0, 0);
        poly1.addRing(&r1);
        r2.addPoint(1, 1); r2.addPoint(3, 1);
        r2.addPoint(3, 3); r2.addPoint(1, 3); r2.addPoint(1, 1);
        poly2.addRing(&r2);

        OGRGeometry *poResult = poly1.Intersection(&poly2);
        // ... 使用 poResult ...
        OGRGeometryFactory::destroyGeometry(poResult);
    }


各种几何类型详解
======================

GDAL 中的几何类型层次结构：

* ``OGRPoint`` — 点，包含 X、Y、Z、M 坐标
* ``OGRLineString`` — 线，点的有序序列
* ``OGRLinearRing`` — 线性环（继承自 ``OGRLineString``），用于构成多边形的边界
* ``OGRCircularString`` — 弧线（GDAL 3.x 增强支持）
* ``OGRCompoundCurve`` — 复合曲线（GDAL 3.x 增强支持）
* ``OGRCurvePolygon`` — 曲线多边形（GDAL 3.x 增强支持）
* ``OGRPolygon`` — 多边形，由外环和零个或多个内环（孔洞）组成
* ``OGRMultiPoint`` — 多点集合
* ``OGRMultiLineString`` — 多线集合
* ``OGRMultiPolygon`` — 多多边形集合
* ``OGRGeometryCollection`` — 几何集合，可包含任意类型的几何

各类型的常用操作示例：

.. code-block:: c++

    #include "ogr_geometry.h"
    #include <vector>

    void geometryTypeExamples()
    {
        // ========== OGRPoint ==========
        OGRPoint pt(116.4074, 39.9042, 50.0);  // X, Y, Z
        pt.setM(100.0);  // 设置 M 值（度量值）
        printf("Point: X=%.4f, Y=%.4f, Z=%.1f, M=%.1f\n",
               pt.getX(), pt.getY(), pt.getZ(), pt.getM());
        printf("  维度: %d\n", pt.getCoordinateDimension());
        printf("  是否 3D: %s\n", pt.Is3D() ? "是" : "否");
        printf("  是否有 M: %s\n", pt.IsMeasured() ? "是" : "否");

        // ========== OGRLineString ==========
        OGRLineString line;
        line.setNumPoints(5);
        line.setPoint(0, 0.0, 0.0);
        line.setPoint(1, 1.0, 1.0);
        line.setPoint(2, 2.0, 0.5);
        line.setPoint(3, 3.0, 1.5);
        line.setPoint(4, 4.0, 0.0);
        printf("LineString: 点数=%d, 长度=%.4f\n",
               line.getNumPoints(), line.getLength());

        // 获取线的起止点
        OGRPoint startPt, endPt;
        line.getStartPoint(&startPt);
        line.getEndPoint(&endPt);
        printf("  起点: (%.1f, %.1f)\n", startPt.getX(), startPt.getY());
        printf("  终点: (%.1f, %.1f)\n", endPt.getX(), endPt.getY());

        // ========== OGRPolygon ==========
        OGRPolygon poly;
        OGRLinearRing outer;
        outer.addPoint(0.0, 0.0);
        outer.addPoint(10.0, 0.0);
        outer.addPoint(10.0, 10.0);
        outer.addPoint(0.0, 10.0);
        outer.addPoint(0.0, 0.0);
        poly.addRing(&outer);

        // 添加内环（孔洞）
        OGRLinearRing hole;
        hole.addPoint(2.0, 2.0);
        hole.addPoint(4.0, 2.0);
        hole.addPoint(4.0, 4.0);
        hole.addPoint(2.0, 4.0);
        hole.addPoint(2.0, 2.0);
        poly.addRing(&hole);

        printf("Polygon: 环数=%d, 面积=%.1f, 周长=%.1f\n",
               poly.getNumInteriorRings() + 1,
               poly.get_Area(),
               poly.getExteriorRing()->get_Length());

        // 获取外环和内环
        OGRLinearRing *poExterior = poly.getExteriorRing();
        printf("  外环点数: %d\n", poExterior->getNumPoints());
        if (poly.getNumInteriorRings() > 0)
        {
            OGRLinearRing *poInterior = poly.getInteriorRing(0);
            printf("  第一个内环点数: %d\n", poInterior->getNumPoints());
        }

        // ========== OGRMultiPoint ==========
        OGRMultiPoint mpt;
        OGRPoint p1(0.0, 0.0), p2(1.0, 1.0), p3(2.0, 2.0);
        mpt.addGeometry(&p1);
        mpt.addGeometry(&p2);
        mpt.addGeometry(&p3);
        printf("MultiPoint: 几何数=%d\n", mpt.getNumGeometries());

        // 遍历子几何
        for (int i = 0; i < mpt.getNumGeometries(); i++)
        {
            OGRPoint *poSubPt = (OGRPoint *)mpt.getGeometryRef(i);
            printf("  [%d] (%.1f, %.1f)\n", i, poSubPt->getX(), poSubPt->getY());
        }

        // ========== OGRMultiPolygon ==========
        OGRMultiPolygon mpoly;
        mpoly.addGeometry(&poly);
        printf("MultiPolygon: 几何数=%d\n", mpoly.getNumGeometries());

        // ========== OGRGeometryCollection ==========
        OGRGeometryCollection gc;
        gc.addGeometry(&pt);
        gc.addGeometry(&line);
        gc.addGeometry(&poly);
        printf("GeometryCollection: 几何数=%d\n", gc.getNumGeometries());

        // ========== 几何类型判断 ==========
        printf("\n类型判断:\n");
        printf("  pt 是点: %s\n",
               wkbFlatten(pt.getGeometryType()) == wkbPoint ? "是" : "否");
        printf("  line 是线: %s\n",
               wkbFlatten(line.getGeometryType()) == wkbLineString ? "是" : "否");
        printf("  poly 是多边形: %s\n",
               wkbFlatten(poly.getGeometryType()) == wkbPolygon ? "是" : "否");
        printf("  gc 是集合: %s\n",
               (wkbFlatten(gc.getGeometryType()) == wkbGeometryCollection)
                   ? "是" : "否");

        // 空几何判断
        OGRPoint emptyPt;
        printf("  emptyPt 是空几何: %s\n", emptyPt.IsEmpty() ? "是" : "否");
    }


.. _gdal3-arrow:

************************************
Arrow/列式 API（RFC 86）
************************************

GDAL 3.6 引入了 RFC 86（Column-oriented read API），提供基于 Apache Arrow 的列式数据读取接口。相比传统的逐要素遍历，Arrow API 可以一次性读取整列数据，大幅提升批量处理性能。

Arrow API 的核心思想
======================

* 以列（column）而非行（feature）为单位组织数据，更契合现代 CPU 的缓存预取和 SIMD 优化。
* 使用 Apache Arrow 内存格式（``struct ArrowArray`` 和 ``struct ArrowSchema``），可与 Python Arrow/Parquet、Rust Arrow 等生态无缝对接。
* 通过 ``GetArrowStream()`` 方法获取 ``ArrowArrayStream`` 接口。

基本用法
======================

.. code-block:: c++

    #include "ogrsf_frmts.h"
    #include "ogr_recordbatch.h"  // Arrow API 头文件
    #include <vector>
    #include <string>

    void arrowReadExample(OGRLayer *poLayer)
    {
        // 获取 Arrow 流
        struct ArrowArrayStream stream;
        if (!poLayer->GetArrowStream(&stream, NULL))
        {
            printf("不支持 Arrow 流\n");
            return;
        }

        // 获取 schema（字段定义）
        struct ArrowSchema schema;
        stream.get_schema(&stream, &schema);
        printf("字段数量: %lld\n", (long long)schema.n_children);
        for (int64_t i = 0; i < schema.n_children; i++)
        {
            printf("  [%lld] %s (格式: %s)\n",
                   (long long)i,
                   schema.children[i]->name,
                   schema.children[i]->format);
        }
        schema.release(&schema);

        // 逐批读取数据
        struct ArrowArray array;
        int nBatch = 0;
        while (stream.get_next(&stream, &array) == 0 && array.release != NULL)
        {
            nBatch++;
            printf("批次 %d: %lld 行\n", nBatch, (long long)array.length);

            // array.n_children 个子数组，每个对应一个字段列
            // array.children[i]->buffers 包含该列的数据缓冲区
            // 具体的缓冲区布局取决于数据类型（Arrow 格式规范）

            array.release(&array);
        }

        stream.release(&stream);
    }

.. attention::

    * Arrow API 是 GDAL 3.6+ 的功能，需要编译时启用 Arrow/Parquet 支持。
    * 部分驱动可能不支持 Arrow 流，调用前应检查返回值。
    * Arrow API 主要面向高性能批量场景，简单的逐要素遍历仍建议使用传统 API。


.. _gdal3-practical:

************************************
实战模式参考
************************************

以下介绍几种在实际生产环境中常见的 GDAL 矢量操作模式。

多线程打开多个数据集实例
==========================

GDAL 的 ``GDALDataset`` 对象本身不是线程安全的。在多线程环境中，每个线程应独立打开自己的数据集实例。通过 ``GDAL_OF_SHARED`` 标志可以在同一进程内共享驱动注册，但每个线程的 ``GDALDataset`` 指针必须独立。

.. code-block:: c++

    #include "ogrsf_frmts.h"
    #include <thread>
    #include <vector>
    #include <mutex>
    #include <string>

    // 全局互斥锁——用于保护非线程安全的操作（如坐标转换）
    std::mutex g_mutex;

    /**
     * 单个工作线程的数据处理函数。
     * 每个线程独立打开数据集，避免竞争。
     */
    void workerThread(const std::string &shpPath, int threadId)
    {
        CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
        CPLSetConfigOption("SHAPE_ENCODING", "");

        // 每个线程独立打开数据集
        GDALDataset *poDS = (GDALDataset *)GDALOpenEx(
            shpPath.c_str(),
            GDAL_OF_VECTOR | GDAL_OF_READONLY,
            NULL, NULL, NULL);

        if (poDS == NULL)
        {
            printf("[线程 %d] 打开失败: %s\n", threadId, CPLGetLastErrorMsg());
            return;
        }

        OGRLayer *poLayer = poDS->GetLayer(0);
        if (poLayer == NULL)
        {
            printf("[线程 %d] 获取图层失败\n", threadId);
            GDALClose(poDS);
            return;
        }

        // 每个线程独立遍历
        OGRFeature *poFeature;
        int nCount = 0;
        poLayer->ResetReading();
        while ((poFeature = poLayer->GetNextFeature()) != NULL)
        {
            // 在此处理每个要素...
            nCount++;
            OGRFeature::DestroyFeature(poFeature);
        }

        printf("[线程 %d] 处理完成，共 %d 个要素\n", threadId, nCount);

        // 每个线程独立关闭数据集
        GDALClose(poDS);
    }

    /**
     * 多线程处理入口：将多个文件分配给不同线程。
     */
    void multiThreadProcess(const std::vector<std::string> &fileList)
    {
        // 只需注册一次（主线程中）
        GDALAllRegister();

        std::vector<std::thread> threads;
        for (size_t i = 0; i < fileList.size(); i++)
        {
            threads.emplace_back(workerThread, fileList[i], (int)i);
        }

        for (auto &t : threads)
        {
            t.join();
        }

        printf("所有线程处理完成\n");
    }


坐标转换加锁
==========================

``OGRCoordinateTransformation`` 对象不是线程安全的。当多个线程共享同一个坐标转换对象时，必须通过互斥锁保护。更好的做法是每个线程创建独立的转换对象。

.. code-block:: c++

    #include "ogrsf_frmts.h"
    #include "ogr_spatialref.h"
    #include <mutex>

    extern std::mutex g_mutex;

    /**
     * 线程安全的坐标转换示例。
     * 方案一：共享 CT 对象，加锁保护。
     */
    void transformWithLock(
        OGRGeometry *poGeom,
        OGRCoordinateTransformation *poCT)
    {
        // 加锁——transform() 不是线程安全的
        std::lock_guard<std::mutex> lock(g_mutex);
        poGeom->transform(poCT);
    }

    /**
     * 方案二（推荐）：每个线程创建独立的 CT 对象，无需加锁。
     */
    OGRCoordinateTransformation *createCT()
    {
        OGRSpatialReference oSrcSRS, oDstSRS;
        oSrcSRS.SetWellKnownGeogCS("WGS84");
        oDstSRS.SetUTM(50, TRUE);
        oDstSRS.SetWellKnownGeogCS("WGS84");
        return OGRCreateCoordinateTransformation(&oSrcSRS, &oDstSRS);
    }

    void workerWithOwnCT(const std::string &shpPath, int threadId)
    {
        GDALDataset *poDS = (GDALDataset *)GDALOpenEx(
            shpPath.c_str(),
            GDAL_OF_VECTOR | GDAL_OF_READONLY,
            NULL, NULL, NULL);
        if (poDS == NULL) return;

        // 每个线程创建独立的坐标转换对象
        OGRCoordinateTransformation *poCT = createCT();
        if (poCT == NULL)
        {
            GDALClose(poDS);
            return;
        }

        OGRLayer *poLayer = poDS->GetLayer(0);
        OGRFeature *poFeature;
        poLayer->ResetReading();
        while ((poFeature = poLayer->GetNextFeature()) != NULL)
        {
            OGRGeometry *poGeom = poFeature->GetGeometryRef();
            if (poGeom != NULL)
            {
                // 无需加锁——每个线程有独立的 CT
                OGRGeometry *poClone = poGeom->clone();
                poClone->transform(poCT);
                // ... 处理变换后的几何 ...
                OGRGeometryFactory::destroyGeometry(poClone);
            }
            OGRFeature::DestroyFeature(poFeature);
        }

        OCTDestroyCT(poCT);
        GDALClose(poDS);
    }


GBK/UTF-8 编码回退
==========================

在中国 GIS 数据处理中，经常遇到 GBK/GB2312 编码的 Shapefile 属性数据（dbf 文件）。GDAL 默认期望 UTF-8 编码，需要特殊处理。

.. code-block:: c++

    #include "ogrsf_frmts.h"
    #include "cpl_conv.h"
    #include <string>

    #ifdef _WIN32
    #include <windows.h>
    #endif

    /**
     * 将 GBK 字符串转换为 UTF-8。
     * 在 Windows 下使用 Win32 API，在其他平台使用 CPLRecode。
     */
    std::string gbkToUtf8(const std::string &gbkStr)
    {
    #ifdef _WIN32
        // Windows 方案：MultiByteToWideChar + WideCharToMultiByte
        int wLen = MultiByteToWideChar(CP_ACP, 0, gbkStr.c_str(), -1, NULL, 0);
        if (wLen <= 0) return gbkStr;

        wchar_t *wBuf = new wchar_t[wLen];
        MultiByteToWideChar(CP_ACP, 0, gbkStr.c_str(), -1, wBuf, wLen);

        int uLen = WideCharToMultiByte(CP_UTF8, 0, wBuf, -1, NULL, 0, NULL, NULL);
        if (uLen <= 0) { delete[] wBuf; return gbkStr; }

        char *uBuf = new char[uLen];
        WideCharToMultiByte(CP_UTF8, 0, wBuf, -1, uBuf, uLen, NULL, NULL);

        std::string result(uBuf);
        delete[] wBuf;
        delete[] uBuf;
        return result;
    #else
        // Linux/macOS 方案：使用 GDAL 的 CPLRecode
        return std::string(CPLRecode(gbkStr.c_str(), "CP936", "UTF-8"));
    #endif
    }

    /**
     * 带编码回退的属性读取。
     * 先尝试 UTF-8 读取，若检测到非 UTF-8 字节则按 GBK 回退。
     */
    std::string readFieldWithEncodingFallback(OGRFeature *poFeature, int iField)
    {
        const char *pszRaw = poFeature->GetFieldAsString(iField);
        if (pszRaw == NULL || pszRaw[0] == '\0')
            return "";

        // 检测是否已经是合法的 UTF-8
        // 简单判断：如果包含高位字节且不是 UTF-8 前缀模式，则按 GBK 处理
        bool isUtf8 = true;
        const unsigned char *p = (const unsigned char *)pszRaw;
        while (*p)
        {
            if (*p < 0x80)
            {
                p++;
            }
            else if ((*p & 0xE0) == 0xC0 && (p[1] & 0xC0) == 0x80)
            {
                p += 2;
            }
            else if ((*p & 0xF0) == 0xE0 && (p[1] & 0xC0) == 0x80
                     && (p[2] & 0xC0) == 0x80)
            {
                p += 3;
            }
            else if ((*p & 0xF8) == 0xF0 && (p[1] & 0xC0) == 0x80
                     && (p[2] & 0xC0) == 0x80 && (p[3] & 0xC0) == 0x80)
            {
                p += 4;
            }
            else
            {
                isUtf8 = false;
                break;
            }
        }

        if (isUtf8)
            return std::string(pszRaw);
        else
            return gbkToUtf8(std::string(pszRaw));
    }

    /**
     * 完整的编码自适应数据读取示例。
     */
    void readWithEncoding(const char *pszShpPath)
    {
        // 关闭 SHAPE_ENCODING，让 GDAL 不做自动转换
        CPLSetConfigOption("SHAPE_ENCODING", "");
        CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

        GDALDataset *poDS = (GDALDataset *)GDALOpenEx(
            pszShpPath,
            GDAL_OF_VECTOR | GDAL_OF_READONLY,
            NULL, NULL, NULL);
        if (poDS == NULL)
        {
            printf("打开失败\n");
            return;
        }

        OGRLayer *poLayer = poDS->GetLayer(0);
        OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();

        OGRFeature *poFeature;
        poLayer->ResetReading();
        while ((poFeature = poLayer->GetNextFeature()) != NULL)
        {
            for (int i = 0; i < poFDefn->GetFieldCount(); i++)
            {
                OGRFieldDefn *poFd = poFDefn->GetFieldDefn(i);
                if (poFd->GetType() == OFTString)
                {
                    // 使用编码回退读取
                    std::string value =
                        readFieldWithEncodingFallback(poFeature, i);
                    printf("  %s: %s\n", poFd->GetNameRef(), value.c_str());
                }
            }
            OGRFeature::DestroyFeature(poFeature);
        }

        GDALClose(poDS);
    }

.. attention::

    * GDAL 的 ``SHAPE_ENCODING`` 配置项可以自动将 dbf 文件从指定编码转换为 UTF-8，但在编码未知时可能导致乱码。
    * 设置 ``SHAPE_ENCODING`` 为空字符串（``""``）可禁用自动转换，由应用层手动处理编码。
    * GDAL 3.x 默认对 Shapefile 使用 UTF-8 编码输出，如果数据源是 GBK，务必正确配置编码。
    * 对于 GeoJSON、GeoPackage 等现代格式，编码始终为 UTF-8，无需特殊处理。
