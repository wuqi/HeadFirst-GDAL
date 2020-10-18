.. highlight:: rst
.. _OGRsql:.. _OGRsql:

.. _OGRsql:

############################
OGR SQL
############################
本章来源：

* GDAL官网: `OGR SQL <http://www.gdal.org/ogr_sql.html>`_  
* GDAL WIKI: `RFC 28: OGR SQL Generalized Expressions <https://trac.osgeo.org/gdal/wiki/rfc28_sqlfunc>`_

********************
简介
********************
``GDALDataset`` 支持使用 ``GDALDataset::ExecuteSQL()`` 进行SQL操作,理论上该方法可以支持任何命令,但实际上主要是SQL SELECT 的子集.本篇主要讨论基本SQL和OGR实现的SQL,以及特定驱动的SQL支持.

自GDAL/OGR 1.10 起,可以使用SQLITE语法代替OGRSQL语法,SQLITE语法将在下文中详细介绍

``OGRLayer::SetAttributeFilter()`` 方法中也支持OGRSQL过滤,用法与SELECT语句中WHERE从句用法一致

.. attention::

    OGR SQL在 版本1.8.0后有效,最好升级到新的版本中

********************
SELECT
********************

语法如下::

    SELECT <field-list> FROM <table_def>
         [LEFT JOIN <table_def> 
          ON [<table_ref>.]<key_field> = [<table_ref>.].<key_field>]*
         [WHERE <where-expr>] 
         [ORDER BY <sort specification list>]

    <field-list> ::= <column-spec> [ { , <column-spec> }... ]

    <column-spec> ::= <field-spec> [ <as clause> ]
                     | CAST ( <field-spec> AS <data type> ) [ <as clause> ]

    <field-spec> ::= [DISTINCT] <field_ref>
                     | <cumm-field-func> ( [DISTINCT] <field-ref> )
                     | <field-expr>
                     | Count(*)

    <field-expr> ::= <field_ref>
                     | <constant-value>
                     | <field-expr> <field-operator> <field-expr>
                     | <field-func> ( <field-expr-list> )
                     | ( <field-expr> )

    <field-expr-list> ::= field-expr
                     |  field-expr , field-expr-list
                     |  <empty>

    <as clause> ::= [ AS ] <column_name>

    <data type> ::= character [ ( field_length ) ]
                    | float [ ( field_length ) ]
                    | numeric [ ( field_length [, field_precision ] ) ]
                    | integer [ ( field_length ) ]
                    | date [ ( field_length ) ]
                    | time [ ( field_length ) ]
                    | timestamp [ ( field_length ) ]

    <cumm-field-func> ::= AVG | MAX | MIN | SUM | COUNT

    <field-operator> ::= '+' | '-' | '/' | '*' | '||'

    <field-func> ::= CONCAT | SUBSTR

    <field_ref>  ::= [<table_ref>.]field_name

********************
SPECIAL FIELDS
********************
OGR SQL中有一些特殊属性可以使用.比如几何属性等.

==============
FID
==============
一般FID不作为要素的属性,只用于标识,但某些情况下能方便的查询,*不包括FID,所以必须单独写出来

.. code-block:: sql

    SELECT FID, * FROM nation;

==============
OGR_GEOMETRY
==============
MapInfo等数据类型中可以在同一层使用多种几何类型,所以可以使用此字段进行选择:


.. code-block:: sql

    SELECT * FROM nation WHERE OGR_GEOMETRY='POINT' OR OGR_GEOMETRY='POLYGON'

==============
OGR_GEOM_WKT
==============
OGR_GEOM_WKT 可以显示要素的WKT串作为输出,也可以在WHERE从句中指定WKT类型等进行过滤::

    SELECT OGR_GEOM_WKT, * FROM nation;
    SELECT OGR_GEOM_WKT, * FROM nation WHERE OGR_GEOM_WKT \
            LIKE 'POINT%' OR OGR_GEOM_WKT LIKE 'POLYGON%';

==============
OGR_GEOM_AREA
==============
该字段使用OGRSurface::get_Area() 方法获取几何要素的面积,如果不是面状要素,则返回0

.. code-block:: sql

    SELECT * FROM nation WHERE OGR_GEOM_AREA > 10000000

==============
OGR_STYLE
==============
该字段代表使用OGRFeature::GetStyleString()获取的字符串,一般用于where从句过滤


.. code-block:: sql

    SELECT * FROM nation WHERE OGR_STYLE LIKE 'LABEL%'

********************
ALTER TABLE
********************
用于增删改字段,主要用法如下:

.. code-block:: sql

    /*添加字段*/
    ALTER TABLE tablename ADD [COLUMN] columnname columntype
    /*重命名字段*/
    ALTER TABLE tablename RENAME [COLUMN] oldcolumnname TO newcolumnname
    /*修改字段类型*/
    ALTER TABLE tablename ALTER [COLUMN] columnname TYPE columntype
    /*删除字段*/
    ALTER TABLE tablename DROP [COLUMN] columnname

支持的数据类型如下::

    boolean     /*GDAL>2.0*/
    character   /*需要指定field_length,默认为1*/
    float       /*field_length*/
    numeric     /*field_length, field_precision*/
    smallint    /*field_length 16bit signed integer GDAL>2.0*/
    integer     /*field_length*/
    bigint      /*field_length 64bit integer GDAL>2.0*/
    date        /*field_length*/
    time        /*field_length*/
    timestamp   /*field_length*/
    geometry,geometry/*geometry_type*/,geometry/*geometry_type,type_code*/

例如,可以使用ogrinfo 命令,修改文件字段

.. code-block:: bash

    ogrinfo -sql "alter table out add COLUMN myfield integer" out.shp

********************
ExecuteSQL()
********************
只有GDALDataset才能执行ExecuteSQL函数,该函数不针对Layer处理

.. code-block:: sql

    /**
    * @param pszSQLCommand     SQL命令
    * @param poSpatialFilter   空间范围设定
    * @param pszDialect        针对不同驱动,有不同语法,一般置为NULL
    */
    OGRLayer * GDALDataset::ExecuteSQL( const char *pszSQLCommand,
                                          OGRGeometry *poSpatialFilter,
                                          const char *pszDialect );


********************
Non-OGR SQL
********************

MySQL, PostgreSQL  PostGIS (PG), Oracle (OCI), SQLite, ODBC, ESRI Personal Geodatabase (PGeo)  MS SQL Spatial (MSSQLSpatial) 这些数据库系统都自己重写了
GDALDataset::ExecuteSQL()  方法,有些语法上有细微的差别,另外其他语句也可以在这些驱动中运行.但是只有 SQL WHERE 语句才能返回 Layer.

