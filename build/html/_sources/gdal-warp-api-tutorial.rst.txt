.. highlight:: rst
.. _GDALWarp:

############################
GDALWarp
############################
本章来源：

* `GDAL源码剖析（十二）之GDAL Warp API使用说明 <http://blog.csdn.net/liminlu0314/article/details/7603162>`_
* `GDALWarpOptions Struct Reference <http://www.gdal.org/structGDALWarpOptions.html#abf726bf4ef0927713370722ee34c6cc4>`_
* `GDAL_Algorithms_C_API <http://www.gdal.org/gdal__alg_8h.html>`_

顺序重新调整,增加GDAL算法介绍，本章是进阶章节，阅读前请务必先熟悉 :ref:`gdaldatamodel` 与 :ref:`read&write` 章节。

********************
GDALWarp简介
********************
GDAL Warp API（在文件 ``gdalwarper.h`` 中定义）是一个高效的进行图像变换的接口。主要由几何变换函数（ ``GDALTransformerFunc`` ），多种图像重采样方式，掩码操作选项等组成。这个接口可以对很大的图像进行处理。

使用步骤如下：

* 在程序中，首先要初始化一个 ``GDALWarpOptions``  结构体的对象
* 然后使用 ``GDALWarpOptions`` 的对象来初始化 ``GDALWarpOperation`` 的对象
* 最后通过调用 ``GDALWarpKernel`` 类里面的 ``GDALWarpOperation::ChunkAndWarpImage()`` 或  ``GDALWarpOperation::ChunkAndWarpMulti()`` 函数来完成图像的变换

下面先介绍 ``GDALWarpOptions`` 和 ``GDALTransformerFunc`` ，然后对完整流程进行解析。

.. _GDALWarpOptions:

**********************************
GDALWarpOptions选项介绍
**********************************
``GDALWarpOptions`` 是对图像变换进行设置的类，成员见下：

.. cpp:class:: GDALWarpOptions

    GDALWarpOptions结构体中包含了很多参数来对变换进行设置。下面对一些比较重要的进行列举说明：

.. cpp:member:: char **     papszWarpOptions

    这个是一个字符串列表，用来设置图像变换过程中的一些选项，样式为 ``NAME=VALUE`` , 可以使用 ``CSLSetNameValue()`` 设置。现在支持的一些键值对设置为：

    * ``INIT_DEST=[value]`` 或者 ``INIT_DEST=NO_DATA`` ：这个选项用来强制设置结果图像的初始值（所有的波段），初始值为指定的value，或者NODATA值。NODATA值从参数padfDstNoDataReal或者参数padfDstNoDataImag中获取。如果这个值没有设置，那么将会使用原始图像的NODATA值来覆盖。
    * ``WRITE_FLUSH=YES/NO`` ：这个选项用来强制设置在处理完每一块后将数据写入磁盘中。在某些时候，这个选项可以更加安全的写入结果数据，但是同时会增加更多的磁盘操作。目前这个默认值为NO。
    * ``SKIP_NOSOURCE=YES/NO`` :跳过没有相应输入的数据块的处理。这将禁止初始化要写入的数据块以及其他所有处理，要谨慎使用。常用于影像拼接、镶嵌情况下，减少额外工作。
    * ``UNIFIED_SRC_NODATA=YES/[NO]`` :默认情况下，每个波段的 ``nodata`` 掩膜数据是独立的。但有些时候所有波段的 ``nodata`` 值一致情况下，可以将此参数设置为YES。

    通常在计算一个数据的部分区域时，变换样区在输出区域每个边界上转换21个点到源文件中，然后在源图中计算一个足够大的窗口。这样做是为了使转换更加高效，但某些情况下，这种做法计算的窗口过小，甚至丢失大量的区域。特别是对于非线性或者是翻转的转换。例如对极地点附近的区域，从WGS84转到Polar Stereographic投影转换，或者某些根本无法进行投影转换的数据。对于这种情况，GDAL提供以下键值对设置来防止窗口计算错误：
    
    * ``SAMPLE_GRID=YES/NO`` : 如果设置为 ``YES`` ,将强制采样样区边缘点和中心点。对于变形很大的投影或者是大部分数据变换后不在源图像上的情况非常适用。
    * ``SAMPLE_STEPS`` : 调整采样密度，默认为21，增加可以增强精确度，但是会降低计算效率。
    * ``SOURCE_EXTRA`` : 该数值是边界增加的像素，默认为1，保证边界不出错，设置更大的画，将增加读取数量，但是可以避免丢失源数据。
    * ``CUTLINE`` : 裁切线，使用wkt表示。与在 ``GDALWarpOptions hCutline`` 中设置相同，在 ``GDALWarpOperation::Initialize()``  调用时将被设置到 ``hCutline`` 属性中。与  ``gdalwarp`` 工具不同，坐标系为图像的坐标系。
    * ``CUTLINE_BLEND_DIST`` : 与 ``dfCutlineBlendDist`` 属性相同
    * ``CUTLINE_ALL_TOUCHED`` : 默认为  ``FALSE``  ，但是可以设置为  ``TURE`` ，保证与裁切线相交的像素会被裁剪，而不仅仅是完全被包含的像素。
    * ``OPTIMIZE_SIZE`` : 默认为  ``FALSE``  ，设置为  ``TRUE`` 时，在压缩格式中将获取更小的文件，可能导致转换变慢。
    * ``NUM_THREADS`` : (GDAL >= 1.10) 设置为一个数值，或者是 ``ALL_CPUS`` 可以设置多线程，使用并行化处理图像变换。没有设置默认为单线程。

