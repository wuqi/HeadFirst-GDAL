.. highlight:: rst
.. _GDALUtilities:

############################
GDAL工具集
############################

主要来自 `GDAL源码剖析（四）之命令行程序说明一 <http://blog.csdn.net/liminlu0314/article/details/6978589>`_ 和 `GDAL源码剖析（四）之命令行程序说明二 <http://blog.csdn.net/liminlu0314/article/details/7218543>`_ 。对部分翻译进行了少量修改和更新。

********************
通用选项
********************
所有GDAL工具都可以使用以下选项::

    --version
        返回GDAL版本.
    --license
        返回license信息.
    --formats
        返回所有支持的格式.
    --format [format]
        某种格式的详细信息.
    --optfile filename
        将一个文件中的内容作为工具参数列表.
    --config key value
        设置系统参数.
    --debug [on/off/value]
        设置debug级别.
    --pause
        在出发调试器的时候，等待用户输入
    --locale [locale]
        为调试设置本机环境(ie. en_US.UTF-8)
    --help-general
        显示通用选项.
        
实例::

    gdalinfo --help-general

.. _gdalinfo:

****************************************
gdalinfo文件信息工具
****************************************
``gdalinfo`` 主要用来显示栅格数据的所有信息,用法如下::

    gdalinfo [-mm] [-stats] [-hist] [-nogcp] [-nomd] 
             [-norat] [-noct] [-nofl] [-checksum] [-proj4] 
             [-mdd domain] [-sd subdataset] datasetname

参数说明:

.. program:: gdalinfo

.. cmdoption:: -mm

    强制计算栅格每个波段的最大最小值

.. cmdoption:: -stats

    显示栅格统计值,如果没有,则强制计算统计值(均值,最大最小值,标准差等)

.. cmdoption:: -approx_stats 

    显示栅格统计值,如果没有,则强制计算.但是是非精确计算,基于缩略图或者部分数据进行计算,如果不需要精确值或者希望快速返回,请使用此参数代替-stats

.. cmdoption:: -hist

    显示所有波段的直方图信息

.. cmdoption:: -nogcp

    禁止地面控制点显示,某些数据类型有上千的地面控制点,使用此选项可以禁止

.. cmdoption:: -nomd

    禁止元数据显示.有些数据集可能包括很多元数据

.. cmdoption:: -nrat

    禁止栅格属性表显示

.. cmdoption:: -noct

    禁止颜色表显示

.. cmdoption:: -checksum

    强制计算所有波段的校验值

.. cmdoption:: -mdd domain

    报告指定域的元数据

.. cmdoption:: -nofl

    只显示文件列表中第一个文件

.. cmdoption:: -sd subdataset

    如果输入数据集包含几个子数据,读取并显示指定的号码（从1开始）的子数据集。

.. cmdoption:: -proj4 (GDAL >= 1.9.0)

    将文件的坐标系用proj4格式的字符串显示


gdalinfo默认显示信息::

    文件类型
    文件大小(以像素表示). 
    坐标信息(用OGC WKT串表示). 
    与该文件关联的地理转换  
    左上右下脚点坐标 
    地面控制点 
    元数据 
    波段数据类型(byte int16...)
    Band color interpretations. 
    波段自动分块大小Band block size. 
    波段描述Band descriptions. 
    波段最大最小值Band min/max values (internally known and possibly computed). 
    波段校验值Band checksum (if computation asked). 
    波段的NODATA值Band NODATA value. 
    Band overview resolutions available. 
    波段长度单位Band unit type (i.e.. "meters" or "feet" for elevation bands). 
    波段伪彩色表Band pseudo-color tables.

实例::

    gdalinfo 1.tif 

.. _gdalwarptool:

****************************************
gdalwarp 图像纠正工具
****************************************
``gdalwarp`` 工具是一个图像镶嵌、重投影、和纠正的工具，用法如下::

    gdalwarp [-s_srs srs_def] [-t_srs srs_def] [-to "NAME=VALUE"]
        [-order n | -tps | -rpc | -geoloc] [-et err_threshold]
        [-refine_gcps tolerance [minimum_gcps]]
        [-te xmin ymin xmax ymax] [-tr xres yres] [-tap] [-ts width height]
        [-wo "NAME=VALUE"] [-ot Byte/Int16/...] [-wt Byte/Int16]
        [-srcnodata "value [value...]"] [-dstnodata "value [value...]"] -dstalpha
        [-r resampling_method] [-wm memory_in_mb] [-multi] [-q]
        [-cutline datasource] [-cl layer] [-cwhere expression]
        [-csql statement] [-cblend dist_in_pixels] [-crop_to_cutline]
        [-of format] [-co "NAME=VALUE"]* [-overwrite]
        [-nomd] [-cvmd meta_conflict_value] [-setci]
        srcfile* dstfile

