.. highlight:: rst
.. _gdal3gnm:

########################
GDAL 地理网络分析（GNM）
########################

假设你有一个城市的道路网络 shapefile，需要回答这样的问题：从 A 点到 B 点，哪条路最近？如果某条路封了，有没有替代路线？某个水源通过管网能覆盖多少户居民？这些问题的本质都是 **网络分析** —— 在一个由节点（交叉口）和边（路段）组成的图（Graph）上，做最短路径、替代路径、连通性分析等计算。

GDAL 从 2.1 版本起内置了地理网络模型（Geographic Network Model, GNM）子系统，把 OGR 矢量要素拓扑化为图结构，并内置了 Dijkstra、K 最短路径、连通分量等经典算法。你不需要自己写图论代码，也不需要安装额外的网络分析库 —— GDAL 本身就够了。

本章从一个完整的实战场景出发，先用命令行工具快速体验，再用 C/C++ 代码逐步深入，最后介绍连通规则、进阶算法和注意事项。

.. contents:: 目录
   :depth: 2
   :local:

.. _gnm-scenario:

**************************************
场景引入：从道路 shapefile 到最短路径
**************************************

我们用一个最典型的场景贯穿全章：**城市道路网络分析**。

你手头有两个 shapefile：

- ``roads.shp`` — 道路线图层，每条记录是一段道路（线要素）
- ``intersections.shp`` — 交叉口点图层，每条记录是一个路口（点要素）

目标是：

1. 把这两个图层导入 GNM 网络
2. 让 GNM 自动根据空间位置，把"路的端点"和"最近的交叉口"连起来（自动拓扑）
3. 计算两个交叉口之间的最短路径
4. 模拟某条路段施工封闭，重新计算替代路径
5. 分析整个网络的连通性（哪些区域互相可达）

下面先用命令行工具完成整个流程，再用代码重写。

.. _gnm-cli-quick:

*******************
快速上手：CLI 工具
*******************

GNM 提供两个命令行工具：

- ``gnmmanage`` — 网络管理（创建、导入、连接、规则、阻塞）
- ``gnmanalyse`` — 网络分析（最短路径、K 最短路径、连通分量）

不需要写任何代码，就能完成完整的网络分析流程。

.. _gnm-cli-create:

第一步：创建网络
================

首先创建一个基于文件系统的网络。GNM 会在指定目录下生成网络元数据和图结构文件：

.. code-block:: bash

    gnmmanage create -ds city_network/ -f "ESRI Shapefile" \
        -dsco FORMAT=ESRI_Shapefile

这行命令做了什么？它调用 GNMFile 驱动，在 ``city_network/`` 目录下初始化一个空的地理网络。此时目录里还没有任何要素图层，只是一个空的"容器"。

如果想用数据库存储（适合多用户协作或大数据量），可以换成 SpatiaLite：

.. code-block:: bash

    gnmmanage create -ds network.sqlite -f SQLite

.. _gnm-cli-import:

第二步：导入图层
================

把道路和交叉口图层导入网络：

.. code-block:: bash

    gnmmanage import -ds city_network/ -l roads roads.shp
    gnmmanage import -ds city_network/ -l intersections intersections.shp

``-l`` 参数指定图层在网络中的名称。导入后，GNM 会为每个要素分配一个全局唯一的 FID（GNMGFID），这个 FID 和原始 shapefile 里的 FID 不一定相同 —— 它是网络内部的标识符。

.. note::

    导入是复制操作，不是引用。原始 shapefile 不受影响，修改原始文件不会同步到网络中。

.. _gnm-cli-connect:

第三步：自动拓扑连接
====================

这是最关键的一步。导入的 roads 和 intersections 目前是两个独立的图层，GNM 不知道它们之间有什么关系。需要"连"起来：

.. code-block:: bash

    gnmmanage autoconnect -ds city_network/ -tolerance 0.001

``autoconnect`` 会扫描所有已导入的图层，找到空间上接近的点和线要素，自动建立连接（Edge）。``-tolerance 0.001`` 是捕捉容差 —— 在这个距离范围内的点和线端点会被视为"重合"并建立连接。

.. warning::

    捕捉容差的单位取决于坐标系！如果 roads.shp 用的是 WGS 84（EPSG:4326），单位是度，0.001 度大约是 111 米。如果用的是投影坐标系（如 UTM），单位是米，0.001 米就太小了。实际使用时要根据数据的坐标系来选择合适的容差值。

连接建立后，GNM 会在内存中维护一个图结构：交叉口是顶点（Vertex），道路段是边（Edge），每条边带有方向和代价（默认代价为 1.0）。

.. _gnm-cli-analyse:

第四步：网络分析
================

现在可以做分析了。假设你想知道 FID 为 1 的交叉口到 FID 为 50 的交叉口之间的最短路径：

.. code-block:: bash

    gnmanalyse dijkstra -ds city_network/ 1 50 \
        -f GeoJSON -ds shortest_path.geojson

