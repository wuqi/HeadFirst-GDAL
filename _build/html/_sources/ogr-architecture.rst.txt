.. highlight:: rst

.. _ograrchitecture:

############################
OGR矢量结构
############################
本章主要介绍OGR中矢量类型的架构,与 :ref:`gdaldatamodel` 一节类似,主要讲解OGR中矢量的整体结构，和其中涉及到的类，OGR设计是参照OpenGIS中的简单要素模型，因此本章中会简单介绍一些OpenGIS中的内容。本章节主要是对应 ``GDAL1.11`` 以下版本， ``GDAL2.0`` 以上版本中，将 :ref:`ogrdirver` 修改为 ``GDALDriver`` ，将 :ref:`datasource` 修改为 ``GDALDataset`` ，其他没有明显变化和修改。本章主要参考 `OGR Architecture <http://gdal.org/1.11/ogr/ogr_arch.html>`_ ， `OpenGIS的一些基本概念 <http://lab.osgeo.cn/7124.html>`_ 。

.. _vector:

************************************
OpenGIS中矢量要素简介
************************************
OpenGIS定义了一组基于数据的服务，而数据的基础是要素 ``Feature`` 。所谓要素，简单地说就是一个独立的对象，在地图中可能表现为一个多边形建筑物，在数据库中即一个独立的条目。要素具有两个必要的组成部分，几何信息和属性信息。

OpenGIS将几何信息分为点、边缘、面和几何集合四种：其中我们熟悉的线 ``Linestring`` 属于边缘的一个子类，而多边形 ``Polygon`` 是面的一个子类。也就是说OpenGIS定义的几何类型并不仅仅是我们常见的点、线、多边形三种，它提供了更复杂更详细的定义，增强了未来的可扩展性。另外，几何类型的设计中采用了组合模式 ``Composite`` ，将几何集合 ``GeometryCollection`` 也定义为一种几何类型，类似地，要素集合 ``FeatureCollection`` 也是一种要素。属性信息没有做太大的限制，可以在实际应用中结合具体的实现进行设置。

相同的几何类型、属性类型的组合成为要素类型 ``FeatureType`` ，要素类型相同的要素可以被存放在一个数据源中。而一个数据源只能拥有一个要素类型。因此，可以用要素类型来描述一组属性相似的要素。在面向对象的模型中，完全可以把要素类型理解为一个类，而要素则是类的实例。

实际理解中，可以将数据源理解为一个数据库，有多个图层，每个图层相当于一张数据表，每个数据表中有一个字段存储几何数据( OGR 1.11以后可以有多个几何数据字段)，其他字段中存储该数据的属性，如下图所示：

.. figure:: img/vector.png
   :alt: vector structS
   :align: center

.. _ogroverview:

******************
OGR类总览
******************
总体结构如下所示：

* ``Geometry`` 几何形状： ``OGRGeometry`` 类以及其子类封装了OpenGIS模型中矢量数据，提供了一些几何操作，以及转换为WKB、WKT的操作，每个几何形状中包含有空间参考系统（投影）。
* ``Spatial Reference`` 空间参考：``OGRSpatialReference`` 类封装了投影和水准面。
* ``Feature`` 要素： ``OGRFeature`` 类封装一个要素，包括其几何形状和其他属性信息
* ``Feature Class Definition`` 要素类定义: ``OGRFeatureDefn`` 类通常存储整个图层的属性的元数据
* ``Layer`` 图层： ``OGRLayer`` 表示一个图层，同类型要素的集合
* ``Data Source`` 数据源： ``OGRDataSource`` 表示存储矢量数据的文件或数据库对象
* ``Drivers`` 驱动： ``OGRSFDriver`` 每个驱动可以可以打开对应类型的数据，转换为数据源，通过  ``OGRSFDriverRegistrar`` 管理。

.. _geometry:

******************
几何形状
******************
几何形状类用于表示各种几何矢量。它的派生类包括了各种形状，有如下类型： ``OGRPoint`` , ``OGRLineString`` , ``OGRPolygon`` , ``OGRGeometryCollection`` , ``OGRMultiPolygon`` , ``OGRMultiPoint`` ,  ``OGRMultiLineString`` ，其派生关系如下：

.. figure:: img/classOGRGeometry.png
   :alt: vector structS
   :align: center

   **OGRGeometry 类**

:ref:`ograrchitecture` 中提到，OGR设计参照的是OpenGIS中的简单要素模型，下面就是OpenGIS规范中的设计：

.. figure:: img/opengis_geometry.*
   :alt: vector structS
   :align: center

   **OpenGIS Geometry 描述**

``OGRCurve`` 、 ``OGRSurface`` 两个中间抽象基类抽象出一些公用的功能，当前在简单要素模型中的一些中间基类没有出现在OGR中，以后可能会改变。

``OGRGeometryFactory`` 类用于将 ``WKT`` 或 ``WKB`` 串转化为几何形状。

``OGRGeometry`` 包含 ``OGRSpatialReference`` 的引用，用于定义几何形状的空间参考。