.. cpp:member:: double     dfWarpMemoryLimit

    设置GDALWarpOperation在处理图像中使用的最大内存数。单位为比特，默认值比较保守（比较小），可以根据自己的内存大小来进行调整这个值。增加这个值可以帮助提高程序的运行效率，但是需要注意内存的大小。这个大小、GDAL的缓存大小，还有你的应用程序以及系统所需要的内存加起来要小于系统的内存，否则会导致错误。一般来说，比如一个内存为256MB的系统，这个值最少设置为64MB比较合理。注意，这个值不包括GDAL读取数据使用的内存。
    
.. cpp:member:: GDALResampleAlg     eResampleAlg

    重采样的算法选择：

    * GRA_NearestNeighbour ：最邻近插值 
    * GRA_Bilinear ：双线性插值 (2x2 kernel)
    * GRA_Cubic ：立方卷积逼近 (4x4 kernel)
    * GRA_CubicSpline：三次样条线逼近(4x4 kernel)
    * GRA_Lanczos ：Lanczos windowed sinc 插值 (6x6 kernel)
    * GRA_Average ：均值 (计算样区内非空值的所有像素均值)
    * GRA_Mode ：众数 (出现频数最高的值)

.. cpp:member:: GDALDataType     eWorkingDataType

    计算时用的数据类型，GDT_Unknown 为程序自动判断
    
.. cpp:member:: GDALDatasetH     hSrcDS

    源数据集
    
.. cpp:member:: GDALDatasetH     hDstDS

    目标数据集
    
.. cpp:member:: int     nBandCount

    所需转换的波段数，0为所有波段
    
.. cpp:member:: int *     panSrcBands

    int数组，源图像中所需处理的波段，从1开始
    
.. cpp:member:: int *     panDstBands

    int数组，输出的波段
    
.. cpp:member:: int     nSrcAlphaBand

    源数据中的Alpha波段，0为禁止
    
.. cpp:member:: int     nDstAlphaBand

    目标数据中的Alpha波段，0为禁止
    
.. cpp:member:: double *     padfSrcNoDataReal

    源数据中 nodata 的实部，NULL表示没有
    
.. cpp:member:: double *     padfSrcNoDataImag

    源数据中 nodata 的虚部，NULL表示没有
    
.. cpp:member:: double *     padfDstNoDataReal

    目标数据中 nodata 的实部，NULL表示没有
    
.. cpp:member:: double *     padfDstNoDataImag

    目标数据中 nodata 的虚部，NULL表示没有
    
.. cpp:member:: GDALProgressFunc     pfnProgress

    进度条显示函数，NULL表示没有
    
.. cpp:member:: void *     pProgressArg

    传给pfnProgress的回调参数
    
.. cpp:member:: GDALTransformerFunc     pfnTransformer

    几何变换函数
    
.. cpp:member:: void *     pTransformerArg

    几何变换函数参数
    
.. cpp:member:: void *     hCutline

    裁切线，OGRPolygonH 类型
    
.. cpp:member:: double     dfCutlineBlendDist

    切割线宽容度，默认为零
    
