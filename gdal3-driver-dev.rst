.. highlight:: rst

.. _gdal3-driver-dev:

################################
GDAL 3.x 自定义驱动开发
################################

本章介绍如何为 GDAL 3.x 开发自定义的栅格和矢量驱动。驱动是 GDAL 的核心扩展机制，每种支持的数据格式都有对应的驱动实现。开发自定义驱动需要理解 GDAL 的类继承体系、回调函数注册机制以及元数据声明方式。本章内容适用于 GDAL 3.x 系列，并涵盖了 3.7 至 3.13 版本中的重要 API 变更。

.. _raster-driver-arch:

************************************
栅格驱动架构
************************************

GDAL 栅格驱动的核心思路是：子类化 ``GDALDataset`` 和 ``GDALRasterBand`` ，实现一组虚函数或回调，然后将驱动注册到 ``GDALDriverManager`` 。下面分别说明各个组成部分。

.. _subclass-dataset-band:

子类化 GDALDataset 和 GDALRasterBand
======================================

一个最简的只读栅格驱动需要两个类：

* 数据集类，继承自 ``GDALDataset`` （或 ``GDALPamDataset`` 以获得持久化元数据支持），负责管理文件句柄、元数据和波段集合。
* 波段类，继承自 ``GDALRasterBand`` （或 ``GDALPamRasterBand`` ），负责读取实际的像素数据。

.. code-block:: c++

    #include "gdal_pam.h"

    class MyRasterBand;

    // 数据集类
    class MyDataset final : public GDALPamDataset
    {
        friend class MyRasterBand;
        VSILFILE *m_fp = nullptr;

    public:
        MyDataset();
        ~MyDataset() override;

        // 静态回调，用于驱动注册
        static GDALDataset *Open(GDALOpenInfo *);
        static int Identify(GDALOpenInfo *);
    };

    // 波段类
    class MyRasterBand final : public GDALPamRasterBand
    {
    public:
        MyRasterBand(MyDataset *poDS, int nBand);
        ~MyRasterBand() override;

        // 必须实现：按块读取像素数据
        CPLErr IReadBlock(int nBlockXOff, int nBlockYOff, void *pData) override;
    };

.. _identify-open:

Identify() 和 Open() 回调
======================================

``Identify()`` 是一个轻量级的文件格式识别函数，仅检查文件头的前几个字节，不执行任何 I/O 操作。它应该尽可能快，用于在驱动枚举阶段快速过滤不匹配的文件。返回值有三种：

* ``TRUE`` (1)：确定是该格式
* ``FALSE`` (0)：确定不是该格式
* ``-1``：不确定，需要调用 ``Open()`` 进一步判断

``Open()`` 负责完整地打开文件、验证格式、创建数据集对象并设置波段。如果文件无法打开或格式不匹配，应返回 ``nullptr`` 。

.. code-block:: c++

    // 轻量级格式识别，仅检查文件头
    int MyDataset::Identify(GDALOpenInfo *poOpenInfo)
    {
        if (poOpenInfo->nHeaderBytes < 16)
            return FALSE;

        // 检查文件头的魔数（magic number）
        if (memcmp(poOpenInfo->pabyHeader, "MYFMT", 5) != 0)
            return FALSE;

        return TRUE;
    }

    // 完整打开文件并构造数据集
    GDALDataset *MyDataset::Open(GDALOpenInfo *poOpenInfo)
    {
        if (!Identify(poOpenInfo))
            return nullptr;

        // 不支持更新模式
        if (poOpenInfo->eAccess == GA_Update)
        {
            ReportUpdateNotSupportedByDriver("MyFormat");
            return nullptr;
        }

        // 检查文件指针
        if (poOpenInfo->fpL == nullptr)
            return nullptr;

        auto poDS = std::make_unique<MyDataset>();

        // 从 GDALOpenInfo 借用文件指针
        std::swap(poDS->m_fp, poOpenInfo->fpL);

        // 从文件头中读取尺寸信息
        const GByte *pabyHeader = poOpenInfo->pabyHeader;
        poDS->nRasterXSize = pabyHeader[8] | (pabyHeader[9] << 8);
        poDS->nRasterYSize = pabyHeader[10] | (pabyHeader[11] << 8);

        if (!GDALCheckDatasetDimensions(poDS->nRasterXSize, poDS->nRasterYSize))
            return nullptr;

        // 创建波段
        poDS->SetBand(1, new MyRasterBand(poDS.get(), 1));

        // 设置描述和加载 PAM 元数据
        poDS->SetDescription(poOpenInfo->pszFilename);
        poDS->TryLoadXML();

        // 初始化概览管理器
        poDS->oOvManager.Initialize(poDS.get(), poOpenInfo->pszFilename);

        return poDS.release();
    }