这条命令执行 Dijkstra 最短路径算法，把结果输出为 GeoJSON 文件。结果文件里每条记录代表路径上的一个要素（顶点或边），包含以下字段：

- ``gnm_fid`` — 要素在网络中的全局 FID
- ``ogrlayer`` — 所属图层名（roads 或 intersections）
- ``ftype`` — 类型：``VERTEX``（顶点）或 ``EDGE``（边）
- ``path_num`` — 路径编号（K 最短路径时用于区分不同路径）

.. note::

    这里的 FID 1 和 50 是 GNM 内部的全局 FID，不是原始 shapefile 里的 FID。可以用 ``gnmmanage info`` 查看网络信息，确认要素的全局 FID。

**查看替代路线**

如果想知道有没有其他路线（比如最短路太堵，想绕路），用 K 最短路径：

.. code-block:: bash

    gnmanalyse kpaths -ds city_network/ 1 50 3 \
        -f GeoJSON -ds k_paths.geojson

最后的 ``3`` 表示找 3 条最短路径。结果中 ``path_num`` 字段标记了每条路径属于第几条。

**查看连通性**

如果想知道从某个交叉口出发，能到达哪些交叉口（比如管网覆盖分析），用连通分量：

.. code-block:: bash

    gnmanalyse resource -ds city_network/ \
        -f GeoJSON -ds connected.geojson

.. _gnm-cli-manage:

更多管理操作
================

**查看网络信息：**

.. code-block:: bash

    gnmmanage info -ds city_network/

**手动连接两个要素：**

有时候 autoconnect 不够精确，需要手动补连接：

.. code-block:: bash

    # 连接 FID 100 和 FID 200，代价 1.5，双向
    gnmmanage connect -ds city_network/ 100 200 -1 -cost 1.5 -inv_cost 2.0 -dir both

``-1`` 表示没有中间连接要素（虚拟连接）。

**阻塞/解阻塞要素（模拟施工封路）：**

.. code-block:: bash

    # 阻塞 FID 300（模拟道路封闭）
    gnmmanage change -ds city_network/ -bl 300

    # 解阻塞
    gnmmanage change -ds city_network/ -unbl 300

    # 解阻塞全部
    gnmmanage change -ds city_network/ -unblall

**断开连接：**

.. code-block:: bash

    gnmmanage disconnect -ds city_network/ 100 200

**添加连通规则：**

.. code-block:: bash

    gnmmanage rule -ds city_network/ "ALLOW CONNECTS ANY"

**删除网络：**

.. code-block:: bash

    gnmmanage delete -ds city_network/

.. _gnm-cpp-workflow:

********************
C++ 编程：完整工作流
********************

下面用 C++ 代码完整重演上面的 CLI 流程。代码是自包含的，可以直接编译运行。

头文件： ``gnm/gnm.h``

.. _gnm-cpp-create:

创建网络并导入图层
================

为什么需要 ``GNMRegisterAll()``？因为 GNM 的文件驱动（GNMFile）和数据库驱动（GNMDatabase）不是标准的 GDAL/OGR 驱动，需要单独注册。``GDALAllRegister()`` 只注册栅格和矢量驱动，不包含 GNM。

