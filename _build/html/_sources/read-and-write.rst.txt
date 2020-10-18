.. highlight:: rst
.. _read&write:

####################
栅格数据读写
####################

本章我们就开始按顺序详细介绍GDAL读写栅格数据的过程。最后将提供完整的读写流程代码。 :ref:`fullcode` 中有整个创建、两影像相加、写入的流程，如果已经大致了解GDAL的读写流程，可以直接参照 :ref:`fullcode` 。

.. _gdaldriver:

******************
GdalDriver
******************
首先，GDAL对每种格式提供了一个驱动 ``GdalDriver`` ， ``GdalDriver`` 将对对应格式的数据进行管理，例如读取、创建、删除、重命名、复制、从已有数据创建新数据集等。所以，我们所有程序开头，都将添加 ``GDALAllRegister()`` 函数，注册所有GDAL支持的数据驱动。

**再次强调，所有的程序都要首先调用GDALAllRegister() 函数** ，否则将无法打开任何数据。

.. _gdalread:

******************
数据读取
******************

GDAL中数据读取的步骤如下：

* 打开数据集
* 打开数据集下所需的波段
* 读取数据

下面分步讲解如何操作：

打开数据集
================
打开 :ref:`multibands` 步骤如下：使用 ``GDALOpen`` 或者 ``GDALOpenShared`` 函数，传入文件名，打开数据集。 ``GDALOpen`` 和 ``GDALOpenShared`` 参数一样，区别在于，在同一线程，如果是相同的文件，多个 ``GDALOpenShared`` 打开的其实是同一个 ``GDALDataset`` 的引用（如果在不同线程使用，为了保证线程安全，返回的将是不同的对象）。

.. code-block:: c++

    #include "gdal_priv.h"
    #include "cpl_conv.h" 

    //...中间的程序

    //所有程序前先加上
    GDALAllRegister();
    //pszFilename代表文件名，GA_ReadOnly表示以只读方式打开
    //也可以使用GA_Update
    //GDALOpenShared和GDALOpen可以互换
    GDALDataset  *poDataset= (GDALDataset *) GDALOpen( pszFilename, GA_ReadOnly );
    if( poDataset == NULL )
    {
        ...;//打开失败处理
    }

打开 :ref:`multidatasets` 方式与上面稍有区别，因为有多个子数据集，所以需要获取真实数据的话，实际上是获取子数据集中的波段，可以使用 :ref:`gdalinfo` 获取如下的 :ref:`subdatasetsdomain` ::

    Subdatasets:
        SUBDATASET_1_NAME=NETCDF:"example.nc":auditTrail
        SUBDATASET_1_DESC=[2x80] auditTrail (8-bit character)
        SUBDATASET_2_NAME=NETCDF:"example.nc":data
        SUBDATASET_2_DESC=[1x61x172] data (32-bit floating-point)
        SUBDATASET_3_NAME=NETCDF:"example.nc":lat
        SUBDATASET_3_DESC=[61x172] lat (32-bit floating-point)
        SUBDATASET_4_NAME=NETCDF:"example.nc":lon
        SUBDATASET_4_DESC=[61x172] lon (32-bit floating-point)

打开子数据集也使用 ``GDALOpen`` 或者 ``GDALOpenShared`` 函数，但是传入的不是整个数据集的名称，而是需要打开的 ``SUBDATASET_N_NAME=`` (_N_中N为第几个子数据集，即 ``1 2 3 4...``)后面的字符串，即子数据集名称。例如需要打开上例中data子数据集，使用如下代码：

.. code-block:: c++

    #include "gdal_priv.h"
    #include "cpl_conv.h" 

    //...中间的程序

    //所有程序前先加上
    GDALAllRegister();
    //也可以使用GA_Update
    //GDALOpenShared和GDALOpen可以互换
    //pszFilename代表子数据集的NAME，注意双引号需要转义，GA_ReadOnly表示以只读方式打开
    char * pszFilename = "NETCDF:\"example.nc\":data";
    GDALDataset  *poDataset= (GDALDataset *) GDALOpen( pszFilename, GA_ReadOnly );
    if( poDataset == NULL )
    {
        ...;//打开失败处理
    }

