.. highlight:: rst

.. _gdal3-vsi-cpl:

################################################
VSI虚拟文件系统与CPL工具函数
################################################

GDAL的CPL（Common Portability Library，通用可移植库）提供了大量底层工具函数，其中VSI（Virtual System Interface，虚拟系统接口）虚拟文件系统是GDAL访问各种数据源的基础抽象层。本章将介绍VSI虚拟文件系统和CPL中常用的字符串、错误处理等工具函数。

.. _vsi-filesystem:

********************
VSI 虚拟文件系统
********************

VSI虚拟文件系统是GDAL对文件I/O的抽象层，它将各种数据源（本地文件、内存、HTTP远程文件、ZIP压缩包、云存储等）统一为类似POSIX文件的接口。通过VSI，开发者可以用相同的代码读取不同来源的数据。

VSI的核心思想是：所有文件路径都以特定前缀开头，GDAL根据前缀自动选择对应的文件系统处理器。例如 ``/vsimem/`` 表示内存文件， ``/vsicurl/`` 表示HTTP远程文件。

.. _vsimem:

/vsimem/ 内存文件系统
======================

``/vsimem/`` 是内存中的虚拟文件系统，文件数据完全存储在内存中，不涉及磁盘I/O。它常用于：

- 创建临时文件，避免磁盘读写开销
- 在GDAL API之间传递中间数据
- 测试和调试时模拟文件操作

主要API函数：

- ``VSIFOpenL()`` ：打开（或创建）一个虚拟文件，返回 ``VSILFILE*`` 句柄
- ``VSIFWriteL()`` / ``VSIFReadL()`` ：写入/读取数据
- ``VSIFCloseL()`` ：关闭文件句柄
- ``VSIGetMemFileBuffer()`` ：获取内存文件的缓冲区指针和大小
- ``VSIUnlink()`` ：删除虚拟文件
- ``VSIFileFromMemBuffer()`` ：从已有的内存缓冲区创建虚拟文件

下面的示例演示了在 ``/vsimem/`` 中创建临时文件、写入数据、读回数据并清理的完整流程：

.. code-block:: c++

    #include "gdal.h"
    #include "cpl_vsi.h"
    #include <cstring>
    #include <cstdio>

    int main()
    {
        GDALAllRegister();

        const char* pszFilename = "/vsimem/temp_test.dat";
        const char* pszData = "Hello, VSI virtual file system!";
        size_t nDataLen = strlen(pszData);

        // 1. 创建内存文件并写入数据
        VSILFILE* fp = VSIFOpenL(pszFilename, "wb");
        if (fp == nullptr)
        {
            printf("无法创建内存文件\n");
            return 1;
        }
        VSIFWriteL(pszData, 1, nDataLen, fp);
        VSIFCloseL(fp);
        printf("写入 %zu 字节到 %s\n", nDataLen, pszFilename);

        // 2. 读取内存文件内容
        fp = VSIFOpenL(pszFilename, "rb");
        if (fp == nullptr)
        {
            printf("无法打开内存文件\n");
            return 1;
        }
        char szBuffer[256] = {0};
        VSIFReadL(szBuffer, 1, nDataLen, fp);
        VSIFCloseL(fp);
        printf("读取内容: %s\n", szBuffer);

        // 3. 使用 VSIGetMemFileBuffer 获取缓冲区（不需要打开文件）
        vsi_l_offset nLength = 0;
        GByte* pabyBuffer = VSIGetMemFileBuffer(pszFilename, &nLength, FALSE);
        if (pabyBuffer != nullptr)
        {
            printf("缓冲区大小: " CPL_FRMT_GIB " 字节\n", (GIntBig)nLength);
        }

        // 4. 清理：删除虚拟文件
        VSIUnlink(pszFilename);
        printf("已删除虚拟文件\n");

        return 0;
    }

也可以使用 ``VSIFileFromMemBuffer()`` 直接从已有的内存缓冲区创建虚拟文件，适用于已有数据需要包装为文件的场景：

.. code-block:: c++

    // 从已有缓冲区创建虚拟文件（bTakeOwnership=TRUE 表示 GDAL 接管内存管理）
    GByte* pabyData = (GByte*)CPLMalloc(1024);
    memset(pabyData, 0xAB, 1024);
    VSILFILE* fp = VSIFileFromMemBuffer("/vsimem/from_buffer.dat",
                                         pabyData, 1024, TRUE);
    // 使用完毕后同样需要 VSIUnlink 清理
    VSIUnlink("/vsimem/from_buffer.dat");

