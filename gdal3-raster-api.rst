.. _gdal3-raster-api:

################################
GDAL 3.x 栅格 API 变化
################################

GDAL 3.x 系列在栅格数据 API 方面引入了大量改进，包括新的打开接口、智能指针管理、
多维数组 API（RFC 75）、线程安全（RFC 101）、新数据类型等。本文档详细介绍这些变化
及对应的代码示例。

.. contents:: 目录
   :depth: 2
   :local:

***********************************
GDALOpenEx() 替代 GDALOpen()
***********************************

从 GDAL 2.0 开始引入、在 GDAL 3.x 中成为推荐方式的 ``GDALOpenEx()`` 提供了比传统
``GDALOpen()`` 更灵活的打开选项，包括：

- **打开标志**：控制只读/更新、共享访问等行为
- **驱动限制**：指定允许尝试的驱动列表，避免不必要的探测开销
- **多数据集访问**：通过 ``GDAL_OF_MULTIDIM_RASTER`` 标志打开多维数组访问

打开标志
================

常用的打开标志包括：

- ``GDAL_OF_READONLY``：以只读模式打开
- ``GDAL_OF_UPDATE``：以读写模式打开
- ``GDAL_OF_SHARED``：使用共享模式打开（同一路径返回同一 GDALDataset 句柄）
- ``GDAL_OF_VERBOSE_ERROR``：打开失败时输出详细错误信息
- ``GDAL_OF_MULTIDIM_RASTER``：以多维数组模式打开（详见后文）

C++ 示例
================

.. code-block:: c++

   #include "gdal_priv.h"
   #include <iostream>

   int main()
   {
       // 基本的只读打开
       GDALDataset* poDS = static_cast<GDALDataset*>(
           GDALOpenEx("input.tif", GDAL_OF_READONLY, nullptr, nullptr, nullptr));
       if (poDS == nullptr) {
           std::cerr << "无法打开数据集: input.tif" << std::endl;
           return 1;
       }
       std::cout << "波段数: " << poDS->GetRasterCount() << std::endl;
       std::cout << "尺寸: " << poDS->GetRasterXSize()
                 << " x " << poDS->GetRasterYSize() << std::endl;
       GDALClose(poDS);

       // 限制驱动为 GeoTIFF 和 PNG，避免全驱动探测
       const char* apszAllowedDrivers[] = {"GTiff", "PNG", nullptr};
       poDS = static_cast<GDALDataset*>(
           GDALOpenEx("image.tif", GDAL_OF_READONLY,
                      apszAllowedDrivers, nullptr, nullptr));
       if (poDS != nullptr) {
           std::cout << "使用受限驱动打开成功" << std::endl;
           GDALClose(poDS);
       }

       // 以更新模式打开
       poDS = static_cast<GDALDataset*>(
           GDALOpenEx("output.tif", GDAL_OF_UPDATE | GDAL_OF_VERBOSE_ERROR,
                      nullptr, nullptr, nullptr));
       if (poDS != nullptr) {
           // 可以写入数据...
           GDALClose(poDS);
       }

       return 0;
   }

C 示例
============

.. code-block:: c

   #include "gdal.h"
   #include <stdio.h>

   int main()
   {
       GDALDatasetH hDS;

       /* 基本只读打开 */
       hDS = GDALOpenEx("input.tif", GDAL_OF_READONLY, NULL, NULL, NULL);
       if (hDS == NULL) {
           fprintf(stderr, "无法打开数据集\n");
           return 1;
       }
       printf("波段数: %d\n", GDALGetRasterCount(hDS));
       GDALClose(hDS);

       /* 限制驱动 */
       const char* apszDrivers[] = {"GTiff", "PNG", NULL};
       hDS = GDALOpenEx("image.tif", GDAL_OF_READONLY, apszDrivers, NULL, NULL);
       if (hDS != NULL) {
           printf("受限驱动打开成功\n");
           GDALClose(hDS);
       }

       return 0;
   }

多数据集访问
========================

通过 ``GDAL_OF_MULTIDIM_RASTER`` 标志，可以以多维数组模式打开数据集，访问
NetCDF/HDF5 等格式中的多维数据。这将在后文多维数组 API 部分详细介绍。

.. code-block:: c++

   // 以多维数组模式打开
   GDALDataset* poDS = static_cast<GDALDataset*>(
       GDALOpenEx("dataset.nc", GDAL_OF_READONLY | GDAL_OF_MULTIDIM_RASTER,
                  nullptr, nullptr, nullptr));

*****************************************
GDALDatasetUniquePtr 智能指针管理
*****************************************

GDAL 3.x 提供了 ``GDALDatasetUniquePtr`` 智能指针（基于 C++ ``std::unique_ptr``），
用于自动管理 GDALDataset 的生命周期，避免忘记调用 ``GDALClose()`` 导致的资源泄漏。

``GDALDatasetUniquePtr`` 定义在 ``gdal_priv.h`` 中，是一个带有自定义删除器的
``std::unique_ptr``，删除器会自动调用 ``GDALClose()``。

基本用法
==============

.. code-block:: c++

   #include "gdal_priv.h"
   #include <iostream>

   void processDataset()
   {
       // 使用 GDALDatasetUniquePtr 管理数据集
       GDALDatasetUniquePtr poDS(
           GDALDataset::Open("input.tif", GDAL_OF_READONLY));
       if (poDS == nullptr) {
           std::cerr << "无法打开数据集" << std::endl;
           return;
       }

       // 正常使用数据集
       std::cout << "波段数: " << poDS->GetRasterCount() << std::endl;

       GDALRasterBand* poBand = poDS->GetRasterBand(1);
       std::cout << "数据类型: " << GDALGetDataTypeName(poBand->GetRasterDataType())
                 << std::endl;

       // 函数结束时自动调用 GDALClose()，无需手动释放
   }

配合 GDALOpenEx 使用
=============================

.. code-block:: c++

   #include "gdal_priv.h"

   void readWithSmartPtr()
   {
       // GDALOpenEx 返回的是 GDALDatasetH，需要转型
       GDALDatasetUniquePtr poDS(static_cast<GDALDataset*>(
           GDALOpenEx("data.vrt", GDAL_OF_READONLY, nullptr, nullptr, nullptr)));

       if (poDS == nullptr) {
           return;
       }

       // 可以安全地在任何提前 return 的路径上使用，不会泄漏
       if (poDS->GetRasterCount() == 0) {
           return;  // 智能指针自动释放
       }

       // 处理数据...
       GDALRasterBand* poBand = poDS->GetRasterBand(1);
       int nXSize = poBand->GetXSize();
       int nYSize = poBand->GetYSize();

       std::vector<float> afData(static_cast<size_t>(nXSize) * nYSize);
       poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize,
                        afData.data(), nXSize, nYSize, GDT_Float32, 0, 0);

       // 无需手动 GDALClose()
   }

工厂方法返回智能指针
===========================

GDAL 3.x 的一些工厂方法也直接返回 ``GDALDatasetUniquePtr``：

.. code-block:: c++

   #include "gdal_priv.h"

   void createDataset()
   {
       GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
       if (poDriver == nullptr) return;

       // Create() 返回的 unique_ptr 会在作用域结束时自动关闭
       GDALDatasetUniquePtr poDS(poDriver->Create(
           "output.tif", 256, 256, 1, GDT_Byte, nullptr));

       if (poDS == nullptr) return;

       // 设置地理变换、投影等...
       double adfGeoTransform[6] = {0.0, 1.0, 0.0, 256.0, 0.0, -1.0};
       poDS->SetGeoTransform(adfGeoTransform);

       // 写入数据...

       // 不需要手动 GDALClose()
   }

*******************
RasterIO() 完整用法
*******************

``GDALDataset::RasterIO()`` 和 ``GDALRasterBand::RasterIO()`` 是 GDAL 中读写
栅格数据的核心接口。

GDALRasterBand::RasterIO()
==========================

用于对单个波段进行读写操作。

.. code-block:: c++

   CPLErr GDALRasterBand::RasterIO(
       GDALRWFlag eRWFlag,        // GF_Read 或 GF_Write
       int nXOff, int nYOff,       // 读写区域在栅格中的偏移（像素）
       int nXSize, int nYSize,     // 读写区域的大小（像素）
       void* pData,                // 数据缓冲区
       int nBufXSize, int nBufYSize, // 缓冲区大小
       GDALDataType eBufType,      // 缓冲区数据类型
       GSpacing nPixelSpace,       // 像素间距（字节），0 表示紧凑排列
       GSpacing nLineSpace         // 行间距（字节），0 表示紧凑排列
   );

GDALDataset::RasterIO()
=======================

用于对整个数据集（多个波段）进行读写操作。

.. code-block:: c++

   CPLErr GDALDataset::RasterIO(
       GDALRWFlag eRWFlag,
       int nXOff, int nYOff,
       int nXSize, int nYSize,
       void* pData,
       int nBufXSize, int nBufYSize,
       GDALDataType eBufType,
       int nBandCount,             // 要读写的波段数
       int* panBandMap,            // 波段索引数组（从 1 开始）
       GSpacing nPixelSpace,
       GSpacing nLineSpace,
       GSpacing nBandSpace         // 波段间距（字节）
   );

