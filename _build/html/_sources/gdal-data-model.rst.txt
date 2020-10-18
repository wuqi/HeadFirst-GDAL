.. highlight:: rst
.. _gdaldatamodel:

################
GDAL数据模型
################
本章主要介绍GDAL的数据模型，通过上一章，我们大致了解数据集和波段的意义，这章中，我们来看GDAL中这些概念的实现。本章基本参考 `GDAL_Datamodel <http://www.gdal.org/gdal_datamodel.html>`_ 为了方便理解，没有完全按照原文翻译，有部分删改。

.. _dataset:

******************
数据集   
******************
上章介绍了，简单的说，一个数据集可以看作为一幅影像。它包括了元数据、投影信息、波段等等，GDAL中用 ``GDALDataset`` 类来表示数据集。数据集除了内部的波段，还包括以下内容：

.. _coordinatesystem:

投影系统
======================
GDAL中，使用的是 `WKT <http://www.geoapi.org/3.0/javadoc/org/opengis/referencing/doc-files/WKT.html>`_ 串来表示投影，具体的表示内容可以参考链接，下面用例子简单的介绍一下，#后面表示注释::

    PROJCS["WGS 84 / UTM zone 52N",                   #投影名称
        GEOGCS["WGS 84",                              #地理坐标系统名
            DATUM["WGS_1984",                         #水平基准面
                SPHEROID["WGS 84",6378137,298.257223  #椭球体名称、长半轴、反偏率
                    AUTHORITY["EPSG","7030"]],        #外部权威的空间参考系统的编码
                AUTHORITY["EPSG","6326"]],
            PRIMEM["Greenwich",0],                    #中央经线Greenwich，0度标准子午线
            UNIT["degree",0.0174532925199433],        #指定测量使用的单位。在地理坐标系下使用角度。
            AUTHORITY["EPSG","4326"]],
        PROJECTION["Transverse_Mercator"],            #投影方法，这里是通用墨卡托投影
        PARAMETER["latitude_of_origin",0],            #PARAMETER表示投影参数,0表示纬度起点为0度
        PARAMETER["central_meridian",129],            #投影带的中央经线是东经129度
        PARAMETER["scale_factor",0.9996],             #中央经线的长度比是0.9996
        PARAMETER["false_easting",500000],            #坐标纵轴向西移动500km
        PARAMETER["false_northing",0],                #横轴没有平移
        UNIT["metre",1,                               #指定测量使用的单位，指定米为测量单位。
            AUTHORITY["EPSG","9001"]],                #外部权威的空间参考系统的编码
        AUTHORITY["EPSG","32652"]]

这部分和地图投影关系很大，如果没有基础，请参考 `地图投影系列介绍 <http://blog.sina.com.cn/s/blog_7f6303470101fzcw.html>`_ 和 `地图投影简明笔记 <http://www.cnblogs.com/yiyezhai/p/3182955.html>`_ 。

使用 `GDALDataset::GetProjectionRef() <http://www.gdal.org/classGDALDataset.html#aa42537e1062ce254d124b29ff3ebe857>`_ 函数可以获取数据集的投影信息，GDAL中使用仿射变换可以将地理坐标和图像坐标进行转换，在下一节中我们将具体展示；使用 `GDALDataset::GetGCPProjection() <http://www.gdal.org/classGDALDataset.html#a35ea63c2f9ea12afd190ca2446d02ddb>`_ 可以获取GCP点的投影。返回均为WKT字符串，如果返回""，则表示该数据集没有投影或者投影没有被识别。

.. _affinegeotransform:

仿射地理变换
======================
GDAL数据集有两种方式表示栅格数据中像元位置（图像中某个点在影像中的行列号）和投影坐标系（不是经纬度，是投影到二维平面的地图坐标，二者可以通过地图投影进行相互转换）间的关系：仿射变换和GCP点。大部分数据都是用仿射变换描述的，本节中描述仿射变换。

仿射变换由六个参数实现， ``GDALDataset::GetGeoTransform()`` 可以获取仿射变换参数数组。将像元位置转换为投影坐标的公式如下：

