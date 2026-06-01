.. highlight:: rst
.. _gdal3gnm:

##################################
GDAL 地理网络分析（GNM）
##################################

GDAL 内置了地理网络模型（Geographic Network Model, GNM）子系统，自 GDAL 2.1 起引入。
GNM 将地理要素建模为图（Graph）中的顶点和边，支持最短路径、K 最短路径、连通分量等网络分析功能。
本章介绍 GNM 的架构、CLI 工具以及 C/C++ 编程接口。

.. contents:: 目录
   :depth: 2
   :local:

.. _gnm-architecture:

******************
GNM 架构概述
******************

GNM 子系统位于 ``gnm/`` 目录，核心组件包括：

- **GNMGraph** — 内存图引擎，使用 STL 容器存储顶点和边，实现 Dijkstra、K 最短路径、连通分量算法
- **GNMNetwork** — 抽象网络基类（继承自 ``GDALDataset``），定义网络操作接口
- **GNMGenericNetwork** — 通用网络实现，提供连接管理、规则系统、自动拓扑构建
- **GNMFile / GNMDatabase** — 两种存储后端驱动

.. _gnm-data-model:

数据模型
========

GNM 将 OGR 数据集中的要素（Feature）视为图的节点，通过显式连接（Edge）建立拓扑关系。

核心数据结构：

- ``GNMGFID`` — 全局要素标识符（``GIntBig``，64 位整数），在所有网络图层中唯一
- ``GNMStdVertex`` — 顶点，包含出边列表和阻塞状态
- ``GNMStdEdge`` — 边，包含源/目标顶点 FID、方向、正向/反向代价、阻塞状态
- ``GNMPATH`` — 路径结果，``std::vector<std::pair<GNMGFID, GNMGFID>>`` （顶点 FID, 边 FID 对）

边方向常量：

- ``GNM_EDGE_DIR_BOTH`` (0) — 双向
- ``GNM_EDGE_DIR_SRCTOTGT`` (1) — 源到目标
- ``GNM_EDGE_DIR_TGTTOSRC`` (2) — 目标到源

图算法类型（ ``GNMGraphAlgorithmType`` 枚举）：

- ``GATDijkstraShortestPath`` (1) — Dijkstra 最短路径
- ``GATKShortestPath`` (2) — Yen's K 最短路径
- ``GATConnectedComponents`` (3) — 连通分量（BFS）

.. _gnm-storage:

存储驱动
========

GNM 有两种存储后端：

GNMFile
-------

基于文件系统的存储。网络元数据、图结构、要素分别存储为目录下的 Shapefile（或其他 OGR 格式）。
默认图层格式为 ESRI Shapefile。

创建时需指定目录路径，GNM 会在该目录下创建网络文件结构。

GNMDatabase
-----------

基于数据库的存储。网络数据存储在关系数据库中（如 SQLite/SpatiaLite、PostgreSQL）。

两种驱动通过 ``GNMRegisterAll()`` 注册。

.. _gnm-cli:

******************
CLI 工具
******************

GNM 提供两个命令行工具： ``gnmmanage`` 用于网络管理， ``gnmanalyse`` 用于网络分析。

.. _gnmmanage:

gnmmanage — 网络管理
=====================

``gnmmanage`` 支持以下子命令：

**info** — 查看网络信息：

.. code-block:: bash

    gnmmanage info -ds my_network/

**create** — 创建网络：

.. code-block:: bash

    # 创建基于文件的网络
    gnmmanage create -ds my_network/ -f "ESRI Shapefile" \
        -dsco FORMAT=ESRI_Shapefile

    # 创建基于数据库的网络
    gnmmanage create -ds network.sqlite -f SQLite

**import** — 导入图层到网络：

.. code-block:: bash

    gnmmanage import -ds my_network/ -l roads roads.shp
    gnmmanage import -ds my_network/ -l intersections points.shp

**connect** — 手动连接两个要素：

