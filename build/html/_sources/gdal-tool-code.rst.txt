.. highlight:: rst
.. _GDALUtilitiesWithCode:

############################
GDAL工具集代码实现(C/C++)
############################

`GDAL2.0` 开始，可以使用代码实现GDAL工具集的功能，即可以使用代码调用gdal工具集，可以提升编码效率。需要注意,使用参数时,需要逐个添加,不能一行按照空格添加。

下面主要介绍 ``gdalinfo`` ``gdalwarp`` ``gdal_translate``  ``ogr2ogr``  几个常用工具的代码实现,其他可用工具见  `GDAL官网gdal_utils.h: GDAL Utilities C API <http://www.gdal.org/gdal__utils_8h.html>`_  说明,包括  ``gdaldem``  ``gdal_rasterize``  ``nearblack``   ``gdal_grid``  ``gdalbuildvrt`` 几个工具，其余工具官方正在添加。


使用前,需要添加以下头文件:

.. code-block:: c++

    #include "gdal_priv.h"
    #include "cpl_string.h"
    #include "gdal_utils.h"


****************************************
gdalinfo
****************************************

:ref:`gdalinfo`  工具功能可以使用 ``GDALInfo`` 函数实现，具体定义如下:

.. code-block:: c++

    /**
    * @param hDataset   需要获取信息的数据集
    * @param psOptions  gdalinfo命令参数
    * @return 返回数据集信息
    */
    char* GDALInfo	(	GDALDatasetH 	hDataset,
    const GDALInfoOptions * 	psOptions 
    )


基本使用方式如下:

.. code-block:: c++

    GDALAllRegister();
    GDALDatasetH TestDs = GDALOpen( "testfile.tiff", GA_ReadOnly );
    char* info = GDALInfo(TestDs,NULL);
    std::cout<<info<<std::endl;
    //do something
    GDALClose(TestDs);
    
如果需要使用命令行参数,则需要创建 ``GDALInfoOptions`` 代码如下:

.. code-block:: c++

    GDALAllRegister();
    //添加命令参数
    char **argv = NULL;
    argv = CSLAddString( argv, "-json");
    argv = CSLAddString( argv, "-hist");
    GDALInfoOptions *opt = GDALInfoOptionsNew(argv,NULL);
    GDALDatasetH TestDs = GDALOpen( "testfile.tiff", GA_ReadOnly );
    char* info = GDALInfo(TestDs,opt);
    GDALInfoOptionsFree(opt);
    std::cout<<info<<std::endl;
    //do something
    GDALClose(TestDs);


****************************************
gdalwarp
****************************************

:ref:`gdalwarptool`  工具功能可以使用 ``GDALWarp`` 函数实现，具体定义如下:

.. code-block:: c++


    /**
    * @param pszDest      输出路径,或者NULL
    * @param hDstDS       输出数据集或为NULL(前两参数必须选一)
    * @param nSrcCount    输入数据集个数
    * @param pahSrcDS     输入数据集列表
    * @param psOptionsIn  命令行参数
    * @param pbUsageError 返回值,标识错误信息
    * @return  输出数据集(如果hDstDS为NULL,返回的数据集必须使用GDALClose关闭)
    */
    GDALDatasetH GDALWarp	(
        const char * 	pszDest,
        GDALDatasetH 	hDstDS,
        int 	nSrcCount,
        GDALDatasetH * 	pahSrcDS,
        const GDALWarpAppOptions * 	psOptionsIn,
        int * 	pbUsageError 
    )

基本使用方式如下:

.. code-block:: c++

    GDALAllRegister();
    //添加命令参数,每次添加一个!!!
    char **argv = NULL;
    argv = CSLAddString( argv, "-order" );
    argv = CSLAddString( argv, "3" );
    argv = CSLAddString( argv, "-ts" );
    argv = CSLAddString( argv, "1000" );
    argv = CSLAddString( argv, "1000" );
    
    //错误实例!!!!
    //error!!!!
    //////argv = CSLAddString( argv, "-order 3 -ts 1000 1000" );//此写法错误,按照上面写!!!!
    //error!!!!
    
    //返回
    int bUsageError = FALSE;
    //输入列表
    GDALDatasetH TestDs = GDALOpen("test.tif", GA_ReadOnly );
    //gdalwarp
    GDALWarpAppOptions *opt = GDALWarpAppOptionsNew( argv, NULL );
    GDALDataset *dst = ( GDALDataset * )GDALWarp( "out.tif", NULL, 1,\
                       &TestDs ,opt, &bUsageError );
    GDALWarpAppOptionsFree( opt );
    CSLDestroy( argv );
    //do something
    //clear env
    GDALClose(dst);
    GDALClose(TestDs);


如果有多个文件输入,使用如下代码:

.. code-block:: c++

    GDALAllRegister();
    //添加命令参数,每次添加一个!!!
    char **argv = NULL;
    argv = CSLAddString( argv, "-order" );
    argv = CSLAddString( argv, "3" );
    argv = CSLAddString( argv, "-ts" );
    argv = CSLAddString( argv, "1000" );
    argv = CSLAddString( argv, "1000" );
    
    //错误实例!!!!
    //error!!!!
    //////argv = CSLAddString( argv, "-order 3 -ts 1000 1000" );//此写法错误,按照上面写!!!!
    //error!!!!
    
    //返回
    int bUsageError = FALSE;
    //输入列表
    GDALDatasetH TestDs = GDALOpen("test.tif", GA_ReadOnly );
    GDALDatasetH *srcList = NULL;
    srcList = ( GDALDatasetH * ) CPLRealloc( srcList, sizeof( GDALDatasetH ) * 1 );
    srcList[0] = TestDs;//有多少写多少
    //gdalwarp
    GDALWarpAppOptions *opt = GDALWarpAppOptionsNew( argv, NULL );
    GDALDataset *dst = ( GDALDataset * )GDALWarp( "out.tif", NULL, 1,\
                       srcList ,opt, &bUsageError );
    GDALWarpAppOptionsFree( opt );
    CSLDestroy( argv );
    //do something
    //clear env
    GDALClose(dst);
    GDALClose(TestDs);
    CPLFree(srcList);


****************************************
gdal_translate
****************************************

:ref:`gdal_translate`  工具功能可以使用 ``GDALTranslate`` 函数实现，具体定义如下:

.. code-block:: c++


    /**
    * @param pszDest      输出路径
    * @param hSrcDataset  输入数据集
    * @param psOptionsIn  命令行参数,可以为空
    * @param pbUsageError 返回值,标识错误信息
    * @return  输出数据集(如果hDstDS为NULL,返回的数据集必须使用GDALClose关闭)
    */
    GDALDatasetH GDALTranslate	(
        const char * 	pszDest,
        GDALDatasetH 	hSrcDataset,
        const GDALTranslateOptions * 	psOptionsIn,
        int * 	pbUsageError 
    )

基本使用方式如下:

.. code-block:: c++

    GDALAllRegister();
    //添加命令参数,每次添加一个!!!
    char **argv = NULL;
    argv = CSLAddString( argv, "-ot" );
    argv = CSLAddString( argv, "UInt16" );
    
    //错误实例!!!!
    //error!!!!
    //////argv = CSLAddString( argv, "-ot UInt16" );//此写法错误,按照上面写!!!!
    //error!!!!
    
    //返回
    int bUsageError = FALSE;
    //输入列表
    GDALDatasetH TestDs = GDALOpen( "test.tif", GA_ReadOnly );
    //GDALTranslate
    GDALTranslateOptions *opt =  GDALTranslateOptionsNew( argv, NULL );
    GDALDataset *dst = ( GDALDataset * )GDALTranslate( "out.tif", TestDs ,opt, &bUsageError );
    GDALTranslateOptionsFree( opt );
    CSLDestroy( argv );

****************************************
ogr2ogr
****************************************

``ogr2ogr``  工具功能可以使用 ``GDALVectorTranslate`` 函数实现，具体定义如下:

.. code-block:: c++


    /**
    * @param pszDest      输出路径,或者NULL
    * @param hDstDS       输出数据集或为NULL(前两参数必须选一)
    * @param nSrcCount    输入数据集个数(至2.1.1为止,只能有1个)
    * @param pahSrcDS     输入数据集列表
    * @param psOptionsIn  命令行参数
    * @param pbUsageError 返回值,标识错误信息
    * @return  输出数据集(如果hDstDS为NULL,返回的数据集必须使用GDALClose关闭)
    */
    GDALDatasetH GDALVectorTranslate	(
        const char * 	pszDest,
        GDALDatasetH 	hDstDS,
        int 	nSrcCount,
        GDALDatasetH * 	pahSrcDS,
        const GDALVectorTranslateOptions * 	psOptionsIn,
        int * 	pbUsageError 
    )

基本使用方式如下:

.. code-block:: c++

    GDALAllRegister();
    //添加命令参数,每次添加一个!!!
    char **argv = NULL;
    argv = CSLAddString( argv, "-f" );
    argv = CSLAddString( argv, "GML" );
    
    //错误实例!!!!
    //error!!!!
    //////argv = CSLAddString( argv, "-f GML" );//此写法错误,按照上面写!!!!
    //error!!!!
    
    //返回
    int bUsageError = FALSE;
    //输入列表
    GDALDatasetH TestDs = GDALOpenEx("test.shp",GDAL_OF_VECTOR, , NULL, NULL, NULL );
    //gdalwarp
    GDALVectorTranslateOptions *opt = GDALVectorTranslateOptionsNew( argv, NULL );
    GDALDataset *dst = ( GDALDataset * )GDALVectorTranslate( "out.gml", NULL, 1, 
                        &TestDs ,opt, &bUsageError );
    GDALVectorTranslateOptionsFree( opt );
    CSLDestroy( argv );
    //do something
    //clear env
    GDALClose(dst);
    GDALClose(TestDs);


