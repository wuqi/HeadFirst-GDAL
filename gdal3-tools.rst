.. highlight:: rst
.. _gdal3-tools:

################################################################
GDAL 3.x 新统一命令行工具（gdal CLI）
################################################################

GDAL 3.11 引入了全新的统一命令行入口 ``gdal`` ，将过去分散的数十个独立工具
（如 ``gdalinfo`` 、 ``gdal_translate`` 、 ``gdalwarp`` 、 ``ogr2ogr`` 等）整合为一个
层次化的子命令体系。新工具采用 ``gdal <领域> <操作>`` 的语法，参数统一使用
``--long-name`` 长名和 ``-s`` 短名，并支持 Pipeline 链式处理。

.. note::

   旧工具在 GDAL 3.x 中仍然可用，但官方建议逐步迁移到新的 ``gdal`` CLI。
   详见下方 :ref:`gdal3-tools-migration` 部分的映射表。

.. contents:: 目录
   :depth: 2
   :local:

************************************
顶层入口概述
************************************

``gdal`` 是所有新 CLI 命令的顶层入口。主要顶层命令包括：

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - 命令
     - 说明
   * - ``gdal info``
     - 获取数据集信息（自动判断栅格/矢量）
   * - ``gdal convert``
     - 格式转换（自动判断栅格/矢量）
   * - ``gdal pipeline``
     - 混合 Pipeline（可串联栅格与矢量步骤）
   * - ``gdal external``
     - 调用外部工具（如 ``gdal external gdal_contour`` ）
   * - ``gdal raster``
     - 所有栅格子命令入口
   * - ``gdal vector``
     - 所有矢量子命令入口
   * - ``gdal mdim``
     - 多维数据子命令入口
   * - ``gdal dataset``
     - 数据集管理子命令入口
   * - ``gdal vsi``
     - 虚拟文件系统（VSI）子命令入口
   * - ``gdal driver``
     - 驱动特定操作子命令入口

语法要点
========

- **长参数**： ``--long-name VALUE`` 或 ``--long-name=VALUE``
- **短参数**： ``-s VALUE``
- **布尔标志**： ``--overwrite`` （无需赋值，出现即启用）
- **多值参数**：部分参数支持逗号分隔，如 ``--bbox=2,49,3,50`` ；
  也可重复使用，如 ``--co COMPRESS=LZW --co TILED=YES``
- **列出可选值**：在参数名后追加 ``=?`` 可列出所有可选值，例如 ``--resampling-method=?``
- **位置参数**：通常按 输入(s) 在前、输出 最后的顺序
- **命名参数** 可放在位置参数之前或之后

示例::

    # 查看版本
    gdal --version

    # 查看所有命令和子命令的 JSON 用法
    gdal --json-usage

    # 列出支持的格式
    gdal --formats

    # 列出 resampling-method 的可选值
    gdal raster reproject --resampling-method=?

************************************
栅格命令（gdal raster \*）
************************************

``gdal raster`` 是所有栅格操作的入口，子命令如下：

