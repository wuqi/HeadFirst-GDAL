.. highlight:: rst
.. _OGRUtilities:

############################
OGR工具集
############################
主要来自 `OGR Utility Programs <http://www.gdal.org/ogr_utilities.html>`_

********************
通用选项
********************
所有ogr工具通用选项

.. program:: ogr

.. cmdoption:: --version

        返回GDAL版本.

.. cmdoption:: --formats

        返回所有支持的格式.

.. cmdoption:: --format format

        某种格式的详细信息.

.. cmdoption:: --optfile file

        将一个文件中的内容作为工具参数列表.

.. cmdoption:: --config key value

        设置系统参数.

.. cmdoption:: --debug value

        设置debug级别.

.. cmdoption:: --help-general

        显示通用选项.

********************
ogrinfo
********************
``ogrinfo`` 主要用来显示矢量数据的所有信息,也可以用sql操作矢量, ``ogrinfo`` 命令打开文件默认为读写方式打开,部分只读驱动会发出警告,用法如下::

    ogrinfo [--help-general] [-ro] [-q] [-where restricted_where|\@filename]
            [-spat xmin ymin xmax ymax] [-geomfield field] [-fid fid]
            [-sql statement|\@filename] [-dialect dialect] [-al] [-so] [-fields={YES/NO}]
            [-geom={YES/NO/SUMMARY/WKT/ISO_WKT}] [-formats] [[-oo NAME=VALUE] ...]
            [-nomd] [-listmdd] [-mdd domain|`all`]*
            [-nocount] [-noextent]
            datasource_name [layer [layer ...]]

参数说明:

.. program:: ogrinfo

.. cmdoption:: -ro:

    以只读方式打开数据集.

.. cmdoption:: -al:

    列出所有图层的所有要素 (used instead of having to give layer names as arguments).

.. cmdoption:: -so:
    
    仅列出摘要:只列出部分要素,只显示投影/要素数量和边界范围等基本信息
    
.. cmdoption:: -q:
    
    隐藏提示信息.

.. cmdoption:: -where restricted_where:

    用 sql where 语句限定范围,仅输出查询部分的要素信息,GDAL2.1后可以使用 ``\filename`` 语法
    
.. cmdoption:: -sql statement:

    执行SQL语句.GDAL2.1以后,可以使用 ``@filename`` 语法指定针对文件查询
    
.. cmdoption:: -dialect dialect:

    特定SQL语法类型 OGRSQL 或 SQLITE .

.. cmdoption:: -spat xmin ymin xmax ymax:

    感兴趣区域范围,仅显示指定范围内容

.. cmdoption:: -geomfield field:

    指定 ``geometry`` 字段名字,用于空间条件过滤查询.

.. cmdoption:: -fid fid:

    指定查询的 ``feature id``

.. cmdoption:: -fields={YES/NO}:

    如果设置为NO,则不显示feature的字段值.
    
.. cmdoption:: -geom={YES/NO/SUMMARY/WKT/ISO_WKT}:

    如何显示geometry,默认为YES,显示为WKT字符串

.. cmdoption:: -oo NAME=VALUE:

    数据集的打开选项(GDAL 2.0)

.. cmdoption:: -nomd

    (GDAL 2.0) 不显示元数据
    
.. cmdoption:: -listmdd
    
    (GDAL 2.0)显示所有元数据.
    
.. cmdoption:: -mdd domain

    (GDAL 2.0) 显示指定元数据信息
    
.. cmdoption:: -nocount
    
    ( GDAL 2.0) 不显示要素数量.
    
.. cmdoption:: -noextent

    (GDAL 2.0) 不显示图层范围.

.. cmdoption:: datasource_name:

    数据集名称
    
.. cmdoption:: layer:

    一个或多个图层名
    
********************
ogr2ogr
********************
``ogr2ogr`` 主要用于矢量格式转换,属性设置,重投影等,用法如下::

    Usage: ogr2ogr [--help-general] [-skipfailures] [-append] [-update]
                   [-select field_list] [-where restricted_where|\@filename] 
                   [-progress] [-sql <sql statement>|\@filename] [-dialect dialect]
                   [-preserve_fid] [-fid FID]
                   [-spat xmin ymin xmax ymax] [-spat_srs srs_def] [-geomfield field]
                   [-a_srs srs_def] [-t_srs srs_def] [-s_srs srs_def]
                   [-f format_name] [-overwrite] [[-dsco NAME=VALUE] ...]
                   dst_datasource_name src_datasource_name
                   [-lco NAME=VALUE] [-nln name]
                   [-nlt type|PROMOTE_TO_MULTI|CONVERT_TO_LINEAR|CONVERT_TO_CURVE]
                   [-dim XY|XYZ|XYM|XYZM|2|3|layer_dim] [layer [layer ...]]

    Advanced options :
                   [-gt n]
                   [[-oo NAME=VALUE] ...] [[-doo NAME=VALUE] ...]
                   [-clipsrc [xmin ymin xmax ymax]|WKT|datasource|spat_extent]
                   [-clipsrcsql sql_statement] [-clipsrclayer layer]
                   [-clipsrcwhere expression]
                   [-clipdst [xmin ymin xmax ymax]|WKT|datasource]
                   [-clipdstsql sql_statement] [-clipdstlayer layer]
                   [-clipdstwhere expression]
                   [-wrapdateline] [-datelineoffset val]
                   [[-simplify tolerance] | [-segmentize max_dist]]
                   [-addfields] [-unsetFid]
                   [-relaxedFieldNameMatch] [-forceNullable] [-unsetDefault]
                   [-fieldTypeToString All|(type1[,type2]*)] [-unsetFieldWidth]
                   [-mapFieldType type1|All=type2[,type3=type4]*]
                   [-fieldmap identity | index1[,index2]*]
                   [-splitlistfields] [-maxsubfields val]
                   [-explodecollections] [-zfield field_name]
                   [-gcp pixel line easting northing [elevation]]* [-order n | -tps]
                   [-nomd] [-mo "META-TAG=VALUE"]* [-noNativeData]