.. _vsicurl:

/vsicurl/ HTTP 远程文件访问
=============================

``/vsicurl/`` 前缀允许GDAL通过HTTP/HTTPS协议访问远程文件，支持范围请求（Range Requests），可以按需读取文件的指定部分，无需下载整个文件。这对访问大型遥感影像非常有用。

.. code-block:: c++

    // 直接打开远程文件，GDAL 会自动处理 HTTP 范围请求
    GDALDatasetH hDS = GDALOpen("/vsicurl/https://example.com/data.tif", GA_ReadOnly);

``/vsicurl/`` 支持许多配置选项，可以通过 ``CPLSetConfigOption()`` 设置，例如：

- ``GDAL_HTTP_CONNECTTIMEOUT`` ：连接超时（秒）
- ``GDAL_HTTP_TIMEOUT`` ：总超时（秒）
- ``CPL_VSIL_CURL_CACHE_SIZE`` ：缓存大小（字节）

.. _vsizip:

/vsizip/ ZIP 压缩文件访问
============================

``/vsizip/`` 前缀允许直接读取ZIP压缩包内的文件，无需先解压。支持读取ZIP内的指定文件。

.. code-block:: c++

    // 读取 ZIP 包内的文件
    GDALDatasetH hDS = GDALOpen("/vsizip/my_archive.zip/subfolder/data.tif", GA_ReadOnly);

    // 也可以不指定内部文件名，GDAL 会自动查找
    GDALDatasetH hDS2 = GDALOpen("/vsizip/my_archive.zip", GA_ReadOnly);

.. _vsicloud:

/vsis3/、/vsigs/、/vsioss/ 云存储访问
=========================================

GDAL 3.x 支持直接访问多种云存储服务：

- ``/vsis3/`` ：Amazon S3 兼容存储（包括 MinIO 等）
- ``/vsigs/`` ：Google Cloud Storage
- ``/vsioss/`` ：阿里云 OSS
- ``/vsiaz/`` ：Azure Blob Storage
- ``/vsiswift/`` ：OpenStack Swift

使用前需要配置相应的认证信息，例如访问 S3：

.. code-block:: c++

    // 配置 S3 认证
    CPLSetConfigOption("AWS_ACCESS_KEY_ID", "your_access_key");
    CPLSetConfigOption("AWS_SECRET_ACCESS_KEY", "your_secret_key");
    CPLSetConfigOption("AWS_DEFAULT_REGION", "us-east-1");

    // 打开 S3 上的文件
    GDALDatasetH hDS = GDALOpen("/vsis3/bucket-name/path/to/data.tif", GA_ReadOnly);

.. _vsi-chain:

VSI 链式组合
==============

VSI前缀可以链式组合，实现复杂的数据访问路径。例如，直接读取远程ZIP包中的文件：

.. code-block:: c++

    // 读取远程 ZIP 包中的文件
    GDALDatasetH hDS = GDALOpen(
        "/vsizip//vsicurl/https://example.com/data.zip/inner.tif",
        GA_ReadOnly);

    // 读取远程 gzip 压缩文件
    GDALDatasetH hDS2 = GDALOpen(
        "/vsigzip//vsicurl/https://example.com/data.csv.gz",
        GA_ReadOnly);

这种链式组合非常灵活，可以将多个VSI处理器串联起来，例如 ``/vsicurl/`` 获取远程数据、 ``/vsizip/`` 解压、 ``/vsimem/`` 缓存等。

.. _gdal-open-flags:

GDALOpenEx 打开标志
====================

``GDALOpenEx()`` 是GDAL 2.x引入的通用数据集打开函数，相比 ``GDALOpen()`` 提供了更多控制选项。通过标志位参数可以指定打开方式：