.. code-block:: c++

    #include "gnm.h"
    #include "ogrsf_frmts.h"
    #include <cstdio>

    int main()
    {
        // 第一步：注册所有驱动，包括 GNM
        GDALAllRegister();
        GNMRegisterAll();

        // 第二步：创建网络
        GDALDriver *poDriver = GetGDALDriverManager()->GetDriverByName("GNMFile");
        if (poDriver == nullptr)
        {
            printf("错误：GNMFile 驱动不可用，请检查 GDAL 编译配置\n");
            return 1;
        }

        const char *apszCreateOpts[] = {
            "FORMAT=ESRI_Shapefile",  // 指定要素图层的存储格式
            nullptr
        };
        GDALDataset *poDS = poDriver->Create(
            "city_network",     // 网络目录路径（会自动创建）
            0, 0, 0, GDT_Unknown,
            const_cast<char **>(apszCreateOpts));

        if (poDS == nullptr)
        {
            printf("创建网络失败: %s\n", CPLGetLastErrorMsg());
            return 1;
        }

        // 将 GDALDataset 转为 GNMGenericNetwork，这是 GNM 的核心接口
        GNMGenericNetwork *poNetwork = dynamic_cast<GNMGenericNetwork *>(poDS);
        if (poNetwork == nullptr)
        {
            printf("转换为 GNMGenericNetwork 失败\n");
            GDALClose(poDS);
            return 1;
        }

        printf("网络创建成功\n");

        // 第三步：导入图层
        // 打开道路 shapefile
        GDALDataset *poRoadsDS = static_cast<GDALDataset *>(
            GDALOpenEx("roads.shp", GDAL_OF_READONLY, nullptr, nullptr, nullptr));
        if (poRoadsDS == nullptr)
        {
            printf("打开 roads.shp 失败: %s\n", CPLGetLastErrorMsg());
            GDALClose(poDS);
            return 1;
        }

        // CopyLayer 会把要素复制到网络目录下，并分配全局 FID
        poNetwork->CopyLayer(poRoadsDS->GetLayer(0), "roads");
        GDALClose(poRoadsDS);
        printf("roads 图层导入完成\n");

        // 打开交叉口 shapefile
        GDALDataset *poInterDS = static_cast<GDALDataset *>(
            GDALOpenEx("intersections.shp", GDAL_OF_READONLY, nullptr, nullptr, nullptr));
        if (poInterDS == nullptr)
        {
            printf("打开 intersections.shp 失败: %s\n", CPLGetLastErrorMsg());
            GDALClose(poDS);
            return 1;
        }

        poNetwork->CopyLayer(poInterDS->GetLayer(0), "intersections");
        GDALClose(poInterDS);
        printf("intersections 图层导入完成\n");

.. _gnm-cpp-autoconnect:

自动拓扑连接
============

导入图层后，道路和交叉口之间还没有拓扑关系。``ConnectPointsByLines()`` 会扫描指定的图层，找到空间上接近的点和线端点，自动建立连接。

为什么代价（cost）很重要？代价决定了路径搜索时"哪条路更优"。默认代价 1.0 意味着所有路段等权。实际应用中，你可以用道路长度、通行时间、拥堵系数等作为代价，让最短路径算法给出更有意义的结果。

.. code-block:: c++

        // 第四步：自动构建拓扑
        char **papszLayers = CSLAddString(nullptr, "roads");
        papszLayers = CSLAddString(papszLayers, "intersections");

        poNetwork->ConnectPointsByLines(
            papszLayers,
            0.001,              // 捕捉容差（WGS 84 坐标系下约为 111 米）
            1.0,                // 正向代价（从源到目标）
            1.0,                // 反向代价（从目标到源）
            GNM_EDGE_DIR_BOTH   // 双向通行
        );
        CSLDestroy(papszLayers);
        printf("自动拓扑连接完成\n");

``ConnectPointsByLines()`` 的工作原理：

1. 遍历 papszLayers 中所有图层的要素
2. 对于点要素，检查是否有线要素的端点在捕捉容差范围内
3. 如果有，就在点和线之间建立一条连接（Edge）
4. 代价和方向由调用者指定

连接建立后，内存中的图结构大概是这样的：

- 交叉口 → 顶点（Vertex）
- 道路段 → 边（Edge），连接两个交叉口顶点
- 每条边有正向代价、反向代价、方向属性

.. _gnm-cpp-shortest-path:

最短路径分析
===================

拓扑建好后，就可以做最短路径分析了。``GetPath()`` 是 GNM 的核心分析函数，它在内存图上运行 Dijkstra 算法，返回一个结果图层。

结果图层为什么需要 ``ReleaseResultSet()``？因为 ``GetPath()`` 返回的是一个临时的内存图层（OGRLayer），它的生命周期由网络管理。如果你不释放它，就会内存泄漏。这和 GDAL 其他返回临时图层的 API（如 ``ExecuteSQL()``）是一样的设计模式。

.. code-block:: c++

        // 第五步：最短路径分析
        // 假设全局 FID=1 是起点，FID=50 是终点
        GNMGFID nStartFID = 1;
        GNMGFID nEndFID = 50;

        OGRLayer *poResult = poNetwork->GetPath(
            nStartFID, nEndFID,
            GATDijkstraShortestPath,    // 使用 Dijkstra 算法
            nullptr                     // 无额外选项
        );

        if (poResult == nullptr)
        {
            printf("路径计算失败: %s\n", CPLGetLastErrorMsg());
        }
        else
        {
            printf("路径要素数: %d\n", (int)poResult->GetFeatureCount());

            // 遍历路径上的每个要素
            OGRFeature *poFeat;
            poResult->ResetReading();
            while ((poFeat = poResult->GetNextFeature()) != nullptr)
            {
                // 结果要素包含四个字段：
                //   gnm_fid  — 全局 FID
                //   ogrlayer — 所属图层名
                //   path_num — 路径编号（Dijkstra 永远是 0）
                //   ftype    — VERTEX 或 EDGE
                printf("  FID=%lld, 图层=%s, 类型=%s\n",
                    poFeat->GetFieldAsInteger64("gnm_fid"),
                    poFeat->GetFieldAsString("ogrlayer"),
                    poFeat->GetFieldAsString("ftype"));
                OGRFeature::DestroyFeature(poFeat);
            }

            // 释放结果图层，防止内存泄漏
            poNetwork->ReleaseResultSet(poResult);
        }

如何把路径结果保存为 shapefile？可以用 OGR 的复制机制，或者直接用 ``gnmanalyse`` 命令行工具输出。在代码中，你可以遍历结果图层，用 OGR 写入新的数据源。

.. _gnm-cpp-kpaths:

K 最短路径：找替代路线
============================

Dijkstra 只返回一条最短路径。如果你需要多条备选路线（比如最短路堵了，想看看第二、第三选择），用 Yen's K 最短路径算法：

.. code-block:: c++

        // 第六步：K 最短路径（找 3 条最短路径）
        const char *apszKOpts[] = { "num_paths=3", nullptr };
        OGRLayer *poKResult = poNetwork->GetPath(
            nStartFID, nEndFID,
            GATKShortestPath,           // 使用 Yen's K 最短路径算法
            apszKOpts
        );

        if (poKResult != nullptr)
        {
            printf("K 最短路径结果要素数: %d\n",
                (int)poKResult->GetFeatureCount());

            OGRFeature *poFeat;
            poKResult->ResetReading();
            while ((poFeat = poKResult->GetNextFeature()) != nullptr)
            {
                // path_num 字段标记了这条要素属于第几条路径
                int nPathNum = poFeat->GetFieldAsInteger("path_num");
                GNMGFID nGFID = poFeat->GetFieldAsInteger64("gnm_fid");
                const char *pszType = poFeat->GetFieldAsString("ftype");
                printf("  路径 %d: FID=%lld, 类型=%s\n", nPathNum, nGFID, pszType);
                OGRFeature::DestroyFeature(poFeat);
            }
            poNetwork->ReleaseResultSet(poKResult);
        }

.. note::

    K 最短路径的结果中，path_num 从 0 开始。path_num=0 是最短路径（和 Dijkstra 结果一致），path_num=1 是次短路径，以此类推。注意 K 最短路径算法的计算量比 Dijkstra 大很多，对于大型网络，设置过大的 K 值可能导致性能问题。

.. _gnm-cpp-blocking:

阻塞要素：模拟道路封闭
============================

实际的道路网络分析中，经常需要模拟"某条路不通"的场景。GNM 的阻塞机制可以临时禁用某些要素，让路径算法绕过它们。

.. code-block:: c++

        // 第七步：模拟道路封闭
        // 假设 FID=300 的路段正在施工
        poNetwork->ChangeBlockState(300, true);  // true = 阻塞
        printf("已阻塞 FID=300 的要素\n");

        // 重新计算最短路径（算法会自动绕过被阻塞的要素）
        OGRLayer *poNewResult = poNetwork->GetPath(
            nStartFID, nEndFID,
            GATDijkstraShortestPath, nullptr);

        if (poNewResult != nullptr)
        {
            printf("绕行路径要素数: %d\n", (int)poNewResult->GetFeatureCount());
            // 遍历结果...
            poNetwork->ReleaseResultSet(poNewResult);
        }
        else
        {
            printf("无法找到绕行路径（网络可能不连通）\n");
        }

        // 施工结束，解除阻塞
        poNetwork->ChangeBlockState(300, false);  // false = 解阻塞

        // 或者一次性解阻塞全部要素
        poNetwork->ChangeAllBlockState(false);

阻塞不仅适用于边（道路段），也适用于顶点（交叉口）。被阻塞的顶点意味着"这个路口不能通行"，所有与之相连的边都会被跳过。

.. _gnm-cpp-components:

连通分量：网络覆盖分析
=========================

连通分量分析回答的是："从某个要素出发，能到达哪些要素？" 这在管网分析中特别有用 —— 比如一个水源能覆盖多少户居民。

.. code-block:: c++

        // 第八步：连通分量分析
        // 从 FID=1 出发，找出所有可达要素
        const char *apszResOpts[] = { "emitter=1", nullptr };
        OGRLayer *poCompResult = poNetwork->GetPath(
            1,      // 起始 FID（发射器）
            -1,     // 结束 FID（连通分量不需要，传 -1）
            GATConnectedComponents,
            apszResOpts
        );

        if (poCompResult != nullptr)
        {
            printf("从 FID=1 可达的要素数: %d\n",
                (int)poCompResult->GetFeatureCount());

            OGRFeature *poFeat;
            poCompResult->ResetReading();
            while ((poFeat = poCompResult->GetNextFeature()) != nullptr)
            {
                printf("  FID=%lld, 图层=%s, 类型=%s\n",
                    poFeat->GetFieldAsInteger64("gnm_fid"),
                    poFeat->GetFieldAsString("ogrlayer"),
                    poFeat->GetFieldAsString("ftype"));
                OGRFeature::DestroyFeature(poFeat);
            }
            poNetwork->ReleaseResultSet(poCompResult);
        }

.. note::

    连通分量分析使用 BFS（广度优先搜索）算法，时间复杂度为 O(V+E)。``emitter`` 参数指定发射器的全局 FID，即搜索的起点。如果不指定 ``emitter``，行为取决于 GNM 的实现版本，建议总是显式指定。

.. _gnm-cpp-cleanup:

清理资源
==============

.. code-block:: c++

        // 第九步：关闭网络
        // GDALClose 会自动释放 GNM 内部的图结构和内存
        GDALClose(poDS);
        printf("网络已关闭\n");

        return 0;
    }

