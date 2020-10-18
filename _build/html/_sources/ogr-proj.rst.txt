.. highlight:: rst
.. _OGRProj:

############################
OGR投影简介
############################
本章来源： `OGR Projections Tutorial <http://www.gdal.org/osr_tutorial.html>`_

********************
简介
********************
``OGRSpatialReference`` ，和 ``OGRCoordinateTransformation`` 类提供的表达坐标系统（投影和基准面）和它们之间的变换的接口。这些服务是仿照OpenGIS规范的坐标变换，并使用WKT格式描述的坐标系统。

`Simple Features for COM <http://www.opengeospatial.org/standards/sfo/>`_ 中可以找到 ``OpenGIS`` 坐标系统和服务的介绍， ``Spatial Reference Systems Abstract Model`` 的介绍可以在 `OGC <http://www.opengeospatial.org/>`_ 网站上找到。 `GeoTIFF Projections Transform List <http://www.remotesensing.org/geotiff/proj_list/>`_ 中也可以找到WKT的一些说明。 `EPSG Geodesy <http://www.epsg.org/>`_ 网站上的资源也非常有用.

********************
定义地理坐标系
********************
坐标系统被封装在  ``OGRSpatialReference`` 类中。有许多方法将 ``OGRSpatialReference`` 对象初始化为一个有效的坐标系统。有两种主要的坐标系统，地理坐标系（经纬度投影）和投影坐标系（如UTM，单位为米或英尺）。

地理坐标系统包含的基准面信息（半长轴，扁率），本初子午线（通常是格林尼治），和单位类型（通常是度）。下面的代码是地理坐标系统实例::

    OGRSpatialReference oSRS;
    oSRS.SetGeogCS( "My geographic coordinate system",
                    "WGS_1984", 
                    "My WGS84 Spheroid", 
                    SRS_WGS84_SEMIMAJOR, SRS_WGS84_INVFLATTENING, 
                    "Greenwich", 0.0, 
                    "degree", SRS_UA_DEGREE_CONV );

上面的代码中，“My geographic coordinate system”、“My WGS84 Spheroid”、“Greenwich”和“degree”不是关键，主要用于向用户显示投影信息。然而，“WGS_1984” 作为确定基准面的关键，是需要遵循一定规则的。

OGRSpatialReference中已有很多的坐标系统的支持，其中包括“NAD27”、“NAD83”、“wgs72”和“WGS84”等，可以直接调用SetWellKnownGeogCS()获取

.. code-block:: c++

    oSRS.SetWellKnownGeogCS( "WGS84" );

此外，可以使用EPSG数据库中的GCS码数表示投影

.. code-block:: c++

    oSRS.SetWellKnownGeogCS( "EPSG:4326" );
    
``OGRSpatialReference`` 可以调用函数将投影序列化,转为WKT串或者其他形式输出。

.. code-block:: c++

    char    *pszWKT = NULL;
    oSRS.SetWellKnownGeogCS( "WGS84" );
    oSRS.exportToWkt( &pszWKT );
    printf( "%s\n", pszWKT );

输出为::

    GEOGCS["WGS 84",
        DATUM["WGS_1984",
            SPHEROID["WGS 84",6378137,298.257223563,
                AUTHORITY["EPSG",7030]],
            TOWGS84[0,0,0,0,0,0,0],
            AUTHORITY["EPSG",6326]],
        PRIMEM["Greenwich",0,AUTHORITY["EPSG",8901]],
        UNIT["DMSH",0.0174532925199433,AUTHORITY["EPSG",9108]],
        AXIS["Lat",NORTH],
        AXIS["Long",EAST],
        AUTHORITY["EPSG",4326]]

``OGRSpatialReference::importFromWkt()`` 方法可以用WKT坐标系统定义。

********************
投影坐标系
********************
投影坐标系统（如UTM、Lambert Conformal Conic等）默认包含了一个地理坐标系统，以及一个平面坐标与地理坐标相互转换的描述。下面的代码定义了一个UTM 17号带投影，默认为WGS84地理坐标系统。

.. code-block:: c++

    OGRSpatialReference     oSRS;
    oSRS.SetProjCS( "UTM 17 (WGS84) in northern hemisphere." );
    oSRS.SetWellKnownGeogCS( "WGS84" );
    oSRS.SetUTM( 17, TRUE );

调用 ``SetProjCS()`` 函数 设置投影名称。``SetWellKnownGeogCS() `` 设置地理坐标系统， ``SetUTM()`` 设置详细的投影变换参数。在现有版本中，上述步骤顺序不可颠倒，否则无法生成有效投影。