**********************************
几何变换函数简介
**********************************
`GDALTransformerFunc <http://www.gdal.org/gdal__alg_8h.html#a7df61123fec15deb3da3acabce19e647>`_  是一个函数签名，用来计算空间点几何变换，也是 ``GDALWarp`` 的核心部分，定义在GDAL算法文件中。 ``GDAL`` 中可用的算法有：

* ``GDALApproxTransform``  近似变换
* ``GDALGCPTransform`` GCP投影变换
* ``GDALGenImgProjTransform`` 图像几何变换
* ``GDALReprojectionTransform`` 重投影变换
* ``GDALTPSTransform`` TPS变换
* ``GDALRPCTransform`` `RPC <http://trac.osgeo.org/gdal/wiki/rfc22_rpc>`_ 变换
* ``GDALGeoLocTransform`` `GeoLoc <http://trac.osgeo.org/gdal/wiki/rfc4_geolocate>`_ 变换

与 ``GDALTransformerFunc`` 对应，默认有几个函数生成几何变换函数参数：

* ``GDALCreateApproxTransformer`` 对应生成近似变换参数
* ``GDALCreateGCPTransformer`` 对应生成GCP投影变换参数
* ``GDALCreateGenImgProjTransformer``  对应生成图像几何变换参数
* ``GDALCreateGenImgProjTransformer2``  对应生成图像几何变换参数
* ``GDALCreateGenImgProjTransformer3``  对应生成图像几何变换参数
* ``GDALCreateReprojectionTransformer`` 对应生成重投影变换参数
* ``GDALCreateTPSTransformer`` 对应生成TPS变换参数
* ``GDALCreateRPCTransformer`` 对应生成  `RPC <http://trac.osgeo.org/gdal/wiki/rfc22_rpc>`_ 变换投影变换参数
* ``GDALCreateGeoLocTransformer`` 对应生成 `GeoLoc <http://trac.osgeo.org/gdal/wiki/rfc4_geolocate>`_ 投影变换参数

这部分看起来比较复杂，实际使用时，比较简便。只需要在 ``GDALWarpOptions->pfnTransformer`` 中指定算法名称， ``GDALWarpOptions->pTransformerArg`` 中指定对应的算法参数函数即可，如下代码所示：

.. code-block:: c++

    psWarpOptions->pfnTransformer = GDALGenImgProjTransform;
    psWarpOptions->pTransformerArg =
        GDALCreateGenImgProjTransformer(pSrcDs,
                                        sFromWkt,
                                        pDstDs,
                                        sToWkt,
                                        FALSE, 0.0, 3);

每个Transformer的所需参数不同，请参见 `GDAL Algorithms C API <http://www.gdal.org/gdal__alg_8h.html#a7671696d085085a0bfba3c3df9ffcc0a>`_ ，近似计算有些特殊，在使用Transformer时要传递几何变换函数，参考 :ref:`optimization` 部分最后一条。

**********************************
完整流程
**********************************
下面介绍使用 ``GDALWarp`` 提供的接口，实现几何变换的完整流程：

创建输出文件
====================
几何变换后输出文件的范围和投影仿射参数都将发生变化，输出的文件大小可能是已知，也可能是未知的。因此创建输出文件方式也有两种：

* 已知的例如重采样或者裁剪，我们知道输出文件大小，需要自己计算文件大小
* 未知例如投影，我们GDAL中提供计算函数，我们将使用 ``GDALSuggestedWarpOutput`` 函数预测输出文件大小和仿射变换，使用 ``GDALDriver->Create()`` 函数创建输出文件

重采样创建文件示例：