.. _readblock-rasterio:

ReadBlock() 和 RasterIO()
======================================

``IReadBlock()`` 是波段类的核心方法，负责读取一个数据块（block）的像素数据。GDAL 默认使用 256x256 的块大小，但可以通过设置 ``nBlockXSize`` 和 ``nBlockYSize`` 来自定义。对于逐行存储的格式，通常将 ``nBlockYSize`` 设为 1。

.. code-block:: c++

    MyRasterBand::MyRasterBand(MyDataset *poDSIn, int nBandIn)
    {
        poDS = poDSIn;
        nBand = nBandIn;
        eDataType = GDT_Byte;              // 像素数据类型
        nBlockXSize = poDS->GetRasterXSize();  // 整行作为一个块
        nBlockYSize = 1;
    }

    CPLErr MyRasterBand::IReadBlock(int /* nBlockXOff */, int nBlockYOff,
                                    void *pImage)
    {
        MyDataset *poGDS = cpl::down_cast<MyDataset *>(poDS);

        // 计算文件偏移量并读取一行数据
        const vsi_l_offset nOffset = HEADER_SIZE +
            static_cast<vsi_l_offset>(nBlockYOff) * nBlockXSize;

        VSIFSeekL(poGDS->m_fp, nOffset, SEEK_SET);
        if (VSIFReadL(pImage, 1, nBlockXSize, poGDS->m_fp) != nBlockXSize)
        {
            CPLError(CE_Failure, CPLE_AppDefined, "Cannot read scanline %d",
                     nBlockYOff);
            return CE_Failure;
        }

        return CE_None;
    }

如果需要更高效的随机访问或支持自定义重采样，可以重写 ``IRasterIO()`` 方法。该方法在 ``GDALDataset`` 和 ``GDALRasterBand`` 层都有对应版本，允许驱动直接处理任意矩形区域的读写请求，而不必经过分块机制。

.. _driver-metadata:

驱动元数据项
======================================

驱动注册时需要声明一组元数据项，用于告知 GDAL 框架该驱动的能力和特征。常用的元数据项包括：

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - 元数据常量
     - 说明
   * - ``GDAL_DCAP_RASTER``
     - 声明为栅格驱动，值为 ``"YES"``
   * - ``GDAL_DCAP_VECTOR``
     - 声明为矢量驱动，值为 ``"YES"``
   * - ``GDAL_DMD_LONGNAME``
     - 驱动的完整名称，如 ``"Japanese DEM (.mem)"``
   * - ``GDAL_DMD_EXTENSION``
     - 默认文件扩展名，如 ``"mem"``
   * - ``GDAL_DMD_EXTENSIONS``
     - 多个扩展名，以空格分隔，如 ``"csv tsv psv"``
   * - ``GDAL_DCAP_VIRTUALIO``
     - 是否支持 :ref:`VSI 虚拟文件系统 <vsi-support>`
   * - ``GDAL_DMD_HELPTOPIC``
     - 文档页面路径
   * - ``GDAL_DCAP_OPEN``
     - 声明驱动支持 ``Open()``
   * - ``GDAL_DCAP_CREATE``
     - 声明驱动支持 ``Create()``
   * - ``GDAL_DCAP_CREATECOPY``
     - 声明驱动支持 ``CreateCopy()``

.. _minimal-raster-driver:

代码示例：最简只读栅格驱动
======================================

以下是一个完整的只读栅格驱动实现，参考了 GDAL 内置的 JDEM 驱动模式。该驱动读取一个假想的简单二进制格式，文件头包含尺寸信息，后续为逐行存储的像素数据。