.. code-block:: c++

    // 常用打开标志
    #define GDAL_OF_READONLY     0x00  // 只读模式
    #define GDAL_OF_UPDATE       0x01  // 更新模式
    #define GDAL_OF_ALL          0x00  // 允许栅格和矢量驱动
    #define GDAL_OF_RASTER       0x02  // 仅允许栅格驱动
    #define GDAL_OF_VECTOR       0x04  // 仅允许矢量驱动
    #define GDAL_OF_GNM          0x08  // 允许 GNM 驱动
    #define GDAL_OF_SHARED       0x20  // 共享模式，相同文件返回同一对象
    #define GDAL_OF_VERBOSE_ERROR 0x40  // 打开失败时输出详细错误
    #define GDAL_OF_INTERNAL     0x80  // 内部数据集，不注册到全局列表
    #define GDAL_OF_THREAD_SAFE  0x200 // 线程安全模式（GDAL 3.10+）

使用示例：

.. code-block:: c++

    // 以只读、栅格、共享模式打开
    GDALDatasetH hDS = GDALOpenEx(
        "data.tif",
        GDAL_OF_READONLY | GDAL_OF_RASTER | GDAL_OF_SHARED,
        nullptr,      // 允许的驱动列表（nullptr 表示全部）
        nullptr,      // 打开选项
        nullptr       // 文件前缀（用于子数据集）
    );

    // 以更新模式打开矢量数据
    GDALDatasetH hDS2 = GDALOpenEx(
        "data.gpkg",
        GDAL_OF_UPDATE | GDAL_OF_VECTOR,
        nullptr, nullptr, nullptr
    );

``GDAL_OF_SHARED`` 标志的作用是：在同一进程中，对同一文件多次以共享模式打开时，返回同一个 ``GDALDataset`` 对象的引用，避免重复加载。这在多处代码需要访问同一文件时很有用。


.. _cpl-string-tools:

********************
CPL 字符串工具
********************

CPL提供了丰富的字符串处理工具，包括C++的 ``CPLString`` 类和C语言的字符串列表操作函数。

.. _cplstring-class:

CPLString 类
==============

``CPLString`` 是GDAL中基于 ``std::string`` 的便捷字符串类，提供了许多实用的扩展方法。它可以直接当作 ``const char*`` 使用（提供了隐式转换运算符），在GDAL代码中广泛使用。

主要方法：

- ``Printf()`` ：格式化赋值，类似 ``sprintf``
- ``Trim()`` ：去除首尾空白
- ``toupper()`` / ``tolower()`` ：大小写转换
- ``replaceAll()`` ：字符串替换
- ``ifind()`` ：不区分大小写的查找
- ``endsWith()`` ：判断是否以指定字符串结尾
- ``URLEncode()`` ：URL编码
- ``SQLQuotedIdentifier()`` ：SQL标识符引用（GDAL 3.13 新增）
- ``SQLQuotedLiteral()`` ：SQL字面量引用（GDAL 3.13 新增）

示例：

.. code-block:: c++

    #include "cpl_string.h"

    // Printf 格式化
    CPLString osMsg;
    osMsg.Printf("波段 %d 的数据类型为 %s", 1, "Byte");
    printf("%s\n", osMsg.c_str());  // 输出: 波段 1 的数据类型为 Byte

    // Trim 去除空白
    CPLString osStr = "  hello world  ";
    osStr.Trim();
    printf("'%s'\n", osStr.c_str());  // 输出: 'hello world'

    // 大小写转换
    CPLString osUpper = CPLString("hello").toupper();
    printf("%s\n", osUpper.c_str());  // 输出: HELLO

    // 字符串替换
    CPLString osPath = "/path/to/file.tif";
    osPath.replaceAll(".tif", ".tfw");
    printf("%s\n", osPath.c_str());  // 输出: /path/to/file.tfw

    // SQLQuotedIdentifier (GDAL 3.13+)
    CPLString osIdent = CPLString("my_table").SQLQuotedIdentifier();
    printf("%s\n", osIdent.c_str());  // 输出: "my_table"

    // SQLQuotedLiteral (GDAL 3.13+)
    CPLString osLit = CPLString("it's a test").SQLQuotedLiteral();
    printf("%s\n", osLit.c_str());  // 输出: 'it''s a test'

.. _csl-string-list:

CSL 字符串列表操作
===================

GDAL中大量使用 ``char**`` 类型的字符串列表（StringList），以NULL指针结尾。CPL提供了 ``CSL*`` 系列函数来操作这类列表：