.. code-block:: bash

    # 连接两个要素（指定源 FID、目标 FID、连接要素 FID、代价、方向）
    gnmmanage connect -ds my_network/ 100 200 -1 -cost 1.5 -inv_cost 2.0 -dir both

**autoconnect** — 自动构建拓扑：

.. code-block:: bash

    # 自动连接点图层和线图层，捕捉容差 0.001
    gnmmanage autoconnect -ds my_network/ -tolerance 0.001

**disconnect** — 断开连接：

.. code-block:: bash

    gnmmanage disconnect -ds my_network/ 100 200

**rule** — 添加连通规则：

.. code-block:: bash

    gnmmanage rule -ds my_network/ "ALLOW CONNECTS ANY"

**change** — 阻塞/解阻塞要素：

.. code-block:: bash

    # 阻塞要素
    gnmmanage change -ds my_network/ -bl 100

    # 解阻塞要素
    gnmmanage change -ds my_network/ -unbl 100

    # 解阻塞全部
    gnmmanage change -ds my_network/ -unblall

**delete** — 删除网络：

.. code-block:: bash

    gnmmanage delete -ds my_network/

.. _gnmanalyse:

gnmanalyse — 网络分析
=======================

``gnmanalyse`` 支持以下子命令：

**dijkstra** — Dijkstra 最短路径：

.. code-block:: bash

    # 从要素 100 到要素 200 的最短路径
    gnmanalyse dijkstra -ds my_network/ 100 200 \
        -f GeoJSON -ds result.geojson

**kpaths** — K 最短路径（Yen's 算法）：

.. code-block:: bash

    # 查找 3 条最短路径
    gnmanalyse kpaths -ds my_network/ 100 200 3 \
        -f GeoJSON -ds result.geojson

**resource** — 连通分量（资源分配）：

.. code-block:: bash

    # 从发射器要素出发，查找所有可达要素
    gnmanalyse resource -ds my_network/ \
        -f GeoJSON -ds result.geojson

结果图层字段说明：

- ``gnm_fid`` — 要素的全局 FID
- ``ogrlayer`` — 所属图层名
- ``path_num`` — 路径编号（K 最短路径时区分不同路径）
- ``ftype`` — 类型：``EDGE`` 或 ``VERTEX``

.. _gnm-connectivity-rules:

******************
连通规则
******************

GNM 的规则系统控制哪些图层之间可以建立连接。规则在 ``ConnectFeatures()`` 时自动检查。

规则语法：

- ``"ALLOW CONNECTS ANY"`` — 允许所有连接（网络创建时的默认规则）
- ``"DENY CONNECTS ANY"`` — 禁止所有连接
- ``"DENY CONNECTS layer1 WITH layer2"`` — 禁止特定图层对之间的连接
- ``"ALLOW CONNECTS layer1 WITH layer2 VIA layer3"`` — 允许通过特定连接图层连接

.. code-block:: c++

    // C++ 设置规则
    poNetwork->CreateRule("DENY CONNECTS water_pipes WITH gas_pipes");
    poNetwork->CreateRule("ALLOW CONNECTS wells WITH pipes VIA junctions");

    // 获取当前规则列表
    char **papszRules = poNetwork->GetRules();
    CSLDestroy(papszRules);

.. _gnm-cpp-api:

******************
C++ 编程接口
******************

头文件： ``gnm/gnm.h``

网络创建与打开
==============

.. code-block:: c++

    #include "gnm.h"
    #include "ogrsf_frmts.h"

    // 注册所有驱动（包括 GNM 驱动）
    GDALAllRegister();
    GNMRegisterAll();

    // ===== 创建网络 =====
    // 获取 GNMFile 驱动
    GDALDriver *poDriver = GetGDALDriverManager()->GetDriverByName("GNMFile");
    if (poDriver == nullptr) {
        printf("GNMFile 驱动不可用\n");
        return 1;
    }

    // 创建网络（指定目录路径）
    const char *apszCreateOpts[] = {
        "FORMAT=ESRI_Shapefile",  // 要素图层格式
        nullptr
    };
    GDALDataset *poDS = poDriver->Create(
        "my_network",           // 网络目录路径
        0, 0, 0, GDT_Unknown,
        const_cast<char **>(apszCreateOpts));

    GNMGenericNetwork *poNetwork = dynamic_cast<GNMGenericNetwork *>(poDS);
    if (poNetwork == nullptr) {
        printf("创建网络失败\n");
        return 1;
    }

    // ===== 打开网络 =====
    GDALDataset *poDS2 = static_cast<GDALDataset *>(
        GDALOpenEx("my_network", GDAL_OF_UPDATE, nullptr, nullptr, nullptr));
    GNMGenericNetwork *poNetwork2 = dynamic_cast<GNMGenericNetwork *>(poDS2);