打开了数据集后，两种数据接下来的处理方式都是一样的。

打开波段
================
GDAL中， **波段起始索引为1** ，所以循环时需要特别注意。使用 ``GDALDataset->GetRasterBand(int BandIndex)`` 函数，可以打开波段。

.. code-block:: c++

    #include "gdal_priv.h"
    #include "cpl_conv.h" 
    //...中间的程序

    //所有程序前先加上
    GDALAllRegister();
    GDALDataset  *poDataset= (GDALDataset *) GDALOpen( pszFilename, GA_ReadOnly );//打开文件
    if( poDataset == NULL )
    {
        //...//打开失败处理
    }
    //打开数据集同上
    //获取影像相关信息
    int i,j,width,height,bandNum;
    width = inDS->GetRasterXSize();//影像宽度
    height = inDS->GetRasterYSize();//影像高度
    bandNum = inDS->GetRasterCount();//影像波段数量

    //将第一波段读入
    double *data = new double[width*height];
    GDALRasterBand *band = ds1->GetRasterBand(1);//获取第一波段，波段从1开始
    //如果是在循环内，需要注意，波段从1开始；
    //for(i=0;i < bandNum;i++){
    // GDALRasterBand *band = ds1->GetRasterBand(i+1);//这里注意下
    // ......对波段处理
    //}
    
读取内容
================
使用 ``RasterIO`` 函数获取波段中的具体内容，该函数的功能强大，参数较多,我们先用代码演示,然后用图来表示。

函数说明：

.. code-block:: c++

    ///CPLErr GDALRasterBand::RasterIO (   
    ///@param eRWFlag,     //读取或者写入,GF_Read或GF_Write
    ///@param nXOff,       //起始点x坐标
    ///@param nYOff,       //起始点y坐标
    ///@param nXSize,      //所需读取(写入)块宽度
    ///@param nYSize,      //所读(写)块高度
    ///@param * pData,     //所读(写)数据,指针,
    ///@param nBufXSize,   //一般跟nXSize一致，用于缩放图像，
    ///                      图像将按nBufXSize/nXsize在x尺度缩放（会自动重采样）
    ///                     ，一般不需要调整
    ///@param nBufYSize,   //一般跟nYSize一致
    ///@param eBufType,    //与pData的实际类型一致,GDT_Float64
    ///                      代表double,其他的可以跳到定义查看  
    ///@param nPixelSpace,  
    ///                      设置为0为自动判断，一般设为0
    ///                      表示的是当前像素值和下一个像素值之间的间隔，单位是字节
    ///                      例:byte类型，就是1，double类型，就是8
    ///@param nLineSpace    
    ///                      设置为0为自动判断，一般设为0
    ///                      表示的是当前行和下一行的间隔，，单位是字节
    ///                      例如，一行300像素，类型为int,此时就是300*4 = 1200
    ///@return             //是否成功,成功返回CE_None ,失败返回 CE_Failure 
    CPLErr GDALRasterBand::RasterIO (   
        GDALRWFlag eRWFlag, 
        int     nXOff, 
        int     nYOff, 
        int     nXSize,          
        int     nYSize,   
        void * pData,    
        int     nBufXSize,           
        int     nBufYSize,   
        GDALDataType    eBufType,
        int     nPixelSpace,      
        int     nLineSpace    
    ) 

示意图：

.. figure:: img/RasterIO.png
   :alt: multiBands Image
   :align: center


读取单幅影像第一波段原始数据代码示例：