- ``CSLAddString()`` ：向列表末尾添加一个字符串，返回新的列表指针
- ``CSLCount()`` ：返回列表中的字符串数量
- ``CSLDestroy()`` ：释放整个字符串列表的内存
- ``CSLFindString()`` ：在列表中查找字符串，返回索引（未找到返回-1）
- ``CSLTokenizeString()`` ：将字符串按空格分词

构建GDAL打开选项列表的典型用法：

.. code-block:: c++

    #include "cpl_string.h"

    // 构建 open option 列表
    char** papszOptions = nullptr;
    papszOptions = CSLAddString(papszOptions, "NUM_THREADS=ALL_CPUS");
    papszOptions = CSLAddString(papszOptions, "COMPRESS=LZW");
    papszOptions = CSLAddString(papszOptions, "TILED=YES");

    // 遍历列表
    int nCount = CSLCount(papszOptions);
    printf("共有 %d 个选项:\n", nCount);
    for (int i = 0; i < nCount; i++)
    {
        printf("  [%d] %s\n", i, papszOptions[i]);
    }

    // 查找特定选项
    int nIndex = CSLFindString(papszOptions, "COMPRESS=LZW");
    if (nIndex >= 0)
    {
        printf("找到 COMPRESS 选项，索引: %d\n", nIndex);
    }

    // 使用完毕后释放
    CSLDestroy(papszOptions);

.. _csl-name-value:

CSLFetchNameValue / CSLSetNameValue 键值对操作
=================================================

字符串列表可以作为键值对字典使用，格式为 ``"KEY=VALUE"`` 或 ``"KEY:VALUE"`` 。 ``CSLFetchNameValue()`` 和 ``CSLSetNameValue()`` 是操作键值对的主要函数。

.. code-block:: c++

    #include "cpl_string.h"

    // 创建键值对列表
    char** papszMetadata = nullptr;
    papszMetadata = CSLSetNameValue(papszMetadata, "TIFFTAG_IMAGEDESCRIPTION", "Sentinel-2");
    papszMetadata = CSLSetNameValue(papszMetadata, "TIFFTAG_DATETIME", "2024:01:15 10:30:00");
    papszMetadata = CSLSetNameValue(papszMetadata, "AREA_OR_POINT", "Area");

    // 读取键值对
    const char* pszDesc = CSLFetchNameValue(papszMetadata, "TIFFTAG_IMAGEDESCRIPTION");
    if (pszDesc != nullptr)
    {
        printf("图像描述: %s\n", pszDesc);  // 输出: 图像描述: Sentinel-2
    }

    // 设置已存在的键会更新其值
    papszMetadata = CSLSetNameValue(papszMetadata, "AREA_OR_POINT", "Point");
    printf("更新后: %s\n", CSLFetchNameValue(papszMetadata, "AREA_OR_POINT"));

    // 释放
    CSLDestroy(papszMetadata);

.. _cpl-number-conv:

字符串转数值
==============

CPL提供了区域设置无关的字符串转数值函数，确保在不同系统环境下结果一致：

- ``CPLAtof()`` ：字符串转 ``double`` ，使用 ``.`` 作为小数点
- ``CPLAtoGIntBig()`` ：字符串转 ``GIntBig`` （64位整数）

.. code-block:: c++

    #include "cpl_conv.h"

    // 字符串转 double（不受系统 locale 影响）
    double dfValue = CPLAtof("3.14159");
    printf("double: %f\n", dfValue);  // 输出: double: 3.141590

    // 字符串转 64 位整数
    GIntBig nValue = CPLAtoGIntBig("1234567890123");
    printf("GIntBig: " CPL_FRMT_GIB "\n", nValue);  // 输出: GIntBig: 1234567890123

.. _cpl-sprintf:

CPLSPrintf 格式化
===================

``CPLSPrintf()`` 返回一个格式化后的字符串指针，该指针指向一个内部静态缓冲区，不需要手动释放（但不能长期持有，下次调用会被覆盖）。适合用于临时构建字符串。

.. code-block:: c++

    #include "cpl_string.h"

    // CPLSPrintf 用于临时字符串构建
    const char* pszMsg = CPLSPrintf("处理波段 %d / %d，进度 %.1f%%", 3, 10, 30.0);
    printf("%s\n", pszMsg);  // 输出: 处理波段 3 / 10，进度 30.0%

    // 注意：指针在下次 CPLSPrintf 调用后失效
    const char* pszMsg2 = CPLSPrintf("新的消息");
    // 此时 pszMsg 已不可靠，pszMsg2 有效