.. list-table::
   :header-rows: 1
   :widths: 22 78

   * - 子命令
     - 说明
   * - ``info``
     - 显示栅格数据集信息（元数据、波段、统计等）
   * - ``convert``
     - 格式转换，如 GeoTIFF 转 COG、转 GeoPackage 栅格等
   * - ``reproject``
     - 重投影到目标坐标系
   * - ``update``
     - 用源数据更新已有数据集（如镶嵌到已有文件）
   * - ``resize``
     - 调整分辨率或尺寸
   * - ``select``
     - 选择/重排波段
   * - ``set-type``
     - 修改像素数据类型（如 Float32 转 Byte）
   * - ``scale``
     - 应用缩放和偏移（值域映射）
   * - ``unscale``
     - 移除缩放和偏移，还原原始物理值
   * - ``edit``
     - 编辑元数据（CRS、bbox、nodata、metadata 等）
   * - ``create``
     - 创建空的栅格数据集
   * - ``clip``
     - 按边界框或矢量裁剪栅格
   * - ``mosaic``
     - 将多个栅格拼接为镶嵌体（类似 gdalbuildvrt）
   * - ``stack``
     - 将多个单波段栅格合并为多波段数据集
   * - ``blend``
     - 多栅格加权混合
   * - ``calc``
     - 栅格代数运算（波段间数学表达式）
   * - ``contour``
     - 从栅格生成等值线矢量
   * - ``polygonize``
     - 栅格多边形化，生成矢量面
   * - ``sieve``
     - 消除小面积区域（筛滤）
   * - ``proximity``
     - 计算到指定像素值的近邻距离
   * - ``hillshade``
     - 山体阴影渲染（DEM 地形可视化）
   * - ``slope``
     - 坡度计算
   * - ``aspect``
     - 坡向计算
   * - ``tpi``
     - 地形位置指数（Topographic Position Index）
   * - ``tri``
     - 地形粗糙度指数（Terrain Ruggedness Index）
   * - ``roughness``
     - 地形起伏度计算
   * - ``viewshed``
     - 可视域分析
   * - ``fill-nodata``
     - 填充 NoData 区域（插值）
   * - ``color-map``
     - 应用颜色表或颜色映射文件
   * - ``rgb-to-palette``
     - RGB 影像转调色板索引影像
   * - ``nodata-to-alpha``
     - 将 NoData 像素转为 Alpha 波段
   * - ``clean-collar``
     - 清理边缘近黑色像素（nearblack 功能）
   * - ``reclassify``
     - 重分类（按规则重新赋值）
   * - ``pansharpen``
     - 全色锐化融合
   * - ``footprint``
     - 计算栅格有效覆盖范围（footprint）
   * - ``neighbors``
     - 邻域统计（均值、众数等）
   * - ``as_features``
     - 将栅格像素转为矢量点/面要素
   * - ``tile``
     - 生成瓦片（TMS/XYZ，类似 gdal2tiles）
   * - ``index``
     - 创建栅格索引图层（类似 gdaltindex）
   * - ``overview add``
     - 添加概要层（金字塔）
   * - ``overview delete``
     - 删除概要层
   * - ``overview refresh``
     - 刷新概要层
   * - ``compare``
     - 比较两个栅格数据集的差异
   * - ``pixel-info``
     - 查询指定坐标处的像素值
   * - ``materialize``
     - 将 Pipeline 中间结果物化到磁盘
   * - ``zonal-stats``
     - 分区统计（按矢量区域统计栅格值）

重点命令示例
============

convert -- 格式转换
--------------------

::

    # GeoTIFF 转 Cloud Optimized GeoTIFF
    gdal raster convert --of=COG in.tif out.tif

    # 带压缩选项
    gdal raster convert --of=COG --co=COMPRESS=DEFLATE in.tif out.tif

    # NetCDF 转 GeoTIFF
    gdal raster convert --of=GTiff in.nc out.tif

reproject -- 重投影
--------------------

::

    # 重投影到 WGS84
    gdal raster reproject --output-crs=EPSG:4326 in.tif out.tif

    # 指定重采样方法和分辨率
    gdal raster reproject --output-crs=EPSG:4326 --resampling=cubic \
        --resolution=60,60 in.tif out.tif

clip -- 裁剪
-------------

::

    # 按边界框裁剪
    gdal raster clip --bbox=2,49,3,50 in.tif out.tif

    # 按矢量文件裁剪
    gdal raster clip --like=mask.gpkg in.tif out.tif

mosaic -- 镶嵌
---------------