.. code-block:: c++

    #include "cpl_port.h"
    #include "gdal_frmts.h"
    #include "gdal_pam.h"
    #include "gdal_driver.h"
    #include "gdal_drivermanager.h"
    #include "gdal_openinfo.h"

    constexpr int HEADER_SIZE = 16;

    /************************************************************************/
    /*                            MyRasterBand                              */
    /************************************************************************/

    class MyDataset;

    class MyRasterBand final : public GDALPamRasterBand
    {
        friend class MyDataset;

    public:
        MyRasterBand(MyDataset *poDS, int nBand);
        CPLErr IReadBlock(int, int, void *) override;
    };

    MyRasterBand::MyRasterBand(MyDataset *poDSIn, int nBandIn)
    {
        poDS = poDSIn;
        nBand = nBandIn;
        eDataType = GDT_Byte;
        nBlockXSize = poDS->GetRasterXSize();
        nBlockYSize = 1;
    }

    CPLErr MyRasterBand::IReadBlock(int /* nBlockXOff */, int nBlockYOff,
                                    void *pImage)
    {
        MyDataset *poGDS = cpl::down_cast<MyDataset *>(poDS);
        const vsi_l_offset nOffset =
            HEADER_SIZE + static_cast<vsi_l_offset>(nBlockYOff) * nBlockXSize;

        VSIFSeekL(poGDS->m_fp, nOffset, SEEK_SET);
        if (VSIFReadL(pImage, 1, nBlockXSize, poGDS->m_fp) != nBlockXSize)
        {
            CPLError(CE_Failure, CPLE_AppDefined, "Cannot read scanline %d",
                     nBlockYOff);
            return CE_Failure;
        }
        return CE_None;
    }

    /************************************************************************/
    /*                             MyDataset                                */
    /************************************************************************/

    class MyDataset final : public GDALPamDataset
    {
        friend class MyRasterBand;
        VSILFILE *m_fp = nullptr;

    public:
        MyDataset();
        ~MyDataset() override;

        static GDALDataset *Open(GDALOpenInfo *);
        static int Identify(GDALOpenInfo *);
    };

    MyDataset::MyDataset() = default;

    MyDataset::~MyDataset()
    {
        FlushCache(true);
        if (m_fp != nullptr)
            CPL_IGNORE_RET_VAL(VSIFCloseL(m_fp));
    }

    int MyDataset::Identify(GDALOpenInfo *poOpenInfo)
    {
        if (poOpenInfo->nHeaderBytes < HEADER_SIZE)
            return FALSE;
        if (memcmp(poOpenInfo->pabyHeader, "MYFMT", 5) != 0)
            return FALSE;
        return TRUE;
    }

    GDALDataset *MyDataset::Open(GDALOpenInfo *poOpenInfo)
    {
        if (!Identify(poOpenInfo))
            return nullptr;

        if (poOpenInfo->eAccess == GA_Update)
        {
            ReportUpdateNotSupportedByDriver("MyFormat");
            return nullptr;
        }

        if (poOpenInfo->fpL == nullptr)
            return nullptr;

        auto poDS = std::make_unique<MyDataset>();
        std::swap(poDS->m_fp, poOpenInfo->fpL);

        const GByte *pabyHeader = poOpenInfo->pabyHeader;
        poDS->nRasterXSize = pabyHeader[8] | (pabyHeader[9] << 8);
        poDS->nRasterYSize = pabyHeader[10] | (pabyHeader[11] << 8);

        if (!GDALCheckDatasetDimensions(poDS->nRasterXSize, poDS->nRasterYSize))
            return nullptr;

        poDS->SetBand(1, new MyRasterBand(poDS.get(), 1));
        poDS->SetDescription(poOpenInfo->pszFilename);
        poDS->TryLoadXML();
        poDS->oOvManager.Initialize(poDS.get(), poOpenInfo->pszFilename);

        return poDS.release();
    }

    /************************************************************************/
    /*                        GDALRegister_MyFormat()                       */
    /************************************************************************/

    void GDALRegister_MyFormat()
    {
        if (GDALGetDriverByName("MyFormat") != nullptr)
            return;

        GDALDriver *poDriver = new GDALDriver();

        poDriver->SetDescription("MyFormat");
        poDriver->SetMetadataItem(GDAL_DCAP_RASTER, "YES");
        poDriver->SetMetadataItem(GDAL_DMD_LONGNAME,
                                  "My Custom Format (.myf)");
        poDriver->SetMetadataItem(GDAL_DMD_EXTENSION, "myf");
        poDriver->SetMetadataItem(GDAL_DCAP_VIRTUALIO, "YES");

        poDriver->pfnOpen = MyDataset::Open;
        poDriver->pfnIdentify = MyDataset::Identify;

        GetGDALDriverManager()->RegisterDriver(poDriver);
    }