.. _cpl-enumerate:

cpl::enumerate（GDAL 3.13 新增）
===================================

``cpl::enumerate()`` 是GDAL 3.13新增的C++工具函数，类似于Python的 ``enumerate()`` ，可以在遍历容器时同时获取索引和值。它返回一个可迭代对象，每次迭代产生 ``std::pair<size_t, T&>`` 。

.. code-block:: c++

    #include "cpl_enumerate.h"
    #include "cpl_string.h"
    #include <vector>
    #include <string>

    int main()
    {
        std::vector<std::string> aosFiles = {
            "data_a.tif", "data_b.tif", "data_c.tif"
        };

        // 使用 cpl::enumerate 同时获取索引和值
        for (auto&& [i, filename] : cpl::enumerate(aosFiles))
        {
            printf("处理第 %zu 个文件: %s\n", i, filename.c_str());
        }
        // 输出:
        // 处理第 0 个文件: data_a.tif
        // 处理第 1 个文件: data_b.tif
        // 处理第 2 个文件: data_c.tif

        // 也适用于 CPLStringList
        CPLStringList aosOptions;
        aosOptions.AddString("COMPRESS=LZW");
        aosOptions.AddString("TILED=YES");
        aosOptions.AddString("BLOCKXSIZE=256");

        for (auto&& [i, opt] : cpl::enumerate(aosOptions))
        {
            printf("选项[%zu]: %s\n", i, opt);
        }

        return 0;
    }

``cpl::enumerate()`` 要求C++17或更高版本。它的实现与C++23的 ``std::ranges::enumerate()`` 类似，为GDAL 3.13中需要索引遍历的场景提供了便利。


.. _cpl-error-handling:

********************
错误处理
********************

GDAL使用统一的错误处理机制，通过 ``CPLError()`` 报告错误，通过 ``CPLErr`` 枚举表示错误级别。

.. _cperr-levels:

CPLErr 错误级别
=================

``CPLErr`` 枚举定义了错误的严重程度：

.. code-block:: c++

    typedef enum
    {
        CE_None    = 0,  // 无错误，通常作为函数成功返回值
        CE_Debug   = 1,  // 调试信息，通过 CPLDebug() 输出
        CE_Warning = 2,  // 警告：不阻止当前操作完成，但值得用户注意
        CE_Failure = 3,  // 错误：当前操作失败，但后续操作可能成功
        CE_Fatal   = 4   // 致命错误：进程将被 abort() 终止
    } CPLErr;

相关函数：

- ``CPLError()`` ：报告一个错误
- ``CPLErrorReset()`` ：重置错误状态
- ``CPLGetLastErrorMsg()`` ：获取最近一次错误的消息文本
- ``CPLGetLastErrorNo()`` ：获取最近一次错误的错误码
- ``CPLGetLastErrorType()`` ：获取最近一次错误的级别

.. code-block:: c++

    #include "cpl_error.h"

    // 报告错误
    CPLError(CE_Failure, CPLE_OpenFailed, "无法打开文件: %s", "data.tif");

    // 重置错误状态
    CPLErrorReset();

    // 尝试某个操作后检查错误
    GDALDatasetH hDS = GDALOpen("nonexistent.tif", GA_ReadOnly);
    if (hDS == nullptr)
    {
        const char* pszMsg = CPLGetLastErrorMsg();
        CPLErr eErr = CPLGetLastErrorType();
        printf("错误级别: %d, 消息: %s\n", eErr, pszMsg);
    }

.. _cpl-error-handler:

自定义错误处理
================

GDAL默认将错误输出到stderr。开发者可以通过 ``CPLSetErrorHandler()`` 或 ``CPLPushErrorHandler()`` 设置自定义错误处理器，实现日志记录、错误过滤等功能。

自定义错误处理器的函数签名：

.. code-block:: c++

    typedef void (CPL_STDCALL *CPLErrorHandler)(CPLErr eErrClass,
                                                 CPLErrorNum err_no,
                                                 const char *pszMsg);

内置的错误处理器：