C++ 读取示例
============

.. code-block:: c++

   #include "gdal_priv.h"
   #include <vector>
   #include <iostream>

   void readSingleBand()
   {
       GDALDatasetUniquePtr poDS(
           GDALDataset::Open("dem.tif", GDAL_OF_READONLY));
       if (poDS == nullptr) return;

       GDALRasterBand* poBand = poDS->GetRasterBand(1);
       int nXSize = poBand->GetXSize();
       int nYSize = poBand->GetYSize();

       // 读取整个波段为 Float32
       std::vector<float> afData(static_cast<size_t>(nXSize) * nYSize);
       CPLErr err = poBand->RasterIO(
           GF_Read,
           0, 0,               // 从左上角开始
           nXSize, nYSize,     // 读取整个波段
           afData.data(),       // 输出缓冲区
           nXSize, nYSize,     // 缓冲区大小 = 栅格大小（无缩放）
           GDT_Float32,        // 输出为 Float32
           0,                   // 像素间距（0 = 紧凑）
           0                    // 行间距（0 = 紧凑）
       );

       if (err != CE_None) {
           std::cerr << "读取失败" << std::endl;
           return;
       }

       // 访问像素 (10, 20)
       float value = afData[20 * nXSize + 10];
       std::cout << "像素 (10,20) = " << value << std::endl;
   }

   void readWithBufferScaling()
   {
       GDALDatasetUniquePtr poDS(
           GDALDataset::Open("large_raster.tif", GDAL_OF_READONLY));
       if (poDS == nullptr) return;

       GDALRasterBand* poBand = poDS->GetRasterBand(1);
       int nXSize = poBand->GetXSize();  // 例如 10000
       int nYSize = poBand->GetYSize();  // 例如 10000

       // 将大栅格缩放到 256x256 的缓冲区中读取（自动重采样）
       int nBufX = 256, nBufY = 256;
       std::vector<uint8_t> aubData(static_cast<size_t>(nBufX) * nBufY);

       CPLErr err = poBand->RasterIO(
           GF_Read,
           0, 0, nXSize, nYSize,    // 读取整个栅格
           aubData.data(),
           nBufX, nBufY,            // 缩放到 256x256
           GDT_Byte,
           0, 0
       );

       // 当 nBufXSize != nXSize 或 nBufYSize != nYSize 时，
       // GDAL 会自动使用最邻近插值进行缩放
       // 从 GDAL 3.13 开始，默认重采样算法改为输出缓冲区类型匹配
   }

C 读取示例
==========

.. code-block:: c

   #include "gdal.h"
   #include <stdlib.h>
   #include <stdio.h>

   void readSingleBandC()
   {
       GDALDatasetH hDS = GDALOpenEx("dem.tif", GDAL_OF_READONLY, NULL, NULL, NULL);
       if (hDS == NULL) return;

       GDALRasterBandH hBand = GDALGetRasterBand(hDS, 1);
       int nXSize = GDALGetRasterBandXSize(hBand);
       int nYSize = GDALGetRasterBandYSize(hBand);

       float* pafData = (float*)malloc(sizeof(float) * nXSize * nYSize);
       if (pafData == NULL) { GDALClose(hDS); return; }

       CPLErr err = GDALRasterIO(
           hBand, GF_Read,
           0, 0, nXSize, nYSize,
           pafData, nXSize, nYSize, GDT_Float32,
           0, 0);

       if (err != CE_None) {
           fprintf(stderr, "读取失败\n");
       } else {
           printf("像素 (0,0) = %f\n", pafData[0]);
       }

       free(pafData);
       GDALClose(hDS);
   }

C++ 写入示例
============

.. code-block:: c++

   #include "gdal_priv.h"
   #include <vector>

   void writeRasterData()
   {
       GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
       if (poDriver == nullptr) return;

       int nXSize = 512, nYSize = 512;
       GDALDatasetUniquePtr poDS(poDriver->Create(
           "output.tif", nXSize, nYSize, 1, GDT_Float32, nullptr));
       if (poDS == nullptr) return;

       // 创建一些示例数据
       std::vector<float> afData(static_cast<size_t>(nXSize) * nYSize);
       for (int y = 0; y < nYSize; y++) {
           for (int x = 0; x < nXSize; x++) {
               afData[y * nXSize + x] = static_cast<float>(x + y);
           }
       }

       // 写入数据
       GDALRasterBand* poBand = poDS->GetRasterBand(1);
       CPLErr err = poBand->RasterIO(
           GF_Write,
           0, 0, nXSize, nYSize,
           afData.data(), nXSize, nYSize,
           GDT_Float32, 0, 0);

       // 设置 NoData 值
       poBand->SetNoDataValue(-9999.0);
   }

多波段读取
==========

.. code-block:: c++

   void readMultiBand()
   {
       GDALDatasetUniquePtr poDS(
           GDALDataset::Open("rgb_image.tif", GDAL_OF_READONLY));
       if (poDS == nullptr) return;

       int nXSize = poDS->GetRasterXSize();
       int nYSize = poDS->GetRasterYSize();
       int nBands = poDS->GetRasterCount();  // 通常为 3 (R, G, B)

       // 使用 GDALDataset::RasterIO() 一次读取所有波段
       // 数据按波段优先排列：RRRRRR...GGGGG...BBBBB...
       std::vector<uint8_t> aubData(
           static_cast<size_t>(nXSize) * nYSize * nBands);

       int anBandMap[] = {1, 2, 3};  // 波段索引（从 1 开始）
       CPLErr err = poDS->RasterIO(
           GF_Read,
           0, 0, nXSize, nYSize,
           aubData.data(),
           nXSize, nYSize,
           GDT_Byte,
           nBands, anBandMap,     // 读取 3 个波段
           0,                     // 像素间距
           0,                     // 行间距
           0                      // 波段间距（0 = 紧凑排列）
       );

       // 访问像素 (x, y) 的红色分量
       int x = 100, y = 200;
       uint8_t red = aubData[0 * nXSize * nYSize + y * nXSize + x];
       uint8_t green = aubData[1 * nXSize * nYSize + y * nXSize + x];
       uint8_t blue = aubData[2 * nXSize * nYSize + y * nXSize + x];
   }

像素间距与行间距的高级用法
==========================

通过设置非零的 ``nPixelSpace`` 和 ``nLineSpace``，可以实现交错排列（interleaved）
数据的读写，这在图像处理中非常有用。

.. code-block:: c++

   void readInterleaved()
   {
       GDALDatasetUniquePtr poDS(
           GDALDataset::Open("rgb.tif", GDAL_OF_READONLY));
       if (poDS == nullptr) return;

       int nXSize = poDS->GetRasterXSize();
       int nYSize = poDS->GetRasterYSize();

       // 交错排列：RGBRGBRGB...（像素交叉）
       std::vector<uint8_t> aubData(static_cast<size_t>(nXSize) * nYSize * 3);

       int anBandMap[] = {1, 2, 3};
       CPLErr err = poDS->RasterIO(
           GF_Read,
           0, 0, nXSize, nYSize,
           aubData.data(),
           nXSize, nYSize,
           GDT_Byte,
           3, anBandMap,
           3,                     // 像素间距 = 3 字节（每像素 3 字节）
           nXSize * 3,            // 行间距 = 每行的总字节数
           1                      // 波段间距 = 1 字节（波段值交错排列）
       );

       // 访问像素 (x, y) 的 RGB
       int x = 50, y = 100;
       size_t offset = (y * nXSize + x) * 3;
       uint8_t r = aubData[offset + 0];
       uint8_t g = aubData[offset + 1];
       uint8_t b = aubData[offset + 2];
   }

***************************
GDAL 3.13 RasterIO 行为变更
***************************

从 GDAL 3.13 开始，当使用 ``RasterIO()`` 进行缩放读取时（即栅格尺寸与缓冲区尺寸不同），
**默认在输出缓冲区类型中进行重采样**，而不是像之前那样在栅格原始数据类型中重采样。

这意味着：

- 如果栅格数据类型为 ``GDT_Float64``，缓冲区类型为 ``GDT_Byte``，缩放后的像素值会先
  转换为 ``GDT_Byte`` 再进行重采样
- 之前的行为是先重采样（在 Float64 中），然后再转换为 Byte

此变更使行为更符合直觉——输出缓冲区中得到的值已经经过类型转换，避免了中间步骤的精度损失
带来的意外结果。

如果需要旧行为，可以显式设置重采样算法：

.. code-block:: c++

   // 使用 GDAL 3.13+ 的新行为（默认）
   // 输出缓冲区类型中重采样
   poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize,
                    pBuf, nBufXSize, nBufYSize, GDT_Byte, 0, 0);

   // 如果需要更精细的控制，可以通过 GDALDataset::RasterIO() 的
   // psExtraArg 参数指定重采样算法

