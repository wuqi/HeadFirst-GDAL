.. highlight:: rst

.. _ogrreadwrite:

################
矢量数据读写
################

.. _ogrread:

******************
数据读取
******************
与栅格的 :ref:`gdalread` 流程类似，但矢量数据定位明显，属性隐含，其几何形状和属性需要分开读取，GDAL有两个版本，读取接口不同，但是流程基本相同，以下是获取流程：

* 注册驱动
* 打开数据集
* 打开图层
* 获取要素类定义
* 遍历要素
* 获取几何形状以及属性信息

``1.x`` 版本完整代码示例：

.. code-block:: c++

    #include "ogrsf_frmts.h"
    int main()
    {
        //注册驱动
        OGRRegisterAll();
        //打开数据集
        OGRDataSource       *poDS;
        poDS = OGRSFDriverRegistrar::Open( "point.shp", FALSE );
        if( poDS == NULL )
        {
            printf( "Open failed.\n" );
            exit( 1 );
        }
        //打开图层
        OGRLayer  *poLayer;
        poLayer = poDS->GetLayerByName( "point" );
        OGRFeature *poFeature;
        //遍历要素
        poLayer->ResetReading();
        while( (poFeature = poLayer->GetNextFeature()) != NULL )
        {
            //获取要素类定义
            OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();
            int iField;
            //获取属性信息
            for( iField = 0; iField < poFDefn->GetFieldCount(); iField++ )
            {
                OGRFieldDefn *poFieldDefn = poFDefn->GetFieldDefn( iField );
                if( poFieldDefn->GetType() == OFTInteger )
                    printf( "%d,", poFeature->GetFieldAsInteger( iField ) );
                else if( poFieldDefn->GetType() == OFTReal )
                    printf( "%.3f,", poFeature->GetFieldAsDouble(iField) );
                else if( poFieldDefn->GetType() == OFTString )
                    printf( "%s,", poFeature->GetFieldAsString(iField) );
                else
                    printf( "%s,", poFeature->GetFieldAsString(iField) );
            }
            //获取几何形状
            OGRGeometry *poGeometry;
            poGeometry = poFeature->GetGeometryRef();
            if( poGeometry != NULL 
                && wkbFlatten(poGeometry->getGeometryType()) == wkbPoint )
            {
                OGRPoint *poPoint = (OGRPoint *) poGeometry;
                printf( "%.3f,%3.f\n", poPoint->getX(), poPoint->getY() );
            }
            else
            {
                printf( "no point geometry\n" );
            }       
            OGRFeature::DestroyFeature( poFeature );
        }
        OGRDataSource::DestroyDataSource( poDS );
    }

2.x 版本中最主要修改为 ``GDALDataset`` 代替``OGRDataSource`` 其余接口大致不变.打开方式略有区别

.. code-block:: c++

    /**
    * @param pszFilename 文件名
    * @param nOpenFlags  GDAL_OF_RASTER/GDAL_OF_VECTOR  数据类型
                         GDAL_OF_READONLY/GDAL_OF_UPDATE 打开方式
                         GDAL_OF_SHARED 共享模式
                         GDAL_OF_VERBOSE_ERROR 返回详细错误
    * @param papszAllowedDrivers NULL或以NULL结尾的列表,指定驱动
    * @param papszOpenOptions NULL或以NULL结尾的字符串列表,根据驱动设置
    * @param papszSiblingFiles NULL或以NULL结尾的文件列表
    */
    GDALDatasetH GDALOpenEx	(	const char * 	pszFilename,
    unsigned int 	nOpenFlags,
    const char *const * 	papszAllowedDrivers,
    const char *const * 	papszOpenOptions,
    const char *const * 	papszSiblingFiles 
    )	


.. attention::

    * 打开前建议添加 ``CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");`` 设置,保证windows下文件名中文路径正常打开
    * 打开前添加 ``CPLSetConfigOption("SHAPE_ENCODING", "");`` 读出来编码和系统编码保持一致


2.x 版本完整代码示例： 