.. code-block:: c++

    #include "gdal_priv.h"
    #include "cpl_conv.h" 
    //...中间的程序

    //所有程序前先加上
    GDALAllRegister();
    GDALDataset  *poDataset= (GDALDataset *) GDALOpen( pszFilename, GA_ReadOnly );//打开文件
    if( poDataset == NULL )
    {
        //...打开失败处理
    }
    //打开数据集同上
    //获取影像相关信息
    int i,j,width,height,bandNum;
    width = inDS->GetRasterXSize();//影像宽度
    height = inDS->GetRasterYSize();//影像高度
    bandNum = inDS->GetRasterCount();//影像波段数量

    //将第一波段读入
    double *data = new double[width*height];
    GDALRasterBand *band = ds1->GetRasterBand(1);//获取第一波段，波段从1开始
    band->RasterIO(GF_Read,0,0,width,height,data,width,height,GDT_Float64,0,0);

.. warning::

    使用RasterIO函数时需要注意以下几点：
    
    * 波段索引从一开始计算
    * pData数组要有足够的大小
    * pData数组数据类型要和eBufType对应
    * 读取完成后必须要释放数组和关闭数据集，波段不需要关闭

关闭数据集
================
程序结束前，必须要关闭数据集，使用 ``GDALClose()`` 函数。

.. code-block:: c++

    GDALClose(ds2);

.. _write:

******************
数据写入
******************
数据写入分为几个步骤

* 获取驱动
* 创建数据集
* 写入数据
* 关闭数据集

获取驱动
========================
:ref:`gdaldriver` 中已经说明，不论读取还是写入， ``GDALDriver`` 都是很重要的存在。如果需要输出特定格式，我们必须首先获取该格式的驱动，才能创建该格式的数据集。获取方式如下：

.. code-block:: c++

    GDALDriver *poDriver;
    
    //一般使用tif作为输出，如果有特殊需求，请参阅gdal文档，一般修改这里改为其他驱动名称即可，例如PNG
    const char *pszFormat ="GTiff";
    poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);//获取特殊的驱动。
    if(poDriver == NULL) {
        return;
    }


.. _createdataset:

创建数据集
========================
根据不同输出需要，创建数据集有两种方式：

* 输出的影像与输入的影像大小、投影都相同，只有数据不同，就可以从已经读入的数据集中创建
* 投影或大小不同，需要创建新的数据集

下面分别说明两种方式如何创建数据集，下文都是以输出GeoTiff作为示例，不同的 ``GdalDriver`` 中，可以设置的参数不同，具体请参考 `GDAL支持格式 <http://www.gdal.org/formats_list.html>`_ 中对每种格式的说明。

从已读入的数据集中创建
--------------------------------
从已经读入的数据集中创建数据集，可以使用 ``GDALDriver`` 的 ``CreateCopy`` 函数。

``CreateCopy`` 函数原型如下:

.. code-block:: c++

    //从已有的dataset中创建
    //@param pszFilename, 输出文件名
    //@param poSrcDS, 已有的dataset
    //@param bStrict, TRUE表示严格等价，一般设置为FALSE，表示拷贝副本用作编辑
    //@param papszOptions, 参数设置，具体可以参考每个driver的文档
    //@param  pfnProgress 回调函数指针
    //@param *pProgressData 传入回调函数的数据
    GDALDataset * GDALDriver::CreateCopy     (
        const char *     pszFilename, 
        GDALDataset *     poSrcDS, 
        int     bStrict, 
        char **     papszOptions, 
        GDALProgressFunc     pfnProgress, 
        void *     pProgressData    
    )
    

使用 ``CreateCopy`` 函数创建数据集代码如下：

.. code-block:: c++

    GDALDataset *OutDs;
    char **papszOptions = NULL;//设置压缩、存储方式等，每种格式不同，可以直接设置为NULL
                               //也可以根据需要设置,例如：
    //papszOptions = CSLSetNameValue( papszOptions, "TILED", "YES" );
    //papszOptions = CSLSetNameValue( papszOptions, "COMPRESS", "PACKBITS" );
    
    //创建与ds1相同的坐标信息、相同大小的文件
    OutDs = poDriver->CreateCopy(OutPut1,ds1,FALSE,papszOptions,NULL,NULL);


直接创建
----------------------------------
直接创建数据集比从已有数据集中创建会多出创建投影的步骤：

创建数据集
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
直接集中创建数据集，可以使用 ``GDALDriver`` 的 ``Create`` 函数。