.. code-block:: c++

    /*
    六个参数分别是：
        geos[0]  top left x 左上角x坐标
        geos[1]  w-e pixel resolution 东西方向像素分辨率
        geos[2]  rotation, 0 if image is "north up" 旋转角度，正北向上时为0
        geos[3]  top left y 左上角y坐标
        geos[4]  rotation, 0 if image is "north up" 旋转角度，正北向上时为0
        geos[5]  n-s pixel resolution 南北向像素分辨率
        x/y为图像的x/y坐标，geox/geoy为对应的投影坐标
    */
    geox = geos[0] + geos[1] * x + geos[2] * y;
    geoy = geos[3] + geos[4] * x + geos[5] * y

注意，上面所说的点/线坐标系是从左上角(0,0)点到右下角，也就是坐标轴从左到右增长，从上到下增长的坐标系（即影象的行列从左下角开始计算）。 点/线位置中心是(0.5,0.5)

.. _GCPs:

GCP点
======================
数据集可以由一系列控制点来定义空间参考坐标系。所有的GCP点共用 ``GDALDataset::GetGCPProjection()`` 获取的地理参考坐标系。GCP结构体在GDAL中表示如下：

.. code-block:: c++

    typedef struct
    {
        char    *pszId;        //ID，可以用字母+数字表示，不能重复
        char    *pszInfo;      //描述信息,一般为空
        double     dfGCPPixel;    //列号
        double    dfGCPLine;     //行号
        double    dfGCPX;        //投影x坐标
        double    dfGCPY;        //投影y坐标
        double    dfGCPZ;        //投影z坐标
    } GDAL_GCP;

**GDAL数据模型没有实现由GCPs产生坐标系的变化的机制** ，而是把具体的操作留给用户实现,一般采用多项式插值实现。

如果数据集有投影的话，会包含仿射变换或GCPS。两者都有的很少见， 如果两者都有，则无法用权威坐标系定义。

.. _metadata:

元数据
======================

GDAL中，元数据是以键值对呈现的辅助数据，可以使用 :ref:`gdalinfo` 获取影像元数据，结果如图所示：

.. figure:: img/keyvalue.png
   :alt: key-value Image
   :align: center

如上图中红色部分所示，在 ``MetaData`` 栏中，所有的数据都是以 **key=value** 的方式组织的。 ``key`` 中不能设置空格和一些特殊字符， ``value`` 为非空值的任意长度的内容。一个数据集的元数据一般不超过100kb，否则会导致性能严重下降。

一些数据格式支持用户自定义的基本元数据，一些数据格式会定义特殊的键。

相似的元数据可以组成一个域，如上面的红色部分和绿色部分，就属于不同的域。默认域没有名称，有些特殊用途的元数据有特殊的域。目前虽然不能列举出一个对象所需要 的所有域，但是程序可以测试任何我们已经知道确切含义的域。

下面介绍些常见的域。
   
.. _subdatasetsdomain:

子数据集域
-------------------
:ref:`multidatasets` 中一般都会有这个域，用于存储子数据集的信息,一般是一个类似下面的信息列表::

    Subdatasets:
        SUBDATASET_1_NAME=NETCDF:"example.nc":auditTrail
        SUBDATASET_1_DESC=[2x80] auditTrail (8-bit character)
        SUBDATASET_2_NAME=NETCDF:"example.nc":data
        SUBDATASET_2_DESC=[1x61x172] data (32-bit floating-point)
        SUBDATASET_3_NAME=NETCDF:"example.nc":lat
        SUBDATASET_3_DESC=[61x172] lat (32-bit floating-point)
        SUBDATASET_4_NAME=NETCDF:"example.nc":lon
        SUBDATASET_4_DESC=[61x172] lon (32-bit floating-point)

``NAME=`` 后面的内容是子数据集名称，用来打开数据集下的子数据集。``DESC=`` 后面跟的是描述。ADRG, ECRGTOC, GEORASTER, GTiff, HDF4, HDF5, netCDF, NITF, NTv2, OGDI, PDF, PostGISRaster, Rasterlite, RPFTOC, RS2, WCS, and WMS这些数据类型都有子数据集。