.. code-block:: c++

    #include "ogrsf_frmts.h"
    int main()
    {
        CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
        CPLSetConfigOption("SHAPE_ENCODING", "");
        //主要修改
        GDALAllRegister();
        GDALDataset       *poDS;
        poDS = (GDALDataset*) GDALOpenEx( "point.shp", GDAL_OF_VECTOR, NULL, NULL, NULL );
        if( poDS == NULL )
        {
            printf( "Open failed.\n" );
            exit( 1 );
        }

        //后面与1.x版本一致
        OGRLayer  *poLayer;
        poLayer = poDS->GetLayerByName( "point" );
        //寻找特定字段
        OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();

        int nSIDIndex = poFDefn->GetFieldIndex("SS_ID");
        if (nSIDIndex < 0)
        {
            printf( "can't find SS_ID field.\n" );
        }

        OGRFeature *poFeature;
        poLayer->ResetReading();
        while( (poFeature = poLayer->GetNextFeature()) != NULL )
        {
            //获取固定字段值
            if(nSIDIndex >= 0)
            {
                std::string strSSID = poFeature->GetFieldAsString(nSIDIndex);
            }
            OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();
            int iField;
            for( iField = 0; iField < poFDefn->GetFieldCount(); iField++ )
            {
                OGRFieldDefn *poFieldDefn = poFDefn->GetFieldDefn( iField );
                if( poFieldDefn->GetType() == OFTInteger )
                    printf( "%d,", poFeature->GetFieldAsInteger( iField ) );
                else if( poFieldDefn->GetType() == OFTInteger64 )
                    printf( CPL_FRMT_GIB ",", poFeature->GetFieldAsInteger64( iField ) );
                else if( poFieldDefn->GetType() == OFTReal )
                    printf( "%.3f,", poFeature->GetFieldAsDouble(iField) );
                else if( poFieldDefn->GetType() == OFTString )
                    printf( "%s,", poFeature->GetFieldAsString(iField) );
                else
                    printf( "%s,", poFeature->GetFieldAsString(iField) );
            }
            OGRGeometry *poGeometry;
            poGeometry = poFeature->GetGeometryRef();
            if( poGeometry != NULL 
                    && wkbFlatten(poGeometry->getGeometryType()) == wkbPoint )
            {
                OGRPoint *poPoint = (OGRPoint *) poGeometry;
                printf( "%.3f,%3.f\n", poPoint->getX(), poPoint->getY() );
            }
            else
            {
                printf( "no point geometry\n" );
            }
            OGRFeature::DestroyFeature( poFeature );
        }
        GDALClose( poDS );
    }


.. attention::

    * 有很多矢量驱动是只读的，无法像栅格那样直接修改和创建文件。
    * 如果需要确定，可以使用 ``ogrinfo --formats`` 确定驱动的权限。


.. _ogrwrite:

******************
数据写入
******************
数据写入与读取类似，流程如下：

* 注册驱动
* 打开或创建数据集
* 创建图层
* 创建要素类定义
* 新建几何形状
* 新建要素
* 关闭数据集

1.x完整代码示例（ ``C++``）：

.. code-block:: c++

    #include "ogrsf_frmts.h"
    int main()
    {
        const char *pszDriverName = "ESRI Shapefile";
        OGRSFDriver *poDriver;
        OGRRegisterAll();
        poDriver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(
                    pszDriverName );
        if( poDriver == NULL )
        {
            printf( "%s driver not available.\n", pszDriverName );
            exit( 1 );
        }
        OGRDataSource *poDS;
        poDS = poDriver->CreateDataSource( "point_out.shp", NULL );
        if( poDS == NULL )
        {
            printf( "Creation of output file failed.\n" );
            exit( 1 );
        }
        OGRLayer *poLayer;
        poLayer = poDS->CreateLayer( "point_out", NULL, wkbPoint, NULL );
        if( poLayer == NULL )
        {
            printf( "Layer creation failed.\n" );
            exit( 1 );
        }
        OGRFieldDefn oField( "Name", OFTString );
        oField.SetWidth(32);
        if( poLayer->CreateField( &oField ) != OGRERR_NONE )
        {
            printf( "Creating Name field failed.\n" );
            exit( 1 );
        }
        double x, y;
        char szName[33];
        while( !feof(stdin) 
               && fscanf( stdin, "%lf,%lf,%32s", &x, &y, szName ) == 3 )
        {
            OGRFeature *poFeature;
            poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
            poFeature->SetField( "Name", szName );
            OGRPoint pt;
            
            pt.setX( x );
            pt.setY( y );
     
            poFeature->SetGeometry( &pt ); 
            if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE )
            {
               printf( "Failed to create feature in shapefile.\n" );
               exit( 1 );
            }
             OGRFeature::DestroyFeature( poFeature );
        }
        OGRDataSource::DestroyDataSource( poDS );
    }
    