::

    # 将目录下所有 GeoTIFF 拼接为 COG
    gdal raster mosaic --of=COG src/*.tif out.tif

    # 创建 VRT 虚拟镶嵌
    gdal raster mosaic src/*.tif out.vrt

stack -- 合并波段
-----------------

::

    # 合并单波段为多波段
    gdal raster stack red.tif green.tif blue.tif rgb.tif

calc -- 栅格代数
-----------------

::

    # 计算 NDVI
    gdal raster calc --calc="(B4-B3)/(B4+B3)" --input=sentinel2.tif ndvi.tif

    # 或指定波段
    gdal raster calc --calc="A*B" --input=a.tif --input=b.tif result.tif

contour -- 等值线
-----------------

::

    # 从 DEM 生成等高线
    gdal raster contour --interval=10 dem.tif contour.shp

hillshade -- 山体阴影
---------------------

::

    gdal raster hillshade dem.tif hillshade.tif

polygonize -- 栅格矢量化
------------------------

::

    gdal raster polygonize classified.tif polygons.gpkg

overview -- 金字塔管理
----------------------

::

    # 添加概要层
    gdal raster overview add --resampling=average --levels=2,4,8,16 my.tif

    # 刷新概要层
    gdal raster overview refresh --resampling=average my.tif

info -- 查看信息
-----------------

::

    # 文本输出
    gdal raster info in.tif

    # JSON 输出
    gdal raster info --of=json in.tif

************************************
矢量命令（gdal vector \*）
************************************

``gdal vector`` 是所有矢量操作的入口，子命令如下：

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - 子命令
     - 说明
   * - ``info``
     - 显示矢量数据集信息（图层、字段、几何类型等）
   * - ``convert``
     - 格式转换，如 Shapefile 转 GeoPackage、转 GeoJSON 等
   * - ``reproject``
     - 重投影到目标坐标系
   * - ``clip``
     - 按边界框或矢量裁剪要素（几何被裁切）
   * - ``filter``
     - 按空间范围或属性条件筛选要素（保留完整几何）
   * - ``select``
     - 选择字段（列选择）
   * - ``create``
     - 创建空的矢量数据集/图层
   * - ``edit``
     - 编辑元数据（CRS、metadata 等）
   * - ``concat``
     - 合并多个矢量数据集（追加或分层堆叠）
   * - ``sql``
     - 执行 SQL 查询或语句
   * - ``rasterize``
     - 矢量栅格化
   * - ``grid``
     - 离散点插值生成栅格（类似 gdal_grid）
   * - ``buffer``
     - 缓冲区分析
   * - ``simplify``
     - 几何简化（Douglas-Peucker 等）
   * - ``simplify-coverage``
     - 覆盖面简化（保持拓扑无重叠/无缝隙）
   * - ``make-valid``
     - 修复无效几何
   * - ``explode-collections``
     - 拆分集合几何（Multi -> Single）
   * - ``combine``
     - 合并两个图层的要素（行合并）
   * - ``make-point``
     - 从 X/Y/Z/M 字段创建点几何
   * - ``dissolve``
     - 要素融合（按属性聚合几何）
   * - ``swap-xy``
     - 交换 X/Y 坐标顺序
   * - ``segmentize``
     - 线段化（加密节点使线段不超过指定长度）
   * - ``sort``
     - 按字段或几何排序要素
   * - ``partition``
     - 按空间分区拆分为多个图层
   * - ``set-field-type``
     - 修改字段数据类型
   * - ``set-geom-type``
     - 修改几何类型
   * - ``rename-layer``
     - 重命名图层
   * - ``export-schema``
     - 导出图层结构/Schema
   * - ``layer-algebra``
     - 图层代数运算（并集、交集、差集、对称差）
   * - ``index``
     - 创建空间索引
   * - ``check-coverage``
     - 检查要素覆盖完整性
   * - ``check-geometry``
     - 检查几何有效性
   * - ``clean-coverage``
     - 清理覆盖面（修复拓扑错误）
   * - ``concave-hull``
     - 生成凹包（Concave Hull）
   * - ``convex-hull``
     - 生成凸包（Convex Hull）
   * - ``update``
     - 更新已有数据集（追加或替换要素）

重点命令示例
============

convert -- 格式转换
--------------------

::

    # Shapefile 转 GeoPackage
    gdal vector convert in.shp out.gpkg

    # 指定输出格式
    gdal vector convert --format=OpenFileGDB in.shp out.gdb

reproject -- 重投影
--------------------

::

    gdal vector reproject --output-crs=EPSG:4326 in.shp out.gpkg

clip -- 裁剪
-------------

::

    # 按边界框裁剪
    gdal vector clip --bbox=2,49,3,50 in.gpkg out.gpkg

    # 按矢量文件裁剪
    gdal vector clip --like=clip_poly.gpkg in.gpkg out.gpkg

filter -- 筛选
---------------

::

    # 按边界框筛选（保留完整几何）
    gdal vector filter --bbox=2,49,3,50 in.gpkg out.gpkg

    # 按属性查询筛选
    gdal vector filter --where "population > 100000" cities.gpkg big_cities.gpkg

sql -- SQL 查询
----------------

::

    # 执行查询
    gdal vector sql --sql "SELECT * FROM countries WHERE pop > 1e6" world.gpkg

    # 就地更新
    gdal vector sql --update my.gpkg --sql "DELETE FROM countries WHERE pop > 1e6"

concat -- 合并数据集
--------------------

::

    # 追加模式（合并为单图层）
    gdal vector concat --mode=append part1.gpkg part2.gpkg merged.gpkg

    # 堆叠模式（每个文件一个图层）
    gdal vector concat --mode=stack *.shp merged.gpkg

buffer -- 缓冲区
-----------------

::

    gdal vector buffer --distance=100 points.gpkg buffered.gpkg

info -- 查看信息
-----------------

::

    # 文本输出
    gdal vector info poly.gpkg

    # JSON 输出
    gdal vector info --of=json poly.gpkg

************************************
多维命令（gdal mdim \*）
************************************

``gdal mdim`` 用于处理多维数据（如 NetCDF、HDF5、Zarr 等）。

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - 子命令
     - 说明
   * - ``info``
     - 显示多维数据集信息（维度、变量、属性等）
   * - ``convert``
     - 多维数据格式转换（如 NetCDF 转 Zarr）
   * - ``mosaic``
     - 多维数据镶嵌

示例::

    # 查看 NetCDF 信息
    gdal mdim info temperatures.nc

    # NetCDF 转 Zarr
    gdal mdim convert temperatures.nc temperatures.zarr

    # 列出支持多维的驱动
    gdal mdim --drivers

************************************
数据集管理（gdal dataset \*）
************************************

.. versionadded:: 3.12

``gdal dataset`` 用于数据集文件级别的管理操作。

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - 子命令
     - 说明
   * - ``identify``
     - 识别数据集类型及其对应驱动
   * - ``check``
     - 检查数据集读取是否存在问题
   * - ``copy``
     - 复制数据集文件
   * - ``rename``
     - 重命名数据集文件
   * - ``delete``
     - 删除数据集文件

示例::

    # 识别数据集驱动
    gdal dataset identify mystery.dat

    # 检查数据集完整性
    gdal dataset check my.tif

    # 复制数据集
    gdal dataset copy src.gpkg dst.gpkg

    # 删除数据集
    gdal dataset delete old_data.gpkg

************************************
VSI 命令（gdal vsi \*）
************************************

``gdal vsi`` 用于操作 GDAL 虚拟文件系统（ :ref:`virtual_file_systems` ）上的文件，
支持 ``/vsis3/`` 、 ``/vsicurl/`` 、 ``/vsipmtiles/`` 等。

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - 子命令
     - 说明
   * - ``list``
     - 列出虚拟目录内容
   * - ``copy``
     - 复制虚拟文件（支持递归）
   * - ``delete``
     - 删除虚拟文件
   * - ``move``
     - 移动/重命名虚拟文件
   * - ``sync``
     - 同步目录（类似 rsync）
   * - ``sozip``
     - SOZip（Seekable Optimized Zip）操作

示例::

    # 列出 S3 存储桶内容（递归、带详情）
    gdal vsi list -lR /vsis3/bucket

    # 远程 PMTiles 递归列表
    gdal vsi list -lR "/vsipmtiles//vsicurl/https://example.com/data.pmtiles"

    # 从 S3 复制文件到本地
    gdal vsi copy /vsis3/bucket/data.tif ./local_data.tif

    # 递归复制整个目录
    gdal vsi copy -r /vsis3/bucket/folder ./local_folder

    # 同步目录
    gdal vsi sync /vsis3/bucket/folder ./local_folder

************************************
Driver 命令（gdal driver \*）
************************************

``gdal driver`` 提供特定格式驱动的专用工具。

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - 子命令
     - 说明
   * - ``cog-validate``
     - 验证 Cloud Optimized GeoTIFF（COG）是否合规
   * - ``gpkg-validate``
     - 验证 GeoPackage 文件是否符合规范
   * - ``gpkg-repack``
     - 重新打包 GeoPackage（压缩整理，减小文件体积）
   * - ``gti-create``
     - 创建 GDAL Tile Index（GTI）文件
   * - ``openfilegdb-repack``
     - 重新打包 OpenFileGDB
   * - ``parquet-create-metadata-file``
     - 为 GeoParquet 创建元数据文件
   * - ``pdf-list-layers``
     - 列出 PDF 文件中的图层

示例::

    # 验证 COG
    gdal driver cog-validate my_cog.tif

    # 验证 GeoPackage
    gdal driver gpkg-validate my.gpkg

    # 重新打包 GeoPackage（减少碎片）
    gdal driver gpkg-repack my.gpkg

************************************
Pipeline 架构
************************************

Pipeline 是 GDAL 3.x CLI 的核心设计理念，允许将多个处理步骤串联执行。

三级 Pipeline
=============

GDAL 提供三个层级的 Pipeline：

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Pipeline
     - 说明
   * - ``gdal pipeline``
     - 混合 Pipeline，可串联栅格与矢量步骤（如栅格 -> 等值线 -> 缓冲区）
   * - ``gdal raster pipeline``
     - 栅格 Pipeline，步骤均为栅格输入/栅格输出
   * - ``gdal vector pipeline``
     - 矢量 Pipeline，步骤均为矢量输入/矢量输出

语法与 ! 分隔符
================

Pipeline 使用 ``!``（感叹号）分隔各步骤。第一个 ``!`` 可省略。::

    gdal pipeline ! read in.tif ! reproject --output-crs=EPSG:4326 ! write out.tif

等价于::

    gdal pipeline read in.tif ! reproject --output-crs=EPSG:4326 ! write out.tif

**规则**：

- **栅格 Pipeline** 首步必须是 ``read`` 、 ``calc`` 、 ``create`` 、 ``mosaic`` 或 ``stack`` ，
  末步必须是 ``write`` 、 ``info`` 或 ``tile``
- **矢量 Pipeline** 首步必须是 ``read`` 或 ``concat`` ，
  末步必须是 ``info`` 、 ``partition`` 或 ``write``
- 除首步和末步外，其他步骤可在 Pipeline 中多次出现

惰性求值
========

Pipeline 中的大多数步骤采用 **惰性求值** （on-demand evaluation）机制：
只有在需要最终输出时才会逐块/逐要素计算，中间结果不会物化到磁盘。
这使得 Pipeline 非常高效，尤其适合大数据量处理。

如需强制物化中间结果（例如多次使用同一中间数据集），可使用 ``materialize`` 步骤::

    gdal raster pipeline ! read in.tif ! reproject --output-crs=EPSG:4326 \
        ! materialize ! clip --bbox=2,49,3,50 ! write out.tif

GDALG 序列化（.gdalg.json）
============================

Pipeline 可以序列化为 ``.gdalg.json`` 文件，而非直接写入最终数据集。
该文件记录了完整的处理命令，可被 GDAL 作为虚拟数据集打开，在读取时才实际执行
Pipeline。概念类似于 VRT 虚拟文件，但实现方式不同。

示例::

    # 序列化为 GDALG 文件
    gdal vector pipeline ! read in.gpkg \
        ! reproject --output-crs=EPSG:32632 \
        ! write reprojected.gdalg.json --overwrite

    # 之后可像普通数据集一样使用
    gdal vector info reprojected.gdalg.json

生成的 ``reprojected.gdalg.json`` 内容如下::

    {
        "type": "gdal_streamed_alg",
        "command_line": "gdal vector pipeline ! read in.gpkg ! reproject --output-crs=EPSG:32632"
    }

Pipeline 代码示例
=================

混合 Pipeline -- 从栅格生成等值线并做缓冲区分析::

    gdal pipeline ! read dem.tif ! contour --interval=50 \
        ! buffer --distance=100 ! write contour_buffer.gpkg --overwrite

栅格 Pipeline -- 重投影 + 裁剪 + 转 COG::

    gdal raster pipeline ! read in.tif \
        ! reproject --output-crs=EPSG:4326 \
        ! clip --bbox=100,30,120,50 \
        ! write --of=COG out.tif --overwrite

矢量 Pipeline -- 筛选 + 投影 + 简化 + 输出::

    gdal vector pipeline ! read roads.shp \
        ! filter --where "type='motorway'" \
        ! reproject --output-crs=EPSG:3857 \
        ! simplify --tolerance=10 \
        ! write roads_web.gpkg --overwrite

多步 Pipeline（筛选 + 投影 + 裁剪）::

    gdal vector pipeline ! read in.gpkg \
        ! filter --bbox=2,49,3,50 \
        ! reproject --output-crs=EPSG:32631 \
        ! write out.gpkg

.. _gdal3-tools-migration:

************************************
旧工具到新工具映射表
************************************

下表列出常用旧工具与新 CLI 命令的对应关系：

栅格工具映射
============

.. list-table::
   :header-rows: 1
   :widths: 30 45 25

   * - 旧工具
     - 新命令
     - 说明
   * - ``gdalinfo``
     - ``gdal raster info``
     - 查看栅格信息
   * - ``gdal_translate``
     - ``gdal raster convert``
     - 格式转换
   * - ``gdalwarp`` （投影）
     - ``gdal raster reproject``
     - 重投影
   * - ``gdalwarp`` （更新）
     - ``gdal raster update``
     - 更新已有数据集
   * - ``gdalwarp`` （裁剪）
     - ``gdal raster clip``
     - 裁剪
   * - ``gdalbuildvrt``
     - ``gdal raster mosaic``
     - 镶嵌
   * - ``gdaladdo``
     - ``gdal raster overview add``
     - 添加金字塔
   * - ``gdaldem hillshade``
     - ``gdal raster hillshade``
     - 山体阴影
   * - ``gdaldem slope``
     - ``gdal raster slope``
     - 坡度
   * - ``gdaldem aspect``
     - ``gdal raster aspect``
     - 坡向
   * - ``gdaldem color-relief``
     - ``gdal raster color-map``
     - 颜色映射
   * - ``nearblack``
     - ``gdal raster clean-collar``
     - 清理边缘
   * - ``gdal_contour``
     - ``gdal raster contour``
     - 等值线
   * - ``gdal_polygonize``
     - ``gdal raster polygonize``
     - 多边形化
   * - ``gdal_sieve``
     - ``gdal raster sieve``
     - 筛滤
   * - ``gdal_proximity``
     - ``gdal raster proximity``
     - 近邻距离
   * - ``gdal_fillnodata``
     - ``gdal raster fill-nodata``
     - 填充 NoData
   * - ``gdal_pansharpen``
     - ``gdal raster pansharpen``
     - 全色锐化
   * - ``gdal2tiles``
     - ``gdal raster tile``
     - 生成瓦片
   * - ``gdaltindex``
     - ``gdal raster index``
     - 栅格索引
   * - ``gdalcompare``
     - ``gdal raster compare``
     - 比较栅格
   * - ``gdallocationinfo``
     - ``gdal raster pixel-info``
     - 像素查询
   * - ``gdal_edit.py``
     - ``gdal raster edit`` / ``gdal raster pipeline ... edit``
     - 编辑元数据

矢量工具映射
============

.. list-table::
   :header-rows: 1
   :widths: 30 45 25

   * - 旧工具
     - 新命令
     - 说明
   * - ``ogrinfo``
     - ``gdal vector info``
     - 查看矢量信息
   * - ``ogr2ogr`` （转换）
     - ``gdal vector convert``
     - 格式转换
   * - ``ogr2ogr`` （投影）
     - ``gdal vector reproject``
     - 重投影
   * - ``ogr2ogr`` （裁剪）
     - ``gdal vector clip``
     - 裁剪
   * - ``ogr2ogr`` （筛选）
     - ``gdal vector filter``
     - 空间/属性筛选
   * - ``ogr2ogr`` （SQL）
     - ``gdal vector sql``
     - SQL 查询
   * - ``ogrmerge.py``
     - ``gdal vector concat``
     - 合并数据集
   * - ``ogr2ogr`` （栅格化）
     - ``gdal vector rasterize``
     - 矢量栅格化
   * - ``gdal_grid``
     - ``gdal vector grid``
     - 离散点插值

其他工具映射
============

.. list-table::
   :header-rows: 1
   :widths: 30 45 25

   * - 旧工具
     - 新命令
     - 说明
   * - ``gdalmdiminfo``
     - ``gdal mdim info``
     - 多维信息
   * - ``gdalmdimtranslate``
     - ``gdal mdim convert``
     - 多维转换
   * - ``gdalmanage``
     - ``gdal dataset identify/delete/rename``
     - 数据集管理
   * - ``gdal_ls.py``
     - ``gdal vsi list``
     - VSI 目录列表
   * - ``gdal_cp.py``
     - ``gdal vsi copy``
     - VSI 文件复制
   * - ``gdal_rm.py``
     - ``gdal vsi delete``
     - VSI 文件删除
   * - ``gdal_srs_info``
     - ``gdal info`` (输出 CRS 信息)
     - CRS 查询

************************************
编程调用
************************************

GDAL 3.12+ 提供了统一的编程 API，可以在 C 、 C++ 和 Python 中调用新的 CLI 算法。

C API
=====

头文件： ``gdalalgorithm.h``

.. code-block:: c

    #include "gdal.h"
    #include "gdalalgorithm.h"
    #include <stdio.h>

    int main()
    {
        GDALAllRegister();

        /* 获取全局算法注册表 */
        GDALAlgorithmRegistryH hRegistry = GDALGetGlobalAlgorithmRegistry();

        /* 通过路径实例化算法 */
        const char *const papszAlgPath[] = { "raster", "reproject", NULL };
        GDALAlgorithmH hAlg = GDALAlgorithmRegistryInstantiateAlgFromPath(
            hRegistry, papszAlgPath);
        GDALAlgorithmRegistryRelease(hRegistry);

        if (!hAlg) {
            fprintf(stderr, "无法实例化算法\n");
            return 1;
        }

        /* 设置参数 */
        {
            GDALAlgorithmArgH hArg = GDALAlgorithmGetArg(hAlg, "input");
            GDALAlgorithmArgSetAsString(hArg, "byte.tif");
            GDALAlgorithmArgRelease(hArg);
        }
        {
            GDALAlgorithmArgH hArg = GDALAlgorithmGetArg(hAlg, "output-format");
            GDALAlgorithmArgSetAsString(hArg, "MEM");
            GDALAlgorithmArgRelease(hArg);
        }
        {
            GDALAlgorithmArgH hArg = GDALAlgorithmGetArg(hAlg, "dst-crs");
            GDALAlgorithmArgSetAsString(hArg, "EPSG:4326");
            GDALAlgorithmArgRelease(hArg);
        }

        /* 执行算法 */
        int ret = 0;
        if (GDALAlgorithmRun(hAlg, NULL, NULL)) {
            /* 获取输出数据集 */
            GDALAlgorithmArgH hArg = GDALAlgorithmGetArg(hAlg, "output");
            GDALArgDatasetValueH hVal = GDALAlgorithmArgGetAsDatasetValue(hArg);
            GDALDatasetH hDS = GDALArgDatasetValueGetDatasetRef(hVal);
            /* ... 使用 hDS ... */
            GDALAlgorithmArgRelease(hArg);
        } else {
            fprintf(stderr, "算法执行失败\n");
            ret = 1;
        }

        /* 清理 */
        GDALAlgorithmFinalize(hAlg);
        GDALAlgorithmRelease(hAlg);
        return ret;
    }