``Create`` 函数原型如下:

.. code-block:: c++

    //Create文件
    //@param pszFilename, 输出文件名
    //@param nXSize, 文件宽度
    //@param nYSize, 文件高度
    //@param nBands, 波段数
    //@param eType, 数据类型
    //@param papszOptions     参数设置，具体可以设置的属性参考每个driver的文档
    //@return dataset
    GDALDataset * GDALDriver::Create ( 
        const char *     pszFilename, 
        int     nXSize, 
        int     nYSize, 
        int     nBands, 
        GDALDataType     eType, 
        char **     papszOptions 
    )


使用 ``Create`` 函数创建数据集代码如下：

.. code-block:: c++

    GDALDataset *OutDs;
    ///设置：
    char **papszOptions = NULL;
    papszOptions = CSLSetNameValue( papszOptions, "TILED", "YES" );
    papszOptions = CSLSetNameValue( papszOptions, "COMPRESS", "PACKBITS" );
    ///...
    ///创建512*512，单波段的，byte类型的数据
    OutDs = poDriver->Create( pszDstFilename, 512, 512, 1, GDT_Byte, 
                                papszOptions );
                                
创建投影
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
在 :ref:`coordinatesystem` 和 :ref:`affinegeotransform` 中已经介绍过GDAL中数据如何投影，下面我们将用代码示例：

.. code-block:: c++

    ///设置仿射变换参数
    double adfGeoTransform[6] = { 444720, 30, 0, 3751320, 0, -30 };
    OutDs->SetGeoTransform( adfGeoTransform );

    //设置投影
    OGRSpatialReference oSRS;
    char *pszSRS_WKT = NULL;
    oSRS.SetUTM( 11, TRUE );
    oSRS.SetWellKnownGeogCS( "NAD27" );
    oSRS.exportToWkt( &pszSRS_WKT );
    poDstDS->SetProjection( pszSRS_WKT );
    CPLFree( pszSRS_WKT );//使用完后释放


写入数据
=========================
写入数据与读取数据基本相同，都是使用 ``GDALRasterBand`` 的 ``RasterIO`` 函数，只是第一个参数变为 ``GF_Write`` ，下面就直接给出代码，不再另作说明。

.. code-block:: c++

    float * outData = new float [512*512];

    //对outData进行计算、处理
    /************************************************************************/
    /*********************************处理************************************/
    /************************************************************************/
    
    GDALRasterBand *outBand = OutDs->GetRasterBand(1);
    outBand->RasterIO(GF_Write,0,0,512,512,outData,512,512,GDT_Float32,0,0);


关闭数据集
=========================
数据集必须关闭后，数据才会写入到输出文件中，否则数据将在缓存中，使用 ``GDALClose()`` 函数关闭数据集。

.. code-block:: c++

    GDALClose(OutDs);
    
    

分块读写
=======================
数据较大时,分块读写将提高效率,实际上大部分的图像格式中包含分块内容,一般是以行分块为主,所以块宽度为行宽可能会提高分块读取速度