2.x 完整代码示例（ ``C++``）：

.. code-block:: c++

    #include "ogrsf_frmts.h"
    int main()
    {
        const char *pszDriverName = "ESRI Shapefile";
        GDALDriver *poDriver;
        GDALAllRegister();
        poDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName );
        if( poDriver == NULL )
        {
            printf( "%s driver not available.\n", pszDriverName );
            exit( 1 );
        }
        GDALDataset *poDS;
        poDS = poDriver->Create( "point_out.shp", 0, 0, 0, GDT_Unknown, NULL );
        if( poDS == NULL )
        {
            printf( "Creation of output file failed.\n" );
            exit( 1 );
        }
        OGRLayer *poLayer;
        poLayer = poDS->CreateLayer( "point_out", NULL, wkbPoint, NULL );
        if( poLayer == NULL )
        {
            printf( "Layer creation failed.\n" );
            exit( 1 );
        }
        OGRFieldDefn oField( "Name", OFTString );
        oField.SetWidth(32);
        if( poLayer->CreateField( &oField ) != OGRERR_NONE )
        {
            printf( "Creating Name field failed.\n" );
            exit( 1 );
        }
        double x, y;
        char szName[33];
        while( !feof(stdin) 
               && fscanf( stdin, "%lf,%lf,%32s", &x, &y, szName ) == 3 )
        {
            OGRFeature *poFeature;
            poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
            poFeature->SetField( "Name", szName );
            OGRPoint pt;
            
            pt.setX( x );
            pt.setY( y );
        
            poFeature->SetGeometry( &pt ); 
            if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE )
            {
                printf( "Failed to create feature in shapefile.\n" );
                exit( 1 );
            }
            OGRFeature::DestroyFeature( poFeature );
        }
        GDALClose( poDS );
    }

******************
数据修改
******************
GDAL可以添加、删除、修改属性信息和要素，下面简单介绍下如何添加/编辑属性字段(2.x)：

.. code-block:: c++

    GDALDataset* dst = (GDALDataset*)GDALOpenEx(shpName, GDAL_OF_UPDATE, NULL, NULL, NULL);
    OGRLayer  *poLayer = dst->GetLayer(0);
    OGRFieldDefn oField("imgNo", OFTString);  // Add fields
    oField.SetWidth(128);
    if (poLayer->CreateField(&oField, TRUE) != OGRERR_NONE)
    {
    printf("Creating Name field failed.\n");
    }
    OGRFeature *poFeature;

    poLayer->ResetReading();
    while ((poFeature = poLayer->GetNextFeature()) != NULL)
    {
        poFeature->SetField("imgNo", "123456");  // Set fields
        poLayer->SetFeature(poFeature);
        OGRFeature::DestroyFeature(poFeature);
    }
    GDALClose(dst);


删除其实也类似,需要注意打开时一定要加上 ``GDAL_OF_UPDATE`` 设置

.. code-block:: c++

    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
    CPLSetConfigOption("SHAPE_ENCODING", "");
    GDALAllRegister();
    GDALDataset *poDS = (GDALDataset*)GDALOpenEx(strIn.c_str(),\
     GDAL_OF_VECTOR| GDAL_OF_UPDATE, NULL, NULL, NULL);
    
    OGRLayer* pLayer = poDS->GetLayer(0);
    if (!pLayer->TestCapability(OLCDeleteFeature))
    {
        LOGI( "Layer does not support delete feature capability");
    }
    pLayer->DeleteFeature(1);
    pLayer->DeleteFeature(2);
    pLayer->DeleteFeature(3);
    pLayer->DeleteFeature(4);
    pLayer->DeleteFeature(5);
    pLayer->SyncToDisk();
    std::stringstream sql;
    //shapefile需要这一步才能真正写入
    sql << "REPACK " << pLayer->GetName();
    poDS->ExecuteSQL(sql.str().c_str(), NULL, NULL);
    GDALClose(poDS);


.. attention::

    * 处理shp文件时记得最后要执行 ``REPACK 表名`` ,否则只是临时标记,不会真正删除。
