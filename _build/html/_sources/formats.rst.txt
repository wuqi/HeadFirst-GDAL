.. highlight:: rst

################
影像是什么
################
从真实世界中获取数字影像有很多方法，比如数码相机、扫描仪、CT或者磁共振成像。无论哪种方法，我们（人类）看到的是影像，而让数字设备来“看“的时候，则是在记录影像中的每一个点的数值。 [#f1]_  

.. image:: img/cameraman.png
   :alt: A matrix of the mirror of cameraman
   :align: center

比如上面的影像，在标出的蓝框中你见到的只是一块灰色区域，但是计算机中识别为一个矩阵，该矩阵包含了所有像素点的强度值。如何获取并存储这些像素值由我们的需求而定，最终在计算机世界里所有影像都可以简化矩阵信息。当然，影像还有其他信息，下面我们即将介绍。

******************
影像元数据
******************
影像元数据是指影像中除了信息矩阵以外的一些参考数据。例如影像的长、宽、波段数、数据类型、存储方式、投影坐标、太阳高度角、获取时间、传感器信息等内容。使用 :ref:`gdalinfo` ，我们就可以看到影像的元数据信息：

.. figure:: img/metaData.png
   :alt: multiBands Image
   :align: center


******************   
GDAL中的影像
******************
GDAL中，影像也可以称为 :ref:`dataset` ，大致分为两种类型，多波段影像和多数据集影像。 :ref:`gdaldatamodel` 中我们将详细介绍GDAL对影像数据如何解析和读写，这章中，我们只简单的分析一下两种数据的共性和区别。

.. _multibands:

多波段数据
================
多波段数据是我们比较熟悉的，例如tiff格式、jpg格式、png格式、img格式等，都是多个波段的数据。我们一般处理的也都是多波段的数据。

多波段的数据实际上可以看为一个二维的数组，在GDAL中就是包含了多个 :ref:`rasterband` 的一个 :ref:`dataset` ，里面保存了影像的所有数据以及元数据。其中数据存在 :ref:`rasterband` 中，每个波段可以当作一维数组进行处理。如下图所示：

.. figure:: img/multBands.png
   :alt: multiBands Image
   :align: center


.. _multidatasets:

多数据集数据
===============
这种数据遥感影像中也比较常见，例如hdf格式、he5格式、netcdf格式等。这些数据在GDAL中，与上文介绍的多波段数据不同，是包含了多个 :ref:`dataset` 的 :ref:`dataset` 。可以简单的理解为：一个多数据集数据包含了多个 :ref:`multibands` ，当然，实际上，这些 :ref:`multibands` 里大部分只有一个波段。如下图所示：

.. figure:: img/multDataset.png
   :alt: multDataset Image
   :align: center

一般我们会将多数据集数据转化为多波段的数据，方便处理。下面就是一个多数据集数据转换为多波段数据的函数，其中的部分内容我们将在后文中陆续展开讲解：

.. literalinclude:: code/sub2tif.c
    :language: c++
    :encoding: utf8

.. [#f1] `来自OpenCV文档 <http://www.opencv.org.cn/opencvdoc/2.3.2/html/doc/tutorials/core/mat%20-%20the%20basic%20image%20container/mat%20-%20the%20basic%20image%20container.html#matthebasicimagecontainer>`_