.. _gnm-c-workflow:

**********************************
C 编程：完整工作流
**********************************

如果你使用 C 语言，GNM 提供了对应的 C API。头文件为 ``gnm/gnm_api.h``。

C API 和 C++ API 是一一对应的，函数名去掉了类名前缀，改为 ``GNM`` 前缀。返回句柄类型为 ``GNMGenericNetworkH``（本质是 ``void*``）。

.. _gnm-c-create:

创建网络与导入图层
================

.. code-block:: c

    #include "gdal.h"
    #include "gnm_api.h"
    #include <stdio.h>

    int main()
    {
        /* 注册所有驱动 */
        GDALAllRegister();
        GNMRegisterAll();

        /* 创建基于文件的网络 */
        GDALDriverH hDriver = GDALGetDriverByName("GNMFile");
        if (!hDriver)
        {
            printf("GNMFile 驱动不可用\n");
            return 1;
        }

        const char *apszCreateOpts[] = { "FORMAT=ESRI_Shapefile", NULL };
        GDALDatasetH hDS = GDALCreate(hDriver, "city_network",
            0, 0, 0, GDT_Unknown, (char **)apszCreateOpts);

        if (!hDS)
        {
            printf("创建网络失败: %s\n", CPLGetLastErrorMsg());
            return 1;
        }

        /* 转换为 GNM 网络句柄 */
        GNMGenericNetworkH hNet = GNMCastToGenericNetwork(hDS);
        if (!hNet)
        {
            printf("转换为 GNM 网络失败\n");
            GDALClose(hDS);
            return 1;
        }

        /* 导入道路图层 */
        GDALDatasetH hRoadsDS = GDALOpenEx("roads.shp",
            GDAL_OF_READONLY, NULL, NULL, NULL);
        if (hRoadsDS)
        {
            OGRLayerH hRoadsLayer = GDALDatasetGetLayer(hRoadsDS, 0);
            GDALDatasetCopyLayer(hDS, hRoadsLayer, "roads");
            GDALClose(hRoadsDS);
            printf("roads 图层导入完成\n");
        }

        /* 导入交叉口图层 */
        GDALDatasetH hInterDS = GDALOpenEx("intersections.shp",
            GDAL_OF_READONLY, NULL, NULL, NULL);
        if (hInterDS)
        {
            OGRLayerH hInterLayer = GDALDatasetGetLayer(hInterDS, 0);
            GDALDatasetCopyLayer(hDS, hInterLayer, "intersections");
            GDALClose(hInterDS);
            printf("intersections 图层导入完成\n");
        }

.. _gnm-c-connect:

自动拓扑与手动连接
=========================

.. code-block:: c

        /* 自动拓扑连接 */
        char *papszLayers[] = { "roads", "intersections", NULL };
        GNMConnectPointsByLines(hNet, papszLayers, 0.001, 1.0, 1.0,
            GNM_EDGE_DIR_BOTH);
        printf("自动拓扑连接完成\n");

        /* 手动连接（当 autoconnect 不够精确时） */
        /* 连接 FID=100 和 FID=200，无中间要素，代价 1.5，双向 */
        GNMConnectFeatures(hNet,
            100, 200,       /* 源 FID, 目标 FID */
            -1,             /* 连接要素 FID（-1 = 虚拟连接） */
            1.5, 2.0,       /* 正向代价, 反向代价 */
            GNM_EDGE_DIR_BOTH);

        /* 修改连接属性（比如改为单向通行） */
        GNMReconnectFeatures(hNet, 100, 200, -1, 3.0, 3.0,
            GNM_EDGE_DIR_SRCTOTGT);

        /* 断开连接 */
        GNMDisconnectFeatures(hNet, 100, 200, -1);

.. _gnm-c-analyse:

网络分析
=============

.. code-block:: c

        /* Dijkstra 最短路径 */
        OGRLayerH hResult = GNMGetPath(hNet, 1, 50,
            GATDijkstraShortestPath, NULL);

        if (hResult)
        {
            int nCount = OGR_L_GetFeatureCount(hResult, 1);
            printf("路径要素数: %d\n", nCount);

            OGRFeatureH hFeat;
            OGR_L_ResetReading(hResult);
            while ((hFeat = OGR_L_GetNextFeature(hResult)) != NULL)
            {
                int idxFID = OGR_F_GetFieldIndex(hFeat, "gnm_fid");
                int idxLayer = OGR_F_GetFieldIndex(hFeat, "ogrlayer");
                int idxType = OGR_F_GetFieldIndex(hFeat, "ftype");

                printf("  FID=%lld, 图层=%s, 类型=%s\n",
                    OGR_F_GetFieldAsInteger64(hFeat, idxFID),
                    OGR_F_GetFieldAsString(hFeat, idxLayer),
                    OGR_F_GetFieldAsString(hFeat, idxType));
                OGR_F_Destroy(hFeat);
            }

            GDALDatasetReleaseResultSet(hDS, hResult);
        }
        else
        {
            printf("路径计算失败: %s\n", CPLGetLastErrorMsg());
        }

        /* K 最短路径（找 3 条路径） */
        const char *apszKOpts[] = { "num_paths=3", NULL };
        OGRLayerH hKResult = GNMGetPath(hNet, 1, 50,
            GATKShortestPath, apszKOpts);

        if (hKResult)
        {
            OGRFeatureH hFeat;
            OGR_L_ResetReading(hKResult);
            while ((hFeat = OGR_L_GetNextFeature(hKResult)) != NULL)
            {
                int nPath = OGR_F_GetFieldAsInteger(hFeat,
                    OGR_F_GetFieldIndex(hFeat, "path_num"));
                int nGFID = (int)OGR_F_GetFieldAsInteger64(hFeat,
                    OGR_F_GetFieldIndex(hFeat, "gnm_fid"));
                printf("路径 %d: FID=%d\n", nPath, nGFID);
                OGR_F_Destroy(hFeat);
            }
            GDALDatasetReleaseResultSet(hDS, hKResult);
        }

        /* 连通分量分析 */
        const char *apszResOpts[] = { "emitter=1", NULL };
        OGRLayerH hCompResult = GNMGetPath(hNet, 1, -1,
            GATConnectedComponents, apszResOpts);

        if (hCompResult)
        {
            printf("可达要素数: %d\n",
                (int)OGR_L_GetFeatureCount(hCompResult, 1));
            /* 遍历结果同上... */
            GDALDatasetReleaseResultSet(hDS, hCompResult);
        }

.. _gnm-c-block:

阻塞与清理
================

.. code-block:: c

        /* 阻塞 FID=300（模拟道路封闭） */
        GNMChangeBlockState(hNet, 300, 1);

        /* 解阻塞 */
        GNMChangeBlockState(hNet, 300, 0);

        /* 解阻塞全部 */
        GNMChangeAllBlockState(hNet, 0);

        /* 关闭网络 */
        GDALClose(hDS);
        printf("网络已关闭\n");

        return 0;
    }