************
新数据类型
************

GDAL 3.x 陆续引入了多种新的栅格数据类型：

GDT_Int8（GDAL 3.7）
=====================

``GDT_Int8`` 表示有符号 8 位整数，值域为 -128 到 127。在 GDAL 3.7 之前，8 位数据
只有无符号的 ``GDT_Byte`` （0-255）。

.. code-block:: c++

   // 检查数据类型
   GDALRasterBand* poBand = poDS->GetRasterBand(1);
   GDALDataType eType = poBand->GetRasterDataType();
   if (eType == GDT_Int8) {
       std::cout << "这是有符号 8 位整数数据" << std::endl;
   }

   // 创建 Int8 数据集
   GDALDatasetUniquePtr poDS(poDriver->Create(
       "signed_byte.tif", 256, 256, 1, GDT_Int8, nullptr));

GDT_UInt8（GDAL 3.13）
=======================

``GDT_UInt8`` 是 ``GDT_Byte`` 的别名，用于明确表示无符号 8 位整数。``GDT_Byte``
将继续保留以保持向后兼容。

GDT_Float16（RFC 100，GDAL 3.11）
====================================

``GDT_Float16`` 表示 IEEE 754 半精度浮点数（16 位），在深度学习和遥感数据中越来越常用。

.. code-block:: c++

   // 读取 Float16 数据到 Float32 缓冲区
   std::vector<float> afBuf(nXSize * nYSize);
   poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize,
                    afBuf.data(), nXSize, nYSize, GDT_Float32, 0, 0);
   // GDAL 自动处理 Float16 → Float32 的转换

数据类型大小查询
================

.. code-block:: c++

   // 查询数据类型的字节大小
   size_t nSize = GDALGetDataTypeSizeBytes(GDT_Int8);     // 1
   size_t nSize2 = GDALGetDataTypeSizeBytes(GDT_Float16); // 2
   size_t nSize3 = GDALGetDataTypeSizeBytes(GDT_Float32); // 4
   size_t nSize4 = GDALGetDataTypeSizeBytes(GDT_Float64); // 8

   // 查询是否为整数类型
   int bIsInt = GDALDataTypeIsInteger(GDT_Int8);   // TRUE
   int bIsFloat = GDALDataTypeIsFloating(GDT_Float16); // TRUE

********************************
GDALGeoTransform 类（GDAL 3.12）
********************************

从 GDAL 3.12 开始，引入了 ``GDALGeoTransform`` 类来替代传统的 ``double[6]`` 数组。
这个类提供了更安全、更易用的接口来管理地理变换参数。

传统的 ``double[6]`` 数组含义：

- [0]: 左上角 X 坐标（原点 X）
- [1]: 像素宽度（X 方向分辨率）
- [2]: 行旋转
- [3]: 左上角 Y 坐标（原点 Y）
- [4]: 列旋转
- [5]: 像素高度（Y 方向分辨率，通常为负值）

基本用法
========

.. code-block:: c++

   #include "gdal_priv.h"

   void useGeoTransform()
   {
       GDALDatasetUniquePtr poDS(
           GDALDataset::Open("georef.tif", GDAL_OF_READONLY));
       if (poDS == nullptr) return;

       // 旧方式：double adfGeoTransform[6];
       // 新方式：使用 GDALGeoTransform
       GDALGeoTransform gt;
       if (poDS->GetGeoTransform(gt) != CE_None) {
           std::cerr << "无法获取地理变换" << std::endl;
           return;
       }

       // 访问各个参数
       double dfOriginX = gt[0];   // 左上角 X
       double dfPixelWidth = gt[1]; // 像素宽度
       double dfRotation1 = gt[2];  // 行旋转
       double dfOriginY = gt[3];   // 左上角 Y
       double dfRotation2 = gt[4];  // 列旋转
       double dfPixelHeight = gt[5]; // 像素高度（通常为负）

       std::cout << "原点: (" << dfOriginX << ", " << dfOriginY << ")" << std::endl;
       std::cout << "分辨率: " << dfPixelWidth << " x " << dfPixelHeight << std::endl;
   }

像素坐标与地理坐标转换
========================

.. code-block:: c++

   void coordinateTransform()
   {
       GDALGeoTransform gt;
       // 假设已从数据集获取了 gt

       // 像素坐标 → 地理坐标
       // GeoX = gt[0] + PixelX * gt[1] + PixelY * gt[2]
       // GeoY = gt[3] + PixelX * gt[4] + PixelY * gt[5]
       double dfPixelX = 100.0, dfPixelY = 200.0;
       double dfGeoX = gt[0] + dfPixelX * gt[1] + dfPixelY * gt[2];
       double dfGeoY = gt[3] + dfPixelX * gt[4] + dfPixelY * gt[5];

       // 使用 GDALGeoTransform 的辅助方法
       double dfGeoX2, dfGeoY2;
       gt.Transform(dfGeoX2, dfGeoY2, dfPixelX, dfPixelY);

       // 地理坐标 → 像素坐标（需要求逆）
       GDALGeoTransform gtInv;
       if (gtInv.Inverse(gt)) {
           double dfPixX, dfPixY;
           gtInv.Transform(dfPixX, dfPixY, dfGeoX, dfGeoY);
           // dfPixX ≈ 100.0, dfPixY ≈ 200.0
       }
   }

向后兼容
==========

旧的 ``double[6]`` 方式仍然完全支持。``GDALGeoTransform`` 提供了隐式转换，可以在
接受 ``const double*`` 的旧 API 中使用。

.. code-block:: c++

   // 旧代码仍然有效
   double adfGT[6];
   poDS->GetGeoTransform(adfGT);

   // 新旧混用也可以
   GDALGeoTransform gt;
   poDS->GetGeoTransform(gt);
   // 可以隐式转换为 const double*
   poDS->SetGeoTransform(gt);

***************************
GDALCloseEx()（GDAL 3.13）
***************************

GDAL 3.13 引入了 ``GDALCloseEx()``，在关闭数据集时支持进度回调，适用于长时间关闭操作
（例如需要刷新大量缓存数据的情况）。

C++ 示例
========

.. code-block:: c++

   #include "gdal_priv.h"

   int CloseProgressFunc(double dfProgress, const char* pszMessage, void* pUserData)
   {
       printf("\r关闭进度: %.1f%%", dfProgress * 100);
       if (pszMessage) printf(" - %s", pszMessage);
       fflush(stdout);
       return TRUE;  // 返回 FALSE 可以中止关闭操作
   }

   void closeWithProgress()
   {
       GDALDataset* poDS = static_cast<GDALDataset*>(
           GDALOpenEx("large_dataset.tif", GDAL_OF_UPDATE, nullptr, nullptr, nullptr));
       if (poDS == nullptr) return;

       // 做一些修改操作...
       // ...

       // 带进度回调关闭
       GDALCloseEx(poDS, GDALCloseExFlags::GDALCEN_None,
                    CloseProgressFunc, nullptr);
       printf("\n");
   }

C 示例
======

.. code-block:: c

   #include "gdal.h"

   int CPL_DLL closeProgress(double dfProgress, const char* pszMsg, void* pUser)
   {
       printf("关闭: %.0f%%\n", dfProgress * 100);
       return TRUE;
   }

   void closeWithProgressC()
   {
       GDALDatasetH hDS = GDALOpenEx("data.tif", GDAL_OF_UPDATE,
                                      NULL, NULL, NULL);
       if (hDS == NULL) return;

       /* 带进度回调关闭 */
       GDALCloseEx(hDS, GDALCEN_None, closeProgress, NULL);
   }

*******************
线程安全（RFC 101）
*******************

GDAL 3.x 的 RFC 101 改进了多线程环境下的安全性。以下是使用多线程 ``RasterIO()`` 时
需要注意的事项：

注意事项
========

1. **同一 GDALDataset 不同波段的并发读取**：从 GDAL 3.x 开始，可以安全地从不同线程
   同时读取同一数据集的不同波段
2. **同一波段的并发读取**：对于某些驱动（如 GeoTIFF），可以在同一波段上并发进行
   ``RasterIO(GF_Read, ...)`` 操作
3. **写入操作**：写入操作通常不是线程安全的，应避免并发写入
4. **驱动差异**：不同驱动的线程安全支持程度不同，应查阅具体驱动文档

多线程读取示例
===============

