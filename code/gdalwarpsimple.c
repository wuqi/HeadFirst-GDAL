/**
图像坐标转地理坐标x
    @param geos   变换参数
    @param x      图像X坐标
    @param y      图像Y坐标
    @return       地理X坐标
 */
double Img2CoordX(const double *geos, double x, double y)
{
  return geos[0] + geos[1] * x + geos[2] * y;
}
/**
图像坐标转地理坐标y
    @param geos   变换参数
    @param x      图像X坐标
    @param y      图像Y坐标
    @return       地理Y坐标
 */
double Img2CoordY(const double *geos, double x, double y)
{
  return geos[3] + geos[4] * x + geos[5] * y;
}

/**
    图像重采样
    @param InFile       输入文件
    @param OutFile      输出文件
    @param fResX        x分辨率
    @param fResY        Y分辨率
    @param iResampleAlg 重采样方式,默认为2
                            GRA_NearestNeighbour=0,
                            GRA_Bilinear=1,
                            GRA_Cubic=2,
                            GRA_CubicSpline=3,
                            GRA_Lanczos=4,
                            GRA_Average=5,
                            GRA_Mode=6
    @param pBandIndex   重采样的波段，默认为NULL，全部波段重采样
    @param pnBandCount   重采样波段总数，默认为0 ，全部波段重采样
*/
int Resample(const char *InFile , const char *OutFile, float fResX ,
             float fResY, int iResampleAlg, int *pBandIndex, int pnBandCount)
{
  GDALAllRegister();
  GDALResampleAlg eResample = (GDALResampleAlg)iResampleAlg;
  //打开文件
  GDALDataset *pDSrc = (GDALDataset *)GDALOpen(InFile, GA_ReadOnly);

  if(pDSrc == NULL) {
    return -1;
  }

  GDALDriver *pDriver = GetGDALDriverManager()->GetDriverByName("GTiff");

  if(pDriver == NULL) {
    GDALClose((GDALDatasetH) pDSrc);
    return -2;
  }

  //计算输出投影参数
  int inBandCount = pDSrc->GetRasterCount();
  const char *strWkt = pDSrc->GetProjectionRef();
  GDALDataType dataType = pDSrc->GetRasterBand(1)->GetRasterDataType();
  double geos[6] = {0};
  pDSrc->GetGeoTransform(geos);
  double lefttopx = Img2CoordX(geos, 0, 0);
  double lefttopy = Img2CoordY(geos, 0, 0);
  int newWidth = (int) pDSrc->GetRasterXSize() * geos[1] / fResX;
  int newHeight = (int) fabs(pDSrc->GetRasterYSize() * geos[5] / fResY);
  geos[0] = lefttopx;
  geos[1] = fResX;
  geos[3] = lefttopy;
  geos[5] = -fResY;
  //输入输出波段对应
  int *pSrcBand = NULL;
  int *pDstBand = NULL;
  int iBandSize = 0;

  if(pBandIndex != NULL && pnBandCount != NULL) {
    iBandSize = pnBandCount;
    pSrcBand = new int[iBandSize];
    pDstBand = new int[iBandSize];

    for(int i = 0; i < iBandSize; i++) {
      pSrcBand[i] = pBandIndex[i];
      pDstBand[i] = i + 1;
    }
  } else {
    iBandSize = inBandCount;
    pSrcBand = new int[iBandSize];
    pDstBand = new int[iBandSize];

    for(int i = 0; i < iBandSize; i++) {
      pSrcBand[i] = i + 1;
      pDstBand[i] = i + 1;
    }
  }

  //创建输出文件
  GDALDataset *pDDst = pDriver->Create(OutFile,
                                       newWidth,
                                       newHeight,
                                       iBandSize,
                                       dataType,
                                       NULL);
  pDDst->SetProjection(strWkt);
  pDDst->SetGeoTransform(geos);
  //参数选项
  GDALWarpOptions *psWo = GDALCreateWarpOptions();
  psWo->papszWarpOptions = CSLDuplicate(NULL);
  //数据源
  psWo->eWorkingDataType = dataType;
  psWo->eResampleAlg = eResample;
  psWo->hSrcDS = (GDALDatasetH) pDSrc;
  psWo->hDstDS = (GDALDatasetH) pDDst;
  //波段
  psWo->nBandCount = iBandSize;
  psWo->panSrcBands = (int *) CPLMalloc(iBandSize * sizeof(int));
  psWo->panDstBands = (int *) CPLMalloc(iBandSize * sizeof(int));

  for(int i = 0; i < iBandSize; i++) {
    psWo->panSrcBands[i] = pSrcBand[i];
    psWo->panDstBands[i] = pDstBand[i];
  }

  //变换函数设置
  void *hTransformArg = NULL;
  hTransformArg =  GDALCreateGenImgProjTransformer2(
                     (GDALDatasetH) pDSrc,
                     (GDALDatasetH) pDDst,
                     NULL);
  GDALTransformerFunc pFnTransformer = GDALGenImgProjTransform;
  psWo->pfnTransformer = pFnTransformer;
  psWo->pTransformerArg = hTransformArg;
  //进度条
  psWo->pfnProgress = GDALTermProgress;
  //优化选项
  psWo->dfWarpMemoryLimit = 512000000;
  char **papszWarpOptions = NULL;
  papszWarpOptions = CSLSetNameValue(papszWarpOptions, "NUM_THREADS", "ALL_CPUS");
  papszWarpOptions = CSLSetNameValue(papszWarpOptions, "WRITE_FLUSH", "YES");
  psWo->papszWarpOptions = papszWarpOptions;
  //开始转换
  GDALWarpOperation oWo;

  if(oWo.Initialize(psWo) != CE_None) {
    GDALClose((GDALDatasetH) pDSrc);
    GDALClose((GDALDatasetH) pDDst);
    return -4;
  }

  oWo.ChunkAndWarpMulti(0, 0, newWidth, newHeight);
  //清理环境
  GDALDestroyGenImgProjTransformer(psWo->pTransformerArg);
  GDALDestroyWarpOptions(psWo);
  GDALClose((GDALDatasetH) pDSrc);
  GDALClose((GDALDatasetH) pDDst);
  delete[] pSrcBand;
  delete[] pDstBand;
  return 0;
}