.. code-block:: c++

    GDALAllRegister();//注册类型，打开影像必须加入此句
    GDALDataset *ds1; 
    ds1 = (GDALDataset *) GDALOpen(input1,GA_ReadOnly);//input1为文件名
    if(ds1 == NULL) {  //读取失败
        AfxMessageBox("cant open the unReferard image!");
        return ;
    }
     
    WidthAll = ds1->GetRasterXSize(); //影像宽度
    HeightAll = ds1->GetRasterYSize();//影像高度
    BandNum = ds1->GetRasterCount();//影像波段数
    //读取波段数据
    GDALRasterBand *poBand;//不需要释放,只需要最后是否GDALDataset
    double **Inputimg1 = new double*[BandNum];
     
    int blockx = 512;//分块大小
    int blocky = 512;
    //分块处理.将影像分成很多blockx*blocky大小的块，通过循环对每一块进行处理
    int nxNum = WidthAll/blockx+1;//计算列方向上块数
    int nyNum = HeightAll/blocky+1;//计算行方向块数
     
    int i,j,k;
    int Width,Height;//块的实际宽和高
    for (i = 0; i < nxNum; ++i) {
        for ( j = 0; j < nyNum; ++j) {
            //确定实际块大小
            Width = (WidthAll - (i+1)*blockx) > 0? blockx : (WidthAll - i * blockx);
            Height = (HeightAll - (j+1)*blocky) > 0? blocky : (HeightAll - j * blocky);
            //分块读取数据
            for ( k = 0; k < BandNum; ++k) {
                Inputimg1[j] = new double[Width*Height];
                //注意,获取波段数从1开始计数!!
                poBand = ds1->GetRasterBand(k+1);
                //将数据写入Inputimg1[j],RasterIOd中，参数具体含义见RasterIO：
                poBand->RasterIO(GF_Read,i*blockx, j*blocky, Width, Height,Inputimg1[j], \
                                 Width,Height,GDT_Float64,0,0);
            }
            /**
            * 处理
            * 处理
            * 处理
            */
        }	
    }
     
    //用完之后,关闭dataset即可,不需要释放GDALRasterBand
    GDALClose(ds1);

.. _gdal2:

******************
GDAL2.0
******************
GDAL2.0版本中,RasterIO添加了 ``GDALRasterIOExtraArg`` 结构体作为参数,默认为空,所有GDAL1的代码可以不用修改直接编译. ``GDALRasterIOExtraArg``  中可以选择重采样方式/偏移/进度条等功能,定义和结构如下:

.. cpp:class:: GDALRasterIOExtraArg

    向RasterIO中传入额外信息

.. code-block:: c++

    /** Structure to pass extra arguments to RasterIO() method
      * @since GDAL 2.0
      */
    typedef struct
    {
        /*! 版本号,方便以后扩展 */ 
        int                    nVersion;
        /*! 重采样算法 */ 
        GDALRIOResampleAlg     eResampleAlg;
        /*! 进度条 callback 函数*/ 
        GDALProgressFunc       pfnProgress;
        /*! 进度条 callback user data */ 
        void                  *pProgressData;
        /*! Indicate if dfXOff, dfYOff, dfXSize and dfYSize are set.
            Mostly reserved from the VRT driver to communicate a more precise
            source window. Must be such that dfXOff - nXOff < 1.0 and
            dfYOff - nYOff < 1.0 and nXSize - dfXSize < 1.0 and nYSize - dfYSize < 1.0 */
        int                    bFloatingPointWindowValidity;
        /*! 像素相对左上角点x轴偏移量,仅在bFloatingPointWindowValidity = TRUE时有效 */
        double                 dfXOff;
        /*! 像素相对左上角点y轴偏移量,仅在bFloatingPointWindowValidity = TRUE时有效 */
        double                 dfYOff;
        /*! 感兴趣区域的像素宽度 bFloatingPointWindowValidity = TRUE时有效 */
        double                 dfXSize;
        /*! 感兴趣区域的像素高度 bFloatingPointWindowValidity = TRUE时有效 */
        double                 dfYSize;
    } GDALRasterIOExtraArg;

.. cpp:enum:: GDALRIOResampleAlg

    重采样方式

.. code-block:: c++
    
    typedef enum
    {
        /*! Nearest neighbour 默认 */                       GRIORA_NearestNeighbour = 0,
        /*! Bilinear (2x2 kernel) */                        GRIORA_Bilinear = 1,
        /*! Cubic Convolution Approximation (4x4 kernel) */ GRIORA_Cubic = 2,
        /*! Cubic B-Spline Approximation (4x4 kernel) */    GRIORA_CubicSpline = 3,
        /*! Lanczos windowed sinc interpolation (6x6 kernel) */ GRIORA_Lanczos = 4,
        /*! Average */                                      GRIORA_Average = 5,
        /*! Mode (selects the value which appears most often of all the sampled points) */
                                                            GRIORA_Mode = 6,
        /*! Gauss blurring */                               GRIORA_Gauss = 7
        /* NOTE: values 8 to 12 are reserved for max,min,med,Q1,Q3 */
    } GDALRIOResampleAlg;