.. code-block:: c++

   #include "gdal_priv.h"
   #include <thread>
   #include <vector>
   #include <mutex>

   std::mutex g_mutex;

   void readBandThread(GDALDataset* poDS, int nBandIndex,
                       std::vector<float>& result)
   {
       GDALRasterBand* poBand = poDS->GetRasterBand(nBandIndex);
       int nXSize = poBand->GetXSize();
       int nYSize = poBand->GetYSize();

       result.resize(static_cast<size_t>(nXSize) * nYSize);

       // 不同波段可以安全地并发读取
       CPLErr err = poBand->RasterIO(
           GF_Read, 0, 0, nXSize, nYSize,
           result.data(), nXSize, nYSize, GDT_Float32, 0, 0);

       if (err != CE_None) {
           std::lock_guard<std::mutex> lock(g_mutex);
           std::cerr << "波段 " << nBandIndex << " 读取失败" << std::endl;
       }
   }

   void multiThreadedRead()
   {
       GDALDatasetUniquePtr poDS(
           GDALDataset::Open("multiband.tif", GDAL_OF_READONLY));
       if (poDS == nullptr) return;

       int nBands = poDS->GetRasterCount();
       std::vector<std::vector<float>> bandData(nBands);
       std::vector<std::thread> threads;

       for (int i = 0; i < nBands; i++) {
           threads.emplace_back(readBandThread, poDS.get(), i + 1,
                                std::ref(bandData[i]));
       }

       for (auto& t : threads) {
           t.join();
       }

       std::cout << "已并发读取 " << nBands << " 个波段" << std::endl;
   }

************************
NoData 与 Mask Band 操作
************************

GDAL 提供了完善的 NoData 值和掩码波段机制来处理无效像素。

SetNoDataValue / GetNoDataValue
=================================

.. code-block:: c++

   #include "gdal_priv.h"

   void noDataOperations()
   {
       GDALDatasetUniquePtr poDS(
           GDALDataset::Open("dem.tif", GDAL_OF_UPDATE));
       if (poDS == nullptr) return;

       GDALRasterBand* poBand = poDS->GetRasterBand(1);

       // 获取 NoData 值
       int bHasNoData = FALSE;
       double dfNoData = poBand->GetNoDataValue(&bHasNoData);
       if (bHasNoData) {
           std::cout << "NoData 值: " << dfNoData << std::endl;
       } else {
           std::cout << "未设置 NoData 值" << std::endl;
       }

       // 设置 NoData 值
       poBand->SetNoDataValue(-9999.0);

       // GDAL 3.5+: 使用 64 位 NoData 值（适用于大整数）
       // poBand->SetNoDataValueAsInt64(GDAL_PAM_DEFAULT_NODATA_VALUE_INT64);
       // poBand->SetNoDataValueAsUInt64(GDAL_PAM_DEFAULT_NODATA_VALUE_UINT64);
   }

C 示例
======

.. code-block:: c

   #include "gdal.h"
   #include <stdio.h>

   void noDataOperationsC()
   {
       GDALDatasetH hDS = GDALOpenEx("dem.tif", GDAL_OF_UPDATE, NULL, NULL, NULL);
       if (hDS == NULL) return;

       GDALRasterBandH hBand = GDALGetRasterBand(hDS, 1);

       /* 获取 NoData */
       int bHasNoData = FALSE;
       double dfNoData = GDALGetRasterNoDataValue(hBand, &bHasNoData);
       if (bHasNoData) {
           printf("NoData = %f\n", dfNoData);
       }

       /* 设置 NoData */
       GDALSetRasterNoDataValue(hBand, -9999.0);

       GDALClose(hDS);
   }

GetMaskBand
============

掩码波段（Mask Band）提供了逐像素的有效性信息，比 NoData 值更灵活。

.. code-block:: c++

   void maskBandOperations()
   {
       GDALDatasetUniquePtr poDS(
           GDALDataset::Open("raster.tif", GDAL_OF_READONLY));
       if (poDS == nullptr) return;

       GDALRasterBand* poBand = poDS->GetRasterBand(1);

       // 获取掩码波段
       GDALRasterBand* poMask = poBand->GetMaskBand();
       if (poMask == nullptr) {
           std::cout << "无掩码波段" << std::endl;
           return;
       }

       int nXSize = poMask->GetXSize();
       int nYSize = poMask->GetYSize();

       // 读取掩码数据（0 = 无效，255 = 有效）
       std::vector<uint8_t> aubMask(static_cast<size_t>(nXSize) * nYSize);
       poMask->RasterIO(GF_Read, 0, 0, nXSize, nYSize,
                        aubMask.data(), nXSize, nYSize, GDT_Byte, 0, 0);

       // 检查掩码标志
       int nFlags = poBand->GetMaskFlags();
       if (nFlags & GMF_ALL_VALID) {
           std::cout << "所有像素均有效" << std::endl;
       }
       if (nFlags & GMF_NODATA) {
           std::cout << "掩码基于 NoData 值" << std::endl;
       }
       if (nFlags & GMF_PER_DATASET) {
           std::cout << "掩码为数据集级别共享" << std::endl;
       }
   }

创建掩码波段
=============

.. code-block:: c++

   void createMaskBand()
   {
       GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
       if (poDriver == nullptr) return;

       GDALDatasetUniquePtr poDS(poDriver->Create(
           "output_with_mask.tif", 256, 256, 1, GDT_Float32, nullptr));
       if (poDS == nullptr) return;

       // 启用数据集级别的 Alpha/Mask 波段
       // 对于 GTiff 驱动，需要在创建选项中指定
       const char* apszOptions[] = {"ALPHA=YES", nullptr};
       GDALDatasetUniquePtr poDS2(poDriver->Create(
           "with_alpha.tif", 256, 256, 2, GDT_Byte, apszOptions));
       // 波段 1 = 数据，波段 2 = Alpha（自动成为掩码波段）

       // 或者显式创建掩码波段
       // 需要驱动支持 CreateMaskBand()
       poDS->GetRasterBand(1)->CreateMaskBand(GMF_PER_DATASET);
   }

***************
Overview 操作
***************

金字塔（Overview）是预先计算的低分辨率版本，用于加速数据浏览。

BuildOverviews
================

.. code-block:: c++

   #include "gdal_priv.h"

   void buildOverviews()
   {
       // 必须以更新模式打开
       GDALDatasetUniquePtr poDS(
           GDALDataset::Open("large_raster.tif", GDAL_OF_UPDATE));
       if (poDS == nullptr) return;

       // 指定缩放因子：2x, 4x, 8x, 16x
       int anOverviewList[] = {2, 4, 8, 16};
       int nOverviews = sizeof(anOverviewList) / sizeof(int);

       // 使用默认重采样算法（NEAREST）构建金字塔
       CPLErr err = poDS->BuildOverviews(
           "NEAREST",           // 重采样算法: NEAREST, AVERAGE, GAUSS, CUBIC, etc.
           nOverviews, anOverviewList,
           0, nullptr,           // 波段列表（0, nullptr = 所有波段）
           GDALDummyProgress, nullptr
       );

       if (err != CE_None) {
           std::cerr << "构建金字塔失败" << std::endl;
       }
   }

带进度回调的金字塔构建
========================

.. code-block:: c++

   int OverviewProgressFunc(double dfProgress, const char* pszMessage,
                            void* pUserData)
   {
       printf("\r构建金字塔: %.1f%%", dfProgress * 100);
       fflush(stdout);
       return TRUE;
   }

   void buildOverviewsWithProgress()
   {
       GDALDatasetUniquePtr poDS(
           GDALDataset::Open("huge_raster.tif", GDAL_OF_UPDATE));
       if (poDS == nullptr) return;

       int anLevels[] = {2, 4, 8, 16, 32};
       CPLErr err = poDS->BuildOverviews(
           "AVERAGE", 5, anLevels,
           0, nullptr,
           OverviewProgressFunc, nullptr
       );
       printf("\n");
   }

C 示例
======

.. code-block:: c

   #include "gdal.h"

   void buildOverviewsC()
   {
       GDALDatasetH hDS = GDALOpenEx("large.tif", GDAL_OF_UPDATE, NULL, NULL, NULL);
       if (hDS == NULL) return;

       int anLevels[] = {2, 4, 8, 16};
       CPLErr err = GDALBuildOverviews(hDS, "NEAREST", 4, anLevels,
                                        0, NULL, NULL, NULL);
       GDALClose(hDS);
   }

GetOverview
============

.. code-block:: c++

   void readOverviews()
   {
       GDALDatasetUniquePtr poDS(
           GDALDataset::Open("with_overviews.tif", GDAL_OF_READONLY));
       if (poDS == nullptr) return;

       GDALRasterBand* poBand = poDS->GetRasterBand(1);

       int nOverviews = poBand->GetOverviewCount();
       std::cout << "金字塔层数: " << nOverviews << std::endl;

       for (int i = 0; i < nOverviews; i++) {
           GDALRasterBand* poOverview = poBand->GetOverview(i);
           if (poOverview != nullptr) {
               std::cout << "  金字塔 " << i << ": "
                         << poOverview->GetXSize() << " x "
                         << poOverview->GetYSize() << std::endl;
           }
       }

       // 获取最接近目标分辨率的金字塔
       // 例如要在 512x512 窗口中显示
       int nDesiredWidth = 512, nDesiredHeight = 512;
       GDALRasterBand* poBestOverview = poBand->GetBestOverview(
           0, 0, poBand->GetXSize(), poBand->GetYSize(),
           nDesiredWidth, nDesiredHeight);
       if (poBestOverview != nullptr) {
           std::cout << "最佳金字塔分辨率: "
                     << poBestOverview->GetXSize() << " x "
                     << poBestOverview->GetYSize() << std::endl;
       }
   }