.. program:: ogr2ogr

.. cmdoption:: -f format_name:

    默认为 ``-f "ESRI Shapefile"`` ,设置输出格式
    
.. cmdoption:: -append:

    添加到已存在的图层中,而非新建图层
    
.. cmdoption:: -update:

    更新已存在的数据集
    
.. cmdoption:: -select field_list:

    将字段复制到新图层中,使用逗号隔开字段名,默认全部复制, GDAL1.11开始可以复制geometry字段
    
.. cmdoption:: -progress:
    
    显示进度条,仅当输入图层有"fast feature count"属性

.. cmdoption:: -sql sql_statement:

    执行SQL语句,结果将输出到文件中.GDAL2.1以后,可以使用 ``@filename`` 语法指定针对文件查询.
    
.. cmdoption:: -dialect dialect:

    特定SQL语法类型 OGRSQL 或 SQLITE .
    
.. cmdoption:: -where restricted_where:

    属性查询 (like SQL WHERE). GDAL2.1以后,可以使用 ``@filename`` 语法指定针对文件查询.
    
.. cmdoption:: -skipfailures:

    跳过错误继续执行
    
.. cmdoption:: -spat xmin ymin xmax ymax:

    空间范围指定,空间范围外的要素将被剔除,范围内几何要素默认不被裁剪,除非设置 ``-clipsrc``
    
.. cmdoption:: -spat_srs srs_def:

    (OGR >= 2.0) 覆盖空间过滤查询的 SRS.
    
.. cmdoption:: -geomfield field:

    (OGR >= 1.11) 空间过滤条件的geometry 字段名.
    
.. cmdoption:: -dsco NAME=VALUE:

    数据集创建选项 (format specific)

.. cmdoption:: -lco NAME=VALUE:

    图层创建选项(format specific)

.. cmdoption:: -nln name:

    指定创建图层的名称

.. cmdoption:: -nlt type:

    指定创建图层的几何类型 NONE, GEOMETRY, POINT, LINESTRING, POLYGON, GEOMETRYCOLLECTION, MULTIPOINT, MULTIPOLYGON or MULTILINESTRING. And CIRCULARSTRING, COMPOUNDCURVE, CURVEPOLYGON, MULTICURVE and MULTISURFACE ( GDAL 2.0). 在类型后添加 "Z", "M", 或 "ZM" 可以设置高程/测量信息.GDAL 1.10后, PROMOTE_TO_MULTI 可以自动将polygon multipolygons混合图层转为multipolygons图层,linestrings 和 multilinestrings混合图层转为multilinestrings图层.GDAL 2.0 后, 可以使用CONVERT_TO_LINEAR将其他图层转为线层, 使用 CONVERT_TO_CURVE 将非线性图层转为广义曲线类型(POLYGON to CURVEPOLYGON, MULTIPOLYGON to MULTISURFACE, LINESTRING to COMPOUNDCURVE, MULTILINESTRING to MULTICURVE).  2.1 后类型可以添加25D,与Z相同
    
.. cmdoption:: -dim val:

    (>GDAL 1.10) 强制坐标维数 ( XY, XYZ, XYM,  XYZM -). 
    
.. cmdoption:: -a_srs srs_def:

    设置输出 SRS

.. cmdoption:: -t_srs srs_def:

    重投影/变换到此 SRS
    
.. cmdoption:: -s_srs srs_def:

    重指定输入 SRS

.. cmdoption:: -preserve_fid:
    
    指定与源数据相同的FID,而非默认生成FID. GDAL 2.0 以后此为默认选项,可以使用 -unsetFid 禁止此行为.

.. cmdoption:: -fid fid:

    处理指定fid要素,与 '-where "fid in (1,3,5)"' 效果类似.

高级选项 :

.. cmdoption:: -oo NAME=VALUE:

    (>= GDAL 2.0) 输入数据集打开选项(format specific)

.. cmdoption:: -doo NAME=VALUE:

    (>=GDAL 2.0) 输出数据集打开选项, (format specific), 仅在 -update 模式中使用
    
.. cmdoption:: -gt n:

    每次提交事务中多少要素进行转换(OGR 1.11 默认20000  , 之前版本为200),越大越快. GDAL 2.0之后可以使用unlimited一次事务中全部处理.
    