C++ API
=======

头文件： ``gdalalgorithm.h``

.. code-block:: cpp

    #include "gdal.h"
    #include "gdalalgorithm.h"
    #include "gdal_dataset.h"
    #include <iostream>

    int main()
    {
        GDALAllRegister();

        /* 获取单例并实例化算法 */
        auto& singleton = GDALGlobalAlgorithmRegistry::GetSingleton();
        GDALAlgorithm *algPtr = singleton.Instantiate("raster", "reproject");
        if (!algPtr) {
            std::cout << "无法实例化算法" << std::endl;
            return 1;
        }
        GDALAlgorithm& alg = *algPtr;

        /* 设置参数（使用 operator[] ） */
        alg["input"] = "byte.tif";
        alg["output-format"] = "MEM";
        alg["dst-crs"] = "EPSG:26711";
        alg["resampling"] = "cubic";
        alg["resolution"] = std::vector<double>{60, 60};

        /* 执行 */
        if (alg.Run()) {
            /* 获取输出数据集 */
            GDALDataset *poDS = alg["output"].Get<GDALArgDatasetValue>().GetDatasetRef();
            std::cout << "成功，宽=" << poDS->GetRasterXSize()
                      << " 高=" << poDS->GetRasterYSize() << std::endl;
            alg.Finalize();
        } else {
            std::cout << "算法执行失败" << std::endl;
            alg.Finalize();
            return 1;
        }

        delete algPtr;
        return 0;
    }