.. code-block:: c++

    GDALAllRegister();
    //打开输入文件
    GDALDataset *pDSrc = (GDALDataset *)GDALOpen(InFile, GA_ReadOnly);

    if(pDSrc == NULL) {
        return -1;
    }
    //创建Driver
    GDALDriver *pDriver = GetGDALDriverManager()->GetDriverByName("GTiff");

    if(pDriver == NULL) {
        GDALClose((GDALDatasetH) pDSrc);
        return -2;
    }
    //获取源数据中的投影/波段数/仿射变换等信息
    int inBandCount = pDSrc->GetRasterCount();
    const char* strWkt = pDSrc->GetProjectionRef();
    GDALDataType dataType = pDSrc->GetRasterBand(1)->GetRasterDataType();
    double geos[6] = {0};
    pDSrc->GetGeoTransform(geos);
    //获取源数据中左上角点地理坐标Img2CoordX/Img2CoordY为图片到投影坐标转换函数
    double lefttopx = Img2CoordX(geos,0,0);
    double lefttopy = Img2CoordY(geos,0,0);
    //计算新高度和宽度,以及投影信息
    int newWidth =(int) pDSrc->GetRasterXSize()*geos[1]/fResX;
    int newHeight =(int) fabs(pDSrc->GetRasterYSize()*geos[5]/fResY);
    geos[0] = lefttopx;
    geos[1] = fResX;
    geos[3] = lefttopy;
    geos[5] = -fResY;
    int *pSrcBand = NULL;
    int *pDstBand = NULL;
    int iBandSize = 0;
    //创建输出文件
    GDALDataset *pDDst = pDriver->Create(OutFile, 
                                         newWidth, 
                                         newHeight, 
                                         iBandSize, 
                                         dataType, 
                                         NULL);
    pDDst->SetProjection(strWkt);
    pDDst->SetGeoTransform(geos);


投影变换创建文件示例：

.. code-block:: c++

    //打开文件
    GDALAllRegister();
    GDALDataset *pProjDS = (GDALDataset *)GDALOpen(ProjFile, GA_ReadOnly);

    if(pProjDS == NULL) {
        return -2;   //打开坐标文件失败
    }
    const char *sToWkt = ...//输出的投影

    //输出文件
    GDALDataset *pDstDs;
    //创建Driver
    GDALDriver* pDriver = (GDALDriver*)GDALGetDriverByName("GTiff");

    if(pDriver == NULL) {
        return -3;   //注册driver失败
    }

    //计算输出投影参数
    
    const char *sFromWkt = GDALGetProjectionRef(pProjDS);
    CPLAssert(sFromWkt != NULL &&strlen(sFromWkt) > 0);
    
    //计算变换参数
    void *pTransformArg;
    pTransformArg = GDALCreateGenImgProjTransformer(pProjDS,
                                                    sFromWkt, 
                                                    NULL, 
                                                    sToWkt,    
                                                    FALSE,
                                                    0, 
                                                    3);
    CPLAssert(pTransformArg != NULL);
    double geos[6];
    int nPixels=0, nLines=0;
    CPLErr eErr;
    eErr = GDALSuggestedWarpOutput(pProjDS,
                                   GDALGenImgProjTransform, 
                                   pTransformArg,
                                   geos, 
                                   &nPixels, 
                                   &nLines);
    CPLAssert(eErr == CE_None);
    GDALDestroyGenImgProjTransformer(pTransformArg);
    //输出数据类型
    GDALDataType eDataType= GDALGetRasterDataType(GDALGetRasterBand(pProjDS,1));
    //计算创建波段数量
    int nCreateBandCount = pProjDS->GetRasterCount();

    //创建文件
    pDstDs = (GDALDataset*)GDALCreate(pDriver,
                                      sOutFileName, 
                                      nPixels, 
                                      nLines,
                                      nCreateBandCount,
                                      eDataType, 
                                      NULL);
    GDALSetProjection(pDstDs, sToWkt);
    GDALSetGeoTransform(pDstDs, geos);



设置选项
====================
选项设置参见 :ref:`GDALWarpOptions` ，设置一般需要设置输入输出数据集，需要操作的波段，几何变换函数和参数，进度条，优化参数这几项。示例如下：

.. code-block:: c++

    //warp 选项
    GDALWarpOptions *psWarpOptions = GDALCreateWarpOptions();
    //源
    psWarpOptions->hSrcDS = pSrcDs;
    psWarpOptions->hDstDS = pDstDs;
    //优化选项
    psWarpOptions->dfWarpMemoryLimit = 512000000;
    char** papszWarpOptions=NULL;
    papszWarpOptions = CSLSetNameValue(papszWarpOptions,"NUM_THREADS","ALL_CPUS");
    papszWarpOptions = CSLSetNameValue(papszWarpOptions,"WRITE_FLUSH","YES");
    psWarpOptions->papszWarpOptions=papszWarpOptions;
    //源和输出对应的波段
    psWarpOptions->nBandCount = nCreateBandCount;
    psWarpOptions->panSrcBands = (int *) CPLMalloc(nCreateBandCount*sizeof(int));
    psWarpOptions->panDstBands = (int *) CPLMalloc(nCreateBandCount*sizeof(int));
    int i;

    for(i=0; i<nCreateBandCount; i++) {
        psWarpOptions->panSrcBands[i] = pSrcBand[i];
        psWarpOptions->panDstBands[i] = pDstBand[i];
    }

    delete[] pSrcBand;
    delete[] pDstBand;
    //进度条
    psWarpOptions->pfnProgress = GDALTermProgress;
    // 几何变换函数和参数
    psWarpOptions->pfnTransformer = GDALGenImgProjTransform;
    psWarpOptions->pTransformerArg =
        GDALCreateGenImgProjTransformer(pSrcDs,
                                        sFromWkt,
                                        pDstDs,
                                        sToWkt,
                                        FALSE, 0.0, 3);


