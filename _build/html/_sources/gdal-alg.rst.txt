.. highlight:: rst
.. _gdalalgs:

############################
GDAL算法简介
############################

主要来自 `GDAL RelatePages <https://www.gdal.org/pages.html>`_ 等 ,对部分翻译进行了少量修改和更新。暂时只介绍几种用过的算法,以后再加。

********************
GDAL网格插值
********************
主要来自 `GDAL Grid Tutorial <https://www.gdal.org/grid_tutorial.html>`_ 

想要了解插值算法可以看 `GDAL源码剖析（十三）之GDAL网格插值说明 <https://blog.csdn.net/liminlu0314/article/details/7654279>`_ ,基本是对GDAL文档的翻译

这里就直接介绍如何使用, ``GDAL`` 里网格插值算法有两大类,使用数据指标或者插值算法,数据指标用的是搜索椭圆内的点的数据指标来算,另一种就是根据离散点插值,有反距离权重等方式.

插值方式有三种:

    1. 直接使用函数 ``GDALGridCreate`` 创建
    2. 使用 ``GDALGridContextCreate`` ``GDALGridContextProcess`` 和 ``GDALGridContextFree`` ,使用重复参数批量生成插值时,这种方式效率较高
    3. 使用工具方式调用 ``GDALGrid`` 创建,详情见 :ref:`GDALUtilitiesWithCode` 部分,此处不再赘述
    
参数比较简单,就不翻译了,直接看代码:

方式一
=============

.. code-block:: c++

    std::vector<double> padfX;
    std::vector<double> padfY;
    std::vector<double> padfZ;
    //TODO: 添加x,y,z离散点,请自行修改
    ...
    //END
    //算最值
    std::vector<double>::iterator it = std::min_element(padfX.begin(), padfX.end());
    double  dfXMin = *it;
    it = std::max_element(padfX.begin(), padfX.end());
    double  dfXMax = *it;
    it = std::min_element(padfY.begin(), padfY.end());
    double  dfYMin = *it;
    it = std::max_element(padfY.begin(), padfY.end());
    double  dfYMax = *it;
    // 离散点内插方法
    //Linear 先构建Delaunay 三角网,然后用三角网插值,插值速度较快,速度仅与构网的离散点数量有关
    //无论是何种方式内插,都需要构建Options
    //插值算法有对应的Option结构体,数据指标(Data Metric)用的是统一的结构体 GDALGridDataMetricsOptions
    GDALGridLinearOptions  *poOptions = new GDALGridLinearOptions();
    poOptions->dfNoDataValue = -9999;
    poOptions->dfRadius = 0;
    
    //构建GRID
    double *pGridData  =  new double[nWidth*nHeight];
    GDALGridCreate(GGA_Linear ,poOptions,padfX.size(),&(padfX[0]),&(padfY[0]),&(padfZ[0]),\
        dfXMin, dfXMax, dfYMin, dfYMax, nWidth, nHeight, GDT_Float64, pGridData, NULL, NULL);
    delete poOptions;
    //处理生成的grid
    ...
    delete[] pGridData;

方式二
=============

.. code-block:: c++

    GDALGridLinearOptions  *poOptions = new GDALGridLinearOptions();
    poOptions->dfNoDataValue = -9999;
    poOptions->dfRadius = 0;
    //构建GRID
    double *pGridData  =  new double[nWidth*nHeight];
    GDALGridContext* context = GDALGridContextCreate(GGA_Linear,poOptions,padfX.size(),&(padfX[0]),&(padfY[0]),&(padfZ[0]),gwFALSE);
    GDALGridContextProcess(context,dfXMin, dfXMax, dfYMin, dfYMax,nWidth, nHeight, GDT_Float64,pGridData, NULL, NULL);
    GDALGridContextFree(context);
    //处理生成的grid
    ...
    delete[] pGridData;
    delete poOptions;


.. warning::

    * 2.2.3以下的版本中, 网格插值使用 ``GGA_Linear`` 方法可能会崩溃,在2.2.3中修复,可以直接修改源码,可按照
      `GDALGrid() with linear algorithm: avoid assertions/segmentation fault… <https://github.com/OSGeo/gdal/commit/ea2627dc5205e30d3463c64718c3bd147ce0b8c0#diff-62b56095c6ca3aa65387c3b8d95808f9>`_ 
      中的代码修改

    
****************************************
 GDAL栅格化
****************************************
主要来自 `gdal_alg.h File Reference <https://www.gdal.org/gdal__alg_8h.html>`_

栅格化三个函数类似, ``GDALRasterizeLayers  GDALRasterizeGeometries GDALRasterizeLayersBuf`` 需要注意的参数只有     ``padfGeomBurnValue`` 或者 ``dfBurnValue``
这个参数是矢量栅格化后的具体值,可以设置为栅格值

.. code-block:: c++

    GDALDataset* outDS;
    //创建outDS
    ...
    std::vector<OGRGeometryH> ahGeometries;
    std::vector<double> adfFullBurnValues;
    //加入各个几何形状和各个几何形状对应的值
    //adfFullBurnValues个数为对应ahGeometries乘以波段数,每个波段都需要设置值

    ahGeometries.push_back((OGRGeometryH)OgrPolygon);
    adfFullBurnValues.push_back(1);
    //几何变换参数,可以参照wrap
    void * hGenTransformArg=NULL;
    hGenTransformArg = GDALCreateGenImgProjTransformer( NULL,
        NULL,
        (GDALDatasetH)outDS,
        outDS->GetProjectionRef(),
        false, 0, 3 );
    CPLErr err = GDALRasterizeGeometries((GDALDatasetH)outDS,
                                        1,
                                        pnbandlist,
                                        ahGeometries.size(),&(ahGeometries[0]),
                                        GDALGenImgProjTransform,
                                        hGenTransformArg,
                                        &(adfFullBurnValues[0]),
                                        papszOptions, NULL, NULL);
    GDALDestroyGenImgProjTransformer(hGenTransformArg);
    GDALClose(outDS);

    delete[]pnbandlist;
    delete[]dburnValues;