Pipeline 编程调用
=================

Pipeline 算法的编程调用与单个算法不同，需要通过 ``ParseCommandLineArguments()`` 传入步骤列表。
步骤之间用 ``!`` 分隔，语法与 CLI 一致。

C API（Pipeline）：

.. code-block:: c

    #include "gdal.h"
    #include "gdalalgorithm.h"

    int main()
    {
        GDALAllRegister();

        /* 实例化 pipeline 算法 */
        GDALAlgorithmRegistryH hRegistry = GDALGetGlobalAlgorithmRegistry();
        const char *const papszPath[] = { "pipeline", NULL };
        GDALAlgorithmH hPipeline = GDALAlgorithmRegistryInstantiateAlgFromPath(
            hRegistry, papszPath);
        GDALAlgorithmRegistryRelease(hRegistry);

        if (!hPipeline) return 1;

        /* 传入 Pipeline 步骤（与 CLI 参数一致） */
        const char *const papszArgs[] = {
            "read", "in.tif", "!",
            "reproject", "--output-crs=EPSG:4326", "!",
            "clip", "--bbox=100,30,120,50", "!",
            "write", "out.tif", NULL
        };

        if (GDALAlgorithmParseCommandLineArguments(hPipeline, papszArgs)) {
            GDALAlgorithmRun(hPipeline, NULL, NULL);
        }

        GDALAlgorithmFinalize(hPipeline);
        GDALAlgorithmRelease(hPipeline);
        return 0;
    }