导入图层与自动拓扑
====================

.. code-block:: c++

    // 打开要导入的 OGR 数据源
    GDALDataset *poSrcDS = static_cast<GDALDataset *>(
        GDALOpenEx("roads.shp", GDAL_OF_READONLY, nullptr, nullptr, nullptr));
    OGRLayer *poSrcLayer = poSrcDS->GetLayer(0);

    // 复制图层到网络
    poNetwork->CopyLayer(poSrcLayer, "roads");
    GDALClose(poSrcDS);

    // 导入第二个图层（点）
    GDALDataset *poSrcDS2 = static_cast<GDALDataset *>(
        GDALOpenEx("intersections.shp", GDAL_OF_READONLY, nullptr, nullptr, nullptr));
    poNetwork->CopyLayer(poSrcDS2->GetLayer(0), "intersections");
    GDALClose(poSrcDS2);

    // 自动构建拓扑：基于捕捉容差连接点和线
    char **papszLayers = CSLAddString(nullptr, "roads");
    papszLayers = CSLAddString(papszLayers, "intersections");
    poNetwork->ConnectPointsByLines(
        papszLayers,
        0.001,          // 捕捉容差（取决于坐标系单位）
        1.0,            // 正向代价
        1.0,            // 反向代价
        GNM_EDGE_DIR_BOTH  // 双向
    );
    CSLDestroy(papszLayers);

手动连接与断开
================

.. code-block:: c++

    // 手动连接两个要素（源 FID=100, 目标 FID=200, 无连接要素=-1）
    poNetwork->ConnectFeatures(
        100, 200,
        -1,             // 连接要素 FID（-1 表示虚拟连接）
        1.5,            // 正向代价
        2.0,            // 反向代价
        GNM_EDGE_DIR_BOTH
    );

    // 断开连接
    poNetwork->DisconnectFeatures(100, 200, -1);

    // 按 FID 断开所有相关连接
    poNetwork->DisconnectFeaturesWithId(100);

    // 修改连接属性
    poNetwork->ReconnectFeatures(100, 200, -1, 3.0, 3.0, GNM_EDGE_DIR_SRCTOTGT);

阻塞与解阻塞
================

.. code-block:: c++

    // 阻塞特定要素（模拟道路封闭）
    poNetwork->ChangeBlockState(100, true);

    // 解阻塞
    poNetwork->ChangeBlockState(100, false);

    // 解阻塞全部
    poNetwork->ChangeAllBlockState(false);

最短路径分析
================

.. code-block:: c++

    // Dijkstra 最短路径
    GNMGFID nStartFID = 100;
    GNMGFID nEndFID = 200;

    OGRLayer *poResult = poNetwork->GetPath(
        nStartFID, nEndFID,
        GATDijkstraShortestPath,
        nullptr          // 额外选项
    );

    if (poResult != nullptr) {
        printf("路径要素数: %d\n", (int)poResult->GetFeatureCount());

        OGRFeature *poFeat;
        poResult->ResetReading();
        while ((poFeat = poResult->GetNextFeature()) != nullptr) {
            // 结果字段: gnm_fid, ogrlayer, path_num, ftype
            printf("  FID=%lld, 图层=%s, 类型=%s\n",
                poFeat->GetFieldAsInteger64("gnm_fid"),
                poFeat->GetFieldAsString("ogrlayer"),
                poFeat->GetFieldAsString("ftype"));
            OGRFeature::DestroyFeature(poFeat);
        }

        // 释放结果图层
        poNetwork->ReleaseResultSet(poResult);
    }