.. _driver-registration:

驱动注册函数
======================================

每个驱动必须提供一个 ``GDALRegister_<DriverName>()`` 函数，该函数完成以下工作：

1. 检查驱动是否已注册，避免重复注册
2. 创建 ``GDALDriver`` 对象
3. 设置驱动描述（短名称）
4. 设置元数据项（能力声明、文件扩展名等）
5. 赋值回调函数指针（ ``pfnOpen`` 、 ``pfnIdentify`` 、 ``pfnCreate`` 等）
6. 调用 ``GetGDALDriverManager()->RegisterDriver()`` 完成注册

.. code-block:: c++

    void GDALRegister_MyFormat()
    {
        // 避免重复注册
        if (GDALGetDriverByName("MyFormat") != nullptr)
            return;

        GDALDriver *poDriver = new GDALDriver();

        // 驱动短名称
        poDriver->SetDescription("MyFormat");

        // 能力声明
        poDriver->SetMetadataItem(GDAL_DCAP_RASTER, "YES");
        poDriver->SetMetadataItem(GDAL_DCAP_OPEN, "YES");

        // 驱动元信息
        poDriver->SetMetadataItem(GDAL_DMD_LONGNAME,
                                  "My Custom Format (.myf)");
        poDriver->SetMetadataItem(GDAL_DMD_EXTENSION, "myf");
        poDriver->SetMetadataItem(GDAL_DMD_HELPTOPIC,
                                  "drivers/raster/myformat.html");

        // VSI 支持（见下文）
        poDriver->SetMetadataItem(GDAL_DCAP_VIRTUALIO, "YES");

        // 回调函数
        poDriver->pfnOpen = MyDataset::Open;
        poDriver->pfnIdentify = MyDataset::Identify;

        // 如果支持创建，还需设置：
        // poDriver->pfnCreate = MyDataset::Create;
        // poDriver->pfnCreateCopy = MyDataset::CreateCopy;

        // 注册到驱动管理器
        GetGDALDriverManager()->RegisterDriver(poDriver);
    }

对于支持写入的驱动，还需要实现 ``Create()`` 或 ``CreateCopy()`` 回调，并在注册时设置 ``GDAL_DCAP_CREATE`` 或 ``GDAL_DCAP_CREATECOPY`` 元数据项。``Create()`` 从零创建一个空的数据集，``CreateCopy()`` 则基于一个已有数据集进行复制转换。

.. _vector-driver-arch:

************************************
矢量驱动架构
************************************

矢量驱动的架构与栅格驱动类似，但核心类是 ``OGRLayer`` 而非 ``GDALRasterBand`` 。一个矢量驱动通常需要：

* 数据集类，继承 ``GDALDataset`` ，管理多个图层
* 图层类，继承 ``OGRLayer`` ，实现要素的读写和查询

.. _vector-subclass:

子类化 GDALDataset 和 OGRLayer
======================================

``OGRLayer`` 是 GDAL/OGR 矢量数据的核心抽象，每个图层代表一组同类型的要素。子类需要实现以下纯虚函数或重要虚函数：

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - 虚函数
     - 说明
   * - ``GetNextFeature()``
     - 遍历要素的核心方法，返回下一个 ``OGRFeature`` ，遍历结束返回 ``nullptr``
   * - ``ICreateFeature()``
     - 向图层写入一个要素
   * - ``SetSpatialFilter()``
     - 设置空间过滤条件
   * - ``GetLayerDefn()``
     - 返回图层的要素类定义（ ``OGRFeatureDefn`` ），包含字段定义和几何类型
   * - ``TestCapability()``
     - 查询图层支持的能力（如读、写、随机读取等）
   * - ``ResetReading()``
     - 重置要素遍历到起始位置
   * - ``GetSpatialRef()``
     - 返回图层的空间参考系统