参数说明：

.. program:: gdalwarp

.. cmdoption:: -s_srs srs def

    设置原始空间参考，坐标系统是可以使用函数OGRSpatialReference.SetFromUserInput()调用的就行，包括EPSG PCS，PROJ4或者后缀名为.prf的wkt文本文件。

.. cmdoption:: -t_srs srs_def

    设置目标空间参考，坐标系统是可以使用函数OGRSpatialReference.SetFromUserInput()调用的就行，包括EPSG PCS，PROJ4或者后缀名为.prf的wkt文本文件。 

.. cmdoption:: -to NAME=VALUE

    设置转换参数选项，具体选项参考函数GDALCreateGenImgProjTransformer2()支持的选项。 

.. cmdoption:: -order n

    多项式纠正次数（1到3），默认的多项式次数根据输入的GCP点个数自动计算。 

.. cmdoption:: -tps

    强制使用TPS（thin plate spline）纠正方法来纠正图像。 

.. cmdoption:: -rpc: 

    强制使用RPC参数纠正。 

.. cmdoption:: -geoloc

    强制使用Geolocation数组。（这个没用过，不太清楚）

.. cmdoption:: -et err_threshold

    指定变换的近似误差阈值，默认为0.125个像元大小（使用像元为单位）。 

.. cmdoption:: -refine_gcps tolerance minimum_gcps

    细化控制点，自动消除离群值。离群值将被淘汰，直到剩下的控制点为minimum_gcps时或检测不到异常值时。通过调整tolerance，去除离群的GCP。不只适用于多项式插值GCP细化。如果没有投影，tolerance以像素为单位，否则它是SRS单位。如果不设置minimum_gcps，根据多项式模型需要的最少控制点来确定（1次2个点，2次三个点，三次6个点）。

.. cmdoption:: -te xmin ymin xmax ymax

    设置输出文件的地理范围（在目标空间参考中）。

.. cmdoption:: -tr xres yres

    设置输出图像的分辨率。 

.. cmdoption:: -tap

    (GDAL >= 1.8.0) (target aligned pixels) align the coordinates of the extent of the output file to the values of the -tr, such that the aligned extent includes the minimum extent. 

.. cmdoption:: -ts width height

    设置输出文件的宽高。如果宽或者高有一个为0，那么将自动计算一个值，注意-ts和-tr不能同时使用。 

.. cmdoption:: -wo "NAME=VALUE"

    设置纠正选项。具体参考GDALWarpOptions::papszWarpOptions的帮助文档。 

.. cmdoption:: -ot type

    指定输出波段的数据类型。 

.. cmdoption:: -wt type

    计算的数据类型。 

.. cmdoption:: -r resampling_method

    重采样方式，主要有下面几种方式：

    * near: 最邻近采样方法（默认值，算法较快，但是质量较差）。
    * bilinear: 双线性内插采样。
    * cubic: 立方卷积采样。 
    * cubicspline: 立方样条采样。 
    * lanczos: Lanczos 窗口辛克采样。 
    * average: average resampling, computes the average of all non-NODATA contributing pixels. (GDAL >= 1.10.0) 
    * mode: mode resampling, selects the value which appears most often of all the sampled points. (GDAL >= 1.10.0) 

.. cmdoption:: -srcnodata value [value...]

    设置输入波段的Nodata值，可以为不同的波段指定不同的值。。如果有多个值，就需要把他们用双引号括起来，以保持在命令参数中作为单一参数输入。掩膜值不会在内插中处理。 

.. cmdoption:: -dstnodata value [value...]

    设置输出波段的Nodata值。 

.. cmdoption:: -dstalpha

    创建一个Alpha波段在输出文件中。 

.. cmdoption:: -wm memory_in_mb

    设置纠正API使用的内存大小，以MB为单位。 

.. cmdoption:: -multi

    是否使用多线程纠正图像，多线程用来分块处理，同时在读取和写入图像均使用多线程技术。 

.. cmdoption:: -q

    不在控制台输出提示信息。

.. cmdoption:: -of format

    输出文件格式，默认为GeoTiff。 

.. cmdoption:: -co "NAME=VALUE"

    指定创建图像选项，具体参考不同的格式说明。 