K 最短路径
================

.. code-block:: c++

    // 查找 3 条最短路径
    const char *apszKOpts[] = { "num_paths=3", nullptr };
    OGRLayer *poResult = poNetwork->GetPath(
        nStartFID, nEndFID,
        GATKShortestPath,
        apszKOpts
    );

    if (poResult != nullptr) {
        // 结果包含多条路径，通过 path_num 字段区分
        OGRFeature *poFeat;
        poResult->ResetReading();
        while ((poFeat = poResult->GetNextFeature()) != nullptr) {
            int nPathNum = poFeat->GetFieldAsInteger("path_num");
            GNMGFID nGFID = poFeat->GetFieldAsInteger64("gnm_fid");
            const char *pszType = poFeat->GetFieldAsString("ftype");
            printf("路径 %d: FID=%lld, 类型=%s\n", nPathNum, nGFID, pszType);
            OGRFeature::DestroyFeature(poFeat);
        }
        poNetwork->ReleaseResultSet(poResult);
    }

连通分量（资源分配）
====================

.. code-block:: c++

    // 设置发射器要素
    poNetwork->ChangeBlockState(100, false);  // 确保未阻塞

    const char *apszResOpts[] = { "emitter=100", nullptr };
    OGRLayer *poResult = poNetwork->GetPath(
        100,                    // 起始 FID（发射器）
        -1,                     // 结束 FID（连通分量不需要）
        GATConnectedComponents,
        apszResOpts
    );

    if (poResult != nullptr) {
        printf("可达要素数: %d\n", (int)poResult->GetFeatureCount());
        // 遍历结果...
        poNetwork->ReleaseResultSet(poResult);
    }

清理
================

.. code-block:: c++

    // 关闭网络（GDALClose 会自动释放）
    GDALClose(poNetwork);

.. _gnm-c-api:

******************
C 编程接口
******************

头文件： ``gnm/gnm_api.h``

网络创建与打开
==============

.. code-block:: c

    #include "gdal.h"
    #include "gnm_api.h"

    int main()
    {
        GDALAllRegister();
        GNMRegisterAll();

        /* 创建网络 */
        GDALDriverH hDriver = GDALGetDriverByName("GNMFile");
        if (!hDriver) return 1;

        const char *apszCreateOpts[] = { "FORMAT=ESRI_Shapefile", NULL };
        GDALDatasetH hDS = GDALCreate(hDriver, "my_network",
            0, 0, 0, GDT_Unknown, (char **)apszCreateOpts);

        GNMGenericNetworkH hNet = GNMCastToGenericNetwork(hDS);
        if (!hNet) return 1;

        /* ... 使用网络 ... */

        GDALClose(hDS);
        return 0;
    }

导入图层与自动拓扑
====================

.. code-block:: c

    /* 导入图层 */
    GDALDatasetH hSrcDS = GDALOpenEx("roads.shp", GDAL_OF_READONLY,
        NULL, NULL, NULL);
    OGRLayerH hSrcLayer = GDALDatasetGetLayer(hSrcDS, 0);

    /* 复制图层到网络 */
    GDALDatasetCopyLayer(hDS, hSrcLayer, "roads");
    GDALClose(hSrcDS);

    /* 自动拓扑构建 */
    char *papszLayers[] = { "roads", "intersections", NULL };
    GNMConnectPointsByLines(hNet, papszLayers, 0.001, 1.0, 1.0,
        GNM_EDGE_DIR_BOTH);

手动连接
================

.. code-block:: c

    /* 连接两个要素 */
    GNMConnectFeatures(hNet,
        100, 200,       /* 源 FID, 目标 FID */
        -1,             /* 连接要素 FID（-1=虚拟） */
        1.5, 2.0,       /* 正向代价, 反向代价 */
        GNM_EDGE_DIR_BOTH);

    /* 断开连接 */
    GNMDisconnectFeatures(hNet, 100, 200, -1);

    /* 修改连接 */
    GNMReconnectFeatures(hNet, 100, 200, -1, 3.0, 3.0,
        GNM_EDGE_DIR_SRCTOTGT);