执行变换
====================
执行较为简单，新建 ``GDALWarpOperation`` ,调用 ``Initialize`` 函数，然后调用 ``ChunkAndWarpMulti`` 或者 ``ChunkAndWarpImage`` ，代码一般如下：

.. code-block:: c++

    GDALWarpOperation opt;
    if(opt.Initialize(psWarpOptions)!=CE_None) {
        GDALClose(pSrcDs);
        GDALClose(outDs);
        return -1;
    }
    opt.ChunkAndWarpMulti(0, 0, CutWidth, CutHeight);


清理环境
====================
完成后，需要释放变换参数、删除数据集。代码如下：

.. code-block:: c++

    GDALDestroyGenImgProjTransformer(psWo->pTransformerArg);
    GDALDestroyWarpOptions(psWo);
    GDALClose((GDALDatasetH) pDSrc);
    GDALClose((GDALDatasetH) pDDst);
    delete[] pSrcBand;
    delete[] pDstBand;

完整代码
====================
完整的重采样代码如下：

.. literalinclude:: code/gdalwarpsimple.c
    :language: c++
    :encoding: utf8

.. _optimization:

**********************************
性能优化
**********************************
下面几点可以在使用变换API的时候提高处理效率。

*  增加变换API使用的内存，可以使程序执行的更快。这个参数叫GDALWarpOptions::dfWarpMemoryLimit。理论上，越大的块对于数据I/O效率越高，并且变换的效率也会越高。然而，变换所需的内存和GDAL缓存应该小于机器的内存，最多是内存的2/3左右。
*  增加GDAL的缓存。这个尤其对于在处理非常大的输入图像很有用。如果所有的输入输出图像的数据频繁的读取会极大的降低运行效率。使用函数GDALSetCacheMax()来设置GDAL使用的最大缓存。
*  当写入一个空的输出文件，在GDALWarpOptions::papszWarpOptions 对象中使用INIT_DEST选项来进行设置。这样可以很大的减少磁盘的IO操作。
*  输入和输出文件使用分块存储的文件格式。分块文件格式可以快速的访问一块数据，相比来说，普通的大数据量的顺序存储文件格式在IO操作中要花费更多的时间。
*  在一次调用中处理所有的波段。一次处理所有的波段比每次处理一个波段要保险。
*  使用GDALWarpOperation::ChunkAndWarpMulti()方法来代替GDALWarpOperation::ChunkAndWarpImage()方法。这个使用多个独立的线程来进行IO操作和变换影像处理，能够提高CPU和IO的效率。执行这个操作，GDAL需要多线程的支持（从GDAL1.8.0开始，默认在Win32平台、Unix平台是支持的，对于之前的版本，需要在配置中进行修改才行）。
*  重采样方式，最邻近象元执行速度最快，双线性内插次之，三次立方卷次最慢。不要使用根据复杂的重采样函数。
*  避免使用复杂的掩码选项。比如，最邻近采样在处理没有掩码的8bit数据要比一般的效率高很多。
*  使用近似的变换代替精确的变换（精确的变换是每个象元都会计算一次）。下面的代码用来说明近似变换的使用方式，近似变换要求指定一个变换的误差dfErrorThreshold，这个误差单位是输出图像的象元个数

.. code-block:: c++
 
    hTransformArg =  GDALCreateApproxTransformer(GDALGenImgProjTransform,  
                                                 hGenImgProjArg, 
                                                 dfErrorThreshold );  
    pfnTransformer = GDALApproxTransform;  