**************************
多维数组 API（RFC 75）
**************************

RFC 75（GDAL 3.1+）引入了全新的多维数组 API，用于访问 NetCDF、HDF5 等科学数据格式中
的多维数据。这是 GDAL 3.x 中最重要的新 API 之一。

.. note::

   多维数组 API 是对传统栅格 API（``GDALDataset`` / ``GDALRasterBand``）的补充，
   不是替代。传统的二维栅格数据仍使用原有 API，多维数据（3D、4D 或更高维度）则使用
   新的多维 API。

核心概念
========

多维数组 API 的核心类：

- **GDALGroup**：组（类似于 HDF5 的 Group 或 NetCDF 的文件/组），可以包含子组和多维数组
- **GDALMDArray**：多维数组，存储实际数据
- **GDALDimension**：维度（例如时间、经度、纬度、高度）
- **GDALExtendedDataType**：扩展数据类型，支持更多种类的数据类型
- **GDALAttribute**：属性，存储元数据信息

类层次结构
===========

::

   GDALDataset
     └── GDALGroup (根组)
           ├── GDALAttribute (属性)
           ├── GDALDimension (维度)
           ├── GDALMDArray (多维数组)
           │     ├── GDALDimension (维度引用)
           │     ├── GDALAttribute (属性)
           │     └── GDALExtendedDataType (数据类型)
           └── GDALGroup (子组)
                 └── ...

打开多维数据集
===============

使用 ``GDAL_OF_MULTIDIM_RASTER`` 标志打开多维数据访问：

C++ 示例
--------

.. code-block:: c++

   #include "gdal_priv.h"
   #include <iostream>

   void openMultiDimDataset()
   {
       // 以多维数组模式打开
       GDALDatasetUniquePtr poDS(static_cast<GDALDataset*>(
           GDALOpenEx("ocean_data.nc",
                      GDAL_OF_READONLY | GDAL_OF_MULTIDIM_RASTER,
                      nullptr, nullptr, nullptr)));
       if (poDS == nullptr) {
           std::cerr << "无法打开数据集" << std::endl;
           return;
       }

       // 获取根组
       auto poRootGroup = poDS->GetRootGroup();
       if (poRootGroup == nullptr) {
           std::cerr << "无法获取根组" << std::endl;
           return;
       }

       std::cout << "根组名称: " << poRootGroup->GetName() << std::endl;
   }

C 示例
------

.. code-block:: c

   #include "gdal.h"
   #include <stdio.h>

   void openMultiDimDatasetC()
   {
       GDALDatasetH hDS = GDALOpenEx("ocean_data.nc",
                                      GDAL_OF_READONLY | GDAL_OF_MULTIDIM_RASTER,
                                      NULL, NULL, NULL);
       if (hDS == NULL) {
           fprintf(stderr, "无法打开数据集\n");
           return;
       }

       /* 获取根组 */
       GDALGroupH hRootGroup = GDALDatasetGetRootGroup(hDS);
       if (hRootGroup == NULL) {
           fprintf(stderr, "无法获取根组\n");
           GDALClose(hDS);
           return;
       }

       printf("根组名称: %s\n", GDALGroupGetName(hRootGroup));

       GDALGroupRelease(hRootGroup);
       GDALClose(hDS);
   }

GDALGroup：组操作
=================

组可以包含子组、多维数组和属性。

遍历组内容
----------

.. code-block:: c++

   void exploreGroup(std::shared_ptr<GDALGroup> poGroup, int nDepth = 0)
   {
       std::string indent(nDepth * 2, ' ');

       // 遍历属性
       auto aoAttrNames = poGroup->GetAttributeNames();
       for (const auto& name : aoAttrNames) {
           auto poAttr = poGroup->GetAttribute(name);
           if (poAttr) {
               std::cout << indent << "属性: " << name;
               if (poAttr->GetDataType().GetClass() == GEDTC_STRING) {
                   std::cout << " = \"" << poAttr->ReadAsString() << "\"";
               } else if (poAttr->GetDataType().GetClass() == GEDTC_NUMERIC) {
                   std::cout << " = " << poAttr->ReadAsDouble();
               }
               std::cout << std::endl;
           }
       }

       // 遍历维度
       auto aoDims = poGroup->GetDimensions();
       for (const auto& poDim : aoDims) {
           std::cout << indent << "维度: " << poDim->GetName()
                     << " (大小=" << poDim->GetSize()
                     << ", 类型=" << poDim->GetType() << ")" << std::endl;
       }

       // 遍历多维数组
       auto aoArrayNames = poGroup->GetMDArrayNames();
       for (const auto& name : aoArrayNames) {
           std::cout << indent << "数组: " << name << std::endl;

           auto poArray = poGroup->OpenMDArray(name);
           if (poArray) {
               // 显示维度信息
               auto aoArrayDims = poArray->GetDimensions();
               std::cout << indent << "  维度: ";
               for (const auto& poDim : aoArrayDims) {
                   std::cout << poDim->GetName() << "("
                             << poDim->GetSize() << ") ";
               }
               std::cout << std::endl;

               // 显示数据类型
               std::cout << indent << "  类型: "
                         << poArray->GetDataType().GetName() << std::endl;
           }
       }

       // 遍历子组
       auto aoGroupNames = poGroup->GetGroupNames();
       for (const auto& name : aoGroupNames) {
           std::cout << indent << "子组: " << name << std::endl;
           auto poSubGroup = poGroup->OpenGroup(name);
           if (poSubGroup) {
               exploreGroup(poSubGroup, nDepth + 1);
           }
       }
   }

OpenMDArray()
-------------

.. code-block:: c++

   void openMDArray()
   {
       GDALDatasetUniquePtr poDS(static_cast<GDALDataset*>(
           GDALOpenEx("atmosphere.nc",
                      GDAL_OF_READONLY | GDAL_OF_MULTIDIM_RASTER,
                      nullptr, nullptr, nullptr)));
       if (poDS == nullptr) return;

       auto poRootGroup = poDS->GetRootGroup();

       // 列出所有可用数组
       auto aoNames = poRootGroup->GetMDArrayNames();
       std::cout << "可用数组:" << std::endl;
       for (const auto& name : aoNames) {
           std::cout << "  " << name << std::endl;
       }

       // 打开特定数组
       auto poTempArray = poRootGroup->OpenMDArray("temperature");
       if (poTempArray == nullptr) {
           std::cerr << "找不到 temperature 数组" << std::endl;
           return;
       }

       std::cout << "temperature 数组维度数: "
                 << poTempArray->GetDimensionCount() << std::endl;
   }

OpenGroup()
-----------

.. code-block:: c++

   void openSubGroup()
   {
       GDALDatasetUniquePtr poDS(static_cast<GDALDataset*>(
           GDALOpenEx("hierarchical.h5",
                      GDAL_OF_READONLY | GDAL_OF_MULTIDIM_RASTER,
                      nullptr, nullptr, nullptr)));
       if (poDS == nullptr) return;

       auto poRootGroup = poDS->GetRootGroup();

       // 列出子组
       auto aoGroupNames = poRootGroup->GetGroupNames();
       for (const auto& name : aoGroupNames) {
           std::cout << "子组: " << name << std::endl;
       }

       // 打开子组
       auto poSubGroup = poRootGroup->OpenGroup("science_data");
       if (poSubGroup == nullptr) return;

       // 在子组中打开数组
       auto poArray = poSubGroup->OpenMDArray("sea_surface_temp");
       if (poArray) {
           std::cout << "找到海表温度数组" << std::endl;
       }
   }

GDALMDArray：多维数组读取
==========================

Read() 方法
-----------

``Read()`` 方法用于读取多维数组的数据，支持指定读取范围和步长。

.. code-block:: c++

   CPLErr GDALMDArray::Read(
       const GUInt64* anStart,           // 每个维度的起始索引
       const size_t* anCount,            // 每个维度读取的元素数
       const GInt64* anStep,             // 每个维度的步长（可选，nullptr 表示步长为 1）
       const GDALExtendedDataType& bufferDataType, // 缓冲区数据类型
       void* pBuffer,                    // 输出缓冲区
       void* pDstBufferAllocStart,       // 缓冲区分配起始地址（可选）
       size_t nDstBufferAllocSize        // 缓冲区分配大小（可选）
   );

完整读取示例
------------