阻塞要素
================

.. code-block:: c

    /* 阻塞 */
    GNMChangeBlockState(hNet, 100, 1);

    /* 解阻塞 */
    GNMChangeBlockState(hNet, 100, 0);

    /* 解阻塞全部 */
    GNMChangeAllBlockState(hNet, 0);

最短路径
================

.. code-block:: c

    /* Dijkstra 最短路径 */
    OGRLayerH hResult = GNMGetPath(hNet, 100, 200,
        GATDijkstraShortestPath, NULL);

    if (hResult) {
        int nCount = OGR_L_GetFeatureCount(hResult, 1);
        printf("路径要素数: %d\n", nCount);

        OGRFeatureH hFeat;
        OGR_L_ResetReading(hResult);
        while ((hFeat = OGR_L_GetNextFeature(hResult)) != NULL) {
            printf("  FID=%lld, 图层=%s, 类型=%s\n",
                OGR_F_GetFieldAsInteger64(hFeat, OGR_F_GetFieldIndex(hFeat, "gnm_fid")),
                OGR_F_GetFieldAsString(hFeat, OGR_F_GetFieldIndex(hFeat, "ogrlayer")),
                OGR_F_GetFieldAsString(hFeat, OGR_F_GetFieldIndex(hFeat, "ftype")));
            OGR_F_Destroy(hFeat);
        }

        GDALDatasetReleaseResultSet(hDS, hResult);
    }

K 最短路径
================

.. code-block:: c

    /* 查找 3 条最短路径 */
    const char *apszKOpts[] = { "num_paths=3", NULL };
    OGRLayerH hResult = GNMGetPath(hNet, 100, 200,
        GATKShortestPath, apszKOpts);

    if (hResult) {
        /* 遍历结果，通过 path_num 字段区分不同路径 */
        OGRFeatureH hFeat;
        OGR_L_ResetReading(hResult);
        while ((hFeat = OGR_L_GetNextFeature(hResult)) != NULL) {
            int nPath = OGR_F_GetFieldAsInteger(hFeat,
                OGR_F_GetFieldIndex(hFeat, "path_num"));
            printf("路径 %d\n", nPath);
            OGR_F_Destroy(hFeat);
        }
        GDALDatasetReleaseResultSet(hDS, hResult);
    }

连通规则
================

.. code-block:: c

    /* 添加规则 */
    GNMCreateRule(hNet, "ALLOW CONNECTS ANY");

    /* 获取规则列表 */
    char **papszRules = GNMGetRules(hNet);
    for (int i = 0; papszRules && papszRules[i]; i++) {
        printf("规则: %s\n", papszRules[i]);
    }
    CSLDestroy(papszRules);

    /* 删除规则 */
    GNMDeleteRule(hNet, "ALLOW CONNECTS ANY");

.. _gnm-notes:

******************
注意事项
******************

.. warning::

    - GNM 是 GDAL 2.1 引入的较老子系统，API 稳定但文档较少
    - 网络创建后，不应在网络目录外修改要素图层，否则可能导致图结构不一致
    - ``ConnectPointsByLines()`` 的捕捉容差取决于坐标系单位（地理坐标系用度，投影坐标系用米）
    - 路径结果图层是内存图层，用完需调用 ``ReleaseResultSet()`` 释放
    - ``GetPath()`` 的第三个参数 ``nEndFID`` 在 ``GATConnectedComponents`` 模式下无意义，可传 -1

.. note::

    - GNM 驱动需要在编译 GDAL 时启用（默认启用）
    - Python 绑定通过 ``osgeo.gnm`` 模块访问，API 与 C++ 类似
    - ``GNM_FILE`` 和 ``GNM_DB`` 是两种存储驱动的名称，分别对应文件系统和数据库存储