.. code-block:: c++

    #include "ogrsf_frmts.h"

    class MyVectorLayer final : public OGRLayer
    {
        OGRFeatureDefn *m_poFeatureDefn;
        OGRSpatialReference m_oSRS;
        int m_nNextFID;
        // ... 内部数据源状态

    public:
        MyVectorLayer(const char *pszName,
                      OGRSpatialReference *poSRS,
                      OGRwkbGeometryType eGeomType);
        ~MyVectorLayer() override;

        // 要素遍历
        void ResetReading() override;
        OGRFeature *GetNextFeature() override;

        // 要素写入
        OGRErr ICreateFeature(OGRFeature *poFeature) override;

        // 空间过滤
        OGRErr ISetSpatialFilter(int iGeomField,
                                 const OGRGeometry *poGeom) override;

        // 图层定义
        const OGRFeatureDefn *GetLayerDefn() override;

        // 能力查询
        int TestCapability(const char *) override;
    };

``GetNextFeature()`` 是最核心的方法。实现时需要注意：

* 要素的 FID 必须在图层内唯一
* 返回的 ``OGRFeature`` 的所有权归调用者，调用者负责释放
* 当没有更多要素时返回 ``nullptr``
* 需要尊重当前的空间过滤条件

``ICreateFeature()`` （注意是 ``I`` 前缀的内部版本，公开的 ``CreateFeature()`` 会进行参数校验后调用它）负责将要素持久化到数据源中。

.. _vector-driver-reg:

代码示例：矢量驱动注册
======================================

矢量驱动的注册函数与栅格驱动类似，但元数据项使用 ``GDAL_DCAP_VECTOR`` 而非 ``GDAL_DCAP_RASTER`` 。

.. code-block:: c++

    /************************************************************************/
    /*                         MyVectorDataset                              */
    /************************************************************************/

    class MyVectorDataset final : public GDALDataset
    {
        std::vector<std::unique_ptr<MyVectorLayer>> m_apoLayers;

    public:
        int GetLayerCount() override;
        OGRLayer *GetLayer(int iLayer) override;

        static GDALDataset *Open(GDALOpenInfo *);
        static int Identify(GDALOpenInfo *);

        // 如果支持创建图层
        OGRLayer *ICreateLayer(const char *pszName,
                               OGRSpatialReference *poSpatialRef,
                               OGRwkbGeometryType eGType,
                               CSLConstList papszOptions) override;
    };

    /************************************************************************/
    /*                      RegisterOGRMyFormat()                           */
    /************************************************************************/

    void RegisterOGRMyFormat()
    {
        if (GDALGetDriverByName("MyVectorFormat") != nullptr)
            return;

        GDALDriver *poDriver = new GDALDriver();

        poDriver->SetDescription("MyVectorFormat");
        poDriver->SetMetadataItem(GDAL_DCAP_VECTOR, "YES");
        poDriver->SetMetadataItem(GDAL_DCAP_OPEN, "YES");
        poDriver->SetMetadataItem(GDAL_DCAP_CREATE_LAYER, "YES");
        poDriver->SetMetadataItem(GDAL_DCAP_CREATE_FIELD, "YES");
        poDriver->SetMetadataItem(GDAL_DMD_LONGNAME,
                                  "My Vector Format (.mvf)");
        poDriver->SetMetadataItem(GDAL_DMD_EXTENSIONS, "mvf");
        poDriver->SetMetadataItem(GDAL_DCAP_VIRTUALIO, "YES");

        poDriver->pfnOpen = MyVectorDataset::Open;
        poDriver->pfnIdentify = MyVectorDataset::Identify;

        GetGDALDriverManager()->RegisterDriver(poDriver);
    }

矢量驱动常用的额外元数据项：

