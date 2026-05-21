# HeadFirst GDAL

GDAL/OGR 中文入门教程，涵盖 GDAL 2.x 到 3.13 的完整知识体系。

## 内容概览

### 基础篇

| 章节 | 主题 |
|------|------|
| preface | 前言 |
| formats | 影像是什么 |
| gdal-data-model | GDAL 数据模型 |
| read-and-write | 栅格数据读写 |
| gdal-utilities | GDAL 工具集 |
| gdal-tool-code | GDAL 工具集代码实现 (C/C++) |
| gdal-alg | GDAL 算法简介 |
| gdal-cheat-sheet | GDAL Cheat Sheet |
| gdal-warp-api-tutorial | GDALWarp |

### 编译篇

| 章节 | 主题 |
|------|------|
| static-build | 静态编译 |
| gdal_3_build | GDAL 3 编译 |

### GDAL 3.x 新内容

| 章节 | 主题 |
|------|------|
| gdal3-overview | GDAL 3.x 概述 (新头文件体系、统一 CLI、CMake) |
| gdal3-raster-api | 栅格 API 新用法 (GDALOpenEx、RasterIO、多维数组 API) |
| gdal3-vector-api | 矢量 API 新用法 (Feature/Layer、几何操作、Arrow API) |
| gdal3-srs-proj | 坐标参考系与投影 (AxisMappingStrategy、TransformBounds、WKT2) |
| gdal3-driver-dev | 自定义 Driver 开发 (栅格/矢量 Driver 架构、延迟加载) |
| gdal3-vsi-cpl | 虚拟文件系统与 CPL 工具函数 |
| gdal3-tools | GDAL 3.x 新工具介绍 (统一 CLI、Pipeline、编程调用) |
| gdal3-algorithms | GDAL 算法库详解 (栅格化、矢量化、插值、视域、Warp、波段代数) |

### OGR 矢量篇

| 章节 | 主题 |
|------|------|
| ogr-architecture | OGR 矢量结构 |
| ogr-proj | OGR 投影简介 |
| ogr-read-write | 矢量数据读写 |
| ogr-sql | OGR SQL |
| sqlite-sql | SQLite SQL |
| ogr-utilities | OGR 工具集 |
| ogr_layer_algebra | OGR Layer Algebra |

## AI 参考文件

`gdal3-full.txt` 是合并了全部 GDAL 3.x 新文档的完整参考文件，可供 LLM 直接消费。

## 构建

```bash
# 安装依赖
pip install sphinx sphinx_rtd_theme

# 构建 HTML
make html

# 构建 PDF (需要 LaTeX)
make latexpdf
```

## 许可证

详见 [LICENSE](LICENSE)