- ``CPLDefaultErrorHandler()`` ：默认处理器，输出到stderr
- ``CPLLoggingErrorHandler()`` ：输出到日志文件
- ``CPLQuietErrorHandler()`` ：静默处理，不输出任何信息

下面是自定义错误处理器的完整示例：

.. code-block:: c++

    #include "gdal.h"
    #include "cpl_error.h"
    #include <cstdio>
    #include <cstring>

    // 自定义错误处理器：将错误信息写入日志
    static void CPL_STDCALL MyErrorHandler(CPLErr eErrClass,
                                            CPLErrorNum err_no,
                                            const char *pszMsg)
    {
        const char* pszLevel = "未知";
        switch (eErrClass)
        {
            case CE_Debug:   pszLevel = "DEBUG"; break;
            case CE_Warning: pszLevel = "WARNING"; break;
            case CE_Failure: pszLevel = "ERROR"; break;
            case CE_Fatal:   pszLevel = "FATAL"; break;
            default: break;
        }

        // 输出到自定义日志（此处简化为 printf）
        printf("[%s] 错误码=%d: %s\n", pszLevel, err_no, pszMsg);

        // 对于致命错误，可以选择终止程序
        if (eErrClass == CE_Fatal)
        {
            printf("遇到致命错误，程序将终止\n");
        }
    }

    int main()
    {
        GDALAllRegister();

        // 设置自定义错误处理器
        CPLErrorHandler pfnOldHandler = CPLSetErrorHandler(MyErrorHandler);

        // 此后所有 GDAL 错误都会经过 MyErrorHandler 处理
        GDALDatasetH hDS = GDALOpen("nonexistent.tif", GA_ReadOnly);
        if (hDS == nullptr)
        {
            printf("打开失败，已通过自定义处理器记录\n");
        }

        // 恢复原来的错误处理器
        CPLSetErrorHandler(pfnOldHandler);

        return 0;
    }

如果希望错误处理器只在某个作用域内生效，可以使用 ``CPLPushErrorHandler()`` 和 ``CPLPopErrorHandler()`` 的栈式管理：

.. code-block:: c++

    // 压入静默错误处理器
    CPLPushErrorHandler(CPLQuietErrorHandler);

    // 此处的错误不会输出到 stderr
    GDALDatasetH hDS = GDALOpen("nonexistent.tif", GA_ReadOnly);

    // 弹出，恢复之前的错误处理器
    CPLPopErrorHandler();

GDAL 3.x 还提供了C++ RAII风格的 ``CPLErrorHandlerPusher`` 类，在构造时压入错误处理器，析构时自动弹出：

.. code-block:: c++

    #include "cpl_error.h"

    void ProcessData()
    {
        // 在此作用域内使用静默错误处理器
        CPLErrorHandlerPusher oPusher(CPLQuietErrorHandler);

        GDALDatasetH hDS = GDALOpen("test.tif", GA_ReadOnly);
        // ... 处理数据 ...
    }
    // 离开作用域后自动恢复之前的错误处理器


.. _cpl-other-tools:

********************
其他工具
********************

.. _cpl-min-max-abs:

CPL_MIN、CPL_MAX、CPL_ABS
============================

GDAL 3.13 对 ``MIN`` 、 ``MAX`` 、 ``ABS`` 宏进行了重命名，使用 ``CPL_`` 前缀以避免与其他库的宏冲突。原来的 ``MIN`` 、 ``MAX`` 、 ``ABS`` 宏已被弃用。

.. code-block:: c++

    #include "cpl_port.h"

    int a = 10, b = 20;

    // 最小值
    int nMin = CPL_MIN(a, b);  // 结果: 10

    // 最大值
    int nMax = CPL_MAX(a, b);  // 结果: 20

    // 绝对值
    int nVal = -42;
    int nAbs = CPL_ABS(nVal);  // 结果: 42

    // 也适用于浮点数
    double dfMin = CPL_MIN(3.14, 2.71);  // 结果: 2.71

.. note::

    GDAL 3.13 起， ``MIN`` 、 ``MAX`` 、 ``ABS`` 宏已被标记为弃用，请使用 ``CPL_MIN`` 、 ``CPL_MAX`` 、 ``CPL_ABS`` 替代。

.. _gintbig:

GIntBig：64位整数类型
========================