* ``GDAL_DCAP_CREATE_LAYER`` ：声明支持创建新图层
* ``GDAL_DCAP_DELETE_LAYER`` ：声明支持删除图层
* ``GDAL_DCAP_CREATE_FIELD`` ：声明支持添加字段
* ``GDAL_DCAP_DELETE_FIELD`` ：声明支持删除字段
* ``GDAL_DCAP_CURVE_GEOMETRIES`` ：声明支持曲线几何
* ``GDAL_DCAP_Z_GEOMETRIES`` ：声明支持三维几何

.. _deferred-plugin-loading:

************************************
延迟插件加载（RFC 96）
************************************

GDAL 3.9 引入了延迟插件加载机制（ :ref:`RFC 96 <rfc-96>` ），允许将插件驱动的实际加载推迟到真正需要时。这对于减少 ``GDALAllRegister()`` 的启动时间非常有效，特别是当系统安装了大量插件驱动时。

.. _deferred-background:

背景与动机
======================================

在 GDAL 3.9 之前，插件驱动在 ``GDALAllRegister()`` 时通过 ``dlopen()`` 全部加载。当系统安装了 100 多个插件时，仅 ``dlopen()`` 调用就需要约 300 毫秒，对短生命周期的进程（如批量运行 ``gdalinfo`` ）影响显著。

延迟加载的核心思想是：在 ``GDALAllRegister()`` 时只注册一个轻量的代理驱动（ ``GDALPluginDriverProxy`` ），该代理携带足够的元数据（如扩展名、 ``Identify()`` 回调）来判断是否需要加载实际驱动。只有当用户真正打开该格式的文件时，才加载实际的共享库。

.. _deferred-proxy:

代理驱动机制
======================================

代理驱动 ``GDALPluginDriverProxy`` 继承自 ``GDALDriver`` ，通过 ``GDALDriverManager::DeclareDeferredPluginDriver()`` 注册。代理驱动需要设置以下信息：

* 驱动名称和描述
* 格式能力声明（ ``GDAL_DCAP_RASTER`` 、 ``GDAL_DCAP_VECTOR`` 等）
* 文件扩展名
* ``pfnIdentify`` 回调（必须编译在 libgdal 核心库中，不依赖外部库）
* 对应的 ``GDAL_DCAP_OPEN`` 、 ``GDAL_DCAP_CREATE`` 等声明

.. code-block:: c++

    // 驱动核心能力的元数据设置函数
    // 在代理驱动和真实驱动中共享
    void FOODriverSetCommonMetadata(GDALDriver *poDriver)
    {
        poDriver->SetDescription("FOO");
        poDriver->SetMetadataItem(GDAL_DMD_LONGNAME, "The FOO format");
        poDriver->SetMetadataItem(GDAL_DCAP_RASTER, "YES");
        poDriver->SetMetadataItem(GDAL_DMD_EXTENSION, "foo");
        poDriver->SetMetadataItem(GDAL_DCAP_OPEN, "YES");
        poDriver->pfnIdentify = FOODatasetIdentify;
    }

    // 延迟插件声明（编译在 libgdal 核心库中）
    #ifdef PLUGIN_FILENAME
    void DeclareDeferredFOOPlugin()
    {
        if (GDALGetDriverByName("FOO") != nullptr)
            return;

        auto poDriver = new GDALPluginDriverProxy(PLUGIN_FILENAME);
        FOODriverSetCommonMetadata(poDriver);
        GetGDALDriverManager()->DeclareDeferredPluginDriver(poDriver);
    }
    #endif

真实驱动的注册函数仍然存在，但只在插件被实际加载后才执行：

.. code-block:: c++

    // 真实驱动注册（在插件共享库中）
    void GDALRegister_FOO()
    {
        if (GDALGetDriverByName("FOO") != nullptr)
            return;

        GDALDriver *poDriver = new GDALDriver();
        FOODriverSetCommonMetadata(poDriver);

        // 设置实际的回调函数
        poDriver->pfnOpen = FOODataset::Open;
        poDriver->pfnCreate = FOODataset::Create;

        GetGDALDriverManager()->RegisterDriver(poDriver);
    }

``gdalallregister.cpp`` 中的注册顺序：

