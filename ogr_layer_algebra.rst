.. hightlight:: rst
.. _OGRLayerAlgebra


############################
OGR Layer Algebra
############################
本章来源 `OGR Layer Algebra <https://gdal.org/development/rfc/rfc39_ogr_layer_algebra.html>`_

********************
简介
********************
`OGRLayer` OGR层类空间分析的基本功能,包括 `Intersection Union  SymDifference Identity Update Clip Erase`几种基础功能

代码如下:

.. code-block:: c++

    char **param = NULL;
    param = CSLAddString(param, "INPUT_PREFIX=o_");
    param = CSLAddString(param, "METHOD_PREFIX=n_");
    param = CSLAddString(param, "SKIP_FAILURES=YES");
    OGRErr err = oldLayer->Union(newLayer, outLayer, 
                                 param, NULL, NULL);
    if (err != OGRERR_NONE)
    {
        gError(LOG_CAT, "Union error");
        CSLDestroy(param);
        break;
    }
    CSLDestroy(param);
    outLayer->SyncToDisk();