很多分析功能暂时没有在 ``OGRGeometry`` 中实现。

虽然理论上可以从现有的 ``OGRGeometry`` 类中派生出更多的其他几何形状，但是目前没有成熟方案。而且，可以使用 ``OGRGeometryFactory`` 创建特殊几何形状而不用修改它。

.. _spatialreference:

******************
空间参考
******************
``OGRSpatialReference`` 类用于实现OpenGIS空间参考系统定义。目前已经支持本地坐标系、地理坐标系、投影坐标系、垂直坐标系、地心坐标系、复合坐标系。

空间坐标系统是从OpenGIS定义的WKT投影格式中继承的。其中简单模式在``Simple Features specifications`` 中定义，复杂模式在 ``Coordinate Transformation specification`` 中定义， ``OGRSpatialReference`` 类实现是建立在 ``Coordinate Transformation specification`` 模式上，但是兼容早期的简单模式。

``OGRCoordinateTransformation`` 类使用 ``PROJ.4`` 库实现坐标转换。请参照坐标转换相关内容。

.. _feature:

******************
要素/要素类定义
******************
``OGRFeature`` 保存几何形状和其属性、ID、要素类标识符。从 OGR1.11开始，要素中可以有多个几何形状字段。

``OGRFeatureDefn`` 用于表示要素属性的类型、名称等元数据信息。一个 ``OGRFeatureDefn`` 通常用于表示一个图层的要素属性信息。具有相同要素类定义的要素通过引用计数的方式共享同一个要素类定义。

要素ID（FID）在图层中是唯一的，用于标识。单独的要素或者还未写入图层的要素可能有空FID，在OGR中FID用长整型表示，而其他模型中并不一定，例如GML中用字符串表示，而Oracle中行ID比长整型要大。

要素类中包含几何类型（使用 ``OGRFeatureDefn::GetGeomType()`` 获取 ``OGRwkbGeometryType`` ），如果是wkbUnknown类型，那么该要素中可以添加任意类型的几何形状。这意味着一个图层中可以有不同的几何形状。

从 ``OGR 1.11`` 开始，要素允许有多个几何字段，每个几何字段可以有不同的类型值和不同的空间参考系统。

``OGRFeatureDefn`` 包含要素类名称，通常作为图层名称。

.. _layer:

******************
图层
******************
``OGRLayer`` 表示数据源中的一层要素，同一图层的要素有相同的属性， ``OGRLayer`` 中也包含从数据源中读写要素的方法。图层主要用于在数据源中读写数据，通常代表一个文件，或者是空间表。

``OGRLayer`` 有顺序和随机读写方法， ``GetNextFeature`` 方法可以顺序读取所有要素，可以使用 ``SetSpatialFilter`` 等滤波器限定该方法获取的要素范围。 

当前 ``OGR`` 有一个缺陷：一个图层只能设置一个空间滤波器，以后将引入 ``OGRLayerView`` 或其他方式来改进。

``OGRLayer`` 和 ``OGRFeatureDefn`` 虽然是一一对应的关系，但是设计为两个类的原因如下：

    1. OGRFeatureDefn 和 OGRFeature 并不依赖于 OGRLayer ，因此两者可以独立存在于内存中。
    2. SF CORBA模型与SFCOM 和 SFSQL 不同，并没有图层与固定属性列的概念，在OGR中实现SFCORBA需要考虑这些。

``OGRLayer`` 是一个抽象类，每个驱动将单独实现， ``OGRLayers`` 通常都由 ``OGRDataSource`` 直接创建，而不能直接初始化或者删除。

.. _datasource:

******************
数据源
******************
``OGRDataSource`` 表示 ``OGRLayer`` 对象集合。通常代表单个文件、一系列文件、数据库或者网关。 ``OGRDataSource`` 通常拥有 ``OGRLayer`` 列表，可以返回它们的引用。

``OGRDataSource`` 是抽象基类，将有每个格式的驱动实现， ``OGRDataSource`` 对象通常由对应的 OGRSFDriver 实例化，删除 ``OGRDataSource`` 对象仅是关闭其对数据源的访问，而非物理删除数据源。

``OGRSFDriver`` 可以通过 ``OGRDataSource`` 的名称(通常为文件名)重新打开数据源。

``OGRDataSource`` 可以通过 ``ExecuteSQL`` 方法执行对数据源的特殊命令，例如用SQL进行查询，虽然一些数据源底层支持空间SQL操作，OGR也实现了对任意数据源的查询操作（SQL的子集）。

.. _ogrdirver:

******************
驱动
******************
``OGRSFDriver`` 用于将对应格式的文件对象实例化， ``OGRSFDriverRegistrar`` 用于注册各种驱动。每个格式有对应的派生类来实例化 ``OGRDataSource`` 和 ``OGRLayer`` 。

在应用程序启动时需要调用所需文件格式的驱动，这些函数实例化对应的驱动对象，通过 ``OGRSFDriverRegistrar`` 注册。当数据源打开时，``OGRSFDriverRegistrar`` 将遍历使用所有的驱动，直到成功返回 ``OGRDataSource`` 对象。