.. code-block:: c++

    void GDALAllRegister()
    {
        auto poDriverManager = GetGDALDriverManager();

        // 延迟驱动声明必须在 AutoLoadDrivers() 之前
        #if defined(DEFERRED_FOO_DRIVER)
        DeclareDeferredFOOPlugin();
        #endif

        // AutoLoadDrivers() 不会加载已被延迟声明的插件
        poDriverManager->AutoLoadDrivers();

        // 内置驱动的正常注册
        #if FRMT_foo
        GDALRegisterFOO();
        #endif
    }

.. _deferred-out-of-tree:

Out-of-tree 驱动的延迟加载
======================================

外部驱动也可以利用延迟加载机制，前提是 libgdal 编译时通过 CMake 变量 ``ADD_EXTERNAL_DEFERRED_PLUGIN_<driver_name>`` 指向包含代理声明代码的外部源文件：

.. code-block:: cmake

    set(ADD_EXTERNAL_DEFERRED_PLUGIN_FOO
        FILEPATH /path/to/foo_deferred_plugin.cpp)

该源文件必须导出一个 C 链接的 ``DeclareDeferredFOO()`` 函数，负责创建 ``GDALPluginDriverProxy`` 并调用 ``DeclareDeferredPluginDriver()`` 。

.. _cslconstlist-change:

************************************
CSLConstList 参数变更（3.13）
************************************

GDAL 3.13 将大量 API 中的 ``char **`` 参数类型替换为 ``CSLConstList`` 。``CSLConstList`` 本质上是 ``const char *const *`` 的类型别名，表示一个只读的字符串列表。这一变更提升了类型安全性，明确了参数的只读语义。

.. _cslconstlist-affected:

受影响的 API
======================================

以下接口的签名在 GDAL 3.13 中发生了变更：

* ``GDALDriver::pfnCreate`` 回调：最后一个参数从 ``char **`` 改为 ``CSLConstList``
* ``GDALDriver::pfnCreateCopy`` 回调：第三个参数从 ``char **`` 改为 ``CSLConstList``
* ``GDALDataset::AddBand()`` ：参数从 ``char **`` 改为 ``CSLConstList``
* ``GDALDataset::AdviseRead()`` ：参数从 ``char **`` 改为 ``CSLConstList``
* ``GDALDataset::BeginAsyncReader()`` ：参数从 ``char **`` 改为 ``CSLConstList``
* ``GDALDataset::CopyLayer()`` ：参数从 ``char **`` 改为 ``CSLConstList``
* ``GDALRasterBand::AdviseRead()`` ：参数从 ``char **`` 改为 ``CSLConstList``
* ``GDALMajorObject::SetMetadata()`` ：参数从 ``char **`` 改为 ``CSLConstList``
* ``GDALMajorObject::GetMetadata()`` ：返回值从 ``char **`` 改为 ``CSLConstList``

.. _cslconstlist-impact:

对 Out-of-tree 驱动的影响
======================================

如果你维护一个外部驱动，需要注意以下事项：

* ``Create()`` 和 ``CreateCopy()`` 的函数签名必须更新。旧签名：

  .. code-block:: c++

      // GDAL 3.12 及更早
      static GDALDataset *Create(const char *pszName,
                                  int nXSize, int nYSize, int nBands,
                                  GDALDataType eType,
                                  char **papszOptions);

  新签名：

  .. code-block:: c++

      // GDAL 3.13+
      static GDALDataset *Create(const char *pszName,
                                  int nXSize, int nYSize, int nBands,
                                  GDALDataType eType,
                                  CSLConstList papszOptions);

* ``GetMetadata()`` 返回 ``CSLConstList`` 后，如果你之前将返回值存储为 ``char **`` ，需要改为 ``CSLConstList`` 。此变更与旧版 GDAL 兼容，因为 ``char **`` 可以隐式转换为 ``CSLConstList`` 。

* ``SetMetadata()`` 接受 ``CSLConstList`` 后，调用方无需修改代码，但驱动实现者应注意参数不可被修改。

.. _dataset-close-change:

*************************************
GDALDataset::Close() 签名变化（3.13）
*************************************

``GDALDataset::Close()`` 方法最初在 GDAL 3.7 中引入（ :ref:`RFC 91 <rfc-91>` ），用于在数据集销毁前提供一个可以返回错误码的清理机会。在 GDAL 3.13 中，其签名增加了进度回调参数：