C++ API（Pipeline）：

.. code-block:: cpp

    #include "gdal.h"
    #include "gdalalgorithm.h"
    #include <iostream>

    int main()
    {
        GDALAllRegister();

        /* 实例化 pipeline 算法 */
        auto pipeline = GDALGlobalAlgorithmRegistry::GetSingleton()
                            .Instantiate("pipeline");
        if (!pipeline) return 1;

        /* 传入 Pipeline 步骤 */
        bool ok = pipeline->ParseCommandLineArguments({
            "read", "in.tif", "!",
            "reproject", "--output-crs=EPSG:4326", "!",
            "clip", "--bbox=100,30,120,50", "!",
            "write", "out.tif"
        });

        if (ok) {
            if (pipeline->Run()) {
                std::cout << "Pipeline 执行成功" << std::endl;
            } else {
                std::cout << "Pipeline 执行失败" << std::endl;
            }
        }

        pipeline->Finalize();
        return 0;
    }

矢量 Pipeline 示例：

.. code-block:: cpp

    /* 矢量 Pipeline：读取 → 筛选 → 投影 → 简化 → 输出 */
    auto pipeline = GDALGlobalAlgorithmRegistry::GetSingleton()
                        .Instantiate("pipeline");
    pipeline->ParseCommandLineArguments({
        "read", "roads.shp", "!",
        "filter", "--where", "type='motorway'", "!",
        "reproject", "--output-crs=EPSG:3857", "!",
        "simplify", "--tolerance=10", "!",
        "write", "roads_web.gpkg"
    });
    pipeline->Run();
    pipeline->Finalize();