.. code-block:: c++

   #include "gdal_priv.h"
   #include <vector>
   #include <iostream>

   void readFullArray()
   {
       GDALDatasetUniquePtr poDS(static_cast<GDALDataset*>(
           GDALOpenEx("ocean_temp.nc",
                      GDAL_OF_READONLY | GDAL_OF_MULTIDIM_RASTER,
                      nullptr, nullptr, nullptr)));
       if (poDS == nullptr) return;

       auto poRootGroup = poDS->GetRootGroup();
       auto poArray = poRootGroup->OpenMDArray("temperature");
       if (poArray == nullptr) return;

       // 获取维度信息
       auto aoDims = poArray->GetDimensions();
       std::cout << "维度数: " << aoDims.size() << std::endl;

       size_t nTotalElements = 1;
       std::vector<GUInt64> anStart(aoDims.size());
       std::vector<size_t> anCount(aoDims.size());

       for (size_t i = 0; i < aoDims.size(); i++) {
           std::cout << "  " << aoDims[i]->GetName()
                     << " 大小=" << aoDims[i]->GetSize() << std::endl;
           anStart[i] = 0;
           anCount[i] = aoDims[i]->GetSize();
           nTotalElements *= anCount[i];
       }

       // 分配缓冲区并读取全部数据
       std::vector<float> afData(nTotalElements);
       GDALExtendedDataType dtFloat = GDALExtendedDataType::Create(GDT_Float32);

       CPLErr err = poArray->Read(
           anStart.data(),    // 起始位置（全为 0）
           anCount.data(),    // 每个维度的大小（读取全部）
           nullptr,           // 步长（默认为 1）
           dtFloat,           // 输出类型
           afData.data(),     // 输出缓冲区
           nullptr, 0
       );

       if (err != CE_None) {
           std::cerr << "读取失败" << std::endl;
           return;
       }

       // 打印第一个元素
       if (!afData.empty()) {
           std::cout << "第一个值: " << afData[0] << std::endl;
       }
   }

部分读取（切片）
-----------------

.. code-block:: c++

   void readSlice()
   {
       GDALDatasetUniquePtr poDS(static_cast<GDALDataset*>(
           GDALOpenEx("climate.nc",
                      GDAL_OF_READONLY | GDAL_OF_MULTIDIM_RASTER,
                      nullptr, nullptr, nullptr)));
       if (poDS == nullptr) return;

       auto poRootGroup = poDS->GetRootGroup();
       auto poArray = poRootGroup->OpenMDArray("temperature");
       if (poArray == nullptr) return;

       auto aoDims = poArray->GetDimensions();
       // 假设维度顺序为 (time, level, lat, lon)
       // 只读取第一个时间步、第一个高度层的数据

       size_t nDims = aoDims.size();
       std::vector<GUInt64> anStart(nDims, 0);
       std::vector<size_t> anCount(nDims);

       anCount[0] = 1;                     // time: 只取 1 个时间步
       anCount[1] = 1;                     // level: 只取 1 个高度层
       anCount[2] = aoDims[2]->GetSize();  // lat: 全部
       anCount[3] = aoDims[3]->GetSize();  // lon: 全部

       size_t nElements = 1;
       for (size_t i = 0; i < nDims; i++) {
           nElements *= anCount[i];
       }

       std::vector<float> afSlice(nElements);
       GDALExtendedDataType dtFloat = GDALExtendedDataType::Create(GDT_Float32);

       poArray->Read(anStart.data(), anCount.data(), nullptr,
                     dtFloat, afSlice.data(), nullptr, 0);

       std::cout << "切片大小: " << anCount[2] << " x " << anCount[3] << std::endl;
   }

带步长的读取
-------------

.. code-block:: c++

   void readWithStep()
   {
       GDALDatasetUniquePtr poDS(static_cast<GDALDataset*>(
           GDALOpenEx("big_data.nc",
                      GDAL_OF_READONLY | GDAL_OF_MULTIDIM_RASTER,
                      nullptr, nullptr, nullptr)));
       if (poDS == nullptr) return;

       auto poRootGroup = poDS->GetRootGroup();
       auto poArray = poRootGroup->OpenMDArray("data");
       if (poArray == nullptr) return;

       auto aoDims = poArray->GetDimensions();

       std::vector<GUInt64> anStart(aoDims.size(), 0);
       std::vector<size_t> anCount(aoDims.size());
       std::vector<GInt64> anStep(aoDims.size(), 1);

       // 每个维度每隔 2 个取一个（降采样）
       for (size_t i = 0; i < aoDims.size(); i++) {
           anCount[i] = (aoDims[i]->GetSize() + 1) / 2;  // 降采样后的大小
           anStep[i] = 2;  // 步长为 2
       }

       size_t nElements = 1;
       for (size_t i = 0; i < aoDims.size(); i++) {
           nElements *= anCount[i];
       }

       std::vector<float> afData(nElements);
       GDALExtendedDataType dtFloat = GDALExtendedDataType::Create(GDT_Float32);

       poArray->Read(anStart.data(), anCount.data(), anStep.data(),
                     dtFloat, afData.data(), nullptr, 0);
   }

GetDimensions()
----------------

.. code-block:: c++

   void getDimensionInfo()
   {
       GDALDatasetUniquePtr poDS(static_cast<GDALDataset*>(
           GDALOpenEx("model_output.nc",
                      GDAL_OF_READONLY | GDAL_OF_MULTIDIM_RASTER,
                      nullptr, nullptr, nullptr)));
       if (poDS == nullptr) return;

       auto poRootGroup = poDS->GetRootGroup();
       auto poArray = poRootGroup->OpenMDArray("wind_speed");
       if (poArray == nullptr) return;

       auto aoDims = poArray->GetDimensions();
       for (const auto& poDim : aoDims) {
           std::cout << "维度名称: " << poDim->GetName() << std::endl;
           std::cout << "  大小: " << poDim->GetSize() << std::endl;
           std::cout << "  类型: " << poDim->GetType() << std::endl;

           // 如果维度有坐标变量（例如 latitude 变量）
           auto poCoord = poDim->GetIndexingVariable();
           if (poCoord) {
               std::cout << "  坐标变量: " << poCoord->GetName() << std::endl;
           }
       }
   }

GetAttribute()
--------------

.. code-block:: c++

   void readAttributes()
   {
       GDALDatasetUniquePtr poDS(static_cast<GDALDataset*>(
           GDALOpenEx("dataset.nc",
                      GDAL_OF_READONLY | GDAL_OF_MULTIDIM_RASTER,
                      nullptr, nullptr, nullptr)));
       if (poDS == nullptr) return;

       auto poRootGroup = poDS->GetRootGroup();
       auto poArray = poRootGroup->OpenMDArray("temperature");
       if (poArray == nullptr) return;

       // 获取数组的所有属性名称
       auto aoAttrNames = poArray->GetAttributeNames();
       for (const auto& name : aoAttrNames) {
           auto poAttr = poArray->GetAttribute(name);
           if (poAttr == nullptr) continue;

           std::cout << "属性: " << name << " = ";

           auto eClass = poAttr->GetDataType().GetClass();
           if (eClass == GEDTC_STRING) {
               std::cout << "\"" << poAttr->ReadAsString() << "\"";
           } else if (eClass == GEDTC_NUMERIC) {
               if (poAttr->GetDimensionCount() == 0) {
                   // 标量数值
                   std::cout << poAttr->ReadAsDouble();
               } else {
                   std::cout << "[数值数组]";
               }
           }

           std::cout << std::endl;
       }

       // 直接获取特定属性
       auto poLongName = poArray->GetAttribute("long_name");
       if (poLongName) {
           std::cout << "long_name: " << poLongName->ReadAsString() << std::endl;
       }

       auto poUnits = poArray->GetAttribute("units");
       if (poUnits) {
           std::cout << "单位: " << poUnits->ReadAsString() << std::endl;
       }

       auto poFillValue = poArray->GetAttribute("_FillValue");
       if (poFillValue) {
           std::cout << "填充值: " << poFillValue->ReadAsDouble() << std::endl;
       }
   }

GDALExtendedDataType：数据类型
================================

.. code-block:: c++

   void checkDataType(std::shared_ptr<GDALMDArray> poArray)
   {
       GDALExtendedDataType dt = poArray->GetDataType();

       std::cout << "数据类型名称: " << dt.GetName() << std::endl;
       std::cout << "数据类型大小: " << dt.GetSize() << " 字节" << std::endl;

       auto eClass = dt.GetClass();
       switch (eClass) {
           case GEDTC_NUMERIC:
               std::cout << "类别: 数值型" << std::endl;
               // 获取对应的 GDALDataType
               {
                   GDALDataType eNativeType = dt.GetNumericDataType();
                   std::cout << "原生类型: "
                             << GDALGetDataTypeName(eNativeType) << std::endl;
               }
               break;
           case GEDTC_STRING:
               std::cout << "类别: 字符串型" << std::endl;
               break;
           case GEDTC_COMPOUND:
               std::cout << "类别: 复合型" << std::endl;
               break;
       }
   }

C++ 完整示例：读取 NetCDF 数据
=================================