.. code-block:: c++

    // GDAL 3.7 - 3.12
    virtual CPLErr Close();

    // GDAL 3.13+
    virtual CPLErr Close(GDALProgressFunc pfnProgress = nullptr,
                         void *pProgressData = nullptr);

.. _close-implementation:

实现 Close() 的最佳实践
======================================

驱动实现 ``Close()`` 时应遵循以下模式：

.. code-block:: c++

    MyDataset::~MyDataset()
    {
        try
        {
            MyDataset::Close();
        }
        catch (const std::exception &exc)
        {
            CPLError(CE_Failure, CPLE_AppDefined,
                     "Exception thrown in MyDataset::Close(): %s",
                     exc.what());
        }
        catch (...)
        {
            CPLError(CE_Failure, CPLE_AppDefined,
                     "Exception thrown in MyDataset::Close()");
        }
    }

    CPLErr MyDataset::Close()
    {
        CPLErr eErr = CE_None;

        // 防止重复关闭
        if (nOpenFlags != OPEN_FLAGS_CLOSED)
        {
            // 刷新缓存
            if (MyDataset::FlushCache(true) != CE_None)
                eErr = CE_Failure;

            // 驱动特定的清理操作
            if (m_fpImage != nullptr)
            {
                if (VSIFCloseL(m_fpImage) != 0)
                {
                    CPLError(CE_Failure, CPLE_FileIO, "VSIFCloseL() failed");
                    eErr = CE_Failure;
                }
                m_fpImage = nullptr;
            }

            // 调用父类的 Close()
            if (GDALDataset::Close() != CE_None)
                eErr = CE_Failure;
        }
        return eErr;
    }

关键要点：

* ``Close()`` 必须可以被多次调用，后续调用应直接返回 ``CE_None``
* 如果驱动实现了 ``Close()`` ，析构函数中必须调用它
* ``Close()`` 调用后，除 ``Close()`` 和析构函数外不应再调用其他方法
* 优先使用 ``Close()`` 而非析构函数进行资源清理，因为 ``Close()`` 可以返回错误码

.. _vsi-support:

************************************
VSI 虚拟文件系统支持声明
************************************

``GDAL_DCAP_VIRTUALIO`` 元数据项用于声明驱动是否支持 GDAL 的 VSI 虚拟文件系统（ ``/vsi*/`` 路径前缀）。支持 VSI 意味着驱动可以通过 ``VSIFOpenL()`` 、 ``VSIFReadL()`` 等 API 读写文件，从而支持以下场景：

* 从压缩文件中读取数据（ ``/vsizip/`` 、 ``/vsitar/`` ）
* 从网络 URL 读取数据（ ``/vsicurl/`` 、 ``/vsis3/`` ）
* 从内存缓冲区读取数据（ ``/vsimem/`` ）

要使驱动支持 VSI，需要：

1. 使用 ``VSILFILE *`` 而非 ``FILE *`` 作为文件句柄类型
2. 使用 ``VSIFOpenL()`` 、 ``VSIFReadL()`` 、 ``VSIFSeekL()`` 等 VSI 系列函数
3. 在注册时声明 ``GDAL_DCAP_VIRTUALIO``

.. code-block:: c++

    // 注册时声明 VSI 支持
    poDriver->SetMetadataItem(GDAL_DCAP_VIRTUALIO, "YES");

    // 在 Open() 中使用 VSI 函数打开文件
    VSILFILE *fp = VSIFOpenL(poOpenInfo->pszFilename, "rb");
    if (fp == nullptr)
        return nullptr;

    // 使用 VSI 函数读取数据
    GByte abyHeader[16];
    if (VSIFReadL(abyHeader, 1, 16, fp) != 16)
    {
        VSIFCloseL(fp);
        return nullptr;
    }

.. warning::

    GDAL 3.13 中， ``VSILFILE *`` 类型不再与 ``FILE *`` 别名（即使在非 DEBUG 构建中）。所有使用 VSI API 的外部驱动必须确保文件句柄类型为 ``VSILFILE *`` 。