.. cmdoption:: -cutline datasource

    使用使用OGR支持的矢量数据进行裁切图像。 

.. cmdoption:: -cl layername

    指定裁切矢量的图层名称。

.. cmdoption:: -cwhere expression

    从裁切矢量中根据属性表查询指定的要素来裁切图像。 

.. cmdoption:: -csql query

    使用SQL语句来从裁切矢量的属性表中查询要素来裁切图像。 

.. cmdoption:: -cblend distance

    Set a blend distance to use to blend over cutlines (in pixels).（这个参数不太清楚，没用过） 

.. cmdoption:: -crop_to_cutline

    (GDAL >= 1.8.0) 使用矢量边界的外接矩形大小作为输出影像的范围。

.. cmdoption:: -overwrite

    (GDAL >= 1.8.0) 如果结果数据存在，那么覆盖结果数据。

.. cmdoption:: srcfile

    输入数据文件名（可以为多个，使用空格隔开）。 

.. cmdoption:: dstfile

    输出数据文件名。 
    如果输出文件已经存在，那么镶嵌到这个文件是可以的。但是数据的空间范围等信息不会被修改，如果要修改为新的数据的空间信息，那么需要使用-overwrite选项来覆盖原文件。        


.. _gdal_translate:

****************************************
gdal_translate格式转换工具
****************************************
``gdal_translate`` 主要用来转换数据格式 ,用法如下::

        gdal_translate [--help-general]
            [-ot {Byte/Int16/UInt16/UInt32/Int32/Float32/Float64/
            CInt16/CInt32/CFloat32/CFloat64}] [-strict]
            [-of format] [-b band] [-mask band] [-expand {gray|rgb|rgba}]
            [-outsize xsize[%] ysize[%]]
            [-unscale] [-scale [src_min src_max [dst_min dst_max]]]
            [-srcwin xoff yoff xsize ysize] [-projwin ulx uly lrx lry] [-epo] [-eco]
            [-a_srs srs_def] [-a_ullr ulx uly lrx lry] [-a_nodata value]
            [-gcp pixel line easting northing [elevation]]*
            [-mo "META-TAG=VALUE"]* [-q] [-sds]
            [-co "NAME=VALUE"]* [-stats]
            src_dataset dst_dataset

参数说明:

.. program:: gdal_translate

.. cmdoption:: -ot: type

    输出文件波段数据类型{Byte/Int16/UInt16/UInt32/Int32/Float32/Float64/CInt16/CInt32/CFloat32/CFloat64}中的一个，默认与元数据集相同

.. cmdoption:: -strict

    严格模式，如果数据不匹配或者数据丢失，可能导致输出失败

.. cmdoption:: -of format

    选择输出格式,默认是GTiff格式

.. cmdoption:: -b band

    选择需要输出的波段,波段起始值为1,如果需要多个波段,或者调整波段顺序,请设置多个-b参数，例如 ``-b 3 -b 2 -b 1``.

.. cmdoption:: -mask band

    选择波段作为掩膜波段

.. cmdoption:: -expand gray|rgb|rgba

    将一个带颜色表的波段分解为3个（rgb）或4个波段（rgba）。主要用来输出JPEG, JPEG2000, MrSID, ECW一类不支持颜色索引的数据格式。gray表示将一个含有颜色表的数据集分解为一个灰度索引的数据集。

.. cmdoption:: -outsize xsize[%] ysize[%]

    输出文件大小,单位为像素,带%则为相对源图像的比率

.. cmdoption:: -scale [src_min src_max [dst_min dst_max]]

    输入像素值的比例范围,从dst_min dst_max到src_min src_max。如果省略了输出范围是0到255。如果省略了输入范围,将自动从源数据计算

.. cmdoption:: -unscale

    将波段中使用元数据中的按比例缩放的值转换为实际值，经常与 ``-ot`` 选项一起使用来重置输出类型。 

.. cmdoption:: -srcwin xoff yoff xsize ysize

    从原图中裁切一个子窗口，起点坐标为 ``xoff,yoff`` ,长为 ``xsize`` ,宽为 ``ysize`` 。

.. cmdoption:: -projwin ulx uly lrx lry

    从原图中裁切一个子窗口，使用地理投影坐标， ``ulx,uly`` 为左上角坐标， ``lrx,lry`` 为右下角坐标

.. cmdoption:: -epo

    ``Error when Partially Outside`` 。如果 ``-srcwin`` 或 ``-projwin`` 裁剪的子窗口部分超出原图，将提示错误