上述定义将WKT版本如下。请注意，UTM 17 在描述参数中被定义::

    PROJCS["UTM 17 (WGS84) in northern hemisphere.",
        GEOGCS["WGS 84",
            DATUM["WGS_1984",
                SPHEROID["WGS 84",6378137,298.257223563,
                    AUTHORITY["EPSG",7030]],
                TOWGS84[0,0,0,0,0,0,0],
                AUTHORITY["EPSG",6326]],
            PRIMEM["Greenwich",0,AUTHORITY["EPSG",8901]],
            UNIT["DMSH",0.0174532925199433,AUTHORITY["EPSG",9108]],
            AXIS["Lat",NORTH],
            AXIS["Long",EAST],
            AUTHORITY["EPSG",4326]],
        PROJECTION["Transverse_Mercator"],
        PARAMETER["latitude_of_origin",0],
        PARAMETER["central_meridian",-81],
        PARAMETER["scale_factor",0.9996],
        PARAMETER["false_easting",500000],
        PARAMETER["false_northing",0]]
        
有许多投影方法包括settm()方法（横轴），setlcc()（Lambert Conformal Conic），和setmercator()。

********************
查询坐标系统
********************
一旦一个 ``OGRSpatialReference`` 已经建立，它的信息可以被查询。 

* ``OGRSpatialReference::IsProjected()`` ``OGRSpatialReference::IsGeographic()`` 函数确定投影类型
* ``OGRSpatialReference::GetSemiMajor()`` , ``OGRSpatialReference::GetSemiMinor()``   ``OGRSpatialReference::GetInvFlattening()`` 方法可用于获取有关球体的信息。
*  ``OGRSpatialReference::GetAttrValue()``  方法可以用来获得projcs，geogcs，基准，球体，和投影名称的字符串
* ``OGRSpatialReference::GetProjParm()`` 方法可用于获取投影参数。 
* ``OGRSpatialReference::GetLinearUnits()`` 方法可用于获取投影的单位,可以转化到米。

下面的代码演示了如何获取信息:

.. code-block:: c++

    const char *pszProjection = poSRS->GetAttrValue("PROJECTION");
    if( pszProjection == NULL )
    {
        if( poSRS->IsGeographic() )
            sprintf( szProj4+strlen(szProj4), "+proj=longlat " );
        else
            sprintf( szProj4+strlen(szProj4), "unknown " );
    }
    else if( EQUAL(pszProjection,SRS_PT_CYLINDRICAL_EQUAL_AREA) )
    {
        sprintf( szProj4+strlen(szProj4),
           "+proj=cea +lon_0=%.9f +lat_ts=%.9f +x_0=%.3f +y_0=%.3f ",
                 poSRS->GetProjParm(SRS_PP_CENTRAL_MERIDIAN,0.0),
                 poSRS->GetProjParm(SRS_PP_STANDARD_PARALLEL_1,0.0),
                 poSRS->GetProjParm(SRS_PP_FALSE_EASTING,0.0),
                 poSRS->GetProjParm(SRS_PP_FALSE_NORTHING,0.0) );
    }
    ...


********************
坐标转换
********************
``OGRCoordinateTransformation`` 类用于不同坐标系之间的转换。使用 ``OGRCreateCoordinateTransformation()``  创建转换对象，然后调用 ``OGRCoordinateTransformation::Transform()`` 方法转换坐标。

.. code-block:: c++

    OGRSpatialReference oSourceSRS, oTargetSRS;
    OGRCoordinateTransformation *poCT;
    double x, y;

    oSourceSRS.importFromEPSG( atoi( papszArgv[i + 1] ) );
    oTargetSRS.importFromEPSG( atoi( papszArgv[i + 2] ) );

    poCT = OGRCreateCoordinateTransformation( &oSourceSRS,
           &oTargetSRS );
    x = atof( papszArgv[i + 3] );
    y = atof( papszArgv[i + 4] );

    if( poCT == NULL || !poCT->Transform( 1, &x, &y ) )
    {
      printf( "Transformation failed.\n" );
    } else {
      printf( "(%f,%f) -> (%f,%f)\n",
              atof( papszArgv[i + 3] ),
              atof( papszArgv[i + 4] ),
              x, y );
    }

投影转换失败可能原因如下:

* ``OGRCreateCoordinateTransformation()``  可能会失败，一般是因为系统之间可以建立没有变换。这可能是由于PROJ.4库不支持。如果 ``OGRCreateCoordinateTransformation()`` 失败，它将返回一个null。
* ``OGRCoordinateTransformation::Transform()`` 方法本身也会失败。

上述代码中没有演示三维点转换，但是该函数是可以用于三维点转换的，如果没有传入Z值，假设点在大地水准面上。

下面的示例演示如何方便地创建一个纬度/经度坐标系统使用相同的地理坐标系统和投影坐标系，并利用这些投影坐标和经纬度之间的转换。

