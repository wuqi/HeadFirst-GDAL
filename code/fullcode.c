#include "gdal_priv.h"
int main(int argc, char *argv[])
{
  GDALAllRegister();//注册类型，读取写入任何类型影像必须加入此句
  //以只读方式打开文件
  GDALDataset *ds1 = (GDALDataset *) GDALOpen("Input1.tif", GA_ReadOnly);

  if(ds1 == NULL) {
    printf("不能打开第一个文件，请检查文件是否存在！");
    return -1;
  }

  int WidthAll = ds1->GetRasterXSize();
  int HightAll = ds1->GetRasterYSize();
  int BandNum = ds1->GetRasterCount();
  float **InBuf1 = new float*[BandNum];
  int i, j, k;

  //分波段读取
  for(i = 0; i < BandNum; i++) {
    InBuf1[i] = new float[WidthAll * HightAll];
    GDALRasterBand *band = ds1->GetRasterBand(i + 1); //获取波段，波段从1开始
    band->RasterIO(GF_Read,
                   0,
                   0,
                   WidthAll,
                   HightAll,
                   InBuf1[i],
                   WidthAll,
                   HightAll,
                   GDT_Float32,
                   0,
                   0);
  }

  //打开第二个文件，假设第一个文件和第二个文件大小、波段数相同
  //以只读方式打开文件
  GDALDataset *ds2 = (GDALDataset *) GDALOpen("Input2.tif", GA_ReadOnly);

  if(ds2 == NULL) {
    printf("不能打开第二个文件，请检查文件是否存在！");
    return ;
  }

  //第二个文件中的数据
  float **InBuf2 = new float*[BandNum];

  for(i = 0; i < BandNum; i++) {
    InBuf2[i] = new float[WidthAll * HightAll];
    GDALRasterBand *band = ds2->GetRasterBand(i + 1);
    band->RasterIO(GF_Read,
                   0,
                   0,
                   WidthAll,
                   HightAll,
                   InBuf2[i],
                   WidthAll,
                   HightAll,
                   GDT_Float32,
                   0,
                   0);
  }

  //写入的数据(可以最后创建写入文件,需要先创建写入数据)
  float **outBuf = new float*[BandNum];

  for(i = 0; i < BandNum; i++) {
    outBuf[i] = new float[WidthAll * HightAll];
  }

  ////////////////////////////////////////////////////
  /////////////////////处理///////////////////////////
  ////////////////////////////////////////////////////
  //简单相加示例，实际上，这部分最好写入函数，或者类中单独封装起来，方便使用
  for(j = 0; j < BandNum; j++) {
    for(k = 0; k < WidthAll * HightAll; k++) {
      outBuf[j][k] = InBuf1[j][k] + InBuf2[j][k];
    }
  }

  ////////////////////////////////////////////////////
  /////////////////////处理///////////////////////////
  ////////////////////////////////////////////////////
  //创建写入图像：
  GDALDriver *poDriver;
  const char *pszFormat = "GTiff";
  poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);

  if(poDriver == NULL) {
    return;
  }

  GDALDataset *OutDs;
  char **papszOptions = NULL;
  char **papszMetadata;
  //这里的参数全是指tif格式的参数，如果是其他格式，请把这里所有注释掉，或者参照文档，自行设定
  //设置压缩类型，envi只认得packbits压缩.
  //papszOptions = CSLSetNameValue( papszOptions, "COMPRESS", "PACKBITS" );
  //设置压缩比,可以不用,有些时候压缩反而更大,因为是无损的,除非图片很大
  //papszOptions = CSLSetNameValue( papszOptions, "ZLEVEL", "9" );
  //设置bsq或者BIP存储 bsq:BAND,bip:PIXEL
  papszOptions = CSLSetNameValue(papszOptions, "INTERLEAVE", "BAND");
  OutDs = poDriver->Create("output.tif",
                           WidthAll,
                           HightAll,
                           BandNum,
                           GDT_Float32,
                           papszOptions);
  double geos[6];
  //获取第一幅图的地理变化信息，这里如果与原图不同，请自己计算
  ds1->GetGeoTransform(geos);
  char *pszProjection = NULL;
  char *pszPrettyWkt = NULL;
  //设置为第一幅图的投影，如果与原图不同，请跳过这个if部分，到下个部分直接设置投影
  //设置投影，如果需要特殊投影，请找到wkt串，自行建立投影后传入
  OGRSpatialReferenceH  hSRS;

  if(GDALGetProjectionRef(ds1) != NULL) {
    pszProjection = (char *) GDALGetProjectionRef(ds1);
    hSRS = OSRNewSpatialReference(NULL);

    if(OSRImportFromWkt(hSRS, &pszProjection) == CE_None) {
      OSRExportToPrettyWkt(hSRS, &pszPrettyWkt, FALSE);
      OutDs->SetProjection(pszPrettyWkt);
    }
  }

  //pszPrettyWkt实际上是ogc wkt串或者是proj4
  //如果指定投影的，可以在http://spatialreference.org/ 网站中搜索
  //找到所需要的投影后，例如西安80  3度带，中央经线117E，就是EPSG:2384，点开后
  //点击ogc wkt或者proj4，抄下来就行,proj4比较短，而且没有双引号不需要转义，比较简单
  //pszPrettyWkt = "+proj=tmerc +lat_0=0 +lon_0=117 +k=1 +x_0=500000
  //                +y_0=0 +a=6378140 +b=6356755.288157528 +units=m +no_defs";
  //OutDs->SetProjection(pszPrettyWkt);
  //设置坐标
  OutDs->SetGeoTransform(geos);
  CPLFree(pszPrettyWkt);

  //写入数据到outds中
  for(i = 0; i < BandNum; i++) {
    GDALRasterBand *outBand = OutDs->GetRasterBand(i + 1);
    outBand->RasterIO(GF_Write,
                      0,
                      0,
                      WidthAll,
                      HightAll,
                      outBuf[i],
                      WidthAll,
                      HightAll,
                      GDT_Float32,
                      0,
                      0);
  }

  //关闭dataset之后，数据才会真正写入到文件中，否则都是在缓存中！！！
  GDALClose(OutDs);
  GDALClose(ds1);
  GDALClose(ds2);

  //清理环境，删除new的数组、变量等
  for(i = 0; i < BandNum; i++) {
    delete[] InBuf1[i];
    delete[] InBuf2[i];
    delete[] outBuf[i];
  }

  delete[] InBuf1;
  delete[] InBuf2;
  delete[] outBuf;
  getchar();
  return 0;
}