``GIntBig`` 是GDAL中定义的64位有理整数类型，在所有平台上保证为64位：

.. code-block:: c++

    #include "cpl_port.h"

    // GIntBig 是 long long 的 typedef
    GIntBig nFileSize = 4294967296LL;  // 4GB

    // 打印 GIntBig 的格式化宏
    printf("文件大小: " CPL_FRMT_GIB " 字节\n", nFileSize);

    // GIntBig 的最大值和最小值
    GIntBig nMax = GINTBIG_MAX;   // 9223372036854775807
    GIntBig nMin = GINTBIG_MIN;   // -9223372036854775808

.. _cpl-memory:

CPLMalloc、CPLCalloc、CPLFree 内存分配
==========================================

CPL提供了内存分配函数，与标准C的 ``malloc`` / ``calloc`` / ``free`` 接口一致，但增加了错误检查：分配失败时会调用 ``CPLError(CE_Fatal, ...)`` 终止程序，而不是返回NULL。

.. code-block:: c++

    #include "cpl_conv.h"

    // CPLMalloc：分配指定大小的内存（不初始化）
    void* pData = CPLMalloc(1024);

    // CPLCalloc：分配并清零（nmemb 个元素，每个 size 字节）
    void* pZeroData = CPLCalloc(100, sizeof(double));  // 100个double，全部初始化为0

    // CPLRealloc：重新分配内存
    pData = CPLRealloc(pData, 2048);

    // CPLFree：释放内存（实际上是 VSIFree 的别名）
    CPLFree(pData);
    CPLFree(pZeroData);

.. tip::

    在C++代码中，推荐使用 ``std::vector`` 或 ``std::unique_ptr`` 等RAII容器管理内存，避免手动调用 ``CPLMalloc`` / ``CPLFree`` 。在需要与C API交互时再使用这些函数。

.. _cplstringlist-class:

CPLStringList 类
==================

``CPLStringList`` 是对 ``char**`` 字符串列表的C++封装类，提供了更方便的操作接口。它内部维护一个 ``char**`` 列表，可以无缝转换为C风格的 ``char**`` 或 ``CSLConstList`` 。

主要特性：

- 支持 ``push_back()`` / ``AddString()`` 添加字符串
- 支持 ``operator[]`` 按索引或按键名访问
- 支持键值对操作 ``AddNameValue()`` / ``SetNameValue()`` / ``FetchNameValue()``
- 支持范围遍历（ ``begin()`` / ``end()`` ）
- 支持排序 ``Sort()``
- ``StealList()`` 可以转移所有权给调用者

.. code-block:: c++

    #include "cpl_string.h"

    // 创建 CPLStringList
    CPLStringList aosOptions;
    aosOptions.AddString("DRIVER=GTiff");
    aosOptions.AddString("SIZE=1024");

    // 键值对操作
    aosOptions.SetNameValue("COMPRESS", "LZW");
    aosOptions.SetNameValue("TILED", "YES");

    // 按索引访问
    printf("第一个选项: %s\n", aosOptions[0]);  // 输出: DRIVER=GTiff

    // 按键名访问
    const char* pszCompress = aosOptions.FetchNameValue("COMPRESS");
    printf("压缩方式: %s\n", pszCompress);  // 输出: LZW

    // 使用 operator[] 按键名访问（GDAL 3.x）
    const char* pszTiled = aosOptions["TILED"];
    printf("是否分块: %s\n", pszTiled);  // 输出: YES

    // 范围遍历
    for (const char* pszOpt : aosOptions)
    {
        printf("  %s\n", pszOpt);
    }

    // 转换为 C 风格 char**（所有权仍在 CPLStringList 中）
    char** papszList = aosOptions.List();

    // 从 C 风格列表构造
    char** papszRaw = nullptr;
    papszRaw = CSLAddString(papszRaw, "A=1");
    papszRaw = CSLAddString(papszRaw, "B=2");
    CPLStringList aosFromRaw(papszRaw, TRUE);  // TRUE 表示接管所有权
    // 注意：接管所有权后，不要再次 CSLDestroy(papszRaw)

    // 从 vector 构造
    std::vector<std::string> aosVec = {"X=10", "Y=20", "Z=30"};
    CPLStringList aosFromVec(aosVec);