.. _gnm-connectivity-rules:

********************************
连通规则：精细控制连接行为
********************************

默认情况下（``"ALLOW CONNECTS ANY"``），GNM 允许任意图层之间的要素建立连接。但在实际项目中，你可能需要更精细的控制 —— 比如：

- 水管只能和水管、阀门连接，不能和燃气管连接
- 道路只能通过交叉口连接，不能直接和建筑物连接

GNM 的规则系统就是为了解决这类问题。规则在 ``ConnectFeatures()`` 和 ``ConnectPointsByLines()`` 时自动检查 —— 如果连接违反了规则，操作会被拒绝。

.. _gnm-rules-syntax:

规则语法
========

规则是一个字符串，格式如下：

.. code-block:: text

    ACTION CONNECTS layer1 [WITH layer2] [VIA layer3]

其中：

- ``ACTION`` — ``ALLOW`` 或 ``DENY``
- ``layer1``, ``layer2`` — 图层名称
- ``VIA layer3`` — 指定中间连接图层（可选）

**示例：**

.. code-block:: text

    ALLOW CONNECTS ANY                         # 允许所有连接（默认规则）
    DENY CONNECTS ANY                          # 禁止所有连接
    DENY CONNECTS water_pipes WITH gas_pipes   # 水管不能直接连燃气管
    ALLOW CONNECTS wells WITH pipes VIA junctions  # 井只能通过接头连接管道