.. cmdoption:: -ds_transaction:

    (>=GDAL 2.0) 强制使用事务

.. cmdoption:: -clipsrc [xmin ymin xmax ymax]|WKT|datasource|spat_extent:

    (>=GDAL 1.7.0) 使用包络框裁切几何要素,如果指定datasource,还需使用 -clipsrcsql -clipsrclayer  -clipsrcwhere 指定裁切要素

.. cmdoption:: -clipsrcsql sql_statement:

    使用sql查询选择所需裁切几何要素

.. cmdoption:: -clipsrclayer layername:

    选择裁切图层
    
.. cmdoption:: -clipsrcwhere expression:

    使用属性查询重新限定裁切要素

.. cmdoption:: -clipdst [xmin ymin xmax ymax]|WKT|datasource|spat_extent:

    (>=GDAL 1.7.0) 重投影后将要素裁切到指定范围内,如果指定数据源,还需使用-clipdstlayer, -clipdstwhere  -clipdstsql指定带裁切的数据

.. cmdoption:: -clipdstsql sql_statement:

    使用sql查询选择所需待裁切几何要素

.. cmdoption:: -clipdstlayer layername:

    选择待裁切图层

.. cmdoption:: -clipdstwhere expression:

    使用属性查询重新限定待裁切要素

.. cmdoption:: -wrapdateline:

    (starting with GDAL 1.7.0) 分割穿过经线的几何要素 (long. = +/- 180deg)

.. cmdoption:: -datelineoffset:

    (starting with GDAL 1.10) 偏移分割经线 (default long. = +/- 10deg, geometries within 170deg to -170deg will be split)

.. cmdoption:: -simplify tolerance:

    (starting with GDAL 1.9.0) 化简图形,化简是保持每个图形的拓扑,不保证整个层的拓扑.

.. cmdoption:: -segmentize max_dist:

    (starting with GDAL 1.6.0) 两节点间最大距离.用于创建分割节点

.. cmdoption:: -fieldTypeToString type1,...:

    (starting with GDAL 1.7.0)将指定字段类型转为String类型到输出图层中,可以指定类型为: Integer, Integer64, Real, String, Date, Time, DateTime, Binary, IntegerList, Integer64List, RealList, StringList. 

.. cmdoption:: -mapFieldType srctype|All=dsttype,...:

    (starting with GDAL 2.0) 将指定字段类型转换为其他类型,可指定类型为: Integer, Integer64, Real, String, Date, Time, DateTime, Binary, IntegerList, Integer64List, RealList, StringList.可以使用括号指定同时被转换的子类型, 例如:Integer(Boolean), Real(Float32)

.. cmdoption:: -unsetFieldWidth:

    (starting with GDAL 1.11)重置字段宽度和精度为0.

.. cmdoption:: -splitlistfields:

    (starting with GDAL 1.8.0) 将列表字段分割为多个字段,例如 StringList, RealList ,IntegerList转为多个 String, Real or Integer.

.. cmdoption:: -maxsubfields val:

   与 -splitlistfields 结合使用,限制分割个数

.. cmdoption:: -explodecollections:

    (starting with GDAL 1.8.0) 为每个geometry生成一个feature

.. cmdoption:: -zfield field_name:

    (starting with GDAL 1.8.0)用指定字段填充Z值

.. cmdoption:: -gcp ungeoref_x ungeoref_y georef_x georef_y elevation:

    (starting with GDAL 1.10.0)指定地面控制点 ,可添加多个.

.. cmdoption:: -order n:

    (starting with GDAL 1.10.0)指定转换时多项式次数,默认根据控制点选择多项式阶数.

.. cmdoption:: -tps:

    (starting with GDAL 1.10.0) 强制使用tps转换

.. cmdoption:: -fieldmap:

    (starting with GDAL 1.10.0)与 -append一起使用,指定需要从源图层复制的字段索引列表,索引从0开始计算

.. cmdoption:: -addfields:

    (starting with GDAL 1.11)这是特定的-append版本.只能添加字段到指定输出图层中,一般用于合并字段不完全相同的文件

.. cmdoption:: -relaxedFieldNameMatch:

    (starting with GDAL 1.11) 放宽字段名称匹配条件 [-relaxedFieldNameMatch] [-forceNullable]

.. cmdoption:: -forceNullable:

    (starting with GDAL 2.0)去掉源图层禁止非空值的约束

.. cmdoption:: -unsetDefault:

    (starting with GDAL 2.0)去掉源图层的默认值

.. cmdoption:: -unsetFid:

    (starting with GDAL 2.0) 重置所有fid

.. cmdoption:: -nomd:

    (starting with GDAL 2.0) 禁止拷贝源图层的元数据

.. cmdoption:: -mo "META-TAG=VALUE":

    (starting with GDAL 2.0)指定元数据 

.. cmdoption:: -noNativeData:

    (starting with GDAL 2.1) To disable copying of native data, i.e. details of source format not captured by OGR abstraction, that are otherwise preserved by some drivers (like GeoJSON) when converting to same format.