.. cmdoption:: -eco

    ``Error when Completely Outside`` 。基本和 ``-epo`` 相同，除了如果子窗口完全超出原图，错误提示不同。

.. cmdoption:: -a_srs srs_def

    指定输出文件投影，投影字符串可以是wkt、proj4、EPSG号等格式

.. cmdoption:: -a_ullr ulx uly lrx lry

    指定输出文件的地理投影边界，使用地理投影坐标， ``ulx,uly`` 为左上角坐标， ``lrx,lry`` 为右下角坐标

.. cmdoption:: -a_nodata value

    指定nodata值。

.. cmdoption:: -mo “META-TAG=VALUE”

    设置元数据

.. cmdoption:: -co “NAME=VALUE”

    设置创建选项，，参考 :ref:`createdataset` 一节代码示例，可以设置多个。

.. cmdoption:: -gcp pixel line easting northing elevation

    设置GCP点，可以设置多个

.. cmdoption:: -q

    静默输出，禁止显示进度条和非错误信息

.. cmdoption:: -sds

    将每个子数据集到单独的输出文件中。主要用于HDF等有子数据集的数据转换

.. cmdoption:: -stats

    强制重计算统计信息

.. cmdoption:: src_dataset

    源数据集名称

.. cmdoption:: dst_dataset

    输出数据集名称

****************************************
gdalmanage文件管理工具
****************************************
``gdalmanage`` 主要用来管理栅格数据,用法如下::

    gdalmanage mode [-r] [-u] [-f format]
                  datasetname [newdatasetname]

参数说明:

.. program:: gdalmanage

.. cmdoption:: mode

    操作模式
    
    * identify datasetname:     显示数据格式
    * copy datasetname newdatasetname:     拷贝数据
    * rename datasetname newdatasetname:  重命名数据
    * delete datasetname:  删除数据

.. cmdoption:: -r

    递归扫描文件/文件夹

.. cmdoption:: -u 

    如果文件无法执行操作，显示操作失败信息

.. cmdoption:: -f format

    指定数据格式，例如GTiff

.. cmdoption:: datasetname

    需要操作的文件名

.. cmdoption:: newdatasetname

    复制或重命名操作时，需要设置此参数。

实例::

    gdalmanage identify -r 50m_raster/

    gdalmanage copy NE1_50M_SR_W.tif ne1_copy.tif

    gdalmanage rename NE1_50M_SR_W.tif ne1_rename.tif

    gdalmanage identify NE1_50M_SR_W.tif

****************************************
gdallocationinfo像元查询工具
****************************************
``gdallocationinfo`` 通过指定一种坐标系中的坐标来进行对其位置的像元值进行输出显示

用法如下::

    gdallocationinfo [--help-general] [-xml] [-lifonly] [-valonly]
                    [-b band]* [-overview overview_level]
                    [-l_srs srs_def] [-geoloc] [-wgs84]
                    srcfile [x y]

参数说明:

.. program:: gdallocationinfo

.. cmdoption:: --help-general

    显示帮助

.. cmdoption:: -xml

    输出xml信息 

.. cmdoption:: -lifonly

    输出LocationInfo 中的信息。

.. cmdoption:: -valonly

    仅输出指定位置的每个波段的像元值

.. cmdoption:: -l_srs srs def

    指定输入的x,y坐标的坐标系
    
.. cmdoption:: -geoloc

    表示输入的x,y坐标是地理参考坐标系

.. cmdoption:: -wgs84

    表示输入的x,y是WGS84坐标系下的经纬度坐标

.. cmdoption:: srcfile

    输入栅格图像的名称
    
.. cmdoption:: x

    查询的X坐标。默认为图像列号，如果使用-l_srs,-wgs84或者-geoloc时按照指定的坐标来处理

.. cmdoption:: y

    查询的Y坐标。默认为图像行号，如果使用-l_srs,-wgs84或者-geoloc时按照指定的坐标来处理

该工具的目的是输出一个像素的各种信息。目前支持下面四项：

* 像素的行列号。
* 输出元数据中的LocationInfo 信息，目前只支持VRT文件。
* 全部波段或子文件中的所有波段中的像元值。
* 未缩放的像元值，如果对波段进行了缩放和偏移操作。

输入x和y坐标是，是通过命令行来进行输入的，一般是先x后y，即先列号后行号；如果使用-geoloc, -wgs84, 或-l_srs 选项后，会自动根据输入进行交换。默认的输出信息是纯文本文件，也可以使用-xml选项将其使用xml格式进行输出。将来会添加其他的信息。