.. code-block:: c++

    OGRSpatialReference    oUTM, *poLatLong;
    OGRCoordinateTransformation *poTransform;
    oUTM.SetProjCS("UTM 17 / WGS84");
    oUTM.SetWellKnownGeogCS( "WGS84" );
    oUTM.SetUTM( 17 );
    poLatLong = oUTM.CloneGeogCS();
    poTransform = OGRCreateCoordinateTransformation( &oUTM, poLatLong );
    if( poTransform == NULL )
    {
        ...
    }
    ...
    if( !poTransform->Transform( nPoints, x, y, z ) )

********************
其他语言接口
********************
C接口定义的坐标系统服务 ``ogr_srs_api.h`` ，和Python绑定可以通过 ``osr.py`` 模块。其他语言用法类似c++，但是缺少一些方法。

.. code-block:: c

    typedef void *OGRSpatialReferenceH;                               
    typedef void *OGRCoordinateTransformationH;
    OGRSpatialReferenceH OSRNewSpatialReference( const char * );
    void    OSRDestroySpatialReference( OGRSpatialReferenceH );
    int     OSRReference( OGRSpatialReferenceH );
    int     OSRDereference( OGRSpatialReferenceH );
    OGRErr  OSRImportFromEPSG( OGRSpatialReferenceH, int );
    OGRErr  OSRImportFromWkt( OGRSpatialReferenceH, char ** );
    OGRErr  OSRExportToWkt( OGRSpatialReferenceH, char ** );
    OGRErr  OSRSetAttrValue( OGRSpatialReferenceH hSRS, const char * pszNodePath,
                             const char * pszNewNodeValue );
    const char *OSRGetAttrValue( OGRSpatialReferenceH hSRS,
                                 const char * pszName, int iChild);
    OGRErr  OSRSetLinearUnits( OGRSpatialReferenceH, const char *, double );
    double  OSRGetLinearUnits( OGRSpatialReferenceH, char ** );
    int     OSRIsGeographic( OGRSpatialReferenceH );
    int     OSRIsProjected( OGRSpatialReferenceH );
    int     OSRIsSameGeogCS( OGRSpatialReferenceH, OGRSpatialReferenceH );
    int     OSRIsSame( OGRSpatialReferenceH, OGRSpatialReferenceH );
    OGRErr  OSRSetProjCS( OGRSpatialReferenceH hSRS, const char * pszName );
    OGRErr  OSRSetWellKnownGeogCS( OGRSpatialReferenceH hSRS,
                                   const char * pszName );
    OGRErr  OSRSetGeogCS( OGRSpatialReferenceH hSRS,
                          const char * pszGeogName,
                          const char * pszDatumName,
                          const char * pszEllipsoidName,
                          double dfSemiMajor, double dfInvFlattening,
                          const char * pszPMName ,
                          double dfPMOffset ,
                          const char * pszUnits,
                          double dfConvertToRadians );
    double  OSRGetSemiMajor( OGRSpatialReferenceH, OGRErr * );
    double  OSRGetSemiMinor( OGRSpatialReferenceH, OGRErr * );
    double  OSRGetInvFlattening( OGRSpatialReferenceH, OGRErr * );
    OGRErr  OSRSetAuthority( OGRSpatialReferenceH hSRS,
                             const char * pszTargetKey,
                             const char * pszAuthority,
                             int nCode );
    OGRErr  OSRSetProjParm( OGRSpatialReferenceH, const char *, double );
    double  OSRGetProjParm( OGRSpatialReferenceH hSRS,
                            const char * pszParmName, 
                            double dfDefault,
                            OGRErr * );
    OGRErr  OSRSetUTM( OGRSpatialReferenceH hSRS, int nZone, int bNorth );
    int     OSRGetUTMZone( OGRSpatialReferenceH hSRS, int *pbNorth );
    OGRCoordinateTransformationH
    OCTNewCoordinateTransformation( OGRSpatialReferenceH hSourceSRS,
                                    OGRSpatialReferenceH hTargetSRS );
    void OCTDestroyCoordinateTransformation( OGRCoordinateTransformationH );
    int OCTTransform( OGRCoordinateTransformationH hCT,
                      int nCount, double *x, double *y, double *z );

Python 接口:

.. code-block:: python

    class osr.SpatialReference
        def __init__(self,obj=None):
        def ImportFromWkt( self, wkt ):
        def ExportToWkt(self):
        def ImportFromEPSG(self,code):
        def IsGeographic(self):
        def IsProjected(self):
        def GetAttrValue(self, name, child = 0):
        def SetAttrValue(self, name, value):
        def SetWellKnownGeogCS(self, name):
        def SetProjCS(self, name = "unnamed" ):
        def IsSameGeogCS(self, other):
        def IsSame(self, other):
        def SetLinearUnits(self, units_name, to_meters ):
        def SetUTM(self, zone, is_north = 1):
    class CoordinateTransformation:
        def __init__(self,source,target):
        def TransformPoint(self, x, y, z = 0):
        def TransformPoints(self, points):

********************
内部实现
********************

内部实现基于 ``PROJ.4`` 库,接口为 ``OGRCoordinateTransformation``