.. code-block:: c++

   #include "gdal_priv.h"
   #include <iostream>
   #include <vector>
   #include <string>

   /**
    * 完整示例：打开 NetCDF 文件，遍历结构，读取多维数据
    */
   int main(int argc, char* argv[])
   {
       const char* pszFilename = (argc > 1) ? argv[1] : "era5_temperature.nc";

       // 以多维数组模式打开 NetCDF
       GDALDatasetUniquePtr poDS(static_cast<GDALDataset*>(
           GDALOpenEx(pszFilename,
                      GDAL_OF_READONLY | GDAL_OF_MULTIDIM_RASTER,
                      nullptr, nullptr, nullptr)));
       if (poDS == nullptr) {
           std::cerr << "无法打开: " << pszFilename << std::endl;
           return 1;
       }

       // 获取根组
       auto poRootGroup = poDS->GetRootGroup();
       std::cout << "=== 文件: " << pszFilename << " ===" << std::endl;
       std::cout << "根组: " << poRootGroup->GetName() << std::endl;

       // 显示全局属性
       std::cout << "\n--- 全局属性 ---" << std::endl;
       auto aoGlobalAttrs = poRootGroup->GetAttributeNames();
       for (const auto& name : aoGlobalAttrs) {
           auto poAttr = poRootGroup->GetAttribute(name);
           if (poAttr && poAttr->GetDataType().GetClass() == GEDTC_STRING) {
               std::cout << name << ": " << poAttr->ReadAsString() << std::endl;
           }
       }

       // 列出所有数组
       std::cout << "\n--- 数据数组 ---" << std::endl;
       auto aoArrayNames = poRootGroup->GetMDArrayNames();
       for (const auto& name : aoArrayNames) {
           auto poArray = poRootGroup->OpenMDArray(name);
           if (!poArray) continue;

           std::cout << name << " ["
                     << poArray->GetDataType().GetName() << "]";

           auto aoDims = poArray->GetDimensions();
           std::cout << " (";
           for (size_t i = 0; i < aoDims.size(); i++) {
               if (i > 0) std::cout << ", ";
               std::cout << aoDims[i]->GetName() << "=" << aoDims[i]->GetSize();
           }
           std::cout << ")" << std::endl;
       }

       // 读取 temperature 数组
       std::cout << "\n--- 读取数据 ---" << std::endl;
       auto poTempArray = poRootGroup->OpenMDArray("temperature");
       if (poTempArray == nullptr) {
           // 尝试其他常见名称
           poTempArray = poRootGroup->OpenMDArray("t2m");
       }
       if (poTempArray == nullptr) {
           std::cerr << "找不到温度数组" << std::endl;
           return 1;
       }

       auto aoDims = poTempArray->GetDimensions();
       size_t nDims = aoDims.size();

       // 读取第一个时间步的所有数据
       std::vector<GUInt64> anStart(nDims, 0);
       std::vector<size_t> anCount(nDims);

       std::cout << "读取切片 [";
       for (size_t i = 0; i < nDims; i++) {
           anCount[i] = aoDims[i]->GetSize();
           if (i == 0) {
               anCount[i] = 1;  // 只取第一个时间步
               std::cout << "0";
           } else {
               std::cout << ":";
           }
       }
       std::cout << ", :, :] (剩余维度取全部)" << std::endl;

       // 计算总元素数
       size_t nTotal = 1;
       for (size_t i = 0; i < nDims; i++) {
           nTotal *= anCount[i];
       }

       std::vector<float> afData(nTotal);
       GDALExtendedDataType dtFloat = GDALExtendedDataType::Create(GDT_Float32);

       CPLErr err = poTempArray->Read(
           anStart.data(), anCount.data(), nullptr,
           dtFloat, afData.data(), nullptr, 0);

       if (err != CE_None) {
           std::cerr << "读取失败!" << std::endl;
           return 1;
       }

       // 查找数据范围
       float fMin = afData[0], fMax = afData[0];
       for (size_t i = 1; i < nTotal; i++) {
           if (afData[i] < fMin) fMin = afData[i];
           if (afData[i] > fMax) fMax = afData[i];
       }
       std::cout << "数据范围: [" << fMin << ", " << fMax << "]" << std::endl;

       // 读取属性
       auto poUnits = poTempArray->GetAttribute("units");
       if (poUnits) {
           std::cout << "单位: " << poUnits->ReadAsString() << std::endl;
       }

       return 0;
   }

C API 对应函数
===============

多维数组 API 也提供了完整的 C 语言接口：

.. code-block:: c

   #include "gdal.h"
   #include <stdio.h>
   #include <stdlib.h>

   /**
    * C API 示例：读取 NetCDF 多维数据
    */
   int main(int argc, char* argv[])
   {
       const char* pszFile = (argc > 1) ? argv[1] : "data.nc";

       /* 以多维模式打开 */
       GDALDatasetH hDS = GDALOpenEx(pszFile,
                                      GDAL_OF_READONLY | GDAL_OF_MULTIDIM_RASTER,
                                      NULL, NULL, NULL);
       if (hDS == NULL) {
           fprintf(stderr, "无法打开: %s\n", pszFile);
           return 1;
       }

       /* 获取根组 */
       GDALGroupH hRoot = GDALDatasetGetRootGroup(hDS);
       if (hRoot == NULL) {
           fprintf(stderr, "无根组\n");
           GDALClose(hDS);
           return 1;
       }
       printf("根组: %s\n", GDALGroupGetName(hRoot));

       /* 列出数组 */
       char** papszArrays = GDALGroupGetMDArrayNames(hRoot, NULL);
       if (papszArrays != NULL) {
           for (int i = 0; papszArrays[i] != NULL; i++) {
               printf("数组: %s\n", papszArrays[i]);
           }
           CSLDestroy(papszArrays);
       }

       /* 打开并读取数组 */
       GDALMDArrayH hArray = GDALGroupOpenMDArray(hRoot, "temperature", NULL);
       if (hArray != NULL) {
           /* 获取维度数 */
           size_t nDimCount = GDALMDArrayGetDimensionCount(hArray);
           printf("维度数: %zu\n", nDimCount);

           /* 获取维度 */
           GDALDimensionH* phDims = GDALMDArrayGetDimensions(hArray, NULL);
           if (phDims != NULL) {
               for (size_t i = 0; i < nDimCount; i++) {
                   printf("  维度 %zu: %s (大小=%llu)\n",
                          i,
                          GDALDimensionGetName(phDims[i]),
                          (unsigned long long)GDALDimensionGetSize(phDims[i]));
               }
               GDALReleaseDimensions(phDims, (int)nDimCount);
           }

           /* 获取总大小并读取 */
           size_t nTotalSize = GDALMDArrayGetTotalSize(hArray);
           float* pfData = (float*)malloc(sizeof(float) * nTotalSize);
           if (pfData != NULL) {
               /* 创建 Float32 数据类型 */
               GDALExtendedDataTypeH hDT =
                   GDALExtendedDataTypeCreate(GDT_Float32);

               /* 读取全部数据 */
               GUInt64* panStart = (GUInt64*)calloc(nDimCount, sizeof(GUInt64));
               size_t* panCount = (size_t*)malloc(sizeof(size_t) * nDimCount);

               GDALDimensionH* phDimArr = GDALMDArrayGetDimensions(hArray, NULL);
               for (size_t i = 0; i < nDimCount; i++) {
                   panStart[i] = 0;
                   panCount[i] = GDALDimensionGetSize(phDimArr[i]);
               }
               GDALReleaseDimensions(phDimArr, (int)nDimCount);

               int bSuccess = GDALMDArrayRead(
                   hArray, panStart, panCount, NULL, NULL,
                   hDT, pfData, NULL, 0);

               if (bSuccess) {
                   printf("读取成功，第一个值: %f\n", pfData[0]);
               }

               free(panStart);
               free(panCount);
               free(pfData);
               GDALExtendedDataTypeRelease(hDT);
           }

           /* 读取属性 */
           GDALAttributeH hAttr = GDALMDArrayGetAttribute(hArray, "units");
           if (hAttr != NULL) {
               const char* pszValue = GDALAttributeReadAsString(hAttr);
               printf("单位: %s\n", pszValue ? pszValue : "(null)");
               GDALAttributeRelease(hAttr);
           }

           GDALMDArrayRelease(hArray);
       }

       GDALGroupRelease(hRoot);
       GDALClose(hDS);
       return 0;
   }

常见 C API 函数对照
=====================

以下是 C++ 和 C API 的主要对应关系：

.. list-table::
   :header-rows: 1
   :widths: 50 50

   * - C++ API
     - C API
   * - ``GDALDataset::GetRootGroup()``
     - ``GDALDatasetGetRootGroup()``
   * - ``GDALGroup::GetMDArrayNames()``
     - ``GDALGroupGetMDArrayNames()``
   * - ``GDALGroup::OpenMDArray()``
     - ``GDALGroupOpenMDArray()``
   * - ``GDALGroup::GetGroupNames()``
     - ``GDALGroupGetGroupNames()``
   * - ``GDALGroup::OpenGroup()``
     - ``GDALGroupOpenGroup()``
   * - ``GDALGroup::GetAttribute()``
     - ``GDALGroupGetAttribute()``
   * - ``GDALMDArray::GetDimensions()``
     - ``GDALMDArrayGetDimensions()``
   * - ``GDALMDArray::Read()``
     - ``GDALMDArrayRead()``
   * - ``GDALMDArray::GetAttribute()``
     - ``GDALMDArrayGetAttribute()``
   * - ``GDALDimension::GetName()``
     - ``GDALDimensionGetName()``
   * - ``GDALDimension::GetSize()``
     - ``GDALDimensionGetSize()``
   * - ``GDALAttribute::ReadAsString()``
     - ``GDALAttributeReadAsString()``