.. note::

    - Pipeline 步骤中的 ``!`` 分隔符是必须的，与 CLI 用法一致
    - 也可以使用 ``"pipeline"`` 子命令路径实例化，如 ``Instantiate("raster", "pipeline")``
      或 ``Instantiate("vector", "pipeline")`` 限定栅格或矢量 Pipeline
    - 进度回调可通过 ``Run()`` 的参数传入，与单个算法一致

Python API
==========

Python 中有两种调用方式： ``gdal.Run()`` 和 ``gdal.alg.*`` 模块。

方式一：gdal.Run()
------------------

.. code-block:: python

    from osgeo import gdal

    gdal.UseExceptions()

    # 转换格式
    gdal.Run("raster convert", input="in.tif", output="out.tif")

    # 重投影并获取输出数据集
    with gdal.Run("raster reproject",
                   input="byte.tif",
                   output_format="MEM",
                   dst_crs="EPSG:4326") as alg:
        ds = alg.Output()
        arr = ds.ReadAsArray()
        print(arr.shape)

    # 获取 JSON 输出
    alg = gdal.Run("raster info", input="byte.tif")
    info_dict = alg.Output()
    print(info_dict)

    # 也可以用列表形式传入路径
    gdal.Run(["raster", "convert"], input="in.tif", output="out.tif")

方式二：gdal.alg.* 模块（3.12+）
---------------------------------

``gdal.alg`` 模块提供了与 CLI 层次结构对应的子模块和函数：

.. code-block:: python

    from osgeo import gdal

    gdal.UseExceptions()

    # 格式转换
    gdal.alg.raster.convert(input="in.tif", output="out.tif")

    # 重投影
    gdal.alg.raster.reproject(
        input="byte.tif",
        output_format="MEM",
        dst_crs="EPSG:4326"
    )

    # 栅格信息
    info = gdal.alg.raster.info(input="byte.tif").Output()

    # 矢量转换
    gdal.alg.vector.convert(input="in.shp", output="out.gpkg")

    # 查看函数帮助
    help(gdal.alg.raster.convert)

    # 列出算法参数名
    gdal.Algorithm("raster", "convert").GetArgNames()

    # Pipeline 方式调用
    gdal.Run("raster pipeline",
             input="read in.tif ! reproject --output-crs=EPSG:4326 ! write out.tif")

.. note::

   Python API 中参数名使用下划线 ``_`` 替代命令行中的连字符 ``-`` 。
   例如命令行的 ``--output-crs`` 在 Python 中为 ``output_crs`` 。