规则的检查顺序是：**后添加的规则优先级更高**。如果你先添加了 ``ALLOW CONNECTS ANY``，然后添加 ``DENY CONNECTS water_pipes WITH gas_pipes``，那么水管和燃气管之间的连接会被拒绝，其他连接仍然允许。

.. _gnm-rules-cpp:

C++ 中使用规则
=====================

.. code-block:: c++

    // 在创建网络后、导入图层前设置规则

    // 先添加默认规则
    poNetwork->CreateRule("ALLOW CONNECTS ANY");

    // 再添加限制规则（优先级更高）
    poNetwork->CreateRule("DENY CONNECTS water_pipes WITH gas_pipes");
    poNetwork->CreateRule("DENY CONNECTS water_pipes WITH electric_cables");

    // 允许通过接头连接
    poNetwork->CreateRule("ALLOW CONNECTS wells WITH pipes VIA junctions");

    // 查看当前所有规则
    char **papszRules = poNetwork->GetRules();
    for (int i = 0; papszRules && papszRules[i]; i++)
    {
        printf("规则 %d: %s\n", i, papszRules[i]);
    }
    CSLDestroy(papszRules);

    // 删除某条规则
    poNetwork->DeleteRule("DENY CONNECTS water_pipes WITH gas_pipes");

.. _gnm-rules-c:

C 中使用规则
===================

.. code-block:: c

    /* 添加规则 */
    GNMCreateRule(hNet, "ALLOW CONNECTS ANY");
    GNMCreateRule(hNet, "DENY CONNECTS water_pipes WITH gas_pipes");

    /* 获取规则列表 */
    char **papszRules = GNMGetRules(hNet);
    for (int i = 0; papszRules && papszRules[i]; i++)
    {
        printf("规则: %s\n", papszRules[i]);
    }
    CSLDestroy(papszRules);

    /* 删除规则 */
    GNMDeleteRule(hNet, "DENY CONNECTS water_pipes WITH gas_pipes");