.. c:macro:: INIT_RASTERIO_EXTRA_ARG

    初始化GDALRasterIOExtraArg的宏
    
.. code-block:: c++

    #define INIT_RASTERIO_EXTRA_ARG(s)  \
        do { (s).nVersion = RASTERIO_EXTRA_ARG_CURRENT_VERSION; \
             (s).eResampleAlg = GRIORA_NearestNeighbour; \
             (s).pfnProgress = NULL; \
             (s).pProgressData = NULL; \
             (s).bFloatingPointWindowValidity = FALSE; } while(0)

GDAL2.0中重采样读取完整代码如下:

.. code-block:: c++

    #include "gdal_priv.h"
    #include "cpl_conv.h" 
    //...中间的程序

    //所有程序前先加上
    GDALAllRegister();
    GDALDataset  *poDataset= (GDALDataset *) GDALOpen( pszFilename, GA_ReadOnly );//打开文件
    if( poDataset == NULL )
    {
        //...打开失败处理
    }
    //打开数据集同上
    //获取影像相关信息
    int i,j,width,height,bandNum;
    width = inDS->GetRasterXSize();//影像宽度
    height = inDS->GetRasterYSize();//影像高度
    bandNum = inDS->GetRasterCount();//影像波段数量

    //将第一波段读入
    double *data = new double[width*height];
    GDALRasterBand *band = ds1->GetRasterBand(1);//获取第一波段，波段从1开始
    GDALRasterIOExtraArg exterArg;
    INIT_RASTERIO_EXTRA_ARG(exterArg);
    exterArg.eResampleAlg = GDALRIOResampleAlg::GRIORA_Cubic;//配置插值方法
    band->RasterIO(GF_Read,0,0,width,height,data,width,height,GDT_Float64,0,0,&exterArg);


.. _fullcode:

******************
完整代码
******************

.. literalinclude:: code/fullcode.c
    :language: c++
    :encoding: utf8

.. _zipandweb:

************************
压缩文件与网络文件读取
************************
GDAL可以直接读取压缩文件或者网络中的影像数据,不过需要在文件名前加上特定前缀,GDAL工具和函数都可以使用.

压缩文件
=======================
GDAL对zip文件有只读支持,对gz文件支持读写,注意随机读写压缩文件效率相当低.

对于tar.gz/.tgz/.tar 等gzip文件来说,打开时,使用如下方式:

.. code-block:: c++

     GDALDataset  *poDataset= (GDALDataset *) GDALOpen( \
                                "/vsigzip/D:/test/1.tgz/aaa/cc.tif",\
                                GA_ReadOnly );//打开文件

注意打开时,添加 ``/vsigzip/`` 前缀

类似,打开zip文件时,添加 ``/vsizip/`` 前缀::

    gdalinfo /vsizip/d:/file.zip/test.tif

网络文件
=======================
在前面添加 ``/vsicurl/`` 前缀即可::

    ogrinfo -ro -al -so /vsicurl/http://example.org/gdal/trunk/data/poly.shp

混合vsizip::

    ogrinfo -ro -al -so /vsizip/vsicurl/http://example.com/data/poly.zip

用账户访问ftp::

    ogrinfo -ro -al -so /vsizip/vsicurl/ftp://usr:passwd@a.com/b/c.zip/d.shp 

.. _other:

******************
其他处理
******************
可以使用 :ref:`gdaldriver` 对已有影像数据进行删除、复制、重命名等处理，可以参见 `GdalDriver类 <http://www.gdal.org/classGDALDriver.html>`_ 中的具体函数说明，下面使用一些例子来说明：

.. code-block:: c++

    GDALAllRegister();//注册类型，读取写入任何类型影像必须加入此句
    GDALDriver *poDriver;
    const char *pszFormat ="GTiff";
    poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    poDriver->Delete("TempFile.tif");//删除
    poDirver->Rename("newName.tif","oldName.tif");//重命名
    poDirver->CopyFiles("Replica.tif","original.tif");//复制