gdal CLI 工具
================

GDAL 提供了两个命令行工具用于多维数组操作：

gdal mdim info
--------------

``gdal mdim info`` 用于查看多维数据集的结构信息。

.. code-block:: bash

   # 查看 NetCDF 文件的完整多维结构
   gdal mdim info ocean_data.nc

   # 输出 JSON 格式
   gdal mdim info -of json ocean_data.nc

   # 只查看特定组
   gdal mdim info ocean_data.nc /science_data

   # 查看特定数组的详细信息（包括统计）
   gdal mdim info ocean_data.nc -array temperature

   # 输出示例：
   # {
   #   "type": "group",
   #   "name": "/",
   #   "attributes": { ... },
   #   "dimensions": [
   #     { "name": "time", "size": 12, "type": "TEMPORAL" },
   #     { "name": "lat", "size": 180, "type": "SPATIAL" },
   #     { "name": "lon", "size": "360", "type": "SPATIAL" }
   #   ],
   #   "arrays": [
   #     {
   #       "name": "temperature",
   #       "dataType": "Float32",
   #       "dimensions": ["time", "lat", "lon"],
   #       ...
   #     }
   #   ]
   # }

gdal mdim convert
-----------------

``gdal mdim convert`` 用于转换多维数据。

.. code-block:: bash

   # 将 NetCDF 中的特定数组转换为新的 NetCDF
   gdal mdim convert input.nc output.nc /temperature

   # 转换时指定子集
   gdal mdim convert input.nc output.nc \
       -array temperature \
       -subset time,0:1

   # 转换为不同格式
   gdal mdim convert input.nc output.zarr /temperature

NetCDF / HDF5 常见操作模式
===========================

读取时间序列
-------------

.. code-block:: c++

   /**
    * 读取 NetCDF 时间序列数据
    * 常见结构：temperature(time, lat, lon)
    * 读取某个空间点的完整时间序列
    */
   void readTimeSeries(const char* pszFile,
                       const char* pszVarName,
                       size_t nLatIdx, size_t nLonIdx)
   {
       GDALDatasetUniquePtr poDS(static_cast<GDALDataset*>(
           GDALOpenEx(pszFile,
                      GDAL_OF_READONLY | GDAL_OF_MULTIDIM_RASTER,
                      nullptr, nullptr, nullptr)));
       if (poDS == nullptr) return;

       auto poRootGroup = poDS->GetRootGroup();
       auto poArray = poRootGroup->OpenMDArray(pszVarName);
       if (poArray == nullptr) return;

       auto aoDims = poArray->GetDimensions();
       // 假设维度顺序：(time, lat, lon)
       size_t nTimeSteps = aoDims[0]->GetSize();

       // 读取特定点的时间序列
       GUInt64 anStart[3] = {0, nLatIdx, nLonIdx};
       size_t anCount[3] = {nTimeSteps, 1, 1};

       std::vector<float> afTimeSeries(nTimeSteps);
       GDALExtendedDataType dtFloat = GDALExtendedDataType::Create(GDT_Float32);

       poArray->Read(anStart, anCount, nullptr,
                     dtFloat, afTimeSeries.data(), nullptr, 0);

       std::cout << "时间序列 (" << nLatIdx << ", " << nLonIdx << "):" << std::endl;
       for (size_t t = 0; t < nTimeSteps; t++) {
           std::cout << "  t=" << t << ": " << afTimeSeries[t] << std::endl;
       }
   }

获取维度坐标
-------------

.. code-block:: c++

   /**
    * 获取维度坐标值（例如纬度数组、经度数组）
    */
   void getDimensionCoordinates(std::shared_ptr<GDALMDArray> poArray)
   {
       auto aoDims = poArray->GetDimensions();

       for (const auto& poDim : aoDims) {
           std::cout << "维度: " << poDim->GetName() << std::endl;

           // 获取维度的坐标变量
           auto poCoordVar = poDim->GetIndexingVariable();
           if (poCoordVar == nullptr) {
               std::cout << "  无坐标变量" << std::endl;
               continue;
           }

           size_t nSize = poDim->GetSize();
           std::vector<double> adfCoords(nSize);

           GUInt64 anStart[1] = {0};
           size_t anCount[1] = {nSize};
           GDALExtendedDataType dtDouble = GDALExtendedDataType::Create(GDT_Float64);

           poCoordVar->Read(anStart, anCount, nullptr,
                            dtDouble, adfCoords.data(), nullptr, 0);

           std::cout << "  坐标范围: [" << adfCoords[0]
                     << ", " << adfCoords[nSize - 1] << "]" << std::endl;
       }
   }

HDF5 组遍历
------------

.. code-block:: c++

   /**
    * 遍历 HDF5 文件的层次结构
    * HDF5 文件通常有深层嵌套结构
    */
   void traverseHDF5(const char* pszFile)
   {
       GDALDatasetUniquePtr poDS(static_cast<GDALDataset*>(
           GDALOpenEx(pszFile,
                      GDAL_OF_READONLY | GDAL_OF_MULTIDIM_RASTER,
                      nullptr, nullptr, nullptr)));
       if (poDS == nullptr) return;

       auto poRoot = poDS->GetRootGroup();

       // 递归遍历所有组和数组
       std::function<void(std::shared_ptr<GDALGroup>, const std::string&)>
           traverse = [&](std::shared_ptr<GDALGroup> poGroup,
                          const std::string& path)
       {
           // 输出当前组的属性
           auto aoAttrs = poGroup->GetAttributeNames();
           for (const auto& attrName : aoAttrs) {
               auto poAttr = poGroup->GetAttribute(attrName);
               if (poAttr && poAttr->GetDataType().GetClass() == GEDTC_STRING) {
                   std::cout << path << " @" << attrName
                             << " = \"" << poAttr->ReadAsString()
                             << "\"" << std::endl;
               }
           }

           // 输出数组
           auto aoArrays = poGroup->GetMDArrayNames();
           for (const auto& arrName : aoArrays) {
               auto poArr = poGroup->OpenMDArray(arrName);
               if (!poArr) continue;

               auto dims = poArr->GetDimensions();
               std::cout << path << "/" << arrName << " ["
                         << poArr->GetDataType().GetName() << "] (";
               for (size_t i = 0; i < dims.size(); i++) {
                   if (i > 0) std::cout << ",";
                   std::cout << dims[i]->GetSize();
               }
               std::cout << ")" << std::endl;
           }

           // 递归子组
           auto aoGroups = poGroup->GetGroupNames();
           for (const auto& grpName : aoGroups) {
               auto poSub = poGroup->OpenGroup(grpName);
               if (poSub) {
                   traverse(poSub, path + "/" + grpName);
               }
           }
       };

       traverse(poRoot, "");
   }

*****************
常见数据类型总结
*****************

下表总结了 GDAL 3.x 支持的主要栅格数据类型：

.. list-table::
   :header-rows: 1
   :widths: 20 15 15 50

   * - 类型名称
     - GDALDataType
     - 字节数
     - 说明
   * - Byte
     - GDT_Byte
     - 1
     - 无符号 8 位整数（0-255）
   * - Int8
     - GDT_Int8
     - 1
     - 有符号 8 位整数（-128~127），GDAL 3.7+
   * - UInt16
     - GDT_UInt16
     - 2
     - 无符号 16 位整数
   * - Int16
     - GDT_Int16
     - 2
     - 有符号 16 位整数
   * - UInt32
     - GDT_UInt32
     - 4
     - 无符号 32 位整数
   * - Int32
     - GDT_Int32
     - 4
     - 有符号 32 位整数
   * - UInt64
     - GDT_UInt64
     - 8
     - 无符号 64 位整数，GDAL 3.7+
   * - Int64
     - GDT_Int64
     - 8
     - 有符号 64 位整数，GDAL 3.7+
   * - Float16
     - GDT_Float16
     - 2
     - IEEE 754 半精度浮点，GDAL 3.11+
   * - Float32
     - GDT_Float32
     - 4
     - IEEE 754 单精度浮点
   * - Float64
     - GDT_Float64
     - 8
     - IEEE 754 双精度浮点
   * - CInt16
     - GDT_CInt16
     - 4
     - 复数（两个 Int16）
   * - CInt32
     - GDT_CInt32
     - 8
     - 复数（两个 Int32）
   * - CFloat32
     - GDT_CFloat32
     - 8
     - 复数（两个 Float32）
   * - CFloat64
     - GDT_CFloat64
     - 16
     - 复数（两个 Float64）