.. _gnm-rules-practical:

实际应用案例
=====================

**案例 1：管网隔离**

城市地下管网中，水管、燃气管、电缆各自独立，但可能共用管沟。用规则确保它们不会被错误连接：

.. code-block:: bash

    gnmmanage rule -ds pipe_network/ "DENY CONNECTS water_pipes WITH gas_pipes"
    gnmmanage rule -ds pipe_network/ "DENY CONNECTS water_pipes WITH electric_cables"
    gnmmanage rule -ds pipe_network/ "DENY CONNECTS gas_pipes WITH electric_cables"

**案例 2：多模式交通网络**

公交线路只能通过公交站连接，地铁线路只能通过地铁站连接，但公交和地铁可以通过换乘站连接：

.. code-block:: bash

    gnmmanage rule -ds transit_network/ "ALLOW CONNECTS bus_routes WITH bus_stops VIA bus_routes"
    gnmmanage rule -ds transit_network/ "ALLOW CONNECTS metro_lines WITH metro_stations VIA metro_lines"
    gnmmanage rule -ds transit_network/ "ALLOW CONNECTS bus_routes WITH metro_lines VIA transfer_stations"

.. _gnm-advanced:

***************
进阶用法
***************

.. _gnm-advanced-direction:

单向道路与方向控制
=======================

很多城市道路是单行道。GNM 的边方向常量可以精确控制通行方向：

- ``GNM_EDGE_DIR_BOTH`` (0) — 双向通行
- ``GNM_EDGE_DIR_SRCTOTGT`` (1) — 只能从源到目标
- ``GNM_EDGE_DIR_TGTTOSRC`` (2) — 只能从目标到源

.. code-block:: c++

    // 手动添加一条单向连接（只能从 FID=100 到 FID=200）
    poNetwork->ConnectFeatures(
        100, 200, -1,
        1.0, 1.0,
        GNM_EDGE_DIR_SRCTOTGT  // 单向：源到目标
    );

    // 修改已有连接的方向
    poNetwork->ReconnectFeatures(
        100, 200, -1,
        1.0, 1.0,
        GNM_EDGE_DIR_TGTTOSRC  // 改为反向
    );

.. _gnm-advanced-cost:

自定义代价：不只是距离
==============================

默认代价为 1.0，意味着每条路段权重相同。实际应用中，代价可以是：

- **路段长度** — 最短物理距离
- **通行时间** — 考虑限速和拥堵
- **综合权重** — 距离 + 路况 + 收费等因素的加权组合

.. code-block:: c++

    // 假设我们用道路长度作为代价
    // 需要在导入图层后，手动计算每条道路的长度，然后建立连接时设置代价

    // 示例：FID=100 到 FID=200 之间的道路长 2.5 公里
    poNetwork->ConnectFeatures(
        100, 200, -1,
        2.5,    // 正向代价（长度）
        2.5,    // 反向代价（长度，双向道路相等）
        GNM_EDGE_DIR_BOTH
    );

    // 如果是单向道路，反向代价可以设为极大值或方向设为单向
    poNetwork->ConnectFeatures(
        300, 400, -1,
        1.8,        // 正向代价
        999999.0,   // 反向代价（实际上不可通行）
        GNM_EDGE_DIR_SRCTOTGT
    );

.. note::

    ``ConnectPointsByLines()`` 自动建立的连接，代价由函数参数统一指定。如果需要每条路段不同的代价，需要在自动连接后，用 ``ReconnectFeatures()`` 逐条修改，或者放弃自动连接，全部手动建立。

.. _gnm-advanced-open-existing:

打开已有网络
====================

网络创建后会持久化到磁盘。下次使用时直接打开即可，不需要重新创建和导入：

.. code-block:: c++

    // 以读写模式打开已有网络
    GDALDataset *poDS = static_cast<GDALDataset *>(
        GDALOpenEx("city_network", GDAL_OF_UPDATE, nullptr, nullptr, nullptr));
    GNMGenericNetwork *poNetwork = dynamic_cast<GNMGenericNetwork *>(poDS);

    if (poNetwork != nullptr)
    {
        // 网络已打开，可以直接做分析
        OGRLayer *poResult = poNetwork->GetPath(1, 50,
            GATDijkstraShortestPath, nullptr);
        // ...
        poNetwork->ReleaseResultSet(poResult);
        GDALClose(poDS);
    }

.. code-block:: c

    /* C 语言版本 */
    GDALDatasetH hDS = GDALOpenEx("city_network",
        GDAL_OF_UPDATE, NULL, NULL, NULL);
    GNMGenericNetworkH hNet = GNMCastToGenericNetwork(hDS);

    if (hNet)
    {
        OGRLayerH hResult = GNMGetPath(hNet, 1, 50,
            GATDijkstraShortestPath, NULL);
        /* ... */
        GDALDatasetReleaseResultSet(hDS, hResult);
        GDALClose(hDS);
    }