实例::

    gdallocationinfo utm.tif 256 256
    Report:
      Location: (256P,256L)
      Band 1:
        Value: 115
        
****************************************
gdaltransform坐标系转换工具
****************************************

``gdaltransform`` 坐标系转换工具,用来转换坐标，从支持的投影，包括GCP点的变换。

用法::

    gdaltransform [--help-general]
                  [-i] [-s_srs srs_def] [-t_srs srs_def] [-to "NAME=VALUE"]
                  [-order n] [-tps] [-rpc] [-geoloc]
                  [-gcp pixel line easting northing [elevation]]*
                  [srcfile [dstfile]]

参数说明：

.. program:: gdaltransform

.. cmdoption:: -s_srs srs def

    原始空间参考，该值将被OGRSpatialReference.SetFromUserInput()函数调用，支持包括EPSG PCS 和GCSes (如EPSG:4296), PROJ.4声明或者一个后缀名为.prf的文件存储的WKT串。

.. cmdoption:: -t_srs srs_def

    目标空间参考，该值将被OGRSpatialReference.SetFromUserInput()函数调用，支持包括EPSG PCS 和GCSes (如EPSG:4296), PROJ.4声明或者一个后缀名为.prf的文件存储的WKT串。

.. cmdoption:: -to NAME=VALUE

    设置转换参数，将会使函数GDALCreateGenImgProjTransformer2()来调用。

.. cmdoption:: -order n

    几何多项式变换次数（1到3），默认情况下会根据输入的GCP点的个数自动确定。

.. cmdoption:: -tps

    强制将GCP点使用TPS转换方式转换。

.. cmdoption:: -rpc

    强制使用RPC参数转换。

.. cmdoption:: -geoloc

    强制使用Geolocation数组转换。

.. cmdoption:: -i

    逆转换，从目标到原始投影转换。

.. cmdoption:: -gcppixel line easting northing [elevation]

    指定GCP点，通常至少需要三个。

.. cmdoption:: srcfile

    原始投影下定义的GCP点文件，如果没有指定，需要从命令行-s_srs或者-gcp参数输入。

.. cmdoption:: dstfile

    目标投影定义文件。

读取的坐标必须是成对出现（或者三个一组），输出也以同样的方式输出，所有的变换共gdalwarp相同，包括使用GCP的变换。注意输入输出必须使用十进制小数，当前不支持DMS（度分秒）的输入输出。如果指定了输入图像，那么输入的坐标是基于图像的行列号，如果输出图像指定，输出的坐标也是图像的行列号。

实例::

    下面的命令行使用基于RPC变换，并且使用-i选项来将经纬度坐标转为图像的行列号：
    gdaltransform -i -rpc 06OCT20025052-P2AS-005553965230_01_P001.TIF
    125.67206 39.85307 50 
    输出：
    3499.49282422381 2910.83892848414 50


    
****************************************
gdalsrsinfo格式化SRS工具
****************************************
``gdalsrsinfo`` 将给定的SRS按照不同的格式显示,用法如下::

    gdalsrsinfo [--help-general/-h] [-p] [-V] [-o] srs_def

参数说明:

.. program:: gdalsrsinfo

.. cmdoption:: --help-general/-h

    显示帮助

.. cmdoption:: -p

    格式化输出信息 (e.g. WKT)

.. cmdoption:: -V

    验证 SRS

.. cmdoption:: -o out_type

    输出类型 { default, all, wkt_all, proj4, wkt, wkt_simple, wkt_noct, wkt_esri, mapinfo, xml }

.. cmdoption:: srs_def

    可以是一个GDAL或者OGR支持的文件，或者是任何GDAL和OGR支持的SRS格式（包括WKT,PROJ4,EPSG:n等）

实例::

    gdalsrsinfo   "EPSG:4326"

    PROJ.4 : '+proj=longlat +datum=WGS84 +no_defs '

    OGC WKT :
    GEOGCS["WGS 84",
        DATUM["WGS_1984",
            SPHEROID["WGS 84",6378137,298.257223563,
                AUTHORITY["EPSG","7030"]],
            AUTHORITY["EPSG","6326"]],
        PRIMEM["Greenwich",0,
            AUTHORITY["EPSG","8901"]],
        UNIT["degree",0.0174532925199433,
            AUTHORITY["EPSG","9122"]],
        AUTHORITY["EPSG","4326"]]