.. _imagestructuredomain:

图像结构域
-------------------
与影像格式相关的元数据信息将存在这个域中，就是上图中绿色部分。这部分元数据在转换格式的时候，一般不会自动拷贝到新数据中。其中有一些典型的条目，例如 ``COMPRESSION`` 表示压缩程度等。

.. _rpcdomain:

RPC域
-------------------
RPC元数据域描述有理多项式系数几何模型，这也是描述像元和地理参考位置之间变换信息用的。更详细的信息参见 `RPCs in GeoTIFF <http://geotiff.maptools.org/rpc_prop.html>`_ 文档。


.. _xmldomains:

xml: 域
-------------------
任何以 ``xml:`` 为前缀名的域都不是一个普通的键值对方式的元数据。它是一个存有xml字符串的单XML文档。

.. _rasterband:

******************
波段
******************
波段是真正存储数据的结构，GDAL中用 ``GDALRasterBand`` 类来表示单个波段。

波段有以下的性质：

* 宽和高：如果是全分辨率的波段，那长和宽与数据集中的定义是一样的。
* 数据类型：有Byte, UInt16, Int16, UInt32, Int32, Float32, Float64, and the complex types CInt16, CInt32, CFloat32, and CFloat64类型.
* 块大小：缓冲块的大小，tiff中一般是以行作为缓冲块
* 描述字串：可选
* nodata数值：可选
* 光栅名称：可选，这个特性可以用来指定一些比较重要的数据。 
* 波段的解释信息，枚举型：
    + GCI_Undefined: 默认值. 
    + GCI_GrayIndex: 灰度值波段 
    + GCI_PaletteIndex: 颜色表索引 
    + GCI_RedBand: 红色波段 
    + GCI_GreenBand: 绿色波段 
    + GCI_BlueBand: 蓝色波段 
    + GCI_AlphaBand: 透明通道
    + GCI_HueBand: 色调
    + GCI_SaturationBand: 饱和度
    + GCI_LightnessBand: 光强 
    + GCI_CyanBand: 青色波段 
    + GCI_MagentaBand: 品红波段 
    + GCI_YellowBand: 黄色波段 
    + GCI_BlackBand: t黑色波段.

 
.. _colortable:

******************
颜色表
******************
颜色表在GDAL中，由多个 ``GDALColorEntry`` 组成， ``GDALColorEntry`` 结构体描述如下：

.. code-block:: c++

    typedef struct
    {
        /- gray, red, cyan or hue -/
        short      c1;
        /- green, magenta, or lightness -/    
        short      c2;
        /- blue, yellow, or saturation -/
        short      c3;
        /- alpha or blackband -/
        short      c4;      
    } GDALColorEntry;

颜色表同时还对应一个调色板（ ``GDALPaletteInterp`` ）， ``GDALColorEntry`` 中的 ``c1/c2/c3/c4`` 的 值可以作为调色板索引得到真正的颜色值。

* GPI_Gray: c1作为灰度值。
* GPI_RGB: c1/c2/c3依次为Red/Green/Blue，c4对应alpha通道。
* GPI_CMYK: c1为cyan，c2为magenta，c3为yellow，c4为black。
* GPI_HLS: c1为hue，c2为lightness，c3为saturation。

用颜色表表示时，每个像素保存的只是像素颜色在颜色表的位置。每个颜色的索引从 0开始递增。这里并没有提供针对颜色表的缩放机制。

.. _overviews:

******************
金字塔层
******************
一个波段可以有零个或多个金字塔层，这个结构在构建影像金字塔的时候将用到。每个层都相当于独立的 ``GDALRasterBand``  。每层的行列数和原始的波段有所不同，但是覆盖的地理区域是相同的。

金字塔层是原始波段的降采样实现的，一般用于快速显示。

波段有 ``HasArbitraryOverviews`` 属性来判定在某些分辨率上是否有金字塔层。该属性可以应用于fft编码或者是网络传输切片等状况中。