.. _gnm-advanced-database:

基于数据库的网络
=======================

对于大型网络或多用户场景，可以用数据库后端：

.. code-block:: c++

    // 使用 SpatiaLite 存储
    GDALDriver *poDriver = GetGDALDriverManager()->GetDriverByName("GNMDatabase");
    const char *apszOpts[] = { nullptr };
    GDALDataset *poDS = poDriver->Create(
        "network.sqlite", 0, 0, 0, GDT_Unknown, const_cast<char **>(apszOpts));

.. code-block:: bash

    # CLI 创建数据库网络
    gnmmanage create -ds network.sqlite -f SQLite

.. _gnm-notes:

**********************
注意事项与技巧
**********************

.. _gnm-notes-coord:

坐标系与捕捉容差
=====================

``ConnectPointsByLines()`` 的捕捉容差（tolerance）的单位取决于数据的坐标系：

- **地理坐标系**（如 EPSG:4326）— 单位是度。0.001 度约等于 111 米，0.0001 度约等于 11 米
- **投影坐标系**（如 UTM）— 单位是米。0.001 米太小，通常用 1-50 米

选错容差会导致两种问题：

- 容差太小：本该连接的要素没有连接，网络不完整
- 容差太大：不该连接的要素被错误连接，路径分析结果不正确

**建议：** 在 autoconnect 之前，先用 QGIS 或 ogrinfo 检查数据的坐标系和要素的空间分布，选择合适的容差。

.. _gnm-notes-fid:

全局 FID vs 原始 FID
==============================

GNM 为每个导入的要素分配一个全局 FID（GNMGFID，64 位整数），这个 FID 在整个网络中唯一，但和原始 shapefile 的 FID 没有固定对应关系。

如何确认某个要素的全局 FID？

.. code-block:: bash

    # 用 gnmmanage info 查看网络信息
    gnmmanage info -ds city_network/

在代码中，可以通过遍历图层来查找：

.. code-block:: c++

    // 遍历 roads 图层，找到原始 FID=42 的要素对应的全局 FID
    OGRLayer *poLayer = poNetwork->GetLayerByName("roads");
    poLayer->ResetReading();
    OGRFeature *poFeat;
    while ((poFeat = poLayer->GetNextFeature()) != nullptr)
    {
        // GetFID() 返回的是全局 FID
        GNMGFID nGFID = poFeat->GetFID();
        printf("全局 FID=%lld\n", nGFID);
        OGRFeature::DestroyFeature(poFeat);
    }

.. _gnm-notes-lifecycle:

结果图层的生命周期
=======================

``GetPath()`` 返回的 OGRLayer 是临时的内存图层，使用完后必须调用 ``ReleaseResultSet()`` 释放。这和 ``GDALDataset::ExecuteSQL()`` 返回的临时图层是同样的设计模式。

忘记释放会导致内存泄漏。在 C 语言中，用 ``GDALDatasetReleaseResultSet(hDS, hResult)`` 释放。

.. _gnm-notes-limitations:

已知限制
=============

- GNM 是 GDAL 2.1 引入的子系统，API 稳定但社区使用不多，文档相对较少
- ``ConnectPointsByLines()`` 只检查点和线端点的空间接近性，不检查线和线之间的拓扑关系（如交叉）
- 网络创建后，不应在网络目录外直接修改要素图层文件，否则可能导致图结构和要素数据不一致
- ``GetPath()`` 的第三个参数 ``nEndFID`` 在 ``GATConnectedComponents`` 模式下无意义，传 -1 即可
- 阻塞状态存储在网络中，关闭并重新打开网络后阻塞状态仍然有效

.. _gnm-notes-tips:

实用技巧
=============

1. **先用 CLI 验证，再写代码。** ``gnmmanage`` 和 ``gnmanalyse`` 可以快速验证网络是否正确构建，避免在代码中反复调试。

2. **用 GeoJSON 作为中间格式。** ``gnmanalyse`` 输出 GeoJSON 后，可以直接在 QGIS 中可视化检查路径结果。

3. **代价函数的设计决定了分析质量。** 不要盲目使用默认代价 1.0。根据业务场景选择合适的代价（长度、时间、费用等）。

4. **大型网络考虑用数据库后端。** 文件系统后端（GNMFile）适合小型网络和原型验证。对于包含数十万条道路的城市网络，建议用 SpatiaLite 或 PostgreSQL。

5. **Python 绑定。** GDAL 的 Python 绑定 ``osgeo.gnm`` 模块提供了与 C++ 类似的 API，适合快速原型和脚本化工作流。

6. **编译要求。** GNM 驱动默认启用，但如果 GDAL 编译时禁用了（``-DGNM_ENABLED=OFF``），则无法使用。运行 ``gdalinfo --formats`` 检查是否包含 GNM 支持。
